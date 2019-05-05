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
# include <sstream>
# include <boost/version.hpp>
# include <boost/filesystem/path.hpp>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <boost/math/special_functions/round.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Base/Writer.h>
#include <Base/Stream.h>
#include <Base/Quantity.h>
#include <Base/Tools.h>

#include "PropertyStandard.h"
#include "PropertyLinks.h"
#include "MaterialPy.h"
#include "ObjectIdentifier.h"
#include "Application.h"
#include "Document.h"
#include "DocumentObject.h"

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
#if PY_MAJOR_VERSION < 3
    if (PyInt_Check(value)) {
        aboutToSetValue();
        _lValue = PyInt_AsLong(value);
#else    
    if (PyLong_Check(value)) {
        aboutToSetValue();
        _lValue = PyLong_AsLong(value);
#endif
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

void PropertyInteger::setPathValue(const ObjectIdentifier &path, const App::any &value)
{
    verifyPath(path);

    if (value.type() == typeid(long))
        setValue(App::any_cast<long>(value));
    else if (value.type() == typeid(double))
        setValue(boost::math::round(App::any_cast<double>(value)));
    else if (value.type() == typeid(Quantity) && App::any_cast<const Quantity &>(value).getUnit().isEmpty())
        setValue(boost::math::round(App::any_cast<const Quantity &>(value).getValue()));
    else if (value.type() == typeid(int))
        setValue(App::any_cast<int>(value));
    else
        throw bad_cast();
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
    if (!p) throw Base::UnicodeError("UTF8 conversion failure at PropertyPath::getPyObject()");
    return p;
}

void PropertyPath::setPyObject(PyObject *value)
{
    std::string path;
    if (PyUnicode_Check(value)) {
#if PY_MAJOR_VERSION >= 3
        path = PyUnicode_AsUTF8(value);
#else
        PyObject* unicode = PyUnicode_AsUTF8String(value);
        path = PyString_AsString(unicode);
        Py_DECREF(unicode);
    }
    else if (PyString_Check(value)) {
        path = PyString_AsString(value);
#endif
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
{
    _editorTypeName = "Gui::PropertyEditor::PropertyEnumItem";
}

PropertyEnumeration::PropertyEnumeration(const App::Enumeration &e)
{
    _enum = e;
}

PropertyEnumeration::~PropertyEnumeration()
{

}

void PropertyEnumeration::setEnums(const char **plEnums)
{
    // Setting the enum is done only once inside the constructor
    // but before the current index is already set. So, this needs
    // to be preserved.
    int index = _enum._index;
    _enum.setEnums(plEnums);
    _enum._index = index;
}

void PropertyEnumeration::setValue(const char *value)
{
    aboutToSetValue();
    _enum.setValue(value);
    hasSetValue();
}

void PropertyEnumeration::setValue(long value)
{
    aboutToSetValue();
    _enum.setValue(value);
    hasSetValue();
}

void PropertyEnumeration::setValue(const Enumeration &source)
{
    aboutToSetValue();
    _enum = source;
    hasSetValue();
}

long PropertyEnumeration::getValue(void) const
{
    return _enum.getInt();
}

bool PropertyEnumeration::isValue(const char *value) const
{
    return _enum.isValue(value);
}

bool PropertyEnumeration::isPartOf(const char *value) const
{
    return _enum.contains(value);
}

const char * PropertyEnumeration::getValueAsString(void) const
{
    if (!_enum.isValid())
        throw Base::RuntimeError("Cannot get value from invalid enumeration");
    return _enum.getCStr();
}

Enumeration PropertyEnumeration::getEnum(void) const
{
    return _enum;
}

std::vector<std::string> PropertyEnumeration::getEnumVector(void) const
{
    return _enum.getEnumVector();
}

const char ** PropertyEnumeration::getEnums(void) const
{
    return _enum.getEnums();
}

bool PropertyEnumeration::isValid(void) const
{
    return _enum.isValid();
}

void PropertyEnumeration::Save(Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<Integer value=\"" <<  _enum.getInt() <<"\"";
    if (_enum.isCustom())
        writer.Stream() << " CustomEnum=\"true\"";
    writer.Stream() << "/>" << std::endl;
    if (_enum.isCustom()) {
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

        _enum.setEnums(values);
    }

    if (val < 0) {
        Base::Console().Warning("Enumeration index %d is out of range, ignore it\n", val);
        val = getValue();
    }

    setValue(val);
}

PyObject * PropertyEnumeration::getPyObject(void)
{
    if (!_enum.isValid()) {
        PyErr_SetString(PyExc_AssertionError, "The enum is empty");
        return 0;
    }

    return Py_BuildValue("s", getValueAsString());
}

void PropertyEnumeration::setPyObject(PyObject *value)
{
#if PY_MAJOR_VERSION < 3
    if (PyInt_Check(value)) {
        long val = PyInt_AsLong(value);
#else
    if (PyLong_Check(value)) {
        long val = PyLong_AsLong(value);
#endif
        if (_enum.isValid()) {
            aboutToSetValue();
            _enum.setValue(val, true);
            hasSetValue();
        }
    }
#if PY_MAJOR_VERSION < 3
    else if (PyString_Check(value)) {
        const char* str = PyString_AsString (value);
        if (_enum.contains(str)) {
            aboutToSetValue();
            _enum.setValue(PyString_AsString (value));
            hasSetValue();
        }
        else {
            std::stringstream out;
            out << "'" << str << "' is not part of the enumeration";
            throw Base::ValueError(out.str());
        }
    }
#endif
    else if (PyUnicode_Check(value)) {
#if PY_MAJOR_VERSION >=3
        std::string str = PyUnicode_AsUTF8(value);
#else
        PyObject* unicode = PyUnicode_AsUTF8String(value);
        std::string str = PyString_AsString(unicode);
        Py_DECREF(unicode);
#endif
        if (_enum.contains(str.c_str())) {
            aboutToSetValue();
            _enum.setValue(str);
            hasSetValue();
        }
        else {
            std::stringstream out;
            out << "'" << str << "' is not part of the enumeration";
            throw Base::ValueError(out.str());
        }
    }
    else if (PySequence_Check(value)) {
        Py_ssize_t nSize = PySequence_Size(value);
        std::vector<std::string> values;
        values.resize(nSize);

        for (Py_ssize_t i = 0; i < nSize; ++i) {
            PyObject *item = PySequence_GetItem(value, i);

            if (PyUnicode_Check(item)) {
#if PY_MAJOR_VERSION >= 3
                values[i] = PyUnicode_AsUTF8(item);
#else
                PyObject* unicode = PyUnicode_AsUTF8String(item);
                values[i] = PyString_AsString(unicode);
                Py_DECREF(unicode);
#endif
            }
#if PY_MAJOR_VERSION < 3
            else if (PyString_Check(item)) {
                values[i] = PyString_AsString(item);
            }
#endif
            else {
                std::string error = std::string("type in list must be str or unicode, not ");
                throw Base::TypeError(error + item->ob_type->tp_name);
            }
        }
        _enum.setEnums(values);
        setValue((long)0);
    }
    else {
        std::string error = std::string("type must be int, str or unicode not ");
        throw Base::TypeError(error + value->ob_type->tp_name);
    }
}

Property * PropertyEnumeration::Copy(void) const
{
    return new PropertyEnumeration(_enum);
}

void PropertyEnumeration::Paste(const Property &from)
{
    aboutToSetValue();

    const PropertyEnumeration& prop = dynamic_cast<const PropertyEnumeration&>(from);
    _enum = prop._enum;

    hasSetValue();
}

void PropertyEnumeration::setPathValue(const ObjectIdentifier &path, const App::any &value)
{
    verifyPath(path);

    if (value.type() == typeid(int))
        setValue(App::any_cast<int>(value));
    else if (value.type() == typeid(double))
        setValue(App::any_cast<double>(value));
    else if (value.type() == typeid(short))
        setValue(App::any_cast<short>(value));
    else if (value.type() == typeid(std::string))
        setValue(App::any_cast<const std::string &>(value).c_str());
    else if (value.type() == typeid(char*))
        setValue(App::any_cast<char*>(value));
    else if (value.type() == typeid(const char*))
        setValue(App::any_cast<const char*>(value));
    else
        throw bad_cast();
}

//**************************************************************************
//**************************************************************************
// PropertyIntegerConstraint
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyIntegerConstraint, App::PropertyInteger)

//**************************************************************************
// Construction/Destruction


PropertyIntegerConstraint::PropertyIntegerConstraint()
  : _ConstStruct(0)
{

}


PropertyIntegerConstraint::~PropertyIntegerConstraint()
{
    if (_ConstStruct && _ConstStruct->isDeletable())
        delete _ConstStruct;
}

void PropertyIntegerConstraint::setConstraints(const Constraints* sConstrain)
{
    if (_ConstStruct != sConstrain) {
        if (_ConstStruct && _ConstStruct->isDeletable())
            delete _ConstStruct;
    }

    _ConstStruct = sConstrain;
}

const PropertyIntegerConstraint::Constraints*  PropertyIntegerConstraint::getConstraints(void) const
{
    return _ConstStruct;
}

void PropertyIntegerConstraint::setPyObject(PyObject *value)
{
#if PY_MAJOR_VERSION < 3
    if (PyInt_Check(value)) {
        long temp = PyInt_AsLong(value);
#else
    if (PyLong_Check(value)) {
        long temp = PyLong_AsLong(value);
#endif
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
    else if (PyTuple_Check(value) && PyTuple_Size(value) == 4) {
        long values[4];
        for (int i=0; i<4; i++) {
            PyObject* item;
            item = PyTuple_GetItem(value,i);
#if PY_MAJOR_VERSION < 3
            if (PyInt_Check(item))
                values[i] = PyInt_AsLong(item);
#else
            if (PyLong_Check(item))
                values[i] = PyLong_AsLong(item);
#endif
            else
                throw Base::TypeError("Type in tuple must be int");
        }

        Constraints* c = new Constraints();
        c->setDeletable(true);
        c->LowerBound = values[1];
        c->UpperBound = values[2];
        c->StepSize = std::max<long>(1, values[3]);
        if (values[0] > c->UpperBound)
            values[0] = c->UpperBound;
        else if (values[0] < c->LowerBound)
            values[0] = c->LowerBound;
        setConstraints(c);

        aboutToSetValue();
        _lValue = values[0];
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

//**************************************************************************
// Base class implementer

PyObject *PropertyIntegerList::getPyObject(void)
{
    PyObject* list = PyList_New(getSize());
    for(int i = 0;i<getSize(); i++)
#if PY_MAJOR_VERSION < 3
        PyList_SetItem( list, i, PyInt_FromLong(_lValueList[i]));
#else
        PyList_SetItem( list, i, PyLong_FromLong(_lValueList[i]));
#endif
    return list;
}

long PropertyIntegerList::getPyValue(PyObject *item) const {
#if PY_MAJOR_VERSION < 3
    if (PyInt_Check(item))
        return PyInt_AsLong(item);
#else
    if (PyLong_Check(item)) 
        return PyLong_AsLong(item);
#endif
    std::string error = std::string("type in list must be int, not ");
    error += item->ob_type->tp_name;
    throw Base::TypeError(error);
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
    setValues(dynamic_cast<const PropertyIntegerList&>(from)._lValueList);
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
#if PY_MAJOR_VERSION < 3
        PySet_Add(set,PyInt_FromLong(*it));
#else
        PySet_Add(set,PyLong_FromLong(*it));
#endif
    return set;
}

void PropertyIntegerSet::setPyObject(PyObject *value)
{ 
    if (PySequence_Check(value)) {
        
        Py_ssize_t nSize = PySequence_Length(value);
        std::set<long> values;

        for (Py_ssize_t i=0; i<nSize;++i) {
            PyObject* item = PySequence_GetItem(value, i);
#if PY_MAJOR_VERSION < 3
            if (!PyInt_Check(item)) {
                std::string error = std::string("type in list must be int, not ");
                error += item->ob_type->tp_name;
                throw Base::TypeError(error);
            }
            values.insert(PyInt_AsLong(item));
#else
            if (!PyLong_Check(item)) {
                std::string error = std::string("type in list must be int, not ");
                error += item->ob_type->tp_name;
                throw Base::TypeError(error);
            }
            values.insert(PyLong_AsLong(item));
#endif
        }

        setValues(values);
    }
#if PY_MAJOR_VERSION < 3
    else if (PyInt_Check(value)) {
        setValue(PyInt_AsLong(value));
#else
    else if (PyLong_Check(value)) {
        setValue(PyLong_AsLong(value));
#endif
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
#if PY_MAJOR_VERSION < 3
    else if(PyInt_Check(value)) {
        aboutToSetValue();
        _dValue = PyInt_AsLong(value);
#else
    else if(PyLong_Check(value)) {
        aboutToSetValue();
        _dValue = PyLong_AsLong(value);
#endif
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

void PropertyFloat::setPathValue(const ObjectIdentifier &path, const App::any &value)
{
    verifyPath(path);

    if (value.type() == typeid(double))
        setValue(App::any_cast<double>(value));
    else if (value.type() == typeid(Quantity) && App::any_cast<const Quantity&>(value).getUnit().isEmpty())
        setValue((App::any_cast<const Quantity&>(value)).getValue());
    else
        throw bad_cast();
}

App::any PropertyFloat::getPathValue(const ObjectIdentifier &path) const
{
    verifyPath(path);
    return _dValue;
}

//**************************************************************************
//**************************************************************************
// PropertyFloatConstraint
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyFloatConstraint, App::PropertyFloat)

//**************************************************************************
// Construction/Destruction


PropertyFloatConstraint::PropertyFloatConstraint()
  : _ConstStruct(0)
{

}

PropertyFloatConstraint::~PropertyFloatConstraint()
{
    if (_ConstStruct && _ConstStruct->isDeletable())
        delete _ConstStruct;
}

void PropertyFloatConstraint::setConstraints(const Constraints* sConstrain)
{
    if (_ConstStruct != sConstrain) {
        if (_ConstStruct && _ConstStruct->isDeletable())
            delete _ConstStruct;
    }
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
#if PY_MAJOR_VERSION < 3
    else if (PyInt_Check(value)) {
        double temp = (double)PyInt_AsLong(value);
#else
    else if (PyLong_Check(value)) {
        double temp = (double)PyLong_AsLong(value);
#endif
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
    else if (PyTuple_Check(value) && PyTuple_Size(value) == 4) {
        double values[4];
        for (int i=0; i<4; i++) {
            PyObject* item;
            item = PyTuple_GetItem(value,i);
            if (PyFloat_Check(item))
                values[i] = PyFloat_AsDouble(item);
#if PY_MAJOR_VERSION < 3
            else if (PyInt_Check(item))
                values[i] = PyInt_AsLong(item);
#else
            else if (PyLong_Check(item))
                values[i] = PyLong_AsLong(item);
#endif
            else
                throw Base::TypeError("Type in tuple must be float or int");
        }

        double stepSize = values[3];
        // need a value > 0
        if (stepSize < DBL_EPSILON)
            throw Base::ValueError("Step size must be greater than zero");

        Constraints* c = new Constraints();
        c->setDeletable(true);
        c->LowerBound = values[1];
        c->UpperBound = values[2];
        c->StepSize = stepSize;
        if (values[0] > c->UpperBound)
            values[0] = c->UpperBound;
        else if (values[0] < c->LowerBound)
            values[0] = c->LowerBound;
        setConstraints(c);

        aboutToSetValue();
        _dValue = values[0];
        hasSetValue();
    }
    else {
        std::string error = std::string("type must be float, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

//**************************************************************************
// PropertyPrecision
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyPrecision, App::PropertyFloatConstraint);

//**************************************************************************
// Construction/Destruction
//
const PropertyFloatConstraint::Constraints PrecisionStandard = {0.0,DBL_MAX,0.001};

PropertyPrecision::PropertyPrecision()
{
    setConstraints(&PrecisionStandard);
}

PropertyPrecision::~PropertyPrecision()
{

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

PyObject *PropertyFloatList::getPyObject(void)
{
    PyObject* list = PyList_New(getSize());
    for (int i = 0;i<getSize(); i++)
         PyList_SetItem( list, i, PyFloat_FromDouble(_lValueList[i]));
    return list;
}

double PropertyFloatList::getPyValue(PyObject *item) const {
    if (PyFloat_Check(item)) {
        return PyFloat_AsDouble(item);
#if PY_MAJOR_VERSION >= 3
    } else if (PyLong_Check(item)) {
        return static_cast<double>(PyLong_AsLong(item));
#else
    } else if (PyInt_Check(item)) {
        return static_cast<double>(PyInt_AsLong(item));
#endif
    } else {
        std::string error = std::string("type in list must be float, not ");
        error += item->ob_type->tp_name;
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
            (getSize()?writer.addFile(getName(), this):"") << "\"/>" << std::endl;
    }
}

void PropertyFloatList::Restore(Base::XMLReader &reader)
{
    reader.readElement("FloatList");
    string file (reader.getAttribute("file") );

    if (!file.empty()) {
        // initiate a file read
        reader.addFile(file.c_str(),this);
    }
}

void PropertyFloatList::SaveDocFile (Base::Writer &writer) const
{
    Base::OutputStream str(writer.Stream());
    uint32_t uCt = (uint32_t)getSize();
    str << uCt;
    if (!isSinglePrecision()) {
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
    if (!isSinglePrecision()) {
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
    setValues(dynamic_cast<const PropertyFloatList&>(from)._lValueList);
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

void PropertyString::setValue(const char* newLabel)
{
    if(!newLabel) return;

    if(_cValue == newLabel)
        return;

    std::string _newLabel;

    std::vector<std::pair<Property*,std::unique_ptr<Property> > > propChanges;
    std::string label;
    auto obj = dynamic_cast<DocumentObject*>(getContainer());
    bool commit = false;

    if(obj && obj->getNameInDocument() && this==&obj->Label &&
       (!obj->getDocument()->testStatus(App::Document::Restoring)||
        obj->getDocument()->testStatus(App::Document::Importing)) && 
       !obj->getDocument()->isPerformingTransaction()) 
    {
        // allow object to control label change

        static ParameterGrp::handle _hPGrp;
        if(!_hPGrp) {
            _hPGrp = GetApplication().GetUserParameter().GetGroup("BaseApp");
            _hPGrp = _hPGrp->GetGroup("Preferences")->GetGroup("Document");
        }
        App::Document* doc = obj->getDocument();
        if(doc && !_hPGrp->GetBool("DuplicateLabels") && !obj->allowDuplicateLabel()) {
            std::vector<std::string> objectLabels;
            std::vector<App::DocumentObject*>::const_iterator it;
            std::vector<App::DocumentObject*> objs = doc->getObjects();
            bool match = false;
            for (it = objs.begin();it != objs.end();++it) {
                if (*it == obj)
                    continue; // don't compare object with itself
                std::string objLabel = (*it)->Label.getValue();
                if (!match && objLabel == newLabel)
                    match = true;
                objectLabels.push_back(objLabel);
            }

            // make sure that there is a name conflict otherwise we don't have to do anything
            if (match && *newLabel) {
                label = newLabel;
                // remove number from end to avoid lengthy names
                size_t lastpos = label.length()-1;
                while (label[lastpos] >= 48 && label[lastpos] <= 57) {
                    // if 'lastpos' becomes 0 then all characters are digits. In this case we use
                    // the complete label again
                    if (lastpos == 0) {
                        lastpos = label.length()-1;
                        break;
                    }
                    lastpos--;
                }

                bool changed = false;
                label = label.substr(0,lastpos+1);
                if(boost::starts_with(obj->getNameInDocument(),label)) {
                    // In case the label has the same base name as object's
                    // internal name, use it as the label instead.
                    const char *objName = obj->getNameInDocument();
                    const char *c = &objName[lastpos+1];
                    for(;*c;++c) {
                        if(*c<48 || *c>57)
                            break;
                    }
                    if(*c == 0) {
                        label = obj->getNameInDocument();
                        changed = true;
                    }
                }
                if(!changed)
                    label = Base::Tools::getUniqueName(label, objectLabels, 3);
            }
        }

        if(label.empty())
            label = newLabel;
        obj->onBeforeChangeLabel(label);
        newLabel = label.c_str();

        if(!obj->getDocument()->testStatus(App::Document::Restoring)) {
            // Only update label reference if we are not restoring. When
            // importing (which also counts as restoring), it is possible the
            // new object changes its label. However, we cannot update label
            // references here, because object restoring is not based on
            // dependency order. It can only be done in afterRestore(). 
            //
            // See PropertyLinkBase::restoreLabelReference() for more details.
            propChanges = PropertyLinkBase::updateLabelReferences(obj,newLabel);
        }

        if(propChanges.size() && !GetApplication().getActiveTransaction()) {
            commit = true;
            std::ostringstream str;
            str << "Change " << obj->getNameInDocument() << ".Label";
            GetApplication().setActiveTransaction(str.str().c_str());
        }
    }

    aboutToSetValue();
    _cValue = newLabel;
    hasSetValue();

    for(auto &change : propChanges)
        change.first->Paste(*change.second.get());

    if(commit)
        GetApplication().closeActiveTransaction();
}

void PropertyString::setValue(const std::string &sString)
{
    setValue(sString.c_str());
}

const char* PropertyString::getValue(void) const
{
    return _cValue.c_str();
}

PyObject *PropertyString::getPyObject(void)
{
    PyObject *p = PyUnicode_DecodeUTF8(_cValue.c_str(),_cValue.size(),0);
    if (!p) throw Base::UnicodeError("UTF8 conversion failure at PropertyString::getPyObject()");
    return p;
}

void PropertyString::setPyObject(PyObject *value)
{
    std::string string;
    if (PyUnicode_Check(value)) {
#if PY_MAJOR_VERSION >= 3
        string = PyUnicode_AsUTF8(value);
#else
        PyObject* unicode = PyUnicode_AsUTF8String(value);
        string = PyString_AsString(unicode);
        Py_DECREF(unicode);
    }
    else if (PyString_Check(value)) {
        string = PyString_AsString(value);
#endif
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
    std::string val;
    auto obj = dynamic_cast<DocumentObject*>(getContainer());
    writer.Stream() << writer.ind() << "<String ";
    bool exported = false;
    if(obj && obj->getNameInDocument() && 
       obj->isExporting() && &obj->Label==this)
    {
        if(obj->allowDuplicateLabel())
            writer.Stream() <<"restore=\"1\" ";
        else if(_cValue==obj->getNameInDocument()) {
            writer.Stream() <<"restore=\"0\" ";
            val = encodeAttribute(obj->getExportName());
            exported = true;
        }
    }
    if(!exported)
        val = encodeAttribute(_cValue);
    writer.Stream() <<"value=\"" << val <<"\"/>" << std::endl;
}

void PropertyString::Restore(Base::XMLReader &reader)
{
    // read my Element
    reader.readElement("String");
    // get the value of my Attribute
    auto obj = dynamic_cast<DocumentObject*>(getContainer());
    if(obj && &obj->Label==this) {
        if(reader.hasAttribute("restore")) {
            int restore = reader.getAttributeAsInteger("restore");
            if(restore == 1) {
                aboutToSetValue();
                _cValue = reader.getAttribute("value");
                hasSetValue();
            }else
                setValue(reader.getName(reader.getAttribute("value")));
        } else
            setValue(reader.getAttribute("value"));
    }else
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
    setValue(dynamic_cast<const PropertyString&>(from)._cValue);
}

unsigned int PropertyString::getMemSize (void) const
{
    return static_cast<unsigned int>(_cValue.size());
}

void PropertyString::setPathValue(const ObjectIdentifier &path, const App::any &value)
{
    verifyPath(path);
    if (value.type() == typeid(bool))
        setValue(App::any_cast<bool>(value)?"True":"False");
    else if (value.type() == typeid(int))
        setValue(std::to_string(App::any_cast<int>(value)));
    else if (value.type() == typeid(double))
        setValue(std::to_string(boost::math::round(App::any_cast<double>(value))));
    else if (value.type() == typeid(Quantity))
        setValue(App::any_cast<Quantity>(value).getUserString().toUtf8().constData());
    else if (value.type() == typeid(std::string))
        setValue(App::any_cast<const std::string &>(value));
    else {
        Base::PyGILStateLocker lock;
        setValue(pyObjectFromAny(value).as_string());
    }
}

App::any PropertyString::getPathValue(const ObjectIdentifier &path) const
{
    verifyPath(path);
    return _cValue;
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
#if PY_MAJOR_VERSION >= 3
    PyObject *p = PyUnicode_FromString(_uuid.getValue().c_str());
#else
    PyObject *p = PyString_FromString(_uuid.getValue().c_str());
#endif
    return p;
}

void PropertyUUID::setPyObject(PyObject *value)
{
    std::string string;
    if (PyUnicode_Check(value)) {
#if PY_MAJOR_VERSION >= 3
        string = PyUnicode_AsUTF8(value);
#else
        PyObject* unicode = PyUnicode_AsUTF8String(value);
        string = PyString_AsString(unicode);
        Py_DECREF(unicode);
    }
    else if (PyString_Check(value)) {
        string = PyString_AsString(value);
#endif
    }
    else {
        std::string error = std::string("type must be unicode or str, not ");
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

void PropertyStringList::setValues(const std::list<std::string>& lValue)
{
    std::vector<std::string> vals;
    vals.reserve(lValue.size());
    for(const auto &v : lValue)
        vals.push_back(v);
    setValues(vals);
}

PyObject *PropertyStringList::getPyObject(void)
{
    PyObject* list = PyList_New(getSize());

    for (int i = 0;i<getSize(); i++) {
        PyObject* item = PyUnicode_DecodeUTF8(_lValueList[i].c_str(), _lValueList[i].size(), 0);
        if (!item) {
            Py_DECREF(list);
            throw Base::UnicodeError("UTF8 conversion failure at PropertyStringList::getPyObject()");
        }
        PyList_SetItem(list, i, item);
    }

    return list;
}

std::string PropertyStringList::getPyValue(PyObject *item) const {
    std::string ret;
    if (PyUnicode_Check(item)) {
#if PY_MAJOR_VERSION >= 3
        ret = PyUnicode_AsUTF8(item);
#else
        PyObject* unicode = PyUnicode_AsUTF8String(item);
        ret = PyString_AsString(unicode);
        Py_DECREF(unicode);
    } else if (PyString_Check(item)) {
        ret = PyString_AsString(item);
#endif
#if PY_MAJOR_VERSION >= 3
    } else if (PyBytes_Check(item)) {
        ret = PyBytes_AsString(item);
#endif
    } else {
        std::string error = std::string("type in list must be str or unicode, not ");
        error += item->ob_type->tp_name;
        throw Base::TypeError(error);
    }
    return ret;
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
    setValues(dynamic_cast<const PropertyStringList&>(from)._lValueList);
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
            throw Base::UnicodeError("UTF8 conversion failure at PropertyMap::getPyObject()");
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
            if (PyUnicode_Check(key)) {
#if PY_MAJOR_VERSION >= 3
                keyStr = PyUnicode_AsUTF8(key);
#else
                PyObject* unicode = PyUnicode_AsUTF8String(key);
                keyStr = PyString_AsString(unicode);
                Py_DECREF(unicode);
            }else if (PyString_Check(key)) {
                keyStr = PyString_AsString(key);
#endif
            }
            else {
                std::string error = std::string("type of the key need to be unicode or string, not");
                error += key->ob_type->tp_name;
                throw Base::TypeError(error);
            }

            // check on the item:
            PyObject* item = PyList_GetItem(itemList, i);
            if (PyUnicode_Check(item)) {
#if PY_MAJOR_VERSION >= 3
                values[keyStr] = PyUnicode_AsUTF8(item);
#else
                PyObject* unicode = PyUnicode_AsUTF8String(item);
                values[keyStr] = PyString_AsString(unicode);
                Py_DECREF(unicode);
            }
            else if (PyString_Check(item)) {
                values[keyStr] = PyString_AsString(item);
#endif
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
    return PyBool_FromLong(_lValue ? 1 : 0);
}

void PropertyBool::setPyObject(PyObject *value)
{
    if (PyBool_Check(value))
        setValue(PyObject_IsTrue(value)!=0);
#if PY_MAJOR_VERSION < 3
    else if(PyInt_Check(value))
        setValue(PyInt_AsLong(value)!=0);
#else
    else if(PyLong_Check(value))
        setValue(PyLong_AsLong(value)!=0);
#endif
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

void PropertyBool::setPathValue(const ObjectIdentifier &path, const App::any &value)
{
    verifyPath(path);

    if (value.type() == typeid(bool))
        setValue(App::any_cast<bool>(value));
    else if (value.type() == typeid(int))
        setValue(App::any_cast<int>(value) != 0);
    else if (value.type() == typeid(double))
        setValue(boost::math::round(App::any_cast<double>(value)));
    else if (value.type() == typeid(Quantity) && App::any_cast<const Quantity&>(value).getUnit().isEmpty())
        setValue(App::any_cast<const Quantity&>(value).getValue() != 0);
    else
        throw bad_cast();
}

App::any PropertyBool::getPathValue(const ObjectIdentifier &path) const
{
    verifyPath(path);

    return _lValue;
}

//**************************************************************************
//**************************************************************************
// PropertyBoolList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyBoolList , App::PropertyLists);

//**************************************************************************
// Construction/Destruction


PropertyBoolList::PropertyBoolList()
{

}

PropertyBoolList::~PropertyBoolList()
{

}

//**************************************************************************
// Base class implementer

PyObject *PropertyBoolList::getPyObject(void)
{
    PyObject* tuple = PyTuple_New(getSize());
    for(int i = 0;i<getSize(); i++) {
        bool v = _lValueList[i];
        if (v) {
            PyTuple_SetItem(tuple, i, PyBool_FromLong(1));
        }
        else {
            PyTuple_SetItem(tuple, i, PyBool_FromLong(0));
        }
    }
    return tuple;
}

void PropertyBoolList::setPyObject(PyObject *value)
{
    // string is also a sequence and must be treated differently
    std::string str;
    if (PyUnicode_Check(value)) {
#if PY_MAJOR_VERSION >= 3
        str = PyUnicode_AsUTF8(value);
#else
        PyObject* unicode = PyUnicode_AsUTF8String(value);
        str = PyString_AsString(unicode);
        Py_DECREF(unicode);
        boost::dynamic_bitset<> values(str);
        setValues(values);
    } else if (PyString_Check(value)) {
        str = PyString_AsString(value);
#endif
        boost::dynamic_bitset<> values(str);
        setValues(values);
    }else
        inherited::setPyObject(value);
}

bool PropertyBoolList::getPyValue(PyObject *item) const {
    if (PyBool_Check(item)) {
        return (PyObject_IsTrue(item) ? true : false);
#if PY_MAJOR_VERSION < 3
    } else if (PyInt_Check(item)) {
        return (PyInt_AsLong(item) ? true : false);
#else
    } else if (PyLong_Check(item)) {
        return (PyLong_AsLong(item) ? true : false);
#endif
    } else {
        std::string error = std::string("type in list must be bool or int, not ");
        error += item->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyBoolList::Save (Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<BoolList value=\"" ;
    std::string bitset;
    boost::to_string(_lValueList, bitset);
    writer.Stream() << bitset <<"\"/>" ;
    writer.Stream() << std::endl;
}

void PropertyBoolList::Restore(Base::XMLReader &reader)
{
    // read my Element
    reader.readElement("BoolList");
    // get the value of my Attribute
    string str = reader.getAttribute("value");
    boost::dynamic_bitset<> bitset(str);
    setValues(bitset);
}

Property *PropertyBoolList::Copy(void) const
{
    PropertyBoolList *p= new PropertyBoolList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyBoolList::Paste(const Property &from)
{
    setValues(dynamic_cast<const PropertyBoolList&>(from)._lValueList);
}

unsigned int PropertyBoolList::getMemSize (void) const
{
    return static_cast<unsigned int>(_lValueList.size());
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

Color PropertyColorList::getPyValue(PyObject *item) const {
    PropertyColor col;
    col.setPyObject(item);
    return col.getValue();
}

void PropertyColorList::Save (Base::Writer &writer) const
{
    if (!writer.isForceXML()) {
        writer.Stream() << writer.ind() << "<ColorList file=\"" << 
            (getSize()?writer.addFile(getName(), this):"") << "\"/>" << std::endl;
    }
}

void PropertyColorList::Restore(Base::XMLReader &reader)
{
    reader.readElement("ColorList");
    if (reader.hasAttribute("file")) {
        std::string file (reader.getAttribute("file"));

        if (!file.empty()) {
            // initiate a file read
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
    setValues(dynamic_cast<const PropertyColorList&>(from)._lValueList);
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

const char* PropertyMaterial::getEditorName(void) const
{
    if(testStatus(MaterialEdit))
        return "Gui::PropertyEditor::PropertyMaterialItem";
    return "";
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

//**************************************************************************
// PropertyMaterialList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyMaterialList, App::PropertyLists);

//**************************************************************************
// Construction/Destruction

PropertyMaterialList::PropertyMaterialList()
{

}

PropertyMaterialList::~PropertyMaterialList()
{

}

//**************************************************************************
// Base class implementer

PyObject *PropertyMaterialList::getPyObject(void)
{
    Py::Tuple tuple(getSize());

    for (int i = 0; i<getSize(); i++) {
        tuple.setItem(i, Py::asObject(new MaterialPy(new Material(_lValueList[i]))));
    }

    return Py::new_reference_to(tuple);
}

Material PropertyMaterialList::getPyValue(PyObject *value) const {
    if (PyObject_TypeCheck(value, &(MaterialPy::Type)))
        return *static_cast<MaterialPy*>(value)->getMaterialPtr();
    else {
        std::string error = std::string("type must be 'Material', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyMaterialList::Save(Base::Writer &writer) const
{
    if (!writer.isForceXML()) {
        writer.Stream() << writer.ind() << "<MaterialList file=\"" << 
            (getSize()?writer.addFile(getName(), this):"") << "\"/>" << std::endl;
    }
}

void PropertyMaterialList::Restore(Base::XMLReader &reader)
{
    reader.readElement("MaterialList");
    if (reader.hasAttribute("file")) {
        std::string file(reader.getAttribute("file"));

        if (!file.empty()) {
            // initiate a file read
            reader.addFile(file.c_str(), this);
        }
    }
}

void PropertyMaterialList::SaveDocFile(Base::Writer &writer) const
{
    Base::OutputStream str(writer.Stream());
    uint32_t uCt = (uint32_t)getSize();
    str << uCt;
    for (std::vector<App::Material>::const_iterator it = _lValueList.begin(); it != _lValueList.end(); ++it) {
        str << it->ambientColor.getPackedValue();
        str << it->diffuseColor.getPackedValue();
        str << it->specularColor.getPackedValue();
        str << it->emissiveColor.getPackedValue();
        str << it->shininess;
        str << it->transparency;
    }
}

void PropertyMaterialList::RestoreDocFile(Base::Reader &reader)
{
    Base::InputStream str(reader);
    uint32_t uCt = 0;
    str >> uCt;
    std::vector<Material> values(uCt);
    uint32_t value; // must be 32 bit long
    float valueF;
    for (std::vector<App::Material>::iterator it = values.begin(); it != values.end(); ++it) {
        str >> value;
        it->ambientColor.setPackedValue(value);
        str >> value;
        it->diffuseColor.setPackedValue(value);
        str >> value;
        it->specularColor.setPackedValue(value);
        str >> value;
        it->emissiveColor.setPackedValue(value);
        str >> valueF;
        it->shininess = valueF;
        str >> valueF;
        it->transparency = valueF;
    }
    setValues(values);
}

const char* PropertyMaterialList::getEditorName(void) const
{
    if(testStatus(NoMaterialListEdit))
        return "";
    return "Gui::PropertyEditor::PropertyMaterialListItem";
}

Property *PropertyMaterialList::Copy(void) const
{
    PropertyMaterialList *p = new PropertyMaterialList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyMaterialList::Paste(const Property &from)
{
    setValues(dynamic_cast<const PropertyMaterialList&>(from)._lValueList);
}

unsigned int PropertyMaterialList::getMemSize(void) const
{
    return static_cast<unsigned int>(_lValueList.size() * sizeof(Material));
}

//**************************************************************************
// PropertyPersistentObject
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyPersistentObject , App::PropertyString);

PyObject *PropertyPersistentObject::getPyObject(void){
    if(_pObject)
        return _pObject->getPyObject();
    return inherited::getPyObject();
}

void PropertyPersistentObject::Save(Base::Writer &writer) const{
    inherited::Save(writer);
#define ELEMENT_PERSISTENT_OBJ "PersistentObject"
    writer.Stream() << writer.ind() << "<" ELEMENT_PERSISTENT_OBJ ">" << std::endl;
    if(_pObject) {
        writer.incInd();
        _pObject->Save(writer);
        writer.decInd();
    }
    writer.Stream() << writer.ind() << "</" ELEMENT_PERSISTENT_OBJ ">" << std::endl;
}

void PropertyPersistentObject::Restore(Base::XMLReader &reader){
    inherited::Restore(reader);
    reader.readElement(ELEMENT_PERSISTENT_OBJ);
    if(_pObject)
        _pObject->Restore(reader);
    reader.readEndElement(ELEMENT_PERSISTENT_OBJ);
}

Property *PropertyPersistentObject::Copy(void) const{
    auto *p= new PropertyPersistentObject();
    p->_cValue = _cValue;
    p->_pObject = _pObject;
    return p;
}

void PropertyPersistentObject::Paste(const Property &from){
    const auto &prop = dynamic_cast<const PropertyPersistentObject&>(from);
    if(_cValue!=prop._cValue || _pObject!=prop._pObject) {
        aboutToSetValue();
        _cValue = prop._cValue;
        _pObject = prop._pObject;
        hasSetValue();
    }
}

unsigned int PropertyPersistentObject::getMemSize (void) const{
    auto size = inherited::getMemSize();
    if(_pObject)
        size += _pObject->getMemSize();
    return size;
}

void PropertyPersistentObject::setValue(const char *type) {
    if(!type) type = "";
    if(type[0]) {
        Base::Type::importModule(type);
        Base::Type t = Base::Type::fromName(type);
        if(t.isBad())
            throw Base::TypeError("Invalid type");
        if(!t.isDerivedFrom(Persistence::getClassTypeId()))
            throw Base::TypeError("Type must be derived from Base::Persistence");
        if(_pObject && _pObject->getTypeId()==t)
            return;
    }
    aboutToSetValue();
    _pObject.reset();
    _cValue = type;
    if(type[0])
        _pObject.reset(static_cast<Base::Persistence*>(Base::Type::createInstanceByName(type)));
    hasSetValue();
}
