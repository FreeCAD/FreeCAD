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

#include <boost/algorithm/string/predicate.hpp>

#include <Base/PyObjectBase.h>
#include <Base/Writer.h>
#include <Base/Reader.h>

#include "Document.h"
#include "DocumentObject.h"
#include "DocumentObjectPy.h"
#include "ObjectIdentifier.h"
#include "PropertyContainer.h"
#include "PropertyLinkBase.h"
#include "PropertyLinkList.h"


FC_LOG_LEVEL_INIT("PropertyLinkList", true, true)

namespace App {


//**************************************************************************
//**************************************************************************
// PropertyLinkListBase
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE_ABSTRACT(App::PropertyLinkListBase, App::PropertyLinkBase)




//**************************************************************************
// PropertyLinkList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyLinkList, App::PropertyLinkListBase)
TYPESYSTEM_SOURCE(App::PropertyLinkListChild, App::PropertyLinkList)
TYPESYSTEM_SOURCE(App::PropertyLinkListGlobal, App::PropertyLinkList)
TYPESYSTEM_SOURCE(App::PropertyLinkListHidden, App::PropertyLinkList)

//**************************************************************************
// Construction/Destruction


PropertyLinkList::PropertyLinkList() = default;

PropertyLinkList::~PropertyLinkList()
{
    // in case this property gety dynamically removed
#ifndef USE_OLD_DAG
    // maintain the back link in the DocumentObject class
    if (_pcScope != LinkScope::Hidden && !_lValueList.empty() && getContainer()
        && getContainer()->isDerivedFrom<App::DocumentObject>()) {
        App::DocumentObject* parent = static_cast<DocumentObject*>(getContainer());
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy)) {
            for (auto* obj : _lValueList) {
                if (obj) {
                    obj->_removeBackLink(parent);
                }
            }
        }
    }
#endif
}

void PropertyLinkList::setSize(int newSize)
{
    for (int i = newSize; i < (int)_lValueList.size(); ++i) {
        auto obj = _lValueList[i];
        if (!obj || !obj->isAttachedToDocument()) {
            continue;
        }
        _nameMap.erase(obj->getNameInDocument());
#ifndef USE_OLD_DAG
        if (_pcScope != LinkScope::Hidden) {
            obj->_removeBackLink(static_cast<DocumentObject*>(getContainer()));
        }
#endif
    }
    _lValueList.resize(newSize);
}

void PropertyLinkList::setSize(int newSize, const_reference def)
{
    auto oldSize = getSize();
    setSize(newSize);
    for (auto i = oldSize; i < newSize; ++i) {
        _lValueList[i] = def;
    }
}

void PropertyLinkList::set1Value(int idx, DocumentObject* const& value)
{
    DocumentObject* obj = nullptr;
    if (idx >= 0 && idx < (int)_lValueList.size()) {
        obj = _lValueList[idx];
        if (obj == value) {
            return;
        }
    }

    if (!value || !value->isAttachedToDocument()) {
        throw Base::ValueError("invalid document object");
    }

    _nameMap.clear();

#ifndef USE_OLD_DAG
    if (getContainer() && getContainer()->isDerivedFrom<App::DocumentObject>()) {
        App::DocumentObject* parent = static_cast<DocumentObject*>(getContainer());
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy) && _pcScope != LinkScope::Hidden) {
            if (obj) {
                obj->_removeBackLink(static_cast<DocumentObject*>(getContainer()));
            }
            if (value) {
                value->_addBackLink(static_cast<DocumentObject*>(getContainer()));
            }
        }
    }
#endif

    inherited::set1Value(idx, value);
}

void PropertyLinkList::setValues(const std::vector<DocumentObject*>& value)
{
    if (value.size() == 1 && !value[0]) {
        // one null element means clear, as backward compatibility for old code
        setValues(std::vector<DocumentObject*>());
        return;
    }

    auto parent = Base::freecad_dynamic_cast<App::DocumentObject>(getContainer());
    for (auto obj : value) {
        if (!obj || !obj->isAttachedToDocument()) {
            throw Base::ValueError("PropertyLinkList: invalid document object");
        }
        if (!testFlag(LinkAllowExternal) && parent && parent->getDocument() != obj->getDocument()) {
            throw Base::ValueError("PropertyLinkList does not support external object");
        }
    }
    _nameMap.clear();

#ifndef USE_OLD_DAG
    // maintain the back link in the DocumentObject class
    if (parent) {
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy) && _pcScope != LinkScope::Hidden) {
            for (auto* obj : _lValueList) {
                if (obj) {
                    obj->_removeBackLink(parent);
                }
            }
            for (auto* obj : value) {
                if (obj) {
                    obj->_addBackLink(parent);
                }
            }
        }
    }
#endif
    inherited::setValues(value);
}

PyObject* PropertyLinkList::getPyObject()
{
    int count = getSize();
#if 0  // FIXME: Should switch to tuple
    Py::Tuple sequence(count);
#else
    Py::List sequence(count);
#endif
    for (int i = 0; i < count; i++) {
        auto obj = _lValueList[i];
        if (obj && obj->isAttachedToDocument()) {
            sequence.setItem(i, Py::asObject(_lValueList[i]->getPyObject()));
        }
        else {
            sequence.setItem(i, Py::None());
        }
    }

    return Py::new_reference_to(sequence);
}

DocumentObject* PropertyLinkList::getPyValue(PyObject* item) const
{
    Base::PyTypeCheck(&item, &DocumentObjectPy::Type);

    return item ? static_cast<DocumentObjectPy*>(item)->getDocumentObjectPtr() : nullptr;
}

void PropertyLinkList::Save(Base::Writer& writer) const
{
    writer.Stream() << writer.ind() << "<LinkList count=\"" << getSize() << "\">" << std::endl;
    writer.incInd();
    for (int i = 0; i < getSize(); i++) {
        DocumentObject* obj = _lValueList[i];
        if (obj) {
            writer.Stream() << writer.ind() << "<Link value=\"" << obj->getExportName() << "\"/>"
                            << std::endl;
        }
        else {
            writer.Stream() << writer.ind() << "<Link value=\"\"/>" << std::endl;
        }
    }

    writer.decInd();
    writer.Stream() << writer.ind() << "</LinkList>" << std::endl;
}

void PropertyLinkList::Restore(Base::XMLReader& reader)
{
    // read my element
    reader.readElement("LinkList");
    // get the value of my attribute
    int count = reader.getAttributeAsInteger("count");
    App::PropertyContainer* container = getContainer();
    if (!container) {
        throw Base::RuntimeError("Property is not part of a container");
    }
    if (!container->isDerivedFrom<App::DocumentObject>()) {
        std::stringstream str;
        str << "Container is not a document object (" << container->getTypeId().getName() << ")";
        throw Base::TypeError(str.str());
    }

    std::vector<DocumentObject*> values;
    values.reserve(count);
    for (int i = 0; i < count; i++) {
        reader.readElement("Link");
        std::string name = reader.getName(reader.getAttribute("value"));
        // In order to do copy/paste it must be allowed to have defined some
        // referenced objects in XML which do not exist anymore in the new
        // document. Thus, we should silently ignore this.
        // Property not in an object!
        DocumentObject* father = static_cast<DocumentObject*>(getContainer());
        App::Document* document = father->getDocument();
        DocumentObject* child = document ? document->getObject(name.c_str()) : nullptr;
        if (child) {
            values.push_back(child);
        }
        else if (reader.isVerbose()) {
            FC_WARN("Lost link to " << (document ? document->getName() : "") << " " << name
                                    << " while loading, maybe an object was not loaded correctly");
        }
    }

    reader.readEndElement("LinkList");

    // assignment
    setValues(values);
}

Property* PropertyLinkList::CopyOnLinkReplace(const App::DocumentObject* parent,
                                              App::DocumentObject* oldObj,
                                              App::DocumentObject* newObj) const
{
    std::vector<DocumentObject*> links;
    bool copied = false;
    bool found = false;
    for (auto it = _lValueList.begin(); it != _lValueList.end(); ++it) {
        auto res = tryReplaceLink(getContainer(), *it, parent, oldObj, newObj);
        if (res.first) {
            found = true;
            if (!copied) {
                copied = true;
                links.insert(links.end(), _lValueList.begin(), it);
            }
            links.push_back(res.first);
        }
        else if (*it == newObj) {
            // in case newObj already exists here, we shall remove all existing
            // entry, and insert it to take over oldObj's position.
            if (!copied) {
                copied = true;
                links.insert(links.end(), _lValueList.begin(), it);
            }
        }
        else if (copied) {
            links.push_back(*it);
        }
    }
    if (!found) {
        return nullptr;
    }
    auto p = new PropertyLinkList();
    p->_lValueList = std::move(links);
    return p;
}

Property* PropertyLinkList::Copy() const
{
    PropertyLinkList* p = new PropertyLinkList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyLinkList::Paste(const Property& from)
{
    if (!from.isDerivedFrom<PropertyLinkList>()) {
        throw Base::TypeError("Incompatible property to paste to");
    }

    setValues(static_cast<const PropertyLinkList&>(from)._lValueList);
}

unsigned int PropertyLinkList::getMemSize() const
{
    return static_cast<unsigned int>(_lValueList.size() * sizeof(App::DocumentObject*));
}


DocumentObject* PropertyLinkList::find(const char* name, int* pindex) const
{
    const int DONT_MAP_UNDER = 10;
    if (!name) {
        return nullptr;
    }
    if (_lValueList.size() <= DONT_MAP_UNDER) {
        int index = -1;
        for (auto obj : _lValueList) {
            ++index;
            if (obj && obj->getNameInDocument() && boost::equals(name, obj->getNameInDocument())) {
                if (pindex) {
                    *pindex = index;
                }
                return obj;
            }
        }
        return nullptr;
    }
    // We're using a map.  Do we need to (re)create it?
    if (_nameMap.empty() || _nameMap.size() > _lValueList.size()) {
        _nameMap.clear();
        for (int i = 0; i < (int)_lValueList.size(); ++i) {
            auto obj = _lValueList[i];
            if (obj && obj->isAttachedToDocument()) {
                _nameMap[obj->getNameInDocument()] = i;
            }
        }
    }
    // Now lookup up in that map
    auto it = _nameMap.find(name);
    if (it == _nameMap.end()) {
        return nullptr;
    }
    if (pindex) {
        *pindex = it->second;
    }
    return _lValueList[it->second];
}

DocumentObject* PropertyLinkList::findUsingMap(const std::string& name, int* pindex) const
{
    if (_nameMap.size() == _lValueList.size()) {
        auto it = _nameMap.find(name);
        if (it == _nameMap.end()) {
            return nullptr;
        }
        if (pindex) {
            *pindex = it->second;
        }
        return _lValueList[it->second];
    }
    return find(name.c_str(), pindex);
}

void PropertyLinkList::getLinks(std::vector<App::DocumentObject*>& objs,
                                bool all,
                                std::vector<std::string>* subs,
                                bool newStyle) const
{
    (void)subs;
    (void)newStyle;
    if (all || _pcScope != LinkScope::Hidden) {
        objs.reserve(objs.size() + _lValueList.size());
        for (auto obj : _lValueList) {
            if (obj && obj->isAttachedToDocument()) {
                objs.push_back(obj);
            }
        }
    }
}

void PropertyLinkList::getLinksTo(std::vector<App::ObjectIdentifier>& identifiers,
                                  App::DocumentObject* obj,
                                  const char* subname,
                                  bool all) const
{
    (void)subname;
    if (!obj || (!all && _pcScope == LinkScope::Hidden)) {
        return;
    }
    int i = -1;
    for (auto docObj : _lValueList) {
        ++i;
        if (docObj == obj) {
            identifiers.emplace_back(*this, i);
            break;
        }
    }
}

void PropertyLinkList::breakLink(App::DocumentObject* obj, bool clear)
{
    if (clear && getContainer() == obj) {
        setValues({});
        return;
    }
    std::vector<App::DocumentObject*> values;
    values.reserve(_lValueList.size());
    for (auto o : _lValueList) {
        if (o != obj) {
            values.push_back(o);
        }
    }
    if (values.size() != _lValueList.size()) {
        setValues(values);
    }
}

bool PropertyLinkList::adjustLink(const std::set<App::DocumentObject*>& inList)
{
    (void)inList;
    return false;
}

}  // namespace App
