/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
    #include <utility>
#endif

#include <boost/algorithm/string/predicate.hpp>
#include <CXX/Extensions.hxx>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include "Application.h"
#include "Document.h"
#include "DocumentObject.h"
#include "DocumentObjectPy.h"
#include "DocumentObserver.h"
#include "ObjectIdentifier.h"
#include "PropertyLinkSub.h"

FC_LOG_LEVEL_INIT("PropertyLinkSub", true, true)

namespace App {

//**************************************************************************
// PropertyLinkSub
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyLinkSub, App::PropertyLinkBase)
TYPESYSTEM_SOURCE(App::PropertyLinkSubChild, App::PropertyLinkSub)
TYPESYSTEM_SOURCE(App::PropertyLinkSubGlobal, App::PropertyLinkSub)
TYPESYSTEM_SOURCE(App::PropertyLinkSubHidden, App::PropertyLinkSub)

//**************************************************************************
// Construction/Destruction


PropertyLinkSub::PropertyLinkSub() = default;

PropertyLinkSub::~PropertyLinkSub()
{
    // in case this property is dynamically removed
#ifndef USE_OLD_DAG
    if (_pcLinkSub && getContainer()
        && getContainer()->isDerivedFrom<App::DocumentObject>()) {
        App::DocumentObject* parent = static_cast<DocumentObject*>(getContainer());
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy) && _pcScope != LinkScope::Hidden) {
            if (_pcLinkSub) {
                _pcLinkSub->_removeBackLink(parent);
            }
        }
    }
#endif
}

void PropertyLinkSub::setSyncSubObject(bool enable)
{
    _Flags.set((std::size_t)LinkSyncSubObject, enable);
}

void PropertyLinkSub::setValue(App::DocumentObject* lValue,
                               const std::vector<std::string>& SubList,
                               std::vector<ShadowSub>&& shadows)
{
    setValue(lValue, std::vector<std::string>(SubList), std::move(shadows));
}

void PropertyLinkSub::setValue(App::DocumentObject* lValue,
                               std::vector<std::string>&& subs,
                               std::vector<ShadowSub>&& shadows)
{
    auto parent = Base::freecad_dynamic_cast<App::DocumentObject>(getContainer());
    if (lValue) {
        if (!lValue->isAttachedToDocument()) {
            throw Base::ValueError("PropertyLinkSub: invalid document object");
        }
        if (!testFlag(LinkAllowExternal) && parent
            && parent->getDocument() != lValue->getDocument()) {
            throw Base::ValueError("PropertyLinkSub does not support external object");
        }
    }
    aboutToSetValue();
#ifndef USE_OLD_DAG
    if (parent) {
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy) && _pcScope != LinkScope::Hidden) {
            if (_pcLinkSub) {
                _pcLinkSub->_removeBackLink(parent);
            }
            if (lValue) {
                lValue->_addBackLink(parent);
            }
        }
    }
#endif
    _pcLinkSub = lValue;
    _cSubList = std::move(subs);
    if (shadows.size() == _cSubList.size()) {
        _ShadowSubList = std::move(shadows);
        onContainerRestored();  // re-register element references
    }
    else {
        updateElementReference(nullptr);
    }
    checkLabelReferences(_cSubList);
    hasSetValue();
}

App::DocumentObject* PropertyLinkSub::getValue() const
{
    return _pcLinkSub;
}

const std::vector<std::string>& PropertyLinkSub::getSubValues() const
{
    return _cSubList;
}

const std::string& getSubNameWithStyle(const std::string& subName,
                                       const PropertyLinkBase::ShadowSub& shadow,
                                       bool newStyle,
                                       std::string& tmp)
{
    if (!newStyle) {
        if (!shadow.oldName.empty()) {
            return shadow.oldName;
        }
    }
    else if (!shadow.newName.empty()) {
        if (Data::hasMissingElement(shadow.oldName.c_str())) {
            auto pos = shadow.newName.rfind('.');
            if (pos != std::string::npos) {
                tmp = shadow.newName.substr(0, pos + 1);
                tmp += shadow.oldName;
                return tmp;
            }
        }
        return shadow.newName;
    }
    return subName;
}

std::vector<std::string> PropertyLinkSub::getSubValues(bool newStyle) const
{
    assert(_cSubList.size() == _ShadowSubList.size());
    std::vector<std::string> ret;
    ret.reserve(_cSubList.size());
    std::string tmp;
    for (size_t i = 0; i < _ShadowSubList.size(); ++i) {
        ret.push_back(getSubNameWithStyle(_cSubList[i], _ShadowSubList[i], newStyle, tmp));
    }
    return ret;
}

std::vector<std::string> PropertyLinkSub::getSubValuesStartsWith(const char* starter,
                                                                 bool newStyle) const
{
    assert(_cSubList.size() == _ShadowSubList.size());
    std::vector<std::string> ret;
    std::string tmp;
    for (size_t i = 0; i < _ShadowSubList.size(); ++i) {
        const auto& sub = getSubNameWithStyle(_cSubList[i], _ShadowSubList[i], newStyle, tmp);
        auto element = Data::findElementName(sub.c_str());
        if (element && boost::starts_with(element, starter)) {
            ret.emplace_back(element);
        }
    }
    return ret;
}

App::DocumentObject* PropertyLinkSub::getValue(Base::Type t) const
{
    return (_pcLinkSub && _pcLinkSub->isDerivedFrom(t)) ? _pcLinkSub : nullptr;
}

PyObject* PropertyLinkSub::getPyObject()
{
    Py::Tuple tup(2);
    Py::List list(static_cast<int>(_cSubList.size()));
    if (_pcLinkSub) {
        tup[0] = Py::asObject(_pcLinkSub->getPyObject());
        int i = 0;
        for (auto& sub : getSubValues(testFlag(LinkNewElement))) {
            list[i++] = Py::String(sub);
        }
        tup[1] = list;
        return Py::new_reference_to(tup);
    }
    else {
        return Py::new_reference_to(Py::None());
    }
}

void PropertyLinkSub::setPyObject(PyObject* value)
{
    if (PyObject_TypeCheck(value, &(DocumentObjectPy::Type))) {
        DocumentObjectPy* pcObject = static_cast<DocumentObjectPy*>(value);
        setValue(pcObject->getDocumentObjectPtr());
    }
    else if (PyTuple_Check(value) || PyList_Check(value)) {
        Py::Sequence seq(value);
        if (seq.size() == 0) {
            setValue(nullptr);
        }
        else if (seq.size() != 2) {
            throw Base::ValueError("Expect input sequence of size 2");
        }
        else if (PyObject_TypeCheck(seq[0].ptr(), &(DocumentObjectPy::Type))) {
            DocumentObjectPy* pcObj = static_cast<DocumentObjectPy*>(seq[0].ptr());
            static const char* errMsg =
                "type of second element in tuple must be str or sequence of str";
            PropertyString propString;
            if (seq[1].isString()) {
                std::vector<std::string> vals;
                propString.setPyObject(seq[1].ptr());
                vals.emplace_back(propString.getValue());
                setValue(pcObj->getDocumentObjectPtr(), std::move(vals));
            }
            else if (seq[1].isSequence()) {
                Py::Sequence list(seq[1]);
                std::vector<std::string> vals(list.size());
                unsigned int i = 0;
                for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it, ++i) {
                    if (!(*it).isString()) {
                        throw Base::TypeError(errMsg);
                    }
                    propString.setPyObject((*it).ptr());
                    vals[i] = propString.getValue();
                }
                setValue(pcObj->getDocumentObjectPtr(), std::move(vals));
            }
            else {
                throw Base::TypeError(errMsg);
            }
        }
        else {
            std::string error =
                std::string("type of first element in tuple must be 'DocumentObject', not ");
            error += seq[0].ptr()->ob_type->tp_name;
            throw Base::TypeError(error);
        }
    }
    else if (Py_None == value) {
        setValue(nullptr);
    }
    else {
        std::string error = std::string(
            "type must be 'DocumentObject', 'NoneType' or ('DocumentObject',['String',]) not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

bool updateLinkReference(App::PropertyLinkBase* prop,
                         App::DocumentObject* feature,
                         bool reverse,
                         bool notify,
                         App::DocumentObject* link,
                         std::vector<std::string>& subs,
                         std::vector<int>& mapped,
                         std::vector<PropertyLinkBase::ShadowSub>& shadows)
{
    if (!feature) {
        shadows.clear();
        prop->unregisterElementReference();
    }
    shadows.resize(subs.size());
    if (!link || !link->isAttachedToDocument()) {
        return false;
    }
    auto owner = dynamic_cast<DocumentObject*>(prop->getContainer());
    if (owner && owner->isRestoring()) {
        return false;
    }
    int i = 0;
    bool touched = false;
    for (auto& sub : subs) {
        if (prop->_updateElementReference(feature,
                                          link,
                                          sub,
                                          shadows[i++],
                                          reverse,
                                          notify && !touched)) {
            touched = true;
        }
    }
    if (!touched) {
        return false;
    }
    for (int idx : mapped) {
        if (idx < (int)subs.size() && !shadows[idx].newName.empty()) {
            subs[idx] = shadows[idx].newName;
        }
    }
    mapped.clear();
    if (owner && feature) {
        owner->onUpdateElementReference(prop);
    }
    return true;
}

void PropertyLinkSub::afterRestore()
{
    _ShadowSubList.resize(_cSubList.size());
    if (!testFlag(LinkRestoreLabel) || !_pcLinkSub || !_pcLinkSub->isAttachedToDocument()) {
        return;
    }
    setFlag(LinkRestoreLabel, false);
    for (std::size_t i = 0; i < _cSubList.size(); ++i) {
        restoreLabelReference(_pcLinkSub, _cSubList[i], &_ShadowSubList[i]);
    }
}

void PropertyLinkSub::onContainerRestored()
{
    unregisterElementReference();
    if (!_pcLinkSub || !_pcLinkSub->isAttachedToDocument()) {
        return;
    }
    for (std::size_t i = 0; i < _cSubList.size(); ++i) {
        _registerElementReference(_pcLinkSub, _cSubList[i], _ShadowSubList[i]);
    }
}

void PropertyLinkSub::updateElementReference(DocumentObject* feature, bool reverse, bool notify)
{
    if (!updateLinkReference(this,
                             feature,
                             reverse,
                             notify,
                             _pcLinkSub,
                             _cSubList,
                             _mapped,
                             _ShadowSubList)) {
        return;
    }
    if (notify) {
        hasSetValue();
    }
}

bool PropertyLinkSub::referenceChanged() const
{
    return !_mapped.empty();
}

std::string
PropertyLinkBase::importSubName(Base::XMLReader& reader, const char* sub, bool& restoreLabel)
{
    if (!reader.doNameMapping()) {
        return sub;
    }
    std::ostringstream str;
    for (const char* dot = strchr(sub, '.'); dot; sub = dot + 1, dot = strchr(sub, '.')) {
        size_t count = dot - sub;
        const char* tail = ".";
        if (count && dot[-1] == '@') {
            // tail=='@' means we are exporting a label reference. So retain
            // this marker so that the label can be restored in afterRestore().
            tail = "@.";
            --count;
            restoreLabel = true;
        }
        str << reader.getName(std::string(sub, count).c_str()) << tail;
    }
    str << sub;
    return str.str();
}

const char* PropertyLinkBase::exportSubName(std::string& output,
                                            const App::DocumentObject* obj,
                                            const char* sub,
                                            bool first_obj)
{
    std::ostringstream str;
    const char* res = sub;

    if (!sub || !sub[0]) {
        return res;
    }

    bool touched = false;
    if (first_obj) {
        auto dot = strchr(sub, '.');
        if (!dot) {
            return res;
        }
        const char* hash;
        for (hash = sub; hash < dot && *hash != '#'; ++hash) {}
        App::Document* doc = nullptr;
        if (*hash == '#') {
            doc = GetApplication().getDocument(std::string(sub, hash - sub).c_str());
        }
        else {
            hash = nullptr;
            if (obj && obj->isAttachedToDocument()) {
                doc = obj->getDocument();
            }
        }
        if (!doc) {
            FC_ERR("Failed to get document for the first object in " << sub);
            return res;
        }
        obj = doc->getObject(std::string(sub, dot - sub).c_str());
        if (!obj || !obj->isAttachedToDocument()) {
            return res;
        }
        if (hash) {
            if (!obj->isExporting()) {
                str << doc->getName() << '#';
            }
            sub = hash + 1;
        }
    }
    else if (!obj || !obj->isAttachedToDocument()) {
        return res;
    }

    for (const char* dot = strchr(sub, '.'); dot; sub = dot + 1, dot = strchr(sub, '.')) {
        // name with trailing '.'
        auto name = std::string(sub, dot - sub + 1);
        if (first_obj) {
            first_obj = false;
        }
        else {
            obj = obj->getSubObject(name.c_str());
        }
        if (!obj || !obj->isAttachedToDocument()) {
            FC_WARN("missing sub object '" << name << "' in '" << sub << "'");
            break;
        }
        if (obj->isExporting()) {
            if (name[0] == '$') {
                if (name.compare(1, name.size() - 2, obj->Label.getValue()) != 0) {
                    str << obj->getExportName(true) << "@.";
                    touched = true;
                    continue;
                }
            }
            else if (name.compare(0, name.size() - 1, obj->getNameInDocument()) == 0) {
                str << obj->getExportName(true) << '.';
                touched = true;
                continue;
            }
        }
        str << name;
    }
    if (!touched) {
        return res;
    }
    str << sub;
    output = str.str();
    return output.c_str();
}

App::DocumentObject* PropertyLinkBase::tryImport(const App::Document* doc,
                                                 const App::DocumentObject* obj,
                                                 const std::map<std::string, std::string>& nameMap)
{
    if (doc && obj && obj->isAttachedToDocument()) {
        auto it = nameMap.find(obj->getExportName(true));
        if (it != nameMap.end()) {
            obj = doc->getObject(it->second.c_str());
            if (!obj) {
                FC_THROWM(Base::RuntimeError, "Cannot find import object " << it->second);
            }
        }
    }
    return const_cast<DocumentObject*>(obj);
}

std::string PropertyLinkBase::tryImportSubName(const App::DocumentObject* obj,
                                               const char* _subname,
                                               const App::Document* doc,
                                               const std::map<std::string, std::string>& nameMap)
{
    if (!doc || !obj || !obj->isAttachedToDocument()) {
        return {};
    }

    std::ostringstream ss;
    std::string subname(_subname);
    char* sub = &subname[0];
    char* next = sub;
    for (char* dot = strchr(next, '.'); dot; next = dot + 1, dot = strchr(next, '.')) {
        StringGuard guard(dot);
        auto sobj = obj->getSubObject(subname.c_str());
        if (!sobj) {
            FC_ERR("Failed to restore label reference " << obj->getFullName() << '.' << subname);
            return {};
        }
        dot[0] = 0;
        if (next[0] == '$') {
            if (strcmp(next + 1, sobj->Label.getValue()) != 0) {
                continue;
            }
        }
        else if (strcmp(next, sobj->getNameInDocument()) != 0) {
            continue;
        }
        auto it = nameMap.find(sobj->getExportName(true));
        if (it == nameMap.end()) {
            continue;
        }
        auto imported = doc->getObject(it->second.c_str());
        if (!imported) {
            FC_THROWM(Base::RuntimeError, "Failed to find imported object " << it->second);
        }
        ss.write(sub, next - sub);
        if (next[0] == '$') {
            ss << '$' << imported->Label.getStrValue() << '.';
        }
        else {
            ss << it->second << '.';
        }
        sub = dot + 1;
    }
    if (sub != subname.c_str()) {
        return ss.str();
    }
    return {};
}

void PropertyLinkBase::_getLinksTo(std::vector<App::ObjectIdentifier>& identifiers,
                                   App::DocumentObject* obj,
                                   const char* subname,
                                   const std::vector<std::string>& subs,
                                   const std::vector<PropertyLinkBase::ShadowSub>& shadows) const
{
    if (!subname) {
        identifiers.emplace_back(*this);
        return;
    }
    App::SubObjectT objT(obj, subname);
    auto subObject = objT.getSubObject();
    auto subElement = objT.getOldElementName();

    int i = -1;
    for (const auto& sub : subs) {
        ++i;
        if (sub == subname) {
            identifiers.emplace_back(*this);
            return;
        }
        if (!subObject) {
            continue;
        }
        // After above, there is a subobject and the subname doesn't match our current entry
        App::SubObjectT sobjT(obj, sub.c_str());
        if (sobjT.getSubObject() == subObject && sobjT.getOldElementName() == subElement) {
            identifiers.emplace_back(*this);
            return;
        }
        // And the oldElementName ( short, I.E. "Edge5" ) doesn't match.
        if (i < (int)shadows.size()) {
            const auto& [shadowNewName, shadowOldName] = shadows[i];
            if (shadowNewName == subname || shadowOldName == subname) {
                identifiers.emplace_back(*this);
                return;
            }
            if (!subObject) {
                continue;
            }
            App::SubObjectT shadowobjT(obj,
                                       shadowNewName.empty() ? shadowOldName.c_str()
                                                             : shadowNewName.c_str());
            if (shadowobjT.getSubObject() == subObject
                && shadowobjT.getOldElementName() == subElement) {
                identifiers.emplace_back(*this);
                return;
            }
        }
    }
}

void PropertyLinkSub::Save(Base::Writer& writer) const
{
    assert(_cSubList.size() == _ShadowSubList.size());

    std::string internal_name;
    // it can happen that the object is still alive but is not part of the document anymore and thus
    // returns 0
    if (_pcLinkSub && _pcLinkSub->isAttachedToDocument()) {
        internal_name = _pcLinkSub->getExportName();
    }
    writer.Stream() << writer.ind() << "<LinkSub value=\"" << internal_name << "\" count=\""
                    << _cSubList.size();
    writer.Stream() << "\">" << std::endl;
    writer.incInd();
    auto owner = dynamic_cast<DocumentObject*>(getContainer());
    bool exporting = owner && owner->isExporting();
    for (unsigned int i = 0; i < _cSubList.size(); i++) {
        const auto& shadow = _ShadowSubList[i];
        // shadow.oldName stores the old style element name. For backward
        // compatibility reason, we shall store the old name into attribute
        // 'value' whenever possible.
        const auto& sub = shadow.oldName.empty() ? _cSubList[i] : shadow.oldName;
        writer.Stream() << writer.ind() << "<Sub value=\"";
        if (exporting) {
            std::string exportName;
            writer.Stream() << encodeAttribute(exportSubName(exportName, _pcLinkSub, sub.c_str()));
            if (!shadow.oldName.empty() && shadow.newName == _cSubList[i]) {
                writer.Stream() << "\" " ATTR_MAPPED "=\"1";
            }
        }
        else {
            writer.Stream() << encodeAttribute(sub);
            if (!_cSubList[i].empty()) {
                if (sub != _cSubList[i]) {
                    // Stores the actual value that is shadowed. For new version FC,
                    // we will restore this shadowed value instead.
                    writer.Stream() << "\" " ATTR_SHADOWED "=\"" << encodeAttribute(_cSubList[i]);
                }
                else if (!shadow.newName.empty()) {
                    // Here means the user set value is old style element name.
                    // We shall then store the shadow somewhere else.
                    writer.Stream() << "\" " ATTR_SHADOW "=\"" << encodeAttribute(shadow.newName);
                }
            }
        }
        writer.Stream() << "\"/>" << std::endl;
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</LinkSub>" << std::endl;
}

void PropertyLinkSub::Restore(Base::XMLReader& reader)
{
    // read my element
    reader.readElement("LinkSub");
    // get the values of my attributes
    std::string name = reader.getName(reader.getAttribute("value"));
    int count = reader.getAttributeAsInteger("count");

    // Property not in a DocumentObject!
    assert(getContainer()->isDerivedFrom<App::DocumentObject>());
    App::Document* document = static_cast<DocumentObject*>(getContainer())->getDocument();

    DocumentObject* pcObject = nullptr;
    if (!name.empty()) {
        pcObject = document ? document->getObject(name.c_str()) : nullptr;
        if (!pcObject) {
            if (reader.isVerbose()) {
                FC_WARN("Lost link to "
                        << name << " while loading, maybe an object was not loaded correctly");
            }
        }
    }

    std::vector<int> mapped;
    std::vector<std::string> values(count);
    std::vector<ShadowSub> shadows(count);
    bool restoreLabel = false;
    // Sub may store '.' separated object names, so be aware of the possible mapping when import
    for (int i = 0; i < count; i++) {
        reader.readElement("Sub");
        shadows[i].oldName = importSubName(reader, reader.getAttribute("value"), restoreLabel);
        if (reader.hasAttribute(ATTR_SHADOWED) && !IGNORE_SHADOW) {
            values[i] = shadows[i].newName =
                importSubName(reader, reader.getAttribute(ATTR_SHADOWED), restoreLabel);
        }
        else {
            values[i] = shadows[i].oldName;
            if (reader.hasAttribute(ATTR_SHADOW) && !IGNORE_SHADOW) {
                shadows[i].newName =
                    importSubName(reader, reader.getAttribute(ATTR_SHADOW), restoreLabel);
            }
        }
        if (reader.hasAttribute(ATTR_MAPPED)) {
            mapped.push_back(i);
        }
    }
    setFlag(LinkRestoreLabel, restoreLabel);

    reader.readEndElement("LinkSub");

    if (pcObject) {
        setValue(pcObject, std::move(values), std::move(shadows));
        _mapped = std::move(mapped);
    }
    else {
        setValue(nullptr);
    }
}

template<class Func, class... Args>
std::vector<std::string> updateLinkSubs(const App::DocumentObject* obj,
                                        const std::vector<std::string>& subs,
                                        Func* f,
                                        Args&&... args)
{
    if (!obj || !obj->isAttachedToDocument()) {
        return {};
    }

    std::vector<std::string> res;
    for (auto it = subs.begin(); it != subs.end(); ++it) {
        const auto& sub = *it;
        auto new_sub = (*f)(obj, sub.c_str(), std::forward<Args>(args)...);
        if (new_sub.size()) {
            if (res.empty()) {
                res.reserve(subs.size());
                res.insert(res.end(), subs.begin(), it);
            }
            res.push_back(std::move(new_sub));
        }
        else if (!res.empty()) {
            res.push_back(sub);
        }
    }
    return res;
}

Property*
PropertyLinkSub::CopyOnImportExternal(const std::map<std::string, std::string>& nameMap) const
{
    auto owner = dynamic_cast<const DocumentObject*>(getContainer());
    if (!owner || !owner->getDocument()) {
        return nullptr;
    }
    if (!_pcLinkSub || !_pcLinkSub->isAttachedToDocument()) {
        return nullptr;
    }

    auto subs =
        updateLinkSubs(_pcLinkSub, _cSubList, &tryImportSubName, owner->getDocument(), nameMap);
    auto linked = tryImport(owner->getDocument(), _pcLinkSub, nameMap);
    if (subs.empty() && linked == _pcLinkSub) {
        return nullptr;
    }

    PropertyLinkSub* p = new PropertyLinkSub();
    p->_pcLinkSub = linked;
    if (subs.empty()) {
        p->_cSubList = _cSubList;
    }
    else {
        p->_cSubList = std::move(subs);
    }
    return p;
}

Property* PropertyLinkSub::CopyOnLabelChange(App::DocumentObject* obj,
                                             const std::string& ref,
                                             const char* newLabel) const
{
    auto owner = dynamic_cast<const DocumentObject*>(getContainer());
    if (!owner || !owner->getDocument()) {
        return nullptr;
    }
    if (!_pcLinkSub || !_pcLinkSub->isAttachedToDocument()) {
        return nullptr;
    }

    auto subs = updateLinkSubs(_pcLinkSub, _cSubList, &updateLabelReference, obj, ref, newLabel);
    if (subs.empty()) {
        return nullptr;
    }

    PropertyLinkSub* p = new PropertyLinkSub();
    p->_pcLinkSub = _pcLinkSub;
    p->_cSubList = std::move(subs);
    return p;
}

Property* PropertyLinkSub::CopyOnLinkReplace(const App::DocumentObject* parent,
                                             App::DocumentObject* oldObj,
                                             App::DocumentObject* newObj) const
{
    auto res = tryReplaceLinkSubs(getContainer(), _pcLinkSub, parent, oldObj, newObj, _cSubList);
    if (res.first) {
        PropertyLinkSub* p = new PropertyLinkSub();
        p->_pcLinkSub = res.first;
        p->_cSubList = std::move(res.second);
        return p;
    }
    return nullptr;
}

Property* PropertyLinkSub::Copy() const
{
    PropertyLinkSub* p = new PropertyLinkSub();
    p->_pcLinkSub = _pcLinkSub;
    p->_cSubList = _cSubList;
    p->_ShadowSubList = _ShadowSubList;
    return p;
}

void PropertyLinkSub::Paste(const Property& from)
{
    if (!from.isDerivedFrom<PropertyLinkSub>()) {
        throw Base::TypeError("Incompatible property to paste to");
    }
    auto& link = static_cast<const PropertyLinkSub&>(from);
    setValue(link._pcLinkSub, link._cSubList, std::vector<ShadowSub>(link._ShadowSubList));
}

void PropertyLinkSub::getLinks(std::vector<App::DocumentObject*>& objs,
                               bool all,
                               std::vector<std::string>* subs,
                               bool newStyle) const
{
    if (all || _pcScope != LinkScope::Hidden) {
        if (_pcLinkSub && _pcLinkSub->isAttachedToDocument()) {
            objs.push_back(_pcLinkSub);
            if (subs) {
                *subs = getSubValues(newStyle);
            }
        }
    }
}

void PropertyLinkSub::getLinksTo(std::vector<App::ObjectIdentifier>& identifiers,
                                 App::DocumentObject* obj,
                                 const char* subname,
                                 bool all) const
{
    if (all || _pcScope != LinkScope::Hidden) {
        if (obj && obj == _pcLinkSub) {
            _getLinksTo(identifiers, obj, subname, _cSubList, _ShadowSubList);
        }
    }
}

void PropertyLinkSub::breakLink(App::DocumentObject* obj, bool clear)
{
    if (obj == _pcLinkSub || (clear && getContainer() == obj)) {
        setValue(nullptr);
    }
}

App::DocumentObject*
adjustLinkSubs(App::PropertyLinkBase* prop,
               const std::set<App::DocumentObject*>& inList,
               App::DocumentObject* link,
               std::vector<std::string>& subs,
               std::map<App::DocumentObject*, std::vector<std::string>>* links)
{
    App::DocumentObject* newLink = nullptr;
    for (auto& sub : subs) {
        size_t pos = sub.find('.');
        for (; pos != std::string::npos; pos = sub.find('.', pos + 1)) {
            auto sobj = link->getSubObject(sub.substr(0, pos + 1).c_str());
            if (!sobj
                || (!prop->testFlag(PropertyLinkBase::LinkAllowExternal)
                    && sobj->getDocument() != link->getDocument())) {
                pos = std::string::npos;
                break;
            }
            if (!newLink) {
                if (inList.count(sobj)) {
                    continue;
                }
                newLink = sobj;
                if (links) {
                    (*links)[sobj].push_back(sub.substr(pos + 1));
                }
                else {
                    sub = sub.substr(pos + 1);
                }
            }
            else if (links) {
                (*links)[sobj].push_back(sub.substr(pos + 1));
            }
            else if (sobj == newLink) {
                sub = sub.substr(pos + 1);
            }
            break;
        }
        if (pos == std::string::npos) {
            return nullptr;
        }
    }
    return newLink;
}

bool PropertyLinkSub::adjustLink(const std::set<App::DocumentObject*>& inList)
{
    if (_pcScope == LinkScope::Hidden) {
        return false;
    }
    if (!_pcLinkSub || !_pcLinkSub->isAttachedToDocument() || !inList.count(_pcLinkSub)) {
        return false;
    }
    auto subs = _cSubList;
    auto link = adjustLinkSubs(this, inList, _pcLinkSub, subs);
    if (link) {
        setValue(link, std::move(subs));
        return true;
    }
    return false;
}

}  // namespace App