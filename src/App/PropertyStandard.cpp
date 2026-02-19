// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <algorithm>
#include <set>
#include <limits>
#include <memory>
#include <list>
#include <map>
#include <string>
#include <vector>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/math/special_functions/round.hpp>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Base/ProgramVersion.h>
#include <Base/Reader.h>
#include <Base/Writer.h>
#include <Base/Quantity.h>
#include <Base/Stream.h>
#include <Base/Tools.h>
#include <Base/PyWrapParseTupleAndKeywords.h>

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

TYPESYSTEM_SOURCE(App::PropertyInteger, App::Property)

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
    auto& self = propSetterSelf<App::PropertyInteger>(*this);

    self.aboutToSetValue();
    self._lValue = lValue;
    self.hasSetValue();
}

long PropertyInteger::getValue() const
{
    auto& self = propGetterSelf<const App::PropertyInteger>(*this);

    return self._lValue;
}

PyObject* PropertyInteger::getPyObject()
{
    auto& self = propGetterSelf<const App::PropertyInteger>(*this);

    return Py_BuildValue("l", self._lValue);
}

void PropertyInteger::setPyObject(PyObject* value)
{
    auto& self = propSetterSelf<App::PropertyInteger>(*this);

    if (PyLong_Check(value)) {
        self.aboutToSetValue();
        self._lValue = PyLong_AsLong(value);
        self.hasSetValue();
    }
    else {
        std::string error = std::string("type must be int, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyInteger::Save(Base::Writer& writer) const
{
    auto& self = propGetterSelf<const App::PropertyInteger>(*this);

    writer.Stream() << writer.ind() << "<Integer value=\"" << self._lValue << "\"/>" << std::endl;
}

void PropertyInteger::Restore(Base::XMLReader& reader)
{
    auto& self = propSetterSelf<App::PropertyInteger>(*this);

    // read my Element
    reader.readElement("Integer");
    // get the value of my Attribute
    self.setValue(reader.getAttribute<long>("value"));
}

Property* PropertyInteger::Copy() const
{
    auto& self = propGetterSelf<const App::PropertyInteger>(*this);

    PropertyInteger* p = new PropertyInteger();
    p->_lValue = self._lValue;
    return p;
}

void PropertyInteger::Paste(const Property& from)
{
    auto& self = propSetterSelf<App::PropertyInteger>(*this);

    self.aboutToSetValue();
    self._lValue = dynamic_cast<const PropertyInteger&>(from)._lValue;
    self.hasSetValue();
}

void PropertyInteger::setPathValue(const ObjectIdentifier& path, const boost::any& value)
{
    auto& self = propSetterSelf<App::PropertyInteger>(*this);

    self.verifyPath(path);

    if (value.type() == typeid(long)) {
        self.setValue(boost::any_cast<long>(value));
    }
    else if (value.type() == typeid(int)) {
        self.setValue(boost::any_cast<int>(value));
    }
    else if (value.type() == typeid(double)) {
        self.setValue(boost::math::round(boost::any_cast<double>(value)));
    }
    else if (value.type() == typeid(float)) {
        self.setValue(boost::math::round(boost::any_cast<float>(value)));
    }
    else if (value.type() == typeid(Quantity)) {
        self.setValue(boost::math::round(boost::any_cast<Quantity>(value).getValue()));
    }
    else {
        throw bad_cast();
    }
}


//**************************************************************************
//**************************************************************************
// PropertyPath
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyPath, App::Property)

//**************************************************************************
// Construction/Destruction

PropertyPath::PropertyPath() = default;

PropertyPath::~PropertyPath() = default;


//**************************************************************************
// Base class implementer


//**************************************************************************
// Setter/getter for the property

void PropertyPath::setValue(const std::filesystem::path& Path)
{
    auto& self = propSetterSelf<App::PropertyPath>(*this);

    self.aboutToSetValue();
    self._cValue = Path;
    hasSetValue();
}

void PropertyPath::setValue(const char* Path)
{
    auto& self = propSetterSelf<App::PropertyPath>(*this);

    self.aboutToSetValue();
    self._cValue = std::filesystem::path(Path);
    hasSetValue();
}

const std::filesystem::path& PropertyPath::getValue() const
{
    auto& self = propGetterSelf<const App::PropertyPath>(*this);

    return self._cValue;
}

PyObject* PropertyPath::getPyObject()
{
    auto& self = propGetterSelf<const App::PropertyPath>(*this);

#if (BOOST_FILESYSTEM_VERSION == 2)
    std::string str = self._cValue.native_file_string();
#else
    std::string str = self._cValue.string();
#endif

    // Returns a new reference, don't increment it!
    PyObject* p = PyUnicode_DecodeUTF8(str.c_str(), str.size(), nullptr);
    if (!p) {
        throw Base::UnicodeError("UTF8 conversion failure at PropertyPath::getPyObject()");
    }
    return p;
}

void PropertyPath::setPyObject(PyObject* value)
{
    auto& self = propSetterSelf<App::PropertyPath>(*this);

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
    self.setValue(path.c_str());
}


void PropertyPath::Save(Base::Writer& writer) const
{
    auto& self = propGetterSelf<const App::PropertyPath>(*this);

    std::string val = encodeAttribute(self._cValue.string());
    writer.Stream() << writer.ind() << "<Path value=\"" << val << "\"/>" << std::endl;
}

void PropertyPath::Restore(Base::XMLReader& reader)
{
    auto& self = propSetterSelf<App::PropertyPath>(*this);

    // read my Element
    reader.readElement("Path");
    // get the value of my Attribute
    self.setValue(reader.getAttribute<const char*>("value"));
}

Property* PropertyPath::Copy() const
{
    auto& self = propGetterSelf<const App::PropertyPath>(*this);

    PropertyPath* p = new PropertyPath();
    p->_cValue = self._cValue;
    return p;
}

void PropertyPath::Paste(const Property& from)
{
    auto& self = propSetterSelf<App::PropertyPath>(*this);

    self.aboutToSetValue();
    self._cValue = dynamic_cast<const PropertyPath&>(from)._cValue;
    hasSetValue();
}

unsigned int PropertyPath::getMemSize() const
{
    auto& self = propGetterSelf<const App::PropertyPath>(*this);

    return static_cast<unsigned int>(self._cValue.string().size());
}

//**************************************************************************
//**************************************************************************
// PropertyEnumeration
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyEnumeration, App::Property)

//**************************************************************************
// Construction/Destruction


PropertyEnumeration::PropertyEnumeration()
{
    _editorTypeName = "Gui::PropertyEditor::PropertyEnumItem";
}

PropertyEnumeration::PropertyEnumeration(const App::Enumeration& e)
{
    _enum = e;
}

PropertyEnumeration::~PropertyEnumeration() = default;

void PropertyEnumeration::setEnums(const char** plEnums)
{
    auto& self = propSetterSelf<App::PropertyEnumeration>(*this);

    // For backward compatibility, if the property container is not attached to
    // any document (i.e. its full name starts with '?'), do not notify, or
    // else existing code may crash.
    bool notify = !boost::starts_with(self.getFullName(), "?");
    if (notify) {
        self.aboutToSetValue();
    }
    self._enum.setEnums(plEnums);
    if (notify) {
        self.hasSetValue();
    }
}

void PropertyEnumeration::setEnums(const std::vector<std::string>& Enums)
{
    auto& self = propSetterSelf<App::PropertyEnumeration>(*this);

    self.setEnumVector(Enums);
}

void PropertyEnumeration::setValue(const char* value)
{
    auto& self = propSetterSelf<App::PropertyEnumeration>(*this);

    self.aboutToSetValue();
    self._enum.setValue(value);
    self.hasSetValue();
}

void PropertyEnumeration::setValue(long value)
{
    auto& self = propSetterSelf<App::PropertyEnumeration>(*this);

    self.aboutToSetValue();
    self._enum.setValue(value);
    self.hasSetValue();
}

void PropertyEnumeration::setValue(const Enumeration& source)
{
    auto& self = propSetterSelf<App::PropertyEnumeration>(*this);

    self.aboutToSetValue();
    self._enum = source;
    hasSetValue();
}

long PropertyEnumeration::getValue() const
{
    auto& self = propGetterSelf<const App::PropertyEnumeration>(*this);

    return self._enum.getInt();
}

bool PropertyEnumeration::isValue(const char* value) const
{
    auto& self = propGetterSelf<const App::PropertyEnumeration>(*this);

    return self._enum.isValue(value);
}

bool PropertyEnumeration::isPartOf(const char* value) const
{
    auto& self = propGetterSelf<const App::PropertyEnumeration>(*this);

    return self._enum.contains(value);
}

const char* PropertyEnumeration::getValueAsString() const
{
    auto& self = propGetterSelf<const App::PropertyEnumeration>(*this);

    if (!self._enum.isValid()) {
        throw Base::RuntimeError("Cannot get value from invalid enumeration");
    }
    return self._enum.getCStr();
}

const Enumeration& PropertyEnumeration::getEnum() const
{
    auto& self = propGetterSelf<const App::PropertyEnumeration>(*this);

    return self._enum;
}

std::vector<std::string> PropertyEnumeration::getEnumVector() const
{
    auto& self = propGetterSelf<const App::PropertyEnumeration>(*this);

    return self._enum.getEnumVector();
}

void PropertyEnumeration::setEnumVector(const std::vector<std::string>& values)
{
    auto& self = propSetterSelf<App::PropertyEnumeration>(*this);

    // For backward compatibility, if the property container is not attached to
    // any document (i.e. its full name starts with '?'), do not notify, or
    // else existing code may crash.
    bool notify = !boost::starts_with(self.getFullName(), "?");
    if (notify) {
        self.aboutToSetValue();
    }
    self._enum.setEnums(values);
    if (notify) {
        self.hasSetValue();
    }
}

bool PropertyEnumeration::hasEnums() const
{
    auto& self = propGetterSelf<const App::PropertyEnumeration>(*this);

    return self._enum.hasEnums();
}

bool PropertyEnumeration::isValid() const
{
    auto& self = propGetterSelf<const App::PropertyEnumeration>(*this);

    return self._enum.isValid();
}

void PropertyEnumeration::Save(Base::Writer& writer) const
{
    auto& self = propGetterSelf<const App::PropertyEnumeration>(*this);

    writer.Stream() << writer.ind() << "<Integer value=\"" << self._enum.getInt() << "\"";
    if (self._enum.isCustom()) {
        writer.Stream() << " CustomEnum=\"true\"";
    }
    writer.Stream() << "/>" << std::endl;
    if (self._enum.isCustom()) {
        std::vector<std::string> items = self.getEnumVector();
        writer.Stream() << writer.ind() << "<CustomEnumList count=\"" << items.size() << "\">"
                        << endl;
        writer.incInd();
        for (auto& item : items) {
            std::string val = encodeAttribute(item);
            writer.Stream() << writer.ind() << "<Enum value=\"" << val << "\"/>" << endl;
        }
        writer.decInd();
        writer.Stream() << writer.ind() << "</CustomEnumList>" << endl;
    }
}

void PropertyEnumeration::Restore(Base::XMLReader& reader)
{
    auto& self = propSetterSelf<App::PropertyEnumeration>(*this);

    // read my Element
    reader.readElement("Integer");
    // get the value of my Attribute
    long val = reader.getAttribute<long>("value");

    self.aboutToSetValue();

    if (reader.hasAttribute("CustomEnum")) {
        reader.readElement("CustomEnumList");
        int count = reader.getAttribute<long>("count");
        std::vector<std::string> values(count);

        for (int i = 0; i < count; i++) {
            reader.readElement("Enum");
            values[i] = reader.getAttribute<const char*>("value");
        }

        reader.readEndElement("CustomEnumList");

        self._enum.setEnums(values);
    }

    if (val < 0) {
        // If the enum is empty at this stage do not print a warning
        if (self._enum.hasEnums()) {
            Base::Console().developerWarning(std::string("PropertyEnumeration"),
                                             "Enumeration index %d is out of range, ignore it\n",
                                             val);
        }
        val = self.getValue();
    }

    self._enum.setValue(val);
    self.hasSetValue();
}

PyObject* PropertyEnumeration::getPyObject()
{
    auto& self = propGetterSelf<const App::PropertyEnumeration>(*this);

    if (!self._enum.isValid()) {
        Py_Return;
    }

    return Py_BuildValue("s", self.getValueAsString());
}

void PropertyEnumeration::setPyObject(PyObject* value)
{
    auto& self = propSetterSelf<App::PropertyEnumeration>(*this);

    if (PyLong_Check(value)) {
        long val = PyLong_AsLong(value);
        if (self._enum.isValid()) {
            self.aboutToSetValue();
            self._enum.setValue(val, true);
            self.hasSetValue();
        }
        return;
    }
    else if (PyUnicode_Check(value)) {
        std::string str = PyUnicode_AsUTF8(value);
        if (self._enum.contains(str.c_str())) {
            self.aboutToSetValue();
            self._enum.setValue(str);
            self.hasSetValue();
        }
        else {
            FC_THROWM(Base::ValueError,
                      "'" << str << "' is not part of the enumeration in " << getFullName());
        }
        return;
    }
    else if (PySequence_Check(value)) {

        try {
            std::vector<std::string> values;

            int idx = -1;
            Py::Sequence seq(value);

            if (seq.size() == 2) {
                Py::Object v(seq[0].ptr());
                if (!v.isString() && v.isSequence()) {
                    idx = Py::Long(seq[1].ptr());
                    seq = v;
                }
            }

            values.resize(seq.size());

            for (int i = 0; i < seq.size(); ++i) {
                values[i] = Py::Object(seq[i].ptr()).as_string();
            }

            self.aboutToSetValue();
            self._enum.setEnums(values);
            if (idx >= 0) {
                self._enum.setValue(idx, true);
            }
            self.hasSetValue();
            return;
        }
        catch (Py::Exception&) {
            Base::PyException e;
            e.reportException();
        }
    }

    FC_THROWM(Base::TypeError,
              "PropertyEnumeration "
                  << getFullName()
                  << " expects type to be int, string, or list(string), or list(list, int)");
}

Property* PropertyEnumeration::Copy() const
{
    auto& self = propGetterSelf<const App::PropertyEnumeration>(*this);

    return new PropertyEnumeration(self._enum);
}

void PropertyEnumeration::Paste(const Property& from)
{
    auto& self = propSetterSelf<App::PropertyEnumeration>(*this);

    const PropertyEnumeration& prop = dynamic_cast<const PropertyEnumeration&>(from);
    self.setValue(prop._enum);
}

void PropertyEnumeration::setPathValue(const ObjectIdentifier&, const boost::any& value)
{
    auto& self = propSetterSelf<App::PropertyEnumeration>(*this);

    if (value.type() == typeid(int)) {
        self.setValue(boost::any_cast<int>(value));
    }
    else if (value.type() == typeid(long)) {
        self.setValue(boost::any_cast<long>(value));
    }
    else if (value.type() == typeid(double)) {
        self.setValue(boost::any_cast<double>(value));
    }
    else if (value.type() == typeid(float)) {
        self.setValue(boost::any_cast<float>(value));
    }
    else if (value.type() == typeid(short)) {
        self.setValue(boost::any_cast<short>(value));
    }
    else if (value.type() == typeid(std::string)) {
        self.setValue(boost::any_cast<std::string>(value).c_str());
    }
    else if (value.type() == typeid(char*)) {
        self.setValue(boost::any_cast<char*>(value));
    }
    else if (value.type() == typeid(const char*)) {
        self.setValue(boost::any_cast<const char*>(value));
    }
    else {
        Base::PyGILStateLocker lock;
        Py::Object pyValue = pyObjectFromAny(value);
        self.setPyObject(pyValue.ptr());
    }
}

bool PropertyEnumeration::setPyPathValue(const ObjectIdentifier&, const Py::Object& value)
{
    auto& self = propSetterSelf<App::PropertyEnumeration>(*this);

    self.setPyObject(value.ptr());
    return true;
}

const boost::any PropertyEnumeration::getPathValue(const ObjectIdentifier& path) const
{
    auto& self = propGetterSelf<const App::PropertyEnumeration>(*this);

    std::string p = path.getSubPathStr();
    if (p == ".Enum" || p == ".All") {
        Base::PyGILStateLocker lock;
        Py::Object res;
        self.getPyPathValue(path, res);
        return pyObjectToAny(res, false);
    }
    else if (p == ".String") {
        auto v = self.getValueAsString();
        return std::string(v ? v : "");
    }
    else {
        return self.getValue();
    }
}

bool PropertyEnumeration::getPyPathValue(const ObjectIdentifier& path, Py::Object& r) const
{
    auto& self = propGetterSelf<const App::PropertyEnumeration>(*this);

    std::string p = path.getSubPathStr();
    if (p == ".Enum" || p == ".All") {
        Base::PyGILStateLocker lock;
        auto maxEnumValue = self._enum.maxValue();
        if (maxEnumValue < 0) {
            return false;  // The enum is invalid
        }
        Py::Tuple res(maxEnumValue + 1);
        std::vector<std::string> enums = self._enum.getEnumVector();
        PropertyString tmp;
        for (int i = 0; i < int(enums.size()); ++i) {
            tmp.setValue(enums[i]);
            res.setItem(i, Py::asObject(tmp.getPyObject()));
        }
        if (p == ".Enum") {
            r = res;
        }
        else {
            Py::Tuple tuple(2);
            tuple.setItem(0, res);
            tuple.setItem(1, Py::Long(self.getValue()));
            r = tuple;
        }
    }
    else if (p == ".String") {
        auto v = self.getValueAsString();
        r = Py::String(v ? v : "");
    }
    else {
        r = Py::Long(self.getValue());
    }
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
    if (_ConstStruct && _ConstStruct->isDeletable()) {
        delete _ConstStruct;
    }
}

void PropertyIntegerConstraint::setConstraints(const Constraints* sConstrain)
{
    auto& self = propSetterSelf<App::PropertyIntegerConstraint>(*this);

    if (self._ConstStruct != sConstrain) {
        if (self._ConstStruct && self._ConstStruct->isDeletable()) {
            delete self._ConstStruct;
        }
    }

    self._ConstStruct = sConstrain;
}

const PropertyIntegerConstraint::Constraints* PropertyIntegerConstraint::getConstraints() const
{
    auto& self = propGetterSelf<const App::PropertyIntegerConstraint>(*this);

    return self._ConstStruct;
}

long PropertyIntegerConstraint::getMinimum() const
{
    auto& self = propGetterSelf<const App::PropertyIntegerConstraint>(*this);

    if (self._ConstStruct) {
        return self._ConstStruct->LowerBound;
    }
    // return the min of int, not long
    return std::numeric_limits<int>::lowest();
}

long PropertyIntegerConstraint::getMaximum() const
{
    auto& self = propGetterSelf<const App::PropertyIntegerConstraint>(*this);

    if (self._ConstStruct) {
        return self._ConstStruct->UpperBound;
    }
    // return the max of int, not long
    return std::numeric_limits<int>::max();
}

long PropertyIntegerConstraint::getStepSize() const
{
    auto& self = propGetterSelf<const App::PropertyIntegerConstraint>(*this);

    if (self._ConstStruct) {
        return self._ConstStruct->StepSize;
    }
    return 1;
}

void PropertyIntegerConstraint::setPyObject(PyObject* value)
{
    auto& self = propSetterSelf<App::PropertyIntegerConstraint>(*this);

    if (PyLong_Check(value)) {
        long temp = PyLong_AsLong(value);
        if (self._ConstStruct) {
            if (temp > self._ConstStruct->UpperBound) {
                temp = self._ConstStruct->UpperBound;
            }
            else if (temp < self._ConstStruct->LowerBound) {
                temp = self._ConstStruct->LowerBound;
            }
        }

        self.aboutToSetValue();
        self._lValue = temp;
        hasSetValue();
    }
    else {
        long valConstr[] = {0,
                            std::numeric_limits<int>::lowest(),
                            std::numeric_limits<int>::max(),
                            1};

        if (PyDict_Check(value)) {
            Py::Tuple dummy;
            static const std::array<const char*, 5> kw = {"value",
                                                          "min",
                                                          "max",
                                                          "step",
                                                          nullptr};

            if (!Base::Wrapped_ParseTupleAndKeywords(dummy.ptr(),
                                                     value,
                                                     "l|lll",
                                                      kw,
                                                     &(valConstr[0]),
                                                     &(valConstr[1]),
                                                     &(valConstr[2]),
                                                     &(valConstr[3]))) {
                throw Py::Exception();
            }
        }
        else if (PyTuple_Check(value)) {
            if (!PyArg_ParseTuple(value,
                                  "llll",
                                  &(valConstr[0]),
                                  &(valConstr[1]),
                                  &(valConstr[2]),
                                  &(valConstr[3]))) {
                throw Py::Exception();
            }
        }
        else {
            std::string error = std::string("type must be int, dict or tuple, not ");
            error += value->ob_type->tp_name;
            throw Base::TypeError(error);
        }

        Constraints* c = new Constraints();
        c->setDeletable(true);
        c->LowerBound = valConstr[1];
        c->UpperBound = valConstr[2];
        c->StepSize = std::max<long>(1, valConstr[3]);
        if (valConstr[0] > c->UpperBound) {
            valConstr[0] = c->UpperBound;
        }
        else if (valConstr[0] < c->LowerBound) {
            valConstr[0] = c->LowerBound;
        }
        setConstraints(c);

        aboutToSetValue();
        self._lValue = valConstr[0];
        hasSetValue();
    }
}

//**************************************************************************
//**************************************************************************
// PropertyPercent
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyPercent, App::PropertyIntegerConstraint)

const PropertyIntegerConstraint::Constraints percent = {0, 100, 1};

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

TYPESYSTEM_SOURCE(App::PropertyIntegerList, App::PropertyLists)

//**************************************************************************
// Construction/Destruction


PropertyIntegerList::PropertyIntegerList() = default;

PropertyIntegerList::~PropertyIntegerList() = default;

//**************************************************************************
// Base class implementer

PyObject* PropertyIntegerList::getPyObject()
{
    auto& self = propGetterSelf<const App::PropertyIntegerList>(*this);

    PyObject* list = PyList_New(self.getSize());
    for (int i = 0; i < self.getSize(); i++) {
        PyList_SetItem(list, i, PyLong_FromLong(self._lValueList[i]));
    }
    return list;
}

long PropertyIntegerList::getPyValue(PyObject* item) const
{
    if (PyLong_Check(item)) {
        return PyLong_AsLong(item);
    }
    std::string error = std::string("type in list must be int, not ");
    error += item->ob_type->tp_name;
    throw Base::TypeError(error);
}

void PropertyIntegerList::Save(Base::Writer& writer) const
{
    auto& self = propGetterSelf<const App::PropertyIntegerList>(*this);

    writer.Stream() << writer.ind() << "<IntegerList count=\"" << self.getSize() << "\">" << endl;
    writer.incInd();
    for (int i = 0; i < self.getSize(); i++) {
        writer.Stream() << writer.ind() << "<I v=\"" << self._lValueList[i] << "\"/>" << endl;
    };
    writer.decInd();
    writer.Stream() << writer.ind() << "</IntegerList>" << endl;
}

void PropertyIntegerList::Restore(Base::XMLReader& reader)
{
    auto& self = propSetterSelf<App::PropertyIntegerList>(*this);

    // read my Element
    reader.readElement("IntegerList");
    // get the value of my Attribute
    int count = reader.getAttribute<long>("count");

    std::vector<long> values(count);
    for (int i = 0; i < count; i++) {
        reader.readElement("I");
        values[i] = reader.getAttribute<long>("v");
    }

    reader.readEndElement("IntegerList");

    // assignment
    self.setValues(values);
}

Property* PropertyIntegerList::Copy() const
{
    auto& self = propGetterSelf<const App::PropertyIntegerList>(*this);

    PropertyIntegerList* p = new PropertyIntegerList();
    p->_lValueList = self._lValueList;
    return p;
}

void PropertyIntegerList::Paste(const Property& from)
{
    auto& self = propSetterSelf<App::PropertyIntegerList>(*this);

    self.setValues(dynamic_cast<const PropertyIntegerList&>(from)._lValueList);
}

unsigned int PropertyIntegerList::getMemSize() const
{
    auto& self = propGetterSelf<const App::PropertyIntegerList>(*this);

    return static_cast<unsigned int>(self._lValueList.size() * sizeof(long));
}


//**************************************************************************
//**************************************************************************
// PropertyIntegerSet
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyIntegerSet, App::Property)

//**************************************************************************
// Construction/Destruction


PropertyIntegerSet::PropertyIntegerSet() = default;

PropertyIntegerSet::~PropertyIntegerSet() = default;


//**************************************************************************
// Base class implementer

void PropertyIntegerSet::setValue(long lValue)
{
    auto& self = propSetterSelf<App::PropertyIntegerSet>(*this);

    self.aboutToSetValue();
    self._lValueSet.clear();
    self._lValueSet.insert(lValue);
    self.hasSetValue();
}

void PropertyIntegerSet::setValues(const std::set<long>& values)
{
    auto& self = propSetterSelf<App::PropertyIntegerSet>(*this);

    self.aboutToSetValue();
    self._lValueSet = values;
    hasSetValue();
}

PyObject* PropertyIntegerSet::getPyObject()
{
    auto& self = propGetterSelf<const App::PropertyIntegerSet>(*this);

    PyObject* set = PySet_New(nullptr);
    for (long it : self._lValueSet) {
        PySet_Add(set, PyLong_FromLong(it));
    }
    return set;
}

void PropertyIntegerSet::setPyObject(PyObject* value)
{
    auto& self = propSetterSelf<App::PropertyIntegerSet>(*this);

    if (PySequence_Check(value)) {

        Py::Sequence sequence(value);
        Py_ssize_t nSize = sequence.size();
        std::set<long> values;

        for (Py_ssize_t i = 0; i < nSize; ++i) {
            Py::Object item = sequence.getItem(i);
            if (!PyLong_Check(item.ptr())) {
                std::string error = std::string("type in list must be int, not ");
                error += item.ptr()->ob_type->tp_name;
                throw Base::TypeError(error);
            }
            values.insert(PyLong_AsLong(item.ptr()));
        }

        self.setValues(values);
    }
    else if (PyLong_Check(value)) {
        self.setValue(PyLong_AsLong(value));
    }
    else {
        std::string error = std::string("type must be int or list of int, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyIntegerSet::Save(Base::Writer& writer) const
{
    auto& self = propGetterSelf<const App::PropertyIntegerSet>(*this);

    writer.Stream() << writer.ind() << "<IntegerSet count=\"" << self._lValueSet.size() << "\">" << endl;
    writer.incInd();
    for (long it : self._lValueSet) {
        writer.Stream() << writer.ind() << "<I v=\"" << it << "\"/>" << endl;
    };
    writer.decInd();
    writer.Stream() << writer.ind() << "</IntegerSet>" << endl;
}

void PropertyIntegerSet::Restore(Base::XMLReader& reader)
{
    auto& self = propSetterSelf<App::PropertyIntegerSet>(*this);

    // read my Element
    reader.readElement("IntegerSet");
    // get the value of my Attribute
    int count = reader.getAttribute<long>("count");

    std::set<long> values;
    for (int i = 0; i < count; i++) {
        reader.readElement("I");
        values.insert(reader.getAttribute<long>("v"));
    }

    reader.readEndElement("IntegerSet");

    // assignment
    self.setValues(values);
}

Property* PropertyIntegerSet::Copy() const
{
    auto& self = propGetterSelf<const App::PropertyIntegerSet>(*this);

    PropertyIntegerSet* p = new PropertyIntegerSet();
    p->_lValueSet = self._lValueSet;
    return p;
}

void PropertyIntegerSet::Paste(const Property& from)
{
    auto& self = propSetterSelf<App::PropertyIntegerSet>(*this);

    self.aboutToSetValue();
    self._lValueSet = dynamic_cast<const PropertyIntegerSet&>(from)._lValueSet;
    hasSetValue();
}

unsigned int PropertyIntegerSet::getMemSize() const
{
    auto& self = propGetterSelf<const App::PropertyIntegerSet>(*this);

    return static_cast<unsigned int>(self._lValueSet.size() * sizeof(long));
}


//**************************************************************************
//**************************************************************************
// PropertyFloat
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyFloat, App::Property)

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
    auto& self = propSetterSelf<App::PropertyFloat>(*this);

    self.aboutToSetValue();
    self._dValue = lValue;
    hasSetValue();
}

double PropertyFloat::getValue() const
{
    auto& self = propGetterSelf<const App::PropertyFloat>(*this);

    return self._dValue;
}

PyObject* PropertyFloat::getPyObject()
{
    auto& self = propGetterSelf<const App::PropertyFloat>(*this);

    return Py_BuildValue("d", self._dValue);
}

void PropertyFloat::setPyObject(PyObject* value)
{
    auto& self = propSetterSelf<App::PropertyFloat>(*this);

    if (PyFloat_Check(value)) {
        self.aboutToSetValue();
        self._dValue = PyFloat_AsDouble(value);
        hasSetValue();
    }
    else if (PyLong_Check(value)) {
        aboutToSetValue();
        self._dValue = PyLong_AsLong(value);
        hasSetValue();
    }
    else {
        std::string error = std::string("type must be float or int, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyFloat::Save(Base::Writer& writer) const
{
    auto& self = propGetterSelf<const App::PropertyFloat>(*this);

    writer.Stream() << writer.ind() << "<Float value=\"" << self._dValue << "\"/>" << std::endl;
}

void PropertyFloat::Restore(Base::XMLReader& reader)
{
    auto& self = propSetterSelf<App::PropertyFloat>(*this);

    // read my Element
    reader.readElement("Float");
    // get the value of my Attribute
    self.setValue(reader.getAttribute<double>("value"));
}

Property* PropertyFloat::Copy() const
{
    auto& self = propGetterSelf<const App::PropertyFloat>(*this);

    PropertyFloat* p = new PropertyFloat();
    p->_dValue = self._dValue;
    return p;
}

void PropertyFloat::Paste(const Property& from)
{
    auto& self = propSetterSelf<App::PropertyFloat>(*this);

    self.aboutToSetValue();
    self._dValue = dynamic_cast<const PropertyFloat&>(from)._dValue;
    hasSetValue();
}

void PropertyFloat::setPathValue(const ObjectIdentifier& path, const boost::any& value)
{
    auto& self = propSetterSelf<App::PropertyFloat>(*this);

    self.verifyPath(path);

    if (value.type() == typeid(long)) {
        self.setValue(boost::any_cast<long>(value));
    }
    else if (value.type() == typeid(unsigned long)) {
        self.setValue(boost::any_cast<unsigned long>(value));
    }
    else if (value.type() == typeid(int)) {
        self.setValue(boost::any_cast<int>(value));
    }
    else if (value.type() == typeid(double)) {
        self.setValue(boost::any_cast<double>(value));
    }
    else if (value.type() == typeid(float)) {
        self.setValue(boost::any_cast<float>(value));
    }
    else if (value.type() == typeid(Quantity)) {
        self.setValue((boost::any_cast<Quantity>(value)).getValue());
    }
    else {
        throw bad_cast();
    }
}

const boost::any PropertyFloat::getPathValue(const ObjectIdentifier& path) const
{
    auto& self = propGetterSelf<const App::PropertyFloat>(*this);

    self.verifyPath(path);
    return self._dValue;
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
    if (_ConstStruct && _ConstStruct->isDeletable()) {
        delete _ConstStruct;
    }
}

void PropertyFloatConstraint::setConstraints(const Constraints* sConstrain)
{
    auto& self = propSetterSelf<App::PropertyFloatConstraint>(*this);

    if (self._ConstStruct != sConstrain) {
        if (self._ConstStruct && self._ConstStruct->isDeletable()) {
            delete self._ConstStruct;
        }
    }
    self._ConstStruct = sConstrain;
}

const PropertyFloatConstraint::Constraints* PropertyFloatConstraint::getConstraints() const
{
    auto& self = propGetterSelf<const App::PropertyFloatConstraint>(*this);

    return self._ConstStruct;
}

double PropertyFloatConstraint::getMinimum() const
{
    auto& self = propGetterSelf<const App::PropertyFloatConstraint>(*this);

    if (self._ConstStruct) {
        return self._ConstStruct->LowerBound;
    }
    return std::numeric_limits<double>::lowest();
}

double PropertyFloatConstraint::getMaximum() const
{
    auto& self = propGetterSelf<const App::PropertyFloatConstraint>(*this);

    if (self._ConstStruct) {
        return self._ConstStruct->UpperBound;
    }
    return std::numeric_limits<double>::max();
}

double PropertyFloatConstraint::getStepSize() const
{
    auto& self = propGetterSelf<const App::PropertyFloatConstraint>(*this);

    if (self._ConstStruct) {
        return self._ConstStruct->StepSize;
    }
    return 1.0;
}

void PropertyFloatConstraint::setPyObject(PyObject* value)
{
    auto& self = propSetterSelf<App::PropertyFloatConstraint>(*this);

    if (PyFloat_Check(value)) {
        double temp = PyFloat_AsDouble(value);
        if (self._ConstStruct) {
            if (temp > self._ConstStruct->UpperBound) {
                temp = self._ConstStruct->UpperBound;
            }
            else if (temp < self._ConstStruct->LowerBound) {
                temp = self._ConstStruct->LowerBound;
            }
        }

        self.aboutToSetValue();
        self._dValue = temp;
        hasSetValue();
    }
    else if (PyLong_Check(value)) {
        double temp = static_cast<double>(PyLong_AsLong(value));
        if (self._ConstStruct) {
            if (temp > self._ConstStruct->UpperBound) {
                temp = self._ConstStruct->UpperBound;
            }
            else if (temp < self._ConstStruct->LowerBound) {
                temp = self._ConstStruct->LowerBound;
            }
        }

        aboutToSetValue();
        self._dValue = temp;
        hasSetValue();
    }
    else {
        double valConstr[] = {0.0,
                              std::numeric_limits<double>::lowest(),
                              std::numeric_limits<double>::max(),
                              1.0};

        if (PyDict_Check(value)) {
            Py::Tuple dummy;
            static const std::array<const char*, 5> kw = {"value",
                                                          "min",
                                                          "max",
                                                          "step",
                                                           nullptr};

            if (!Base::Wrapped_ParseTupleAndKeywords(dummy.ptr(),
                                                     value,
                                                     "d|ddd",
                                                     kw,
                                                     &(valConstr[0]),
                                                     &(valConstr[1]),
                                                     &(valConstr[2]),
                                                     &(valConstr[3]))) {
                throw Py::Exception();
            }
        }
        else if (PyTuple_Check(value)) {
            if (!PyArg_ParseTuple(value,
                                  "dddd",
                                  &(valConstr[0]),
                                  &(valConstr[1]),
                                  &(valConstr[2]),
                                  &(valConstr[3]))) {
                throw Py::Exception();
            }
        }
        else {
            std::string error = std::string("type must be float, dict or tuple, not ");
            error += value->ob_type->tp_name;
            throw Base::TypeError(error);
        }

        double stepSize = valConstr[3];
        // need a value > 0
        if (stepSize < std::numeric_limits<double>::epsilon()) {
            throw Base::ValueError("Step size must be greater than zero");
        }

        Constraints* c = new Constraints();
        c->setDeletable(true);
        c->LowerBound = valConstr[1];
        c->UpperBound = valConstr[2];
        c->StepSize = stepSize;
        if (valConstr[0] > c->UpperBound) {
            valConstr[0] = c->UpperBound;
        }
        else if (valConstr[0] < c->LowerBound) {
            valConstr[0] = c->LowerBound;
        }
        setConstraints(c);

        aboutToSetValue();
        self._dValue = valConstr[0];
        hasSetValue();
    }
}

//**************************************************************************
// PropertyPrecision
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyPrecision, App::PropertyFloatConstraint)

//**************************************************************************
// Construction/Destruction
//
const PropertyFloatConstraint::Constraints PrecisionStandard = {
    0.0, std::numeric_limits<double>::max(), 0.001};

PropertyPrecision::PropertyPrecision()
{
    setConstraints(&PrecisionStandard);
}

PropertyPrecision::~PropertyPrecision() = default;


//**************************************************************************
// PropertyFloatList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyFloatList, App::PropertyLists)

//**************************************************************************
// Construction/Destruction


PropertyFloatList::PropertyFloatList() = default;

PropertyFloatList::~PropertyFloatList() = default;

//**************************************************************************
// Base class implementer

PyObject* PropertyFloatList::getPyObject()
{
    auto& self = propGetterSelf<const App::PropertyFloatList>(*this);

    PyObject* list = PyList_New(self.getSize());
    for (int i = 0; i < self.getSize(); i++) {
        PyList_SetItem(list, i, PyFloat_FromDouble(self._lValueList[i]));
    }
    return list;
}

double PropertyFloatList::getPyValue(PyObject* item) const
{
    if (PyFloat_Check(item)) {
        return PyFloat_AsDouble(item);
    }
    else if (PyLong_Check(item)) {
        return static_cast<double>(PyLong_AsLong(item));
    }
    else {
        std::string error = std::string("type in list must be float, not ");
        error += item->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyFloatList::Save(Base::Writer& writer) const
{
    auto& self = propGetterSelf<const App::PropertyFloatList>(*this);

    if (writer.isForceXML()) {
        writer.Stream() << writer.ind() << "<FloatList count=\"" << self.getSize() << "\">" << endl;
        writer.incInd();
        for (int i = 0; i < self.getSize(); i++) {
            writer.Stream() << writer.ind() << "<F v=\"" << self._lValueList[i] << "\"/>" << endl;
        };
        writer.decInd();
        writer.Stream() << writer.ind() << "</FloatList>" << endl;
    }
    else {
        writer.Stream() << writer.ind() << "<FloatList file=\""
                        << (self.getSize() ? writer.addFile(self.getName(), &self) : "") << "\"/>"
                        << std::endl;
    }
}

void PropertyFloatList::Restore(Base::XMLReader& reader)
{
    auto& self = propGetterSelf<const App::PropertyFloatList>(*this);

    reader.readElement("FloatList");
    string file(reader.getAttribute<const char*>("file"));

    if (!file.empty()) {
        // initiate a file read
        reader.addFile(file.c_str(), &self);
    }
}

void PropertyFloatList::SaveDocFile(Base::Writer& writer) const
{
    auto& self = propGetterSelf<const App::PropertyFloatList>(*this);

    Base::OutputStream str(writer.Stream());
    uint32_t uCt = (uint32_t)self.getSize();
    str << uCt;
    if (!self.isSinglePrecision()) {
        for (double it : self._lValueList) {
            str << it;
        }
    }
    else {
        for (double it : self._lValueList) {
            float v = static_cast<float>(it);
            str << v;
        }
    }
}

void PropertyFloatList::RestoreDocFile(Base::Reader& reader)
{
    auto& self = propSetterSelf<App::PropertyFloatList>(*this);

    Base::InputStream str(reader);
    uint32_t uCt = 0;
    str >> uCt;
    std::vector<double> values(uCt);
    if (!self.isSinglePrecision()) {
        for (double& it : values) {
            str >> it;
        }
    }
    else {
        for (double& it : values) {
            float val;
            str >> val;
            it = val;
        }
    }
    self.setValues(values);
}

Property* PropertyFloatList::Copy() const
{
    auto& self = propGetterSelf<const App::PropertyFloatList>(*this);

    PropertyFloatList* p = new PropertyFloatList();
    p->_lValueList = self._lValueList;
    return p;
}

void PropertyFloatList::Paste(const Property& from)
{
    auto& self = propSetterSelf<App::PropertyFloatList>(*this);

    self.setValues(dynamic_cast<const PropertyFloatList&>(from)._lValueList);
}

unsigned int PropertyFloatList::getMemSize() const
{
    auto& self = propGetterSelf<const App::PropertyFloatList>(*this);

    return static_cast<unsigned int>(self._lValueList.size() * sizeof(double));
}

//**************************************************************************
//**************************************************************************
// PropertyString
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyString, App::Property)

PropertyString::PropertyString() = default;

PropertyString::~PropertyString() = default;

void PropertyString::setValue(const char* newValue)
{
    auto& self = propSetterSelf<App::PropertyString>(*this);

    if (!newValue) {
        return;
    }

    if (self._cValue == newValue) {
        return;
    }

    std::vector<std::pair<Property*, std::unique_ptr<Property>>> propChanges;
    std::string newValueStr = newValue;
    auto obj = freecad_cast<DocumentObject*>(self.getContainer());
    bool commit = false;

    if (obj && &self == &obj->Label) {
        propChanges = obj->onProposedLabelChange(newValueStr);
        if (self._cValue == newValueStr) {
            // OnProposedLabelChange has changed the new value to what the current value is
            return;
        }
        if (!propChanges.empty() && !GetApplication().getActiveTransaction()) {
            commit = true;
            std::ostringstream str;
            str << "Change " << obj->getNameInDocument() << ".Label";
            GetApplication().setActiveTransaction(str.str().c_str());
        }
    }

    self.aboutToSetValue();
    self._cValue = newValueStr;
    hasSetValue();

    for (auto& change : propChanges) {
        change.first->Paste(*change.second);
    }

    if (commit) {
        GetApplication().closeActiveTransaction();
    }
}

void PropertyString::setValue(const std::string& sString)
{
    auto& self = propSetterSelf<App::PropertyString>(*this);

    self.setValue(sString.c_str());
}

const char* PropertyString::getValue() const
{
    auto& self = propGetterSelf<const App::PropertyString>(*this);

    return self._cValue.c_str();
}

PyObject* PropertyString::getPyObject()
{
    auto& self = propGetterSelf<const App::PropertyString>(*this);

    PyObject* p = PyUnicode_DecodeUTF8(self._cValue.c_str(), self._cValue.size(), nullptr);
    if (!p) {
        throw Base::UnicodeError("UTF8 conversion failure at PropertyString::getPyObject()");
    }
    return p;
}

void PropertyString::setPyObject(PyObject* value)
{
    auto& self = propSetterSelf<App::PropertyString>(*this);

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
    self.setValue(string);
}

void PropertyString::Save(Base::Writer& writer) const
{
    auto& self = propGetterSelf<const App::PropertyString>(*this);

    auto verifyXMLString = [&self](std::string& input) {
        const std::string output = this->validateXMLString(input);
        if (output != input) {
            Base::Console().warning("XML output: Validate invalid string:\n'%s'\n'%s'\n",
                                    input, output);
        }
        return output;
    };
    std::string val;
    auto obj = freecad_cast<DocumentObject*>(self.getContainer());
    writer.Stream() << writer.ind() << "<String ";
    bool exported = false;
    if (obj && obj->isAttachedToDocument() && obj->isExporting() && &obj->Label == &self) {
        if (obj->allowDuplicateLabel()) {
            writer.Stream() << "restore=\"1\" ";
        }
        else if (self._cValue == obj->getNameInDocument()) {
            writer.Stream() << "restore=\"0\" ";
            val = encodeAttribute(obj->getExportName());
            val = verifyXMLString(val);
            exported = true;
        }
    }
    if (!exported) {
        val = encodeAttribute(self._cValue);
        val = verifyXMLString(val);
    }
    writer.Stream() << "value=\"" << val << "\"/>" << std::endl;
}

void PropertyString::Restore(Base::XMLReader& reader)
{
    auto& self = propSetterSelf<App::PropertyString>(*this);

    // read my Element
    reader.readElement("String");
    // get the value of my Attribute
    auto obj = freecad_cast<DocumentObject*>(self.getContainer());
    if (obj && &obj->Label == &self) {
        if (reader.hasAttribute("restore")) {
            int restore = reader.getAttribute<long>("restore");
            if (restore == 1) {
                self.aboutToSetValue();
                self._cValue = reader.getAttribute<const char*>("value");
                hasSetValue();
            }
            else {
                setValue(reader.getName(reader.getAttribute<const char*>("value")));
            }
        }
        else {
            setValue(reader.getAttribute<const char*>("value"));
        }
    }
    else {
        setValue(reader.getAttribute<const char*>("value"));
    }
}

Property* PropertyString::Copy() const
{
    auto& self = propGetterSelf<const App::PropertyString>(*this);

    PropertyString* p = new PropertyString();
    p->_cValue = self._cValue;
    return p;
}

void PropertyString::Paste(const Property& from)
{
    auto& self = propSetterSelf<App::PropertyString>(*this);

    self.setValue(dynamic_cast<const PropertyString&>(from)._cValue);
}

unsigned int PropertyString::getMemSize() const
{
    auto& self = propGetterSelf<const App::PropertyString>(*this);

    return static_cast<unsigned int>(self._cValue.size());
}

void PropertyString::setPathValue(const ObjectIdentifier& path, const boost::any& value)
{
    auto& self = propSetterSelf<App::PropertyString>(*this);

    self.verifyPath(path);
    if (value.type() == typeid(bool)) {
        self.setValue(boost::any_cast<bool>(value) ? "True" : "False");
    }
    else if (value.type() == typeid(int)) {
        self.setValue(std::to_string(boost::any_cast<int>(value)));
    }
    else if (value.type() == typeid(long)) {
        self.setValue(std::to_string(boost::any_cast<long>(value)));
    }
    else if (value.type() == typeid(double)) {
        self.setValue(std::to_string(App::any_cast<double>(value)));
    }
    else if (value.type() == typeid(float)) {
        self.setValue(std::to_string(App::any_cast<float>(value)));
    }
    else if (value.type() == typeid(Quantity)) {
        self.setValue(boost::any_cast<Quantity>(value).getUserString().c_str());
    }
    else if (value.type() == typeid(std::string)) {
        self.setValue(boost::any_cast<const std::string &>(value));
    }
    else {
        Base::PyGILStateLocker lock;
        self.setValue(pyObjectFromAny(value).as_string());
    }
}

const boost::any PropertyString::getPathValue(const ObjectIdentifier& path) const
{
    auto& self = propGetterSelf<const App::PropertyString>(*this);

    self.verifyPath(path);
    return self._cValue;
}

//**************************************************************************
//**************************************************************************
// PropertyUUID
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyUUID, App::Property)

PropertyUUID::PropertyUUID() = default;

PropertyUUID::~PropertyUUID() = default;

void PropertyUUID::setValue(const Base::Uuid& id)
{
    auto& self = propSetterSelf<App::PropertyUUID>(*this);

    self.aboutToSetValue();
    self._uuid = id;
    hasSetValue();
}

void PropertyUUID::setValue(const char* sString)
{
    auto& self = propSetterSelf<App::PropertyUUID>(*this);

    if (sString) {
        self.aboutToSetValue();
        self._uuid.setValue(sString);
        self.hasSetValue();
    }
}

void PropertyUUID::setValue(const std::string& sString)
{
    auto& self = propSetterSelf<App::PropertyUUID>(*this);

    self.aboutToSetValue();
    self._uuid.setValue(sString);
    self.hasSetValue();
}

const std::string& PropertyUUID::getValueStr() const
{
    auto& self = propGetterSelf<const App::PropertyUUID>(*this);

    return self._uuid.getValue();
}

const Base::Uuid& PropertyUUID::getValue() const
{
    auto& self = propGetterSelf<const App::PropertyUUID>(*this);

    return self._uuid;
}

PyObject* PropertyUUID::getPyObject()
{
    auto& self = propGetterSelf<const App::PropertyUUID>(*this);

    PyObject* p = PyUnicode_FromString(self._uuid.getValue().c_str());
    return p;
}

void PropertyUUID::setPyObject(PyObject* value)
{
    auto& self = propSetterSelf<App::PropertyUUID>(*this);

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
        self.setValue(uid);
    }
    catch (const std::exception& e) {
        throw Base::RuntimeError(e.what());
    }
}

void PropertyUUID::Save(Base::Writer& writer) const
{
    auto& self = propGetterSelf<const App::PropertyUUID>(*this);

    writer.Stream() << writer.ind() << "<Uuid value=\"" << self._uuid.getValue() << "\"/>" << std::endl;
}

void PropertyUUID::Restore(Base::XMLReader& reader)
{
    auto& self = propSetterSelf<App::PropertyUUID>(*this);

    // read my Element
    reader.readElement("Uuid");
    // get the value of my Attribute
    self.setValue(reader.getAttribute<const char*>("value"));
}

Property* PropertyUUID::Copy() const
{
    auto& self = propGetterSelf<const App::PropertyUUID>(*this);

    PropertyUUID* p = new PropertyUUID();
    p->_uuid = self._uuid;
    return p;
}

void PropertyUUID::Paste(const Property& from)
{
    auto& self = propSetterSelf<App::PropertyUUID>(*this);

    self.aboutToSetValue();
    self._uuid = dynamic_cast<const PropertyUUID&>(from)._uuid;
    hasSetValue();
}

unsigned int PropertyUUID::getMemSize() const
{
    auto& self = propGetterSelf<const App::PropertyUUID>(*this);

    return static_cast<unsigned int>(sizeof(self._uuid));
}

//**************************************************************************
// PropertyFont
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyFont, App::PropertyString)

PropertyFont::PropertyFont() = default;

PropertyFont::~PropertyFont() = default;

//**************************************************************************
// PropertyStringList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyStringList, App::PropertyLists)

PropertyStringList::PropertyStringList() = default;

PropertyStringList::~PropertyStringList() = default;

//**************************************************************************
// Base class implementer

void PropertyStringList::setValues(const std::list<std::string>& lValue)
{
    auto& self = propSetterSelf<App::PropertyStringList>(*this);

    std::vector<std::string> vals;
    vals.reserve(lValue.size());
    for (const auto& v : lValue) {
        vals.push_back(v);
    }
    self.setValues(vals);
}

PyObject* PropertyStringList::getPyObject()
{
    auto& self = propGetterSelf<const App::PropertyStringList>(*this);

    PyObject* list = PyList_New(self.getSize());

    for (int i = 0; i < self.getSize(); i++) {
        PyObject* item =
            PyUnicode_DecodeUTF8(self._lValueList[i].c_str(), self._lValueList[i].size(), nullptr);
        if (!item) {
            Py_DECREF(list);
            throw Base::UnicodeError(
                "UTF8 conversion failure at PropertyStringList::getPyObject()");
        }
        PyList_SetItem(list, i, item);
    }

    return list;
}

std::string PropertyStringList::getPyValue(PyObject* item) const
{
    std::string ret;
    if (PyUnicode_Check(item)) {
        ret = PyUnicode_AsUTF8(item);
    }
    else if (PyBytes_Check(item)) {
        ret = PyBytes_AsString(item);
    }
    else {
        std::string error = std::string("type in list must be str or unicode, not ");
        error += item->ob_type->tp_name;
        throw Base::TypeError(error);
    }
    return ret;
}

unsigned int PropertyStringList::getMemSize() const
{
    auto& self = propGetterSelf<const App::PropertyStringList>(*this);

    size_t size = 0;
    for (int i = 0; i < self.getSize(); i++) {
        size += self._lValueList[i].size();
    }
    return static_cast<unsigned int>(size);
}

void PropertyStringList::Save(Base::Writer& writer) const
{
    auto& self = propGetterSelf<const App::PropertyStringList>(*this);

    writer.Stream() << writer.ind() << "<StringList count=\"" << self.getSize() << "\">" << endl;
    writer.incInd();
    for (int i = 0; i < self.getSize(); i++) {
        std::string val = encodeAttribute(self._lValueList[i]);
        writer.Stream() << writer.ind() << "<String value=\"" << val << "\"/>" << endl;
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</StringList>" << endl;
}

void PropertyStringList::Restore(Base::XMLReader& reader)
{
    auto& self = propSetterSelf<App::PropertyStringList>(*this);

    // read my Element
    reader.readElement("StringList");
    // get the value of my Attribute
    int count = reader.getAttribute<long>("count");

    std::vector<std::string> values(count);
    for (int i = 0; i < count; i++) {
        reader.readElement("String");
        values[i] = reader.getAttribute<const char*>("value");
    }

    reader.readEndElement("StringList");

    // assignment
    self.setValues(values);
}

Property* PropertyStringList::Copy() const
{
    auto& self = propGetterSelf<const App::PropertyStringList>(*this);

    PropertyStringList* p = new PropertyStringList();
    p->_lValueList = self._lValueList;
    return p;
}

void PropertyStringList::Paste(const Property& from)
{
    auto& self = propSetterSelf<App::PropertyStringList>(*this);

    self.setValues(dynamic_cast<const PropertyStringList&>(from)._lValueList);
}


//**************************************************************************
// PropertyMap
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyMap, App::Property)

PropertyMap::PropertyMap() = default;

PropertyMap::~PropertyMap() = default;

//**************************************************************************
// Base class implementer


int PropertyMap::getSize() const
{
    auto& self = propGetterSelf<const App::PropertyMap>(*this);

    return static_cast<int>(self._lValueList.size());
}

void PropertyMap::setValue(const std::string& key, const std::string& value)
{
    auto& self = propSetterSelf<App::PropertyMap>(*this);

    self.aboutToSetValue();
    self._lValueList[key] = value;
    self.hasSetValue();
}

void PropertyMap::setValues(const std::map<std::string, std::string>& map)
{
    auto& self = propSetterSelf<App::PropertyMap>(*this);

    self.aboutToSetValue();
    self._lValueList = map;
    hasSetValue();
}

const std::string& PropertyMap::operator[](const std::string& key) const
{
    auto& self = propGetterSelf<const App::PropertyMap>(*this);

    static std::string empty;
    auto it = self._lValueList.find(key);
    if (it != self._lValueList.end()) {
        return it->second;
    }
    return empty;
}

PyObject* PropertyMap::getPyObject()
{
    auto& self = propSetterSelf<App::PropertyMap>(*this);

    PyObject* dict = PyDict_New();

    for (auto it = self._lValueList.begin(); it != self._lValueList.end(); ++it) {
        PyObject* item = PyUnicode_DecodeUTF8(it->second.c_str(), it->second.size(), nullptr);
        if (!item) {
            Py_DECREF(dict);
            throw Base::UnicodeError("UTF8 conversion failure at PropertyMap::getPyObject()");
        }
        PyDict_SetItemString(dict, it->first.c_str(), item);
        Py_DECREF(item);
    }

    return dict;
}

void PropertyMap::setPyObject(PyObject* value)
{
    auto& self = propSetterSelf<App::PropertyMap>(*this);

    if (PyMapping_Check(value)) {
        std::map<std::string, std::string> values;
        // get key and item list
        PyObject* keyList = PyMapping_Keys(value);
        PyObject* itemList = PyMapping_Values(value);
        Py_ssize_t nSize = PyList_Size(keyList);

        for (Py_ssize_t i = 0; i < nSize; ++i) {
            // check on the key:
            std::string keyStr;
            PyObject* key = PyList_GetItem(keyList, i);
            if (PyUnicode_Check(key)) {
                keyStr = PyUnicode_AsUTF8(key);
            }
            else {
                std::string error("type of the key need to be string, not ");
                error += key->ob_type->tp_name;
                throw Base::TypeError(error);
            }

            // check on the item:
            PyObject* item = PyList_GetItem(itemList, i);
            if (PyUnicode_Check(item)) {
                values[keyStr] = PyUnicode_AsUTF8(item);
            }
            else {
                std::string error("type in values must be string, not ");
                error += item->ob_type->tp_name;
                throw Base::TypeError(error);
            }
        }

        Py_XDECREF(itemList);
        Py_XDECREF(keyList);

        self.setValues(values);
    }
    else {
        std::string error("type must be a dict or object with mapping protocol, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

unsigned int PropertyMap::getMemSize() const
{
    auto& self = propGetterSelf<const App::PropertyMap>(*this);

    size_t size = 0;
    for (const auto& it : self._lValueList) {
        size += it.second.size();
        size += it.first.size();
    }
    return size;
}

void PropertyMap::Save(Base::Writer& writer) const
{
    auto& self = propGetterSelf<const App::PropertyMap>(*this);

    writer.Stream() << writer.ind() << "<Map count=\"" << self.getSize() << "\">" << endl;
    writer.incInd();
    for (const auto& it : self._lValueList) {
        writer.Stream() << writer.ind() << "<Item key=\"" << encodeAttribute(it.first)
                        << "\" value=\"" << encodeAttribute(it.second) << "\"/>" << endl;
    }

    writer.decInd();
    writer.Stream() << writer.ind() << "</Map>" << endl;
}

void PropertyMap::Restore(Base::XMLReader& reader)
{
    auto& self = propSetterSelf<App::PropertyMap>(*this);

    // read my Element
    reader.readElement("Map");
    // get the value of my Attribute
    int count = reader.getAttribute<long>("count");

    std::map<std::string, std::string> values;
    for (int i = 0; i < count; i++) {
        reader.readElement("Item");
        values[reader.getAttribute<const char*>("key")] = reader.getAttribute<const char*>("value");
    }

    reader.readEndElement("Map");

    // assignment
    self.setValues(values);
}

Property* PropertyMap::Copy() const
{
    auto& self = propGetterSelf<const App::PropertyMap>(*this);

    PropertyMap* p = new PropertyMap();
    p->_lValueList = self._lValueList;
    return p;
}

void PropertyMap::Paste(const Property& from)
{
    auto& self = propSetterSelf<App::PropertyMap>(*this);

    self.aboutToSetValue();
    self._lValueList = dynamic_cast<const PropertyMap&>(from)._lValueList;
    hasSetValue();
}


//**************************************************************************
//**************************************************************************
// PropertyBool
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyBool, App::Property)

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
    auto& self = propSetterSelf<App::PropertyBool>(*this);

    self.aboutToSetValue();
    self._lValue = lValue;
    hasSetValue();
}

bool PropertyBool::getValue() const
{
    auto& self = propGetterSelf<const App::PropertyBool>(*this);

    return self._lValue;
}

PyObject* PropertyBool::getPyObject()
{
    auto& self = propGetterSelf<const App::PropertyBool>(*this);

    return PyBool_FromLong(self._lValue ? 1 : 0);
}

void PropertyBool::setPyObject(PyObject* value)
{
    auto& self = propSetterSelf<App::PropertyBool>(*this);

    if (PyBool_Check(value) || PyLong_Check(value)) {
        self.setValue(Base::asBoolean(value));
    }
    else {
        std::string error = std::string("type must be bool, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyBool::Save(Base::Writer& writer) const
{
    auto& self = propGetterSelf<const App::PropertyBool>(*this);

    writer.Stream() << writer.ind() << "<Bool value=\"";
    if (self._lValue) {
        writer.Stream() << "true" << "\"/>";
    }
    else {
        writer.Stream() << "false" << "\"/>";
    }
    writer.Stream() << std::endl;
}

void PropertyBool::Restore(Base::XMLReader& reader)
{
    auto& self = propSetterSelf<App::PropertyBool>(*this);

    // read my Element
    reader.readElement("Bool");
    // get the value of my Attribute
    string b = reader.getAttribute<const char*>("value");
    (b == "true") ? self.setValue(true) : self.setValue(false);
}


Property* PropertyBool::Copy() const
{
    auto& self = propGetterSelf<const App::PropertyBool>(*this);

    PropertyBool* p = new PropertyBool();
    p->_lValue = self._lValue;
    return p;
}

void PropertyBool::Paste(const Property& from)
{
    auto& self = propSetterSelf<App::PropertyBool>(*this);

    self.aboutToSetValue();
    self._lValue = dynamic_cast<const PropertyBool&>(from)._lValue;
    hasSetValue();
}

void PropertyBool::setPathValue(const ObjectIdentifier& path, const boost::any& value)
{
    auto& self = propSetterSelf<App::PropertyBool>(*this);

    self.verifyPath(path);

    if (value.type() == typeid(bool)) {
        self.setValue(boost::any_cast<bool>(value));
    }
    else if (value.type() == typeid(int)) {
        self.setValue(boost::any_cast<int>(value) != 0);
    }
    else if (value.type() == typeid(long)) {
        self.setValue(boost::any_cast<long>(value) != 0);
    }
    else if (value.type() == typeid(double)) {
        self.setValue(boost::math::round(boost::any_cast<double>(value)));
    }
    else if (value.type() == typeid(float)) {
        self.setValue(boost::math::round(boost::any_cast<float>(value)));
    }
    else if (value.type() == typeid(Quantity)) {
        self.setValue(boost::any_cast<Quantity>(value).getValue() != 0);
    }
    else {
        throw bad_cast();
    }
}

const boost::any PropertyBool::getPathValue(const ObjectIdentifier& path) const
{
    auto& self = propGetterSelf<const App::PropertyBool>(*this);

    self.verifyPath(path);

    return self._lValue;
}

//**************************************************************************
//**************************************************************************
// PropertyBoolList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyBoolList, App::PropertyLists)

//**************************************************************************
// Construction/Destruction


PropertyBoolList::PropertyBoolList() = default;

PropertyBoolList::~PropertyBoolList() = default;

//**************************************************************************
// Base class implementer

PyObject* PropertyBoolList::getPyObject()
{
    auto& self = propGetterSelf<const App::PropertyBoolList>(*this);

    PyObject* tuple = PyTuple_New(self.getSize());
    for (int i = 0; i < self.getSize(); i++) {
        bool v = self._lValueList[i];
        if (v) {
            PyTuple_SetItem(tuple, i, PyBool_FromLong(1));
        }
        else {
            PyTuple_SetItem(tuple, i, PyBool_FromLong(0));
        }
    }
    return tuple;
}

void PropertyBoolList::setPyObject(PyObject* value)
{
    auto& self = propSetterSelf<App::PropertyBoolList>(*this);

    // string is also a sequence and must be treated differently
    std::string str;
    if (PyUnicode_Check(value)) {
        str = PyUnicode_AsUTF8(value);
        boost::dynamic_bitset<> values(str);
        self.setValues(values);
    }
    else {
        inherited::setPyObject(value);
    }
}

bool PropertyBoolList::getPyValue(PyObject* item) const
{
    if (PyBool_Check(item)) {
        return Base::asBoolean(item);
    }
    else if (PyLong_Check(item)) {
        return (PyLong_AsLong(item) ? true : false);
    }
    else {
        std::string error = std::string("type in list must be bool or int, not ");
        error += item->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyBoolList::Save(Base::Writer& writer) const
{
    auto& self = propGetterSelf<const App::PropertyBoolList>(*this);

    writer.Stream() << writer.ind() << "<BoolList value=\"";
    std::string bitset;
    boost::to_string(self._lValueList, bitset);
    writer.Stream() << bitset << "\"/>";
    writer.Stream() << std::endl;
}

void PropertyBoolList::Restore(Base::XMLReader& reader)
{
    auto& self = propSetterSelf<App::PropertyBoolList>(*this);

    // read my Element
    reader.readElement("BoolList");
    // get the value of my Attribute
    string str = reader.getAttribute<const char*>("value");
    boost::dynamic_bitset<> bitset(str);
    self.setValues(bitset);
}

Property* PropertyBoolList::Copy() const
{
    auto& self = propGetterSelf<const App::PropertyBoolList>(*this);

    PropertyBoolList* p = new PropertyBoolList();
    p->_lValueList = self._lValueList;
    return p;
}

void PropertyBoolList::Paste(const Property& from)
{
    auto& self = propSetterSelf<App::PropertyBoolList>(*this);

    self.setValues(dynamic_cast<const PropertyBoolList&>(from)._lValueList);
}

unsigned int PropertyBoolList::getMemSize() const
{
    auto& self = propGetterSelf<const App::PropertyBoolList>(*this);

    return static_cast<unsigned int>(self._lValueList.size());
}

//**************************************************************************
//**************************************************************************
// PropertyColor
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

namespace
{
/// The definition of "alpha" was corrected in FreeCAD 1.1 -- returns true if the reader is working
/// on a file that pre-dates that correction.
bool readerRequiresAlphaConversion(const Base::XMLReader &reader)
{
    return Base::getVersion(reader.ProgramVersion) < Base::Version::v1_1;
}

/// Given a material, invert the alpha channel of all of its colors.
void convertAlphaInMaterial(App::Material& material)
{
    material.ambientColor.a = 1.0F - material.ambientColor.a;
    material.diffuseColor.a = 1.0F - material.diffuseColor.a;
    material.specularColor.a = 1.0F - material.specularColor.a;
    material.emissiveColor.a = 1.0F - material.emissiveColor.a;
}
}

TYPESYSTEM_SOURCE(App::PropertyColor, App::Property)

//**************************************************************************
// Construction/Destruction

PropertyColor::PropertyColor() = default;

PropertyColor::~PropertyColor() = default;

//**************************************************************************
// Base class implementer

void PropertyColor::setValue(const Base::Color& col)
{
    auto& self = propSetterSelf<App::PropertyColor>(*this);

    self.aboutToSetValue();
    self._cCol = col;
    hasSetValue();
}

void PropertyColor::setValue(uint32_t rgba)
{
    auto& self = propSetterSelf<App::PropertyColor>(*this);

    self.aboutToSetValue();
    self._cCol.setPackedValue(rgba);
    self.hasSetValue();
}

void PropertyColor::setValue(float r, float g, float b, float a)
{
    auto& self = propSetterSelf<App::PropertyColor>(*this);

    self.aboutToSetValue();
    self._cCol.set(r, g, b, a);
    self.hasSetValue();
}

const Base::Color& PropertyColor::getValue() const
{
    auto& self = propGetterSelf<const App::PropertyColor>(*this);

    return self._cCol;
}

PyObject* PropertyColor::getPyObject()
{
    auto& self = propGetterSelf<const App::PropertyColor>(*this);

    PyObject* rgba = PyTuple_New(4);
    PyObject* r = PyFloat_FromDouble(self._cCol.r);
    PyObject* g = PyFloat_FromDouble(self._cCol.g);
    PyObject* b = PyFloat_FromDouble(self._cCol.b);
    PyObject* a = PyFloat_FromDouble(self._cCol.a);

    PyTuple_SetItem(rgba, 0, r);
    PyTuple_SetItem(rgba, 1, g);
    PyTuple_SetItem(rgba, 2, b);
    PyTuple_SetItem(rgba, 3, a);

    return rgba;
}

void PropertyColor::setPyObject(PyObject* value)
{
    auto& self = propSetterSelf<App::PropertyColor>(*this);

    Base::Color cCol;
    if (PyTuple_Check(value) && (PyTuple_Size(value) == 3 || PyTuple_Size(value) == 4)) {
        PyObject* item;
        item = PyTuple_GetItem(value, 0);
        if (PyFloat_Check(item)) {
            cCol.r = (float)PyFloat_AsDouble(item);
            item = PyTuple_GetItem(value, 1);
            if (PyFloat_Check(item)) {
                cCol.g = (float)PyFloat_AsDouble(item);
            }
            else {
                throw Base::TypeError("Type in tuple must be consistent (float)");
            }
            item = PyTuple_GetItem(value, 2);
            if (PyFloat_Check(item)) {
                cCol.b = (float)PyFloat_AsDouble(item);
            }
            else {
                throw Base::TypeError("Type in tuple must be consistent (float)");
            }
            if (PyTuple_Size(value) == 4) {
                item = PyTuple_GetItem(value, 3);
                if (PyFloat_Check(item)) {
                    cCol.a = (float)PyFloat_AsDouble(item);
                }
                else {
                    throw Base::TypeError("Type in tuple must be consistent (float)");
                }
            }
        }
        else if (PyLong_Check(item)) {
            cCol.r = PyLong_AsLong(item) / 255.0;
            item = PyTuple_GetItem(value, 1);
            if (PyLong_Check(item)) {
                cCol.g = PyLong_AsLong(item) / 255.0;
            }
            else {
                throw Base::TypeError("Type in tuple must be consistent (integer)");
            }
            item = PyTuple_GetItem(value, 2);
            if (PyLong_Check(item)) {
                cCol.b = PyLong_AsLong(item) / 255.0;
            }
            else {
                throw Base::TypeError("Type in tuple must be consistent (integer)");
            }
            if (PyTuple_Size(value) == 4) {
                item = PyTuple_GetItem(value, 3);
                if (PyLong_Check(item)) {
                    cCol.a = PyLong_AsLong(item) / 255.0;
                }
                else {
                    throw Base::TypeError("Type in tuple must be consistent (integer)");
                }
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
        std::string error =
            std::string("type must be integer or tuple of float or tuple integer, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }

    self.setValue(cCol);
}

void PropertyColor::Save(Base::Writer& writer) const
{
    auto& self = propGetterSelf<const App::PropertyColor>(*this);

    writer.Stream() << writer.ind() << "<PropertyColor value=\"" << self._cCol.getPackedValue() << "\"/>"
                    << endl;
}

void PropertyColor::Restore(Base::XMLReader& reader)
{
    auto& self = propSetterSelf<App::PropertyColor>(*this);

    // read my Element
    reader.readElement("PropertyColor");
    // get the value of my Attribute
    unsigned long rgba = reader.getAttribute<unsigned long>("value");
    if (readerRequiresAlphaConversion(reader)) {
        // Convert transparency / alpha value
        constexpr unsigned long alphaMax = 0xff;
        unsigned long alpha = alphaMax - (rgba & alphaMax);
        rgba = rgba - (rgba & alphaMax) + alpha;
    }
    self.setValue(rgba);
}

Property* PropertyColor::Copy() const
{
    auto& self = propGetterSelf<const App::PropertyColor>(*this);

    PropertyColor* p = new PropertyColor();
    p->_cCol = self._cCol;
    return p;
}

void PropertyColor::Paste(const Property& from)
{
    auto& self = propSetterSelf<App::PropertyColor>(*this);

    self.aboutToSetValue();
    self._cCol = dynamic_cast<const PropertyColor&>(from)._cCol;
    hasSetValue();
}

//**************************************************************************
// PropertyColorList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyColorList, App::PropertyLists)

//**************************************************************************
// Construction/Destruction

PropertyColorList::PropertyColorList() = default;

PropertyColorList::~PropertyColorList() = default;

//**************************************************************************
// Base class implementer

PyObject* PropertyColorList::getPyObject()
{
    auto& self = propGetterSelf<const App::PropertyColorList>(*this);

    PyObject* list = PyList_New(self.getSize());

    for (int i = 0; i < self.getSize(); i++) {
        PyObject* rgba = PyTuple_New(4);
        PyObject* r = PyFloat_FromDouble(self._lValueList[i].r);
        PyObject* g = PyFloat_FromDouble(self._lValueList[i].g);
        PyObject* b = PyFloat_FromDouble(self._lValueList[i].b);
        PyObject* a = PyFloat_FromDouble(self._lValueList[i].a);

        PyTuple_SetItem(rgba, 0, r);
        PyTuple_SetItem(rgba, 1, g);
        PyTuple_SetItem(rgba, 2, b);
        PyTuple_SetItem(rgba, 3, a);

        PyList_SetItem(list, i, rgba);
    }

    return list;
}

Base::Color PropertyColorList::getPyValue(PyObject* item) const
{
    auto& self = propGetterSelf<const App::PropertyColorList>(*this);

    PropertyColor col;
    col.setPyObject(item);
    return col.getValue();
}

void PropertyColorList::Save(Base::Writer& writer) const
{
    auto& self = propGetterSelf<const App::PropertyColorList>(*this);

    if (!writer.isForceXML()) {
        writer.Stream() << writer.ind() << "<ColorList file=\""
                        << (self.getSize() ? writer.addFile(self.getName(), &self) : "") << "\"/>"
                        << std::endl;
    }
}

void PropertyColorList::Restore(Base::XMLReader& reader)
{
    auto& self = propSetterSelf<App::PropertyColorList>(*this);

    reader.readElement("ColorList");
    if (reader.hasAttribute("file")) {
        std::string file(reader.getAttribute<const char*>("file"));

        if (!file.empty()) {
            // initiate a file read
            reader.addFile(file.c_str(), &self);
        }

        self.requiresAlphaConversion = readerRequiresAlphaConversion(reader);
    }
}

void PropertyColorList::SaveDocFile(Base::Writer& writer) const
{
    auto& self = propGetterSelf<const App::PropertyColorList>(*this);

    Base::OutputStream str(writer.Stream());
    uint32_t uCt = (uint32_t)self.getSize();
    str << uCt;
    for (auto it : self._lValueList) {
        str << it.getPackedValue();
    }
}

void PropertyColorList::RestoreDocFile(Base::Reader& reader)
{
    auto& self = propSetterSelf<App::PropertyColorList>(*this);

    Base::InputStream str(reader);
    uint32_t uCt = 0;
    str >> uCt;
    std::vector<Base::Color> values(uCt);
    uint32_t value;  // must be 32 bit long
    for (auto& it : values) {
        str >> value;
        it.setPackedValue(value);
    }
    if (self.requiresAlphaConversion) {
        for (auto& it : values) {
            it.a = 1.0F - it.a;
        }
    }
    self.setValues(values);
}

Property* PropertyColorList::Copy() const
{
    auto& self = propGetterSelf<const App::PropertyColorList>(*this);

    PropertyColorList* p = new PropertyColorList();
    p->_lValueList = self._lValueList;
    return p;
}

void PropertyColorList::Paste(const Property& from)
{
    auto& self = propSetterSelf<App::PropertyColorList>(*this);

    self.setValues(dynamic_cast<const PropertyColorList&>(from)._lValueList);
}

unsigned int PropertyColorList::getMemSize() const
{
    auto& self = propGetterSelf<const App::PropertyColorList>(*this);

    return static_cast<unsigned int>(self._lValueList.size() * sizeof(Base::Color));
}

//**************************************************************************
//**************************************************************************
// PropertyMaterial
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyMaterial, App::Property)

PropertyMaterial::PropertyMaterial() = default;

PropertyMaterial::~PropertyMaterial() = default;

void PropertyMaterial::setValue(const Material& mat)
{
    auto& self = propSetterSelf<App::PropertyMaterial>(*this);

    self.aboutToSetValue();
    self._cMat = mat;
    hasSetValue();
}

void PropertyMaterial::setValue(const Base::Color& col)
{
    auto& self = propSetterSelf<App::PropertyMaterial>(*this);

    self.setDiffuseColor(col);
}

void PropertyMaterial::setValue(float r, float g, float b, float a)
{
    auto& self = propSetterSelf<App::PropertyMaterial>(*this);

    self.setDiffuseColor(r, g, b, a);
}

void PropertyMaterial::setValue(uint32_t rgba)
{
    auto& self = propSetterSelf<App::PropertyMaterial>(*this);

    self.setDiffuseColor(rgba);
}

const Material& PropertyMaterial::getValue() const
{
    auto& self = propGetterSelf<const App::PropertyMaterial>(*this);

    return self._cMat;
}

void PropertyMaterial::setAmbientColor(const Base::Color& col)
{
    auto& self = propSetterSelf<App::PropertyMaterial>(*this);

    self.aboutToSetValue();
    self._cMat.ambientColor = col;
    hasSetValue();
}

void PropertyMaterial::setAmbientColor(float r, float g, float b, float a)
{
    auto& self = propSetterSelf<App::PropertyMaterial>(*this);

    self.aboutToSetValue();
    self._cMat.ambientColor.set(r, g, b, a);
    self.hasSetValue();
}

void PropertyMaterial::setAmbientColor(uint32_t rgba)
{
    auto& self = propSetterSelf<App::PropertyMaterial>(*this);

    self.aboutToSetValue();
    self._cMat.ambientColor.setPackedValue(rgba);
    self.hasSetValue();
}

void PropertyMaterial::setDiffuseColor(const Base::Color& col)
{
    auto& self = propSetterSelf<App::PropertyMaterial>(*this);

    self.aboutToSetValue();
    self._cMat.diffuseColor = col;
    hasSetValue();
}

void PropertyMaterial::setDiffuseColor(float r, float g, float b, float a)
{
    auto& self = propSetterSelf<App::PropertyMaterial>(*this);

    self.aboutToSetValue();
    self._cMat.diffuseColor.set(r, g, b, a);
    self.hasSetValue();
}

void PropertyMaterial::setDiffuseColor(uint32_t rgba)
{
    auto& self = propSetterSelf<App::PropertyMaterial>(*this);

    self.aboutToSetValue();
    self._cMat.diffuseColor.setPackedValue(rgba);
    self.hasSetValue();
}

void PropertyMaterial::setSpecularColor(const Base::Color& col)
{
    auto& self = propSetterSelf<App::PropertyMaterial>(*this);

    self.aboutToSetValue();
    self._cMat.specularColor = col;
    hasSetValue();
}

void PropertyMaterial::setSpecularColor(float r, float g, float b, float a)
{
    auto& self = propSetterSelf<App::PropertyMaterial>(*this);

    self.aboutToSetValue();
    self._cMat.specularColor.set(r, g, b, a);
    self.hasSetValue();
}

void PropertyMaterial::setSpecularColor(uint32_t rgba)
{
    auto& self = propSetterSelf<App::PropertyMaterial>(*this);

    self.aboutToSetValue();
    self._cMat.specularColor.setPackedValue(rgba);
    self.hasSetValue();
}

void PropertyMaterial::setEmissiveColor(const Base::Color& col)
{
    auto& self = propSetterSelf<App::PropertyMaterial>(*this);

    self.aboutToSetValue();
    self._cMat.emissiveColor = col;
    hasSetValue();
}

void PropertyMaterial::setEmissiveColor(float r, float g, float b, float a)
{
    auto& self = propSetterSelf<App::PropertyMaterial>(*this);

    self.aboutToSetValue();
    self._cMat.emissiveColor.set(r, g, b, a);
    self.hasSetValue();
}

void PropertyMaterial::setEmissiveColor(uint32_t rgba)
{
    auto& self = propSetterSelf<App::PropertyMaterial>(*this);

    self.aboutToSetValue();
    self._cMat.emissiveColor.setPackedValue(rgba);
    self.hasSetValue();
}

void PropertyMaterial::setShininess(float val)
{
    auto& self = propSetterSelf<App::PropertyMaterial>(*this);

    self.aboutToSetValue();
    self._cMat.shininess = val;
    hasSetValue();
}

void PropertyMaterial::setTransparency(float val)
{
    auto& self = propSetterSelf<App::PropertyMaterial>(*this);

    self.aboutToSetValue();
    self._cMat.transparency = val;
    hasSetValue();
}

const Base::Color& PropertyMaterial::getAmbientColor() const
{
    auto& self = propGetterSelf<const App::PropertyMaterial>(*this);

    return self._cMat.ambientColor;
}

const Base::Color& PropertyMaterial::getDiffuseColor() const
{
    auto& self = propGetterSelf<const App::PropertyMaterial>(*this);

    return self._cMat.diffuseColor;
}

const Base::Color& PropertyMaterial::getSpecularColor() const
{
    auto& self = propGetterSelf<const App::PropertyMaterial>(*this);

    return self._cMat.specularColor;
}

const Base::Color& PropertyMaterial::getEmissiveColor() const
{
    auto& self = propGetterSelf<const App::PropertyMaterial>(*this);

    return self._cMat.emissiveColor;
}

double PropertyMaterial::getShininess() const
{
    auto& self = propGetterSelf<const App::PropertyMaterial>(*this);

    return self._cMat.shininess;
}

double PropertyMaterial::getTransparency() const
{
    auto& self = propGetterSelf<const App::PropertyMaterial>(*this);

    return self._cMat.transparency;
}

PyObject* PropertyMaterial::getPyObject()
{
    auto& self = propGetterSelf<const App::PropertyMaterial>(*this);

    return new MaterialPy(new Material(self._cMat));
}

void PropertyMaterial::setPyObject(PyObject* value)
{
    auto& self = propSetterSelf<App::PropertyMaterial>(*this);

    if (PyObject_TypeCheck(value, &(MaterialPy::Type))) {
        self.setValue(*static_cast<MaterialPy*>(value)->getMaterialPtr());
    }
    else {
        self.setValue(MaterialPy::toColor(value));
    }
}

void PropertyMaterial::Save(Base::Writer& writer) const
{
    auto& self = propGetterSelf<const App::PropertyMaterial>(*this);

    // clang-format off
    writer.Stream() << writer.ind()
                    << "<PropertyMaterial ambientColor=\"" << self._cMat.ambientColor.getPackedValue()
                    << "\" diffuseColor=\"" << self._cMat.diffuseColor.getPackedValue()
                    << "\" specularColor=\"" << self._cMat.specularColor.getPackedValue()
                    << "\" emissiveColor=\"" << self._cMat.emissiveColor.getPackedValue()
                    << "\" shininess=\"" << self._cMat.shininess
                    << "\" transparency=\"" << self._cMat.transparency
                    << "\" image=\"" << self._cMat.image
                    << "\" imagePath=\"" << self._cMat.imagePath
                    << "\" uuid=\"" << self._cMat.uuid
                    << "\"/>" << std::endl;
    // clang-format on
}

void PropertyMaterial::Restore(Base::XMLReader& reader)
{
    auto& self = propSetterSelf<App::PropertyMaterial>(*this);

    // read my Element
    reader.readElement("PropertyMaterial");
    // get the value of my Attribute
    self.aboutToSetValue();
    self._cMat.ambientColor.setPackedValue(reader.getAttribute<unsigned long>("ambientColor"));
    self._cMat.diffuseColor.setPackedValue(reader.getAttribute<unsigned long>("diffuseColor"));
    self._cMat.specularColor.setPackedValue(reader.getAttribute<unsigned long>("specularColor"));
    self._cMat.emissiveColor.setPackedValue(reader.getAttribute<unsigned long>("emissiveColor"));
    self._cMat.shininess = (float)reader.getAttribute<double>("shininess");
    self._cMat.transparency = (float)reader.getAttribute<double>("transparency");
    if (readerRequiresAlphaConversion(reader)) {
        convertAlphaInMaterial(self._cMat);
    }
    if (reader.hasAttribute("image")) {
        self._cMat.image = reader.getAttribute<const char*>("image");
    }
    if (reader.hasAttribute("imagePath")) {
        self._cMat.imagePath = reader.getAttribute<const char*>("imagePath");
    }
    if (reader.hasAttribute("uuid")) {
        self._cMat.uuid = reader.getAttribute<const char*>("uuid");
    }
    hasSetValue();
}

const char* PropertyMaterial::getEditorName() const
{
    auto& self = propGetterSelf<const App::PropertyMaterial>(*this);

    if (self.testStatus(MaterialEdit)) {
        return "Gui::PropertyEditor::PropertyMaterialItem";
    }
    return "";
}

Property* PropertyMaterial::Copy() const
{
    auto& self = propGetterSelf<const App::PropertyMaterial>(*this);

    PropertyMaterial* p = new PropertyMaterial();
    p->_cMat = self._cMat;
    return p;
}

void PropertyMaterial::Paste(const Property& from)
{
    auto& self = propSetterSelf<App::PropertyMaterial>(*this);

    self.aboutToSetValue();
    self._cMat = dynamic_cast<const PropertyMaterial&>(from)._cMat;
    hasSetValue();
}

//**************************************************************************
// PropertyMaterialList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyMaterialList, App::PropertyLists)

//**************************************************************************
// Construction/Destruction

PropertyMaterialList::PropertyMaterialList()
{
    setMinimumSizeOne();
}

PropertyMaterialList::~PropertyMaterialList() = default;

//**************************************************************************
// Base class implementer

void PropertyMaterialList::setValues(const std::vector<App::Material>& newValues)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    if (!newValues.empty()) {
        PropertyListsT<Material>::setValues(newValues);
    }
    else {
        self.aboutToSetValue();
        self.setSize(1);
        self.hasSetValue();
    }
}

PyObject* PropertyMaterialList::getPyObject()
{
    auto& self = propGetterSelf<const App::PropertyMaterialList>(*this);

    Py::Tuple tuple(self.getSize());

    for (int i = 0; i < self.getSize(); i++) {
        tuple.setItem(i, Py::asObject(new MaterialPy(new Material(self._lValueList[i]))));
    }

    return Py::new_reference_to(tuple);
}

void PropertyMaterialList::verifyIndex(int index) const
{
    auto& self = propGetterSelf<const App::PropertyMaterialList>(*this);

    int size = self.getSize();
    if (index < -1 || index > size) {
        throw Base::RuntimeError("index out of bound");
    }
}

void PropertyMaterialList::setMinimumSizeOne()
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    int size = self.getSize();
    if (size < 1) {
        self.setSize(1);
    }
}

int PropertyMaterialList::resizeByOneIfNeeded(int index)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    int size = self.getSize();
    if (index == -1 || index == size) {
        index = size;
        self.setSize(size + 1);
    }

    return index;
}

void PropertyMaterialList::setValue()
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    Material empty;
    self.setValue(empty);
}

void PropertyMaterialList::setValue(const Material& mat)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.aboutToSetValue();
    self.setSize(1);
    for (auto& material : self._lValueList) {
        material = mat;
    }
    self.hasSetValue();
}

void PropertyMaterialList::setValue(int index, const Material& mat)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.verifyIndex(index);

    self.aboutToSetValue();
    index = self.resizeByOneIfNeeded(index);
    self._lValueList[index] = mat;
    self.hasSetValue();
}

void PropertyMaterialList::setAmbientColor(const Base::Color& col)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.aboutToSetValue();
    self.setMinimumSizeOne();
    for (auto& material : self._lValueList) {
        material.ambientColor = col;
    }
    self.hasSetValue();
}

void PropertyMaterialList::setAmbientColor(float r, float g, float b, float a)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.aboutToSetValue();
    self.setMinimumSizeOne();
    for (auto& material : self._lValueList) {
        material.ambientColor.set(r, g, b, a);
    }
    self.hasSetValue();
}

void PropertyMaterialList::setAmbientColor(uint32_t rgba)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.aboutToSetValue();
    self.setMinimumSizeOne();
    for (auto& material : self._lValueList) {
        material.ambientColor.setPackedValue(rgba);
    }
    self.hasSetValue();
}

void PropertyMaterialList::setAmbientColor(int index, const Base::Color& col)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.verifyIndex(index);

    self.aboutToSetValue();
    index = self.resizeByOneIfNeeded(index);
    self._lValueList[index].ambientColor = col;
    self.hasSetValue();
}

void PropertyMaterialList::setAmbientColor(int index, float r, float g, float b, float a)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.verifyIndex(index);

    self.aboutToSetValue();
    index = self.resizeByOneIfNeeded(index);
    self._lValueList[index].ambientColor.set(r, g, b, a);
    self.hasSetValue();
}

void PropertyMaterialList::setAmbientColor(int index, uint32_t rgba)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.verifyIndex(index);

    self.aboutToSetValue();
    index = self.resizeByOneIfNeeded(index);
    self._lValueList[index].ambientColor.setPackedValue(rgba);
    self.hasSetValue();
}

void PropertyMaterialList::setDiffuseColor(const Base::Color& col)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.aboutToSetValue();
    self.setMinimumSizeOne();
    for (auto& material : self._lValueList) {
        material.diffuseColor = col;
    }
    self.hasSetValue();
}

void PropertyMaterialList::setDiffuseColor(float r, float g, float b, float a)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.aboutToSetValue();
    self.setMinimumSizeOne();
    for (auto& material : self._lValueList) {
        material.diffuseColor.set(r, g, b, a);
    }
    self.hasSetValue();
}

void PropertyMaterialList::setDiffuseColor(uint32_t rgba)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.aboutToSetValue();
    self.setMinimumSizeOne();
    for (auto& material : self._lValueList) {
        material.diffuseColor.setPackedValue(rgba);
    }
    self.hasSetValue();
}

void PropertyMaterialList::setDiffuseColor(int index, const Base::Color& col)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.verifyIndex(index);

    self.aboutToSetValue();
    index = self.resizeByOneIfNeeded(index);
    self._lValueList[index].diffuseColor = col;
    self.hasSetValue();
}

void PropertyMaterialList::setDiffuseColor(int index, float r, float g, float b, float a)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.verifyIndex(index);

    self.aboutToSetValue();
    index = self.resizeByOneIfNeeded(index);
    self._lValueList[index].diffuseColor.set(r, g, b, a);
    self.hasSetValue();
}

void PropertyMaterialList::setDiffuseColor(int index, uint32_t rgba)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.verifyIndex(index);

    self.aboutToSetValue();
    index = self.resizeByOneIfNeeded(index);
    self._lValueList[index].diffuseColor.setPackedValue(rgba);
    self.hasSetValue();
}

void PropertyMaterialList::setDiffuseColors(const std::vector<Base::Color>& colors)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.aboutToSetValue();
    self.setSize(colors.size(), self._lValueList[0]);

    for (std::size_t i = 0; i < colors.size(); i++) {
        self._lValueList[i].diffuseColor = colors[i];
    }
    self.hasSetValue();
}

void PropertyMaterialList::setSpecularColor(const Base::Color& col)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.aboutToSetValue();
    self.setMinimumSizeOne();
    for (auto& material : self._lValueList) {
        material.specularColor = col;
    }
    self.hasSetValue();
}

void PropertyMaterialList::setSpecularColor(float r, float g, float b, float a)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.aboutToSetValue();
    self.setMinimumSizeOne();
    for (auto& material : self._lValueList) {
        material.specularColor.set(r, g, b, a);
    }
    self.hasSetValue();
}

void PropertyMaterialList::setSpecularColor(uint32_t rgba)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.aboutToSetValue();
    self.setMinimumSizeOne();
    for (auto& material : self._lValueList) {
        material.specularColor.setPackedValue(rgba);
    }
    self.hasSetValue();
}

void PropertyMaterialList::setSpecularColor(int index, const Base::Color& col)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.verifyIndex(index);

    self.aboutToSetValue();
    index = self.resizeByOneIfNeeded(index);
    self._lValueList[index].specularColor = col;
    self.hasSetValue();
}

void PropertyMaterialList::setSpecularColor(int index, float r, float g, float b, float a)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.verifyIndex(index);

    self.aboutToSetValue();
    index = self.resizeByOneIfNeeded(index);
    self._lValueList[index].specularColor.set(r, g, b, a);
    self.hasSetValue();
}

void PropertyMaterialList::setSpecularColor(int index, uint32_t rgba)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.verifyIndex(index);

    self.aboutToSetValue();
    index = self.resizeByOneIfNeeded(index);
    self._lValueList[index].specularColor.setPackedValue(rgba);
    self.hasSetValue();
}

void PropertyMaterialList::setEmissiveColor(const Base::Color& col)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.aboutToSetValue();
    self.setMinimumSizeOne();
    for (auto& material : self._lValueList) {
        material.emissiveColor = col;
    }
    self.hasSetValue();
}

void PropertyMaterialList::setEmissiveColor(float r, float g, float b, float a)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.aboutToSetValue();
    self.setMinimumSizeOne();
    for (auto& material : self._lValueList) {
        material.emissiveColor.set(r, g, b, a);
    }
    self.hasSetValue();
}

void PropertyMaterialList::setEmissiveColor(uint32_t rgba)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.aboutToSetValue();
    self.setMinimumSizeOne();
    for (auto& material : self._lValueList) {
        material.emissiveColor.setPackedValue(rgba);
    }
    self.hasSetValue();
}

void PropertyMaterialList::setEmissiveColor(int index, const Base::Color& col)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.verifyIndex(index);

    self.aboutToSetValue();
    index = self.resizeByOneIfNeeded(index);
    self._lValueList[index].emissiveColor = col;
    self.hasSetValue();
}

void PropertyMaterialList::setEmissiveColor(int index, float r, float g, float b, float a)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.verifyIndex(index);

    self.aboutToSetValue();
    index = self.resizeByOneIfNeeded(index);
    self._lValueList[index].emissiveColor.set(r, g, b, a);
    self.hasSetValue();
}

void PropertyMaterialList::setEmissiveColor(int index, uint32_t rgba)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.verifyIndex(index);

    self.aboutToSetValue();
    index = self.resizeByOneIfNeeded(index);
    self._lValueList[index].emissiveColor.setPackedValue(rgba);
    self.hasSetValue();
}

void PropertyMaterialList::setShininess(float val)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.aboutToSetValue();
    self.setMinimumSizeOne();
    for (auto& material : self._lValueList) {
        material.shininess = val;
    }
    self.hasSetValue();
}

void PropertyMaterialList::setShininess(int index, float val)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.verifyIndex(index);

    self.aboutToSetValue();
    index = self.resizeByOneIfNeeded(index);
    self._lValueList[index].shininess = val;
    self.hasSetValue();
}

void PropertyMaterialList::setTransparency(float val)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.aboutToSetValue();
    self.setMinimumSizeOne();
    for (auto& material : self._lValueList) {
        material.transparency = val;
    }
    self.hasSetValue();
}

void PropertyMaterialList::setTransparency(int index, float val)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.verifyIndex(index);

    self.aboutToSetValue();
    index = self.resizeByOneIfNeeded(index);
    self._lValueList[index].transparency = val;
    self.hasSetValue();
}

void PropertyMaterialList::setTransparencies(const std::vector<float>& transparencies)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.aboutToSetValue();
    self.setSize(transparencies.size(), self._lValueList[0]);

    for (std::size_t i = 0; i < transparencies.size(); i++) {
        self._lValueList[i].transparency = transparencies[i];
    }
    self.hasSetValue();
}

const Base::Color& PropertyMaterialList::getAmbientColor() const
{
    auto& self = propGetterSelf<const App::PropertyMaterialList>(*this);

    return self._lValueList[0].ambientColor;
}

const Base::Color& PropertyMaterialList::getAmbientColor(int index) const
{
    auto& self = propGetterSelf<const App::PropertyMaterialList>(*this);

    return self._lValueList[index].ambientColor;
}

const Base::Color& PropertyMaterialList::getDiffuseColor() const
{
    auto& self = propGetterSelf<const App::PropertyMaterialList>(*this);

    return self._lValueList[0].diffuseColor;
}

const Base::Color& PropertyMaterialList::getDiffuseColor(int index) const
{
    auto& self = propGetterSelf<const App::PropertyMaterialList>(*this);

    return self._lValueList[index].diffuseColor;
}

std::vector<Base::Color> PropertyMaterialList::getDiffuseColors() const
{
    auto& self = propGetterSelf<const App::PropertyMaterialList>(*this);

    std::vector<Base::Color> list;
    for (auto& material : self._lValueList) {
        list.push_back(material.diffuseColor);
    }

    return list;
}

const Base::Color& PropertyMaterialList::getSpecularColor() const
{
    auto& self = propGetterSelf<const App::PropertyMaterialList>(*this);

    return self._lValueList[0].specularColor;
}

const Base::Color& PropertyMaterialList::getSpecularColor(int index) const
{
    auto& self = propGetterSelf<const App::PropertyMaterialList>(*this);

    return self._lValueList[index].specularColor;
}

const Base::Color& PropertyMaterialList::getEmissiveColor() const
{
    auto& self = propGetterSelf<const App::PropertyMaterialList>(*this);

    return self._lValueList[0].emissiveColor;
}

const Base::Color& PropertyMaterialList::getEmissiveColor(int index) const
{
    auto& self = propGetterSelf<const App::PropertyMaterialList>(*this);

    return self._lValueList[index].emissiveColor;
}

float PropertyMaterialList::getShininess() const
{
    auto& self = propGetterSelf<const App::PropertyMaterialList>(*this);

    return self._lValueList[0].shininess;
}

float PropertyMaterialList::getShininess(int index) const
{
    auto& self = propGetterSelf<const App::PropertyMaterialList>(*this);

    return self._lValueList[index].shininess;
}

float PropertyMaterialList::getTransparency() const
{
    auto& self = propGetterSelf<const App::PropertyMaterialList>(*this);

    return self._lValueList[0].transparency;
}

float PropertyMaterialList::getTransparency(int index) const
{
    auto& self = propGetterSelf<const App::PropertyMaterialList>(*this);

    return self._lValueList[index].transparency;
}

std::vector<float> PropertyMaterialList::getTransparencies() const
{
    auto& self = propGetterSelf<const App::PropertyMaterialList>(*this);

    std::vector<float> list;
    for (auto& material : self._lValueList) {
        list.push_back(material.transparency);
    }

    return list;
}

Material PropertyMaterialList::getPyValue(PyObject* value) const
{
    auto& self = propGetterSelf<const App::PropertyMaterialList>(*this);

    if (PyObject_TypeCheck(value, &(MaterialPy::Type))) {
        return *static_cast<MaterialPy*>(value)->getMaterialPtr();
    }
    else {
        std::string error = std::string("type must be 'Material', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyMaterialList::Save(Base::Writer& writer) const
{
    auto& self = propGetterSelf<const App::PropertyMaterialList>(*this);

    if (!writer.isForceXML()) {
        writer.Stream() << writer.ind() << "<MaterialList file=\""
                        << (self.getSize() ? writer.addFile(self.getName(), &self) : "") << "\""
                        << " version=\"3\"/>" << std::endl;
    }
}

void PropertyMaterialList::Restore(Base::XMLReader& reader)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    reader.readElement("MaterialList");
    if (reader.hasAttribute("file")) {
        std::string file(reader.getAttribute<const char*>("file"));
        if (reader.hasAttribute("version")) {
            self.formatVersion = static_cast<Format>(reader.getAttribute<long>("version"));
        }

        if (!file.empty()) {
            // initiate a file read
            reader.addFile(file.c_str(), &self);
        }

        self.requiresAlphaConversion = readerRequiresAlphaConversion(reader);
    }
}

void PropertyMaterialList::SaveDocFile(Base::Writer& writer) const
{
    auto& self = propGetterSelf<const App::PropertyMaterialList>(*this);

    Base::OutputStream str(writer.Stream());
    uint32_t uCt = (uint32_t)self.getSize();
    str << uCt;
    for (const auto& it : self._lValueList) {
        str << it.ambientColor.getPackedValue();
        str << it.diffuseColor.getPackedValue();
        str << it.specularColor.getPackedValue();
        str << it.emissiveColor.getPackedValue();
        str << it.shininess;
        str << it.transparency;
    }

    // Apply the latest changes last for backwards compatibility
    for (const auto& it : self._lValueList) {
        self.writeString(str, it.image);
        self.writeString(str, it.imagePath);
        self.writeString(str, it.uuid);
    }
}

void PropertyMaterialList::writeString(Base::OutputStream& str, const std::string& value) const
{
    auto& self = propGetterSelf<const App::PropertyMaterialList>(*this);

    uint32_t uCt = (uint32_t)value.size();
    str << uCt;
    str.write(value.c_str(), uCt);
}

void PropertyMaterialList::RestoreDocFile(Base::Reader& reader)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    Base::InputStream str(reader);
    if (self.formatVersion == Version_2) {
        // V2 is same as V0
        uint32_t count = 0;
        str >> count;
        self.RestoreDocFileV0(count, reader);
    }
    else if (self.formatVersion == Version_3) {
        // Default to the latest
        self.RestoreDocFileV3(reader);
    }
    else {
        int32_t version;
        str >> version;
        if (version < 0) {
            // This was a failed attempt at versioning, but is included
            // to support files created during development. In can be removed
            // in a future release once dev files are migrated.
            uint32_t count = 0;
            str >> count;
            self.RestoreDocFileV0(count, reader);
        }
        else {
            uint32_t uCt = static_cast<uint32_t>(version);
            self.RestoreDocFileV0(uCt, reader);
        }
    }
}

void PropertyMaterialList::RestoreDocFileV0(uint32_t count, Base::Reader& reader)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    Base::InputStream str(reader);
    std::vector<Material> values(count);
    uint32_t value {};  // must be 32 bit long
    float valueF {};
    for (auto& it : values) {
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
    self.convertAlpha(values);
    self.setValues(values);
}

void PropertyMaterialList::RestoreDocFileV3(Base::Reader& reader)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    Base::InputStream str(reader);
    uint32_t count = 0;
    str >> count;
    std::vector<Material> values(count);
    uint32_t value {};  // must be 32 bit long
    float valueF {};
    for (auto& it : values) {
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
    for (auto& it : values) {
        self.readString(str, it.image);
        self.readString(str, it.imagePath);
        self.readString(str, it.uuid);
    }
    self.convertAlpha(values);
    self.setValues(values);
}

void PropertyMaterialList::convertAlpha(std::vector<App::Material>& materials) const
{
    auto& self = propGetterSelf<const App::PropertyMaterialList>(*this);

    if (self.requiresAlphaConversion) {
        std::ranges::for_each(materials, convertAlphaInMaterial);
    }
}

void PropertyMaterialList::readString(Base::InputStream& str, std::string& value)
{
    auto& self = propGetterSelf<const App::PropertyMaterialList>(*this);

    uint32_t uCt {};
    str >> uCt;

    std::vector<char> temp(uCt);
    str.read(temp.data(), uCt);
    value.assign(temp.data(), temp.size());
}

const char* PropertyMaterialList::getEditorName() const
{
    auto& self = propGetterSelf<const App::PropertyMaterialList>(*this);

    if (self.testStatus(NoMaterialListEdit)) {
        return "";
    }
    return "Gui::PropertyEditor::PropertyMaterialListItem";
}

Property* PropertyMaterialList::Copy() const
{
    auto& self = propGetterSelf<const App::PropertyMaterialList>(*this);

    PropertyMaterialList* p = new PropertyMaterialList();
    p->_lValueList = self._lValueList;
    return p;
}

void PropertyMaterialList::Paste(const Property& from)
{
    auto& self = propSetterSelf<App::PropertyMaterialList>(*this);

    self.setValues(dynamic_cast<const PropertyMaterialList&>(from)._lValueList);
}

unsigned int PropertyMaterialList::getMemSize() const
{
    auto& self = propGetterSelf<const App::PropertyMaterialList>(*this);

    return static_cast<unsigned int>(self._lValueList.size() * sizeof(Material));
}

//**************************************************************************
// PropertyPersistentObject
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyPersistentObject, App::PropertyString)

PyObject* PropertyPersistentObject::getPyObject()
{
    auto& self = propGetterSelf<const App::PropertyPersistentObject>(*this);

    if (self._pObject) {
        return self._pObject->getPyObject();
    }
    return inherited::getPyObject();
}

void PropertyPersistentObject::Save(Base::Writer& writer) const
{
    auto& self = propGetterSelf<const App::PropertyPersistentObject>(*this);

    inherited::Save(writer);
    writer.Stream() << writer.ind() << "<PersistentObject>" << std::endl;
    if (self._pObject) {
        writer.incInd();
        self._pObject->Save(writer);
        writer.decInd();
    }
    writer.Stream() << writer.ind() << "</PersistentObject>" << std::endl;
}

void PropertyPersistentObject::Restore(Base::XMLReader& reader)
{
    auto& self = propGetterSelf<const App::PropertyPersistentObject>(*this);

    inherited::Restore(reader);
    reader.readElement("PersistentObject");
    if (self._pObject) {
        self._pObject->Restore(reader);
    }
    reader.readEndElement("PersistentObject");
}

Property* PropertyPersistentObject::Copy() const
{
    auto& self = propGetterSelf<const App::PropertyPersistentObject>(*this);

    auto* p = new PropertyPersistentObject();
    p->_cValue = self._cValue;
    p->_pObject = self._pObject;
    return p;
}

void PropertyPersistentObject::Paste(const Property& from)
{
    auto& self = propSetterSelf<App::PropertyPersistentObject>(*this);

    const auto& prop = dynamic_cast<const PropertyPersistentObject&>(from);
    if (self._cValue != prop._cValue || self._pObject != prop._pObject) {
        self.aboutToSetValue();
        self._cValue = prop._cValue;
        self._pObject = prop._pObject;
        hasSetValue();
    }
}

unsigned int PropertyPersistentObject::getMemSize() const
{
    auto& self = propGetterSelf<const App::PropertyPersistentObject>(*this);

    auto size = inherited::getMemSize();
    if (self._pObject) {
        size += self._pObject->getMemSize();
    }
    return size;
}

void PropertyPersistentObject::setValue(const char* type)
{
    auto& self = propSetterSelf<App::PropertyPersistentObject>(*this);

    if (!Base::Tools::isNullOrEmpty(type)) {
        Base::Type t = Base::Type::getTypeIfDerivedFrom(type, Persistence::getClassTypeId());
        if (t.isBad()) {
            throw Base::TypeError("Invalid type or type must be derived from Base::Persistence");
        }
        if (self._pObject && self._pObject->getTypeId() == t) {
            return;
        }
    }
    self.aboutToSetValue();
    self._pObject.reset();
    self._cValue = type;
    if (type[0]) {
        self._pObject.reset(static_cast<Base::Persistence*>(Base::Type::createInstanceByName(type)));
    }
    hasSetValue();
}
