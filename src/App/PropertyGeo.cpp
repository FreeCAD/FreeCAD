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

#include <Base/MatrixPy.h>
#include <Base/PlacementPy.h>
#include <Base/Reader.h>

#include <Base/Quantity.h>
#include <Base/QuantityPy.h>
#include <Base/Rotation.h>
#include <Base/RotationPy.h>
#include <Base/Stream.h>
#include <Base/Tools.h>
#include <Base/VectorPy.h>
#include <Base/Writer.h>

#include "ComplexGeoData.h"
#include "Document.h"
#include "PropertyGeo.h"
#include "Placement.h"
#include "ObjectIdentifier.h"


using namespace App;
using namespace Base;
using namespace std;


//**************************************************************************
//**************************************************************************
// PropertyVector
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyVector, App::Property)

//**************************************************************************
// Construction/Destruction


PropertyVector::PropertyVector() = default;

PropertyVector::~PropertyVector() = default;

//**************************************************************************
// Base class implementer

void PropertyVector::setValue(const Base::Vector3d& vec)
{
    auto& self = propSetterSelf<App::PropertyVector>(*this);

    self.aboutToSetValue();
    self._cVec = vec;
    self.hasSetValue();
}

void PropertyVector::setValue(double x, double y, double z)
{
    auto& self = propSetterSelf<App::PropertyVector>(*this);

    self.aboutToSetValue();
    self._cVec.Set(x, y, z);
    self.hasSetValue();
}

const Base::Vector3d& PropertyVector::getValue() const
{
    auto& self = propGetterSelf<const App::PropertyVector>(*this);

    return self._cVec;
}

PyObject* PropertyVector::getPyObject()
{
    auto& self = propGetterSelf<const App::PropertyVector>(*this);

    return new Base::VectorPy(self._cVec);
}

void PropertyVector::setPyObject(PyObject* value)
{
    auto& self = propSetterSelf<App::PropertyVector>(*this);

    if (PyObject_TypeCheck(value, &(Base::VectorPy::Type))) {
        Base::VectorPy* pcObject = static_cast<Base::VectorPy*>(value);
        Base::Vector3d* val = pcObject->getVectorPtr();
        self.setValue(*val);
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
        self.setValue(cVec);
    }
    else {
        std::string error = std::string("type must be 'Vector' or tuple of three floats, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyVector::Save(Base::Writer& writer) const
{
    auto& self = propGetterSelf<const App::PropertyVector>(*this);

    // clang-format off
    writer.Stream() << writer.ind()
                    << "<PropertyVector"
                    << " valueX=\"" << self._cVec.x << "\""
                    << " valueY=\"" << self._cVec.y << "\""
                    << " valueZ=\"" << self._cVec.z << "\""
                    << "/>\n";
    // clang-format on
}

void PropertyVector::Restore(Base::XMLReader& reader)
{
    auto& self = propSetterSelf<App::PropertyVector>(*this);

    // read my Element
    reader.readElement("PropertyVector");
    // get the value of my Attribute
    self.aboutToSetValue();
    self._cVec.x = reader.getAttribute<double>("valueX");
    self._cVec.y = reader.getAttribute<double>("valueY");
    self._cVec.z = reader.getAttribute<double>("valueZ");
    self.hasSetValue();
}


Property* PropertyVector::Copy() const
{
    auto& self = propGetterSelf<const App::PropertyVector>(*this);

    PropertyVector* p = new PropertyVector();
    p->_cVec = self._cVec;
    return p;
}

void PropertyVector::Paste(const Property& from)
{
    auto& self = propSetterSelf<App::PropertyVector>(*this);

    self.aboutToSetValue();
    self._cVec = dynamic_cast<const PropertyVector&>(from)._cVec;
    self.hasSetValue();
}

void PropertyVector::getPaths(std::vector<ObjectIdentifier>& paths) const
{
    auto& self = propGetterSelf<const App::PropertyVector>(*this);

    paths.push_back(ObjectIdentifier(self)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("x")));
    paths.push_back(ObjectIdentifier(self)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("y")));
    paths.push_back(ObjectIdentifier(self)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("z")));
}

const boost::any PropertyVector::getPathValue(const ObjectIdentifier& path) const
{
    auto& self = propGetterSelf<const App::PropertyVector>(*this);

    Base::Unit unit = self.getUnit();
    if (unit != Unit::One) {
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
    auto& self = propGetterSelf<const App::PropertyVector>(*this);

    Base::Unit unit = self.getUnit();
    if (unit == Unit::One) {
        return false;
    }

    std::string p = path.getSubPathStr();
    if (p == ".x") {
        res = Py::asObject(new QuantityPy(new Quantity(self.getValue().x, unit)));
    }
    else if (p == ".y") {
        res = Py::asObject(new QuantityPy(new Quantity(self.getValue().y, unit)));
    }
    else if (p == ".z") {
        res = Py::asObject(new QuantityPy(new Quantity(self.getValue().z, unit)));
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

//**************************************************************************
// PropertyVectorList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyVectorList, App::PropertyLists)

//**************************************************************************
// Construction/Destruction

PropertyVectorList::PropertyVectorList() = default;

PropertyVectorList::~PropertyVectorList() = default;

//**************************************************************************
// Base class implementer

void PropertyVectorList::setValue(double x, double y, double z)
{
    auto& self = propSetterSelf<App::PropertyVectorList>(*this);

    self.setValue(Base::Vector3d(x, y, z));
}

PyObject* PropertyVectorList::getPyObject()
{
    auto& self = propGetterSelf<const App::PropertyVectorList>(*this);

    PyObject* list = PyList_New(self.getSize());

    for (int i = 0; i < self.getSize(); i++) {
        PyList_SetItem(list, i, new VectorPy(self._lValueList[i]));
    }

    return list;
}

Base::Vector3d PropertyVectorList::getPyValue(PyObject* item) const
{
    PropertyVector val;
    val.setPyObject(item);
    return val.getValue();
}

void PropertyVectorList::Save(Base::Writer& writer) const
{
    auto& self = propGetterSelf<const App::PropertyVectorList>(*this);

    if (!writer.isForceXML()) {
        writer.Stream() << writer.ind() << "<VectorList file=\"" << writer.addFile(self.getName(), &self)
                        << "\"/>" << std::endl;
    }
}

void PropertyVectorList::Restore(Base::XMLReader& reader)
{
    reader.readElement("VectorList");
    std::string file(reader.getAttribute<const char*>("file"));

    if (!file.empty()) {
        // initiate a file read
        reader.addFile(file.c_str(), this);
    }
}

void PropertyVectorList::SaveDocFile(Base::Writer& writer) const
{
    auto& self = propGetterSelf<const App::PropertyVectorList>(*this);

    Base::OutputStream str(writer.Stream());
    uint32_t uCt = (uint32_t)self.getSize();
    str << uCt;
    if (!self.isSinglePrecision()) {
        for (const auto& it : self._lValueList) {
            str << it.x << it.y << it.z;
        }
    }
    else {
        for (const auto& it : self._lValueList) {
            float x = (float)it.x;
            float y = (float)it.y;
            float z = (float)it.z;
            str << x << y << z;
        }
    }
}

void PropertyVectorList::RestoreDocFile(Base::Reader& reader)
{
    auto& self = propSetterSelf<App::PropertyVectorList>(*this);

    Base::InputStream str(reader);
    uint32_t uCt = 0;
    str >> uCt;
    std::vector<Base::Vector3d> values(uCt);
    if (!self.isSinglePrecision()) {
        for (auto& it : values) {
            str >> it.x >> it.y >> it.z;
        }
    }
    else {
        Base::Vector3f vec;
        for (auto& it : values) {
            str >> vec.x >> vec.y >> vec.z;
            it.Set(vec.x, vec.y, vec.z);
        }
    }
    self.setValues(values);
}

Property* PropertyVectorList::Copy() const
{
    auto& self = propGetterSelf<const App::PropertyVectorList>(*this);

    PropertyVectorList* p = new PropertyVectorList();
    p->_lValueList = self._lValueList;
    return p;
}

void PropertyVectorList::Paste(const Property& from)
{
    auto& self = propSetterSelf<App::PropertyVectorList>(*this);

    self.setValues(dynamic_cast<const PropertyVectorList&>(from)._lValueList);
}

unsigned int PropertyVectorList::getMemSize() const
{
    auto& self = propGetterSelf<const App::PropertyVectorList>(*this);

    return static_cast<unsigned int>(self._lValueList.size() * sizeof(Base::Vector3d));
}

//**************************************************************************
//**************************************************************************
// PropertyMatrix
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyMatrix, App::Property)

//**************************************************************************
// Construction/Destruction


PropertyMatrix::PropertyMatrix() = default;


PropertyMatrix::~PropertyMatrix() = default;

//**************************************************************************
// Base class implementer


void PropertyMatrix::setValue(const Base::Matrix4D& mat)
{
    auto& self = propSetterSelf<App::PropertyMatrix>(*this);

    self.aboutToSetValue();
    self._cMat = mat;
    self.hasSetValue();
}


const Base::Matrix4D& PropertyMatrix::getValue() const
{
    auto& self = propGetterSelf<const App::PropertyMatrix>(*this);

    return self._cMat;
}

PyObject* PropertyMatrix::getPyObject()
{
    auto& self = propGetterSelf<const App::PropertyMatrix>(*this);

    return new Base::MatrixPy(self._cMat);
}

void PropertyMatrix::setPyObject(PyObject* value)
{
    auto& self = propSetterSelf<App::PropertyMatrix>(*this);

    if (PyObject_TypeCheck(value, &(Base::MatrixPy::Type))) {
        Base::MatrixPy* pcObject = static_cast<Base::MatrixPy*>(value);
        self.setValue(pcObject->value());
    }
    else if (PyTuple_Check(value) && PyTuple_Size(value) == 16) {
        PyObject* item;
        Base::Matrix4D cMatrix;

        const int dim = 4;
        for (int x = 0; x < dim; x++) {
            for (int y = 0; y < dim; y++) {
                item = PyTuple_GetItem(value, x + y * dim);
                if (PyFloat_Check(item)) {
                    cMatrix[x][y] = PyFloat_AsDouble(item);
                }
                else if (PyLong_Check(item)) {
                    cMatrix[x][y] = (double)PyLong_AsLong(item);
                }
                else {
                    throw Base::TypeError(
                        "Not allowed type used in matrix tuple (a number expected)...");
                }
            }
        }

        self.setValue(cMatrix);
    }
    else {
        std::string error = std::string("type must be 'Matrix' or tuple of 16 float or int, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyMatrix::Save(Base::Writer& writer) const
{
    auto& self = propGetterSelf<const App::PropertyMatrix>(*this);

    // clang-format off
    writer.Stream() << writer.ind() << "<PropertyMatrix";
    writer.Stream() << " a11=\"" <<  self._cMat[0][0] << "\" a12=\"" <<  self._cMat[0][1] << "\" a13=\"" <<  self._cMat[0][2] << "\" a14=\"" <<  self._cMat[0][3] << "\"";
    writer.Stream() << " a21=\"" <<  self._cMat[1][0] << "\" a22=\"" <<  self._cMat[1][1] << "\" a23=\"" <<  self._cMat[1][2] << "\" a24=\"" <<  self._cMat[1][3] << "\"";
    writer.Stream() << " a31=\"" <<  self._cMat[2][0] << "\" a32=\"" <<  self._cMat[2][1] << "\" a33=\"" <<  self._cMat[2][2] << "\" a34=\"" <<  self._cMat[2][3] << "\"";
    writer.Stream() << " a41=\"" <<  self._cMat[3][0] << "\" a42=\"" <<  self._cMat[3][1] << "\" a43=\"" <<  self._cMat[3][2] << "\" a44=\"" <<  self._cMat[3][3] << "\"";
    writer.Stream() <<"/>" << endl;
    // clang-format on
}

void PropertyMatrix::Restore(Base::XMLReader& reader)
{
    auto& self = propSetterSelf<App::PropertyMatrix>(*this);

    // read my Element
    reader.readElement("PropertyMatrix");
    // get the value of my Attribute
    self.aboutToSetValue();
    self._cMat[0][0] = reader.getAttribute<double>("a11");
    self._cMat[0][1] = reader.getAttribute<double>("a12");
    self._cMat[0][2] = reader.getAttribute<double>("a13");
    self._cMat[0][3] = reader.getAttribute<double>("a14");

    self._cMat[1][0] = reader.getAttribute<double>("a21");
    self._cMat[1][1] = reader.getAttribute<double>("a22");
    self._cMat[1][2] = reader.getAttribute<double>("a23");
    self._cMat[1][3] = reader.getAttribute<double>("a24");

    self._cMat[2][0] = reader.getAttribute<double>("a31");
    self._cMat[2][1] = reader.getAttribute<double>("a32");
    self._cMat[2][2] = reader.getAttribute<double>("a33");
    self._cMat[2][3] = reader.getAttribute<double>("a34");

    self._cMat[3][0] = reader.getAttribute<double>("a41");
    self._cMat[3][1] = reader.getAttribute<double>("a42");
    self._cMat[3][2] = reader.getAttribute<double>("a43");
    self._cMat[3][3] = reader.getAttribute<double>("a44");
    self.hasSetValue();
}


Property* PropertyMatrix::Copy() const
{
    auto& self = propGetterSelf<const App::PropertyMatrix>(*this);

    PropertyMatrix* p = new PropertyMatrix();
    p->_cMat = self._cMat;
    return p;
}

void PropertyMatrix::Paste(const Property& from)
{
    auto& self = propSetterSelf<App::PropertyMatrix>(*this);

    self.aboutToSetValue();
    self._cMat = dynamic_cast<const PropertyMatrix&>(from)._cMat;
    self.hasSetValue();
}

//**************************************************************************
//**************************************************************************
// PropertyPlacement
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyPlacement, App::Property)

//**************************************************************************
// Construction/Destruction


PropertyPlacement::PropertyPlacement() = default;


PropertyPlacement::~PropertyPlacement() = default;

//**************************************************************************
// Base class implementer


void PropertyPlacement::setValue(const Base::Placement& pos)
{
    auto& self = propSetterSelf<App::PropertyPlacement>(*this);

    self.aboutToSetValue();
    self._cPos = pos;
    self.hasSetValue();
}

bool PropertyPlacement::setValueIfChanged(const Base::Placement& pos, double tol, double atol)
{
    auto& self = propSetterSelf<App::PropertyPlacement>(*this);

    if (self._cPos.getPosition().IsEqual(pos.getPosition(), tol)
        && self._cPos.getRotation().isSame(pos.getRotation(), atol)) {
        return false;
    }
    self.setValue(pos);
    return true;
}


const Base::Placement& PropertyPlacement::getValue() const
{
    auto& self = propGetterSelf<const App::PropertyPlacement>(*this);

    return self._cPos;
}

void PropertyPlacement::getPaths(std::vector<ObjectIdentifier>& paths) const
{
    auto& self = propGetterSelf<const App::PropertyPlacement>(*this);

    paths.push_back(ObjectIdentifier(self)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Base"))
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("x")));
    paths.push_back(ObjectIdentifier(self)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Base"))
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("y")));
    paths.push_back(ObjectIdentifier(self)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Base"))
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("z")));
    paths.push_back(ObjectIdentifier(self)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Rotation"))
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Angle")));
    paths.push_back(ObjectIdentifier(self)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Rotation"))
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Axis"))
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("x")));
    paths.push_back(ObjectIdentifier(self)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Rotation"))
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Axis"))
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("y")));
    paths.push_back(ObjectIdentifier(self)
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
    auto& self = propSetterSelf<App::PropertyPlacement>(*this);

    auto updateAxis = [&self](int index, double coord) {
        Base::Vector3d axis;
        double angle;
        Base::Vector3d base = self._cPos.getPosition();
        Base::Rotation rot = self._cPos.getRotation();
        rot.getRawValue(axis, angle);
        axis[index] = coord;
        rot.setValue(axis, angle);
        Base::Placement plm(base, rot);
        self.setValue(plm);
    };

    auto updateYawPitchRoll = [&self](int index, double angle) {
        Base::Vector3d base = self._cPos.getPosition();
        Base::Rotation rot = self._cPos.getRotation();
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
        self.setValue(plm);
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
    auto& self = propGetterSelf<const App::PropertyPlacement>(*this);

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
            Unit::Angle);
    }
    else if (p == ".Base.x" || p == ".Base.y" || p == ".Base.z") {
        // Convert double to quantity
        return Base::Quantity(boost::any_cast<double>(Property::getPathValue(path)), Unit::Length);
    }
    else if (p == ".Rotation.Axis.x") {
        return getAxis(self._cPos).x;
    }
    else if (p == ".Rotation.Axis.y") {
        return getAxis(self._cPos).y;
    }
    else if (p == ".Rotation.Axis.z") {
        return getAxis(self._cPos).z;
    }
    else if (p == ".Rotation.Yaw") {
        return getYawPitchRoll(self._cPos).x;
    }
    else if (p == ".Rotation.Pitch") {
        return getYawPitchRoll(self._cPos).y;
    }
    else if (p == ".Rotation.Roll") {
        return getYawPitchRoll(self._cPos).z;
    }
    else {
        return Property::getPathValue(path);
    }
}

bool PropertyPlacement::getPyPathValue(const ObjectIdentifier& path, Py::Object& res) const
{
    auto& self = propGetterSelf<const App::PropertyPlacement>(*this);

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
        self._cPos.getRotation().getValue(axis, angle);
        res = Py::asObject(new QuantityPy(new Quantity(Base::toDegrees(angle), Unit::Angle)));
        return true;
    }
    else if (p == ".Base.x") {
        res = Py::asObject(new QuantityPy(new Quantity(self._cPos.getPosition().x, Unit::Length)));
        return true;
    }
    else if (p == ".Base.y") {
        res = Py::asObject(new QuantityPy(new Quantity(self._cPos.getPosition().y, Unit::Length)));
        return true;
    }
    else if (p == ".Base.z") {
        res = Py::asObject(new QuantityPy(new Quantity(self._cPos.getPosition().z, Unit::Length)));
        return true;
    }
    else if (p == ".Rotation.Axis.x") {
        res = Py::Float(getAxis(self._cPos).x);
        return true;
    }
    else if (p == ".Rotation.Axis.y") {
        res = Py::Float(getAxis(self._cPos).y);
        return true;
    }
    else if (p == ".Rotation.Axis.z") {
        res = Py::Float(getAxis(self._cPos).z);
        return true;
    }
    else if (p == ".Rotation.Yaw") {
        res = Py::Float(getYawPitchRoll(self._cPos).x);
        return true;
    }
    else if (p == ".Rotation.Pitch") {
        res = Py::Float(getYawPitchRoll(self._cPos).y);
        return true;
    }
    else if (p == ".Rotation.Roll") {
        res = Py::Float(getYawPitchRoll(self._cPos).z);
        return true;
    }

    return false;
}

PyObject* PropertyPlacement::getPyObject()
{
    auto& self = propGetterSelf<const App::PropertyPlacement>(*this);

    return new Base::PlacementPy(new Base::Placement(self._cPos));
}

void PropertyPlacement::setPyObject(PyObject* value)
{
    auto& self = propSetterSelf<App::PropertyPlacement>(*this);

    if (PyObject_TypeCheck(value, &(Base::MatrixPy::Type))) {
        Base::MatrixPy* pcObject = static_cast<Base::MatrixPy*>(value);
        Base::Matrix4D mat = pcObject->value();
        Base::Placement p;
        p.fromMatrix(mat);
        self.setValue(p);
    }
    else if (PyObject_TypeCheck(value, &(Base::PlacementPy::Type))) {
        self.setValue(*static_cast<Base::PlacementPy*>(value)->getPlacementPtr());
    }
    else {
        std::string error = std::string("type must be 'Matrix' or 'Placement', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyPlacement::Save(Base::Writer& writer) const
{
    auto& self = propGetterSelf<const App::PropertyPlacement>(*this);

    // clang-format off
    writer.Stream() << writer.ind() << "<PropertyPlacement";
    writer.Stream() << " Px=\"" << self._cPos.getPosition().x << "\""
                    << " Py=\"" << self._cPos.getPosition().y << "\""
                    << " Pz=\"" << self._cPos.getPosition().z << "\"";

    writer.Stream() << " Q0=\"" << self._cPos.getRotation()[0] << "\""
                    << " Q1=\"" << self._cPos.getRotation()[1] << "\""
                    << " Q2=\"" << self._cPos.getRotation()[2] << "\""
                    << " Q3=\"" << self._cPos.getRotation()[3] << "\"";
    Vector3d axis;
    double rfAngle {};
    self._cPos.getRotation().getRawValue(axis, rfAngle);
    writer.Stream() << " A=\"" << rfAngle << "\""
                    << " Ox=\"" << axis.x << "\""
                    << " Oy=\"" << axis.y << "\""
                    << " Oz=\"" << axis.z << "\"";
    writer.Stream() << "/>" << std::endl;
    // clang-format on
}

void PropertyPlacement::Restore(Base::XMLReader& reader)
{
    auto& self = propSetterSelf<App::PropertyPlacement>(*this);

    // read my Element
    reader.readElement("PropertyPlacement");
    // get the value of my Attribute
    self.aboutToSetValue();

    if (reader.hasAttribute("A")) {
        self._cPos = Base::Placement(Vector3d(reader.getAttribute<double>("Px"),
                                         reader.getAttribute<double>("Py"),
                                         reader.getAttribute<double>("Pz")),
                                Rotation(Vector3d(reader.getAttribute<double>("Ox"),
                                                  reader.getAttribute<double>("Oy"),
                                                  reader.getAttribute<double>("Oz")),
                                         reader.getAttribute<double>("A")));
    }
    else {
        self._cPos = Base::Placement(Vector3d(reader.getAttribute<double>("Px"),
                                         reader.getAttribute<double>("Py"),
                                         reader.getAttribute<double>("Pz")),
                                Rotation(reader.getAttribute<double>("Q0"),
                                         reader.getAttribute<double>("Q1"),
                                         reader.getAttribute<double>("Q2"),
                                         reader.getAttribute<double>("Q3")));
    }

    self.hasSetValue();
}


Property* PropertyPlacement::Copy() const
{
    auto& self = propGetterSelf<const App::PropertyPlacement>(*this);

    PropertyPlacement* p = new PropertyPlacement();
    p->_cPos = self._cPos;
    return p;
}

void PropertyPlacement::Paste(const Property& from)
{
    auto& self = propSetterSelf<App::PropertyPlacement>(*this);

    self.aboutToSetValue();
    self._cPos = dynamic_cast<const PropertyPlacement&>(from)._cPos;
    self.hasSetValue();
}


//**************************************************************************
// PropertyPlacementList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyPlacementList, App::PropertyLists)

//**************************************************************************
// Construction/Destruction

PropertyPlacementList::PropertyPlacementList() = default;

PropertyPlacementList::~PropertyPlacementList() = default;

//**************************************************************************
// Base class implementer

PyObject* PropertyPlacementList::getPyObject()
{
    auto& self = propGetterSelf<const App::PropertyPlacementList>(*this);

    PyObject* list = PyList_New(self.getSize());

    for (int i = 0; i < self.getSize(); i++) {
        PyList_SetItem(list, i, new Base::PlacementPy(new Base::Placement(self._lValueList[i])));
    }

    return list;
}

Base::Placement PropertyPlacementList::getPyValue(PyObject* item) const
{
    PropertyPlacement val;
    val.setPyObject(item);
    return val.getValue();
}

void PropertyPlacementList::Save(Base::Writer& writer) const
{
    auto& self = propGetterSelf<const App::PropertyPlacementList>(*this);

    if (!writer.isForceXML()) {
        writer.Stream() << writer.ind() << "<PlacementList file=\""
                        << writer.addFile(self.getName(), &self) << "\"/>" << std::endl;
    }
}

void PropertyPlacementList::Restore(Base::XMLReader& reader)
{
    reader.readElement("PlacementList");
    std::string file(reader.getAttribute<const char*>("file"));

    if (!file.empty()) {
        // initiate a file read
        reader.addFile(file.c_str(), this);
    }
}

void PropertyPlacementList::SaveDocFile(Base::Writer& writer) const
{
    auto& self = propGetterSelf<const App::PropertyPlacementList>(*this);

    Base::OutputStream str(writer.Stream());
    uint32_t uCt = (uint32_t)self.getSize();
    str << uCt;
    if (!self.isSinglePrecision()) {
        for (const auto& it : self._lValueList) {
            str << it.getPosition().x << it.getPosition().y << it.getPosition().z
                << it.getRotation()[0] << it.getRotation()[1] << it.getRotation()[2]
                << it.getRotation()[3];
        }
    }
    else {
        for (const auto& it : self._lValueList) {
            float x = (float)it.getPosition().x;
            float y = (float)it.getPosition().y;
            float z = (float)it.getPosition().z;
            float q0 = (float)it.getRotation()[0];
            float q1 = (float)it.getRotation()[1];
            float q2 = (float)it.getRotation()[2];
            float q3 = (float)it.getRotation()[3];
            str << x << y << z << q0 << q1 << q2 << q3;
        }
    }
}

void PropertyPlacementList::RestoreDocFile(Base::Reader& reader)
{
    auto& self = propSetterSelf<App::PropertyPlacementList>(*this);

    Base::InputStream str(reader);
    uint32_t uCt = 0;
    str >> uCt;
    std::vector<Base::Placement> values(uCt);
    if (!self.isSinglePrecision()) {
        for (auto& it : values) {
            Base::Vector3d pos;
            double q0, q1, q2, q3;
            str >> pos.x >> pos.y >> pos.z >> q0 >> q1 >> q2 >> q3;
            Base::Rotation rot(q0, q1, q2, q3);
            it.setPosition(pos);
            it.setRotation(rot);
        }
    }
    else {
        float x {}, y {}, z {};
        float q0 {}, q1 {}, q2 {}, q3 {};
        for (auto& it : values) {
            str >> x >> y >> z >> q0 >> q1 >> q2 >> q3;
            Base::Vector3d pos(x, y, z);
            Base::Rotation rot(q0, q1, q2, q3);
            it.setPosition(pos);
            it.setRotation(rot);
        }
    }
    self.setValues(values);
}

Property* PropertyPlacementList::Copy() const
{
    auto& self = propGetterSelf<const App::PropertyPlacementList>(*this);

    PropertyPlacementList* p = new PropertyPlacementList();
    p->_lValueList = self._lValueList;
    return p;
}

void PropertyPlacementList::Paste(const Property& from)
{
    auto& self = propSetterSelf<App::PropertyPlacementList>(*this);

    self.setValues(dynamic_cast<const PropertyPlacementList&>(from)._lValueList);
}

unsigned int PropertyPlacementList::getMemSize() const
{
    auto& self = propGetterSelf<const App::PropertyPlacementList>(*this);

    return static_cast<unsigned int>(self._lValueList.size() * sizeof(Base::Vector3d));
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
    auto& self = propGetterSelf<const App::PropertyPlacementLink>(*this);

    if (self._pcLink->isDerivedFrom<App::Placement>()) {
        return dynamic_cast<App::Placement*>(self._pcLink);
    }
    else {
        return nullptr;
    }
}

//**************************************************************************
// Base class implementer

Property* PropertyPlacementLink::Copy() const
{
    auto& self = propGetterSelf<const App::PropertyPlacementLink>(*this);

    PropertyPlacementLink* p = new PropertyPlacementLink();
    p->_pcLink = self._pcLink;
    return p;
}

void PropertyPlacementLink::Paste(const Property& from)
{
    auto& self = propSetterSelf<App::PropertyPlacementLink>(*this);

    self.aboutToSetValue();
    self._pcLink = dynamic_cast<const PropertyPlacementLink&>(from)._pcLink;
    self.hasSetValue();
}

//**************************************************************************
//**************************************************************************
// PropertyRotation
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyRotation, App::Property)

PropertyRotation::PropertyRotation() = default;


PropertyRotation::~PropertyRotation() = default;

void PropertyRotation::setValue(const Base::Rotation& rot)
{
    auto& self = propSetterSelf<App::PropertyRotation>(*this);

    self.aboutToSetValue();
    self._rot = rot;
    self.hasSetValue();
}

bool PropertyRotation::setValueIfChanged(const Base::Rotation& rot, double atol)
{
    auto& self = propSetterSelf<App::PropertyRotation>(*this);

    if (self._rot.isSame(rot, atol)) {
        return false;
    }

    self.setValue(rot);
    return true;
}


const Base::Rotation& PropertyRotation::getValue() const
{
    auto& self = propGetterSelf<const App::PropertyRotation>(*this);

    return self._rot;
}

void PropertyRotation::getPaths(std::vector<ObjectIdentifier>& paths) const
{
    auto& self = propGetterSelf<const App::PropertyRotation>(*this);

    paths.push_back(ObjectIdentifier(self)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Angle")));
    paths.push_back(ObjectIdentifier(self)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Axis"))
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("x")));
    paths.push_back(ObjectIdentifier(self)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Axis"))
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("y")));
    paths.push_back(ObjectIdentifier(self)
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("Axis"))
                    << ObjectIdentifier::SimpleComponent(ObjectIdentifier::String("z")));
}

void PropertyRotation::setPathValue(const ObjectIdentifier& path, const boost::any& value)
{
    auto& self = propSetterSelf<App::PropertyRotation>(*this);

    auto updateAxis = [&self](int index, double coord) {
        Base::Vector3d axis;
        double angle;
        self._rot.getRawValue(axis, angle);

        axis[index] = coord;
        self.setValue(Base::Rotation {axis, angle});
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
    auto& self = propGetterSelf<const App::PropertyRotation>(*this);

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
            Unit::Angle);
    }
    else if (p == ".Axis.x") {
        return getAxis(self._rot).x;
    }
    else if (p == ".Axis.y") {
        return getAxis(self._rot).y;
    }
    else if (p == ".Axis.z") {
        return getAxis(self._rot).z;
    }
    else {
        return Property::getPathValue(path);
    }
}

bool PropertyRotation::getPyPathValue(const ObjectIdentifier& path, Py::Object& res) const
{
    auto& self = propGetterSelf<const App::PropertyRotation>(*this);

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
        self._rot.getValue(axis, angle);
        res = Py::asObject(new QuantityPy(new Quantity(Base::toDegrees(angle), Unit::Angle)));
        return true;
    }
    else if (p == ".Axis.x") {
        res = Py::Float(getAxis(self._rot).x);
        return true;
    }
    else if (p == ".Axis.y") {
        res = Py::Float(getAxis(self._rot).y);
        return true;
    }
    else if (p == ".Axis.z") {
        res = Py::Float(getAxis(self._rot).z);
        return true;
    }

    return false;
}

PyObject* PropertyRotation::getPyObject()
{
    auto& self = propGetterSelf<const App::PropertyRotation>(*this);

    return new Base::RotationPy(new Base::Rotation(self._rot));
}

void PropertyRotation::setPyObject(PyObject* value)
{
    auto& self = propSetterSelf<App::PropertyRotation>(*this);

    if (PyObject_TypeCheck(value, &(Base::MatrixPy::Type))) {
        Base::MatrixPy* object = static_cast<Base::MatrixPy*>(value);
        Base::Matrix4D mat = object->value();
        Base::Rotation p;
        p.setValue(mat);
        self.setValue(p);
    }
    else if (PyObject_TypeCheck(value, &(Base::RotationPy::Type))) {
        self.setValue(*static_cast<Base::RotationPy*>(value)->getRotationPtr());
    }
    else {
        std::string error = std::string("type must be 'Matrix' or 'Rotation', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyRotation::Save(Base::Writer& writer) const
{
    auto& self = propGetterSelf<const App::PropertyRotation>(*this);

    Vector3d axis;
    double rfAngle {};
    self._rot.getRawValue(axis, rfAngle);

    writer.Stream() << writer.ind() << "<PropertyRotation";
    writer.Stream() << " A=\"" << rfAngle << "\""
                    << " Ox=\"" << axis.x << "\""
                    << " Oy=\"" << axis.y << "\""
                    << " Oz=\"" << axis.z << "\""
                    << "/>\n";
}

void PropertyRotation::Restore(Base::XMLReader& reader)
{
    auto& self = propSetterSelf<App::PropertyRotation>(*this);

    reader.readElement("PropertyRotation");
    self.aboutToSetValue();

    self._rot = Rotation(Vector3d(reader.getAttribute<double>("Ox"),
                             reader.getAttribute<double>("Oy"),
                             reader.getAttribute<double>("Oz")),
                    reader.getAttribute<double>("A"));
    self.hasSetValue();
}

Property* PropertyRotation::Copy() const
{
    auto& self = propGetterSelf<const App::PropertyRotation>(*this);

    PropertyRotation* p = new PropertyRotation();
    p->_rot = self._rot;
    return p;
}

void PropertyRotation::Paste(const Property& from)
{
    auto& self = propSetterSelf<App::PropertyRotation>(*this);

    self.aboutToSetValue();
    self._rot = dynamic_cast<const PropertyRotation&>(from)._rot;
    self.hasSetValue();
}

// ------------------------------------------------------------

TYPESYSTEM_SOURCE_ABSTRACT(App::PropertyGeometry, App::Property)

PropertyGeometry::PropertyGeometry() = default;

PropertyGeometry::~PropertyGeometry() = default;

// ------------------------------------------------------------

TYPESYSTEM_SOURCE_ABSTRACT(App::PropertyComplexGeoData, App::PropertyGeometry)

PropertyComplexGeoData::PropertyComplexGeoData() = default;

PropertyComplexGeoData::~PropertyComplexGeoData() = default;

std::string PropertyComplexGeoData::getElementMapVersion(bool) const
{
    auto& self = propGetterSelf<const App::PropertyComplexGeoData>(*this);

    auto data = self.getComplexData();
    if (!data) {
        return std::string();
    }
    return data->getElementMapVersion();
}

bool PropertyComplexGeoData::checkElementMapVersion(const char* ver) const
{
    auto& self = propGetterSelf<const App::PropertyComplexGeoData>(*this);

    auto data = self.getComplexData();
    if (!data) {
        return false;
    }
    return data->checkElementMapVersion(ver + 2);
}


void PropertyComplexGeoData::afterRestore()
{
    auto& self = propGetterSelf<const App::PropertyComplexGeoData>(*this);

    auto data = self.getComplexData();
    if (data && data->isRestoreFailed()) {
        data->resetRestoreFailure();
        auto owner = freecad_cast<DocumentObject*>(self.getContainer());
        if (owner && owner->getDocument()
            && !owner->getDocument()->testStatus(App::Document::PartialDoc)) {
            owner->getDocument()->addRecomputeObject(owner);
        }
    }
    PropertyGeometry::afterRestore();
}
