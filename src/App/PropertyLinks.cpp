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
#   include <assert.h>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <CXX/Objects.hxx>
#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Base/Writer.h>
#include <Base/Console.h>

#include "DocumentObject.h"
#include "DocumentObjectPy.h"
#include "Document.h"

#include "PropertyLinks.h"

using namespace App;
using namespace Base;
using namespace std;




//**************************************************************************
//**************************************************************************
// PropertyLink
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyLink , App::Property)

//**************************************************************************
// Construction/Destruction


PropertyLink::PropertyLink()
:_pcLink(0)
{

}


PropertyLink::~PropertyLink()
{

}

//**************************************************************************
// Base class implementer

void PropertyLink::setValue(App::DocumentObject * lValue)
{
    aboutToSetValue();
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
    writer.Stream() << writer.ind() << "<Link value=\"" <<  (_pcLink?_pcLink->getNameInDocument():"") <<"\"/>" << std::endl;
}

void PropertyLink::Restore(Base::XMLReader &reader)
{
    // read my element
    reader.readElement("Link");
    // get the value of my attribute
    std::string name = reader.getAttribute("value");

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
    aboutToSetValue();
    _pcLink = dynamic_cast<const PropertyLink&>(from)._pcLink;
    hasSetValue();
}

//**************************************************************************
// PropertyLinkList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyLinkList, App::PropertyLists)

//**************************************************************************
// Construction/Destruction


PropertyLinkList::PropertyLinkList()
{

}

PropertyLinkList::~PropertyLinkList()
{

}

void PropertyLinkList::setSize(int newSize)
{
    _lValueList.resize(newSize);
}

int PropertyLinkList::getSize(void) const
{
    return static_cast<int>(_lValueList.size());
}

void PropertyLinkList::setValue(DocumentObject* lValue)
{
    if (lValue){
        aboutToSetValue();
        _lValueList.resize(1);
        _lValueList[0] = lValue;
        hasSetValue();
    }
}

void PropertyLinkList::setValues(const std::vector<DocumentObject*>& lValue)
{
    aboutToSetValue();
    _lValueList = lValue;
    hasSetValue();
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
        sequence.setItem(i, Py::asObject(_lValueList[i]->getPyObject()));
    }

    return Py::new_reference_to(sequence);
}

void PropertyLinkList::setPyObject(PyObject *value)
{
    if (PyTuple_Check(value) || PyList_Check(value)) {
        Py::Sequence list(value);
        Py::Sequence::size_type size = list.size();
        std::vector<DocumentObject*> values;
        values.resize(size);

        for (Py::Sequence::size_type i = 0; i < size; i++) {
            Py::Object item = list[i];
            if (!PyObject_TypeCheck(*item, &(DocumentObjectPy::Type))) {
                std::string error = std::string("type in list must be 'DocumentObject', not ");
                error += (*item)->ob_type->tp_name;
                throw Base::TypeError(error);
            }

            values[i] = static_cast<DocumentObjectPy*>(*item)->getDocumentObjectPtr();
        }

        setValues(values);
    }
    else if (PyObject_TypeCheck(value, &(DocumentObjectPy::Type))) {
        DocumentObjectPy  *pcObject = static_cast<DocumentObjectPy*>(value);
        setValue(pcObject->getDocumentObjectPtr());
    }
    else {
        std::string error = std::string("type must be 'DocumentObject' or list of 'DocumentObject', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyLinkList::Save(Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<LinkList count=\"" << getSize() << "\">" << endl;
    writer.incInd();
    for (int i = 0; i<getSize(); i++)
        writer.Stream() << writer.ind() << "<Link value=\"" << _lValueList[i]->getNameInDocument() << "\"/>" << endl;;
    writer.decInd();
    writer.Stream() << writer.ind() << "</LinkList>" << endl;
}

void PropertyLinkList::Restore(Base::XMLReader &reader)
{
    // read my element
    reader.readElement("LinkList");
    // get the value of my attribute
    int count = reader.getAttributeAsInteger("count");
    assert(getContainer()->getTypeId().isDerivedFrom(App::DocumentObject::getClassTypeId()));

    std::vector<DocumentObject*> values;
    values.reserve(count);
    for (int i = 0; i < count; i++) {
        reader.readElement("Link");
        std::string name = reader.getAttribute("value");
        // In order to do copy/paste it must be allowed to have defined some
        // referenced objects in XML which do not exist anymore in the new
        // document. Thus, we should silently ingore this.
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
    aboutToSetValue();
    _lValueList = dynamic_cast<const PropertyLinkList&>(from)._lValueList;
    hasSetValue();
}

unsigned int PropertyLinkList::getMemSize(void) const
{
    return static_cast<unsigned int>(_lValueList.size() * sizeof(App::DocumentObject *));
}

//**************************************************************************
// PropertyLinkSub
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyLinkSub , App::Property)

//**************************************************************************
// Construction/Destruction


PropertyLinkSub::PropertyLinkSub()
:_pcLinkSub(0)
{

}


PropertyLinkSub::~PropertyLinkSub()
{

}

//**************************************************************************
// Base class implementer

void PropertyLinkSub::setValue(App::DocumentObject * lValue, const std::vector<std::string> &SubList)
{
    aboutToSetValue();
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

void PropertyLinkSub::Save (Base::Writer &writer) const
{
    const char* internal_name = "";
    // it can happen that the object is still alive but is not part of the document anymore and thus
    // returns 0
    if (_pcLinkSub && _pcLinkSub->getNameInDocument())
        internal_name = _pcLinkSub->getNameInDocument();
    writer.Stream() << writer.ind() << "<LinkSub value=\"" <<  internal_name <<"\" count=\"" <<  _cSubList.size() <<"\">" << std::endl;
    writer.incInd();
    for(unsigned int i = 0;i<_cSubList.size(); i++)
        writer.Stream() << writer.ind() << "<Sub value=\"" <<  _cSubList[i]<<"\"/>" << endl;
    writer.decInd();
    writer.Stream() << writer.ind() << "</LinkSub>" << endl ;
}

void PropertyLinkSub::Restore(Base::XMLReader &reader)
{
    // read my element
    reader.readElement("LinkSub");
    // get the values of my attributes
    std::string name = reader.getAttribute("value");
    int count = reader.getAttributeAsInteger("count");

    // Property not in a DocumentObject!
    assert(getContainer()->getTypeId().isDerivedFrom(App::DocumentObject::getClassTypeId()) );

    std::vector<std::string> values(count);
    for (int i = 0; i < count; i++) {
        reader.readElement("Sub");
        values[i] = reader.getAttribute("value");
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

Property *PropertyLinkSub::Copy(void) const
{
    PropertyLinkSub *p= new PropertyLinkSub();
    p->_pcLinkSub = _pcLinkSub;
    p->_cSubList = _cSubList;
    return p;
}

void PropertyLinkSub::Paste(const Property &from)
{
    aboutToSetValue();
    _pcLinkSub = dynamic_cast<const PropertyLinkSub&>(from)._pcLinkSub;
    _cSubList = dynamic_cast<const PropertyLinkSub&>(from)._cSubList;
    hasSetValue();
}

//**************************************************************************
// PropertyLinkSubList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyLinkSubList , App::PropertyLists)

//**************************************************************************
// Construction/Destruction


PropertyLinkSubList::PropertyLinkSubList()
{

}

PropertyLinkSubList::~PropertyLinkSubList()
{

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
        throw Base::Exception("PropertyLinkSubList::setValues: size of subelements list != size of objects list");
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
        throw Base::Exception("PropertyLinkSubList::setValues: size of subelements list != size of objects list");
    aboutToSetValue();
    _lValueList = lValue;
    _lSubList   = lSubNames;
    hasSetValue();
}

void PropertyLinkSubList::setValue(DocumentObject* lValue, const std::vector<string> &SubList)
{
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

const string PropertyLinkSubList::getPyReprString()
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
            strm << "App.getDocument('" << obj->getDocument()->getName() << "')." << obj->getNameInDocument();
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
        throw Base::Exception("PropertyLinkSubList::getSubListValues: size of subelements list != size of objects list");

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
            "obj=\"" << _lValueList[i]->getNameInDocument() << "\" " <<
            "sub=\"" << _lSubList[i] << "\"/>" << endl;
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
        std::string name = reader.getAttribute("obj");
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
                                    "an object was not loaded correctly\n",name.c_str());
        std::string subName = reader.getAttribute("sub");
        SubNames.push_back(subName);
    }

    reader.readEndElement("LinkSubList");

    // assignment
    setValues(values,SubNames);
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
    aboutToSetValue();
    _lValueList = dynamic_cast<const PropertyLinkSubList&>(from)._lValueList;
    _lSubList   = dynamic_cast<const PropertyLinkSubList&>(from)._lSubList;
    hasSetValue();
}

unsigned int PropertyLinkSubList::getMemSize (void) const
{
   unsigned int size = static_cast<unsigned int>(_lValueList.size() * sizeof(App::DocumentObject *));
   for(int i = 0;i<getSize(); i++)
       size += _lSubList[i].size();

   return size;
}
