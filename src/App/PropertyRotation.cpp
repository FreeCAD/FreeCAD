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

#include <boost/any.hpp>

#include <Base/MatrixPy.h>
#include <Base/PlacementPy.h>
#include <Base/Reader.h>
#include <Base/RotationPy.h>
#include <Base/Tools.h>
#include <Base/QuantityPy.h>
#include <Base/UnitsApi.h>
#include <Base/Writer.h>

#include "ObjectIdentifier.h"
#include "PropertyRotation.h"



namespace
{
double toDouble(const boost::any& value)
{
    double avalue {};

    if (value.type() == typeid(Base::Quantity)) {
        avalue = boost::any_cast<Base::Quantity>(value).getValue();
    }
    else if (value.type() == typeid(double)) {
        avalue = boost::any_cast<double>(value);
    }
    else if (value.type() == typeid(int)) {
        avalue = boost::any_cast<int>(value);
    }
    else if (value.type() == typeid(unsigned int)) {
        avalue = boost::any_cast<unsigned int>(value);
    }
    else if (value.type() == typeid(short)) {
        avalue = boost::any_cast<short>(value);
    }
    else if (value.type() == typeid(unsigned short)) {
        avalue = boost::any_cast<unsigned short>(value);
    }
    else if (value.type() == typeid(long)) {
        avalue = boost::any_cast<long>(value);
    }
    else if (value.type() == typeid(unsigned long)) {
        avalue = boost::any_cast<unsigned long>(value);
    }
    else {
        throw std::bad_cast();
    }
    return avalue;
}
}  // namespace

namespace App {

TYPESYSTEM_SOURCE(App::PropertyRotation, App::Property)

PropertyRotation::PropertyRotation() = default;


PropertyRotation::~PropertyRotation() = default;

void PropertyRotation::setValue(const Base::Rotation& rot)
{
    aboutToSetValue();
    _rot = rot;
    hasSetValue();
}

bool PropertyRotation::setValueIfChanged(const Base::Rotation& rot, double atol)
{
    if (_rot.isSame(rot, atol)) {
        return false;
    }

    setValue(rot);
    return true;
}


const Base::Rotation& PropertyRotation::getValue() const
{
    return _rot;
}

void PropertyRotation::getPaths(std::vector<ObjectIdentifier>& paths) const
{
    paths.push_back(ObjectIdentifier(*this)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Angle")));
    paths.push_back(ObjectIdentifier(*this)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Axis"))
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("x")));
    paths.push_back(ObjectIdentifier(*this)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Axis"))
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("y")));
    paths.push_back(ObjectIdentifier(*this)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Axis"))
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("z")));
}

void PropertyRotation::setPathValue(const ObjectIdentifier& path, const boost::any& value)
{
    auto updateAxis = [this](int index, double coord) {
        Base::Vector3d axis;
        double angle;
        _rot.getRawValue(axis, angle);

        axis[index] = coord;
        setValue(Base::Rotation {axis, angle});
    };

    std::string subpath = path.getSubPathStr();
    if (subpath == ".Angle") {
        double avalue = toDouble(value);
        Property::setPathValue(path, Base::toRadians(avalue));
    }
    else if (subpath == ".Axis.x") {
        updateAxis(0, toDouble(value));
    }
    else if (subpath == ".Axis.y") {
        updateAxis(1, toDouble(value));
    }
    else if (subpath == ".Axis.z") {
        updateAxis(2, toDouble(value));
    }
    else {
        Property::setPathValue(path, value);
    }
}

const boost::any PropertyRotation::getPathValue(const ObjectIdentifier& path) const
{
    auto getAxis = [](const Base::Rotation& rot) {
        Base::Vector3d axis;
        double angle;
        rot.getRawValue(axis, angle);
        return axis;
    };
    std::string p = path.getSubPathStr();

    if (p == ".Angle") {
        // Convert angle to degrees
        return Base::Quantity(
            Base::toDegrees(boost::any_cast<double>(Property::getPathValue(path))),
            Base::Unit::Angle);
    }
    else if (p == ".Axis.x") {
        return getAxis(_rot).x;
    }
    else if (p == ".Axis.y") {
        return getAxis(_rot).y;
    }
    else if (p == ".Axis.z") {
        return getAxis(_rot).z;
    }
    else {
        return Property::getPathValue(path);
    }
}

bool PropertyRotation::getPyPathValue(const ObjectIdentifier& path, Py::Object& res) const
{
    auto getAxis = [](const Base::Rotation& rot) {
        Base::Vector3d axis;
        double angle;
        rot.getRawValue(axis, angle);
        return axis;
    };

    std::string p = path.getSubPathStr();
    if (p == ".Angle") {
        Base::Vector3d axis;
        double angle;
        _rot.getValue(axis, angle);
        res = Py::asObject(new Base::QuantityPy(new Base::Quantity(Base::toDegrees(angle), Base::Unit::Angle)));
        return true;
    }
    else if (p == ".Axis.x") {
        res = Py::Float(getAxis(_rot).x);
        return true;
    }
    else if (p == ".Axis.y") {
        res = Py::Float(getAxis(_rot).y);
        return true;
    }
    else if (p == ".Axis.z") {
        res = Py::Float(getAxis(_rot).z);
        return true;
    }

    return false;
}

PyObject* PropertyRotation::getPyObject()
{
    return new Base::RotationPy(new Base::Rotation(_rot));
}

void PropertyRotation::setPyObject(PyObject* value)
{
    if (PyObject_TypeCheck(value, &(Base::MatrixPy::Type))) {
        Base::MatrixPy* object = static_cast<Base::MatrixPy*>(value);
        Base::Matrix4D mat = object->value();
        Base::Rotation p;
        p.setValue(mat);
        setValue(p);
    }
    else if (PyObject_TypeCheck(value, &(Base::RotationPy::Type))) {
        setValue(*static_cast<Base::RotationPy*>(value)->getRotationPtr());
    }
    else {
        std::string error = std::string("type must be 'Matrix' or 'Rotation', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyRotation::Save(Base::Writer& writer) const
{
    Base::Vector3d axis;
    double rfAngle {};
    _rot.getRawValue(axis, rfAngle);

    writer.Stream() << writer.ind() << "<PropertyRotation";
    writer.Stream() << " A=\"" << rfAngle << "\""
                    << " Ox=\"" << axis.x << "\""
                    << " Oy=\"" << axis.y << "\""
                    << " Oz=\"" << axis.z << "\""
                    << "/>\n";
}

void PropertyRotation::Restore(Base::XMLReader& reader)
{
    reader.readElement("PropertyRotation");
    aboutToSetValue();

    _rot = Base::Rotation(Base::Vector3d(reader.getAttributeAsFloat("Ox"),
                                         reader.getAttributeAsFloat("Oy"),
                                         reader.getAttributeAsFloat("Oz")),
                    reader.getAttributeAsFloat("A"));
    hasSetValue();
}

Property* PropertyRotation::Copy() const
{
    PropertyRotation* p = new PropertyRotation();
    p->_rot = _rot;
    return p;
}

void PropertyRotation::Paste(const Property& from)
{
    aboutToSetValue();
    _rot = dynamic_cast<const PropertyRotation&>(from)._rot;
    hasSetValue();
}

}  // namespace App
