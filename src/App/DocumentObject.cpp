/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de)          *
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


#include "PreCompiled.h"

#ifndef _PreComp_
#endif

#include <Base/Writer.h>
#include <Base/Tools.h>
#include <Base/Console.h>
#include <Base/Exception.h>

#include "Application.h"
#include "Document.h"
#include "DocumentObject.h"
#include "DocumentObjectGroup.h"
#include "PropertyLinks.h"
#include "PropertyGeo.h"
#include "PropertyExpressionEngine.h"
#include "DocumentObjectExtension.h"
#include "GeoFeatureGroupExtension.h"
#include <App/DocumentObjectPy.h>
#include <boost/bind.hpp>

FC_LOG_LEVEL_INIT("App",true,true)

using namespace App;


PROPERTY_SOURCE(App::DocumentObject, App::TransactionalObject)

DocumentObjectExecReturn *DocumentObject::StdReturn = 0;

//===========================================================================
// DocumentObject
//===========================================================================

DocumentObject::DocumentObject(void)
    : ExpressionEngine(),_pDoc(0),pcNameInDocument(0),_Id(0)
{
    // define Label of type 'Output' to avoid being marked as touched after relabeling
    ADD_PROPERTY_TYPE(Label,("Unnamed"),"Base",Prop_Output,"User name of the object (UTF8)");
    ADD_PROPERTY_TYPE(Label2,(""),"Base",Prop_Hidden,"User description of the object (UTF8)");
    Label2.setStatus(App::Property::Output,true);
    ADD_PROPERTY_TYPE(ExpressionEngine,(),"Base",Prop_Hidden,"Property expressions");

    ADD_PROPERTY(Visibility, (true));

    // default set Visibility status to hidden and output (no touch) for
    // compatibitily reason. We use setStatus instead of PropertyType to 
    // allow user to change its status later
    Visibility.setStatus(Property::Output,true);
    Visibility.setStatus(Property::Hidden,true);
    Visibility.setStatus(Property::NoModify,true);
}

DocumentObject::~DocumentObject(void)
{
    if (!PythonObject.is(Py::_None())){
        Base::PyGILStateLocker lock;
        // Remark: The API of Py::Object has been changed to set whether the wrapper owns the passed
        // Python object or not. In the constructor we forced the wrapper to own the object so we need
        // not to dec'ref the Python object any more.
        // But we must still invalidate the Python object because it need not to be
        // destructed right now because the interpreter can own several references to it.
        Base::PyObjectBase* obj = (Base::PyObjectBase*)PythonObject.ptr();
        // Call before decrementing the reference counter, otherwise a heap error can occur
        obj->setInvalid();
    }
}

App::DocumentObjectExecReturn *DocumentObject::recompute(void)
{
    //check if the links are valid before making the recompute
    if(!GeoFeatureGroupExtension::areLinksValid(this))
#if 1
        Base::Console().Warning("%s: Links go out of the allowed scope\n", getTypeId().getName());
#else
        return new App::DocumentObjectExecReturn("Links go out of the allowed scope", this);
#endif

    // set/unset the execution bit
    Base::ObjectStatusLocker<ObjectStatus, DocumentObject> exe(App::Recompute, this);
    return this->execute();
}

DocumentObjectExecReturn *DocumentObject::execute(void)
{
    //call all extensions
    auto vector = getExtensionsDerivedFromType<App::DocumentObjectExtension>();
    for(auto ext : vector) {
        auto ret = ext->extensionExecute();
        if (ret != StdReturn)
            return ret;
    }

    return StdReturn;
}

bool DocumentObject::recomputeFeature(bool recursive)
{
    Document* doc = this->getDocument();
    if (doc)
        return doc->recomputeFeature(this,recursive);
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
    if(!noRecompute)
        StatusBits.set(ObjectStatus::Enforce);
    StatusBits.set(ObjectStatus::Touch);
    if (_pDoc)
        _pDoc->signalTouchedObject(*this);
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
void DocumentObject::enforceRecompute(void)
{
    touch(false);
}

/**
 * @brief Check whether the document object must be recomputed or not.
 * This means that the 'Enforce' flag is set or that \ref mustExecute()
 * returns a value > 0.
 * @return true if document object must be recomputed, false if not.
 */
bool DocumentObject::mustRecompute(void) const
{
    if (StatusBits.test(ObjectStatus::Enforce))
        return true;

    return mustExecute() > 0;
}

short DocumentObject::mustExecute(void) const
{
    if (ExpressionEngine.isTouched())
        return 1;

    //ask all extensions
    auto vector = getExtensionsDerivedFromType<App::DocumentObjectExtension>();
    for(auto ext : vector) {
        if (ext->extensionMustExecute())
            return 1;
    }

    return 0;
}

const char* DocumentObject::getStatusString(void) const
{
    if (isError()) {
        const char* text = getDocument()->getErrorDescription(this);
        return text ? text : "Error";
    }
    else if (isTouched())
        return "Touched";
    else
        return "Valid";
}

std::string DocumentObject::getFullName() const {
    if(!getDocument() || !pcNameInDocument)
        return "?";
    std::string name(getDocument()->getName());
    name += '#';
    name += *pcNameInDocument;
    return name;
}

const char *DocumentObject::getNameInDocument() const
{
    // Note: It can happen that we query the internal name of an object even if it is not
    // part of a document (anymore). This is the case e.g. if we have a reference in Python
    // to an object that has been removed from the document. In this case we should rather
    // return 0.
    //assert(pcNameInDocument);
    if (!pcNameInDocument) return 0;
    return pcNameInDocument->c_str();
}

int DocumentObject::isExporting() const {
    if(!getDocument() || !getNameInDocument())
        return 0;
    return getDocument()->isExporting(this);
}

std::string DocumentObject::getExportName(bool forced) const {
    if(!pcNameInDocument)
        return std::string();

    if(!forced && !isExporting())
        return *pcNameInDocument;

    // '@' is an invalid character for an internal name, which ensures the
    // following returned name will be unique in any document. Saving external
    // object like that shall only happens in Document::exportObjects(). We
    // shall strip out this '@' and the following document name during restoring.
    return *pcNameInDocument + '@' + getDocument()->getName();
}

bool DocumentObject::isAttachedToDocument() const
{
    return (pcNameInDocument != 0);
}

const char* DocumentObject::detachFromDocument()
{
    const std::string* name = pcNameInDocument;
    pcNameInDocument = 0;
    return name ? name->c_str() : 0;
}

const std::vector<DocumentObject*> &DocumentObject::getOutList() const {
    if(!_outListCached) {
        _outList.clear();
        getOutList(0,_outList);
        _outListCached = true;
    }
    return _outList;
}

std::vector<DocumentObject*> DocumentObject::getOutList(int options) const
{
    std::vector<DocumentObject*> res;
    getOutList(options,res);
    return res;
}

void DocumentObject::getOutList(int options, std::vector<DocumentObject*> &res) const {
    if(_outListCached && !options) {
        res.insert(res.end(),_outList.begin(),_outList.end());
        return;
    }
    std::vector<Property*> props;
    getPropertyList(props);
    bool noHidden = !!(options & OutListNoHidden);
    bool noXLinked = !!(options & OutListNoXLinked);
    for(auto prop : props) {
        auto link = dynamic_cast<PropertyLinkBase*>(prop);
        if(link && (!noXLinked || !PropertyXLink::supportXLink(prop)))
            link->getLinks(res,noHidden);
    }
    if(!(options & OutListNoExpression))
        ExpressionEngine.getLinks(res);
}

std::vector<App::DocumentObject*> DocumentObject::getOutListOfProperty(App::Property* prop) const
{
    std::vector<DocumentObject*> ret;
    if (!prop || prop->getContainer() != this)
        return ret;

    auto link = dynamic_cast<PropertyLinkBase*>(prop);
    if(link)
        link->getLinks(ret);
    return ret;
}

#ifdef USE_OLD_DAG
std::vector<App::DocumentObject*> DocumentObject::getInList(void) const
{
    if (_pDoc)
        return _pDoc->getInList(this);
    else
        return std::vector<App::DocumentObject*>();
}

#else // ifndef USE_OLD_DAG

const std::vector<App::DocumentObject*> &DocumentObject::getInList(void) const
{
    return _inList;
}

#endif // if USE_OLD_DAG


#if 0

void _getInListRecursive(std::set<DocumentObject*>& objSet,
                         const DocumentObject* obj,
                         const DocumentObject* checkObj, int depth)
{
    for (const auto objIt : obj->getInList()) {
        // if the check object is in the recursive inList we have a cycle!
        if (objIt == checkObj || depth <= 0) {
            throw Base::RuntimeError("DocumentObject::getInListRecursive(): cyclic dependency detected!");
        }

        // if the element was already in the set then there is no need to process it again
        auto pair = objSet.insert(objIt);
        if (pair.second)
            _getInListRecursive(objSet, objIt, checkObj, depth-1);
    }
}

std::vector<App::DocumentObject*> DocumentObject::getInListRecursive(void) const
{
    // number of objects in document is a good estimate in result size
    // int maxDepth = getDocument()->countObjects() +2;
    int maxDepth = GetApplication().checkLinkDepth(0);
    std::vector<App::DocumentObject*> result;
    result.reserve(maxDepth);

    // using a rcursie helper to collect all InLists
    _getInListRecursive(result, this, this, maxDepth);

    std::vector<App::DocumentObject*> array;
    array.insert(array.begin(), result.begin(), result.end());
    return array;
}

#else
// The original algorithm is highly inefficient in some special case.
// Considering an object is linked by every other objects. After exculding this
// object, there is another object linked by every other of the remaining
// objects, and so on.  The vector 'result' above will be of magnitude n^2.
// Even if we replace the vector with a set, we still need to visit that amount
// of objects. And this may not be the worst case. getInListEx() has no such
// problem.

std::vector<App::DocumentObject*> DocumentObject::getInListRecursive(void) const {
    std::set<App::DocumentObject*> res;
    getInListEx(res,true);
    return std::vector<App::DocumentObject*>(res.begin(),res.end());
}

#endif

// More efficient algorithm to find the recursive inList of an object,
// including possible external parents.  One shortcoming of this algorithm is
// it does not detect cyclic reference, althgouth it won't crash either.
void DocumentObject::getInListEx(std::set<App::DocumentObject*> &inList, bool recursive) const
{
    std::map<DocumentObject*,std::set<App::DocumentObject*> > outLists;

    // collect all objects and their outLists from all documents.
    for(auto doc : GetApplication().getDocuments()) {
        for(auto obj : doc->getObjects()) {
            if(!obj || !obj->getNameInDocument() || obj==this)
                continue;
            const auto &outList = obj->getOutList();
            outLists[obj].insert(outList.begin(),outList.end());
        }
    }

    std::stack<DocumentObject*> pendings;
    pendings.push(const_cast<DocumentObject*>(this));
    while(pendings.size()) {
        auto obj = pendings.top();
        pendings.pop();
        for(auto &v : outLists) {
            if(v.first == obj) continue;
            auto &outList = v.second;
            // Check the outList to see if the object is there, and pend the
            // object for recrusive check if it's not already in the inList
            if(outList.find(obj)!=outList.end() && 
               inList.insert(v.first).second &&
               recursive)
            {
                pendings.push(v.first);
            }
        }
    }
}

std::set<App::DocumentObject*> DocumentObject::getInListEx(bool recursive) const {
    std::set<App::DocumentObject*> ret;
    getInListEx(ret,recursive);
    return ret;
}

void _getOutListRecursive(std::set<DocumentObject*>& objSet,
                          const DocumentObject* obj,
                          const DocumentObject* checkObj, int depth)
{
    for (const auto objIt : obj->getOutList()) {
        // if the check object is in the recursive inList we have a cycle!
        if (objIt == checkObj || depth <= 0) {
            throw Base::RuntimeError("DocumentObject::getOutListRecursive(): cyclic dependency detected!");
        }

        // if the element was already in the set then there is no need to process it again
        auto pair = objSet.insert(objIt);
        if (pair.second)
            _getOutListRecursive(objSet, objIt, checkObj, depth-1);
    }
}

std::vector<App::DocumentObject*> DocumentObject::getOutListRecursive(void) const
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
bool _isInInListRecursive(const DocumentObject* act,
                          const DocumentObject* checkObj, int depth)
{
#ifndef  USE_OLD_DAG
    for (auto obj : act->getInList()) {
        if (obj == checkObj)
            return true;
        // if we reach the depth limit we have a cycle!
        if (depth <= 0) {
            throw Base::RuntimeError("DocumentObject::isInInListRecursive(): cyclic dependency detected!");
        }

        if (_isInInListRecursive(obj, checkObj, depth - 1))
            return true;
    }
#else
    (void)act;
    (void)checkObj;
    (void)depth;
#endif

    return false;
}

bool DocumentObject::isInInListRecursive(DocumentObject *linkTo) const
{
#if 0
    int maxDepth = getDocument()->countObjects() + 2;
    return _isInInListRecursive(this, linkTo, maxDepth);
#else
    return this==linkTo || getInListEx(true).count(linkTo);
#endif
}

bool DocumentObject::isInInList(DocumentObject *linkTo) const
{
#ifndef  USE_OLD_DAG
    if (std::find(_inList.begin(), _inList.end(), linkTo) != _inList.end())
        return true;
    else
        return false;
#else
    (void)linkTo;
    return false;
#endif
}

// helper for isInOutListRecursive()
bool _isInOutListRecursive(const DocumentObject* act,
                           const DocumentObject* checkObj, int depth)
{
#ifndef  USE_OLD_DAG
    for (auto obj : act->getOutList()) {
        if (obj == checkObj)
            return true;
        // if we reach the depth limit we have a cycle!
        if (depth <= 0) {
            throw Base::RuntimeError("DocumentObject::isInOutListRecursive(): cyclic dependency detected!");
        }

        if (_isInOutListRecursive(obj, checkObj, depth - 1))
            return true;
    }
#else
    (void)act;
    (void)checkObj;
    (void)depth;
#endif

    return false;
}

bool DocumentObject::isInOutListRecursive(DocumentObject *linkTo) const
{
    int maxDepth = getDocument()->countObjects() + 2;
    return _isInOutListRecursive(this, linkTo, maxDepth);
}

std::vector<std::list<App::DocumentObject*> >
DocumentObject::getPathsByOutList(App::DocumentObject* to) const
{
    return _pDoc->getPathsByOutList(this, to);
}

DocumentObjectGroup* DocumentObject::getGroup() const
{
    return dynamic_cast<DocumentObjectGroup*>(GroupExtension::getGroupOfObject(this));
}

bool DocumentObject::testIfLinkDAGCompatible(DocumentObject *linkTo) const
{
    std::vector<App::DocumentObject*> linkTo_in_vector;
    linkTo_in_vector.push_back(linkTo);
    return this->testIfLinkDAGCompatible(linkTo_in_vector);
}

bool DocumentObject::testIfLinkDAGCompatible(const std::vector<DocumentObject *> &linksTo) const
{
#if 0
    Document* doc = this->getDocument();
    if (!doc)
        throw Base::RuntimeError("DocumentObject::testIfLinkIsDAG: object is not in any document.");
    std::vector<App::DocumentObject*> deplist = doc->getDependencyList(linksTo);
    if( std::find(deplist.begin(),deplist.end(),this) != deplist.end() )
        //found this in dependency list
        return false;
    else
        return true;
#else
    auto inLists = getInListEx(true);
    inLists.emplace(const_cast<DocumentObject*>(this));
    for(auto obj : linksTo)
        if(inLists.count(obj))
            return false;
    return true;
#endif
}

bool DocumentObject::testIfLinkDAGCompatible(PropertyLinkSubList &linksTo) const
{
    const std::vector<App::DocumentObject*> &linksTo_in_vector = linksTo.getValues();
    return this->testIfLinkDAGCompatible(linksTo_in_vector);
}

bool DocumentObject::testIfLinkDAGCompatible(PropertyLinkSub &linkTo) const
{
    std::vector<App::DocumentObject*> linkTo_in_vector;
    linkTo_in_vector.reserve(1);
    linkTo_in_vector.push_back(linkTo.getValue());
    return this->testIfLinkDAGCompatible(linkTo_in_vector);
}

void DocumentObject::onLostLinkToObject(DocumentObject*)
{

}

App::Document *DocumentObject::getDocument(void) const
{
    return _pDoc;
}

void DocumentObject::setDocument(App::Document* doc)
{
    _pDoc=doc;
    onSettingDocument();
}

void DocumentObject::onAboutToRemoveProperty(const char* prop)
{
    if (_pDoc) {
        _pDoc->removePropertyOfObject(this, prop);
        clearOutListCache();
    }
}

void DocumentObject::onBeforeChange(const Property* prop)
{
    // Store current name in oldLabel, to be able to easily retrieve old name of document object later
    // when renaming expressions.
    if (prop == &Label)
        oldLabel = Label.getStrValue();

    if (_pDoc)
        onBeforeChangeProperty(_pDoc, prop);

    signalBeforeChange(*this,*prop);
}

/// get called by the container when a Property was changed
void DocumentObject::onChanged(const Property* prop)
{
    if(!GetApplication().isRestoring() && 
       prop && !prop->testStatus(Property::PartialTrigger) &&
       getDocument() && 
       getDocument()->testStatus(Document::PartialDoc))
    {
        static App::Document *warnedDoc;
        if(warnedDoc != getDocument()) {
            warnedDoc = getDocument();
            FC_WARN("Changes to partial loaded document will not be saved: "
                    << getFullName() << '.' << prop->getName());
        }
    }

    // Delay signaling view provider until the document object has handled the
    // change
    // if (_pDoc)
    //     _pDoc->onChangedProperty(this,prop);

    if (prop == &Label && _pDoc && oldLabel != Label.getStrValue())
        _pDoc->signalRelabelObject(*this);

    // set object touched if it is an input property
    if (!testStatus(ObjectStatus::NoTouch) 
            && !(prop->getType() & Prop_Output) 
            && !prop->testStatus(Property::Output)) 
    {
        if(!StatusBits.test(ObjectStatus::Touch)) {
            FC_LOG("touch '" << getFullName() << "' on change of '" << prop->getName() << "'");
            StatusBits.set(ObjectStatus::Touch);
        }
        // must execute on document recompute
        if (!(prop->getType() & Prop_NoRecompute))
            StatusBits.set(ObjectStatus::Enforce);
    }

    //call the parent for appropriate handling
    TransactionalObject::onChanged(prop);

    // Now signal the view provider
    if (_pDoc)
        _pDoc->onChangedProperty(this,prop);

    signalChanged(*this,*prop);
}

void DocumentObject::clearOutListCache() const {
    _outList.clear();
    _outListMap.clear();
    _outListCached = false;
}

PyObject *DocumentObject::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DocumentObjectPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

DocumentObject *DocumentObject::getSubObject(const char *subname,
        PyObject **pyObj, Base::Matrix4D *mat, bool transform, int depth) const
{
    DocumentObject *ret = 0;
    auto exts = getExtensionsDerivedFromType<App::DocumentObjectExtension>();
    for(auto ext : exts) {
        if(ext->extensionGetSubObject(ret,subname,pyObj,mat,transform, depth))
            return ret;
    }

    std::string name;
    const char *dot=0;
    if(!subname || !(dot=strchr(subname,'.'))) {
        ret = const_cast<DocumentObject*>(this);
    }else if(subname[0]=='$') {
        name = std::string(subname+1,dot);
        for(auto obj : getOutList()) {
            if(name == obj->Label.getValue()) {
                ret = obj;
                break;
            }
        }
    }else{
        name = std::string(subname,dot);
        const auto &outList = getOutList();
        if(outList.size()!=_outListMap.size()) {
            _outListMap.clear();
            for(auto obj : outList)
                _outListMap[obj->getNameInDocument()] = obj;
        }
        auto it = _outListMap.find(name.c_str());
        if(it != _outListMap.end())
            ret = it->second;
    }

    // TODO: By right, normal object's placement does not transform its sub
    // objects (think of the claimed children of a Fusion). But I do think we
    // should change that.
    if(transform && mat) {
        auto pla = dynamic_cast<PropertyPlacement*>(getPropertyByName("Placement"));
        if(pla)
            *mat *= pla->getValue().toMatrix();
    }

    if(ret && dot)
        return ret->getSubObject(dot+1,pyObj,mat,true,depth+1);
    return ret;
}

std::vector<std::string> DocumentObject::getSubObjects(int reason) const {
    std::vector<std::string> ret;
    auto exts = getExtensionsDerivedFromType<App::DocumentObjectExtension>();
    for(auto ext : exts) {
        if(ext->extensionGetSubObjects(ret,reason))
            return ret;
    }
    return ret;
}

std::vector<std::pair<App::DocumentObject *,std::string> > DocumentObject::getParents(int depth) const {
    std::vector<std::pair<App::DocumentObject *,std::string> > ret;
    if(!getNameInDocument())
        return ret;
    std::map<std::string, std::pair<App::DocumentObject*, std::string> > map;
    GetApplication().checkLinkDepth(depth);
    std::string name(getNameInDocument());
    name += ".";
    for(auto parent : getInList()) {
        if(!parent || !parent->getNameInDocument())
            continue;
        if(!parent->hasChildElement() && 
           !parent->hasExtension(GeoFeatureGroupExtension::getExtensionClassTypeId()))
            continue;
        if(!parent->getSubObject(name.c_str()))
            continue;

        auto links = GetApplication().getLinksTo(parent,App::GetLinkRecursive);
        links.insert(parent);
        for(auto parent : links) {
            auto parents = parent->getParents(depth+1);
            if(parents.empty()) 
                parents.emplace_back(parent,std::string());
            for(auto &v : parents) {
                std::ostringstream ss;
                if(v.first->getDocument()==getDocument())
                    ss << '#' << v.first->getNameInDocument();
                else
                    ss << v.first->getFullName();
                ss << '.' << name;
                map.emplace(ss.str(),std::make_pair(v.first,v.second+name));
            }
        }
    }
    ret.reserve(map.size());
    for(auto &v : map)
        ret.push_back(std::move(v.second));
    return ret;
}

DocumentObject *DocumentObject::getLinkedObject(
        bool recursive, Base::Matrix4D *mat, bool transform, int depth) const 
{
    DocumentObject *ret = 0;
    auto exts = getExtensionsDerivedFromType<App::DocumentObjectExtension>();
    for(auto ext : exts) {
        if(ext->extensionGetLinkedObject(ret,recursive,mat,transform,depth))
            return ret;
    }
    if(transform && mat) {
        auto pla = dynamic_cast<PropertyPlacement*>(getPropertyByName("Placement"));
        if(pla)
            *mat *= pla->getValue().toMatrix();
    }
    return const_cast<DocumentObject*>(this);
}

void DocumentObject::Save (Base::Writer &writer) const
{
    if (this->getNameInDocument())
        writer.ObjectName = this->getNameInDocument();
    App::ExtensionContainer::Save(writer);
}

/**
 * @brief Associate the expression \expr with the object identifier \a path in this document object.
 * @param path Target object identifier for the result of the expression
 * @param expr Expression tree
 */

void DocumentObject::setExpression(const ObjectIdentifier &path, boost::shared_ptr<Expression> expr)
{
    ExpressionEngine.setValue(path, expr);
}

/**
 * @brief Get expression information associated with \a path.
 * @param path Object identifier
 * @return Expression info, containing expression and optional comment.
 */

const PropertyExpressionEngine::ExpressionInfo DocumentObject::getExpression(const ObjectIdentifier &path) const
{
    App::any value = ExpressionEngine.getPathValue(path);

    if (value.type() == typeid(PropertyExpressionEngine::ExpressionInfo))
        return App::any_cast<PropertyExpressionEngine::ExpressionInfo>(value);
    else
        return PropertyExpressionEngine::ExpressionInfo();
}

/**
 * @brief Invoke ExpressionEngine's renameObjectIdentifier, to possibly rewrite expressions using
 * the \a paths map with current and new identifiers.
 *
 * @param paths
 */

void DocumentObject::renameObjectIdentifiers(const std::map<ObjectIdentifier, ObjectIdentifier> &paths)
{
    ExpressionEngine.renameObjectIdentifiers(paths);
}

void DocumentObject::onDocumentRestored()
{
    //call all extensions
    auto vector = getExtensionsDerivedFromType<App::DocumentObjectExtension>();
    for(auto ext : vector)
        ext->onExtendedDocumentRestored();
    if(Visibility.testStatus(Property::Output))
        Visibility.setStatus(Property::NoModify,true);
}

void DocumentObject::onSettingDocument()
{
    //call all extensions
    auto vector = getExtensionsDerivedFromType<App::DocumentObjectExtension>();
    for(auto ext : vector)
        ext->onExtendedSettingDocument();
}

void DocumentObject::setupObject()
{
    //call all extensions
    auto vector = getExtensionsDerivedFromType<App::DocumentObjectExtension>();
    for(auto ext : vector)
        ext->onExtendedSetupObject();
}

void DocumentObject::unsetupObject()
{
    //call all extensions
    auto vector = getExtensionsDerivedFromType<App::DocumentObjectExtension>();
    for(auto ext : vector)
        ext->onExtendedUnsetupObject();
}

void App::DocumentObject::_removeBackLink(DocumentObject* rmvObj)
{
#ifndef USE_OLD_DAG
    //do not use erase-remove idom, as this erases ALL entries that match. we only want to remove a
    //single one.
    auto it = std::find(_inList.begin(), _inList.end(), rmvObj);
    if(it != _inList.end())
        _inList.erase(it);
#else
    (void)rmvObj;
#endif
}

void App::DocumentObject::_addBackLink(DocumentObject* newObj)
{
#ifndef USE_OLD_DAG
    //we need to add all links, even if they are available multiple times. The reason for this is the
    //removal: If a link loses this object it removes the backlink. If we would have added it only once
    //this removal would clear the object from the inlist, even though there may be other link properties 
    //from this object that link to us.
    _inList.push_back(newObj);
#else
    (void)newObj;
#endif //USE_OLD_DAG    
}

int DocumentObject::setElementVisible(const char *element, bool visible) {
    for(auto ext : getExtensionsDerivedFromType<DocumentObjectExtension>()) {
        int ret = ext->extensionSetElementVisible(element,visible);
        if(ret>=0) return ret;
    }

    return -1;
}

int DocumentObject::isElementVisible(const char *element) const {
    for(auto ext : getExtensionsDerivedFromType<DocumentObjectExtension>()) {
        int ret = ext->extensionIsElementVisible(element);
        if(ret>=0) return ret;
    }

    return -1;
}

bool DocumentObject::hasChildElement() const {
    for(auto ext : getExtensionsDerivedFromType<DocumentObjectExtension>()) {
        if(ext->extensionHasChildElement())
            return true;
    }
    return false;
}

DocumentObject *DocumentObject::resolve(const char *subname, 
        App::DocumentObject **parent, std::string *childName, const char **subElement, 
        PyObject **pyObj, Base::Matrix4D *pmat, bool transform, int depth) const
{
    auto self = const_cast<DocumentObject*>(this);
    if(parent) *parent = 0;
    if(subElement) *subElement = 0;

    auto obj = getSubObject(subname,pyObj,pmat,transform,depth);
    if(!obj || !subname || *subname==0)
        return self;

    if(!parent && !subElement)
        return obj;

    // NOTE, the convension of '.' separated SubName demands a mandatory ending
    // '.' for each object name in SubName, even if there is no subelement
    // following it. So finding the last dot will give us the end of the last
    // object name.
    const char *dot=0;
    if(Data::ComplexGeoData::isMappedElement(subname) ||
       !(dot=strrchr(subname,'.')) ||
       dot == subname) 
    {
        if(subElement)
            *subElement = dot?dot+1:subname;
        return obj; // this means no parent object reference in SubName
    }

    if(parent)
        *parent = self;

    bool elementMapChecked = false;
    const char *lastDot = dot;
    for(--dot;;--dot) {
        // check for the second last dot, which is the end of the last parent object
        if(*dot == '.' || dot == subname) {
            // We can't get parent object by its name, because the object may be
            // externally linked (i.e. in a different document). So go through
            // getSubObject again.
            if(!elementMapChecked) {
                elementMapChecked = true;
                const char *sub = dot==subname?dot:dot+1;
                if(Data::ComplexGeoData::isMappedElement(sub)) {
                    lastDot = dot;
                    if(dot==subname) 
                        break;
                    else
                        continue;
                }
            }
            if(dot==subname)
                break;
            auto sobj = getSubObject(std::string(subname,dot-subname+1).c_str());
            if(sobj!=obj) {
                if(parent) {
                    // Link/LinkGroup has special visiblility handling of plain
                    // group, so keep ascending
                    if(!sobj->hasExtension(GroupExtension::getExtensionClassTypeId(),false)) {
                        *parent = sobj;
                        break;
                    }
                    for(auto ddot=dot-1;ddot!=subname;--ddot) {
                        if(*ddot != '.') continue;
                        auto sobj = getSubObject(std::string(subname,ddot-subname+1).c_str());
                        if(!sobj->hasExtension(GroupExtension::getExtensionClassTypeId(),false)) {
                            *parent = sobj;
                            break;
                        }
                    }
                }
                break;
            }
        }
    }
    if(childName && lastDot!=dot) {
        if(*dot == '.')
            ++dot;
        const char *nextDot = strchr(dot,'.');
        assert(nextDot);
        *childName = std::string(dot,nextDot-dot);
    }
    if(subElement)
        *subElement = *lastDot=='.'?lastDot+1:lastDot;
    return obj;
}

DocumentObject *DocumentObject::resolveRelativeLink(std::string &subname,
        DocumentObject *&link, std::string &linkSub) const
{
    if(!link || !link->getNameInDocument() || !getNameInDocument())
        return 0;
    auto ret = const_cast<DocumentObject*>(this);
    if(link != ret) {
        auto sub = subname.c_str();
        auto nextsub = sub;
        for(auto dot=strchr(nextsub,'.');dot;nextsub=dot+1,dot=strchr(nextsub,'.')) {
            std::string subcheck(sub,nextsub-sub);
            subcheck += link->getNameInDocument();
            subcheck += '.';
            if(getSubObject(subcheck.c_str())==link) {
                ret = getSubObject(std::string(sub,dot+1-sub).c_str());
                if(!ret) 
                    return 0;
                subname = std::string(dot+1);
                break;
            }
        }
        return ret;
    }

    size_t pos=0,linkPos=0;
    std::string linkssub,ssub;
    do {
        linkPos = linkSub.find('.',linkPos);
        if(linkPos == std::string::npos) {
            link = 0;
            return 0;
        }
        ++linkPos;
        pos = subname.find('.',pos);
        if(pos == std::string::npos) {
            subname.clear();
            ret = 0;
            break;
        }
        ++pos;
    }while(subname.compare(0,pos,linkSub,0,linkPos)==0);

    if(pos != std::string::npos) {
        ret = getSubObject(subname.substr(0,pos).c_str());
        if(!ret) {
            link = 0;
            return 0;
        }
        subname = subname.substr(pos);
    }
    if(linkPos) {
        link = link->getSubObject(linkSub.substr(0,linkPos).c_str());
        if(!link)
            return 0;
        linkSub = linkSub.substr(linkPos);
    }
    return ret;
}

bool DocumentObject::adjustRelativeLinks(
        const std::set<App::DocumentObject *> &inList,
        std::set<App::DocumentObject *> *visited)
{
    if(visited)
        visited->insert(this);

    bool touched = false;
    std::vector<Property*> props;
    getPropertyList(props);
    for(auto prop : props) {
        auto linkProp = dynamic_cast<PropertyLinkBase*>(prop);
        if(linkProp && linkProp->adjustLink(inList))
            touched = true;
    }
    if(!touched)
        return false;

    if(visited) {
        for(auto obj : getOutList()) {
            if(!visited->count(obj)) 
                obj->adjustRelativeLinks(inList,visited);
        }
    }
    return true;
}

std::string DocumentObject::getElementMapVersion(const App::Property *_prop, bool restored) const {
    auto prop = dynamic_cast<const PropertyComplexGeoData*>(_prop);
    if(!prop) 
        return std::string();
    return prop->getElementMapVersion(restored);
}

const std::string &DocumentObject::hiddenMarker() {
    static std::string marker("!hide");
    return marker;
}

const char *DocumentObject::hasHiddenMarker(const char *subname) {
    if(!subname) return 0;
    const char *marker = strrchr(subname,'.');
    if(!marker)
        marker = subname;
    else
        ++marker;
    return hiddenMarker()==marker?marker:0;
}

bool DocumentObject::redirectSubName(std::ostringstream &, DocumentObject *, DocumentObject *) const {
    return false;
}

