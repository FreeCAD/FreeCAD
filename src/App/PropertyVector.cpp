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

#include <Base/QuantityPy.h>
#include <Base/Reader.h>
#include <Base/VectorPy.h>
#include <Base/Writer.h>

#include "ObjectIdentifier.h"
#include "PropertyVector.h"

namespace App {

TYPESYSTEM_SOURCE(App::PropertyVector, App::Property)

//**************************************************************************
// Construction/Destruction


PropertyVector::PropertyVector() = default;

PropertyVector::~PropertyVector() = default;

//**************************************************************************
// Base class implementer

void PropertyVector::setValue(const Base::Vector3d& vec)
{
    aboutToSetValue();
    _cVec = vec;
    hasSetValue();
}

void PropertyVector::setValue(double x, double y, double z)
{
    aboutToSetValue();
    _cVec.Set(x, y, z);
    hasSetValue();
}

const Base::Vector3d& PropertyVector::getValue() const
{
    return _cVec;
}

PyObject* PropertyVector::getPyObject()
{
    return new Base::VectorPy(_cVec);
}

void PropertyVector::setPyObject(PyObject* value)
{
    if (PyObject_TypeCheck(value, &(Base::VectorPy::Type))) {
        Base::VectorPy* pcObject = static_cast<Base::VectorPy*>(value);
        Base::Vector3d* val = pcObject->getVectorPtr();
        setValue(*val);
    }
    else if (PyTuple_Check(value) && PyTuple_Size(value) == 3) {
        PyObject* item {};
        Base::Vector3d cVec;
        // x
        item = PyTuple_GetItem(value, 0);
        if (PyFloat_Check(item)) {
            cVec.x = PyFloat_AsDouble(item);
        }
        else if (PyLong_Check(item)) {
            cVec.x = (double)PyLong_AsLong(item);
        }
        else {
            throw Base::TypeError("Not allowed type used in tuple (float expected)...");
        }
        // y
        item = PyTuple_GetItem(value, 1);
        if (PyFloat_Check(item)) {
            cVec.y = PyFloat_AsDouble(item);
        }
        else if (PyLong_Check(item)) {
            cVec.y = (double)PyLong_AsLong(item);
        }
        else {
            throw Base::TypeError("Not allowed type used in tuple (float expected)...");
        }
        // z
        item = PyTuple_GetItem(value, 2);
        if (PyFloat_Check(item)) {
            cVec.z = PyFloat_AsDouble(item);
        }
        else if (PyLong_Check(item)) {
            cVec.z = (double)PyLong_AsLong(item);
        }
        else {
            throw Base::TypeError("Not allowed type used in tuple (float expected)...");
        }
        setValue(cVec);
    }
    else {
        std::string error = std::string("type must be 'Vector' or tuple of three floats, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyVector::Save(Base::Writer& writer) const
{
    // clang-format off
    writer.Stream() << writer.ind()
                    << "<PropertyVector"
                    << " valueX=\"" << _cVec.x << "\""
                    << " valueY=\"" << _cVec.y << "\""
                    << " valueZ=\"" << _cVec.z << "\""
                    << "/>\n";
    // clang-format on
}

void PropertyVector::Restore(Base::XMLReader& reader)
{
    // read my Element
    reader.readElement("PropertyVector");
    // get the value of my Attribute
    aboutToSetValue();
    _cVec.x = reader.getAttributeAsFloat("valueX");
    _cVec.y = reader.getAttributeAsFloat("valueY");
    _cVec.z = reader.getAttributeAsFloat("valueZ");
    hasSetValue();
}


Property* PropertyVector::Copy() const
{
    PropertyVector* p = new PropertyVector();
    p->_cVec = _cVec;
    return p;
}

void PropertyVector::Paste(const Property& from)
{
    aboutToSetValue();
    _cVec = dynamic_cast<const PropertyVector&>(from)._cVec;
    hasSetValue();
}

void PropertyVector::getPaths(std::vector<ObjectIdentifier>& paths) const
{
    paths.push_back(ObjectIdentifier(*this)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("x")));
    paths.push_back(ObjectIdentifier(*this)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("y")));
    paths.push_back(ObjectIdentifier(*this)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("z")));
}

const boost::any PropertyVector::getPathValue(const ObjectIdentifier& path) const
{
    Base::Unit unit = getUnit();
    if (!unit.isEmpty()) {
        std::string p = path.getSubPathStr();
        if (p == ".x" || p == ".y" || p == ".z") {
            // Convert double to quantity
            return Base::Quantity(boost::any_cast<double>(Property::getPathValue(path)), unit);
        }
    }
    return Property::getPathValue(path);
}

bool PropertyVector::getPyPathValue(const ObjectIdentifier& path, Py::Object& res) const
{
    Base::Unit unit = getUnit();
    if (unit.isEmpty()) {
        return false;
    }

    std::string p = path.getSubPathStr();
    if (p == ".x") {
        res = Py::asObject(new Base::QuantityPy(new Base::Quantity(getValue().x, unit)));
    }
    else if (p == ".y") {
        res = Py::asObject(new Base::QuantityPy(new Base::Quantity(getValue().y, unit)));
    }
    else if (p == ".z") {
        res = Py::asObject(new Base::QuantityPy(new Base::Quantity(getValue().z, unit)));
    }
    else {
        return false;
    }
    return true;
}


//**************************************************************************
// PropertyVectorDistance
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyVectorDistance, App::PropertyVector)

//**************************************************************************
// Construction/Destruction


PropertyVectorDistance::PropertyVectorDistance() = default;

PropertyVectorDistance::~PropertyVectorDistance() = default;

//**************************************************************************
// PropertyPosition
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyPosition, App::PropertyVector)

//**************************************************************************
// Construction/Destruction


PropertyPosition::PropertyPosition() = default;

PropertyPosition::~PropertyPosition() = default;

//**************************************************************************
// PropertyPosition
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyDirection, App::PropertyVector)

//**************************************************************************
// Construction/Destruction


PropertyDirection::PropertyDirection() = default;

PropertyDirection::~PropertyDirection() = default;

}  // namespace App
