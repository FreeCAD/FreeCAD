/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include <stack>
#include <memory>
#include <map>
#include <set>
#include <vector>
#include <string>

#include <Base/Console.h>
#include <Base/Matrix.h>
#include <Base/Placement.h>
#include <Base/Tools.h>
#include <Base/Writer.h>

#include "Expression.h"
#include "Application.h"
#include "ElementNamingUtils.h"
#include "Document.h"
#include "DocumentObject.h"
#include "DocumentObjectPy.h"
#include "DocumentObjectExtension.h"
#include "DocumentObjectGroup.h"
#include "GeoFeatureGroupExtension.h"
#include "Link.h"
#include "ObjectIdentifier.h"
#include "PropertyExpressionEngine.h"
#include "PropertyLinks.h"


FC_LOG_LEVEL_INIT("App", true, true)

using namespace App;


PROPERTY_SOURCE(App::DocumentObject, App::TransactionalObject)

DocumentObjectExecReturn* DocumentObject::StdReturn = nullptr;

//===========================================================================
// DocumentObject
//===========================================================================

DocumentObject::DocumentObject()
    : ExpressionEngine()
{
    // define Label of type 'Output' to avoid being marked as touched after relabeling
    ADD_PROPERTY_TYPE(Label, ("Unnamed"), "Base", Prop_Output, "User name of the object (UTF8)");
    ADD_PROPERTY_TYPE(Label2, (""), "Base", Prop_Hidden, "User description of the object (UTF8)");
    Label2.setStatus(App::Property::Output, true);
    ADD_PROPERTY_TYPE(ExpressionEngine, (), "Base", Prop_Hidden, "Property expressions");

    ADD_PROPERTY(Visibility, (true));

    // default set Visibility status to hidden and output (no touch) for
    // compatibitily reason. We use setStatus instead of PropertyType to
    // allow user to change its status later
    Visibility.setStatus(Property::Output, true);
    Visibility.setStatus(Property::Hidden, true);
    Visibility.setStatus(Property::NoModify, true);
}

DocumentObject::~DocumentObject()
{
    if (!PythonObject.is(Py::_None())) {
        Base::PyGILStateLocker lock;
        // Remark: The API of Py::Object has been changed to set whether the wrapper owns the passed
        // Python object or not. In the constructor we forced the wrapper to own the object so we
        // need not to dec'ref the Python object any more. But we must still invalidate the Python
        // object because it need not to be destructed right now because the interpreter can own
        // several references to it.
        Base::PyObjectBase* obj = static_cast<Base::PyObjectBase*>(PythonObject.ptr());
        // Call before decrementing the reference counter, otherwise a heap error can occur
        obj->setInvalid();
    }
}

void DocumentObject::printInvalidLinks() const
{
    try {
        // Get objects that have invalid link scope, and print their names.
        // Truncate the invalid object list name strings for readability, if they happen to be very
        // long.
        std::vector<App::DocumentObject*> invalid_linkobjs;
        std::string objnames, scopenames;
        GeoFeatureGroupExtension::getInvalidLinkObjects(this, invalid_linkobjs);
        for (auto& obj : invalid_linkobjs) {
            objnames += obj->getNameInDocument();
            objnames += " ";
            for (auto& scope : obj->getParents()) {
                if (scopenames.length() > 80) {
                    scopenames += "... ";
                    break;
                }

                scopenames += scope.first->getNameInDocument();
                scopenames += " ";
            }

            if (objnames.length() > 80) {
                objnames += "... ";
                break;
            }
        }

        if (objnames.empty()) {
            objnames = "N/A";
        }
        else {
            objnames.pop_back();
        }

        if (scopenames.empty()) {
            scopenames = "N/A";
        }
        else {
            scopenames.pop_back();
        }

        Base::Console().warning("%s: Link(s) to object(s) '%s' go out of the allowed scope '%s'. "
                                "Instead, the linked object(s) reside within '%s'.\n",
                                getTypeId().getName(),
                                objnames.c_str(),
                                getNameInDocument(),
                                scopenames.c_str());
    }
    catch (const Base::Exception& e) {
        e.reportException();
    }
}

App::DocumentObjectExecReturn* DocumentObject::recompute()
{
    // check if the links are valid before making the recompute
    if (!GeoFeatureGroupExtension::areLinksValid(this)) {
        printInvalidLinks();
    }

    // set/unset the execution bit
    Base::ObjectStatusLocker<ObjectStatus, DocumentObject> exe(App::Recompute, this);

    // mark the object to recompute its extensions
    this->setStatus(App::RecomputeExtension, true);

    auto ret = this->execute();
    if (ret == StdReturn) {
        // most feature classes don't call the execute() method of its base class
        // so execute the extensions now
        if (this->testStatus(App::RecomputeExtension)) {
            ret = executeExtensions();
        }
    }

    return ret;
}

DocumentObjectExecReturn* DocumentObject::execute()
{
    return executeExtensions();
}

App::DocumentObjectExecReturn* DocumentObject::executeExtensions()
{
    // execute extensions but stop on error
    this->setStatus(App::RecomputeExtension, false);  // reset the flag
    auto vector = getExtensionsDerivedFromType<App::DocumentObjectExtension>();
    for (auto ext : vector) {
        auto ret = ext->extensionExecute();
        if (ret != StdReturn) {
            return ret;
        }
    }

    return StdReturn;
}

bool DocumentObject::recomputeFeature(bool recursive)
{
    Document* doc = this->getDocument();
    if (doc) {
        return doc->recomputeFeature(this, recursive);
    }
    return isValid();
}

/**
 * @brief Set this document object touched.
 * Touching a document object does not mean to recompute it, it only means that
 * other document objects that link it (i.e. its InList) will be recomputed.
 * If it should be forced to recompute a document object then use
 * \ref enforceRecompute() instead.
 */
void DocumentObject::touch(bool noRecompute)
{
    if (!noRecompute) {
        StatusBits.set(ObjectStatus::Enforce);
    }
    StatusBits.set(ObjectStatus::Touch);
    if (_pDoc) {
        _pDoc->signalTouchedObject(*this);
    }
}

/**
 * @brief Set this document object freezed.
 * A freezed document object does not recompute ever.
 */
void DocumentObject::freeze()
{
    StatusBits.set(ObjectStatus::Freeze);

    // store read-only property names
    this->readOnlyProperties.clear();
    std::vector<std::pair<const char*, Property*>> list;
    static_cast<App::PropertyContainer*>(this)->getPropertyNamedList(list);
    for (auto pair: list){
        if (pair.second->isReadOnly()){
            this->readOnlyProperties.push_back(pair.first);
        } else {
            pair.second->setReadOnly(true);
        }
    }

    // use the signalTouchedObject to refresh the Gui
    if (_pDoc) {
        _pDoc->signalTouchedObject(*this);
    }
}

/**
 * @brief Set this document object unfreezed.
 * A freezed document object does not recompute ever.
 */
void DocumentObject::unfreeze(bool noRecompute)
{
    StatusBits.reset(ObjectStatus::Freeze);

    // reset read-only property status
    std::vector<std::pair<const char*, Property*>> list;
    static_cast<App::PropertyContainer*>(this)->getPropertyNamedList(list);

    for (auto pair: list){
        if (! std::count(readOnlyProperties.begin(), readOnlyProperties.end(), pair.first)){
            pair.second->setReadOnly(false);
        }
    }

    touch(noRecompute);
}

/**
 * @brief Check whether the document object is touched or not.
 * @return true if document object is touched, false if not.
 */
bool DocumentObject::isTouched() const
{
    return ExpressionEngine.isTouched() || StatusBits.test(ObjectStatus::Touch);
}

/**
 * @brief Enforces this document object to be recomputed.
 * This can be useful to recompute the feature without
 * having to change one of its input properties.
 */
void DocumentObject::enforceRecompute()
{
    touch(false);
}

/**
 * @brief Check whether the document object must be recomputed or not.
 * This means that the 'Enforce' flag is set or that \ref mustExecute()
 * returns a value > 0.
 * @return true if document object must be recomputed, false if not.
 */
bool DocumentObject::mustRecompute() const
{
    if (StatusBits.test(ObjectStatus::Freeze)) {
        return false;
    }

    if (StatusBits.test(ObjectStatus::Enforce)) {
        return true;
    }

    return mustExecute() > 0;
}

short DocumentObject::mustExecute() const
{
    if (ExpressionEngine.isTouched()) {
        return 1;
    }

    // ask all extensions
    auto vector = getExtensionsDerivedFromType<App::DocumentObjectExtension>();
    for (auto ext : vector) {
        if (ext->extensionMustExecute()) {
            return 1;
        }
    }

    return 0;
}

const char* DocumentObject::getStatusString() const
{
    if (isError()) {
        const char* text = getDocument()->getErrorDescription(this);
        return text ? text : "Error";
    }
    else if (isFreezed()){
        return "Freezed";
    }
    else if (isTouched()) {
        return "Touched";
    }
    else {
        return "Valid";
    }
}

std::string DocumentObject::getFullName() const
{
    if (!getDocument() || !isAttachedToDocument()) {
        return "?";
    }
    std::string name(getDocument()->getName());
    name += '#';
    name += *pcNameInDocument;
    return name;
}

std::string DocumentObject::getFullLabel() const
{
    if (!getDocument()) {
        return "?";
    }

    auto name = getDocument()->Label.getStrValue();
    name += "#";
    name += Label.getStrValue();
    return name;
}

const char* DocumentObject::getDagKey() const
{
    if (!pcNameInDocument) {
        return nullptr;
    }
    return pcNameInDocument->c_str();
}

const char* DocumentObject::getNameInDocument() const
{
    // Note: It can happen that we query the internal name of an object even if it is not
    // part of a document (anymore). This is the case e.g. if we have a reference in Python
    // to an object that has been removed from the document. In this case we should rather
    // return 0.
    // assert(pcNameInDocument);
    if (!pcNameInDocument) {
        return nullptr;
    }
    return pcNameInDocument->c_str();
}

int DocumentObject::isExporting() const
{
    if (!getDocument() || !isAttachedToDocument()) {
        return 0;
    }
    return getDocument()->isExporting(this);
}

std::string DocumentObject::getExportName(bool forced) const
{
    if (!isAttachedToDocument()) {
        return {};
    }

    if (!forced && !isExporting()) {
        return *pcNameInDocument;
    }

    // '@' is an invalid character for an internal name, which ensures the
    // following returned name will be unique in any document. Saving external
    // object like that shall only happens in Document::exportObjects(). We
    // shall strip out this '@' and the following document name during restoring.
    return *pcNameInDocument + '@' + getDocument()->getName();
}

bool DocumentObject::isAttachedToDocument() const
{
    return (pcNameInDocument != nullptr);
}

const char* DocumentObject::detachFromDocument()
{
    const std::string* name = pcNameInDocument;
    pcNameInDocument = nullptr;
    return name ? name->c_str() : nullptr;
}

const std::vector<DocumentObject*>& DocumentObject::getOutList() const
{
    if (!_outListCached) {
        _outList.clear();
        getOutList(0, _outList);
        _outListCached = true;
    }
    return _outList;
}

std::vector<DocumentObject*> DocumentObject::getOutList(int options) const
{
    std::vector<DocumentObject*> res;
    getOutList(options, res);
    return res;
}

void DocumentObject::getOutList(int options, std::vector<DocumentObject*>& res) const
{
    if (_outListCached && !options) {
        res.insert(res.end(), _outList.begin(), _outList.end());
        return;
    }
    std::vector<Property*> props;
    getPropertyList(props);
    bool noHidden = !!(options & OutListNoHidden);
    std::size_t size = res.size();
    for (auto prop : props) {
        auto link = freecad_cast<PropertyLinkBase*>(prop);
        if (link) {
            link->getLinks(res, noHidden);
        }
    }
    if (!(options & OutListNoExpression)) {
        ExpressionEngine.getLinks(res);
    }

    if (options & OutListNoXLinked) {
        for (auto it = res.begin() + size; it != res.end();) {
            auto obj = *it;
            if (obj && obj->getDocument() != getDocument()) {
                it = res.erase(it);
            }
            else {
                ++it;
            }
        }
    }
}

std::vector<App::DocumentObject*> DocumentObject::getOutListOfProperty(App::Property* prop) const
{
    std::vector<DocumentObject*> ret;
    if (!prop || prop->getContainer() != this) {
        return ret;
    }

    auto link = freecad_cast<PropertyLinkBase*>(prop);
    if (link) {
        link->getLinks(ret);
    }
    return ret;
}

const std::vector<App::DocumentObject*>& DocumentObject::getInList() const
{
    return _inList;
}

// The original algorithm is highly inefficient in some special case.
// Considering an object is linked by every other objects. After excluding this
// object, there is another object linked by every other of the remaining
// objects, and so on.  The vector 'result' above will be of magnitude n^2.
// Even if we replace the vector with a set, we still need to visit that amount
// of objects. And this may not be the worst case. getInListEx() has no such
// problem.

std::vector<App::DocumentObject*> DocumentObject::getInListRecursive() const
{
    std::set<App::DocumentObject*> inSet;
    std::vector<App::DocumentObject*> res;
    getInListEx(inSet, true, &res);
    return res;
}


// More efficient algorithm to find the recursive inList of an object,
// including possible external parents.  One shortcoming of this algorithm is
// it does not detect cyclic reference, althgouth it won't crash either.
void DocumentObject::getInListEx(std::set<App::DocumentObject*>& inSet,
                                 bool recursive,
                                 std::vector<App::DocumentObject*>* inList) const
{
    if (!recursive) {
        inSet.insert(_inList.begin(), _inList.end());
        if (inList) {
            *inList = _inList;
        }
        return;
    }

    std::stack<DocumentObject*> pendings;
    pendings.push(const_cast<DocumentObject*>(this));
    while (!pendings.empty()) {
        auto obj = pendings.top();
        pendings.pop();
        for (auto o : obj->getInList()) {
            if (o && o->isAttachedToDocument() && inSet.insert(o).second) {
                pendings.push(o);
                if (inList) {
                    inList->push_back(o);
                }
            }
        }
    }
}

std::set<App::DocumentObject*> DocumentObject::getInListEx(bool recursive) const
{
    std::set<App::DocumentObject*> ret;
    getInListEx(ret, recursive);
    return ret;
}

void _getOutListRecursive(std::set<DocumentObject*>& objSet,
                          const DocumentObject* obj,
                          const DocumentObject* checkObj,
                          int depth)
{
    for (const auto objIt : obj->getOutList()) {
        // if the check object is in the recursive inList we have a cycle!
        if (objIt == checkObj || depth <= 0) {
            throw Base::BadGraphError(
                "DocumentObject::getOutListRecursive(): cyclic dependency detected!");
        }

        // if the element was already in the set then there is no need to process it again
        auto pair = objSet.insert(objIt);
        if (pair.second) {
            _getOutListRecursive(objSet, objIt, checkObj, depth - 1);
        }
    }
}

std::vector<App::DocumentObject*> DocumentObject::getOutListRecursive() const
{
    // number of objects in document is a good estimate in result size
    int maxDepth = GetApplication().checkLinkDepth(0);
    std::set<App::DocumentObject*> result;

    // using a recursive helper to collect all OutLists
    _getOutListRecursive(result, this, this, maxDepth);

    std::vector<App::DocumentObject*> array;
    array.insert(array.begin(), result.begin(), result.end());
    return array;
}

// helper for isInInListRecursive()
bool _isInInListRecursive(const DocumentObject* act, const DocumentObject* checkObj, int depth)
{
    for (auto obj : act->getInList()) {
        if (obj == checkObj) {
            return true;
        }
        // if we reach the depth limit we have a cycle!
        if (depth <= 0) {
            throw Base::BadGraphError(
                "DocumentObject::isInInListRecursive(): cyclic dependency detected!");
        }

        if (_isInInListRecursive(obj, checkObj, depth - 1)) {
            return true;
        }
    }

    return false;
}

bool DocumentObject::isInInListRecursive(DocumentObject* linkTo) const
{
    return this == linkTo || getInListEx(true).contains(linkTo);
}

bool DocumentObject::isInInList(DocumentObject* linkTo) const
{
    if (std::ranges::find(_inList, linkTo) != _inList.end()) {
        return true;
    }
    else {
        return false;
    }
}

// helper for isInOutListRecursive()
bool _isInOutListRecursive(const DocumentObject* act, const DocumentObject* checkObj, int depth)
{
    for (auto obj : act->getOutList()) {
        if (obj == checkObj) {
            return true;
        }
        // if we reach the depth limit we have a cycle!
        if (depth <= 0) {
            throw Base::BadGraphError(
                "DocumentObject::isInOutListRecursive(): cyclic dependency detected!");
        }

        if (_isInOutListRecursive(obj, checkObj, depth - 1)) {
            return true;
        }
    }

    return false;
}

bool DocumentObject::isInOutListRecursive(DocumentObject* linkTo) const
{
    int maxDepth = getDocument()->countObjects() + 2;
    return _isInOutListRecursive(this, linkTo, maxDepth);
}

std::vector<std::list<App::DocumentObject*>>
DocumentObject::getPathsByOutList(App::DocumentObject* to) const
{
    return _pDoc->getPathsByOutList(this, to);
}

DocumentObjectGroup* DocumentObject::getGroup() const
{
    return freecad_cast<DocumentObjectGroup*>(GroupExtension::getGroupOfObject(this));
}

bool DocumentObject::testIfLinkDAGCompatible(DocumentObject* linkTo) const
{
    std::vector<App::DocumentObject*> linkTo_in_vector;
    linkTo_in_vector.push_back(linkTo);
    return this->testIfLinkDAGCompatible(linkTo_in_vector);
}

bool DocumentObject::testIfLinkDAGCompatible(const std::vector<DocumentObject*>& linksTo) const
{
    auto inLists = getInListEx(true);
    inLists.emplace(const_cast<DocumentObject*>(this));
    for (auto obj : linksTo) {
        if (inLists.contains(obj)) {
            return false;
        }
    }
    return true;
}

bool DocumentObject::testIfLinkDAGCompatible(PropertyLinkSubList& linksTo) const
{
    const std::vector<App::DocumentObject*>& linksTo_in_vector = linksTo.getValues();
    return this->testIfLinkDAGCompatible(linksTo_in_vector);
}

bool DocumentObject::testIfLinkDAGCompatible(PropertyLinkSub& linkTo) const
{
    std::vector<App::DocumentObject*> linkTo_in_vector;
    linkTo_in_vector.reserve(1);
    linkTo_in_vector.push_back(linkTo.getValue());
    return this->testIfLinkDAGCompatible(linkTo_in_vector);
}

void DocumentObject::onLostLinkToObject(DocumentObject*)
{}

App::Document* DocumentObject::getDocument() const
{
    return _pDoc;
}

void DocumentObject::setDocument(App::Document* doc)
{
    _pDoc = doc;
    onSettingDocument();
}

bool DocumentObject::removeDynamicProperty(const char* name)
{
    if (!_pDoc || testStatus(ObjectStatus::Destroy)) {
        return false;
    }

    Property* prop = getDynamicPropertyByName(name);
    if (!prop || prop->testStatus(App::Property::LockDynamic)) {
        return false;
    }

    if (prop->isDerivedFrom<PropertyLinkBase>()) {
        clearOutListCache();
    }

    _pDoc->addOrRemovePropertyOfObject(this, prop, false);

    auto expressions = ExpressionEngine.getExpressions();
    std::vector<App::ObjectIdentifier> removeExpr;

    for (const auto& it : expressions) {
        if (it.first.getProperty() == prop) {
            removeExpr.push_back(it.first);
        }
    }

    for (const auto& it : removeExpr) {
        ExpressionEngine.setValue(it, std::shared_ptr<Expression>());
    }

    return TransactionalObject::removeDynamicProperty(name);
}

bool DocumentObject::renameDynamicProperty(Property* prop, const char* name)
{
    std::string oldName = prop->getName();

    auto expressions = ExpressionEngine.getExpressions();
    std::vector<std::shared_ptr<Expression>> expressionsToMove;
    std::vector<App::ObjectIdentifier> idsWithExprsToRemove;

    for (const auto& [id, expr] : expressions) {
        if (id.getProperty() == prop) {
            idsWithExprsToRemove.push_back(id);
            expressionsToMove.emplace_back(expr->copy());
        }
    }

    for (const auto& it : idsWithExprsToRemove) {
        ExpressionEngine.setValue(it, std::shared_ptr<Expression>());
    }

    bool renamed = TransactionalObject::renameDynamicProperty(prop, name);
    if (renamed && _pDoc) {
        _pDoc->renamePropertyOfObject(this, prop, oldName.c_str());
    }


    App::ObjectIdentifier idNewProp(prop->getContainer(), std::string(name));
    for (auto& exprToMove : expressionsToMove) {
        ExpressionEngine.setValue(idNewProp, exprToMove);
    }

    return renamed;
}

App::Property* DocumentObject::addDynamicProperty(const char* type,
                                                  const char* name,
                                                  const char* group,
                                                  const char* doc,
                                                  short attr,
                                                  bool ro,
                                                  bool hidden)
{
    auto prop = TransactionalObject::addDynamicProperty(type, name, group, doc, attr, ro, hidden);
    if (prop && _pDoc) {
        _pDoc->addOrRemovePropertyOfObject(this, prop, true);
    }
    return prop;
}

void DocumentObject::onBeforeChange(const Property* prop)
{
    if (isFreezed() && prop != &Visibility) {
        return;
    }

    // Store current name in oldLabel, to be able to easily retrieve old name of document object later
    // when renaming expressions.
    if (prop == &Label)
        oldLabel = Label.getStrValue();

    if (_pDoc){
        onBeforeChangeProperty(_pDoc, prop);
    }

    signalBeforeChange(*this, *prop);
}

std::vector<std::pair<Property*, std::unique_ptr<Property>>>
DocumentObject::onProposedLabelChange(std::string& newLabel)
{
    // Note that this work can't be done in onBeforeChangeLabel because FeaturePython overrides this
    // method and does not initially base-call it.

    // We re only called if the new label differs from the old one, and our code to check duplicates
    // may not work if this is not the case.
    std::string oldLabel = Label.getStrValue();
    assert(newLabel != oldLabel);
    if (!isAttachedToDocument()) {
        return {};
    }
    App::Document* doc = getDocument();
    if (doc->isPerformingTransaction()
        || (doc->testStatus(App::Document::Restoring)
            && !doc->testStatus(App::Document::Importing))) {
        return {};
    }
    static ParameterGrp::handle _hPGrp;
    if (!_hPGrp) {
        _hPGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document");
    }
    if (doc && !newLabel.empty() && !_hPGrp->GetBool("DuplicateLabels") && !allowDuplicateLabel()
        && doc->containsLabel(newLabel)) {
        // The label already exists but settings are such that duplicate labels should not be assigned.
        std::string objName = getNameInDocument();
        if (!doc->containsLabel(objName) && doc->haveSameBaseName(objName, newLabel)) {
            // The object name is not already a Label and the base name of the proposed label
            // equals the base name of the object Name, so we use the object Name as the replacement Label.
            newLabel = objName;
        }
        else {
            // Otherwise we generate a unique Label using newLabel as a prototype name. In doing so,
            // we must also act as if the current value of the property is not an existing Label
            // entry.
            // We deregister the old label so it does not interfere with making the new label,
            // and re-register it after. This is probably a bit less efficient that having a special
            // make-unique-label-as-if-this-one-did-not-exist method, but such a method would be a real
            // ugly wart.
            doc->unregisterLabel(oldLabel);
            newLabel = doc->makeUniqueLabel(newLabel);
            doc->registerLabel(oldLabel);
        }
    }

    // Despite our efforts to make a unique label, onBeforeLabelChange can change it.
    onBeforeChangeLabel(newLabel);

    if (oldLabel == newLabel || getDocument()->testStatus(App::Document::Restoring)) {
        // Don't update label reference if we are restoring or if the label is unchanged.
        // When importing (which also counts as restoring), it is possible the
        // new object changes its label. However, we cannot update label
        // references here, because object being restored is not based on
        // dependency order. It can only be done in afterRestore().
        //
        // See PropertyLinkBase::restoreLabelReference() for more details.
        return {};
    }
    return PropertyLinkBase::updateLabelReferences(this, newLabel.c_str());
}

void DocumentObject::onEarlyChange(const Property* prop)
{
    if (isFreezed() && prop != &Visibility) {
        return;
    }

    if (GetApplication().isClosingAll()) {
        return;
    }

    if (!GetApplication().isRestoring() && !prop->testStatus(Property::PartialTrigger)
        && getDocument() && getDocument()->testStatus(Document::PartialDoc)) {
        static App::Document* warnedDoc;
        if (warnedDoc != getDocument()) {
            warnedDoc = getDocument();
            FC_WARN("Changes to partial loaded document will not be saved: " << getFullName() << '.'
                                                                             << prop->getName());
        }
    }

    signalEarlyChanged(*this, *prop);
}

/// get called by the container when a Property was changed
void DocumentObject::onChanged(const Property* prop)
{
    if (prop == &Label && _pDoc && _pDoc->containsObject(this) && oldLabel != Label.getStrValue()) {
        _pDoc->unregisterLabel(oldLabel);
        _pDoc->registerLabel(Label.getStrValue());
    }

    if (isFreezed() && prop != &Visibility) {
        return;
    }

    if (GetApplication().isClosingAll()) {
        return;
    }

    if (!GetApplication().isRestoring() && !prop->testStatus(Property::PartialTrigger)
        && getDocument() && getDocument()->testStatus(Document::PartialDoc)) {
        static App::Document* warnedDoc;
        if (warnedDoc != getDocument()) {
            warnedDoc = getDocument();
            FC_WARN("Changes to partial loaded document will not be saved: " << getFullName() << '.'
                                                                             << prop->getName());
        }
    }

    // Delay signaling view provider until the document object has handled the
    // change
    // if (_pDoc)
    //     _pDoc->onChangedProperty(this,prop);

    if (prop == &Label && _pDoc && oldLabel != Label.getStrValue()) {
        _pDoc->signalRelabelObject(*this);
    }

    // set object touched if it is an input property
    if (!testStatus(ObjectStatus::NoTouch) && !(prop->getType() & Prop_Output)
        && !prop->testStatus(Property::Output)) {
        if (!StatusBits.test(ObjectStatus::Touch)) {
            FC_TRACE("touch '" << getFullName() << "' on change of '" << prop->getName() << "'");
            StatusBits.set(ObjectStatus::Touch);
        }
        // must execute on document recompute
        if (!(prop->getType() & Prop_NoRecompute)) {
            StatusBits.set(ObjectStatus::Enforce);
        }
    }

    // call the parent for appropriate handling
    TransactionalObject::onChanged(prop);

    // Now signal the view provider
    if (_pDoc) {
        _pDoc->onChangedProperty(this, prop);
    }

    signalChanged(*this, *prop);
}

void DocumentObject::clearOutListCache() const
{
    _outList.clear();
    _outListMap.clear();
    _outListCached = false;
}

PyObject* DocumentObject::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DocumentObjectPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

DocumentObject* DocumentObject::getSubObject(const char* subname,
                                             PyObject** pyObj,
                                             Base::Matrix4D* mat,
                                             bool transform,
                                             int depth) const
{
    DocumentObject* ret = nullptr;
    auto exts = getExtensionsDerivedFromType<App::DocumentObjectExtension>();
    for (auto ext : exts) {
        if (ext->extensionGetSubObject(ret, subname, pyObj, mat, transform, depth)) {
            return ret;
        }
    }

    std::string name;
    const char* dot = nullptr;
    if (!subname || !(dot = strchr(subname, '.'))) {
        ret = const_cast<DocumentObject*>(this);
    }
    else if (subname[0] == '$') {
        name = std::string(subname + 1, dot);
        for (auto obj : getOutList()) {
            if (name == obj->Label.getValue()) {
                ret = obj;
                break;
            }
        }
    }
    else {
        name = std::string(subname, dot);
        const auto& outList = getOutList();
        if (outList.size() != _outListMap.size()) {
            _outListMap.clear();
            for (auto obj : outList) {
                _outListMap[obj->getDagKey()] = obj;
            }
        }
        auto it = _outListMap.find(name.c_str());
        if (it != _outListMap.end()) {
            ret = it->second;
        }
    }

    // TODO: By right, normal object's placement does not transform its sub
    // objects (think of the claimed children of a Fusion). But I do think we
    // should change that.
    if (transform && mat) {
        auto pla = freecad_cast<PropertyPlacement*>(getPropertyByName("Placement"));
        if (pla) {
            *mat *= pla->getValue().toMatrix();
        }
    }

    if (ret && dot) {
        return ret->getSubObject(dot + 1, pyObj, mat, true, depth + 1);
    }
    return ret;
}

namespace
{
std::vector<DocumentObject*>
getSubObjectListFlatten(const std::vector<App::DocumentObject*>& resNotFlatten,
                        std::vector<int>* const subsizes,
                        const App::DocumentObject* sobj,
                        App::DocumentObject** container,
                        bool& lastChild)
{
    auto res {resNotFlatten};
    auto linked = sobj->getLinkedObject();
    if (*container) {
        auto grp = App::GeoFeatureGroupExtension::getGroupOfObject(linked);
        if (grp != *container) {
            *container = nullptr;
        }
        else {
            if (lastChild && !res.empty()) {
                res.pop_back();
                if (subsizes) {
                    subsizes->pop_back();
                }
            }
            lastChild = true;
        }
    }
    if (linked->getExtensionByType<App::GeoFeatureGroupExtension>(true)) {
        *container = linked;
        lastChild = false;
    }
    else if (linked != sobj || sobj->hasChildElement()) {
        // Check for Link or LinkGroup
        *container = nullptr;
    }
    else if (auto ext = sobj->getExtensionByType<LinkBaseExtension>(true)) {
        // check for Link array
        if (ext->getElementCountValue() != 0) {
            *container = nullptr;
        }
    }
    return res;
}
}  // namespace

std::vector<DocumentObject*> DocumentObject::getSubObjectList(const char* subname,
                                                              std::vector<int>* const subsizes,
                                                              bool flatten) const
{
    std::vector<DocumentObject*> res;
    res.push_back(const_cast<DocumentObject*>(this));
    if (subsizes) {
        subsizes->push_back(0);
    }
    if (!subname || (subname[0] == '\0')) {
        return res;
    }
    auto element = Data::findElementName(subname);
    std::string sub(subname, element - subname);
    App::DocumentObject* container = nullptr;

    bool lastChild = false;
    if (flatten) {
        auto linked = getLinkedObject();
        if (linked->getExtensionByType<App::GeoFeatureGroupExtension>(true)) {
            container = const_cast<DocumentObject*>(this);
        }
        else if (auto grp = App::GeoFeatureGroupExtension::getGroupOfObject(linked)) {
            container = grp;
            lastChild = true;
        }
    }
    for (auto pos = sub.find('.'); pos != std::string::npos; pos = sub.find('.', pos + 1)) {
        char subTail = sub[pos + 1];
        sub[pos + 1] = '\0';
        auto sobj = getSubObject(sub.c_str());
        if (!sobj || !sobj->isAttachedToDocument()) {
            continue;
        }

        if (flatten) {
            res = getSubObjectListFlatten(res, subsizes, sobj, &container, lastChild);
        }
        res.push_back(sobj);
        if (subsizes) {
            subsizes->push_back((int)pos + 1);
        }
        sub[pos + 1] = subTail;
    }
    return res;
}

std::vector<std::string> DocumentObject::getSubObjects(int reason) const
{
    std::vector<std::string> ret;
    auto exts = getExtensionsDerivedFromType<App::DocumentObjectExtension>();
    for (auto ext : exts) {
        if (ext->extensionGetSubObjects(ret, reason)) {
            return ret;
        }
    }
    return ret;
}

std::vector<std::pair<App::DocumentObject*, std::string>>
DocumentObject::getParents(int depth) const
{
    std::vector<std::pair<App::DocumentObject*, std::string>> ret;
    if (!isAttachedToDocument() || !GetApplication().checkLinkDepth(depth, MessageOption::Throw)) {
        return ret;
    }

    std::string name(getNameInDocument());
    name += ".";
    for (auto parent : getInList()) {
        if (!parent || !parent->isAttachedToDocument()) {
            continue;
        }

        if (!parent->hasChildElement()
            && !parent->hasExtension(GeoFeatureGroupExtension::getExtensionClassTypeId())) {
            continue;
        }

        if (!parent->getSubObject(name.c_str())) {
            continue;
        }

        auto links = GetApplication().getLinksTo(parent, App::GetLinkRecursive);
        links.insert(parent);

        for (auto parent : links) {
            auto parents = parent->getParents(depth + 1);
            if (parents.empty()) {
                parents.emplace_back(parent, std::string());
            }

            for (auto& v : parents) {
                ret.emplace_back(v.first, v.second + name);
            }
        }
    }

    return ret;
}

App::DocumentObject* DocumentObject::getFirstParent() const
{
    for (auto obj : getInList()) {
        if (obj->hasExtension(App::GroupExtension::getExtensionClassTypeId(), true)) {
            return obj;
        }
    }

    return nullptr;
}

DocumentObject* DocumentObject::getLinkedObject(bool recursive,
                                                Base::Matrix4D* mat,
                                                bool transform,
                                                int depth) const
{
    DocumentObject* ret = nullptr;
    auto exts = getExtensionsDerivedFromType<App::DocumentObjectExtension>();
    for (auto ext : exts) {
        if (ext->extensionGetLinkedObject(ret, recursive, mat, transform, depth)) {
            return ret;
        }
    }
    if (transform && mat) {
        auto pla = freecad_cast<PropertyPlacement*>(getPropertyByName("Placement"));
        if (pla) {
            *mat *= pla->getValue().toMatrix();
        }
    }
    return const_cast<DocumentObject*>(this);
}

void DocumentObject::Save(Base::Writer& writer) const
{
    if (this->isFreezed()) {
        throw Base::AbortException("At least one object is frozen, unable to save.");
    }

    if (this->isAttachedToDocument()){
        writer.ObjectName = this->getNameInDocument();
    }
    App::ExtensionContainer::Save(writer);
}

/**
 * @brief Associate the expression \expr with the object identifier \a path in this document object.
 * @param path Target object identifier for the result of the expression
 * @param expr Expression tree
 */

void DocumentObject::setExpression(const ObjectIdentifier& path, std::shared_ptr<Expression> expr)
{
    ExpressionEngine.setValue(path, std::move(expr));
}

/**
 * @brief Clear the expression of the object identifier \a path in this document object.
 * @param path Target object identifier
 */

void DocumentObject::clearExpression(const ObjectIdentifier& path)
{
    setExpression(path, std::shared_ptr<Expression>());
}

/**
 * @brief Get expression information associated with \a path.
 * @param path Object identifier
 * @return Expression info, containing expression and optional comment.
 */

const PropertyExpressionEngine::ExpressionInfo
DocumentObject::getExpression(const ObjectIdentifier& path) const
{
    boost::any value = ExpressionEngine.getPathValue(path);

    if (value.type() == typeid(PropertyExpressionEngine::ExpressionInfo)) {
        return boost::any_cast<PropertyExpressionEngine::ExpressionInfo>(value);
    }
    else {
        return PropertyExpressionEngine::ExpressionInfo();
    }
}

/**
 * @brief Invoke ExpressionEngine's renameObjectIdentifier, to possibly rewrite expressions using
 * the \a paths map with current and new identifiers.
 *
 * @param paths
 */

void DocumentObject::renameObjectIdentifiers(
    const std::map<ObjectIdentifier, ObjectIdentifier>& paths)
{
    ExpressionEngine.renameObjectIdentifiers(paths);
}

void DocumentObject::onDocumentRestored()
{
    // call all extensions
    auto vector = getExtensionsDerivedFromType<App::DocumentObjectExtension>();
    for (auto ext : vector) {
        ext->onExtendedDocumentRestored();
    }
    if (Visibility.testStatus(Property::Output)) {
        Visibility.setStatus(Property::NoModify, true);
    }
}

void DocumentObject::restoreFinished()
{
    // some link type property cannot restore link information until other
    // objects has been restored. For example, PropertyExpressionEngine and
    // PropertySheet with expression containing label reference.
    // So on document load they are handled in Document::afterRestore, but if the user
    // use dumpContent and restoreContent then they need to be handled here.
    std::vector<App::Property*> props;
    getPropertyList(props);
    for (auto prop : props) {
        prop->afterRestore();
    }
}

void DocumentObject::onUndoRedoFinished()
{}

void DocumentObject::onSettingDocument()
{
    // call all extensions
    auto vector = getExtensionsDerivedFromType<App::DocumentObjectExtension>();
    for (auto ext : vector) {
        ext->onExtendedSettingDocument();
    }
}

void DocumentObject::setupObject()
{
    // call all extensions
    auto vector = getExtensionsDerivedFromType<App::DocumentObjectExtension>();
    for (auto ext : vector) {
        ext->onExtendedSetupObject();
    }
}

void DocumentObject::unsetupObject()
{
    // call all extensions
    auto vector = getExtensionsDerivedFromType<App::DocumentObjectExtension>();
    for (auto ext : vector) {
        ext->onExtendedUnsetupObject();
    }
}

void App::DocumentObject::_removeBackLink(DocumentObject* rmvObj)
{
    // do not use erase-remove idom, as this erases ALL entries that match. we only want to remove a
    // single one.
    auto it = std::ranges::find(_inList, rmvObj);
    if (it != _inList.end()) {
        _inList.erase(it);
    }
}

void App::DocumentObject::_addBackLink(DocumentObject* newObj)
{
    // we need to add all links, even if they are available multiple times. The reason for this is
    // the removal: If a link loses this object it removes the backlink. If we would have added it
    // only once this removal would clear the object from the inlist, even though there may be other
    // link properties from this object that link to us.
    _inList.push_back(newObj);
}

int DocumentObject::setElementVisible(const char* element, bool visible)
{
    for (auto ext : getExtensionsDerivedFromType<DocumentObjectExtension>()) {
        int ret = ext->extensionSetElementVisible(element, visible);
        if (ret >= 0) {
            return ret;
        }
    }

    return -1;
}

int DocumentObject::isElementVisible(const char* element) const
{
    for (auto ext : getExtensionsDerivedFromType<DocumentObjectExtension>()) {
        int ret = ext->extensionIsElementVisible(element);
        if (ret >= 0) {
            return ret;
        }
    }

    return -1;
}

bool DocumentObject::hasChildElement() const
{
    for (auto ext : getExtensionsDerivedFromType<DocumentObjectExtension>()) {
        if (ext->extensionHasChildElement()) {
            return true;
        }
    }
    return false;
}

DocumentObject* DocumentObject::resolve(const char* subname,
                                        App::DocumentObject** parent,
                                        std::string* childName,
                                        const char** subElement,
                                        PyObject** pyObj,
                                        Base::Matrix4D* pmat,
                                        bool transform,
                                        int depth) const
{
    auto self = const_cast<DocumentObject*>(this);
    if (parent) {
        *parent = nullptr;
    }
    if (subElement) {
        *subElement = nullptr;
    }

    auto obj = getSubObject(subname, pyObj, pmat, transform, depth);
    if (!obj || !subname || *subname == 0) {
        return self;
    }

    if (!parent && !subElement) {
        return obj;
    }

    // NOTE, the convention of '.' separated SubName demands a mandatory ending
    // '.' for each object name in SubName, even if there is no subelement
    // following it. So finding the last dot will give us the end of the last
    // object name.
    const char* dot = nullptr;
    if (Data::isMappedElement(subname) || !(dot = strrchr(subname, '.')) || dot == subname) {
        if (subElement) {
            *subElement = dot ? dot + 1 : subname;
        }
        return obj;  // this means no parent object reference in SubName
    }

    if (parent) {
        *parent = self;
    }

    bool elementMapChecked = false;
    const char* lastDot = dot;
    for (--dot;; --dot) {
        // check for the second last dot, which is the end of the last parent object
        if (*dot == '.' || dot == subname) {
            // We can't get parent object by its name, because the object may be
            // externally linked (i.e. in a different document). So go through
            // getSubObject again.
            if (!elementMapChecked) {
                elementMapChecked = true;
                const char* sub = dot == subname ? dot : dot + 1;
                if (Data::isMappedElement(sub)) {
                    lastDot = dot;
                    if (dot == subname) {
                        break;
                    }
                    else {
                        continue;
                    }
                }
            }
            if (dot == subname) {
                break;
            }
            auto sobj = getSubObject(std::string(subname, dot - subname + 1).c_str());
            if (sobj != obj) {
                if (parent) {
                    // Link/LinkGroup has special visibility handling of plain
                    // group, so keep ascending
                    if (!sobj->hasExtension(GroupExtension::getExtensionClassTypeId(), false)) {
                        *parent = sobj;
                        break;
                    }
                    for (auto ddot = dot - 1; ddot != subname; --ddot) {
                        if (*ddot != '.') {
                            continue;
                        }
                        auto sobj = getSubObject(std::string(subname, ddot - subname + 1).c_str());
                        if (!sobj->hasExtension(GroupExtension::getExtensionClassTypeId(), false)) {
                            *parent = sobj;
                            break;
                        }
                    }
                }
                break;
            }
        }
    }
    if (childName && lastDot != dot) {
        if (*dot == '.') {
            ++dot;
        }
        const char* nextDot = strchr(dot, '.');
        assert(nextDot);
        *childName = std::string(dot, nextDot - dot);
    }
    if (subElement) {
        *subElement = *lastDot == '.' ? lastDot + 1 : lastDot;
    }
    return obj;
}

DocumentObject* DocumentObject::resolveRelativeLink(std::string& subname,
                                                    DocumentObject*& link,
                                                    std::string& linkSub) const
{
    if (!link || !link->isAttachedToDocument() || !isAttachedToDocument()) {
        return nullptr;
    }
    auto ret = const_cast<DocumentObject*>(this);
    if (link != ret) {
        auto sub = subname.c_str();
        auto nextsub = sub;
        for (auto dot = strchr(nextsub, '.'); dot; nextsub = dot + 1, dot = strchr(nextsub, '.')) {
            std::string subcheck(sub, nextsub - sub);
            subcheck += link->getNameInDocument();
            subcheck += '.';
            if (getSubObject(subcheck.c_str()) == link) {
                ret = getSubObject(std::string(sub, dot + 1 - sub).c_str());
                if (!ret) {
                    return nullptr;
                }
                subname = std::string(dot + 1);
                break;
            }
        }
        return ret;
    }

    size_t pos = 0, linkPos = 0;
    std::string linkssub, ssub;
    do {
        linkPos = linkSub.find('.', linkPos);
        if (linkPos == std::string::npos) {
            link = nullptr;
            return nullptr;
        }
        ++linkPos;
        pos = subname.find('.', pos);
        if (pos == std::string::npos) {
            subname.clear();
            ret = nullptr;
            break;
        }
        ++pos;
    } while (subname.compare(0, pos, linkSub, 0, linkPos) == 0);

    if (pos != std::string::npos) {
        ret = getSubObject(subname.substr(0, pos).c_str());
        if (!ret) {
            link = nullptr;
            return nullptr;
        }
        subname = subname.substr(pos);
    }
    if (linkPos) {
        link = link->getSubObject(linkSub.substr(0, linkPos).c_str());
        if (!link) {
            return nullptr;
        }
        linkSub = linkSub.substr(linkPos);
    }
    return ret;
}

bool DocumentObject::adjustRelativeLinks(const std::set<App::DocumentObject*>& inList,
                                         std::set<App::DocumentObject*>* visited)
{
    if (visited) {
        visited->insert(this);
    }

    bool touched = false;
    std::vector<Property*> props;
    getPropertyList(props);
    for (auto prop : props) {
        auto linkProp = freecad_cast<PropertyLinkBase*>(prop);
        if (linkProp && linkProp->adjustLink(inList)) {
            touched = true;
        }
    }
    if (visited) {
        for (auto obj : getOutList()) {
            if (!visited->count(obj)) {
                if (obj->adjustRelativeLinks(inList, visited)) {
                    touched = true;
                }
            }
        }
    }
    return touched;
}

std::string DocumentObject::getElementMapVersion(const App::Property* _prop, bool restored) const
{
    auto prop = freecad_cast<const PropertyComplexGeoData*>(_prop);
    if (!prop) {
        return std::string();
    }
    return prop->getElementMapVersion(restored);
}

bool DocumentObject::checkElementMapVersion(const App::Property* _prop, const char* ver) const
{
    auto prop = freecad_cast<const PropertyComplexGeoData*>(_prop);
    if (!prop) {
        return false;
    }
    return prop->checkElementMapVersion(ver);
}

const std::string& DocumentObject::hiddenMarker()
{
    static std::string marker("!hide");
    return marker;
}

const char* DocumentObject::hasHiddenMarker(const char* subname)
{
    if (!subname) {
        return nullptr;
    }
    const char* marker = strrchr(subname, '.');
    if (!marker) {
        marker = subname;
    }
    else {
        ++marker;
    }
    return hiddenMarker() == marker ? marker : nullptr;
}

bool DocumentObject::redirectSubName(std::ostringstream&, DocumentObject*, DocumentObject*) const
{
    return false;
}

void DocumentObject::onPropertyStatusChanged(const Property& prop, unsigned long oldStatus)
{
    (void)oldStatus;
    if (!Document::isAnyRestoring() && isAttachedToDocument() && getDocument()) {
        getDocument()->signalChangePropertyEditor(*getDocument(), prop);
    }
}

Base::Placement DocumentObject::getPlacementOf(const std::string& sub, DocumentObject* targetObj)
{
    Base::Placement plc;
    auto* propPlacement = freecad_cast<App::PropertyPlacement*>(getPropertyByName("Placement"));
    if (propPlacement) {
        // If the object has no placement (like a Group), plc stays identity so we can proceed.
        plc = propPlacement->getValue();
    }

    std::vector<std::string> names = Base::Tools::splitSubName(sub);

    if (names.empty() || this == targetObj) {
        return plc;
    }

    DocumentObject* subObj = getDocument()->getObject(names.front().c_str());

    if (!subObj) {
        return plc;
    }

    std::vector<std::string> newNames(names.begin() + 1, names.end());
    std::string newSub = Base::Tools::joinList(newNames, ".");

    return plc * subObj->getPlacementOf(newSub, targetObj);
}

Base::Placement DocumentObject::getPlacement() const
{
    Base::Placement plc;
    if (auto* prop = getPlacementProperty()) {
        plc = prop->getValue();
    }
    return plc;
}

App::PropertyPlacement* DocumentObject::getPlacementProperty() const
{
    if (auto linkExtension = getExtensionByType<App::LinkBaseExtension>(true)) {
        if (auto linkPlacementProp = linkExtension->getLinkPlacementProperty()) {
            return linkPlacementProp;
        }

        return linkExtension->getPlacementProperty();
    }

    return getPropertyByName<App::PropertyPlacement>("Placement");
}


