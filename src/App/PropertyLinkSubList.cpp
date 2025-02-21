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

#include <Base/Writer.h>
#include <Base/Reader.h>

#include "PropertyLinkList.h"
#include "PropertyLinkSub.h"
#include "PropertyLinkSubList.h"
#include "PropertyContainer.h"
#include "DocumentObject.h"
#include "DocumentObjectPy.h"
#include "DocumentObserver.h"
#include "ObjectIdentifier.h"
#include "Document.h"

namespace App {

//**************************************************************************
// PropertyLinkSubList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyLinkSubList, App::PropertyLinkBase)
TYPESYSTEM_SOURCE(App::PropertyLinkSubListChild, App::PropertyLinkSubList)
TYPESYSTEM_SOURCE(App::PropertyLinkSubListGlobal, App::PropertyLinkSubList)
TYPESYSTEM_SOURCE(App::PropertyLinkSubListHidden, App::PropertyLinkSubList)

//**************************************************************************
// Construction/Destruction


PropertyLinkSubList::PropertyLinkSubList() = default;

PropertyLinkSubList::~PropertyLinkSubList()
{
    // in case this property is dynamically removed
#ifndef USE_OLD_DAG
    // maintain backlinks
    if (!_lValueList.empty() && getContainer()
        && getContainer()->isDerivedFrom<App::DocumentObject>()) {
        App::DocumentObject* parent = static_cast<DocumentObject*>(getContainer());
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy) && _pcScope != LinkScope::Hidden) {
            for (auto* obj : _lValueList) {
                if (obj) {
                    obj->_removeBackLink(parent);
                }
            }
        }
    }
#endif
}

void PropertyLinkSubList::setSyncSubObject(bool enable)
{
    _Flags.set((std::size_t)LinkSyncSubObject, enable);
}

void PropertyLinkSubList::verifyObject(App::DocumentObject* obj, App::DocumentObject* parent)
{
    if (obj) {
        if (!obj->isAttachedToDocument()) {
            throw Base::ValueError("PropertyLinkSubList: invalid document object");
        }
        if (!testFlag(LinkAllowExternal) && parent && parent->getDocument() != obj->getDocument()) {
            throw Base::ValueError("PropertyLinkSubList does not support external object");
        }
    }
}

void PropertyLinkSubList::setSize(int newSize)
{
    _lValueList.resize(newSize);
    _lSubList.resize(newSize);
    _ShadowSubList.resize(newSize);
}

int PropertyLinkSubList::getSize() const
{
    return static_cast<int>(_lValueList.size());
}

void PropertyLinkSubList::setValue(DocumentObject* lValue, const char* SubName)
{
    auto parent = Base::freecad_dynamic_cast<App::DocumentObject>(getContainer());
    verifyObject(lValue, parent);

#ifndef USE_OLD_DAG
    // maintain backlinks
    if (parent) {
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy) && _pcScope != LinkScope::Hidden) {
            for (auto* obj : _lValueList) {
                if (obj) {
                    obj->_removeBackLink(parent);
                }
            }
            if (lValue) {
                lValue->_addBackLink(parent);
            }
        }
    }
#endif

    if (lValue) {
        aboutToSetValue();
        _lValueList.resize(1);
        _lValueList[0] = lValue;
        _lSubList.resize(1);
        _lSubList[0] = SubName;
    }
    else {
        aboutToSetValue();
        _lValueList.clear();
        _lSubList.clear();
    }
    updateElementReference(nullptr);
    checkLabelReferences(_lSubList);
    hasSetValue();
}

void PropertyLinkSubList::setValues(const std::vector<DocumentObject*>& lValue,
                                    const std::vector<const char*>& lSubNames)
{
    auto parent = Base::freecad_dynamic_cast<App::DocumentObject>(getContainer());
    for (auto obj : lValue) {
        verifyObject(obj, parent);
    }

    if (lValue.size() != lSubNames.size()) {
        throw Base::ValueError(
            "PropertyLinkSubList::setValues: size of subelements list != size of objects list");
    }

#ifndef USE_OLD_DAG
    // maintain backlinks.
    if (parent) {
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy) && _pcScope != LinkScope::Hidden) {
            //_lValueList can contain items multiple times, but we trust the document
            // object to ensure that this works
            for (auto* obj : _lValueList) {
                if (obj) {
                    obj->_removeBackLink(parent);
                }
            }

            // maintain backlinks. lValue can contain items multiple times, but we trust the
            // document object to ensure that the backlink is only added once
            for (auto* obj : lValue) {
                if (obj) {
                    obj->_addBackLink(parent);
                }
            }
        }
    }
#endif

    aboutToSetValue();
    _lValueList = lValue;
    _lSubList.resize(lSubNames.size());
    int i = 0;
    for (std::vector<const char*>::const_iterator it = lSubNames.begin(); it != lSubNames.end();
         ++it, ++i) {
        if (*it) {
            _lSubList[i] = *it;
        }
    }
    updateElementReference(nullptr);
    checkLabelReferences(_lSubList);
    hasSetValue();
}

void PropertyLinkSubList::setValues(const std::vector<DocumentObject*>& lValue,
                                    const std::vector<std::string>& lSubNames,
                                    std::vector<ShadowSub>&& ShadowSubList)
{
    setValues(std::vector<DocumentObject*>(lValue),
              std::vector<std::string>(lSubNames),
              std::move(ShadowSubList));
}

void PropertyLinkSubList::setValues(std::vector<DocumentObject*>&& lValue,
                                    std::vector<std::string>&& lSubNames,
                                    std::vector<ShadowSub>&& ShadowSubList)
{
    auto parent = Base::freecad_dynamic_cast<App::DocumentObject>(getContainer());
    for (auto obj : lValue) {
        verifyObject(obj, parent);
    }
    if (lValue.size() != lSubNames.size()) {
        throw Base::ValueError(
            "PropertyLinkSubList::setValues: size of subelements list != size of objects list");
    }

#ifndef USE_OLD_DAG
    // maintain backlinks.
    if (parent) {
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy) && _pcScope != LinkScope::Hidden) {
            //_lValueList can contain items multiple times, but we trust the document
            // object to ensure that this works
            for (auto* obj : _lValueList) {
                if (obj) {
                    obj->_removeBackLink(parent);
                }
            }

            // maintain backlinks. lValue can contain items multiple times, but we trust the
            // document object to ensure that the backlink is only added once
            for (auto* obj : lValue) {
                if (obj) {
                    obj->_addBackLink(parent);
                }
            }
        }
    }
#endif

    aboutToSetValue();
    _lValueList = std::move(lValue);
    _lSubList = std::move(lSubNames);
    if (ShadowSubList.size() == _lSubList.size()) {
        _ShadowSubList = std::move(ShadowSubList);
        onContainerRestored();  // re-register element references
    }
    else {
        updateElementReference(nullptr);
    }
    checkLabelReferences(_lSubList);
    hasSetValue();
}

void PropertyLinkSubList::setValue(DocumentObject* lValue, const std::vector<std::string>& SubList)
{
    auto parent = dynamic_cast<App::DocumentObject*>(getContainer());
    verifyObject(lValue, parent);

#ifndef USE_OLD_DAG
    // maintain backlinks.
    if (parent) {
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy) && _pcScope != LinkScope::Hidden) {
            //_lValueList can contain items multiple times, but we trust the document
            // object to ensure that this works
            for (auto* obj : _lValueList) {
                if (obj) {
                    obj->_removeBackLink(parent);
                }
            }

            // maintain backlinks. lValue can contain items multiple times, but we trust the
            // document object to ensure that the backlink is only added once
            if (lValue) {
                lValue->_addBackLink(parent);
            }
        }
    }
#endif

    aboutToSetValue();
    std::size_t size = SubList.size();
    this->_lValueList.clear();
    this->_lSubList.clear();
    if (size == 0) {
        if (lValue) {
            this->_lValueList.push_back(lValue);
            this->_lSubList.emplace_back();
        }
    }
    else {
        this->_lSubList = SubList;
        this->_lValueList.insert(this->_lValueList.begin(), size, lValue);
    }
    updateElementReference(nullptr);
    checkLabelReferences(_lSubList);
    hasSetValue();
}

void PropertyLinkSubList::addValue(App::DocumentObject* obj,
                                   const std::vector<std::string>& subs,
                                   bool reset)
{
    auto parent = Base::freecad_dynamic_cast<App::DocumentObject>(getContainer());
    verifyObject(obj, parent);

#ifndef USE_OLD_DAG
    // maintain backlinks.
    if (parent) {
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy) && _pcScope != LinkScope::Hidden) {
            //_lValueList can contain items multiple times, but we trust the document
            // object to ensure that this works
            if (reset) {
                for (auto* value : _lValueList) {
                    if (value && value == obj) {
                        value->_removeBackLink(parent);
                    }
                }
            }

            // maintain backlinks. lValue can contain items multiple times, but we trust the
            // document object to ensure that the backlink is only added once
            if (obj) {
                obj->_addBackLink(parent);
            }
        }
    }
#endif

    std::vector<DocumentObject*> valueList;
    std::vector<std::string> subList;

    if (reset) {
        for (std::size_t i = 0; i < _lValueList.size(); i++) {
            if (_lValueList[i] != obj) {
                valueList.push_back(_lValueList[i]);
                subList.push_back(_lSubList[i]);
            }
        }
    }
    else {
        valueList = _lValueList;
        subList = _lSubList;
    }

    std::size_t size = subs.size();
    if (size == 0) {
        if (obj) {
            valueList.push_back(obj);
            subList.emplace_back();
        }
    }
    else if (obj) {
        subList.insert(subList.end(), subs.begin(), subs.end());
        valueList.insert(valueList.end(), size, obj);
    }

    aboutToSetValue();
    _lValueList = valueList;
    _lSubList = subList;
    updateElementReference(nullptr);
    checkLabelReferences(_lSubList);
    hasSetValue();
}

const std::string PropertyLinkSubList::getPyReprString() const
{
    assert(this->_lValueList.size() == this->_lSubList.size());

    if (this->_lValueList.empty()) {
        return std::string("None");
    }

    std::stringstream strm;
    strm << "[";
    for (std::size_t i = 0; i < this->_lSubList.size(); i++) {
        if (i > 0) {
            strm << ",(";
        }
        else {
            strm << "(";
        }
        App::DocumentObject* obj = this->_lValueList[i];
        if (obj) {
            strm << "App.getDocument('" << obj->getDocument()->getName() << "').getObject('"
                 << obj->getNameInDocument() << "')";
        }
        else {
            strm << "None";
        }
        strm << ",";
        strm << "'" << this->_lSubList[i] << "'";
        strm << ")";
    }
    strm << "]";
    return strm.str();
}

DocumentObject* PropertyLinkSubList::getValue() const
{
    App::DocumentObject* ret = nullptr;
    // FIXME: cache this to avoid iterating each time, to improve speed
    for (auto i : this->_lValueList) {
        if (!ret) {
            ret = i;
        }
        if (ret != i) {
            return nullptr;
        }
    }
    return ret;
}

int PropertyLinkSubList::removeValue(App::DocumentObject* lValue)
{
    assert(this->_lValueList.size() == this->_lSubList.size());

    std::size_t num = std::count(this->_lValueList.begin(), this->_lValueList.end(), lValue);
    if (num == 0) {
        return 0;
    }

    std::vector<DocumentObject*> links;
    std::vector<std::string> subs;
    links.reserve(this->_lValueList.size() - num);
    subs.reserve(this->_lSubList.size() - num);

    for (std::size_t i = 0; i < this->_lValueList.size(); ++i) {
        if (this->_lValueList[i] != lValue) {
            links.push_back(this->_lValueList[i]);
            subs.push_back(this->_lSubList[i]);
        }
    }

    setValues(links, subs);
    return static_cast<int>(num);
}

void PropertyLinkSubList::setSubListValues(const std::vector<SubSet>& values)
{
    std::vector<DocumentObject*> links;
    std::vector<std::string> subs;
    for (std::vector<SubSet>::const_iterator it = values.begin();
         it != values.end();
         ++it) {
        if (it->second.empty()) {
            links.push_back(it->first);
            subs.emplace_back();
            continue;
        }
        for (std::vector<std::string>::const_iterator jt = it->second.begin();
             jt != it->second.end();
             ++jt) {
            links.push_back(it->first);
            subs.push_back(*jt);
        }
    }
    setValues(links, subs);
}

std::vector<SubSet> PropertyLinkSubList::getSubListValues(bool newStyle) const
{
    std::vector<SubSet> values;
    if (_lValueList.size() != _lSubList.size()) {
        throw Base::ValueError("PropertyLinkSubList::getSubListValues: size of subelements list != "
                               "size of objects list");
    }

    assert(_ShadowSubList.size() == _lSubList.size());

    for (std::size_t i = 0; i < _lValueList.size(); i++) {
        App::DocumentObject* link = _lValueList[i];
        std::string sub;
        if (newStyle && !_ShadowSubList[i].newName.empty()) {
            sub = _ShadowSubList[i].newName;
        }
        else if (!newStyle && !_ShadowSubList[i].oldName.empty()) {
            sub = _ShadowSubList[i].oldName;
        }
        else {
            sub = _lSubList[i];
        }
        if (values.empty() || values.back().first != link) {
            // new object started, start a new subset.
            values.emplace_back(link, std::vector<std::string>());
        }
        values.back().second.push_back(sub);
    }
    return values;
}

PyObject* PropertyLinkSubList::getPyObject()
{
    std::vector<SubSet> subLists = getSubListValues();
    std::size_t count = subLists.size();
#if 0  // FIXME: Should switch to tuple
    Py::Tuple sequence(count);
#else
    Py::List sequence(count);
#endif
    for (std::size_t i = 0; i < count; i++) {
        Py::Tuple tup(2);
        tup[0] = Py::asObject(subLists[i].first->getPyObject());

        const std::vector<std::string>& sub = subLists[i].second;
        Py::Tuple items(sub.size());
        for (std::size_t j = 0; j < sub.size(); j++) {
            items[j] = Py::String(sub[j]);
        }

        tup[1] = items;
        sequence[i] = tup;
    }

    return Py::new_reference_to(sequence);
}

void PropertyLinkSubList::setPyObject(PyObject* value)
{
    try {  // try PropertyLinkSub syntax
        PropertyLinkSub dummy;
        dummy.setPyObject(value);
        this->setValue(dummy.getValue(), dummy.getSubValues());
        return;
    }
    catch (...) {
    }
    try {
        // try PropertyLinkList syntax
        PropertyLinkList dummy;
        dummy.setPyObject(value);
        const auto& values = dummy.getValues();
        std::vector<std::string> subs(values.size());
        this->setValues(values, subs);
        return;
    }
    catch (...) {
    }

    static const char* errMsg =
        "Expects sequence of items of type DocObj, (DocObj,SubName), or (DocObj, (SubName,...))";

    if (!PyTuple_Check(value) && !PyList_Check(value)) {
        throw Base::TypeError(errMsg);
    }

    Py::Sequence list(value);
    Py::Sequence::size_type size = list.size();

    std::vector<DocumentObject*> values;
    values.reserve(size);
    std::vector<std::string> SubNames;
    SubNames.reserve(size);
    for (Py::Sequence::size_type i = 0; i < size; i++) {
        Py::Object item = list[i];
        if ((item.isTuple() || item.isSequence()) && PySequence_Size(*item) == 2) {
            Py::Sequence seq(item);
            if (PyObject_TypeCheck(seq[0].ptr(), &(DocumentObjectPy::Type))) {
                auto obj = static_cast<DocumentObjectPy*>(seq[0].ptr())->getDocumentObjectPtr();
                PropertyString propString;
                if (seq[1].isString()) {
                    values.push_back(obj);
                    propString.setPyObject(seq[1].ptr());
                    SubNames.emplace_back(propString.getValue());
                }
                else if (seq[1].isSequence()) {
                    Py::Sequence list(seq[1]);
                    for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                        if (!(*it).isString()) {
                            throw Base::TypeError(errMsg);
                        }
                        values.push_back(obj);
                        propString.setPyObject((*it).ptr());
                        SubNames.emplace_back(propString.getValue());
                    }
                }
                else {
                    throw Base::TypeError(errMsg);
                }
            }
        }
        else if (PyObject_TypeCheck(*item, &(DocumentObjectPy::Type))) {
            DocumentObjectPy* pcObj;
            pcObj = static_cast<DocumentObjectPy*>(*item);
            values.push_back(pcObj->getDocumentObjectPtr());
            SubNames.emplace_back();
        }
        else {
            throw Base::TypeError(errMsg);
        }
    }
    setValues(values, SubNames);
}

void PropertyLinkSubList::afterRestore()
{
    assert(_lSubList.size() == _ShadowSubList.size());
    if (!testFlag(LinkRestoreLabel)) {
        return;
    }
    setFlag(LinkRestoreLabel, false);
    for (size_t i = 0; i < _lSubList.size(); ++i) {
        restoreLabelReference(_lValueList[i], _lSubList[i], &_ShadowSubList[i]);
    }
}

void PropertyLinkSubList::onContainerRestored()
{
    unregisterElementReference();
    for (size_t i = 0; i < _lSubList.size(); ++i) {
        _registerElementReference(_lValueList[i], _lSubList[i], _ShadowSubList[i]);
    }
}

void PropertyLinkSubList::updateElementReference(DocumentObject* feature, bool reverse, bool notify)
{
    if (!feature) {
        _ShadowSubList.clear();
        unregisterElementReference();
    }
    _ShadowSubList.resize(_lSubList.size());
    auto owner = freecad_dynamic_cast<DocumentObject>(getContainer());
    if (owner && owner->isRestoring()) {
        return;
    }
    int i = 0;
    bool touched = false;
    for (auto& sub : _lSubList) {
        auto obj = _lValueList[i];
        if (_updateElementReference(feature,
                                    obj,
                                    sub,
                                    _ShadowSubList[i++],
                                    reverse,
                                    notify && !touched)) {
            touched = true;
        }
    }
    if (!touched) {
        return;
    }

    std::vector<int> mapped;
    mapped.reserve(_mapped.size());
    for (int idx : _mapped) {
        if (idx < (int)_lSubList.size()) {
            if (!_ShadowSubList[idx].newName.empty()) {
                _lSubList[idx] = _ShadowSubList[idx].newName;
            }
            else {
                mapped.push_back(idx);
            }
        }
    }
    _mapped.swap(mapped);
    if (owner && feature) {
        owner->onUpdateElementReference(this);
    }
    if (notify) {
        hasSetValue();
    }
}

bool PropertyLinkSubList::referenceChanged() const
{
    return !_mapped.empty();
}

void PropertyLinkSubList::Save(Base::Writer& writer) const
{
    assert(_lSubList.size() == _ShadowSubList.size());

    int count = 0;
    for (auto obj : _lValueList) {
        if (obj && obj->isAttachedToDocument()) {
            ++count;
        }
    }
    writer.Stream() << writer.ind() << "<LinkSubList count=\"" << count << "\">" << std::endl;
    writer.incInd();
    auto owner = dynamic_cast<DocumentObject*>(getContainer());
    bool exporting = owner && owner->isExporting();
    for (int i = 0; i < getSize(); i++) {
        auto obj = _lValueList[i];
        if (!obj || !obj->isAttachedToDocument()) {
            continue;
        }
        const auto& shadow = _ShadowSubList[i];
        // shadow.oldName stores the old style element name. For backward
        // compatibility reason, we shall store the old name into attribute
        // 'value' whenever possible.
        const auto& sub = shadow.oldName.empty() ? _lSubList[i] : shadow.oldName;

        writer.Stream() << writer.ind() << "<Link obj=\"" << obj->getExportName() << "\" sub=\"";
        if (exporting) {
            std::string exportName;
            writer.Stream() << encodeAttribute(exportSubName(exportName, obj, sub.c_str()));
            if (!shadow.oldName.empty() && _lSubList[i] == shadow.newName) {
                writer.Stream() << "\" " ATTR_MAPPED "=\"1";
            }
        }
        else {
            writer.Stream() << encodeAttribute(sub);
            if (!_lSubList[i].empty()) {
                if (sub != _lSubList[i]) {
                    // Stores the actual value that is shadowed. For new version FC,
                    // we will restore this shadowed value instead.
                    writer.Stream() << "\" " ATTR_SHADOWED "=\"" << encodeAttribute(_lSubList[i]);
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
    writer.Stream() << writer.ind() << "</LinkSubList>" << std::endl;
}

void PropertyLinkSubList::Restore(Base::XMLReader& reader)
{
    // read my element
    reader.readElement("LinkSubList");
    // get the value of my attribute
    int count = reader.getAttributeAsInteger("count");

    std::vector<DocumentObject*> values;
    values.reserve(count);
    std::vector<std::string> SubNames;
    SubNames.reserve(count);
    std::vector<ShadowSub> shadows;
    shadows.reserve(count);
    DocumentObject* father = dynamic_cast<DocumentObject*>(getContainer());
    App::Document* document = father ? father->getDocument() : nullptr;
    std::vector<int> mapped;
    bool restoreLabel = false;
    for (int i = 0; i < count; i++) {
        reader.readElement("Link");
        std::string name = reader.getName(reader.getAttribute("obj"));
        // In order to do copy/paste it must be allowed to have defined some
        // referenced objects in XML which do not exist anymore in the new
        // document. Thus, we should silently ignore this.
        // Property not in an object!
        DocumentObject* child = document ? document->getObject(name.c_str()) : nullptr;
        if (child) {
            values.push_back(child);
            shadows.emplace_back();
            auto& shadow = shadows.back();
            shadow.oldName = importSubName(reader, reader.getAttribute("sub"), restoreLabel);
            if (reader.hasAttribute(ATTR_SHADOWED) && !IGNORE_SHADOW) {
                shadow.newName =
                    importSubName(reader, reader.getAttribute(ATTR_SHADOWED), restoreLabel);
                SubNames.push_back(shadow.newName);
            }
            else {
                SubNames.push_back(shadow.oldName);
                if (reader.hasAttribute(ATTR_SHADOW) && !IGNORE_SHADOW) {
                    shadow.newName =
                        importSubName(reader, reader.getAttribute(ATTR_SHADOW), restoreLabel);
                }
            }
            if (reader.hasAttribute(ATTR_MAPPED)) {
                mapped.push_back(i);
            }
        }
        else if (reader.isVerbose()) {
            Base::Console().Warning("Lost link to '%s' while loading, maybe "
                                    "an object was not loaded correctly\n",
                                    name.c_str());
        }
    }
    setFlag(LinkRestoreLabel, restoreLabel);

    reader.readEndElement("LinkSubList");

    // assignment
    setValues(values, SubNames, std::move(shadows));
    _mapped.swap(mapped);
}

bool PropertyLinkSubList::upgrade(Base::XMLReader& reader, const char* typeName)
{
    Base::Type type = Base::Type::fromName(typeName);
    if (type.isDerivedFrom(PropertyLink::getClassTypeId())) {
        PropertyLink prop;
        prop.setContainer(getContainer());
        prop.Restore(reader);
        setValue(prop.getValue());
        return true;
    }
    else if (type.isDerivedFrom(PropertyLinkList::getClassTypeId())) {
        PropertyLinkList prop;
        prop.setContainer(getContainer());
        prop.Restore(reader);
        std::vector<std::string> subnames;
        subnames.resize(prop.getSize());
        setValues(prop.getValues(), subnames);
        return true;
    }
    else if (type.isDerivedFrom(PropertyLinkSub::getClassTypeId())) {
        PropertyLinkSub prop;
        prop.setContainer(getContainer());
        prop.Restore(reader);
        setValue(prop.getValue(), prop.getSubValues());
        return true;
    }

    return false;
}

Property*
PropertyLinkSubList::CopyOnImportExternal(const std::map<std::string, std::string>& nameMap) const
{
    auto owner = dynamic_cast<const DocumentObject*>(getContainer());
    if (!owner || !owner->getDocument() || _lValueList.size() != _lSubList.size()) {
        return nullptr;
    }
    std::vector<App::DocumentObject*> values;
    std::vector<std::string> subs;
    auto itSub = _lSubList.begin();
    for (auto itValue = _lValueList.begin(); itValue != _lValueList.end(); ++itValue, ++itSub) {
        auto value = *itValue;
        const auto& sub = *itSub;
        if (!value || !value->isAttachedToDocument()) {
            if (!values.empty()) {
                values.push_back(value);
                subs.push_back(sub);
            }
            continue;
        }
        auto linked = tryImport(owner->getDocument(), value, nameMap);
        auto new_sub = tryImportSubName(value, sub.c_str(), owner->getDocument(), nameMap);
        if (linked != value || !new_sub.empty()) {
            if (values.empty()) {
                values.reserve(_lValueList.size());
                values.insert(values.end(), _lValueList.begin(), itValue);
                subs.reserve(_lSubList.size());
                subs.insert(subs.end(), _lSubList.begin(), itSub);
            }
            values.push_back(linked);
            subs.push_back(std::move(new_sub));
        }
        else if (!values.empty()) {
            values.push_back(linked);
            subs.push_back(sub);
        }
    }
    if (values.empty()) {
        return nullptr;
    }
    std::unique_ptr<PropertyLinkSubList> p(new PropertyLinkSubList);
    p->_lValueList = std::move(values);
    p->_lSubList = std::move(subs);
    return p.release();
}

Property* PropertyLinkSubList::CopyOnLabelChange(App::DocumentObject* obj,
                                                 const std::string& ref,
                                                 const char* newLabel) const
{
    auto owner = dynamic_cast<const DocumentObject*>(getContainer());
    if (!owner || !owner->getDocument()) {
        return nullptr;
    }
    std::vector<App::DocumentObject*> values;
    std::vector<std::string> subs;
    auto itSub = _lSubList.begin();
    for (auto itValue = _lValueList.begin(); itValue != _lValueList.end(); ++itValue, ++itSub) {
        auto value = *itValue;
        const auto& sub = *itSub;
        if (!value || !value->isAttachedToDocument()) {
            if (!values.empty()) {
                values.push_back(value);
                subs.push_back(sub);
            }
            continue;
        }
        auto new_sub = updateLabelReference(value, sub.c_str(), obj, ref, newLabel);
        if (!new_sub.empty()) {
            if (values.empty()) {
                values.reserve(_lValueList.size());
                values.insert(values.end(), _lValueList.begin(), itValue);
                subs.reserve(_lSubList.size());
                subs.insert(subs.end(), _lSubList.begin(), itSub);
            }
            values.push_back(value);
            subs.push_back(std::move(new_sub));
        }
        else if (!values.empty()) {
            values.push_back(value);
            subs.push_back(sub);
        }
    }
    if (values.empty()) {
        return nullptr;
    }
    std::unique_ptr<PropertyLinkSubList> p(new PropertyLinkSubList);
    p->_lValueList = std::move(values);
    p->_lSubList = std::move(subs);
    return p.release();
}

Property* PropertyLinkSubList::CopyOnLinkReplace(const App::DocumentObject* parent,
                                                 App::DocumentObject* oldObj,
                                                 App::DocumentObject* newObj) const
{
    std::vector<App::DocumentObject*> values;
    std::vector<std::string> subs;
    auto itSub = _lSubList.begin();
    std::vector<size_t> positions;
    for (auto itValue = _lValueList.begin(); itValue != _lValueList.end(); ++itValue, ++itSub) {
        auto value = *itValue;
        const auto& sub = *itSub;
        if (!value || !value->isAttachedToDocument()) {
            if (!values.empty()) {
                values.push_back(value);
                subs.push_back(sub);
            }
            continue;
        }
        auto res = tryReplaceLink(getContainer(), value, parent, oldObj, newObj, sub.c_str());
        if (res.first) {
            if (values.empty()) {
                values.reserve(_lValueList.size());
                values.insert(values.end(), _lValueList.begin(), itValue);
                subs.reserve(_lSubList.size());
                subs.insert(subs.end(), _lSubList.begin(), itSub);
            }
            if (res.first == newObj) {
                // check for duplication
                auto itS = subs.begin();
                for (auto itV = values.begin(); itV != values.end();) {
                    if (*itV == res.first && *itS == res.second) {
                        itV = values.erase(itV);
                        itS = subs.erase(itS);
                    }
                    else {
                        ++itV;
                        ++itS;
                    }
                }
                positions.push_back(values.size());
            }
            values.push_back(res.first);
            subs.push_back(std::move(res.second));
        }
        else if (!values.empty()) {
            bool duplicate = false;
            if (value == newObj) {
                for (auto pos : positions) {
                    if (sub == subs[pos]) {
                        duplicate = true;
                        break;
                    }
                }
            }
            if (!duplicate) {
                values.push_back(value);
                subs.push_back(sub);
            }
        }
    }
    if (values.empty()) {
        return nullptr;
    }
    std::unique_ptr<PropertyLinkSubList> p(new PropertyLinkSubList);
    p->_lValueList = std::move(values);
    p->_lSubList = std::move(subs);
    return p.release();
}

Property* PropertyLinkSubList::Copy() const
{
    PropertyLinkSubList* p = new PropertyLinkSubList();
    p->_lValueList = _lValueList;
    p->_lSubList = _lSubList;
    p->_ShadowSubList = _ShadowSubList;
    return p;
}

void PropertyLinkSubList::Paste(const Property& from)
{
    if (!from.isDerivedFrom<PropertyLinkSubList>()) {
        throw Base::TypeError("Incompatible property to paste to");
    }
    auto& link = static_cast<const PropertyLinkSubList&>(from);
    setValues(link._lValueList, link._lSubList, std::vector<ShadowSub>(link._ShadowSubList));
}

unsigned int PropertyLinkSubList::getMemSize() const
{
    unsigned int size =
        static_cast<unsigned int>(_lValueList.size() * sizeof(App::DocumentObject*));
    for (int i = 0; i < getSize(); i++) {
        size += _lSubList[i].size();
    }
    return size;
}

std::vector<std::string> PropertyLinkSubList::getSubValues(bool newStyle) const
{
    assert(_lSubList.size() == _ShadowSubList.size());
    std::vector<std::string> ret;
    ret.reserve(_ShadowSubList.size());
    std::string tmp;
    for (size_t i = 0; i < _ShadowSubList.size(); ++i) {
        ret.push_back(getSubNameWithStyle(_lSubList[i], _ShadowSubList[i], newStyle, tmp));
    }
    return ret;
}

void PropertyLinkSubList::getLinks(std::vector<App::DocumentObject*>& objs,
                                   bool all,
                                   std::vector<std::string>* subs,
                                   bool newStyle) const
{
    if (all || _pcScope != LinkScope::Hidden) {
        objs.reserve(objs.size() + _lValueList.size());
        for (auto obj : _lValueList) {
            if (obj && obj->isAttachedToDocument()) {
                objs.push_back(obj);
            }
        }
        if (subs) {
            auto _subs = getSubValues(newStyle);
            subs->reserve(subs->size() + _subs.size());
            std::move(_subs.begin(), _subs.end(), std::back_inserter(*subs));
        }
    }
}

void PropertyLinkSubList::getLinksTo(std::vector<App::ObjectIdentifier>& identifiers,
                                     App::DocumentObject* obj,
                                     const char* subname,
                                     bool all) const
{
    if (!obj || (!all && _pcScope == LinkScope::Hidden)) {
        return;
    }
    App::SubObjectT objT(obj, subname);
    auto subObject = objT.getSubObject();
    auto subElement = objT.getOldElementName();

    int i = -1;
    for (const auto& docObj : _lValueList) {
        ++i;
        if (docObj != obj) {
            continue;
        }
        // If we don't specify a subname we looking for all; or if the subname is in our
        // property, add this entry to our result
        if (!subname || (i < (int)_lSubList.size() && subname == _lSubList[i])) {
            identifiers.emplace_back(*this, i);
            continue;
        }
        // If we couldn't find any subobjects or this object's index is in our list, ignore it
        if (!subObject || i < (int)_lSubList.size()) {
            continue;
        }
        App::SubObjectT sobjT(obj, _lSubList[i].c_str());
        if (sobjT.getSubObject() == subObject && sobjT.getOldElementName() == subElement) {
            identifiers.emplace_back(*this);
            continue;
        }
        if (i < (int)_ShadowSubList.size()) {
            const auto& shadow = _ShadowSubList[i];
            App::SubObjectT sobjT(obj,
                                  shadow.newName.empty() ? shadow.oldName.c_str()
                                                         : shadow.newName.c_str());
            if (sobjT.getSubObject() == subObject && sobjT.getOldElementName() == subElement) {
                identifiers.emplace_back(*this);
                continue;
            }
        }
    }
}

void PropertyLinkSubList::breakLink(App::DocumentObject* obj, bool clear)
{
    std::vector<DocumentObject*> values;
    std::vector<std::string> subs;

    if (clear && getContainer() == obj) {
        setValues(values, subs);
        return;
    }
    assert(_lValueList.size() == _lSubList.size());

    values.reserve(_lValueList.size());
    subs.reserve(_lSubList.size());

    int i = -1;
    for (auto o : _lValueList) {
        ++i;
        if (o == obj) {
            continue;
        }
        values.push_back(o);
        subs.push_back(_lSubList[i]);
    }
    if (values.size() != _lValueList.size()) {
        setValues(values, subs);
    }
}

bool PropertyLinkSubList::adjustLink(const std::set<App::DocumentObject*>& inList)
{
    if (_pcScope == LinkScope::Hidden) {
        return false;
    }
    auto subs = _lSubList;
    auto links = _lValueList;
    int idx = -1;
    bool touched = false;
    for (std::string& sub : subs) {
        ++idx;
        auto& link = links[idx];
        if (!link || !link->isAttachedToDocument() || !inList.count(link)) {
            continue;
        }
        touched = true;
        size_t pos = sub.find('.');
        for (; pos != std::string::npos; pos = sub.find('.', pos + 1)) {
            auto sobj = link->getSubObject(sub.substr(0, pos + 1).c_str());
            if (!sobj || sobj->getDocument() != link->getDocument()) {
                pos = std::string::npos;
                break;
            }
            if (!inList.count(sobj)) {
                link = sobj;
                sub = sub.substr(pos + 1);
                break;
            }
        }
        if (pos == std::string::npos) {
            return false;
        }
    }
    if (touched) {
        setValues(links, subs);
    }
    return touched;
}

}  // namespace
