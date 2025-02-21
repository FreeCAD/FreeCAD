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

#include <boost/math/special_functions/round.hpp>

#include <Base/PyObjectBase.h>
#include <Base/Quantity.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include "PropertyBool.h"


namespace App {

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
    aboutToSetValue();
    _lValue = lValue;
    hasSetValue();
}

bool PropertyBool::getValue() const
{
    return _lValue;
}

PyObject* PropertyBool::getPyObject()
{
    return PyBool_FromLong(_lValue ? 1 : 0);
}

void PropertyBool::setPyObject(PyObject* value)
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

void PropertyBool::Save(Base::Writer& writer) const
{
    writer.Stream() << writer.ind() << "<Bool value=\"";
    if (_lValue) {
        writer.Stream() << "true" << "\"/>";
    }
    else {
        writer.Stream() << "false" << "\"/>";
    }
    writer.Stream() << std::endl;
}

void PropertyBool::Restore(Base::XMLReader& reader)
{
    // read my Element
    reader.readElement("Bool");
    // get the value of my Attribute
    std::string b = reader.getAttribute("value");
    (b == "true") ? setValue(true) : setValue(false);
}


Property* PropertyBool::Copy() const
{
    PropertyBool* p = new PropertyBool();
    p->_lValue = _lValue;
    return p;
}

void PropertyBool::Paste(const Property& from)
{
    aboutToSetValue();
    _lValue = dynamic_cast<const PropertyBool&>(from)._lValue;
    hasSetValue();
}

void PropertyBool::setPathValue(const ObjectIdentifier& path, const boost::any& value)
{
    verifyPath(path);

    if (value.type() == typeid(bool)) {
        setValue(boost::any_cast<bool>(value));
    }
    else if (value.type() == typeid(int)) {
        setValue(boost::any_cast<int>(value) != 0);
    }
    else if (value.type() == typeid(long)) {
        setValue(boost::any_cast<long>(value) != 0);
    }
    else if (value.type() == typeid(double)) {
        setValue(boost::math::round(boost::any_cast<double>(value)));
    }
    else if (value.type() == typeid(float)) {
        setValue(boost::math::round(boost::any_cast<float>(value)));
    }
    else if (value.type() == typeid(Base::Quantity)) {
        setValue(boost::any_cast<Base::Quantity>(value).getValue() != 0);
    }
    else {
        throw std::bad_cast();
    }
}

const boost::any PropertyBool::getPathValue(const ObjectIdentifier& path) const
{
    verifyPath(path);

    return _lValue;
}

}  // namespace App
