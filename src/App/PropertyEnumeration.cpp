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
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include "ObjectIdentifier.h"
#include "PropertyEnumeration.h"
#include "PropertyString.h"


namespace App {

TYPESYSTEM_SOURCE(App::PropertyEnumeration, App::PropertyInteger)

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
    // For backward compatibility, if the property container is not attached to
    // any document (i.e. its full name starts with '?'), do not notify, or
    // else existing code may crash.
    bool notify = !boost::starts_with(getFullName(), "?");
    if (notify) {
        aboutToSetValue();
    }
    _enum.setEnums(plEnums);
    if (notify) {
        hasSetValue();
    }
}

void PropertyEnumeration::setEnums(const std::vector<std::string>& Enums)
{
    setEnumVector(Enums);
}

void PropertyEnumeration::setValue(const char* value)
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

void PropertyEnumeration::setValue(const Enumeration& source)
{
    aboutToSetValue();
    _enum = source;
    hasSetValue();
}

long PropertyEnumeration::getValue() const
{
    return _enum.getInt();
}

bool PropertyEnumeration::isValue(const char* value) const
{
    return _enum.isValue(value);
}

bool PropertyEnumeration::isPartOf(const char* value) const
{
    return _enum.contains(value);
}

const char* PropertyEnumeration::getValueAsString() const
{
    if (!_enum.isValid()) {
        throw Base::RuntimeError("Cannot get value from invalid enumeration");
    }
    return _enum.getCStr();
}

const Enumeration& PropertyEnumeration::getEnum() const
{
    return _enum;
}

std::vector<std::string> PropertyEnumeration::getEnumVector() const
{
    return _enum.getEnumVector();
}

void PropertyEnumeration::setEnumVector(const std::vector<std::string>& values)
{
    // For backward compatibility, if the property container is not attached to
    // any document (i.e. its full name starts with '?'), do not notify, or
    // else existing code may crash.
    bool notify = !boost::starts_with(getFullName(), "?");
    if (notify) {
        aboutToSetValue();
    }
    _enum.setEnums(values);
    if (notify) {
        hasSetValue();
    }
}

bool PropertyEnumeration::hasEnums() const
{
    return _enum.hasEnums();
}

bool PropertyEnumeration::isValid() const
{
    return _enum.isValid();
}

void PropertyEnumeration::Save(Base::Writer& writer) const
{
    writer.Stream() << writer.ind() << "<Integer value=\"" << _enum.getInt() << "\"";
    if (_enum.isCustom()) {
        writer.Stream() << " CustomEnum=\"true\"";
    }
    writer.Stream() << "/>" << std::endl;
    if (_enum.isCustom()) {
        std::vector<std::string> items = getEnumVector();
        writer.Stream() << writer.ind() << "<CustomEnumList count=\"" << items.size() << "\">"
                        << std::endl;
        writer.incInd();
        for (auto& item : items) {
            std::string val = encodeAttribute(item);
            writer.Stream() << writer.ind() << "<Enum value=\"" << val << "\"/>" << std::endl;
        }
        writer.decInd();
        writer.Stream() << writer.ind() << "</CustomEnumList>" << std::endl;
    }
}

void PropertyEnumeration::Restore(Base::XMLReader& reader)
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

        for (int i = 0; i < count; i++) {
            reader.readElement("Enum");
            values[i] = reader.getAttribute("value");
        }

        reader.readEndElement("CustomEnumList");

        _enum.setEnums(values);
    }

    if (val < 0) {
        // If the enum is empty at this stage do not print a warning
        if (_enum.hasEnums()) {
            Base::Console().DeveloperWarning(std::string("PropertyEnumeration"),
                                             "Enumeration index %d is out of range, ignore it\n",
                                             val);
        }
        val = getValue();
    }

    _enum.setValue(val);
    hasSetValue();
}

PyObject* PropertyEnumeration::getPyObject()
{
    if (!_enum.isValid()) {
        Py_Return;
    }

    return Py_BuildValue("s", getValueAsString());
}

void PropertyEnumeration::setPyObject(PyObject* value)
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

            aboutToSetValue();
            _enum.setEnums(values);
            if (idx >= 0) {
                _enum.setValue(idx, true);
            }
            hasSetValue();
            return;
        }
        catch (Py::Exception&) {
            Base::PyException e;
            e.ReportException();
        }
    }

    FC_THROWM(Base::TypeError,
              "PropertyEnumeration "
                  << getFullName()
                  << " expects type to be int, string, or list(string), or list(list, int)");
}

Property* PropertyEnumeration::Copy() const
{
    return new PropertyEnumeration(_enum);
}

void PropertyEnumeration::Paste(const Property& from)
{
    const PropertyEnumeration& prop = dynamic_cast<const PropertyEnumeration&>(from);
    setValue(prop._enum);
}

void PropertyEnumeration::setPathValue(const ObjectIdentifier&, const boost::any& value)
{
    if (value.type() == typeid(int)) {
        setValue(boost::any_cast<int>(value));
    }
    else if (value.type() == typeid(long)) {
        setValue(boost::any_cast<long>(value));
    }
    else if (value.type() == typeid(double)) {
        setValue(boost::any_cast<double>(value));
    }
    else if (value.type() == typeid(float)) {
        setValue(boost::any_cast<float>(value));
    }
    else if (value.type() == typeid(short)) {
        setValue(boost::any_cast<short>(value));
    }
    else if (value.type() == typeid(std::string)) {
        setValue(boost::any_cast<std::string>(value).c_str());
    }
    else if (value.type() == typeid(char*)) {
        setValue(boost::any_cast<char*>(value));
    }
    else if (value.type() == typeid(const char*)) {
        setValue(boost::any_cast<const char*>(value));
    }
    else {
        Base::PyGILStateLocker lock;
        Py::Object pyValue = pyObjectFromAny(value);
        setPyObject(pyValue.ptr());
    }
}

bool PropertyEnumeration::setPyPathValue(const ObjectIdentifier&, const Py::Object& value)
{
    setPyObject(value.ptr());
    return true;
}

const boost::any PropertyEnumeration::getPathValue(const ObjectIdentifier& path) const
{
    std::string p = path.getSubPathStr();
    if (p == ".Enum" || p == ".All") {
        Base::PyGILStateLocker lock;
        Py::Object res;
        getPyPathValue(path, res);
        return pyObjectToAny(res, false);
    }
    else if (p == ".String") {
        auto v = getValueAsString();
        return std::string(v ? v : "");
    }
    else {
        return getValue();
    }
}

bool PropertyEnumeration::getPyPathValue(const ObjectIdentifier& path, Py::Object& r) const
{
    std::string p = path.getSubPathStr();
    if (p == ".Enum" || p == ".All") {
        Base::PyGILStateLocker lock;
        Py::Tuple res(_enum.maxValue() + 1);
        std::vector<std::string> enums = _enum.getEnumVector();
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
            tuple.setItem(1, Py::Long(getValue()));
            r = tuple;
        }
    }
    else if (p == ".String") {
        auto v = getValueAsString();
        r = Py::String(v ? v : "");
    }
    else {
        r = Py::Long(getValue());
    }
    return true;
}

}  // namespace App
