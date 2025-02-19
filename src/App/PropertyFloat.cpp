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

#include <Base/PyObjectBase.h>
#include <Base/Quantity.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include "PropertyFloat.h"


namespace App {

TYPESYSTEM_SOURCE(App::PropertyFloat, App::Property)

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
    _dValue = lValue;
    hasSetValue();
}

double PropertyFloat::getValue() const
{
    return _dValue;
}

PyObject* PropertyFloat::getPyObject()
{
    return Py_BuildValue("d", _dValue);
}

void PropertyFloat::setPyObject(PyObject* value)
{
    if (PyFloat_Check(value)) {
        aboutToSetValue();
        _dValue = PyFloat_AsDouble(value);
        hasSetValue();
    }
    else if (PyLong_Check(value)) {
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

void PropertyFloat::Save(Base::Writer& writer) const
{
    writer.Stream() << writer.ind() << "<Float value=\"" << _dValue << "\"/>" << std::endl;
}

void PropertyFloat::Restore(Base::XMLReader& reader)
{
    // read my Element
    reader.readElement("Float");
    // get the value of my Attribute
    setValue(reader.getAttributeAsFloat("value"));
}

Property* PropertyFloat::Copy() const
{
    PropertyFloat* p = new PropertyFloat();
    p->_dValue = _dValue;
    return p;
}

void PropertyFloat::Paste(const Property& from)
{
    aboutToSetValue();
    _dValue = dynamic_cast<const PropertyFloat&>(from)._dValue;
    hasSetValue();
}

void PropertyFloat::setPathValue(const ObjectIdentifier& path, const boost::any& value)
{
    verifyPath(path);

    if (value.type() == typeid(long)) {
        setValue(boost::any_cast<long>(value));
    }
    else if (value.type() == typeid(unsigned long)) {
        setValue(boost::any_cast<unsigned long>(value));
    }
    else if (value.type() == typeid(int)) {
        setValue(boost::any_cast<int>(value));
    }
    else if (value.type() == typeid(double)) {
        setValue(boost::any_cast<double>(value));
    }
    else if (value.type() == typeid(float)) {
        setValue(boost::any_cast<float>(value));
    }
    else if (value.type() == typeid(Base::Quantity)) {
        setValue((boost::any_cast<Base::Quantity>(value)).getValue());
    }
    else {
        throw std::bad_cast();
    }
}

const boost::any PropertyFloat::getPathValue(const ObjectIdentifier& path) const
{
    verifyPath(path);
    return _dValue;
}

}  // namespace App
