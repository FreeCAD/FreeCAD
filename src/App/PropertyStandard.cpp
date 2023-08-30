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
# include <xercesc/dom/DOM.hpp>
#endif

#include <boost/algorithm/string/predicate.hpp>
#include <boost/math/special_functions/round.hpp>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Base/Reader.h>
#include <Base/DocumentReader.h>
#include <Base/Writer.h>
#include <Base/Quantity.h>
#include <Base/Stream.h>
#include <Base/Tools.h>

#include "PropertyStandard.h"
#include "Application.h"
#include "Document.h"
#include "DocumentObject.h"
#include "MaterialPy.h"
#include "ObjectIdentifier.h"


using namespace App;
using namespace Base;
using namespace std;




//**************************************************************************
//**************************************************************************
// PropertyInteger
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyInteger , App::Property)

//**************************************************************************
// Construction/Destruction


PropertyInteger::PropertyInteger()
{
    _lValue = 0;
}


PropertyInteger::~PropertyInteger() = default;

//**************************************************************************
// Base class implementer


void PropertyInteger::setValue(long lValue)
{
    aboutToSetValue();
    _lValue=lValue;
    hasSetValue();
}

long PropertyInteger::getValue() const
{
    return _lValue;
}

PyObject *PropertyInteger::getPyObject()
{
    return Py_BuildValue("l", _lValue);
}

void PropertyInteger::setPyObject(PyObject *value)
{
    if (PyLong_Check(value)) {
        aboutToSetValue();
        _lValue = PyLong_AsLong(value);
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

void PropertyInteger::Restore(Base::DocumentReader &reader,XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *ContainerDOM)
{
	// read my Element
	auto IntegerDOM = reader.FindElement(ContainerDOM,"Integer");
	if(IntegerDOM){
		// get the value of my Attribute
		const char* value_cstr = reader.GetAttribute(IntegerDOM,"value");
		if(value_cstr){
			long value = reader.ContentToInt( value_cstr );
			setValue(value);
		}
	}
}

Property *PropertyInteger::Copy() const
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

void PropertyInteger::setPathValue(const ObjectIdentifier &path, const boost::any &value)
{
    verifyPath(path);

    if (value.type() == typeid(long))
        setValue(boost::any_cast<long>(value));
    else if (value.type() == typeid(int))
        setValue(boost::any_cast<int>(value));
    else if (value.type() == typeid(double))
        setValue(boost::math::round(boost::any_cast<double>(value)));
    else if (value.type() == typeid(float))
        setValue(boost::math::round(boost::any_cast<float>(value)));
    else if (value.type() == typeid(Quantity))
        setValue(boost::math::round(boost::any_cast<Quantity>(value).getValue()));
    else
        throw bad_cast();
}


//**************************************************************************
//**************************************************************************
// PropertyPath
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyPath , App::Property)

//**************************************************************************
// Construction/Destruction

PropertyPath::PropertyPath() = default;

PropertyPath::~PropertyPath() = default;


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
#if (BOOST_FILESYSTEM_VERSION == 2)
    _cValue = boost::filesystem::path(Path,boost::filesystem::no_check );
    //_cValue = boost::filesystem::path(Path,boost::filesystem::native );
    //_cValue = boost::filesystem::path(Path,boost::filesystem::windows_name );
#else
    _cValue = boost::filesystem::path(Path);
#endif
    hasSetValue();
}

const boost::filesystem::path &PropertyPath::getValue() const
{
    return _cValue;
}

PyObject *PropertyPath::getPyObject()
{
#if (BOOST_FILESYSTEM_VERSION == 2)
    std::string str = _cValue.native_file_string();
#else
    std::string str = _cValue.string();
#endif

    // Returns a new reference, don't increment it!
    PyObject *p = PyUnicode_DecodeUTF8(str.c_str(),str.size(),nullptr);
    if (!p) throw Base::UnicodeError("UTF8 conversion failure at PropertyPath::getPyObject()");
    return p;
}

void PropertyPath::setPyObject(PyObject *value)
{
    std::string path;
    if (PyUnicode_Check(value)) {
        path = PyUnicode_AsUTF8(value);
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

void PropertyPath::Restore(Base::DocumentReader &reader,XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *ContainerDOM)
{
	auto PathDOM = reader.FindElement(ContainerDOM,"Path");
	if(PathDOM){
		// get the value of my Attribute
		const char* value_cstr = reader.GetAttribute(PathDOM,"value");
		if(value_cstr){
			setValue(value_cstr);
		}
	}
}

Property *PropertyPath::Copy() const
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

unsigned int PropertyPath::getMemSize () const
{
    return static_cast<unsigned int>(_cValue.string().size());
}

//**************************************************************************
//**************************************************************************
// PropertyEnumeration
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyEnumeration, App::PropertyInteger)

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

PropertyEnumeration::~PropertyEnumeration() = default;

void PropertyEnumeration::setEnums(const char **plEnums)
{
    // For backward compatibility, if the property container is not attached to
    // any document (i.e. its full name starts with '?'), do not notify, or
    // else existing code may crash.
    bool notify = !boost::starts_with(getFullName(), "?");
    if (notify)
        aboutToSetValue();
    _enum.setEnums(plEnums);
    if (notify)
        hasSetValue();
}

void PropertyEnumeration::setEnums(const std::vector<std::string> &Enums)
{
    setEnumVector(Enums);
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

long PropertyEnumeration::getValue() const
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

const char * PropertyEnumeration::getValueAsString() const
{
    if (!_enum.isValid())
        throw Base::RuntimeError("Cannot get value from invalid enumeration");
    return _enum.getCStr();
}

const Enumeration & PropertyEnumeration::getEnum() const
{
    return _enum;
}

std::vector<std::string> PropertyEnumeration::getEnumVector() const
{
    return _enum.getEnumVector();
}

void PropertyEnumeration::setEnumVector(const std::vector<std::string> &values)
{
    // For backward compatibility, if the property container is not attached to
    // any document (i.e. its full name starts with '?'), do not notify, or
    // else existing code may crash.
    bool notify = !boost::starts_with(getFullName(), "?");
    if (notify)
        aboutToSetValue();
    _enum.setEnums(values);
    if (notify)
        hasSetValue();
}

bool PropertyEnumeration::hasEnums() const
{
    return _enum.hasEnums();
}

bool PropertyEnumeration::isValid() const
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
        for(auto & item : items) {
            std::string val = encodeAttribute(item);
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

    aboutToSetValue();

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
        // If the enum is empty at this stage do not print a warning
        if (_enum.hasEnums())
            Base::Console().Warning("Enumeration index %d is out of range, ignore it\n", val);
        val = getValue();
    }

    _enum.setValue(val);
    hasSetValue();
}

void PropertyEnumeration::Restore(Base::DocumentReader &reader,XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *ContainerDOM)
{	
	auto IntegerDOM = reader.FindElement(ContainerDOM,"Integer");
	if(IntegerDOM){
		// get the value of my Attribute
		const char* value_cstr = reader.GetAttribute(IntegerDOM,"value");
		if(value_cstr){
			long val = reader.ContentToInt( value_cstr );
			aboutToSetValue();
			
			const char* CustomEnum_cstr = reader.GetAttribute(IntegerDOM,"CustomEnum");
			if(CustomEnum_cstr){
				auto CustomEnumListDOM = reader.FindElement(IntegerDOM,"CustomEnumList");
				if(CustomEnumListDOM){
					const char* count_cstr = reader.GetAttribute(IntegerDOM,"count");
					if(count_cstr){
						long count = reader.ContentToInt( count_cstr );
						std::vector<std::string> values(count);
						
						if(count >= 1){
							auto prev_EnumDOM = reader.FindElement(CustomEnumListDOM,"Enum");
							const char* enum_value_cstr = reader.GetAttribute(prev_EnumDOM,"value");
							values[0] = enum_value_cstr;
							for(int i = 1; i < count; i++) {
								auto EnumDOM_i = reader.FindNextElement(prev_EnumDOM,"Enum");
								const char* _enum_value_cstr = reader.GetAttribute(EnumDOM_i,"value");
								values[i] = _enum_value_cstr;
								
								prev_EnumDOM = EnumDOM_i;
							}
						}
						_enum.setEnums(values);
					}
				}
			}
			if (val < 0) {
				// If the enum is empty at this stage do not print a warning
				if (_enum.hasEnums())
				    Base::Console().Warning("Enumeration index %d is out of range, ignore it\n", val);
				val = getValue();
			}
			
			_enum.setValue(val);
    		hasSetValue();
		}
	}
}

PyObject * PropertyEnumeration::getPyObject()
{
    if (!_enum.isValid()) {
        Py_Return;
    }

    return Py_BuildValue("s", getValueAsString());
}

void PropertyEnumeration::setPyObject(PyObject *value)
{
    if (PyLong_Check(value)) {
        long val = PyLong_AsLong(value);
        if (_enum.isValid()) {
            aboutToSetValue();
            _enum.setValue(val, true);
            hasSetValue();
        }
        return;
    }
    else if (PyUnicode_Check(value)) {
        std::string str = PyUnicode_AsUTF8(value);
        if (_enum.contains(str.c_str())) {
            aboutToSetValue();
            _enum.setValue(str);
            hasSetValue();
        }
        else {
            FC_THROWM(Base::ValueError, "'" << str 
                    << "' is not part of the enumeration in "
                    << getFullName());
        }
        return;
    }
    else if (PySequence_Check(value)) {

        try {
            std::vector<std::string> values;

            int idx = -1;
            Py::Sequence seq(value);

            if(seq.size() == 2) {
                Py::Object v(seq[0].ptr());
                if(!v.isString() && v.isSequence()) {
                    idx = Py::Int(seq[1].ptr());
                    seq = v;
                }
            }

            values.resize(seq.size());

            for (int i = 0; i < seq.size(); ++i)
                values[i] = Py::Object(seq[i].ptr()).as_string();

            aboutToSetValue();
            _enum.setEnums(values);
            if (idx>=0)
                _enum.setValue(idx,true);
            hasSetValue();
            return;
        } catch (Py::Exception &) {
            Base::PyException e;
            e.ReportException();
        }
    }

    FC_THROWM(Base::TypeError, "PropertyEnumeration " << getFullName()
            << " expects type to be int, string, or list(string), or list(list, int)");
}

Property * PropertyEnumeration::Copy() const
{
    return new PropertyEnumeration(_enum);
}

void PropertyEnumeration::Paste(const Property &from)
{
    const PropertyEnumeration& prop = dynamic_cast<const PropertyEnumeration&>(from);
    setValue(prop._enum);
}

void PropertyEnumeration::setPathValue(const ObjectIdentifier &, const boost::any &value)
{
    if (value.type() == typeid(int))
        setValue(boost::any_cast<int>(value));
    else if (value.type() == typeid(long))
        setValue(boost::any_cast<long>(value));
    else if (value.type() == typeid(double))
        setValue(boost::any_cast<double>(value));
    else if (value.type() == typeid(float))
        setValue(boost::any_cast<float>(value));
    else if (value.type() == typeid(short))
        setValue(boost::any_cast<short>(value));
    else if (value.type() == typeid(std::string))
        setValue(boost::any_cast<std::string>(value).c_str());
    else if (value.type() == typeid(char*))
        setValue(boost::any_cast<char*>(value));
    else if (value.type() == typeid(const char*))
        setValue(boost::any_cast<const char*>(value));
    else {
        Base::PyGILStateLocker lock;
        Py::Object pyValue = pyObjectFromAny(value);
        setPyObject(pyValue.ptr());
    }
}

bool PropertyEnumeration::setPyPathValue(const ObjectIdentifier &, const Py::Object &value)
{
    setPyObject(value.ptr());
    return true;
}

const boost::any PropertyEnumeration::getPathValue(const ObjectIdentifier &path) const
{
    std::string p = path.getSubPathStr();
    if (p == ".Enum" || p == ".All") {
        Base::PyGILStateLocker lock;
        Py::Object res;
        getPyPathValue(path, res);
        return pyObjectToAny(res,false);
    }
    else if (p == ".String") {
        auto v = getValueAsString();
        return std::string(v?v:"");
    } else
        return getValue();
}

bool PropertyEnumeration::getPyPathValue(const ObjectIdentifier &path, Py::Object &r) const
{
    std::string p = path.getSubPathStr();
    if (p == ".Enum" || p == ".All") {
        Base::PyGILStateLocker lock;
        Py::Tuple res(_enum.maxValue()+1);
        std::vector<std::string> enums = _enum.getEnumVector();
        PropertyString tmp;
        for(int i=0;i< int(enums.size());++i) {
            tmp.setValue(enums[i]);
            res.setItem(i,Py::asObject(tmp.getPyObject()));
        }
        if (p == ".Enum")
            r = res;
        else {
            Py::Tuple tuple(2);
            tuple.setItem(0, res);
            tuple.setItem(1, Py::Int(getValue()));
            r = tuple;
        }
    } else if (p == ".String") {
        auto v = getValueAsString();
        r = Py::String(v?v:"");
    } else 
        r = Py::Int(getValue());
    return true;
}

//**************************************************************************
//**************************************************************************
// PropertyIntegerConstraint
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyIntegerConstraint, App::PropertyInteger)

//**************************************************************************
// Construction/Destruction


PropertyIntegerConstraint::PropertyIntegerConstraint() = default;

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

const PropertyIntegerConstraint::Constraints*  PropertyIntegerConstraint::getConstraints() const
{
    return _ConstStruct;
}

long PropertyIntegerConstraint::getMinimum() const
{
    if (_ConstStruct)
        return _ConstStruct->LowerBound;
    // return the min of int, not long
    return std::numeric_limits<int>::min();
}

long PropertyIntegerConstraint::getMaximum() const
{
    if (_ConstStruct)
        return _ConstStruct->UpperBound;
    // return the max of int, not long
    return std::numeric_limits<int>::max();
}

long PropertyIntegerConstraint::getStepSize() const
{
    if (_ConstStruct)
        return _ConstStruct->StepSize;
    return 1;
}

void PropertyIntegerConstraint::setPyObject(PyObject *value)
{
    if (PyLong_Check(value)) {
        long temp = PyLong_AsLong(value);
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
            if (PyLong_Check(item))
                values[i] = PyLong_AsLong(item);
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

TYPESYSTEM_SOURCE(App::PropertyPercent , App::PropertyIntegerConstraint)

const PropertyIntegerConstraint::Constraints percent = {0,100,1};

//**************************************************************************
// Construction/Destruction


PropertyPercent::PropertyPercent()
{
    _ConstStruct = &percent;
}

PropertyPercent::~PropertyPercent() = default;

//**************************************************************************
//**************************************************************************
// PropertyIntegerList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyIntegerList , App::PropertyLists)

//**************************************************************************
// Construction/Destruction


PropertyIntegerList::PropertyIntegerList() = default;

PropertyIntegerList::~PropertyIntegerList() = default;

//**************************************************************************
// Base class implementer

PyObject *PropertyIntegerList::getPyObject()
{
    PyObject* list = PyList_New(getSize());
    for(int i = 0;i<getSize(); i++)
        PyList_SetItem( list, i, PyLong_FromLong(_lValueList[i]));
    return list;
}

long PropertyIntegerList::getPyValue(PyObject *item) const {
    if (PyLong_Check(item))
        return PyLong_AsLong(item);
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

void PropertyIntegerList::Restore(Base::DocumentReader &reader,XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *ContainerDOM)
{
	auto IntegerListDOM = reader.FindElement(ContainerDOM,"IntegerList");
	if(IntegerListDOM){
		// get the value of my Attribute
		const char* count_cstr = reader.GetAttribute(IntegerListDOM,"count");
		long count = reader.ContentToInt( count_cstr );	
		std::vector<long> values(count);
		if(count >= 1){
			auto prev_I_DOM = reader.FindElement(IntegerListDOM,"I");
			const char* v_cstr = reader.GetAttribute(prev_I_DOM,"v");
			long v = reader.ContentToInt( v_cstr );
			values[0] = v;
			
			for(int i = 1; i < count; i++) {
				auto I_DOM_i = reader.FindNextElement(prev_I_DOM,"I");
				const char* _v_cstr = reader.GetAttribute(I_DOM_i,"v");
				long v = reader.ContentToInt( _v_cstr );
				values[i] = v;
				
				prev_I_DOM = I_DOM_i;
			}
		}
		//assignment
		setValues(values);
	}
}


Property *PropertyIntegerList::Copy() const
{
    PropertyIntegerList *p= new PropertyIntegerList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyIntegerList::Paste(const Property &from)
{
    setValues(dynamic_cast<const PropertyIntegerList&>(from)._lValueList);
}

unsigned int PropertyIntegerList::getMemSize () const
{
    return static_cast<unsigned int>(_lValueList.size() * sizeof(long));
}




//**************************************************************************
//**************************************************************************
// PropertyIntegerSet
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyIntegerSet , App::Property)

//**************************************************************************
// Construction/Destruction


PropertyIntegerSet::PropertyIntegerSet() = default;

PropertyIntegerSet::~PropertyIntegerSet() = default;


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

PyObject *PropertyIntegerSet::getPyObject()
{
    PyObject* set = PySet_New(nullptr);
    for(long it : _lValueSet)
        PySet_Add(set,PyLong_FromLong(it));
    return set;
}

void PropertyIntegerSet::setPyObject(PyObject *value)
{
    if (PySequence_Check(value)) {

        Py_ssize_t nSize = PySequence_Length(value);
        std::set<long> values;

        for (Py_ssize_t i=0; i<nSize;++i) {
            PyObject* item = PySequence_GetItem(value, i);
            if (!PyLong_Check(item)) {
                std::string error = std::string("type in list must be int, not ");
                error += item->ob_type->tp_name;
                throw Base::TypeError(error);
            }
            values.insert(PyLong_AsLong(item));
        }

        setValues(values);
    }
    else if (PyLong_Check(value)) {
        setValue(PyLong_AsLong(value));
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
    for(long it : _lValueSet)
        writer.Stream() << writer.ind() << "<I v=\"" <<  it <<"\"/>" << endl; ;
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

void PropertyIntegerSet::Restore(Base::DocumentReader &reader,XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *ContainerDOM)
{
	auto IntegerSetDOM = reader.FindElement(ContainerDOM,"IntegerSet");
	if(IntegerSetDOM){
		// get the value of my Attribute
		const char* count_cstr = reader.GetAttribute(IntegerSetDOM,"count");
		long count = reader.ContentToInt( count_cstr );	
		std::set<long> values;
		
		if(count >= 1){
			auto prev_I_DOM = reader.FindElement(IntegerSetDOM,"I");
			const char* v_cstr = reader.GetAttribute(prev_I_DOM,"v");
			long v = reader.ContentToInt( v_cstr );
			values.insert( v );
			
			for(int i = 1; i < count; i++) {
				auto I_DOM_i = reader.FindNextElement(prev_I_DOM,"I");
				const char* _v_cstr = reader.GetAttribute(I_DOM_i,"v");
				long v = reader.ContentToInt( _v_cstr );
				values.insert( v );
				
				prev_I_DOM = I_DOM_i;
			}
		}
		//assignment
		setValues(values);
	}
}

Property *PropertyIntegerSet::Copy() const
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

unsigned int PropertyIntegerSet::getMemSize () const
{
    return static_cast<unsigned int>(_lValueSet.size() * sizeof(long));
}



//**************************************************************************
//**************************************************************************
// PropertyFloat
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyFloat , App::Property)

//**************************************************************************
// Construction/Destruction


PropertyFloat::PropertyFloat()
{
    _dValue = 0.0;
}

PropertyFloat::~PropertyFloat() = default;

//**************************************************************************
// Base class implementer

void PropertyFloat::setValue(double lValue)
{
    aboutToSetValue();
    _dValue=lValue;
    hasSetValue();
}

double PropertyFloat::getValue() const
{
    return _dValue;
}

PyObject *PropertyFloat::getPyObject()
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
    else if(PyLong_Check(value)) {
        aboutToSetValue();
        _dValue = PyLong_AsLong(value);
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

void PropertyFloat::Restore(Base::DocumentReader &reader,XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *ContainerDOM)
{
	auto FloatDOM = reader.FindElement(ContainerDOM,"Float");
	if(FloatDOM){
		// get the value of my Attribute
		const char* value_cstr = reader.GetAttribute(FloatDOM,"value");
		if(value_cstr){
			double value = reader.ContentToFloat( value_cstr );
			setValue(value);
		}
	}
}



Property *PropertyFloat::Copy() const
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

void PropertyFloat::setPathValue(const ObjectIdentifier &path, const boost::any &value)
{
    verifyPath(path);

    if (value.type() == typeid(long))
        setValue(boost::any_cast<long>(value));
    else if (value.type() == typeid(unsigned long))
        setValue(boost::any_cast<unsigned long>(value));
    else if (value.type() == typeid(int))
        setValue(boost::any_cast<int>(value));
    else if (value.type() == typeid(double))
        setValue(boost::any_cast<double>(value));
    else if (value.type() == typeid(float))
        setValue(boost::any_cast<float>(value));
    else if (value.type() == typeid(Quantity))
        setValue((boost::any_cast<Quantity>(value)).getValue());
    else
        throw bad_cast();
}

const boost::any PropertyFloat::getPathValue(const ObjectIdentifier &path) const
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


PropertyFloatConstraint::PropertyFloatConstraint() = default;

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

const PropertyFloatConstraint::Constraints*  PropertyFloatConstraint::getConstraints() const
{
    return _ConstStruct;
}

double PropertyFloatConstraint::getMinimum() const
{
    if (_ConstStruct)
        return _ConstStruct->LowerBound;
    return std::numeric_limits<double>::min();
}

double PropertyFloatConstraint::getMaximum() const
{
    if (_ConstStruct)
        return _ConstStruct->UpperBound;
    return std::numeric_limits<double>::max();
}

double PropertyFloatConstraint::getStepSize() const
{
    if (_ConstStruct)
        return _ConstStruct->StepSize;
    return 1.0;
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
    else if (PyLong_Check(value)) {
        double temp = (double)PyLong_AsLong(value);
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
            else if (PyLong_Check(item))
                values[i] = PyLong_AsLong(item);
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

TYPESYSTEM_SOURCE(App::PropertyPrecision, App::PropertyFloatConstraint)

//**************************************************************************
// Construction/Destruction
//
const PropertyFloatConstraint::Constraints PrecisionStandard = {0.0,DBL_MAX,0.001};

PropertyPrecision::PropertyPrecision()
{
    setConstraints(&PrecisionStandard);
}

PropertyPrecision::~PropertyPrecision() = default;


//**************************************************************************
// PropertyFloatList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyFloatList , App::PropertyLists)

//**************************************************************************
// Construction/Destruction


PropertyFloatList::PropertyFloatList() = default;

PropertyFloatList::~PropertyFloatList() = default;

//**************************************************************************
// Base class implementer

PyObject *PropertyFloatList::getPyObject()
{
    PyObject* list = PyList_New(getSize());
    for (int i = 0;i<getSize(); i++)
         PyList_SetItem( list, i, PyFloat_FromDouble(_lValueList[i]));
    return list;
}

double PropertyFloatList::getPyValue(PyObject *item) const {
    if (PyFloat_Check(item)) {
        return PyFloat_AsDouble(item);
    } else if (PyLong_Check(item)) {
        return static_cast<double>(PyLong_AsLong(item));
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

void PropertyFloatList::Restore(Base::DocumentReader &reader,XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *ContainerDOM)
{
	//TODO for this its needed to implement addFile() into Reader.cpp, and ReadFiles() since its not possible to have an XMLReader object at this point, because it gives error to use both methods(XMLReader progressive reading and DocumentReader reading) probably because there is something wrong with the zipios implementation, it looks like its locking file or in some way makes the file structure invalid to be readed by xerces by both methods.
	//worked around reimplementing ReadFiles in DocumentReader.cpp:
	auto FloatListDOM = reader.FindElement(ContainerDOM,"FloatList");
	if(FloatListDOM){
		// get the value of my Attribute
		const char* file_cstr = reader.GetAttribute(FloatListDOM,"file");
		if(file_cstr){
			string file ( file_cstr );
			if (!file.empty()) {
				// initiate a file read
				reader.addFile(file.c_str(),this);
			}
		}
	}
}

void PropertyFloatList::SaveDocFile (Base::Writer &writer) const
{
    Base::OutputStream str(writer.Stream());
    uint32_t uCt = (uint32_t)getSize();
    str << uCt;
    if (!isSinglePrecision()) {
        for (double it : _lValueList) {
            str << it;
        }
    }
    else {
        for (double it : _lValueList) {
            float v = static_cast<float>(it);
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
        for (double & it : values) {
            str >> it;
        }
    }
    else {
        for (double & it : values) {
            float val;
            str >> val;
            it = val;
        }
    }
    setValues(values);
}

Property *PropertyFloatList::Copy() const
{
    PropertyFloatList *p= new PropertyFloatList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyFloatList::Paste(const Property &from)
{
    setValues(dynamic_cast<const PropertyFloatList&>(from)._lValueList);
}

unsigned int PropertyFloatList::getMemSize () const
{
    return static_cast<unsigned int>(_lValueList.size() * sizeof(double));
}

//**************************************************************************
//**************************************************************************
// PropertyString
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyString , App::Property)

PropertyString::PropertyString() = default;

PropertyString::~PropertyString() = default;

void PropertyString::setValue(const char* newLabel)
{
    if(!newLabel)
        return;

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
                if(label != obj->getNameInDocument()
                        && boost::starts_with(obj->getNameInDocument(),label))
                {
                    // In case the label has the same base name as object's
                    // internal name, use it as the label instead.
                    const char *objName = obj->getNameInDocument();
                    const char *c = &objName[lastpos+1];
                    for(;*c;++c) {
                        if(*c<48 || *c>57)
                            break;
                    }
                    if(*c == 0 && std::find(objectLabels.begin(), objectLabels.end(),
                                            obj->getNameInDocument())==objectLabels.end())
                    {
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

        if(!propChanges.empty() && !GetApplication().getActiveTransaction()) {
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

const char* PropertyString::getValue() const
{
    return _cValue.c_str();
}

PyObject *PropertyString::getPyObject()
{
    PyObject *p = PyUnicode_DecodeUTF8(_cValue.c_str(),_cValue.size(),nullptr);
    if (!p) throw Base::UnicodeError("UTF8 conversion failure at PropertyString::getPyObject()");
    return p;
}

void PropertyString::setPyObject(PyObject *value)
{
    std::string string;
    if (PyUnicode_Check(value)) {
        string = PyUnicode_AsUTF8(value);
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

void PropertyString::Restore(Base::DocumentReader &reader,XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *ContainerDOM)
{
	auto StringDOM = reader.FindElement(ContainerDOM,"String");
	const char* value_cstr = reader.GetAttribute(StringDOM,"value");
	
	auto obj = dynamic_cast<DocumentObject*>(getContainer());
    if(obj && &obj->Label==this) {
		const char* restore_cstr = reader.GetAttribute(StringDOM,"restore");
		
        if(restore_cstr) {
        	int restore = reader.ContentToInt( restore_cstr );
            if(restore == 1) {
                aboutToSetValue();
                _cValue = value_cstr;
                hasSetValue();
                return;
            }
        	return;
        }
    }
    setValue(value_cstr);
        
	
}

Property *PropertyString::Copy() const
{
    PropertyString *p= new PropertyString();
    p->_cValue = _cValue;
    return p;
}

void PropertyString::Paste(const Property &from)
{
    setValue(dynamic_cast<const PropertyString&>(from)._cValue);
}

unsigned int PropertyString::getMemSize () const
{
    return static_cast<unsigned int>(_cValue.size());
}

void PropertyString::setPathValue(const ObjectIdentifier &path, const boost::any &value)
{
    verifyPath(path);
    if (value.type() == typeid(bool))
        setValue(boost::any_cast<bool>(value)?"True":"False");
    else if (value.type() == typeid(int))
        setValue(std::to_string(boost::any_cast<int>(value)));
    else if (value.type() == typeid(long))
        setValue(std::to_string(boost::any_cast<long>(value)));
    else if (value.type() == typeid(double))
        setValue(std::to_string(App::any_cast<double>(value)));
    else if (value.type() == typeid(float))
        setValue(std::to_string(App::any_cast<float>(value)));
    else if (value.type() == typeid(Quantity))
        setValue(boost::any_cast<Quantity>(value).getUserString().toUtf8().constData());
    else if (value.type() == typeid(std::string))
        setValue(boost::any_cast<const std::string &>(value));
    else {
        Base::PyGILStateLocker lock;
        setValue(pyObjectFromAny(value).as_string());
    }
}

const boost::any PropertyString::getPathValue(const ObjectIdentifier &path) const
{
    verifyPath(path);
    return _cValue;
}

//**************************************************************************
//**************************************************************************
// PropertyUUID
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyUUID , App::Property)

PropertyUUID::PropertyUUID() = default;

PropertyUUID::~PropertyUUID() = default;

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

const std::string& PropertyUUID::getValueStr() const
{
    return _uuid.getValue();
}

const Base::Uuid& PropertyUUID::getValue() const
{
    return _uuid;
}

PyObject *PropertyUUID::getPyObject()
{
    PyObject *p = PyUnicode_FromString(_uuid.getValue().c_str());
    return p;
}

void PropertyUUID::setPyObject(PyObject *value)
{
    std::string string;
    if (PyUnicode_Check(value)) {
        string = PyUnicode_AsUTF8(value);
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

void PropertyUUID::Restore(Base::DocumentReader &reader,XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *ContainerDOM)
{
	auto UuidDOM = reader.FindElement(ContainerDOM,"Uuid");
	if(UuidDOM){
		// get the value of my Attribute
		const char* value_cstr = reader.GetAttribute(UuidDOM,"value");
		if(value_cstr){
			setValue(value_cstr);
		}
	}
}

Property *PropertyUUID::Copy() const
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

unsigned int PropertyUUID::getMemSize () const
{
    return static_cast<unsigned int>(sizeof(_uuid));
}

//**************************************************************************
// PropertyFont
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyFont , App::PropertyString)

PropertyFont::PropertyFont() = default;

PropertyFont::~PropertyFont() = default;

//**************************************************************************
// PropertyStringList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyStringList , App::PropertyLists)

PropertyStringList::PropertyStringList() = default;

PropertyStringList::~PropertyStringList() = default;

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

PyObject *PropertyStringList::getPyObject()
{
    PyObject* list = PyList_New(getSize());

    for (int i = 0;i<getSize(); i++) {
        PyObject* item = PyUnicode_DecodeUTF8(_lValueList[i].c_str(), _lValueList[i].size(), nullptr);
        if (!item) {
            Py_DECREF(list);
            throw Base::UnicodeError("UTF8 conversion failure at PropertyStringList::getPyObject()");
        }
        PyList_SetItem(list, i, item);
    }

    return list;
}

std::string PropertyStringList::getPyValue(PyObject *item) const
{
    std::string ret;
    if (PyUnicode_Check(item)) {
        ret = PyUnicode_AsUTF8(item);
    } else if (PyBytes_Check(item)) {
        ret = PyBytes_AsString(item);
    } else {
        std::string error = std::string("type in list must be str or unicode, not ");
        error += item->ob_type->tp_name;
        throw Base::TypeError(error);
    }
    return ret;
}

unsigned int PropertyStringList::getMemSize () const
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

void PropertyStringList::Restore(Base::DocumentReader &reader,XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *ContainerDOM)
{
	auto StringListDOM = reader.FindElement(ContainerDOM,"StringList");
	if(StringListDOM){
		// get the value of my Attribute
		const char* count_cstr = reader.GetAttribute(StringListDOM,"count");
		long count = reader.ContentToInt( count_cstr );
		std::vector<std::string> values(count);
		if(count >= 1){
			auto prev_StringDOM = reader.FindElement(StringListDOM,"String");
			const char* value_cstr = reader.GetAttribute(prev_StringDOM,"value");
			values[0] = value_cstr;
			for(int i = 1; i < count; i++) {
				auto StringDOM_i = reader.FindNextElement(prev_StringDOM,"String");
				const char* _value_cstr = reader.GetAttribute(StringDOM_i,"value");
				values[i] = _value_cstr;
				
				prev_StringDOM = StringDOM_i;
			}
		
		}
		// assignment
    	setValues(values);
	}
}

Property *PropertyStringList::Copy() const
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

TYPESYSTEM_SOURCE(App::PropertyMap , App::Property)

PropertyMap::PropertyMap() = default;

PropertyMap::~PropertyMap() = default;

//**************************************************************************
// Base class implementer


int PropertyMap::getSize() const
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


PyObject *PropertyMap::getPyObject()
{
    PyObject* dict = PyDict_New();

    for (std::map<std::string,std::string>::const_iterator it = _lValueList.begin();it!= _lValueList.end(); ++it) {
        PyObject* item = PyUnicode_DecodeUTF8(it->second.c_str(), it->second.size(), nullptr);
        if (!item) {
            Py_DECREF(dict);
            throw Base::UnicodeError("UTF8 conversion failure at PropertyMap::getPyObject()");
        }
        PyDict_SetItemString(dict,it->first.c_str(),item);
        Py_DECREF(item);
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
                keyStr = PyUnicode_AsUTF8(key);
            }
            else {
                std::string error = std::string("type of the key need to be unicode or string, not");
                error += key->ob_type->tp_name;
                throw Base::TypeError(error);
            }

            // check on the item:
            PyObject* item = PyList_GetItem(itemList, i);
            if (PyUnicode_Check(item)) {
                values[keyStr] = PyUnicode_AsUTF8(item);
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

unsigned int PropertyMap::getMemSize () const
{
    size_t size=0;
    for (const auto & it : _lValueList) {
        size += it.second.size();
        size += it.first.size();
    }
    return size;
}

void PropertyMap::Save (Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<Map count=\"" <<  getSize() <<"\">" << endl;
    writer.incInd();
    for (const auto & it : _lValueList) {
        writer.Stream() << writer.ind() << "<Item key=\"" <<  encodeAttribute(it.first)
                                        << "\" value=\"" <<  encodeAttribute(it.second) <<"\"/>" << endl;
    }

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

void PropertyMap::Restore(Base::DocumentReader &reader,XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *ContainerDOM)
{
	auto MapDOM = reader.FindElement(ContainerDOM,"Map");
	if(MapDOM){
		// get the value of my Attribute
		const char* count_cstr = reader.GetAttribute(MapDOM,"count");
		int count = reader.ContentToInt( count_cstr );
		std::map<std::string,std::string> values;
		auto prev_ItemDOM = reader.FindElement(MapDOM,"Item");
		const char* key_cstr = reader.GetAttribute(prev_ItemDOM,"key");
		const char* value_cstr = reader.GetAttribute(prev_ItemDOM,"value");
		values[key_cstr] = value_cstr;
		
		for(int i = 1; i < count; i++) {
			auto ItemDOM_i = reader.FindNextElement(prev_ItemDOM,"Item");
			const char* key_cstr = reader.GetAttribute(ItemDOM_i,"key");
			const char* value_cstr = reader.GetAttribute(ItemDOM_i,"value");
			values[key_cstr] = value_cstr;
			
			prev_ItemDOM = ItemDOM_i;
		}
		// assignment
		setValues(values);
	}
}

Property *PropertyMap::Copy() const
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

TYPESYSTEM_SOURCE(App::PropertyBool , App::Property)

//**************************************************************************
// Construction/Destruction

PropertyBool::PropertyBool()
{
    _lValue = false;
}

PropertyBool::~PropertyBool() = default;

//**************************************************************************
// Setter/getter for the property

void PropertyBool::setValue(bool lValue)
{
    aboutToSetValue();
    _lValue=lValue;
    hasSetValue();
}

bool PropertyBool::getValue() const
{
    return _lValue;
}

PyObject *PropertyBool::getPyObject()
{
    return PyBool_FromLong(_lValue ? 1 : 0);
}

void PropertyBool::setPyObject(PyObject *value)
{
    if (PyBool_Check(value) || PyLong_Check(value)) {
        setValue(Base::asBoolean(value));
    }
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

void PropertyBool::Restore(Base::DocumentReader &reader,XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *ContainerDOM)
{
	auto BoolDOM = reader.FindElement(ContainerDOM,"Bool");
	if(BoolDOM){
		// get the value of my Attribute
		string b = reader.GetAttribute(BoolDOM,"value");
		(b == "true") ? setValue(true) : setValue(false);
	}
}

Property *PropertyBool::Copy() const
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

void PropertyBool::setPathValue(const ObjectIdentifier &path, const boost::any &value)
{
    verifyPath(path);

    if (value.type() == typeid(bool))
        setValue(boost::any_cast<bool>(value));
    else if (value.type() == typeid(int))
        setValue(boost::any_cast<int>(value) != 0);
    else if (value.type() == typeid(long))
        setValue(boost::any_cast<long>(value) != 0);
    else if (value.type() == typeid(double))
        setValue(boost::math::round(boost::any_cast<double>(value)));
    else if (value.type() == typeid(float))
        setValue(boost::math::round(boost::any_cast<float>(value)));
    else if (value.type() == typeid(Quantity))
        setValue(boost::any_cast<Quantity>(value).getValue() != 0);
    else
        throw bad_cast();
}

const boost::any PropertyBool::getPathValue(const ObjectIdentifier &path) const
{
    verifyPath(path);

    return _lValue;
}

//**************************************************************************
//**************************************************************************
// PropertyBoolList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyBoolList , App::PropertyLists)

//**************************************************************************
// Construction/Destruction


PropertyBoolList::PropertyBoolList() = default;

PropertyBoolList::~PropertyBoolList() = default;

//**************************************************************************
// Base class implementer

PyObject *PropertyBoolList::getPyObject()
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
        str = PyUnicode_AsUTF8(value);
        boost::dynamic_bitset<> values(str);
        setValues(values);
    }else
        inherited::setPyObject(value);
}

bool PropertyBoolList::getPyValue(PyObject *item) const {
    if (PyBool_Check(item)) {
        return Base::asBoolean(item);
    } else if (PyLong_Check(item)) {
        return (PyLong_AsLong(item) ? true : false);
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

void PropertyBoolList::Restore(Base::DocumentReader &reader,XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *ContainerDOM)
{
	// read my Element
	auto BoolListDOM = reader.FindElement(ContainerDOM,"BoolList");
	if(BoolListDOM){
		// get the value of my Attribute
		string str = reader.GetAttribute(BoolListDOM,"value");
		boost::dynamic_bitset<> bitset(str);
    	setValues(bitset);
	}
}

Property *PropertyBoolList::Copy() const
{
    PropertyBoolList *p= new PropertyBoolList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyBoolList::Paste(const Property &from)
{
    setValues(dynamic_cast<const PropertyBoolList&>(from)._lValueList);
}

unsigned int PropertyBoolList::getMemSize () const
{
    return static_cast<unsigned int>(_lValueList.size());
}

//**************************************************************************
//**************************************************************************
// PropertyColor
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyColor , App::Property)

//**************************************************************************
// Construction/Destruction

PropertyColor::PropertyColor() = default;

PropertyColor::~PropertyColor() = default;

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

const Color& PropertyColor::getValue() const
{
    return _cCol;
}

PyObject *PropertyColor::getPyObject()
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
    if (PyTuple_Check(value) && (PyTuple_Size(value) == 3 || PyTuple_Size(value) == 4) ) {
        PyObject* item;
        item = PyTuple_GetItem(value,0);
        if (PyFloat_Check(item)) {
            cCol.r = (float)PyFloat_AsDouble(item);
            item = PyTuple_GetItem(value,1);
            if (PyFloat_Check(item))
                cCol.g = (float)PyFloat_AsDouble(item);
            else
                throw Base::TypeError("Type in tuple must be consistent (float)");
            item = PyTuple_GetItem(value,2);
            if (PyFloat_Check(item))
                cCol.b = (float)PyFloat_AsDouble(item);
            else
                throw Base::TypeError("Type in tuple must be consistent (float)");
            if (PyTuple_Size(value) == 4) {
                item = PyTuple_GetItem(value,3);
                if (PyFloat_Check(item))
                    cCol.a = (float)PyFloat_AsDouble(item);
                else
                    throw Base::TypeError("Type in tuple must be consistent (float)");
            }
        }
        else if (PyLong_Check(item)) {
            cCol.r = PyLong_AsLong(item)/255.0;
            item = PyTuple_GetItem(value,1);
            if (PyLong_Check(item))
                cCol.g = PyLong_AsLong(item)/255.0;
            else
                throw Base::TypeError("Type in tuple must be consistent (integer)");
            item = PyTuple_GetItem(value,2);
            if (PyLong_Check(item))
                cCol.b = PyLong_AsLong(item)/255.0;
            else
                throw Base::TypeError("Type in tuple must be consistent (integer)");
            if (PyTuple_Size(value) == 4) {
                item = PyTuple_GetItem(value,3);
                if (PyLong_Check(item))
                    cCol.a = PyLong_AsLong(item)/255.0;
                else
                    throw Base::TypeError("Type in tuple must be consistent (integer)");
            }
        }
        else {
            throw Base::TypeError("Type in tuple must be float or integer");
        }
    }
    else if (PyLong_Check(value)) {
        cCol.setPackedValue(PyLong_AsUnsignedLong(value));
    }
    else {
        std::string error = std::string("type must be integer or tuple of float or tuple integer, not ");
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

void PropertyColor::Restore(Base::DocumentReader &reader,XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *ContainerDOM)
{
	// read my Element
	auto PropertyColorDOM = reader.FindElement(ContainerDOM,"PropertyColor");
	if(PropertyColorDOM){
		const char* val_cstr = reader.GetAttribute(PropertyColorDOM,"value");
		unsigned long rgba = reader.ContentToUnsigned(val_cstr);
    	setValue(rgba);
	}
}

Property *PropertyColor::Copy() const
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

TYPESYSTEM_SOURCE(App::PropertyColorList , App::PropertyLists)

//**************************************************************************
// Construction/Destruction

PropertyColorList::PropertyColorList() = default;

PropertyColorList::~PropertyColorList() = default;

//**************************************************************************
// Base class implementer

PyObject *PropertyColorList::getPyObject()
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

void PropertyColorList::Restore(Base::DocumentReader &reader,XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *ContainerDOM)
{
	auto ColorListDOM = reader.FindElement(ContainerDOM,"ColorList");
	if(ColorListDOM){
		// get the value of my Attribute
		const char* file_cstr = reader.GetAttribute(ColorListDOM,"file");
		if(file_cstr){
			std::string file (file_cstr);
			if (!file.empty()) {
		        // initiate a file read
		        reader.addFile(file.c_str(),this);
		    }
		}
	}
}

void PropertyColorList::SaveDocFile (Base::Writer &writer) const
{
    Base::OutputStream str(writer.Stream());
    uint32_t uCt = (uint32_t)getSize();
    str << uCt;
    for (auto it : _lValueList) {
        str << it.getPackedValue();
    }
}

void PropertyColorList::RestoreDocFile(Base::Reader &reader)
{
    Base::InputStream str(reader);
    uint32_t uCt=0;
    str >> uCt;
    std::vector<Color> values(uCt);
    uint32_t value; // must be 32 bit long
    for (auto & it : values) {
        str >> value;
        it.setPackedValue(value);
    }
    setValues(values);
}

Property *PropertyColorList::Copy() const
{
    PropertyColorList *p= new PropertyColorList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyColorList::Paste(const Property &from)
{
    setValues(dynamic_cast<const PropertyColorList&>(from)._lValueList);
}

unsigned int PropertyColorList::getMemSize () const
{
    return static_cast<unsigned int>(_lValueList.size() * sizeof(Color));
}

//**************************************************************************
//**************************************************************************
// PropertyMaterial
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyMaterial , App::Property)

PropertyMaterial::PropertyMaterial() = default;

PropertyMaterial::~PropertyMaterial() = default;

void PropertyMaterial::setValue(const Material &mat)
{
    aboutToSetValue();
    _cMat=mat;
    hasSetValue();
}

const Material& PropertyMaterial::getValue() const
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

PyObject *PropertyMaterial::getPyObject()
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
        << "\" diffuseColor=\""  <<  _cMat.diffuseColor.getPackedValue()
        << "\" specularColor=\"" <<  _cMat.specularColor.getPackedValue()
        << "\" emissiveColor=\"" <<  _cMat.emissiveColor.getPackedValue()
        << "\" shininess=\""     <<  _cMat.shininess
        << "\" transparency=\""  <<  _cMat.transparency
        << "\"/>" << endl;
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

void PropertyMaterial::Restore(Base::DocumentReader &reader,XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *ContainerDOM)
{
	// read my Element
	auto PropertyMaterialDOM = reader.FindElement(ContainerDOM,"PropertyMaterial");
	if(PropertyMaterialDOM){
		// get the value of my Attribute
		aboutToSetValue();
		
		const char* ambientColor_cstr = reader.GetAttribute(PropertyMaterialDOM,"ambientColor");
		const char* diffuseColor_cstr = reader.GetAttribute(PropertyMaterialDOM,"diffuseColor");
		const char* specularColor_cstr = reader.GetAttribute(PropertyMaterialDOM,"specularColor");
		const char* emissiveColor_cstr = reader.GetAttribute(PropertyMaterialDOM,"emissiveColor");
		const char* shininess_cstr = reader.GetAttribute(PropertyMaterialDOM,"shininess");
		const char* transparency_cstr = reader.GetAttribute(PropertyMaterialDOM,"transparency");
		
		_cMat.ambientColor.setPackedValue(reader.ContentToUnsigned(ambientColor_cstr));
		_cMat.diffuseColor.setPackedValue(reader.ContentToUnsigned(diffuseColor_cstr));
		_cMat.specularColor.setPackedValue(reader.ContentToUnsigned(specularColor_cstr));
		_cMat.emissiveColor.setPackedValue(reader.ContentToUnsigned(emissiveColor_cstr));
		_cMat.shininess = (float)reader.ContentToFloat(shininess_cstr);
		_cMat.transparency = (float)reader.ContentToFloat(transparency_cstr);
		hasSetValue();
	}
}


const char* PropertyMaterial::getEditorName() const
{
    if(testStatus(MaterialEdit))
        return "Gui::PropertyEditor::PropertyMaterialItem";
    return "";
}

Property *PropertyMaterial::Copy() const
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

TYPESYSTEM_SOURCE(App::PropertyMaterialList, App::PropertyLists)

//**************************************************************************
// Construction/Destruction

PropertyMaterialList::PropertyMaterialList() = default;

PropertyMaterialList::~PropertyMaterialList() = default;

//**************************************************************************
// Base class implementer

PyObject *PropertyMaterialList::getPyObject()
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

void PropertyMaterialList::Restore(Base::DocumentReader &reader,XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *ContainerDOM)
{
	auto MaterialListDOM = reader.FindElement(ContainerDOM,"MaterialList");
	if(MaterialListDOM){
		// get the value of my Attribute
		const char* file_cstr = reader.GetAttribute(MaterialListDOM,"file");
		if(file_cstr){
			std::string file (file_cstr);
			if (!file.empty()) {
		        // initiate a file read
		        reader.addFile(file.c_str(),this);
		    }
		}
	}
}

void PropertyMaterialList::SaveDocFile(Base::Writer &writer) const
{
    Base::OutputStream str(writer.Stream());
    uint32_t uCt = (uint32_t)getSize();
    str << uCt;
    for (const auto & it : _lValueList) {
        str << it.ambientColor.getPackedValue();
        str << it.diffuseColor.getPackedValue();
        str << it.specularColor.getPackedValue();
        str << it.emissiveColor.getPackedValue();
        str << it.shininess;
        str << it.transparency;
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
    for (auto & it : values) {
        str >> value;
        it.ambientColor.setPackedValue(value);
        str >> value;
        it.diffuseColor.setPackedValue(value);
        str >> value;
        it.specularColor.setPackedValue(value);
        str >> value;
        it.emissiveColor.setPackedValue(value);
        str >> valueF;
        it.shininess = valueF;
        str >> valueF;
        it.transparency = valueF;
    }
    setValues(values);
}

const char* PropertyMaterialList::getEditorName() const
{
    if(testStatus(NoMaterialListEdit))
        return "";
    return "Gui::PropertyEditor::PropertyMaterialListItem";
}

Property *PropertyMaterialList::Copy() const
{
    PropertyMaterialList *p = new PropertyMaterialList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyMaterialList::Paste(const Property &from)
{
    setValues(dynamic_cast<const PropertyMaterialList&>(from)._lValueList);
}

unsigned int PropertyMaterialList::getMemSize() const
{
    return static_cast<unsigned int>(_lValueList.size() * sizeof(Material));
}

//**************************************************************************
// PropertyPersistentObject
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyPersistentObject , App::PropertyString)

PyObject *PropertyPersistentObject::getPyObject(){
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

void PropertyPersistentObject::Restore(Base::DocumentReader &reader,XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *ContainerDOM)
{
	inherited::Restore(reader,ContainerDOM);
	auto element_persistent_obj_DOM = reader.FindElement(ELEMENT_PERSISTENT_OBJ);
	if(_pObject)
        _pObject->Restore(reader,element_persistent_obj_DOM);
}

Property *PropertyPersistentObject::Copy() const{
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

unsigned int PropertyPersistentObject::getMemSize () const{
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
