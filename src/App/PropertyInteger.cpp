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

#include <boost/math/special_functions/round.hpp>

#include <Base/PyObjectBase.h>
#include <Base/Quantity.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include "PropertyInteger.h"


namespace App {

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
    aboutToSetValue();
    _lValue = lValue;
    hasSetValue();
}

long PropertyInteger::getValue() const
{
    return _lValue;
}

PyObject* PropertyInteger::getPyObject()
{
    return Py_BuildValue("l", _lValue);
}

void PropertyInteger::setPyObject(PyObject* value)
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

void PropertyInteger::Save(Base::Writer& writer) const
{
    writer.Stream() << writer.ind() << "<Integer value=\"" << _lValue << "\"/>" << std::endl;
}

void PropertyInteger::Restore(Base::XMLReader& reader)
{
    // read my Element
    reader.readElement("Integer");
    // get the value of my Attribute
    setValue(reader.getAttributeAsInteger("value"));
}

Property* PropertyInteger::Copy() const
{
    PropertyInteger* p = new PropertyInteger();
    p->_lValue = _lValue;
    return p;
}

void PropertyInteger::Paste(const Property& from)
{
    aboutToSetValue();
    _lValue = dynamic_cast<const PropertyInteger&>(from)._lValue;
    hasSetValue();
}

void PropertyInteger::setPathValue(const ObjectIdentifier& path, const boost::any& value)
{
    verifyPath(path);

    if (value.type() == typeid(long)) {
        setValue(boost::any_cast<long>(value));
    }
    else if (value.type() == typeid(int)) {
        setValue(boost::any_cast<int>(value));
    }
    else if (value.type() == typeid(double)) {
        setValue(boost::math::round(boost::any_cast<double>(value)));
    }
    else if (value.type() == typeid(float)) {
        setValue(boost::math::round(boost::any_cast<float>(value)));
    }
    else if (value.type() == typeid(Base::Quantity)) {
        setValue(boost::math::round(boost::any_cast<Base::Quantity>(value).getValue()));
    }
    else {
        throw std::bad_cast();
    }
}

}  // namespace App