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

#include <Base/Matrix.h>
#include <Base/MatrixPy.h>
#include <Base/PlacementPy.h>
#include <Base/Quantity.h>
#include <Base/QuantityPy.h>
#include <Base/Reader.h>
#include <Base/Tools.h>
#include <Base/Writer.h>

#include "DocumentObject.h"
#include "ObjectIdentifier.h"
#include "Placement.h"
#include "PropertyPlacement.h"


namespace App {

TYPESYSTEM_SOURCE(App::PropertyPlacement, App::Property)

PropertyPlacement::PropertyPlacement() = default;


PropertyPlacement::~PropertyPlacement() = default;

//**************************************************************************
// Base class implementer


void PropertyPlacement::setValue(const Base::Placement& pos)
{
    aboutToSetValue();
    _cPos = pos;
    hasSetValue();
}

bool PropertyPlacement::setValueIfChanged(const Base::Placement& pos, double tol, double atol)
{
    if (_cPos.getPosition().IsEqual(pos.getPosition(), tol)
        && _cPos.getRotation().isSame(pos.getRotation(), atol)) {
        return false;
    }
    setValue(pos);
    return true;
}


const Base::Placement& PropertyPlacement::getValue() const
{
    return _cPos;
}

void PropertyPlacement::getPaths(std::vector<ObjectIdentifier>& paths) const
{
    paths.push_back(ObjectIdentifier(*this)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Base"))
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("x")));
    paths.push_back(ObjectIdentifier(*this)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Base"))
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("y")));
    paths.push_back(ObjectIdentifier(*this)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Base"))
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("z")));
    paths.push_back(ObjectIdentifier(*this)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Rotation"))
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Angle")));
    paths.push_back(ObjectIdentifier(*this)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Rotation"))
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Axis"))
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("x")));
    paths.push_back(ObjectIdentifier(*this)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Rotation"))
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Axis"))
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("y")));
    paths.push_back(ObjectIdentifier(*this)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Rotation"))
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Axis"))
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("z")));
}

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

void PropertyPlacement::setPathValue(const ObjectIdentifier& path, const boost::any& value)
{
    auto updateAxis = [this](int index, double coord) {
        Base::Vector3d axis;
        double angle;
        Base::Vector3d base = _cPos.getPosition();
        Base::Rotation rot = _cPos.getRotation();
        rot.getRawValue(axis, angle);
        axis[index] = coord;
        rot.setValue(axis, angle);
        Base::Placement plm(base, rot);
        setValue(plm);
    };

    auto updateYawPitchRoll = [this](int index, double angle) {
        Base::Vector3d base = _cPos.getPosition();
        Base::Rotation rot = _cPos.getRotation();
        double yaw, pitch, roll;
        rot.getYawPitchRoll(yaw, pitch, roll);
        if (index == 0) {
            if (angle < -180.0 || angle > 180.0) {
                throw Base::ValueError("Yaw angle is out of range [-180, +180]");
            }
            yaw = angle;
        }
        else if (index == 1) {
            if (angle < -90.0 || angle > 90.0) {
                throw Base::ValueError("Pitch angle is out of range [-90, +90]");
            }
            pitch = angle;
        }
        else if (index == 2) {
            if (angle < -180.0 || angle > 180.0) {
                throw Base::ValueError("Roll angle is out of range [-180, +180]");
            }
            roll = angle;
        }
        rot.setYawPitchRoll(yaw, pitch, roll);
        Base::Placement plm(base, rot);
        setValue(plm);
    };

    std::string subpath = path.getSubPathStr();
    if (subpath == ".Rotation.Angle") {
        double avalue = toDouble(value);
        Property::setPathValue(path, Base::toRadians(avalue));
    }
    else if (subpath == ".Rotation.Axis.x") {
        updateAxis(0, toDouble(value));
    }
    else if (subpath == ".Rotation.Axis.y") {
        updateAxis(1, toDouble(value));
    }
    else if (subpath == ".Rotation.Axis.z") {
        updateAxis(2, toDouble(value));
    }
    else if (subpath == ".Rotation.Yaw") {
        updateYawPitchRoll(0, toDouble(value));
    }
    else if (subpath == ".Rotation.Pitch") {
        updateYawPitchRoll(1, toDouble(value));
    }
    else if (subpath == ".Rotation.Roll") {
        updateYawPitchRoll(2, toDouble(value));
    }
    else {
        Property::setPathValue(path, value);
    }
}

const boost::any PropertyPlacement::getPathValue(const ObjectIdentifier& path) const
{
    auto getAxis = [](const Base::Placement& plm) {
        Base::Vector3d axis;
        double angle;
        const Base::Rotation& rot = plm.getRotation();
        rot.getRawValue(axis, angle);
        return axis;
    };

    auto getYawPitchRoll = [](const Base::Placement& plm) {
        Base::Vector3d ypr;
        const Base::Rotation& rot = plm.getRotation();
        rot.getYawPitchRoll(ypr.x, ypr.y, ypr.z);
        return ypr;
    };

    std::string p = path.getSubPathStr();

    if (p == ".Rotation.Angle") {
        // Convert angle to degrees
        return Base::Quantity(
            Base::toDegrees(boost::any_cast<double>(Property::getPathValue(path))),
            Base::Unit::Angle);
    }
    else if (p == ".Base.x" || p == ".Base.y" || p == ".Base.z") {
        // Convert double to quantity
        return Base::Quantity(boost::any_cast<double>(Property::getPathValue(path)), Base::Unit::Length);
    }
    else if (p == ".Rotation.Axis.x") {
        return getAxis(_cPos).x;
    }
    else if (p == ".Rotation.Axis.y") {
        return getAxis(_cPos).y;
    }
    else if (p == ".Rotation.Axis.z") {
        return getAxis(_cPos).z;
    }
    else if (p == ".Rotation.Yaw") {
        return getYawPitchRoll(_cPos).x;
    }
    else if (p == ".Rotation.Pitch") {
        return getYawPitchRoll(_cPos).y;
    }
    else if (p == ".Rotation.Roll") {
        return getYawPitchRoll(_cPos).z;
    }
    else {
        return Property::getPathValue(path);
    }
}

bool PropertyPlacement::getPyPathValue(const ObjectIdentifier& path, Py::Object& res) const
{
    auto getAxis = [](const Base::Placement& plm) {
        Base::Vector3d axis;
        double angle;
        const Base::Rotation& rot = plm.getRotation();
        rot.getRawValue(axis, angle);
        return axis;
    };

    auto getYawPitchRoll = [](const Base::Placement& plm) {
        Base::Vector3d ypr;
        const Base::Rotation& rot = plm.getRotation();
        rot.getYawPitchRoll(ypr.x, ypr.y, ypr.z);
        return ypr;
    };

    std::string p = path.getSubPathStr();
    if (p == ".Rotation.Angle") {
        Base::Vector3d axis;
        double angle;
        _cPos.getRotation().getValue(axis, angle);
        res = Py::asObject(new Base::QuantityPy(new Base::Quantity(Base::toDegrees(angle), Base::Unit::Angle)));
        return true;
    }
    else if (p == ".Base.x") {
        res = Py::asObject(new Base::QuantityPy(new Base::Quantity(_cPos.getPosition().x, Base::Unit::Length)));
        return true;
    }
    else if (p == ".Base.y") {
        res = Py::asObject(new Base::QuantityPy(new Base::Quantity(_cPos.getPosition().y, Base::Unit::Length)));
        return true;
    }
    else if (p == ".Base.z") {
        res = Py::asObject(new Base::QuantityPy(new Base::Quantity(_cPos.getPosition().z, Base::Unit::Length)));
        return true;
    }
    else if (p == ".Rotation.Axis.x") {
        res = Py::Float(getAxis(_cPos).x);
        return true;
    }
    else if (p == ".Rotation.Axis.y") {
        res = Py::Float(getAxis(_cPos).y);
        return true;
    }
    else if (p == ".Rotation.Axis.z") {
        res = Py::Float(getAxis(_cPos).z);
        return true;
    }
    else if (p == ".Rotation.Yaw") {
        res = Py::Float(getYawPitchRoll(_cPos).x);
        return true;
    }
    else if (p == ".Rotation.Pitch") {
        res = Py::Float(getYawPitchRoll(_cPos).y);
        return true;
    }
    else if (p == ".Rotation.Roll") {
        res = Py::Float(getYawPitchRoll(_cPos).z);
        return true;
    }

    return false;
}

PyObject* PropertyPlacement::getPyObject()
{
    return new Base::PlacementPy(new Base::Placement(_cPos));
}

void PropertyPlacement::setPyObject(PyObject* value)
{
    if (PyObject_TypeCheck(value, &(Base::MatrixPy::Type))) {
        Base::MatrixPy* pcObject = static_cast<Base::MatrixPy*>(value);
        Base::Matrix4D mat = pcObject->value();
        Base::Placement p;
        p.fromMatrix(mat);
        setValue(p);
    }
    else if (PyObject_TypeCheck(value, &(Base::PlacementPy::Type))) {
        setValue(*static_cast<Base::PlacementPy*>(value)->getPlacementPtr());
    }
    else {
        std::string error = std::string("type must be 'Matrix' or 'Placement', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyPlacement::Save(Base::Writer& writer) const
{
    // clang-format off
    writer.Stream() << writer.ind() << "<PropertyPlacement";
    writer.Stream() << " Px=\"" << _cPos.getPosition().x << "\""
                    << " Py=\"" << _cPos.getPosition().y << "\""
                    << " Pz=\"" << _cPos.getPosition().z << "\"";

    writer.Stream() << " Q0=\"" << _cPos.getRotation()[0] << "\""
                    << " Q1=\"" << _cPos.getRotation()[1] << "\""
                    << " Q2=\"" << _cPos.getRotation()[2] << "\""
                    << " Q3=\"" << _cPos.getRotation()[3] << "\"";
    Base::Vector3d axis;
    double rfAngle {};
    _cPos.getRotation().getRawValue(axis, rfAngle);
    writer.Stream() << " A=\"" << rfAngle << "\""
                    << " Ox=\"" << axis.x << "\""
                    << " Oy=\"" << axis.y << "\""
                    << " Oz=\"" << axis.z << "\"";
    writer.Stream() << "/>" << std::endl;
    // clang-format on
}

void PropertyPlacement::Restore(Base::XMLReader& reader)
{
    // read my Element
    reader.readElement("PropertyPlacement");
    // get the value of my Attribute
    aboutToSetValue();

    if (reader.hasAttribute("A")) {
        _cPos = Base::Placement(Base::Vector3d(reader.getAttributeAsFloat("Px"),
                                               reader.getAttributeAsFloat("Py"),
                                               reader.getAttributeAsFloat("Pz")),
                                Base::Rotation(Base::Vector3d(reader.getAttributeAsFloat("Ox"),
                                                              reader.getAttributeAsFloat("Oy"),
                                                              reader.getAttributeAsFloat("Oz")),
                                                              reader.getAttributeAsFloat("A")));
    }
    else {
        _cPos = Base::Placement(Base::Vector3d(reader.getAttributeAsFloat("Px"),
                                               reader.getAttributeAsFloat("Py"),
                                               reader.getAttributeAsFloat("Pz")),
                                Base::Rotation(reader.getAttributeAsFloat("Q0"),
                                               reader.getAttributeAsFloat("Q1"),
                                               reader.getAttributeAsFloat("Q2"),
                                               reader.getAttributeAsFloat("Q3")));
    }

    hasSetValue();
}


Property* PropertyPlacement::Copy() const
{
    PropertyPlacement* p = new PropertyPlacement();
    p->_cPos = _cPos;
    return p;
}

void PropertyPlacement::Paste(const Property& from)
{
    aboutToSetValue();
    _cPos = dynamic_cast<const PropertyPlacement&>(from)._cPos;
    hasSetValue();
}


//**************************************************************************
//**************************************************************************
// PropertyPlacement
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyPlacementLink, App::PropertyLink)

//**************************************************************************
// Construction/Destruction


PropertyPlacementLink::PropertyPlacementLink() = default;


PropertyPlacementLink::~PropertyPlacementLink() = default;

App::Placement* PropertyPlacementLink::getPlacementObject() const
{
    if (_pcLink->isDerivedFrom<App::Placement>()) {
        return dynamic_cast<App::Placement*>(_pcLink);
    }
    else {
        return nullptr;
    }
}

//**************************************************************************
// Base class implementer

Property* PropertyPlacementLink::Copy() const
{
    PropertyPlacementLink* p = new PropertyPlacementLink();
    p->_pcLink = _pcLink;
    return p;
}

void PropertyPlacementLink::Paste(const Property& from)
{
    aboutToSetValue();
    _pcLink = dynamic_cast<const PropertyPlacementLink&>(from)._pcLink;
    hasSetValue();
}

}  // namespace App
