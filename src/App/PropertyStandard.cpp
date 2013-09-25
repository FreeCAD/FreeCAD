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
# include <boost/version.hpp>
# include <boost/filesystem/path.hpp>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......

#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Base/Writer.h>
#include <Base/Stream.h>

#include "PropertyStandard.h"
#include "MaterialPy.h"
#define new DEBUG_CLIENTBLOCK
using namespace App;
using namespace Base;
using namespace std;




//**************************************************************************
//**************************************************************************
// PropertyInteger
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyInteger , App::Property);

//**************************************************************************
// Construction/Destruction


PropertyInteger::PropertyInteger()
{
    _lValue = 0;
}


PropertyInteger::~PropertyInteger()
{

}

//**************************************************************************
// Base class implementer


void PropertyInteger::setValue(long lValue)
{
    aboutToSetValue();
    _lValue=lValue;
    hasSetValue();
}

long PropertyInteger::getValue(void) const
{
    return _lValue;
}

PyObject *PropertyInteger::getPyObject(void)
{
    return Py_BuildValue("l", _lValue);
}

void PropertyInteger::setPyObject(PyObject *value)
{ 
    if (PyInt_Check(value)) {
        aboutToSetValue();
        _lValue = PyInt_AsLong(value);
        hasSetValue();
    } 
    else {
        std::string error = std::string("type must be int, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyInteger::Save (Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<Integer value=\"" <<  _lValue <<"\"/>" << std::endl;
}

void PropertyInteger::Restore(Base::XMLReader &reader)
{
    // read my Element
    reader.readElement("Integer");
    // get the value of my Attribute
    setValue(reader.getAttributeAsInteger("value"));
}

Property *PropertyInteger::Copy(void) const
{
    PropertyInteger *p= new PropertyInteger();
    p->_lValue = _lValue;
    return p;
}

void PropertyInteger::Paste(const Property &from)
{
    aboutToSetValue();
    _lValue = dynamic_cast<const PropertyInteger&>(from)._lValue;
    hasSetValue();
}


//**************************************************************************
//**************************************************************************
// PropertyPath
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyPath , App::Property);

//**************************************************************************
// Construction/Destruction

PropertyPath::PropertyPath()
{

}

PropertyPath::~PropertyPath()
{

}


//**************************************************************************
// Base class implementer


//**************************************************************************
// Setter/getter for the property

void PropertyPath::setValue(const boost::filesystem::path &Path)
{
    aboutToSetValue();
    _cValue = Path;
    hasSetValue();
}

void PropertyPath::setValue(const char * Path)
{
    aboutToSetValue();
#if (BOOST_VERSION < 104600) || (BOOST_FILESYSTEM_VERSION == 2)
    _cValue = boost::filesystem::path(Path,boost::filesystem::no_check );
    //_cValue = boost::filesystem::path(Path,boost::filesystem::native );
    //_cValue = boost::filesystem::path(Path,boost::filesystem::windows_name );
#else
    _cValue = boost::filesystem::path(Path);
#endif
    hasSetValue();
}

boost::filesystem::path PropertyPath::getValue(void) const
{
    return _cValue;
}

PyObject *PropertyPath::getPyObject(void)
{
#if (BOOST_VERSION < 104600) || (BOOST_FILESYSTEM_VERSION == 2)
    std::string str = _cValue.native_file_string();
#else
    std::string str = _cValue.string();
#endif

    // Returns a new reference, don't increment it!
    PyObject *p = PyUnicode_DecodeUTF8(str.c_str(),str.size(),0);
    if (!p) throw Base::Exception("UTF8 conversion failure at PropertyPath::getPyObject()");
    return p;
}

void PropertyPath::setPyObject(PyObject *value)
{
    std::string path;
    if (PyUnicode_Check(value)) {
        PyObject* unicode = PyUnicode_AsUTF8String(value);
        path = PyString_AsString(unicode);
        Py_DECREF(unicode);
    }
    else if (PyString_Check(value)) {
        path = PyString_AsString(value);
    }
    else {
        std::string error = std::string("type must be str or unicode, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }

    // assign the path
    setValue(path.c_str());
}


void PropertyPath::Save (Base::Writer &writer) const
{
    std::string val = encodeAttribute(_cValue.string());
    writer.Stream() << writer.ind() << "<Path value=\"" <<  val <<"\"/>" << std::endl;
}

void PropertyPath::Restore(Base::XMLReader &reader)
{
    // read my Element
    reader.readElement("Path");
    // get the value of my Attribute
    setValue(reader.getAttribute("value"));
}

Property *PropertyPath::Copy(void) const
{
    PropertyPath *p= new PropertyPath();
    p->_cValue = _cValue;
    return p;
}

void PropertyPath::Paste(const Property &from)
{
    aboutToSetValue();
    _cValue = dynamic_cast<const PropertyPath&>(from)._cValue;
    hasSetValue();
}

unsigned int PropertyPath::getMemSize (void) const
{
    return static_cast<unsigned int>(_cValue.string().size());
}

//**************************************************************************
//**************************************************************************
// PropertyEnumeration
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyEnumeration, App::PropertyInteger);

//**************************************************************************
// Construction/Destruction


PropertyEnumeration::PropertyEnumeration()
  : _CustomEnum(false), _EnumArray(0)
{

}

PropertyEnumeration::~PropertyEnumeration()
{

}

void PropertyEnumeration::setEnums(const char** plEnums)
{
    _EnumArray = plEnums;
# ifdef FC_DEBUG
    if (_EnumArray) {
        // check for NULL termination
        const char* p = *_EnumArray;
        unsigned int i=0;
        while(*(p++) != NULL)i++;
            // very unlikely to have enums with more then 5000 entries!
            assert(i<5000);
    }
# endif
}

void PropertyEnumeration::setValue(const char* value)
{
    // using string methods without set, use setEnums(const char** plEnums) first!
    assert(_EnumArray);

    // set zero if there is no enum array
    if(!_EnumArray){
        PropertyInteger::setValue(0);
        return;
    }

    unsigned int i=0;
    const char** plEnums = _EnumArray;

    // search for the right entry
    while(1){
        // end of list? set zero
        if(*plEnums==NULL){
            PropertyInteger::setValue(0);
            break;
        }
        if(strcmp(*plEnums,value)==0){
            PropertyInteger::setValue(i);
            break;
        }
        plEnums++;
        i++;
    }
}

void PropertyEnumeration::setValue(long value)
{
# ifdef FC_DEBUG
    assert(value>=0 && value<5000);
    if(_EnumArray){
        const char** plEnums = _EnumArray;
        long i=0;
        while(*(plEnums++) != NULL)i++;
        // very unlikely to have enums with more then 5000 entries!
        // Note: Do NOT call assert() because this code might be executed from Python console!
        if ( value < 0 || i <= value )
            throw Base::Exception("Out of range");
    }
# endif
    PropertyInteger::setValue(value);
}

/// checks if the property is set to a certain string value
bool PropertyEnumeration::isValue(const char* value) const
{
    // using string methods without set, use setEnums(const char** plEnums) first!
    assert(_EnumArray);
    return strcmp(_EnumArray[getValue()],value)==0;
}

/// checks if a string is included in the enumeration
bool PropertyEnumeration::isPartOf(const char* value) const
{
    // using string methods without set, use setEnums(const char** plEnums) first!
    assert(_EnumArray);

    const char** plEnums = _EnumArray;

    // search for the right entry
    while(1){
        // end of list?
        if(*plEnums==NULL) 
            return false;
        if(strcmp(*plEnums,value)==0) 
            return true;
        plEnums++;
    }
}

/// get the value as string
const char* PropertyEnumeration::getValueAsString(void) const
{
    // using string methods without set, use setEnums(const char** plEnums) first!
    assert(_EnumArray);
    return _EnumArray[getValue()];
}

std::vector<std::string> PropertyEnumeration::getEnumVector(void) const
{
    // using string methods without set, use setEnums(const char** plEnums) first!
    assert(_EnumArray);

    std::vector<std::string> result;
    const char** plEnums = _EnumArray;

    // end of list?
    while(*plEnums!=NULL){ 
        result.push_back(*plEnums);
        plEnums++;
    }

    return result;
}

void PropertyEnumeration::setEnumVector(const std::vector<std::string>& values)
{
    delete [] _EnumArray;
    _EnumArray = new const char*[values.size()+1];
    int i=0;
    for (std::vector<std::string>::const_iterator it = values.begin(); it != values.end(); ++it) {
#if defined (_MSC_VER)
        _EnumArray[i++] = _strdup(it->c_str());
#else
        _EnumArray[i++] = strdup(it->c_str());
#endif
    }

    _EnumArray[i] = 0; // null termination
}

const char** PropertyEnumeration::getEnums(void) const
{
    return _EnumArray;
}

void PropertyEnumeration::Save(Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<Integer value=\"" <<  _lValue <<"\"";
    if (_CustomEnum)
        writer.Stream() << " CustomEnum=\"true\"";
    writer.Stream() << "/>" << std::endl;
    if (_CustomEnum) {
        std::vector<std::string> items = getEnumVector();
        writer.Stream() << writer.ind() << "<CustomEnumList count=\"" <<  items.size() <<"\">" << endl;
        writer.incInd();
        for(std::vector<std::string>::iterator it = items.begin(); it != items.end(); ++it) {
            std::string val = encodeAttribute(*it);
            writer.Stream() << writer.ind() << "<Enum value=\"" <<  val <<"\"/>" << endl;
        }
        writer.decInd();
        writer.Stream() << writer.ind() << "</CustomEnumList>" << endl;
    }
}

void PropertyEnumeration::Restore(Base::XMLReader &reader)
{
    // read my Element
    reader.readElement("Integer");
    // get the value of my Attribute
    long val = reader.getAttributeAsInteger("value");

    if (reader.hasAttribute("CustomEnum")) {
        reader.readElement("CustomEnumList");
        int count = reader.getAttributeAsInteger("count");
        std::vector<std::string> values(count);
        for(int i = 0; i < count; i++) {
            reader.readElement("Enum");
            values[i] = reader.getAttribute("value");
        }

        reader.readEndElement("CustomEnumList");

        _CustomEnum = true;
        setEnumVector(values);
    }

    setValue(val);
}

PyObject *PropertyEnumeration::getPyObject(void)
{
    if (!_EnumArray) {
        PyErr_SetString(PyExc_AssertionError, "The enum is empty");
        return 0;
    }

    return Py_BuildValue("s", getValueAsString());
}

void PropertyEnumeration::setPyObject(PyObject *value)
{ 
    if (PyInt_Check(value)) {
        long val = PyInt_AsLong(value);
        if (_EnumArray) {
            const char** plEnums = _EnumArray;
            long i=0;
            while(*(plEnums++) != NULL)i++;
            if (val < 0 || i <= val)
                throw Base::ValueError("Out of range");
            PropertyInteger::setValue(val);
        }
    }
    else if (PyString_Check(value)) {
        const char* str = PyString_AsString (value);
        if (_EnumArray && isPartOf(str))
            setValue(PyString_AsString (value));
        else
            throw Base::ValueError("not part of the enum");
    }
    else if (PyList_Check(value)) {
        Py_ssize_t nSize = PyList_Size(value);
        std::vector<std::string> values;
        values.resize(nSize);

        for (Py_ssize_t i=0; i<nSize;++i) {
            PyObject* item = PyList_GetItem(value, i);
            if (!PyString_Check(item)) {
                std::string error = std::string("type in list must be str, not ");
                error += item->ob_type->tp_name;
                throw Base::TypeError(error);
            }
            values[i] = PyString_AsString(item);
        }

        _CustomEnum = true;
        setEnumVector(values);
        setValue((long)0);
    }
    else {
        std::string error = std::string("type must be int or str, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

Property *PropertyEnumeration::Copy(void) const
{
    PropertyEnumeration *p= new PropertyEnumeration();
    p->_lValue = _lValue;
    if (_CustomEnum) {
        p->_CustomEnum = true;
        p->setEnumVector(getEnumVector());
    }
    return p;
}

void PropertyEnumeration::Paste(const Property &from)
{
    aboutToSetValue();
    const PropertyEnumeration& prop = dynamic_cast<const PropertyEnumeration&>(from);
    _lValue = prop._lValue;
    if (prop._CustomEnum) {
        this->_CustomEnum = true;
        this->setEnumVector(prop.getEnumVector());
    }
    hasSetValue();
}

//**************************************************************************
//**************************************************************************
// PropertyIntegerConstraint
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyIntegerConstraint, App::PropertyInteger);

//**************************************************************************
// Construction/Destruction


PropertyIntegerConstraint::PropertyIntegerConstraint()
  : _ConstStruct(0)
{

}


PropertyIntegerConstraint::~PropertyIntegerConstraint()
{

}

void PropertyIntegerConstraint::setConstraints(const Constraints* sConstrain)
{
    _ConstStruct = sConstrain;
}

const PropertyIntegerConstraint::Constraints*  PropertyIntegerConstraint::getConstraints(void) const
{
    return _ConstStruct;
}

void PropertyIntegerConstraint::setPyObject(PyObject *value)
{ 
    if (PyInt_Check(value)) {
        long temp = PyInt_AsLong(value);
        if (_ConstStruct) {
            if (temp > _ConstStruct->UpperBound)
                temp = _ConstStruct->UpperBound;
            else if(temp < _ConstStruct->LowerBound)
                temp = _ConstStruct->LowerBound;
        }
        aboutToSetValue();
        _lValue = temp;
        hasSetValue();
    } 
    else {
        std::string error = std::string("type must be int, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

//**************************************************************************
//**************************************************************************
// PropertyPercent
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyPercent , App::PropertyIntegerConstraint);

const PropertyIntegerConstraint::Constraints percent = {0,100,1};

//**************************************************************************
// Construction/Destruction


PropertyPercent::PropertyPercent()
{
    _ConstStruct = &percent;
}

PropertyPercent::~PropertyPercent()
{
}

//**************************************************************************
//**************************************************************************
// PropertyIntegerList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyIntegerList , App::PropertyLists);

//**************************************************************************
// Construction/Destruction


PropertyIntegerList::PropertyIntegerList()
{

}

PropertyIntegerList::~PropertyIntegerList()
{

}

void PropertyIntegerList::setSize(int newSize)
{
    _lValueList.resize(newSize);
}

int PropertyIntegerList::getSize(void) const
{
    return static_cast<int>(_lValueList.size());
}

//**************************************************************************
// Base class implementer

void PropertyIntegerList::setValue(long lValue)
{
    aboutToSetValue();
    _lValueList.resize(1);
    _lValueList[0]=lValue;
    hasSetValue();
}

void PropertyIntegerList::setValues(const std::vector<long>& values)
{
    aboutToSetValue();
    _lValueList = values;
    hasSetValue();
}

PyObject *PropertyIntegerList::getPyObject(void)
{
    PyObject* list = PyList_New(getSize());
    for(int i = 0;i<getSize(); i++)
        PyList_SetItem( list, i, PyInt_FromLong(_lValueList[i]));
    return list;
}

void PropertyIntegerList::setPyObject(PyObject *value)
{ 
    if (PySequence_Check(value)) {
        Py_ssize_t nSize = PySequence_Size(value);
        std::vector<long> values;
        values.resize(nSize);

        for (Py_ssize_t i=0; i<nSize;++i) {
            PyObject* item =  PySequence_GetItem(value, i);
            if (!PyInt_Check(item)) {
                std::string error = std::string("type in list must be int, not ");
                error += item->ob_type->tp_name;
                throw Base::TypeError(error);
            }
            values[i] = PyInt_AsLong(item);
        }

        setValues(values);
    }
    else if (PyInt_Check(value)) {
        setValue(PyInt_AsLong(value));
    }
    else {
        std::string error = std::string("type must be int or a sequence of int, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyIntegerList::Save (Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<IntegerList count=\"" <<  getSize() <<"\">" << endl;
    writer.incInd();
    for(int i = 0;i<getSize(); i++)
        writer.Stream() << writer.ind() << "<I v=\"" <<  _lValueList[i] <<"\"/>" << endl; ;
    writer.decInd();
    writer.Stream() << writer.ind() << "</IntegerList>" << endl ;
}

void PropertyIntegerList::Restore(Base::XMLReader &reader)
{
    // read my Element
    reader.readElement("IntegerList");
    // get the value of my Attribute
    int count = reader.getAttributeAsInteger("count");
    
    std::vector<long> values(count);
    for(int i = 0; i < count; i++) {
        reader.readElement("I");
        values[i] = reader.getAttributeAsInteger("v");
    }
    
    reader.readEndElement("IntegerList");

    //assignment
    setValues(values);
}

Property *PropertyIntegerList::Copy(void) const
{
    PropertyIntegerList *p= new PropertyIntegerList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyIntegerList::Paste(const Property &from)
{
    aboutToSetValue();
    _lValueList = dynamic_cast<const PropertyIntegerList&>(from)._lValueList;
    hasSetValue();
}

unsigned int PropertyIntegerList::getMemSize (void) const
{
    return static_cast<unsigned int>(_lValueList.size() * sizeof(long));
}




//**************************************************************************
//**************************************************************************
// PropertyIntegerSet
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyIntegerSet , App::Property);

//**************************************************************************
// Construction/Destruction


PropertyIntegerSet::PropertyIntegerSet()
{

}

PropertyIntegerSet::~PropertyIntegerSet()
{

}


//**************************************************************************
// Base class implementer

void PropertyIntegerSet::setValue(long lValue)
{
    aboutToSetValue();
    _lValueSet.clear();
    _lValueSet.insert(lValue);
    hasSetValue();
}

void PropertyIntegerSet::setValues(const std::set<long>& values)
{
    aboutToSetValue();
    _lValueSet = values;
    hasSetValue();
}

PyObject *PropertyIntegerSet::getPyObject(void)
{
    PyObject* set = PySet_New(NULL);
    for(std::set<long>::const_iterator it=_lValueSet.begin();it!=_lValueSet.end();++it)
        PySet_Add(set,PyInt_FromLong(*it));
    return set;
}

void PropertyIntegerSet::setPyObject(PyObject *value)
{ 
    if (PySequence_Check(value)) {
        
        Py_ssize_t nSize = PySequence_Length(value);
        std::set<long> values;

        for (Py_ssize_t i=0; i<nSize;++i) {
            PyObject* item = PySequence_GetItem(value, i);
            if (!PyInt_Check(item)) {
                std::string error = std::string("type in list must be int, not ");
                error += item->ob_type->tp_name;
                throw Base::TypeError(error);
            }
            values.insert(PyInt_AsLong(item));
        }

        setValues(values);
    }
    else if (PyInt_Check(value)) {
        setValue(PyInt_AsLong(value));
    }
    else {
        std::string error = std::string("type must be int or list of int, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyIntegerSet::Save (Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<IntegerSet count=\"" <<  _lValueSet.size() <<"\">" << endl;
    writer.incInd();
    for(std::set<long>::const_iterator it=_lValueSet.begin();it!=_lValueSet.end();++it)
        writer.Stream() << writer.ind() << "<I v=\"" <<  *it <<"\"/>" << endl; ;
    writer.decInd();
    writer.Stream() << writer.ind() << "</IntegerSet>" << endl ;
}

void PropertyIntegerSet::Restore(Base::XMLReader &reader)
{
    // read my Element
    reader.readElement("IntegerSet");
    // get the value of my Attribute
    int count = reader.getAttributeAsInteger("count");
    
    std::set<long> values;
    for(int i = 0; i < count; i++) {
        reader.readElement("I");
        values.insert(reader.getAttributeAsInteger("v"));
    }
    
    reader.readEndElement("IntegerSet");

    //assignment
    setValues(values);
}

Property *PropertyIntegerSet::Copy(void) const
{
    PropertyIntegerSet *p= new PropertyIntegerSet();
    p->_lValueSet = _lValueSet;
    return p;
}

void PropertyIntegerSet::Paste(const Property &from)
{
    aboutToSetValue();
    _lValueSet = dynamic_cast<const PropertyIntegerSet&>(from)._lValueSet;
    hasSetValue();
}

unsigned int PropertyIntegerSet::getMemSize (void) const
{
    return static_cast<unsigned int>(_lValueSet.size() * sizeof(long));
}



//**************************************************************************
//**************************************************************************
// PropertyFloat
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyFloat , App::Property);

//**************************************************************************
// Construction/Destruction


PropertyFloat::PropertyFloat()
{
    _dValue = 0.0;
}

PropertyFloat::~PropertyFloat()
{

}

//**************************************************************************
// Base class implementer

void PropertyFloat::setValue(double lValue)
{
    aboutToSetValue();
    _dValue=lValue;
    hasSetValue();
}

double PropertyFloat::getValue(void) const
{
    return _dValue;
}

PyObject *PropertyFloat::getPyObject(void)
{
    return Py_BuildValue("d", _dValue);
}

void PropertyFloat::setPyObject(PyObject *value)
{
    if (PyFloat_Check(value)) {
        aboutToSetValue();
        _dValue = PyFloat_AsDouble(value);
        hasSetValue();
    }
    else if(PyInt_Check(value)) {
        aboutToSetValue();
        _dValue = PyInt_AsLong(value);
        hasSetValue();
    }
    else {
        std::string error = std::string("type must be float or int, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyFloat::Save (Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<Float value=\"" <<  _dValue <<"\"/>" << std::endl;
}

void PropertyFloat::Restore(Base::XMLReader &reader)
{
    // read my Element
    reader.readElement("Float");
    // get the value of my Attribute
    setValue(reader.getAttributeAsFloat("value"));
}

Property *PropertyFloat::Copy(void) const
{
    PropertyFloat *p= new PropertyFloat();
    p->_dValue = _dValue;
    return p;
}

void PropertyFloat::Paste(const Property &from)
{
    aboutToSetValue();
    _dValue = dynamic_cast<const PropertyFloat&>(from)._dValue;
    hasSetValue();
}

//**************************************************************************
//**************************************************************************
// PropertyFloatConstraint
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyFloatConstraint, App::PropertyFloat);

//**************************************************************************
// Construction/Destruction


PropertyFloatConstraint::PropertyFloatConstraint()
  : _ConstStruct(0)
{

}

PropertyFloatConstraint::~PropertyFloatConstraint()
{

}

void PropertyFloatConstraint::setConstraints(const Constraints* sConstrain)
{
    _ConstStruct = sConstrain;
}

const PropertyFloatConstraint::Constraints*  PropertyFloatConstraint::getConstraints(void) const
{
    return _ConstStruct;
}

void PropertyFloatConstraint::setPyObject(PyObject *value)
{ 
    if (PyFloat_Check(value)) {
        double temp = PyFloat_AsDouble(value);
        if (_ConstStruct) {
            if (temp > _ConstStruct->UpperBound)
                temp = _ConstStruct->UpperBound;
            else if (temp < _ConstStruct->LowerBound)
                temp = _ConstStruct->LowerBound;
        }
    
        aboutToSetValue();
        _dValue = temp;
        hasSetValue();
    } 
    else if (PyInt_Check(value)) {
        double temp = (double)PyInt_AsLong(value);
        if (_ConstStruct) {
            if (temp > _ConstStruct->UpperBound)
                temp = _ConstStruct->UpperBound;
            else if (temp < _ConstStruct->LowerBound)
                temp = _ConstStruct->LowerBound;
        }
    
        aboutToSetValue();
        _dValue = temp;
        hasSetValue();
    } 
    else {
        std::string error = std::string("type must be float, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}


//**************************************************************************
// PropertyFloatList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyFloatList , App::PropertyLists);

//**************************************************************************
// Construction/Destruction


PropertyFloatList::PropertyFloatList()
{

}

PropertyFloatList::~PropertyFloatList()
{

}

//**************************************************************************
// Base class implementer

void PropertyFloatList::setSize(int newSize)
{
    _lValueList.resize(newSize);
}

int PropertyFloatList::getSize(void) const
{
    return static_cast<int>(_lValueList.size());
}

void PropertyFloatList::setValue(double lValue)
{
    aboutToSetValue();
    _lValueList.resize(1);
    _lValueList[0]=lValue;
    hasSetValue();
}

void PropertyFloatList::setValues(const std::vector<double>& values)
{
    aboutToSetValue();
    _lValueList = values;
    hasSetValue();
}

PyObject *PropertyFloatList::getPyObject(void)
{
    PyObject* list = PyList_New(getSize());
    for (int i = 0;i<getSize(); i++)
         PyList_SetItem( list, i, PyFloat_FromDouble(_lValueList[i]));
    return list;
}

void PropertyFloatList::setPyObject(PyObject *value)
{ 
    if (PyList_Check(value)) {
        Py_ssize_t nSize = PyList_Size(value);
        std::vector<double> values;
        values.resize(nSize);

        for (Py_ssize_t i=0; i<nSize;++i) {
            PyObject* item = PyList_GetItem(value, i);
            if (!PyFloat_Check(item)) {
                std::string error = std::string("type in list must be float, not ");
                error += item->ob_type->tp_name;
                throw Base::TypeError(error);
            }
            
            values[i] = PyFloat_AsDouble(item);
        }

        setValues(values);
    }
    else if (PyFloat_Check(value)) {
        setValue(PyFloat_AsDouble(value));
    } 
    else {
        std::string error = std::string("type must be float or list of float, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyFloatList::Save (Base::Writer &writer) const
{
    if (writer.isForceXML()) {
        writer.Stream() << writer.ind() << "<FloatList count=\"" <<  getSize() <<"\">" << endl;
        writer.incInd();
        for(int i = 0;i<getSize(); i++)
            writer.Stream() << writer.ind() << "<F v=\"" <<  _lValueList[i] <<"\"/>" << endl; ;
        writer.decInd();
        writer.Stream() << writer.ind() <<"</FloatList>" << endl ;
    }
    else {
        writer.Stream() << writer.ind() << "<FloatList file=\"" << 
        writer.addFile(getName(), this) << "\"/>" << std::endl;
    }
}

void PropertyFloatList::Restore(Base::XMLReader &reader)
{
    reader.readElement("FloatList");
    string file (reader.getAttribute("file") );

    if (!file.empty()) {
        // initate a file read
        reader.addFile(file.c_str(),this);
    }
}

void PropertyFloatList::SaveDocFile (Base::Writer &writer) const
{
    Base::OutputStream str(writer.Stream());
    uint32_t uCt = (uint32_t)getSize();
    str << uCt;
    if (writer.getFileVersion() > 0) {
        for (std::vector<double>::const_iterator it = _lValueList.begin(); it != _lValueList.end(); ++it) {
            str << *it;
        }
    }
    else {
        for (std::vector<double>::const_iterator it = _lValueList.begin(); it != _lValueList.end(); ++it) {
            float v = (float)*it;
            str << v;
        }
    }
}

void PropertyFloatList::RestoreDocFile(Base::Reader &reader)
{
    Base::InputStream str(reader);
    uint32_t uCt=0;
    str >> uCt;
    std::vector<double> values(uCt);
    if (reader.getFileVersion() > 0) {
        for (std::vector<double>::iterator it = values.begin(); it != values.end(); ++it) {
            str >> *it;
        }
    }
    else {
        for (std::vector<double>::iterator it = values.begin(); it != values.end(); ++it) {
            float val;
            str >> val;
            (*it) = val;
        }
    }
    setValues(values);
}

Property *PropertyFloatList::Copy(void) const
{
    PropertyFloatList *p= new PropertyFloatList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyFloatList::Paste(const Property &from)
{
    aboutToSetValue();
    _lValueList = dynamic_cast<const PropertyFloatList&>(from)._lValueList;
    hasSetValue();
}

unsigned int PropertyFloatList::getMemSize (void) const
{
    return static_cast<unsigned int>(_lValueList.size() * sizeof(double));
}

//**************************************************************************
//**************************************************************************
// PropertyString
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyString , App::Property);

PropertyString::PropertyString()
{

}

PropertyString::~PropertyString()
{

}

void PropertyString::setValue(const char* sString)
{
    if (sString) {
        aboutToSetValue();
        _cValue = sString;
        hasSetValue();
    }
}

void PropertyString::setValue(const std::string &sString)
{
    aboutToSetValue();
    _cValue = sString;
    hasSetValue();
}

const char* PropertyString::getValue(void) const
{
    return _cValue.c_str();
}

PyObject *PropertyString::getPyObject(void)
{
    PyObject *p = PyUnicode_DecodeUTF8(_cValue.c_str(),_cValue.size(),0);
    if (!p) throw Base::Exception("UTF8 conversion failure at PropertyString::getPyObject()");
    return p;
}

void PropertyString::setPyObject(PyObject *value)
{
    std::string string;
    if (PyUnicode_Check(value)) {
        PyObject* unicode = PyUnicode_AsUTF8String(value);
        string = PyString_AsString(unicode);
        Py_DECREF(unicode);
    }
    else if (PyString_Check(value)) {
        string = PyString_AsString(value);
    }
    else {
        std::string error = std::string("type must be str or unicode, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }

    // assign the string
    setValue(string);
}

void PropertyString::Save (Base::Writer &writer) const
{
    std::string val = encodeAttribute(_cValue);
    writer.Stream() << writer.ind() << "<String value=\"" <<  val <<"\"/>" << std::endl;
}

void PropertyString::Restore(Base::XMLReader &reader)
{
    // read my Element
    reader.readElement("String");
    // get the value of my Attribute
    setValue(reader.getAttribute("value"));
}

Property *PropertyString::Copy(void) const
{
    PropertyString *p= new PropertyString();
    p->_cValue = _cValue;
    return p;
}

void PropertyString::Paste(const Property &from)
{
    aboutToSetValue();
    _cValue = dynamic_cast<const PropertyString&>(from)._cValue;
    hasSetValue();
}

unsigned int PropertyString::getMemSize (void) const
{
    return static_cast<unsigned int>(_cValue.size());
}

//**************************************************************************
//**************************************************************************
// PropertyUUID
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyUUID , App::Property);

PropertyUUID::PropertyUUID()
{

}

PropertyUUID::~PropertyUUID()
{

}

void PropertyUUID::setValue(const Base::Uuid &id)
{
    aboutToSetValue();
    _uuid = id;
    hasSetValue();
}

void PropertyUUID::setValue(const char* sString)
{
    if (sString) {
        aboutToSetValue();
        _uuid.setValue(sString);
        hasSetValue();
    }
}

void PropertyUUID::setValue(const std::string &sString)
{
    aboutToSetValue();
    _uuid.setValue(sString);
    hasSetValue();
}

const std::string& PropertyUUID::getValueStr(void) const
{
    return _uuid.getValue();
}

const Base::Uuid& PropertyUUID::getValue(void) const
{
    return _uuid;
}

PyObject *PropertyUUID::getPyObject(void)
{
    PyObject *p = PyString_FromString(_uuid.getValue().c_str());
    return p;
}

void PropertyUUID::setPyObject(PyObject *value)
{
    std::string string;
    if (PyString_Check(value)) {
        string = PyString_AsString(value);
    }
    else {
        std::string error = std::string("type must be a str, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }

    try {
        // assign the string
        Base::Uuid uid;
        uid.setValue(string);
        setValue(uid);
    }
    catch (const std::exception& e) {
        throw Base::RuntimeError(e.what());
    }
}

void PropertyUUID::Save (Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<Uuid value=\"" << _uuid.getValue() <<"\"/>" << std::endl;
}

void PropertyUUID::Restore(Base::XMLReader &reader)
{
    // read my Element
    reader.readElement("Uuid");
    // get the value of my Attribute
    setValue(reader.getAttribute("value"));
}

Property *PropertyUUID::Copy(void) const
{
    PropertyUUID *p= new PropertyUUID();
    p->_uuid = _uuid;
    return p;
}

void PropertyUUID::Paste(const Property &from)
{
    aboutToSetValue();
    _uuid = dynamic_cast<const PropertyUUID&>(from)._uuid;
    hasSetValue();
}

unsigned int PropertyUUID::getMemSize (void) const
{
    return static_cast<unsigned int>(sizeof(_uuid));
}

//**************************************************************************
// PropertyFont
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyFont , App::PropertyString);

PropertyFont::PropertyFont()
{

}

PropertyFont::~PropertyFont()
{

}

//**************************************************************************
// PropertyStringList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyStringList , App::PropertyLists);

PropertyStringList::PropertyStringList()
{

}

PropertyStringList::~PropertyStringList()
{

}

//**************************************************************************
// Base class implementer

void PropertyStringList::setSize(int newSize)
{
    _lValueList.resize(newSize);
}

int PropertyStringList::getSize(void) const
{
    return static_cast<int>(_lValueList.size());
}

void PropertyStringList::setValue(const std::string& lValue)
{
    aboutToSetValue();
    _lValueList.resize(1);
    _lValueList[0]=lValue;
    hasSetValue();
}

void PropertyStringList::setValues(const std::vector<std::string>& lValue)
{
    aboutToSetValue();
    _lValueList=lValue;
    hasSetValue();
}

void PropertyStringList::setValues(const std::list<std::string>& lValue)
{
    aboutToSetValue();
    _lValueList.resize(lValue.size());
    std::copy(lValue.begin(), lValue.end(), _lValueList.begin());
    hasSetValue();
}

PyObject *PropertyStringList::getPyObject(void)
{
    PyObject* list = PyList_New(getSize());

    for (int i = 0;i<getSize(); i++) {
        PyObject* item = PyUnicode_DecodeUTF8(_lValueList[i].c_str(), _lValueList[i].size(), 0);
        if (!item) {
            Py_DECREF(list);
            throw Base::Exception("UTF8 conversion failure at PropertyStringList::getPyObject()");
        }
        PyList_SetItem(list, i, item);
    }

    return list;
}

void PropertyStringList::setPyObject(PyObject *value)
{
    if (PyList_Check(value)) {
        Py_ssize_t nSize = PyList_Size(value);
        std::vector<std::string> values;
        values.resize(nSize);

        for (Py_ssize_t i=0; i<nSize;++i) {
            PyObject* item = PyList_GetItem(value, i);
            if (PyUnicode_Check(item)) {
                PyObject* unicode = PyUnicode_AsUTF8String(item);
                values[i] = PyString_AsString(unicode);
                Py_DECREF(unicode);
            }
            else if (PyString_Check(item)) {
                values[i] = PyString_AsString(item);
            }
            else {
                std::string error = std::string("type in list must be str or unicode, not ");
                error += item->ob_type->tp_name;
                throw Base::TypeError(error);
            }
        }
        
        setValues(values);
    }
    else if (PyString_Check(value)) {
        setValue(PyString_AsString(value));
    }
    else {
        std::string error = std::string("type must be str or list of str, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

unsigned int PropertyStringList::getMemSize (void) const
{
    size_t size=0;
    for(int i = 0;i<getSize(); i++) 
        size += _lValueList[i].size();
    return static_cast<unsigned int>(size);
}

void PropertyStringList::Save (Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<StringList count=\"" <<  getSize() <<"\">" << endl;
    writer.incInd();
    for(int i = 0;i<getSize(); i++) {
        std::string val = encodeAttribute(_lValueList[i]);
        writer.Stream() << writer.ind() << "<String value=\"" <<  val <<"\"/>" << endl;
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</StringList>" << endl ;
}

void PropertyStringList::Restore(Base::XMLReader &reader)
{
    // read my Element
    reader.readElement("StringList");
    // get the value of my Attribute
    int count = reader.getAttributeAsInteger("count");

    std::vector<std::string> values(count);
    for(int i = 0; i < count; i++) {
        reader.readElement("String");
        values[i] = reader.getAttribute("value");
    }
    
    reader.readEndElement("StringList");

    // assignment
    setValues(values);
}

Property *PropertyStringList::Copy(void) const
{
    PropertyStringList *p= new PropertyStringList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyStringList::Paste(const Property &from)
{
    aboutToSetValue();
    _lValueList = dynamic_cast<const PropertyStringList&>(from)._lValueList;
    hasSetValue();
}


//**************************************************************************
// PropertyMap
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyMap , App::Property);

PropertyMap::PropertyMap()
{

}

PropertyMap::~PropertyMap()
{

}

//**************************************************************************
// Base class implementer


int PropertyMap::getSize(void) const
{
    return static_cast<int>(_lValueList.size());
}

void PropertyMap::setValue(const std::string& key,const std::string& value)
{
    aboutToSetValue();
    _lValueList[key] = value;
    hasSetValue();
}

void PropertyMap::setValues(const std::map<std::string,std::string>& map)
{
    aboutToSetValue();
    _lValueList=map;
    hasSetValue();
}



const std::string& PropertyMap::operator[] (const std::string& key) const 
{
    static std::string empty;
    std::map<std::string,std::string>::const_iterator it = _lValueList.find(key);
    if(it!=_lValueList.end())
        return it->second;
    else
        return empty;
} 


PyObject *PropertyMap::getPyObject(void)
{
    PyObject* dict = PyDict_New();

    for (std::map<std::string,std::string>::const_iterator it = _lValueList.begin();it!= _lValueList.end(); ++it) {
        PyObject* item = PyUnicode_DecodeUTF8(it->second.c_str(), it->second.size(), 0);
        if (!item) {
            Py_DECREF(dict);
            throw Base::Exception("UTF8 conversion failure at PropertyMap::getPyObject()");
        }
        PyDict_SetItemString(dict,it->first.c_str(),item);
    }

    return dict;
}

void PropertyMap::setPyObject(PyObject *value)
{
    if (PyDict_Check(value)) {

        std::map<std::string,std::string> values;
        // get key and item list
        PyObject* keyList = PyDict_Keys(value);

        PyObject* itemList = PyDict_Values(value);
        Py_ssize_t nSize = PyList_Size(keyList);

        for (Py_ssize_t i=0; i<nSize;++i) {

            // check on the key:
            std::string keyStr;
            PyObject* key = PyList_GetItem(keyList, i);
            if (PyString_Check(key)) {
                keyStr = PyString_AsString(key);
            }
            else {
                std::string error = std::string("type of the key need to be a string, not");
                error += key->ob_type->tp_name;
                throw Base::TypeError(error);
            }

            // check on the item:
            PyObject* item = PyList_GetItem(itemList, i);
            if (PyUnicode_Check(item)) {
                PyObject* unicode = PyUnicode_AsUTF8String(item);
                values[keyStr] = PyString_AsString(unicode);
                Py_DECREF(unicode);
            }
            else if (PyString_Check(item)) {
                values[keyStr] = PyString_AsString(item);
            }
            else {
                std::string error = std::string("type in list must be string or unicode, not ");
                error += item->ob_type->tp_name;
                throw Base::TypeError(error);
            }
        }
        
        setValues(values);
    }
    else {
        std::string error = std::string("type must be a dict object");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

unsigned int PropertyMap::getMemSize (void) const
{
    size_t size=0;
    for (std::map<std::string,std::string>::const_iterator it = _lValueList.begin();it!= _lValueList.end(); ++it) {
        size += it->second.size();
        size += it->first.size();
    }
    return size;
}

void PropertyMap::Save (Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<Map count=\"" <<  getSize() <<"\">" << endl;
    writer.incInd();
    for (std::map<std::string,std::string>::const_iterator it = _lValueList.begin();it!= _lValueList.end(); ++it) 
        writer.Stream() << writer.ind() << "<Item key=\"" <<  it->first <<"\" value=\"" <<  encodeAttribute(it->second) <<"\"/>" << endl;

    writer.decInd();
    writer.Stream() << writer.ind() << "</Map>" << endl ;
}

void PropertyMap::Restore(Base::XMLReader &reader)
{
    // read my Element
    reader.readElement("Map");
    // get the value of my Attribute
    int count = reader.getAttributeAsInteger("count");

    std::map<std::string,std::string> values;
    for(int i = 0; i < count; i++) {
        reader.readElement("Item");
        values[reader.getAttribute("key")] = reader.getAttribute("value");
    }
    
    reader.readEndElement("Map");

    // assignment
    setValues(values);
}

Property *PropertyMap::Copy(void) const
{
    PropertyMap *p= new PropertyMap();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyMap::Paste(const Property &from)
{
    aboutToSetValue();
    _lValueList = dynamic_cast<const PropertyMap&>(from)._lValueList;
    hasSetValue();
}




//**************************************************************************
//**************************************************************************
// PropertyBool
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyBool , App::Property);

//**************************************************************************
// Construction/Destruction

PropertyBool::PropertyBool()
{
    _lValue = false;
}

PropertyBool::~PropertyBool()
{

}

//**************************************************************************
// Setter/getter for the property

void PropertyBool::setValue(bool lValue)
{
    aboutToSetValue();
    _lValue=lValue;
    hasSetValue();
}

bool PropertyBool::getValue(void) const
{
    return _lValue;
}

PyObject *PropertyBool::getPyObject(void)
{
    if (_lValue)
        {Py_INCREF(Py_True); return Py_True;}
    else
        {Py_INCREF(Py_False); return Py_False;}

}

void PropertyBool::setPyObject(PyObject *value)
{
    if (PyBool_Check(value))
        setValue(PyObject_IsTrue(value)!=0);
    else if(PyInt_Check(value))
        setValue(PyInt_AsLong(value)!=0);
    else {
        std::string error = std::string("type must be bool, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyBool::Save (Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<Bool value=\"" ;
    if (_lValue)
        writer.Stream() << "true" <<"\"/>" ;
    else
        writer.Stream() << "false" <<"\"/>" ;
    writer.Stream() << std::endl;
}

void PropertyBool::Restore(Base::XMLReader &reader)
{
    // read my Element
    reader.readElement("Bool");
    // get the value of my Attribute
    string b = reader.getAttribute("value");
    (b == "true") ? setValue(true) : setValue(false);
}


Property *PropertyBool::Copy(void) const
{
    PropertyBool *p= new PropertyBool();
    p->_lValue = _lValue;
    return p;
}

void PropertyBool::Paste(const Property &from)
{
    aboutToSetValue();
    _lValue = dynamic_cast<const PropertyBool&>(from)._lValue;
    hasSetValue();
}

//**************************************************************************
//**************************************************************************
// PropertyColor
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyColor , App::Property);

//**************************************************************************
// Construction/Destruction

PropertyColor::PropertyColor()
{

}

PropertyColor::~PropertyColor()
{

}

//**************************************************************************
// Base class implementer

void PropertyColor::setValue(const Color &col)
{
    aboutToSetValue();
    _cCol=col;
    hasSetValue();
}

void PropertyColor::setValue(uint32_t rgba)
{
    aboutToSetValue();
    _cCol.setPackedValue(rgba);
    hasSetValue();
}

void PropertyColor::setValue(float r, float g, float b, float a)
{
    aboutToSetValue();
    _cCol.set(r,g,b,a);
    hasSetValue();
}

const Color& PropertyColor::getValue(void) const 
{
    return _cCol;
}

PyObject *PropertyColor::getPyObject(void)
{
    PyObject* rgba = PyTuple_New(4);
    PyObject* r = PyFloat_FromDouble(_cCol.r);
    PyObject* g = PyFloat_FromDouble(_cCol.g);
    PyObject* b = PyFloat_FromDouble(_cCol.b);
    PyObject* a = PyFloat_FromDouble(_cCol.a);

    PyTuple_SetItem(rgba, 0, r);
    PyTuple_SetItem(rgba, 1, g);
    PyTuple_SetItem(rgba, 2, b);
    PyTuple_SetItem(rgba, 3, a);

    return rgba;
}

void PropertyColor::setPyObject(PyObject *value)
{
    App::Color cCol;
    if (PyTuple_Check(value) && PyTuple_Size(value) == 3) {
        PyObject* item;
        item = PyTuple_GetItem(value,0);
        if (PyFloat_Check(item))
            cCol.r = (float)PyFloat_AsDouble(item);
        else
            throw Base::TypeError("Type in tuple must be float");
        item = PyTuple_GetItem(value,1);
        if (PyFloat_Check(item))
            cCol.g = (float)PyFloat_AsDouble(item);
        else
            throw Base::TypeError("Type in tuple must be float");
        item = PyTuple_GetItem(value,2);
        if (PyFloat_Check(item))
            cCol.b = (float)PyFloat_AsDouble(item);
        else
            throw Base::TypeError("Type in tuple must be float");
    }
    else if (PyTuple_Check(value) && PyTuple_Size(value) == 4) {
        PyObject* item;
        item = PyTuple_GetItem(value,0);
        if (PyFloat_Check(item))
            cCol.r = (float)PyFloat_AsDouble(item);
        else
            throw Base::TypeError("Type in tuple must be float");
        item = PyTuple_GetItem(value,1);
        if (PyFloat_Check(item))
            cCol.g = (float)PyFloat_AsDouble(item);
        else
            throw Base::TypeError("Type in tuple must be float");
        item = PyTuple_GetItem(value,2);
        if (PyFloat_Check(item))
            cCol.b = (float)PyFloat_AsDouble(item);
        else
            throw Base::TypeError("Type in tuple must be float");
        item = PyTuple_GetItem(value,3);
        if (PyFloat_Check(item))
            cCol.a = (float)PyFloat_AsDouble(item);
        else
            throw Base::TypeError("Type in tuple must be float");
    }
    else if (PyLong_Check(value)) {
        cCol.setPackedValue(PyLong_AsUnsignedLong(value));
    }
    else {
        std::string error = std::string("type must be int or tuple of float, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }

    setValue( cCol );
}

void PropertyColor::Save (Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<PropertyColor value=\"" 
    <<  _cCol.getPackedValue() <<"\"/>" << endl;
}

void PropertyColor::Restore(Base::XMLReader &reader)
{
    // read my Element
    reader.readElement("PropertyColor");
    // get the value of my Attribute
    unsigned long rgba = reader.getAttributeAsUnsigned("value");
    setValue(rgba);
}

Property *PropertyColor::Copy(void) const
{
    PropertyColor *p= new PropertyColor();
    p->_cCol = _cCol;
    return p;
}

void PropertyColor::Paste(const Property &from)
{
    aboutToSetValue();
    _cCol = dynamic_cast<const PropertyColor&>(from)._cCol;
    hasSetValue();
}

//**************************************************************************
// PropertyColorList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyColorList , App::PropertyLists);

//**************************************************************************
// Construction/Destruction

PropertyColorList::PropertyColorList()
{

}

PropertyColorList::~PropertyColorList()
{

}

//**************************************************************************
// Base class implementer

void PropertyColorList::setSize(int newSize)
{
    _lValueList.resize(newSize);
}

int PropertyColorList::getSize(void) const
{
    return static_cast<int>(_lValueList.size());
}

void PropertyColorList::setValue(const Color& lValue)
{
    aboutToSetValue();
    _lValueList.resize(1);
    _lValueList[0]=lValue;
    hasSetValue();
}

void PropertyColorList::setValues (const std::vector<Color>& values)
{
    aboutToSetValue();
    _lValueList=values;
    hasSetValue();
}

PyObject *PropertyColorList::getPyObject(void)
{
    PyObject* list = PyList_New(getSize());

    for(int i = 0;i<getSize(); i++) {
        PyObject* rgba = PyTuple_New(4);
        PyObject* r = PyFloat_FromDouble(_lValueList[i].r);
        PyObject* g = PyFloat_FromDouble(_lValueList[i].g);
        PyObject* b = PyFloat_FromDouble(_lValueList[i].b);
        PyObject* a = PyFloat_FromDouble(_lValueList[i].a);

        PyTuple_SetItem(rgba, 0, r);
        PyTuple_SetItem(rgba, 1, g);
        PyTuple_SetItem(rgba, 2, b);
        PyTuple_SetItem(rgba, 3, a);

        PyList_SetItem( list, i, rgba );
    }

    return list;
}

void PropertyColorList::setPyObject(PyObject *value)
{
    if (PyList_Check(value)) {
        Py_ssize_t nSize = PyList_Size(value);
        std::vector<Color> values;
        values.resize(nSize);

        for (Py_ssize_t i=0; i<nSize;++i) {
            PyObject* item = PyList_GetItem(value, i);
            PropertyColor col;
            col.setPyObject(item);
            values[i] = col.getValue();
        }

        setValues(values);
    }
    else if (PyTuple_Check(value) && PyTuple_Size(value) == 3) {
        PropertyColor col;
        col.setPyObject( value );
        setValue( col.getValue() );
    }
    else if (PyTuple_Check(value) && PyTuple_Size(value) == 4) {
        PropertyColor col;
        col.setPyObject( value );
        setValue( col.getValue() );
    }
    else {
        std::string error = std::string("not allowed type, ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyColorList::Save (Base::Writer &writer) const
{
    if (!writer.isForceXML()) {
        writer.Stream() << writer.ind() << "<ColorList file=\"" << writer.addFile(getName(), this) << "\"/>" << std::endl;
    }
}

void PropertyColorList::Restore(Base::XMLReader &reader)
{
    reader.readElement("ColorList");
    if (reader.hasAttribute("file")) {
        std::string file (reader.getAttribute("file"));

        if (!file.empty()) {
            // initate a file read
            reader.addFile(file.c_str(),this);
        }
    }
}

void PropertyColorList::SaveDocFile (Base::Writer &writer) const
{
    Base::OutputStream str(writer.Stream());
    uint32_t uCt = (uint32_t)getSize();
    str << uCt;
    for (std::vector<App::Color>::const_iterator it = _lValueList.begin(); it != _lValueList.end(); ++it) {
        str << it->getPackedValue();
    }
}

void PropertyColorList::RestoreDocFile(Base::Reader &reader)
{
    Base::InputStream str(reader);
    uint32_t uCt=0;
    str >> uCt;
    std::vector<Color> values(uCt);
    uint32_t value; // must be 32 bit long
    for (std::vector<App::Color>::iterator it = values.begin(); it != values.end(); ++it) {
        str >> value;
        it->setPackedValue(value);
    }
    setValues(values);
}

Property *PropertyColorList::Copy(void) const
{
    PropertyColorList *p= new PropertyColorList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyColorList::Paste(const Property &from)
{
    aboutToSetValue();
    _lValueList = dynamic_cast<const PropertyColorList&>(from)._lValueList;
    hasSetValue();
}

unsigned int PropertyColorList::getMemSize (void) const
{
    return static_cast<unsigned int>(_lValueList.size() * sizeof(Color));
}

//**************************************************************************
//**************************************************************************
// PropertyMaterial
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyMaterial , App::Property);

PropertyMaterial::PropertyMaterial()
{

}

PropertyMaterial::~PropertyMaterial()
{

}

void PropertyMaterial::setValue(const Material &mat)
{
    aboutToSetValue();
    _cMat=mat;
    hasSetValue();
}

const Material& PropertyMaterial::getValue(void) const 
{
    return _cMat;
}

void PropertyMaterial::setAmbientColor(const Color& col)
{
    aboutToSetValue();
    _cMat.ambientColor = col;
    hasSetValue();
}

void PropertyMaterial::setDiffuseColor(const Color& col)
{
    aboutToSetValue();
    _cMat.diffuseColor = col;
    hasSetValue();
}

void PropertyMaterial::setSpecularColor(const Color& col)
{
    aboutToSetValue();
    _cMat.specularColor = col;
    hasSetValue();
}

void PropertyMaterial::setEmissiveColor(const Color& col)
{
    aboutToSetValue();
    _cMat.emissiveColor = col;
    hasSetValue();
}

void PropertyMaterial::setShininess(float val)
{
    aboutToSetValue();
    _cMat.shininess = val;
    hasSetValue();
}

void PropertyMaterial::setTransparency(float val)
{
    aboutToSetValue();
    _cMat.transparency = val;
    hasSetValue();
}

PyObject *PropertyMaterial::getPyObject(void)
{
    return new MaterialPy(new Material(_cMat));
}

void PropertyMaterial::setPyObject(PyObject *value)
{
    if (PyObject_TypeCheck(value, &(MaterialPy::Type))) {
        setValue(*static_cast<MaterialPy*>(value)->getMaterialPtr());
    }
    else {
        std::string error = std::string("type must be 'Material', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyMaterial::Save (Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<PropertyMaterial ambientColor=\"" 
        <<  _cMat.ambientColor.getPackedValue() 
        << "\" diffuseColor=\"" <<  _cMat.diffuseColor.getPackedValue() 
        << "\" specularColor=\"" <<  _cMat.specularColor.getPackedValue()
        << "\" emissiveColor=\"" <<  _cMat.emissiveColor.getPackedValue()
        << "\" shininess=\"" <<  _cMat.shininess << "\" transparency=\"" 
        <<  _cMat.transparency << "\"/>" << endl;
}

void PropertyMaterial::Restore(Base::XMLReader &reader)
{
    // read my Element
    reader.readElement("PropertyMaterial");
    // get the value of my Attribute
    aboutToSetValue();
    _cMat.ambientColor.setPackedValue(reader.getAttributeAsUnsigned("ambientColor"));
    _cMat.diffuseColor.setPackedValue(reader.getAttributeAsUnsigned("diffuseColor"));
    _cMat.specularColor.setPackedValue(reader.getAttributeAsUnsigned("specularColor"));
    _cMat.emissiveColor.setPackedValue(reader.getAttributeAsUnsigned("emissiveColor"));
    _cMat.shininess = (float)reader.getAttributeAsFloat("shininess");
    _cMat.transparency = (float)reader.getAttributeAsFloat("transparency");
    hasSetValue();
}

Property *PropertyMaterial::Copy(void) const
{
    PropertyMaterial *p= new PropertyMaterial();
    p->_cMat = _cMat;
    return p;
}

void PropertyMaterial::Paste(const Property &from)
{
    aboutToSetValue();
    _cMat = dynamic_cast<const PropertyMaterial&>(from)._cMat;
    hasSetValue();
}


