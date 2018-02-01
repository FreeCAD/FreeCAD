/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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
# include <assert.h>
# include <sstream>
#endif

#include <QFileInfo>
#include <QDateTime>
#include <QDir>

#include <boost/bind.hpp>

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <CXX/Objects.hxx>
#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Base/Writer.h>
#include <Base/Console.h>

#include "Application.h"
#include "DocumentObject.h"
#include "DocumentObjectPy.h"
#include "Document.h"

#include "PropertyLinks.h"

FC_LOG_LEVEL_INIT("PropertyLinks",true,true);

using namespace App;
using namespace Base;
using namespace std;

//**************************************************************************
//**************************************************************************
// PropertyLink
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyLink , App::Property)
TYPESYSTEM_SOURCE(App::PropertyLinkChild , App::PropertyLink)
TYPESYSTEM_SOURCE(App::PropertyLinkGlobal , App::PropertyLink)

//**************************************************************************
// Construction/Destruction


PropertyLink::PropertyLink()
:_pcLink(0)
{

}


PropertyLink::~PropertyLink()
{
    resetLink();
}

//**************************************************************************
// Base class implementer

void PropertyLink::resetLink() {
    //in case this property gets dynamically removed
#ifndef USE_OLD_DAG
    // maintain the back link in the DocumentObject class if it is from a document object
    if (_pcLink && getContainer() && getContainer()->isDerivedFrom(App::DocumentObject::getClassTypeId())) {
        App::DocumentObject* parent = static_cast<DocumentObject*>(getContainer());
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy)) {
            if (_pcLink)
                _pcLink->_removeBackLink(parent);
        }
    }
#endif
    _pcLink = 0;
}

void PropertyLink::setValue(App::DocumentObject * lValue)
{
    aboutToSetValue();
#ifndef USE_OLD_DAG
    // maintain the back link in the DocumentObject class if it is from a document object
    if (getContainer() && getContainer()->isDerivedFrom(App::DocumentObject::getClassTypeId())) {
        App::DocumentObject* parent = static_cast<DocumentObject*>(getContainer());
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy)) {
            if (_pcLink)
                _pcLink->_removeBackLink(parent);
            if (lValue)
                lValue->_addBackLink(parent);
        }
    }
#endif
    _pcLink=lValue;
    hasSetValue();
}

App::DocumentObject * PropertyLink::getValue(void) const
{
    return _pcLink;
}

App::DocumentObject * PropertyLink::getValue(Base::Type t) const
{
    return (_pcLink && _pcLink->getTypeId().isDerivedFrom(t)) ? _pcLink : 0;
}

PyObject *PropertyLink::getPyObject(void)
{
    if (_pcLink)
        return _pcLink->getPyObject();
    else
        Py_Return;
}

void PropertyLink::setPyObject(PyObject *value)
{
    if (PyObject_TypeCheck(value, &(DocumentObjectPy::Type))) {
        DocumentObjectPy  *pcObject = (DocumentObjectPy*)value;
        setValue(pcObject->getDocumentObjectPtr());
    }
    else if (Py_None == value) {
        setValue(0);
    }
    else {
        std::string error = std::string("type must be 'DocumentObject' or 'NoneType', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyLink::Save (Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<Link value=\"" <<  (_pcLink?_pcLink->getExportName():"") <<"\"/>" << std::endl;
}

void PropertyLink::Restore(Base::XMLReader &reader)
{
    // read my element
    reader.readElement("Link");
    // get the value of my attribute
    std::string name = reader.getName(reader.getAttribute("value"));

    // Property not in a DocumentObject!
    assert(getContainer()->getTypeId().isDerivedFrom(App::DocumentObject::getClassTypeId()) );

    if (name != "") {
        DocumentObject* parent = static_cast<DocumentObject*>(getContainer());

        App::Document* document = parent->getDocument();
        DocumentObject* object = document ? document->getObject(name.c_str()) : 0;
        if (!object) {
            if (reader.isVerbose()) {
                Base::Console().Warning("Lost link to '%s' while loading, maybe "
                                        "an object was not loaded correctly\n",name.c_str());
            }
        }
        else if (parent == object) {
            if (reader.isVerbose()) {
                Base::Console().Warning("Object '%s' links to itself, nullify it\n",name.c_str());
            }
            object = 0;
        }

        setValue(object);
    }
    else {
        setValue(0);
    }
}

Property *PropertyLink::Copy(void) const
{
    PropertyLink *p= new PropertyLink();
    p->_pcLink = _pcLink;
    return p;
}

void PropertyLink::Paste(const Property &from)
{
    if(!from.isDerivedFrom(PropertyLink::getClassTypeId()))
        throw Base::TypeError("Incompatible property to paste to");

    setValue(static_cast<const PropertyLink&>(from)._pcLink);
}

//**************************************************************************
// PropertyLinkList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyLinkList, App::PropertyLists)
TYPESYSTEM_SOURCE(App::PropertyLinkListChild , App::PropertyLinkList)
TYPESYSTEM_SOURCE(App::PropertyLinkListGlobal , App::PropertyLinkList)

//**************************************************************************
// Construction/Destruction


PropertyLinkList::PropertyLinkList()
{

}

PropertyLinkList::~PropertyLinkList()
{
    //in case this property gety dynamically removed
#ifndef USE_OLD_DAG   
    //maintain the back link in the DocumentObject class
    if (!_lValueList.empty() && getContainer() && getContainer()->isDerivedFrom(App::DocumentObject::getClassTypeId())) {
        App::DocumentObject* parent = static_cast<DocumentObject*>(getContainer());
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy)) {
            for(auto *obj : _lValueList)
                obj->_removeBackLink(parent);
        }
    }
#endif

}

void PropertyLinkList::setSize(int newSize)
{
    for(int i=newSize;i<(int)_lValueList.size();++i) {
        auto obj = _lValueList[i];
        if(!obj && !obj->getNameInDocument())
            continue;
        _nameMap.erase(obj->getNameInDocument());
#ifndef USE_OLD_DAG
        obj->_removeBackLink(static_cast<DocumentObject*>(getContainer()));
#endif
    }
    _lValueList.resize(newSize);
}

void PropertyLinkList::setSize(int newSize, const_reference def) {
    auto oldSize = getSize();
    setSize(newSize);
    for(auto i=oldSize;i<newSize;++i)
        _lValueList[i] = def;
}

void PropertyLinkList::set1Value(int idx, DocumentObject* const &value, bool touch) {
    DocumentObject *obj = 0;
    if(idx>=0 && idx<(int)_lValueList.size()) {
        obj = _lValueList[idx];
        if(obj == value) return;
    }

    if(!value || !value->getNameInDocument())
        throw Base::ValueError("invalid document object");

    _nameMap.clear();

#ifndef USE_OLD_DAG   
    if (getContainer() && getContainer()->isDerivedFrom(App::DocumentObject::getClassTypeId())) {
        App::DocumentObject* parent = static_cast<DocumentObject*>(getContainer());
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy)) {
            if(obj) 
                obj->_removeBackLink(static_cast<DocumentObject*>(getContainer()));
            if(value)
                value->_addBackLink(static_cast<DocumentObject*>(getContainer()));
        }
    }
#endif

    _set1Value(idx,value,touch);
}

void PropertyLinkList::setValues(const std::vector<DocumentObject*>& lValue) {
    if(lValue.size()==1 && !lValue[0]) {
        // one null element means clear, as backward compatibility for old code
        setValues(std::vector<DocumentObject*>());
        return;
    }

    for(auto obj : lValue) {
        if(!obj || !obj->getNameInDocument())
            throw Base::ValueError("PropertyLinkList: invalid document object");
    }
    _nameMap.clear();

#ifndef USE_OLD_DAG
    //maintain the back link in the DocumentObject class
    if (getContainer() && getContainer()->isDerivedFrom(App::DocumentObject::getClassTypeId())) {
        App::DocumentObject* parent = static_cast<DocumentObject*>(getContainer());
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy)) {
            for(auto obj : _lValueList)
                if(obj) obj->_removeBackLink(static_cast<DocumentObject*>(getContainer()));
            for(auto obj : lValue)
                if(obj) obj->_addBackLink(static_cast<DocumentObject*>(getContainer()));
        }
    }
#endif
    inherited::setValues(lValue);
}

PyObject *PropertyLinkList::getPyObject(void)
{
    int count = getSize();
#if 0//FIXME: Should switch to tuple
    Py::Tuple sequence(count);
#else
    Py::List sequence(count);
#endif
    for (int i = 0; i<count; i++) {
        auto obj = _lValueList[i];
        if(obj)
            sequence.setItem(i, Py::asObject(_lValueList[i]->getPyObject()));
        else
            sequence.setItem(i, Py::None());
    }

    return Py::new_reference_to(sequence);
}

DocumentObject *PropertyLinkList::getPyValue(PyObject *item) const {
    if (!PyObject_TypeCheck(item, &(DocumentObjectPy::Type))) {
        std::string error = std::string("type must be 'DocumentObject' or list of 'DocumentObject', not ");
        error += item->ob_type->tp_name;
        throw Base::TypeError(error);
    }
    return static_cast<DocumentObjectPy*>(item)->getDocumentObjectPtr();
}

void PropertyLinkList::Save(Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<LinkList count=\"" << getSize() << "\">" << endl;
    writer.incInd();
    for (int i = 0; i<getSize(); i++) {
        DocumentObject* obj = _lValueList[i];
        if (obj)
            writer.Stream() << writer.ind() << "<Link value=\"" << obj->getExportName() << "\"/>" << endl;
        else
            writer.Stream() << writer.ind() << "<Link value=\"\"/>" << endl;
    }

    writer.decInd();
    writer.Stream() << writer.ind() << "</LinkList>" << endl;
}

void PropertyLinkList::Restore(Base::XMLReader &reader)
{
    // read my element
    reader.readElement("LinkList");
    // get the value of my attribute
    int count = reader.getAttributeAsInteger("count");
    App::PropertyContainer* container = getContainer();
    if (!container)
        throw Base::RuntimeError("Property is not part of a container");
    if (!container->getTypeId().isDerivedFrom(App::DocumentObject::getClassTypeId())) {
        std::stringstream str;
        str << "Container is not a document object ("
            << container->getTypeId().getName() << ")";
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
        DocumentObject* child = document ? document->getObject(name.c_str()) : 0;
        if (child)
            values.push_back(child);
        else if (reader.isVerbose())
            Base::Console().Warning("Lost link to '%s' while loading, maybe "
            "an object was not loaded correctly\n", name.c_str());
    }

    reader.readEndElement("LinkList");

    // assignment
    setValues(values);
}

Property *PropertyLinkList::Copy(void) const
{
    PropertyLinkList *p = new PropertyLinkList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyLinkList::Paste(const Property &from)
{
    setValues(dynamic_cast<const PropertyLinkList&>(from)._lValueList);
}

unsigned int PropertyLinkList::getMemSize(void) const
{
    return static_cast<unsigned int>(_lValueList.size() * sizeof(App::DocumentObject *));
}

DocumentObject *PropertyLinkList::find(const char *name, int *pindex) const {
    if(!name) return 0;
    if(_nameMap.empty()) {
        for(int i=0;i<(int)_lValueList.size();++i) {
            auto obj = _lValueList[i];
            if(obj && obj->getNameInDocument()) 
                _nameMap[obj->getNameInDocument()] = i;
        }
    }
    auto it = _nameMap.find(name);
    if(it == _nameMap.end())
        return 0;
    if(pindex) *pindex = it->second;
    return _lValueList[it->second];
}

//**************************************************************************
// PropertyLinkSub
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyLinkSub , App::Property)
TYPESYSTEM_SOURCE(App::PropertyLinkSubChild , App::PropertyLinkSub)
TYPESYSTEM_SOURCE(App::PropertyLinkSubGlobal , App::PropertyLinkSub)

//**************************************************************************
// Construction/Destruction


PropertyLinkSub::PropertyLinkSub()
:_pcLinkSub(0)
{

}


PropertyLinkSub::~PropertyLinkSub()
{
    //in case this property is dynamically removed
#ifndef USE_OLD_DAG
    if (_pcLinkSub && getContainer() && getContainer()->isDerivedFrom(App::DocumentObject::getClassTypeId())) {
        App::DocumentObject* parent = static_cast<DocumentObject*>(getContainer());
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy)) {
            if (_pcLinkSub)
                _pcLinkSub->_removeBackLink(parent);
        }
    }
#endif
}

//**************************************************************************
// Base class implementer

void PropertyLinkSub::setValue(App::DocumentObject * lValue, const std::vector<std::string> &SubList)
{
    aboutToSetValue();
#ifndef USE_OLD_DAG
    if (getContainer() && getContainer()->isDerivedFrom(App::DocumentObject::getClassTypeId())) {
        App::DocumentObject* parent = static_cast<DocumentObject*>(getContainer());
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy)) {
            if (_pcLinkSub)
                _pcLinkSub->_removeBackLink(parent);
            if (lValue)
                lValue->_addBackLink(parent);
        }
    }
#endif
    _pcLinkSub=lValue;
    _cSubList = SubList;
    hasSetValue();
}

App::DocumentObject * PropertyLinkSub::getValue(void) const
{
    return _pcLinkSub;
}

const std::vector<std::string>& PropertyLinkSub::getSubValues(void) const
{
    return _cSubList;
}

std::vector<std::string> PropertyLinkSub::getSubValuesStartsWith(const char* starter) const
{
    std::vector<std::string> temp;
    for(std::vector<std::string>::const_iterator it=_cSubList.begin();it!=_cSubList.end();++it)
        if(strncmp(starter,it->c_str(),strlen(starter))==0)
            temp.push_back(*it);
    return temp;
}

App::DocumentObject * PropertyLinkSub::getValue(Base::Type t) const
{
    return (_pcLinkSub && _pcLinkSub->getTypeId().isDerivedFrom(t)) ? _pcLinkSub : 0;
}

PyObject *PropertyLinkSub::getPyObject(void)
{
    Py::Tuple tup(2);
    Py::List list(static_cast<int>(_cSubList.size()));
    if (_pcLinkSub) {
        _pcLinkSub->getPyObject();
        tup[0] = Py::Object(_pcLinkSub->getPyObject());
        for(unsigned int i = 0;i<_cSubList.size(); i++)
            list[i] = Py::String(_cSubList[i]);
        tup[1] = list;
        return Py::new_reference_to(tup);
    }
    else {
        return Py::new_reference_to(Py::None());
    }
}

void PropertyLinkSub::setPyObject(PyObject *value)
{
    if (PyObject_TypeCheck(value, &(DocumentObjectPy::Type))) {
        DocumentObjectPy  *pcObject = (DocumentObjectPy*)value;
        setValue(pcObject->getDocumentObjectPtr());
    }
    else if (PyTuple_Check(value) || PyList_Check(value)) {
        Py::Sequence seq(value);
        if(seq.size() == 0)
            setValue(NULL);
        else if (PyObject_TypeCheck(seq[0].ptr(), &(DocumentObjectPy::Type))){
            DocumentObjectPy  *pcObj = (DocumentObjectPy*)seq[0].ptr();
            if (seq[1].isString()) {
                std::vector<std::string> vals;
                vals.push_back((std::string)Py::String(seq[1]));
                setValue(pcObj->getDocumentObjectPtr(),vals);
            }
            else if (seq[1].isSequence()) {
                Py::Sequence list(seq[1]);
                std::vector<std::string> vals(list.size());
                unsigned int i=0;
                for (Py::Sequence::iterator it = list.begin();it!=list.end();++it,++i)
                    vals[i] = Py::String(*it);
                setValue(pcObj->getDocumentObjectPtr(),vals);
            }
            else {
                std::string error = std::string("type of second element in tuple must be str or sequence of str");
                throw Base::TypeError(error);
            }
        }
        else {
            std::string error = std::string("type of first element in tuple must be 'DocumentObject', not ");
            error += seq[0].ptr()->ob_type->tp_name;
            throw Base::TypeError(error);
        }
    }
    else if(Py_None == value) {
        setValue(0);
    }
    else {
        std::string error = std::string("type must be 'DocumentObject', 'NoneType' or ('DocumentObject',['String',]) not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

static std::string importSubName(Base::XMLReader &reader, const char *sub) {
    std::ostringstream str;
    for(const char *dot=strchr(sub,'.');dot;sub=dot+1,dot=strchr(sub,'.')) 
        str << reader.getName(std::string(sub,dot-sub).c_str()) << '.';
    str << sub;
    return str.str();
}

static std::string exportSubName(const App::DocumentObject *obj, const char *sub) 
{
    if(!obj || !obj->getNameInDocument() || !obj->getDocument()->isExporting()) 
        return std::string(sub);

    std::ostringstream str;
    for(const char *dot=strchr(sub,'.');dot;sub=dot+1,dot=strchr(sub,'.')) {
        // name with trailing '.'
        auto name = std::string(sub,dot-sub+1);
        obj = obj->getSubObject(name.c_str());
        if(!obj || !obj->getNameInDocument()) {
            FC_ERR("missing sub object '" << name << "' in '" << sub <<"'");
            break;
        }
        if(name.compare(0,name.size()-1,obj->getNameInDocument())==0)
            str << obj->getExportName() << '.';
        else
            str << name;
    }
    str << sub;
    return str.str();
}

static std::string tryImportSubName(const std::map<std::string,std::string> &nameMap, 
        const App::DocumentObject *obj, const char *sub)
{
    if(!obj || !obj->getNameInDocument()) 
        return std::string();

    bool changed = false;
    std::ostringstream str;
    for(const char *dot=strchr(sub,'.');dot;sub=dot+1,dot=strchr(sub,'.')) {
        // name with trailing '.'
        auto name = std::string(sub,dot-sub+1);
        obj = obj->getSubObject(name.c_str());
        if(!obj || !obj->getNameInDocument()) {
            FC_ERR("missing sub object '" << name << "' in '" << sub <<"'");
            break;
        }
        if(name.compare(0,name.size()-1,obj->getNameInDocument())==0) {
            auto export_name = obj->getExportName(true);
            auto it = nameMap.find(export_name);
            if(it!=nameMap.end() && name.compare(0,name.size()-1,it->second)!=0) {
                str << it->second << '.';
                changed = true;
                continue;
            }
        }
        str << name;
    }
    if(changed) {
        str << sub;
        return str.str();
    }
    return std::string();
}

void PropertyLinkSub::Save (Base::Writer &writer) const
{
    std::string internal_name;
    // it can happen that the object is still alive but is not part of the document anymore and thus
    // returns 0
    if (_pcLinkSub && _pcLinkSub->getNameInDocument())
        internal_name = _pcLinkSub->getExportName();
    writer.Stream() << writer.ind() << "<LinkSub value=\"" <<  internal_name <<"\" count=\"" <<  _cSubList.size() <<"\">" << std::endl;
    writer.incInd();
    for(unsigned int i = 0;i<_cSubList.size(); i++)
        writer.Stream() << writer.ind() << "<Sub value=\"" << 
           exportSubName(_pcLinkSub,_cSubList[i].c_str()) <<"\"/>" << endl;
    writer.decInd();
    writer.Stream() << writer.ind() << "</LinkSub>" << endl ;
}

void PropertyLinkSub::Restore(Base::XMLReader &reader)
{
    // read my element
    reader.readElement("LinkSub");
    // get the values of my attributes
    std::string name = reader.getName(reader.getAttribute("value"));
    int count = reader.getAttributeAsInteger("count");

    // Property not in a DocumentObject!
    assert(getContainer()->getTypeId().isDerivedFrom(App::DocumentObject::getClassTypeId()) );

    std::vector<std::string> values(count);
    // Sub may store '.' separated object names, so be aware of the possible mapping when import
    for (int i = 0; i < count; i++) {
        reader.readElement("Sub");
        values[i] = importSubName(reader,reader.getAttribute("value"));
    }

    reader.readEndElement("LinkSub");

    DocumentObject *pcObject;
    if (!name.empty()) {
        App::Document* document = static_cast<DocumentObject*>(getContainer())->getDocument();
        pcObject = document ? document->getObject(name.c_str()) : 0;
        if (!pcObject) {
            if (reader.isVerbose()) {
                Base::Console().Warning("Lost link to '%s' while loading, maybe "
                                        "an object was not loaded correctly\n",name.c_str());
            }
        }
        setValue(pcObject,values);
    }
    else {
       setValue(0);
    }
}

Property *PropertyLinkSub::CopyOnImportExternal(
        const std::map<std::string,std::string> &nameMap) const
{
    if(!_pcLinkSub || !_pcLinkSub->getNameInDocument())
        return 0;

    bool touched = false;
    std::vector<std::string> ret;
    for(const auto &sub : _cSubList) {
        auto new_sub = tryImportSubName(nameMap,_pcLinkSub,sub.c_str());
        if(new_sub.size()) {
            touched = true;
            ret.push_back(new_sub);
        }else
            ret.push_back(sub);
    }
    if(!touched) 
        return 0;

    PropertyLinkSub *p= new PropertyLinkSub();
    p->_pcLinkSub = _pcLinkSub;
    p->_cSubList.swap(ret);
    return p;
}

Property *PropertyLinkSub::Copy(void) const
{
    PropertyLinkSub *p= new PropertyLinkSub();
    p->_pcLinkSub = _pcLinkSub;
    p->_cSubList = _cSubList;
    return p;
}

void PropertyLinkSub::Paste(const Property &from)
{
    setValue(dynamic_cast<const PropertyLinkSub&>(from)._pcLinkSub, dynamic_cast<const PropertyLinkSub&>(from)._cSubList);
}

//**************************************************************************
// PropertyLinkSubList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyLinkSubList , App::PropertyLists)
TYPESYSTEM_SOURCE(App::PropertyLinkSubListChild , App::PropertyLinkSubList)
TYPESYSTEM_SOURCE(App::PropertyLinkSubListGlobal , App::PropertyLinkSubList)

//**************************************************************************
// Construction/Destruction


PropertyLinkSubList::PropertyLinkSubList()
{

}

PropertyLinkSubList::~PropertyLinkSubList()
{
    //in case this property is dynamically removed
#ifndef USE_OLD_DAG
    //maintain backlinks
    if (!_lValueList.empty() && getContainer() && getContainer()->isDerivedFrom(App::DocumentObject::getClassTypeId())) {
        App::DocumentObject* parent = static_cast<DocumentObject*>(getContainer());
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy)) {
            for(auto *obj : _lValueList)
                obj->_removeBackLink(parent);
        }
    }
#endif
}

void PropertyLinkSubList::setSize(int newSize)
{
    _lValueList.resize(newSize);
    _lSubList  .resize(newSize);
}

int PropertyLinkSubList::getSize(void) const
{
    return static_cast<int>(_lValueList.size());
}

void PropertyLinkSubList::setValue(DocumentObject* lValue,const char* SubName)
{
#ifndef USE_OLD_DAG
    //maintain backlinks
    if (getContainer() && getContainer()->isDerivedFrom(App::DocumentObject::getClassTypeId())) {
        App::DocumentObject* parent = static_cast<DocumentObject*>(getContainer());
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy)) {
            for(auto *obj : _lValueList)
                obj->_removeBackLink(parent);
            if (lValue)
                lValue->_addBackLink(parent);
        }
    }
#endif
    
    if (lValue) {
        aboutToSetValue();
        _lValueList.resize(1);
        _lValueList[0]=lValue;
        _lSubList.resize(1);
        _lSubList[0]=SubName;
        hasSetValue();
    }
    else {
        aboutToSetValue();
        _lValueList.clear();
        _lSubList.clear();
        hasSetValue();
    }
}

void PropertyLinkSubList::setValues(const std::vector<DocumentObject*>& lValue,const std::vector<const char*>& lSubNames)
{   
    if (lValue.size() != lSubNames.size())
        throw Base::ValueError("PropertyLinkSubList::setValues: size of subelements list != size of objects list");
    
#ifndef USE_OLD_DAG
    //maintain backlinks. 
    if (getContainer() && getContainer()->isDerivedFrom(App::DocumentObject::getClassTypeId())) {
        App::DocumentObject* parent = static_cast<DocumentObject*>(getContainer());
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy)) {
            //_lValueList can contain items multiple times, but we trust the document
            //object to ensure that this works
            for(auto *obj : _lValueList)
                obj->_removeBackLink(parent);

            //maintain backlinks. lValue can contain items multiple times, but we trust the document
            //object to ensure that the backlink is only added once
            for(auto *obj : lValue)
                obj->_addBackLink(parent);
        }
    }
#endif

    aboutToSetValue();
    _lValueList = lValue;
    _lSubList.resize(lSubNames.size());
    int i = 0;
    for (std::vector<const char*>::const_iterator it = lSubNames.begin();it!=lSubNames.end();++it)
        _lSubList[i]  = *it;
    hasSetValue();
}

void PropertyLinkSubList::setValues(const std::vector<DocumentObject*>& lValue,const std::vector<std::string>& lSubNames)
{
    if (lValue.size() != lSubNames.size())
        throw Base::ValueError("PropertyLinkSubList::setValues: size of subelements list != size of objects list");
    
#ifndef USE_OLD_DAG
    //maintain backlinks. 
    if (getContainer() && getContainer()->isDerivedFrom(App::DocumentObject::getClassTypeId())) {
        App::DocumentObject* parent = static_cast<DocumentObject*>(getContainer());
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy)) {
            //_lValueList can contain items multiple times, but we trust the document
            //object to ensure that this works
            for(auto *obj : _lValueList)
                obj->_removeBackLink(parent);

            //maintain backlinks. lValue can contain items multiple times, but we trust the document
            //object to ensure that the backlink is only added once
            for(auto *obj : lValue)
                obj->_addBackLink(parent);
        }
    }
#endif

    aboutToSetValue();
    _lValueList = lValue;
    _lSubList   = lSubNames;
    hasSetValue();
}

void PropertyLinkSubList::setValue(DocumentObject* lValue, const std::vector<string> &SubList)
{
#ifndef USE_OLD_DAG   
    //maintain backlinks.
    if (getContainer() && getContainer()->isDerivedFrom(App::DocumentObject::getClassTypeId())) {
        App::DocumentObject* parent = static_cast<DocumentObject*>(getContainer());
        // before accessing internals make sure the object is not about to be destroyed
        // otherwise the backlink contains dangling pointers
        if (!parent->testStatus(ObjectStatus::Destroy)) {
            //_lValueList can contain items multiple times, but we trust the document
            //object to ensure that this works
            for(auto *obj : _lValueList)
                obj->_removeBackLink(parent);

            //maintain backlinks. lValue can contain items multiple times, but we trust the document
            //object to ensure that the backlink is only added once
            if (lValue)
                lValue->_addBackLink(parent);
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
            this->_lSubList.push_back(std::string());
        }
    }
    else {
        this->_lSubList = SubList;
        this->_lValueList.insert(this->_lValueList.begin(), size, lValue);
    }
    hasSetValue();
}

const string PropertyLinkSubList::getPyReprString() const
{
    assert(this->_lValueList.size() == this->_lSubList.size());

    if (this->_lValueList.size() == 0)
        return std::string("None");

    std::stringstream strm;
    strm << "[";
    for (std::size_t i = 0; i < this->_lSubList.size(); i++) {
        if (i>0)
            strm << ",(";
        else
            strm << "(";
        App::DocumentObject* obj = this->_lValueList[i];
        if (obj) {
            strm << "App.getDocument('" << obj->getDocument()->getName() 
                 << "').getObject('" << obj->getNameInDocument() << "')";
        } else {
            strm << "None";
        }
        strm << ",";
        strm << "'" << this->_lSubList[i] << "'";
        strm << ")";
    }
    strm << "]";
    return strm.str();
}

DocumentObject *PropertyLinkSubList::getValue() const
{
    App::DocumentObject* ret = 0;
    //FIXME: cache this to avoid iterating each time, to improve speed
    for (std::size_t i = 0; i < this->_lValueList.size(); i++) {
        if (ret == 0)
            ret = this->_lValueList[i];
        if (ret != this->_lValueList[i])
            return 0;
    }
    return ret;
}

int PropertyLinkSubList::removeValue(App::DocumentObject *lValue)
{
    assert(this->_lValueList.size() == this->_lSubList.size());

    std::size_t num = std::count(this->_lValueList.begin(), this->_lValueList.end(), lValue);
    if (num == 0)
        return 0;

    std::vector<DocumentObject*> links;
    std::vector<std::string> subs;
    links.reserve(this->_lValueList.size() - num);
    subs.reserve(this->_lSubList.size() - num);

    for (std::size_t i=0; i<this->_lValueList.size(); ++i) {
        if (this->_lValueList[i] != lValue) {
            links.push_back(this->_lValueList[i]);
            subs.push_back(this->_lSubList[i]);
        }
    }

    setValues(links, subs);
    return static_cast<int>(num);
}

void PropertyLinkSubList::setSubListValues(const std::vector<PropertyLinkSubList::SubSet>& values)
{
    std::vector<DocumentObject*> links;
    std::vector<std::string> subs;

    for (std::vector<PropertyLinkSubList::SubSet>::const_iterator it = values.begin(); it != values.end(); ++it) {
        for (std::vector<std::string>::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
            links.push_back(it->first);
            subs.push_back(*jt);
        }
    }

    setValues(links, subs);
}

std::vector<PropertyLinkSubList::SubSet> PropertyLinkSubList::getSubListValues() const
{
    std::vector<PropertyLinkSubList::SubSet> values;
    if (_lValueList.size() != _lSubList.size())
        throw Base::ValueError("PropertyLinkSubList::getSubListValues: size of subelements list != size of objects list");

    std::map<App::DocumentObject*, std::vector<std::string> > tmp;
    for (std::size_t i = 0; i < _lValueList.size(); i++) {
        App::DocumentObject* link = _lValueList[i];
        std::string sub = _lSubList[i];
        if (tmp.find(link) == tmp.end()) {
            // make sure to keep the same order as in '_lValueList'
            PropertyLinkSubList::SubSet item;
            item.first = link;
            values.push_back(item);
        }

        tmp[link].push_back(sub);
    }

    for (std::vector<PropertyLinkSubList::SubSet>::iterator it = values.begin(); it != values.end(); ++it) {
        it->second = tmp[it->first];
    }

    return values;
}

PyObject *PropertyLinkSubList::getPyObject(void)
{
#if 1
    std::vector<SubSet> subLists = getSubListValues();
    std::size_t count = subLists.size();
#if 0//FIXME: Should switch to tuple
    Py::Tuple sequence(count);
#else
    Py::List sequence(count);
#endif
    for (std::size_t i = 0; i<count; i++) {
        Py::Tuple tup(2);
        tup[0] = Py::Object(subLists[i].first->getPyObject());

        const std::vector<std::string>& sub = subLists[i].second;
        Py::Tuple items(sub.size());
        for (std::size_t j = 0; j < sub.size(); j++) {
            items[j] = Py::String(sub[j]);
        }

        tup[1] = items;
        sequence[i] = tup;
    }

    return Py::new_reference_to(sequence);
#else
    unsigned int count = getSize();
#if 0//FIXME: Should switch to tuple
    Py::Tuple sequence(count);
#else
    Py::List sequence(count);
#endif
    for (unsigned int i = 0; i<count; i++){
        Py::Tuple tup(2);
        tup[0] = Py::Object(_lValueList[i]->getPyObject());
        std::string subItem;
        if (_lSubList.size() > i)
            subItem = _lSubList[i];
        tup[1] = Py::String(subItem);
        sequence[i] = tup;
    }
    return Py::new_reference_to(sequence);
#endif
}

void PropertyLinkSubList::setPyObject(PyObject *value)
{
    try { //try PropertyLinkSub syntax
        PropertyLinkSub dummy;
        dummy.setPyObject(value);
        this->setValue(dummy.getValue(), dummy.getSubValues());
    }
    catch (Base::TypeError) {
        if (PyTuple_Check(value) || PyList_Check(value)) {
            Py::Sequence list(value);
            Py::Sequence::size_type size = list.size();

            std::vector<DocumentObject*> values;
            values.reserve(size);
            std::vector<std::string>     SubNames;
            SubNames.reserve(size);
            for (Py::Sequence::size_type i=0; i<size; i++) {
                Py::Object item = list[i];
                if (item.isTuple()) {
                    Py::Tuple tup(item);
                    if (PyObject_TypeCheck(tup[0].ptr(), &(DocumentObjectPy::Type))){
                        if (tup[1].isString()) {
                            DocumentObjectPy  *pcObj;
                            pcObj = static_cast<DocumentObjectPy*>(tup[0].ptr());
                            values.push_back(pcObj->getDocumentObjectPtr());
                            SubNames.push_back(Py::String(tup[1]));
                        }
                        else if (tup[1].isSequence()) {
                            Py::Sequence list(tup[1]);
                            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                                SubNames.push_back(Py::String(*it));
                            }

                            DocumentObjectPy  *pcObj;
                            pcObj = static_cast<DocumentObjectPy*>(tup[0].ptr());
                            values.insert(values.end(), list.size(), pcObj->getDocumentObjectPtr());
                        }
                    }
                    else {
                        std::string error = std::string("type of first item must be 'DocumentObject', not ");
                        error += Py_TYPE(tup[0].ptr())->tp_name;
                        throw Base::TypeError(error);
                    }
                }
                else if (PyObject_TypeCheck(*item, &(DocumentObjectPy::Type))) {
                    DocumentObjectPy *pcObj;
                    pcObj = static_cast<DocumentObjectPy*>(*item);
                    values.push_back(pcObj->getDocumentObjectPtr());
                }
                else if (item.isString()) {
                    SubNames.push_back(Py::String(item));
                }
            }

            setValues(values,SubNames);
        }
        else {
            std::string error = std::string("type must be 'DocumentObject' or list of 'DocumentObject', not ");
            error += value->ob_type->tp_name;
            throw Base::TypeError(error);
        }
    }
}

void PropertyLinkSubList::Save (Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<LinkSubList count=\"" <<  getSize() <<"\">" << endl;
    writer.incInd();
    for (int i = 0; i < getSize(); i++) {
        writer.Stream() << writer.ind() <<
            "<Link " <<
            "obj=\"" << _lValueList[i]->getExportName() << "\" " <<
            "sub=\"" << exportSubName(_lValueList[i],_lSubList[i].c_str()) << "\"/>" << endl;
    }

    writer.decInd();
    writer.Stream() << writer.ind() << "</LinkSubList>" << endl ;
}

void PropertyLinkSubList::Restore(Base::XMLReader &reader)
{
    // read my element
    reader.readElement("LinkSubList");
    // get the value of my attribute
    int count = reader.getAttributeAsInteger("count");

    std::vector<DocumentObject*> values;
    values.reserve(count);
    std::vector<std::string> SubNames;
    SubNames.reserve(count);
    for (int i = 0; i < count; i++) {
        reader.readElement("Link");
        std::string name = reader.getName(reader.getAttribute("obj"));
        // In order to do copy/paste it must be allowed to have defined some
        // referenced objects in XML which do not exist anymore in the new
        // document. Thus, we should silently ignore this.
        // Property not in an object!
        DocumentObject* father = dynamic_cast<DocumentObject*>(getContainer());
        App::Document* document = father ? father->getDocument() : 0;
        DocumentObject* child = document ? document->getObject(name.c_str()) : 0;
        if (child) {
            values.push_back(child);
            SubNames.push_back(importSubName(reader,reader.getAttribute("sub")));
        } else if (reader.isVerbose())
            Base::Console().Warning("Lost link to '%s' while loading, maybe "
                                    "an object was not loaded correctly\n",name.c_str());
    }

    reader.readEndElement("LinkSubList");

    // assignment
    setValues(values,SubNames);
}

Property *PropertyLinkSubList::CopyOnImportExternal(
        const std::map<std::string,std::string> &nameMap) const
{
    std::unique_ptr<PropertyLinkSubList> p(new PropertyLinkSubList);
    p->_lValueList.reserve(_lValueList.size());
    p->_lSubList.reserve(_lSubList.size());
    bool touched = false;
    for(size_t i=0;i<_lValueList.size();++i) {
        if(!_lValueList[i] || !_lValueList[i]->getNameInDocument())
            continue;
        p->_lValueList.push_back(_lValueList[i]);
        auto new_sub = tryImportSubName(nameMap,_lValueList[i],_lSubList[i].c_str());
        if(new_sub.size()) {
            touched = true;
            p->_lSubList.push_back(new_sub);
        }else
            p->_lSubList.push_back(_lSubList[i]);
    }
    if(!touched) 
        return 0;
    return p.release();
}

Property *PropertyLinkSubList::Copy(void) const
{
    PropertyLinkSubList *p = new PropertyLinkSubList();
    p->_lValueList = _lValueList;
    p->_lSubList   = _lSubList;
    return p;
}

void PropertyLinkSubList::Paste(const Property &from)
{
    setValues(dynamic_cast<const PropertyLinkSubList&>(from)._lValueList, dynamic_cast<const PropertyLinkSubList&>(from)._lSubList);
}

unsigned int PropertyLinkSubList::getMemSize (void) const
{
   unsigned int size = static_cast<unsigned int>(_lValueList.size() * sizeof(App::DocumentObject *));
   for(int i = 0;i<getSize(); i++)
       size += _lSubList[i].size();

   return size;
}
//**************************************************************************
// PropertyXLink::DocInfo
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Key on aboslute path. 
// Becuase of possible symbolic links, multiple entry may refer to the same
// file. We use QFileInfo::canonicalPath to resolve that.
typedef std::map<QString,PropertyXLink::DocInfoPtr> DocInfoMap;
DocInfoMap _DocInfoMap;

class PropertyXLink::DocInfo : 
    public std::enable_shared_from_this<PropertyXLink::DocInfo> 
{
public:
    typedef boost::BOOST_SIGNALS_NAMESPACE::scoped_connection Connection;
    Connection connFinishRestoreDocument;
    Connection connDeleteDocument;
    Connection connSaveDocument;
    Connection connDeletedObject;
    Connection connNewObject;

    DocInfoMap::iterator myPos;
    App::Document *pcDoc;
    std::set<PropertyXLink*> links;

    static std::string getDocPath(
            const char *filename, App::Document *pDoc, bool relative, QString *fullPath = 0) 
    {
        bool absolute;
        // make sure the filename is aboluste path
        QString path = QDir::cleanPath(QString::fromUtf8(filename));
        if((absolute=QFileInfo(path).isAbsolute())) {
            if(fullPath)
                *fullPath = path;
            if(!relative)
                return std::string(path.toUtf8().constData());
        }

        const char *docPath = pDoc->FileName.getValue();
        if(!docPath || *docPath==0)
            throw Base::Exception("Owner document not saved");
        
        QDir docDir(QFileInfo(QString::fromUtf8(docPath)).absoluteDir());
        if(!absolute) {
            path = QDir::cleanPath(docDir.absoluteFilePath(path));
            if(fullPath)
                *fullPath = path;
        }

        if(relative)
            return std::string(docDir.relativeFilePath(path).toUtf8().constData());
        else
            return std::string(path.toUtf8().constData());
    }

    static DocInfoPtr get(const char *filename, App::Document *pDoc, bool relative, PropertyXLink *l) {
        QString path;
        l->filePath = getDocPath(filename,pDoc,relative,&path);
        l->relativePath = relative;

        FC_LOG("finding doc " << filename);

        auto it = _DocInfoMap.find(path);
        DocInfoPtr info;
        if(it != _DocInfoMap.end()) 
            info = it->second;
        else {
            info = std::make_shared<DocInfo>();
            auto ret = _DocInfoMap.insert(std::make_pair(path,info));
            info->init(ret.first);
        }
        info->links.insert(l);
        return info;
    }

    static QString getFullPath(const char *p) {
        if(!p) return QString();
        return QFileInfo(QString::fromUtf8(p)).canonicalFilePath();
    }

    QString getFullPath() const {
        return QFileInfo(myPos->first).canonicalFilePath();
    }

    const char *filePath() const {
        return myPos->first.toUtf8().constData();
    }

    DocInfo()
        :pcDoc(0)
    {}

    ~DocInfo() {
    }

    void deinit() {
        FC_LOG("deinit " << (pcDoc?pcDoc->getName():filePath()));
        assert(links.empty());
        connFinishRestoreDocument.disconnect();
        connDeleteDocument.disconnect();
        connSaveDocument.disconnect();
        connDeletedObject.disconnect();
        connNewObject.disconnect();

        auto me = shared_from_this();
        _DocInfoMap.erase(myPos);
        myPos = _DocInfoMap.end();
        pcDoc = 0;
    }

    void init(DocInfoMap::iterator pos) {
        myPos = pos;
        App::Application &app = App::GetApplication();
        connFinishRestoreDocument = app.signalFinishRestoreDocument.connect(
            boost::bind(&DocInfo::slotFinishRestoreDocument,this,_1));
        connDeleteDocument = app.signalDeleteDocument.connect(
            boost::bind(&DocInfo::slotDeleteDocument,this,_1));
        connSaveDocument = app.signalSaveDocument.connect(
            boost::bind(&DocInfo::slotSaveDocument,this,_1));

        QString fullpath(getFullPath());
        if(fullpath.isEmpty())
            FC_LOG("doc not found " << filePath());
        else{
            for(App::Document *doc : App::GetApplication().getDocuments()) {
                if(getFullPath(doc->FileName.getValue()) == fullpath) {
                    attach(doc);
                    return;
                }
            }
            FC_LOG("document pending " << filePath());
            app.addPendingDocument(fullpath.toUtf8().constData());
        }
    }

    void attach(Document *doc) {
        assert(!pcDoc);
        pcDoc = doc;
        FC_LOG("attaching " << doc->getName() << ", " << doc->FileName.getValue());
        connNewObject = doc->signalNewObject.connect(
                boost::bind(&DocInfo::onNewObject,this,_1));
        connDeletedObject = doc->signalDeletedObject.connect(
                boost::bind(&DocInfo::onDeletedObject,this,_1));
        for(auto it=links.begin(),itNext=it;it!=links.end();it=itNext) {
            ++itNext;
            auto link = *it;
            auto obj = doc->getObject(link->objectName.c_str());
            if(!obj) 
                FC_WARN("object '" << link->objectName << "' not found in document '" 
                        << doc->getName() << "'");
            else
                link->setValue(obj);
        }
    }

    void onNewObject(const DocumentObject &obj) {
        for(auto it=links.begin(),itNext=it;it!=links.end();it=itNext) {
            ++itNext;
            auto link = *it;
            if(!link->_pcLink && link->objectName==obj.getNameInDocument())
                link->setValue(const_cast<DocumentObject*>(&obj));
        }
    }

    void onDeletedObject(const DocumentObject &obj) {
        std::set<PropertyXLink *> tmp;
        tmp.swap(links);
        for(auto it=tmp.begin(),itNext=it;it!=tmp.end();it=itNext) {
            ++itNext;
            auto link = *it;
            if(link->_pcLink==&obj)
                link->setValue(0);
            else if(link->getContainer()!=&obj)
                continue;
            tmp.erase(it);
        }
        if(tmp.empty()) 
            deinit();
        else
            links.swap(tmp);
    }

    void remove(PropertyXLink *l) {
        auto owner = dynamic_cast<DocumentObject*>(l->getContainer());
        if(owner && owner->getNameInDocument())
            FC_LOG("removing " << owner->getNameInDocument());
        auto it = links.find(l);
        if(it != links.end()) {
            links.erase(it);
            if(links.empty())
                deinit();
        }
    }

    void slotFinishRestoreDocument(const App::Document &doc) {
        if(pcDoc) return;
        QString fullpath(getFullPath());
        if(!fullpath.isEmpty() && getFullPath(doc.FileName.getValue())==fullpath)
            attach(const_cast<App::Document*>(&doc));
    }

    void slotSaveDocument(const App::Document &doc) {
        if(!pcDoc) {
            slotFinishRestoreDocument(doc);
            return;
        }
        if(&doc!=pcDoc) return;

        QFileInfo info(myPos->first);
        QString path(info.canonicalFilePath());
        const char *filename = doc.FileName.getValue();
        QString docPath(getFullPath(filename));

        if(path.isEmpty() || path!=docPath) {
            FC_LOG("document '" << doc.getName() << "' path changed");
            auto me = shared_from_this();
            auto ret = _DocInfoMap.insert(std::make_pair(path,me));
            if(!ret.second) {
                // is that even possible?
                FC_WARN("document '" << doc.getName() << "' path exists, detach");
                slotDeleteDocument(doc);
                return;
            }
            _DocInfoMap.erase(myPos);
            myPos = ret.first;

            std::set<PropertyXLink *> tmp;
            tmp.swap(links);
            for(auto link : tmp) {
                auto owner = static_cast<DocumentObject*>(link->getContainer());
                QString path = QString::fromUtf8(link->filePath.c_str());
                bool relative = QFileInfo(path).isRelative();
                // adjust file path for each PropertyXLink
                DocInfo::get(filename,owner->getDocument(),relative?1:0,link);
            }
        }

        // time stamp changed, touch the linking document. Unfortunately, there
        // is no way to setModfied() for an App::Document. We don't want to touch
        // all PropertyXLink for a document, because the linked object is
        // potentially unchanged. So we just touch at most one.
        std::set<Document*> docs;
        for(auto link : links) {
            auto doc = static_cast<DocumentObject*>(link->getContainer())->getDocument();
            auto ret = docs.insert(doc);
            if(ret.second && !doc->isTouched())
                link->touch();
        }
    }

    void slotDeleteDocument(const App::Document &doc) {
        for(auto it=links.begin(),itNext=it;it!=links.end();it=itNext) {
            ++itNext;
            auto link = *it;
            auto obj = dynamic_cast<DocumentObject*>(link->getContainer());
            if(obj && obj->getDocument() == &doc) {
                links.erase(it);
                // must call unlink here, so that PropertyLink::resetLink can
                // remove back link before the owner object is marked as being
                // destroyed
                link->unlink();
            }
        }
        if(links.empty()) {
            deinit();
            return;
        }
        if(pcDoc!=&doc) return;
        for(auto link : links)
            link->detach();
        pcDoc = 0;
    }

    bool hasXLink(const App::Document *doc) const{
        for(auto link : links) {
            auto obj = dynamic_cast<DocumentObject*>(link->getContainer());
            if(obj && obj->getDocument() == doc)
                return true;
        }
        return false;
    }
};

//**************************************************************************
// PropertyXLink
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyXLink , App::PropertyLink)

PropertyXLink::PropertyXLink():relativePath(true)
{

}

static std::map<std::string, std::set<PropertyXLink*> > _LabelMap;
static std::map<PropertyXLink*, std::set<std::string> > _LinkMap;

static void unregisterXLinkLabel(PropertyXLink *link) {
    auto it = _LinkMap.find(link);
    if(it == _LinkMap.end())
        return;
    for(auto &label : it->second) {
        auto itLabel = _LabelMap.find(label);
        if(itLabel!=_LabelMap.end())
            itLabel->second.erase(link);
    }
}

static void registerXLinkLabel(PropertyXLink *link) {
    unregisterXLinkLabel(link);
    const char *dot;
    for(const char *subname=link->getSubName();
        (subname=strchr(subname,'$'))!=0;
        subname=dot+1)
    {
        ++subname;
        dot = strchr(subname,'.');
        if(!dot) break;
        std::string label(subname,dot-subname);
        _LabelMap[label].insert(link);
        _LinkMap[link].insert(label);
    }
}

PropertyXLink::~PropertyXLink() {
    unlink();
    unregisterXLinkLabel(this);
}

std::vector<std::pair<PropertyXLink*,std::string> > 
PropertyXLink::updateLabel(App::DocumentObject *obj, const char *newLabel) {
    std::vector<std::pair<PropertyXLink*,std::string> >  ret;
    auto it = _LabelMap.find(obj->Label.getValue());
    if(it == _LabelMap.end())
        return ret;
    std::string label("$");
    label += obj->Label.getValue();
    label += '.';
    for(auto link : it->second) {
        DocumentObject* parent = dynamic_cast<DocumentObject*>(link->getContainer());
        if(!parent || !parent->getNameInDocument() ||
           !link->_pcLink || !link->_pcLink->getNameInDocument())
            continue;

        // Because the label is allowed to be the same across different
        // hierarchy, we have to search for all occurance, and make sure the
        // referenced sub-object at the found hierarchy is actually the given
        // object.
        for(const char *pos = link->subName.c_str();
            ((pos=strstr(pos,label.c_str()))!=0);
            pos += label.size())
        {
            const char *subname = link->subName.c_str();
            auto sub = link->subName.substr(0,pos+label.size()-subname);
            auto sobj = link->_pcLink->getSubObject(sub.c_str());
            if(sobj == obj) {
                ret.push_back(std::make_pair(link,std::string()));
                auto &change = ret.back();
                change.second = subname;
                change.second.replace(pos+1-subname,label.size()-2,newLabel);
                break;
            }
        }
    }
    return ret;
}

void PropertyXLink::unlink() {
    if(docInfo) {
        docInfo->remove(this);
        docInfo.reset();
    }
    objectName.clear();
    resetLink();
}

void PropertyXLink::detach() {
    if(docInfo) {
        aboutToSetValue();
        resetLink();
        hasSetValue();
    }
}

void PropertyXLink::setSubName(const char *subname, bool transaction) {
    if(transaction)
        aboutToSetValue();
    if(subname)
        subName = subname;
    else
        subName.clear();
    registerXLinkLabel(this);
    if(transaction)
        hasSetValue();
}

void PropertyXLink::setValue(App::DocumentObject * lValue) {
    setValue(lValue,0,true);
}

void PropertyXLink::setValue(App::DocumentObject * lValue, const char *subname, bool relative)
{
    if(!subname) subname = "";
    if(_pcLink==lValue && subName==subname && relative==relativePath)
        return;

    if(lValue && (!lValue->getNameInDocument() || !lValue->getDocument())) {
        throw Base::Exception("Invalid object");
        return;
    }

    auto owner = dynamic_cast<DocumentObject*>(getContainer());
    if(!owner || !owner->getNameInDocument()) 
        throw Base::Exception("invalid container");

    if(lValue == owner)
        throw Base::Exception("self linking");

    DocInfoPtr info;
    const char *name = "";
    if(!subname) subname = "";

    if(lValue) {
        if(lValue->getDocument() != owner->getDocument()) {
            if(!docInfo || 
               lValue->getDocument()!=docInfo->pcDoc || 
                relative!=this->relativePath) 
            {
                const char *filename = lValue->getDocument()->FileName.getValue();
                if(!filename || *filename==0) 
                    throw Base::Exception("Linked document not saved");
                FC_LOG("xlink set to new document " << lValue->getDocument()->getName());
                info = DocInfo::get(filename,owner->getDocument(),relative,this);
                assert(info && info->pcDoc == lValue->getDocument());
            }else
                info = docInfo;
        }
        name = lValue->getNameInDocument();
    }

    aboutToSetValue();
#ifndef USE_OLD_DAG
    if (!owner->testStatus(ObjectStatus::Destroy)) {
        if(_pcLink)
            _pcLink->_removeBackLink(owner);
        if(lValue)
            lValue->_addBackLink(owner);
    }
#endif
    if(docInfo!=info) {
        unlink();
        docInfo = info;
    }
    _pcLink=lValue;
    objectName = name;
    setSubName(subname,false);
    hasSetValue();
}

void PropertyXLink::setValue(
        const char *filename, const char *name, const char *subname, bool relative) 
{
    if(!name || *name==0) {
        setValue(0);
        return;
    }
    auto owner = dynamic_cast<DocumentObject*>(getContainer());
    if(!owner || !owner->getNameInDocument()) 
        throw Base::Exception("invalid container");

    DocumentObject *pObject=0;
    DocInfoPtr info;
    if(filename && *filename!=0) {
        info = DocInfo::get(filename,owner->getDocument(),relative,this);
        if(info->pcDoc) 
            pObject = info->pcDoc->getObject(name);
    }else
        pObject = owner->getDocument()->getObject(name);

    if(pObject || !info) {
        setValue(pObject,subname,relative);
        return;
    }
    aboutToSetValue();
#ifndef USE_OLD_DAG
    if (_pcLink && !owner->testStatus(ObjectStatus::Destroy))
        _pcLink->_removeBackLink(owner);
#endif
    _pcLink = 0;
    if(docInfo!=info) {
        unlink();
        docInfo = info;
    }
    objectName = name;
    setSubName(subname,false);
    hasSetValue();
}

App::Document *PropertyXLink::getDocument() const {
    return docInfo?docInfo->pcDoc:0;
}

const char *PropertyXLink::getDocumentPath() const {
    return docInfo?docInfo->filePath():0;
}

const char *PropertyXLink::getObjectName() const {
    return objectName.c_str();
}

bool PropertyXLink::isRestored() const {
    if(!docInfo) return true;
    return _pcLink && docInfo->pcDoc && 
        stamp==docInfo->pcDoc->LastModifiedDate.getValue();
}

void PropertyXLink::Save (Base::Writer &writer) const {
    auto owner = dynamic_cast<const DocumentObject *>(getContainer());
    if(!owner || !owner->getDocument())
        return;
    auto exporting = owner->getDocument()->isExporting();
    if(_pcLink && exporting==Document::Exporting) {
        // this means, we are exporting with all dependency included. So, store as
        // local object
        writer.Stream() << writer.ind() << "<XLink name=\"" << 
            _pcLink->getExportName() << "\" sub=\"" << 
            exportSubName(_pcLink,subName.c_str()) << 
            "\"/>" << std::endl;
        return;
    }
    const char *path = filePath.c_str();
    std::string _path;
    if(exporting==Document::ExportKeepExternal && relativePath && filePath.size()) {
        // if we are exporting while keeping external reference, try to use
        // aboslute file path for easy transition into document at different
        // directory
        if(docInfo) 
            _path = docInfo->filePath();
        else {
            auto pDoc = owner->getDocument();
            const char *docPath = pDoc->FileName.getValue();
            if(docPath && docPath[0])
                _path = PropertyXLink::DocInfo::getDocPath(filePath.c_str(),pDoc,false);
            else 
                FC_WARN("PropertXLink export without absolute path");
        }
        if(_path.size())
            path = _path.c_str();
    }
    writer.Stream() << writer.ind() << 
        "<XLink file=\"" << path << 
        "\" stamp=\"" << (docInfo&&docInfo->pcDoc?docInfo->pcDoc->LastModifiedDate.getValue():"") <<
        "\" name=\"" << objectName <<
        "\" sub=\"" << subName <<
        "\" relative=\"" << (relativePath?"true":"false") << "\"/>" << std::endl;
}

void PropertyXLink::Restore(Base::XMLReader &reader)
{
    // read my element
    reader.readElement("XLink");
    std::string stamp,file;
    if(reader.hasAttribute("stamp"))
        stamp = reader.getAttribute("stamp");
    if(reader.hasAttribute("file"))
        file = reader.getAttribute("file");
    relativePath = true;
    if(reader.hasAttribute("relative"))
        relativePath = strcmp(reader.getAttribute("relative"),"true")==0;
    std::string name;
    if(file.empty()) 
        name = reader.getName(reader.getAttribute("name"));
    else
        name = reader.getAttribute("name");
    std::string subname;
    if(reader.getAttribute("sub"))
        subname = importSubName(reader,reader.getAttribute("sub"));


    // Property not in a DocumentObject!
    assert(getContainer()->getTypeId().isDerivedFrom(App::DocumentObject::getClassTypeId()));

    if (name.empty()) {
        setValue(0);
        return;
    }

    if(file.size()) {
        this->stamp = stamp;
        setValue(file.c_str(),name.c_str(),subname.c_str(),relativePath);
        return;
    }

    DocumentObject* parent = static_cast<DocumentObject*>(getContainer());
    Document *document = parent->getDocument();
    DocumentObject* object = document ? document->getObject(name.c_str()) : 0;
    if(!object) {
        if(reader.isVerbose()) {
            FC_WARN("Lost link to '" << name << "' while loading, maybe "
                    "an object was not loaded correctly");
        }
    }
    setValue(object,subname.c_str(),relativePath);
}

Property *PropertyXLink::CopyOnImportExternal(
        const std::map<std::string,std::string> &nameMap) const
{
    auto owner = dynamic_cast<const DocumentObject*>(getContainer());
    if(!owner || !_pcLink)
        return 0;
    auto linked = _pcLink;
    auto export_name = linked->getExportName(true);
    auto it = nameMap.find(export_name);
    if(it!=nameMap.end()) {
        linked = owner->getDocument()->getObject(it->second.c_str());
        if(!linked) {
            FC_ERR("cannot find imported external object '" << it->second << "' in '" <<
                    owner->getDocument()->getName());
            linked = _pcLink;
        }
    }
    auto sub = tryImportSubName(nameMap,_pcLink,subName.c_str());

    if(linked==_pcLink && sub.empty())
        return 0;
    auto p = new PropertyXLink;
    p->_pcLink = linked;
    p->subName = sub;
    return p;
}

Property *PropertyXLink::Copy(void) const
{
    PropertyXLink *p= new PropertyXLink();
    p->_pcLink = _pcLink;
    p->docInfo = docInfo;
    p->objectName = objectName;
    p->subName = subName;
    p->filePath = filePath;
    p->relativePath = relativePath;
    return p;
}

void PropertyXLink::Paste(const Property &from)
{
    if(!from.isDerivedFrom(PropertyXLink::getClassTypeId()))
        throw Base::Exception("Incompatible proeprty to paste to");

    const auto &other = static_cast<const PropertyXLink&>(from);
    if(other._pcLink)
        setValue(const_cast<DocumentObject*>(other._pcLink),
                other.subName.c_str(),other.relativePath);
    else
        setValue(other.filePath.c_str(),other.objectName.c_str(),
                other.subName.c_str(),other.relativePath);
}

bool PropertyXLink::hasXLink(const App::Document *doc) {
    for(auto &v : _DocInfoMap) {
        if(v.second->hasXLink(doc))
            return true;
    }
    return false;
}

std::map<App::Document*,std::set<App::Document*> > 
PropertyXLink::getDocumentOutList(App::Document *doc) {
    std::map<App::Document*,std::set<App::Document*> > ret;
    for(auto &v : _DocInfoMap) {
        for(auto link : v.second->links) {
            if(!v.second->pcDoc) continue;
            auto obj = dynamic_cast<App::DocumentObject*>(link->getContainer());
            if(!obj || !obj->getNameInDocument() || !obj->getDocument())
                continue;
            if(doc && obj->getDocument()!=doc)
                continue;
            ret[obj->getDocument()].insert(v.second->pcDoc);
        }
    }
    return ret;
}

std::map<App::Document*,std::set<App::Document*> > 
PropertyXLink::getDocumentInList(App::Document *doc) {
    std::map<App::Document*,std::set<App::Document*> > ret;
    for(auto &v : _DocInfoMap) {
        if(!v.second->pcDoc || (doc && doc!=v.second->pcDoc))
            continue;
        auto &docs = ret[v.second->pcDoc];
        for(auto link : v.second->links) {
            auto obj = dynamic_cast<App::DocumentObject*>(link->getContainer());
            if(obj && obj->getNameInDocument() && obj->getDocument())
                docs.insert(obj->getDocument());
        }
    }
    return ret;
}

PyObject *PropertyXLink::getPyObject(void)
{
    if(!_pcLink)
        Py_Return;
    if(subName.empty())
        return _pcLink->getPyObject();
    Py::Tuple ret(2);
    ret.setItem(0,Py::Object(_pcLink->getPyObject(),true));
    ret.setItem(1,Py::String(subName.c_str()));
    return Py::new_reference_to(ret);
}

void PropertyXLink::setPyObject(PyObject *value) {
    if(PySequence_Check(value)) {
        Py::Sequence seq(value);
        if(seq.size()<2 || seq.size()>3) 
            throw Base::ValueError("Expect input sequence of size 2 or 3");
        std::string subname;
        PyObject *pyObj = seq[0].ptr();
        PyObject *pySub = seq[1].ptr();
        PyObject *pyRelative = seq.size()<3?Py_True:seq[2].ptr();
        if(pyObj == Py_None) {
            setValue(0);
            return;
        } else if(!PyObject_TypeCheck(pyObj, &DocumentObjectPy::Type))
            throw Base::TypeError("Expect the first element to be of 'DocumentObject'");
        if (PyUnicode_Check(pySub)) {
#if PY_MAJOR_VERSION >= 3
            subname = PyUnicode_AsUTF8(pySub);
#else
            PyObject* unicode = PyUnicode_AsUTF8String(pySub);
            subname = PyString_AsString(unicode);
            Py_DECREF(unicode);
        }else if (PyString_Check(pySub)) {
            subname = PyString_AsString(pySub);
#endif
        }else
            throw Base::TypeError("Expect the second element to be a string");
        setValue(static_cast<DocumentObjectPy*>(pyObj)->getDocumentObjectPtr(),
                subname.c_str(),PyObject_IsTrue(pyRelative));
    } else if(PyObject_TypeCheck(value, &(DocumentObjectPy::Type))) {
        setValue(static_cast<DocumentObjectPy*>(value)->getDocumentObjectPtr());
    } else if (Py_None == value) {
        setValue(0);
    } else {
        throw Base::TypeError("type must be 'DocumentObject', 'None', or '(DocumentObject, SubName)' or "
                "'DocumentObject, SubName, useRelativePath)', not ");
    }
}


