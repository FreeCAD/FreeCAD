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

void PropertyExternalGeometryList::setValue(DocumentObject* lValue, const std::vector<string> &SubList, const std::vector<bool> &vbool)
{
    std::vector<bool> lvbool(vbool);

    if(lvbool.size() == 0)
        lvbool.resize(SubList.size(),false);

    if(lvbool.size() != SubList.size())
        throw Base::ValueError("PropertyExternalGeometryList::setValue: size of boolean list != size of SubList list");

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
            this->_lboolList = lvbool;
        }
    }
    else {
        this->_lSubList = SubList;
        this->_lValueList.insert(this->_lValueList.begin(), size, lValue);
        this->_lboolList = lvbool;
    }
    hasSetValue();
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