/***************************************************************************
 *   Copyright (c) 2010 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <Base/PlacementPy.h>
#include <Base/PyWrapParseTupleAndKeywords.h>
#include <Base/UnitsApi.h>

// clang-format off
// inclusion of the generated files (generated out of WaypointPy.xml)
#include "WaypointPy.h"
#include "WaypointPy.cpp"
// clang-format on


using namespace Robot;
using namespace Base;

// returns a string which represents the object e.g. when printed in python
std::string WaypointPy::representation() const
{
    double A, B, C;
    std::stringstream str;
    getWaypointPtr()->EndPos.getRotation().getYawPitchRoll(A, B, C);
    str.precision(5);
    str << "Waypoint [";
    if (getWaypointPtr()->Type == Waypoint::PTP) {
        str << "PTP ";
    }
    else if (getWaypointPtr()->Type == Waypoint::LINE) {
        str << "LIN ";
    }
    else if (getWaypointPtr()->Type == Waypoint::CIRC) {
        str << "CIRC ";
    }
    else if (getWaypointPtr()->Type == Waypoint::WAIT) {
        str << "WAIT ";
    }
    else if (getWaypointPtr()->Type == Waypoint::UNDEF) {
        str << "UNDEF ";
    }
    str << getWaypointPtr()->Name;
    str << " (";
    str << getWaypointPtr()->EndPos.getPosition().x << ","
        << getWaypointPtr()->EndPos.getPosition().y << ","
        << getWaypointPtr()->EndPos.getPosition().z;
    str << ";" << A << "," << B << "," << C << ")";
    str << "v=" << getWaypointPtr()->Velocity << " ";
    if (getWaypointPtr()->Cont) {
        str << "Cont ";
    }
    if (getWaypointPtr()->Tool != 0) {
        str << "Tool" << getWaypointPtr()->Tool << " ";
    }
    if (getWaypointPtr()->Base != 0) {
        str << "Tool" << getWaypointPtr()->Base << " ";
    }
    str << "]";

    return str.str();
}

PyObject* WaypointPy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    // create a new instance of WaypointPy and the Twin object
    return new WaypointPy(new Waypoint);
}

// constructor method
int WaypointPy::PyInit(PyObject* args, PyObject* kwd)
{
    PyObject* pos;
    char* name = "P";
    char* type = "PTP";
    PyObject* vel = nullptr;
    PyObject* acc = nullptr;
    int cont = 0;
    int tool = 0;
    int base = 0;

    static const std::array<const char*, 9>
        kwlist {"Pos", "type", "name", "vel", "cont", "tool", "base", "acc", nullptr};

    if (!Base::Wrapped_ParseTupleAndKeywords(args,
                                             kwd,
                                             "O!|ssOiiiO",
                                             kwlist,
                                             &(Base::PlacementPy::Type),
                                             &pos,  // the placement object
                                             &type,
                                             &name,
                                             &vel,
                                             &cont,
                                             &tool,
                                             &base,
                                             &acc)) {
        return -1;
    }

    Base::Placement TempPos = *static_cast<Base::PlacementPy*>(pos)->getPlacementPtr();
    getWaypointPtr()->EndPos = TempPos;
    getWaypointPtr()->Name = name;
    std::string typeStr(type);
    if (typeStr == "PTP") {
        getWaypointPtr()->Type = Waypoint::PTP;
    }
    else if (typeStr == "LIN") {
        getWaypointPtr()->Type = Waypoint::LINE;
    }
    else if (typeStr == "CIRC") {
        getWaypointPtr()->Type = Waypoint::CIRC;
    }
    else if (typeStr == "WAIT") {
        getWaypointPtr()->Type = Waypoint::WAIT;
    }
    else {
        getWaypointPtr()->Type = Waypoint::UNDEF;
    }

    if (!vel) {
        switch (getWaypointPtr()->Type) {
            case Waypoint::PTP:
                getWaypointPtr()->Velocity = 100;
                break;
            case Waypoint::LINE:
                getWaypointPtr()->Velocity = 2000;
                break;
            case Waypoint::CIRC:
                getWaypointPtr()->Velocity = 2000;
                break;
            default:
                getWaypointPtr()->Velocity = 0;
        }
    }
    else {
        getWaypointPtr()->Velocity = Base::UnitsApi::toDouble(vel, Base::Unit::Velocity);
    }
    getWaypointPtr()->Cont = cont ? true : false;
    getWaypointPtr()->Tool = tool;
    getWaypointPtr()->Base = base;
    if (!acc) {
        getWaypointPtr()->Acceleration = 100;
    }
    else {
        getWaypointPtr()->Acceleration = Base::UnitsApi::toDouble(acc, Base::Unit::Acceleration);
    }

    return 0;
}


Py::Float WaypointPy::getVelocity() const
{
    return Py::Float(getWaypointPtr()->Velocity);
}

void WaypointPy::setVelocity(Py::Float arg)
{
    getWaypointPtr()->Velocity = (float)arg.operator double();
}


Py::String WaypointPy::getName() const
{
    return {getWaypointPtr()->Name};
}

void WaypointPy::setName(Py::String arg)
{
    getWaypointPtr()->Name = arg.as_std_string("ascii");
}

Py::String WaypointPy::getType() const
{
    if (getWaypointPtr()->Type == Waypoint::PTP) {
        return Py::String("PTP");
    }
    else if (getWaypointPtr()->Type == Waypoint::LINE) {
        return Py::String("LIN");
    }
    else if (getWaypointPtr()->Type == Waypoint::CIRC) {
        return Py::String("CIRC");
    }
    else if (getWaypointPtr()->Type == Waypoint::WAIT) {
        return Py::String("WAIT");
    }
    else if (getWaypointPtr()->Type == Waypoint::UNDEF) {
        return Py::String("UNDEF");
    }
    else {
        throw Base::TypeError("Unknown waypoint type! Only: PTP,LIN,CIRC,WAIT are supported.");
    }
}

void WaypointPy::setType(Py::String arg)
{
    std::string typeStr(arg.as_std_string("ascii"));
    if (typeStr == "PTP") {
        getWaypointPtr()->Type = Waypoint::PTP;
    }
    else if (typeStr == "LIN") {
        getWaypointPtr()->Type = Waypoint::LINE;
    }
    else if (typeStr == "CIRC") {
        getWaypointPtr()->Type = Waypoint::CIRC;
    }
    else if (typeStr == "WAIT") {
        getWaypointPtr()->Type = Waypoint::WAIT;
    }
    else {
        throw Base::TypeError("Unknown waypoint type! Only: PTP,LIN,CIRC,WAIT are allowed.");
    }
}


Py::Object WaypointPy::getPos() const
{
    return Py::Object(new PlacementPy(new Placement(getWaypointPtr()->EndPos)), true);
}

void WaypointPy::setPos(Py::Object arg)
{
    Py::Type PlacementType(Base::getTypeAsObject(&(Base::PlacementPy::Type)));
    if (arg.isType(PlacementType)) {
        getWaypointPtr()->EndPos = *static_cast<Base::PlacementPy*>((*arg))->getPlacementPtr();
    }
}

Py::Boolean WaypointPy::getCont() const
{
    return {getWaypointPtr()->Cont};
}

void WaypointPy::setCont(Py::Boolean arg)
{
    getWaypointPtr()->Cont = (bool)arg;
}

Py::Long WaypointPy::getTool() const
{
    return Py::Long((long)getWaypointPtr()->Tool);
}

void WaypointPy::setTool(Py::Long arg)
{
    long value = static_cast<long>(arg);
    if (value >= 0) {
        getWaypointPtr()->Tool = value;
    }
    else {
        throw Py::ValueError("negative tool not allowed!");
    }
}

Py::Long WaypointPy::getBase() const
{
    return Py::Long((long)getWaypointPtr()->Base);
}

void WaypointPy::setBase(Py::Long arg)
{
    long value = static_cast<long>(arg);
    if (value >= 0) {
        getWaypointPtr()->Base = value;
    }
    else {
        throw Py::ValueError("negative base not allowed!");
    }
}

PyObject* WaypointPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int WaypointPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
