/***************************************************************************
 *   Copyright (c) 2018 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Base/Writer.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <App/ObjectIdentifier.h>
#include <App/DocumentObject.h>
#include <App/DocumentObjectPy.h>
#include <App/Document.h>

#include "PropertyExternalGeometryList.h"

using namespace App;
using namespace Base;
using namespace std;
using namespace Sketcher;


//**************************************************************************
// PropertyExternalGeometryList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(Sketcher::PropertyExternalGeometryList, App::PropertyLinkSubList);

//**************************************************************************
// Construction/Destruction


PropertyExternalGeometryList::PropertyExternalGeometryList()
{

}

PropertyExternalGeometryList::~PropertyExternalGeometryList()
{

}

void PropertyExternalGeometryList::setSize(int newSize)
{
    PropertyLinkSubList::setSize(newSize);
    _lboolList.resize(newSize);
}

void PropertyExternalGeometryList::setValue(App::DocumentObject* lValue,const char* SubName, bool vbool)
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
        _lboolList.resize(1);
        _lboolList[0]=vbool;
        hasSetValue();
    }
    else {
        aboutToSetValue();
        _lValueList.clear();
        _lSubList.clear();
        _lboolList.clear();
        hasSetValue();
    }
}

void PropertyExternalGeometryList::setValues(const std::vector<DocumentObject*>& lValue,const std::vector<const char*>& lSubNames,
                                             const std::vector<bool> &vbool)
{
    if (lValue.size() != lSubNames.size())
        throw Base::ValueError("PropertyExternalGeometryList::setValues: size of subelements list != size of objects list");

    std::vector<bool> lvbool(vbool);

    if(lvbool.size() == 0)
        lvbool.resize(lValue.size(),false);

    if(lvbool.size() != lValue.size())
        throw Base::ValueError("PropertyExternalGeometryList::setValues: size of boolean list != size of objects list");

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
    for (std::vector<const char*>::const_iterator it = lSubNames.begin();it!=lSubNames.end();++it,++i)
        _lSubList[i]  = *it;
    _lboolList = lvbool;
    hasSetValue();
}

void PropertyExternalGeometryList::setValues(const std::vector<DocumentObject*>& lValue,const std::vector<std::string>& lSubNames,
                                             const std::vector<bool> &vbool)
{
    if (lValue.size() != lSubNames.size())
        throw Base::ValueError("PropertyExternalGeometryList::setValues: size of subelements list != size of objects list");

    std::vector<bool> lvbool(vbool);

    if(lvbool.size() == 0)
        lvbool.resize(lValue.size(),false);

    if(lvbool.size() != lValue.size())
        throw Base::ValueError("PropertyExternalGeometryList::setValues: size of boolean list != size of objects list");

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
    _lboolList = lvbool;
    hasSetValue();
}

void PropertyExternalGeometryList::setValue(DocumentObject* lValue, const std::vector<string> &SubList)
{
     // PropertyLinkSubList interface
  if(this->_lboolList.size() == SubList.size()) { // same number of elements
      PropertyLinkSubList::setValue(lValue, SubList);
  }
  else {
    if(this->_lboolList.size() != 0) 
      throw Base::ValueError("PropertyExternalGeometryList::setValue: PropertyLinkSubList interface disregards previous defining state"); 
    
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
    this->_lboolList.clear();
    if (size == 0) {
        if (lValue) {
            this->_lValueList.push_back(lValue);
            this->_lSubList.push_back(std::string());
            this->_lboolList.push_back(false);
        }
    }
    else {
        this->_lSubList = SubList;
        this->_lValueList.insert(this->_lValueList.begin(), size, lValue);
        this->_lboolList.insert(this->_lboolList.begin(), size, false);
    }
    hasSetValue();
  }
}

void PropertyExternalGeometryList::setValue(App::DocumentObject* lValue,const char* SubList)
{
  // PropertyLinkSubList interface

  if(this->_lboolList.size() == 1) { // retain boolean state
    PropertyLinkSubList::setValue(lValue, SubList);
  }
  else {
    
    if(this->_lboolList.size() != 0)
      throw Base::ValueError("PropertyExternalGeometryList::setValue: PropertyLinkSubList interface disregards previous defining state");
    
    setValue(lValue, SubList, false); 
  }
   
}

void PropertyExternalGeometryList::setValues(const std::vector<App::DocumentObject*>& lValue,const std::vector<const char*>& SubList)
{
   // PropertyLinkSubList interface
  if(this->_lboolList.size() == lValue.size()) { // same number of elements
      PropertyLinkSubList::setValues(lValue, SubList);
  }
  else {
    if(this->_lboolList.size() != 0) 
      throw Base::ValueError("PropertyExternalGeometryList::setValue: PropertyLinkSubList interface disregards previous defining state");      
      
    setValues(lValue, SubList, std::vector<bool>(lValue.size(),false));
  }
}

void PropertyExternalGeometryList::setValues(const std::vector<App::DocumentObject*>& lValue,const std::vector<std::string>& SubList)
{
   // PropertyLinkSubList interface
  if(this->_lboolList.size() == lValue.size()) { // same number of elements
      PropertyLinkSubList::setValues(lValue, SubList);
  }
  else {
    if(this->_lboolList.size() != 0) 
      throw Base::ValueError("PropertyExternalGeometryList::setValue: PropertyLinkSubList interface disregards previous defining state");      
      
    setValues(lValue, SubList, std::vector<bool>(lValue.size(),false));
  }
}

const string PropertyExternalGeometryList::getPyReprString() const
{
    assert(this->_lValueList.size() == this->_lSubList.size());
    assert(this->_lValueList.size() == this->_lboolList.size());

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
        strm << ",";
        strm << "'" << (this->_lboolList[i]?"True":"False") << "'";
        strm << ")";
    }
    strm << "]";
    return strm.str();
}

int PropertyExternalGeometryList::removeValue(App::DocumentObject *lValue)
{
    assert(this->_lValueList.size() == this->_lSubList.size());
    assert(this->_lValueList.size() == this->_lboolList.size());

    std::size_t num = std::count(this->_lValueList.begin(), this->_lValueList.end(), lValue);
    if (num == 0)
        return 0;

    std::vector<DocumentObject*> links;
    std::vector<std::string> subs;
    std::vector<bool> bools;

    links.reserve(this->_lValueList.size() - num);
    subs.reserve(this->_lSubList.size() - num);
    bools.reserve(this->_lboolList.size() - num);

    for (std::size_t i=0; i<this->_lValueList.size(); ++i) {
        if (this->_lValueList[i] != lValue) {
            links.push_back(this->_lValueList[i]);
            subs.push_back(this->_lSubList[i]);
            bools.push_back(this->_lboolList[i]);
        }
    }

    setValues(links, subs, bools);
    return static_cast<int>(num);
}

void PropertyExternalGeometryList::setSubListValues(const std::vector<PropertyExternalGeometryList::SubSet>& values)
{
    std::vector<DocumentObject*> links;
    std::vector<std::string> subs;
    std::vector<bool> bools;

    for (std::vector<PropertyExternalGeometryList::SubSet>::const_iterator it = values.begin(); it != values.end(); ++it) {

        if(std::get<1>(*it).size() != std::get<2>(*it).size())
            throw Base::ValueError("PropertyExternalGeometryList::setSubListValues: size of boolean list != size of SubList list");

        std::vector<std::string>::const_iterator jt = std::get<1>(*it).begin();
        std::vector<bool>::const_iterator kt = std::get<2>(*it).begin();

        for (; jt != std::get<1>(*it).end(); ++jt,++kt) {
            links.push_back(std::get<0>(*it));
            subs.push_back(*jt);
            bools.push_back(*kt);
        }
    }

    setValues(links, subs,bools);
}

void PropertyExternalGeometryList::setSubListValues(const std::vector<PropertyLinkSubList::SubSet>& values)
{
  // PropertyLinkSubList interface
  if(this->_lboolList.size() == values.size()) { // same number of elements
      PropertyLinkSubList::setSubListValues(values);
  }
  else {
    if(this->_lboolList.size() != 0) 
      throw Base::ValueError("PropertyExternalGeometryList::setValue: PropertyLinkSubList interface disregards previous defining state");      
    
    std::vector<PropertyExternalGeometryList::SubSet> ess;
    
    // typedef std::tuple<App::DocumentObject*, std::vector<std::string>, std::vector<bool> > SubSet;
    // typedef std::pair<DocumentObject*, std::vector<std::string> > SubSet;
    
    for(auto &el : values) {
      ess.push_back(std::make_tuple(el.first,el.second,std::vector<bool>(el.second.size(),false)));
    }
    
    
    setSubListValues(ess);
  }
}

std::vector<PropertyExternalGeometryList::SubSet> PropertyExternalGeometryList::getSubListValues() const
{
    std::vector<PropertyExternalGeometryList::SubSet> values;
    if (_lValueList.size() != _lSubList.size())
        throw Base::ValueError("PropertyExternalGeometryList::getSubListValues: size of subelements list != size of objects list");

    if(_lboolList.size() != _lValueList.size())
        throw Base::ValueError("PropertyExternalGeometryList::getSubListValues: size of boolean list != size of objects list");

    std::map<App::DocumentObject*, std::vector<std::string> > tmp;
    std::map<App::DocumentObject*, std::vector<bool> > tmp2;
    std::vector<App::DocumentObject*> keylinks;

    for (std::size_t i = 0; i < _lValueList.size(); i++) {
        App::DocumentObject* link = _lValueList[i];
        std::string sub = _lSubList[i];
        bool vbool = _lboolList[i];
        if (tmp.find(link) == tmp.end()) {
            // make sure to keep the same order as in '_lValueList'
            PropertyExternalGeometryList::SubSet item;
            keylinks.push_back(link);
        }

        tmp[link].push_back(sub);
        tmp2[link].push_back(vbool);
    }

    for (std::vector<App::DocumentObject*>::iterator it = keylinks.begin(); it != keylinks.end(); ++it) {
        values.push_back(std::make_tuple(*it,tmp[*it],tmp2[*it]));
    }

    return values;
}

PyObject *PropertyExternalGeometryList::getPyObject(void)
{
    std::vector<PropertyExternalGeometryList::SubSet> subLists = getSubListValues();
    std::size_t count = subLists.size();

    Py::List sequence(count);
    for (std::size_t i = 0; i<count; i++) {
        Py::Tuple tup(3);
        tup[0] = Py::Object(std::get<0>(subLists[i])->getPyObject());

        const std::vector<std::string>& sub = std::get<1>(subLists[i]);

        Py::Tuple items(sub.size());
        for (std::size_t j = 0; j < sub.size(); j++) {
            items[j] = Py::String(sub[j]);
        }

        tup[1] = items;

        const std::vector<bool>& bools = std::get<2>(subLists[i]);

        Py::Tuple bitems(bools.size());
        for (std::size_t j = 0; j < bools.size(); j++) {
            bitems[j] = Py::Boolean(bools[j]);
        }

        tup[2] = bitems;

        sequence[i] = tup;
    }

    return Py::new_reference_to(sequence);
}

void PropertyExternalGeometryList::setPyObject(PyObject *value)
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

void PropertyExternalGeometryList::Save (Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<LinkSubList count=\"" <<  getSize() <<"\">" << endl;
    writer.incInd();
    for (int i = 0; i < getSize(); i++) {
        writer.Stream() << writer.ind() <<
        "<Link " <<
        "obj=\"" << _lValueList[i]->getNameInDocument() << "\" " <<
        "sub=\"" << _lSubList[i] << "\" " <<
        "vbool=\"" << _lboolList[i] << "\"/>" << endl;
    }

    writer.decInd();
    writer.Stream() << writer.ind() << "</LinkSubList>" << endl ;
}

void PropertyExternalGeometryList::Restore(Base::XMLReader &reader)
{
    // read my element
    reader.readElement("LinkSubList");
    // get the value of my attribute
    int count = reader.getAttributeAsInteger("count");
    
    std::vector<DocumentObject*> values;
    values.reserve(count);
    std::vector<std::string> SubNames;
    SubNames.reserve(count);
    std::vector<bool> bools;
    bools.reserve(count);
    for (int i = 0; i < count; i++) {
        reader.readElement("Link");
        std::string name = reader.getAttribute("obj");
        // In order to do copy/paste it must be allowed to have defined some
        // referenced objects in XML which do not exist anymore in the new
        // document. Thus, we should silently ignore this.
        // Property not in an object!
        DocumentObject* father = dynamic_cast<DocumentObject*>(getContainer());
        App::Document* document = father ? father->getDocument() : 0;
        DocumentObject* child = document ? document->getObject(name.c_str()) : 0;
        if (child)
            values.push_back(child);
        else if (reader.isVerbose())
            Base::Console().Warning("Lost link to '%s' while loading, maybe "
            "an object was not loaded correctly\n",name.c_str());
        std::string subName = reader.getAttribute("sub");
        SubNames.push_back(subName);
        bool vbool = reader.getAttributeAsInteger("vbool")? true : false;
        bools.push_back(vbool);
    }

    reader.readEndElement("LinkSubList");

    // assignment
    setValues(values,SubNames,bools);
}

Property *PropertyExternalGeometryList::Copy(void) const
{
    PropertyExternalGeometryList *p = new PropertyExternalGeometryList();
    p->_lValueList = _lValueList;
    p->_lSubList   = _lSubList;
    p->_lboolList = _lboolList;
    return p;
}

void PropertyExternalGeometryList::Paste(const Property &from)
{
    setValues(dynamic_cast<const PropertyExternalGeometryList&>(from)._lValueList, dynamic_cast<const PropertyExternalGeometryList&>(from)._lSubList, dynamic_cast<const PropertyExternalGeometryList&>(from)._lboolList);
}

unsigned int PropertyExternalGeometryList::getMemSize (void) const
{
    unsigned int size = static_cast<unsigned int>(_lValueList.size() * sizeof(App::DocumentObject *));
    for(int i = 0;i<getSize(); i++)
        size += _lSubList[i].size();

    size += static_cast<unsigned int>(_lboolList.size() * sizeof(bool));
    return size;
}