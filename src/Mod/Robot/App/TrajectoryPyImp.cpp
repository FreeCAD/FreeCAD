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

// clang-format off
// inclusion of the generated files (generated out of TrajectoryPy.xml)
#include <Mod/Robot/App/TrajectoryPy.h>
#include <Mod/Robot/App/TrajectoryPy.cpp>
#include <Mod/Robot/App/WaypointPy.h>
// clang-format on


using namespace Robot;

// returns a string which represents the object e.g. when printed in python
std::string TrajectoryPy::representation() const
{
    std::stringstream str;
    str.precision(5);
    str << "Trajectory [";
    str << "size:" << getTrajectoryPtr()->getSize() << " ";
    str << "length:" << getTrajectoryPtr()->getLength() << " ";
    str << "duration:" << getTrajectoryPtr()->getDuration() << " ";
    str << "]";

    return str.str();
}

PyObject* TrajectoryPy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    // create a new instance of TrajectoryPy and the Twin object
    return new TrajectoryPy(new Trajectory);
}

// constructor method
int TrajectoryPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    PyObject* pcObj = nullptr;
    if (!PyArg_ParseTuple(args, "|O!", &(PyList_Type), &pcObj)) {
        return -1;
    }

    if (pcObj) {
        Py::List list(pcObj);
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
            if (PyObject_TypeCheck((*it).ptr(), &(Robot::WaypointPy::Type))) {
                Robot::Waypoint& wp =
                    *static_cast<Robot::WaypointPy*>((*it).ptr())->getWaypointPtr();
                getTrajectoryPtr()->addWaypoint(wp);
            }
        }
    }
    getTrajectoryPtr()->generateTrajectory();
    return 0;
}


PyObject* TrajectoryPy::insertWaypoints(PyObject* args)
{

    PyObject* o;
    if (PyArg_ParseTuple(args, "O!", &(Base::PlacementPy::Type), &o)) {
        Base::Placement* plm = static_cast<Base::PlacementPy*>(o)->getPlacementPtr();
        getTrajectoryPtr()->addWaypoint(Robot::Waypoint("Pt", *plm));
        getTrajectoryPtr()->generateTrajectory();

        return new TrajectoryPy(new Robot::Trajectory(*getTrajectoryPtr()));
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!", &(Robot::WaypointPy::Type), &o)) {
        Robot::Waypoint& wp = *static_cast<Robot::WaypointPy*>(o)->getWaypointPtr();
        getTrajectoryPtr()->addWaypoint(wp);
        getTrajectoryPtr()->generateTrajectory();

        return new TrajectoryPy(new Robot::Trajectory(*getTrajectoryPtr()));
        // Py_Return;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!", &(PyList_Type), &o)) {
        Py::List list(o);
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
            if (PyObject_TypeCheck((*it).ptr(), &(Robot::WaypointPy::Type))) {
                Robot::Waypoint& wp =
                    *static_cast<Robot::WaypointPy*>((*it).ptr())->getWaypointPtr();
                getTrajectoryPtr()->addWaypoint(wp);
            }
        }
        getTrajectoryPtr()->generateTrajectory();

        return new TrajectoryPy(new Robot::Trajectory(*getTrajectoryPtr()));
    }

    Py_Error(PyExc_TypeError, "Wrong parameters - waypoint or placement expected");
}

PyObject* TrajectoryPy::position(PyObject* args)
{
    double pos;
    if (!PyArg_ParseTuple(args, "d", &pos)) {
        return nullptr;
    }

    return (new Base::PlacementPy(new Base::Placement(getTrajectoryPtr()->getPosition(pos))));
}

PyObject* TrajectoryPy::velocity(PyObject* args)
{
    double pos;
    if (!PyArg_ParseTuple(args, "d", &pos)) {
        return nullptr;
    }

    // return velocity as float
    return Py::new_reference_to(Py::Float(getTrajectoryPtr()->getVelocity(pos)));
}

PyObject* TrajectoryPy::deleteLast(PyObject* args)
{
    int n = 1;
    if (!PyArg_ParseTuple(args, "|i", &n)) {
        return nullptr;
    }
    getTrajectoryPtr()->deleteLast(n);
    return new TrajectoryPy(new Robot::Trajectory(*getTrajectoryPtr()));
}


Py::Float TrajectoryPy::getDuration() const
{
    return Py::Float(getTrajectoryPtr()->getDuration());
}

Py::List TrajectoryPy::getWaypoints() const
{
    Py::List list;
    for (unsigned int i = 0; i < getTrajectoryPtr()->getSize(); i++) {
        list.append(Py::asObject(
            new Robot::WaypointPy(new Robot::Waypoint(getTrajectoryPtr()->getWaypoint(i)))));
    }

    return list;
}

Py::Float TrajectoryPy::getLength() const
{
    return Py::Float(getTrajectoryPtr()->getLength());
}


void TrajectoryPy::setWaypoints(Py::List)
{}

PyObject* TrajectoryPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int TrajectoryPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
