/***************************************************************************
 *   Copyright (c) 2017 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
 *   write to the Free Software Foundation, Inc., 51 Franklin Street,      *
 *   Fifth Floor, Boston, MA  02110-1301, USA                              *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#include "AxisPy.h"
#include "CoordinateSystemPy.h"
#include "CoordinateSystemPy.cpp"
#include "GeometryPyCXX.h"
#include "PlacementPy.h"
#include "VectorPy.h"


using namespace Base;

// returns a string which represents the object e.g. when printed in python
std::string CoordinateSystemPy::representation() const
{
    return {"<CoordinateSystem object>"};
}

PyObject *CoordinateSystemPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of CoordinateSystemPy and the Twin object
    return new CoordinateSystemPy(new CoordinateSystem);
}

// constructor method
int CoordinateSystemPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

PyObject* CoordinateSystemPy::setAxes(PyObject * args)
{
    PyObject *axis{}, *xdir{};
    if (PyArg_ParseTuple(args, "O!O!", &(AxisPy::Type), &axis, &(VectorPy::Type), &xdir)) {
        getCoordinateSystemPtr()->setAxes(*static_cast<AxisPy*>(axis)->getAxisPtr(),
                                          *static_cast<VectorPy*>(xdir)->getVectorPtr());
        Py_Return;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!O!", &(VectorPy::Type), &axis, &(VectorPy::Type), &xdir)) {
        getCoordinateSystemPtr()->setAxes(*static_cast<VectorPy*>(axis)->getVectorPtr(),
                                          *static_cast<VectorPy*>(xdir)->getVectorPtr());
        Py_Return;
    }

    PyErr_SetString(PyExc_TypeError, "Axis and Vector or Vector and Vector expected");
    return nullptr;
}

PyObject* CoordinateSystemPy::displacement(PyObject * args)
{
    PyObject *cs{};
    if (!PyArg_ParseTuple(args, "O!", &(CoordinateSystemPy::Type), &cs))
        return nullptr;
    Placement p = getCoordinateSystemPtr()->displacement(
        *static_cast<CoordinateSystemPy*>(cs)->getCoordinateSystemPtr());
    return new PlacementPy(new Placement(p));
}

PyObject* CoordinateSystemPy::transformTo(PyObject * args)
{
    PyObject *vec{};
    if (!PyArg_ParseTuple(args, "O!", &(VectorPy::Type), &vec))
        return nullptr;
    Vector3d v = static_cast<VectorPy*>(vec)->value();
    getCoordinateSystemPtr()->transformTo(v);
    return new VectorPy(new Vector3d(v));
}

PyObject* CoordinateSystemPy::transform(PyObject * args)
{
    PyObject *plm{};
    if (PyArg_ParseTuple(args, "O!", &(PlacementPy::Type), &plm)) {
        getCoordinateSystemPtr()->transform(*static_cast<PlacementPy*>(plm)->getPlacementPtr());
        Py_Return;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!", &(RotationPy::Type), &plm)) {
        getCoordinateSystemPtr()->transform(*static_cast<RotationPy*>(plm)->getRotationPtr());
        Py_Return;
    }

    PyErr_SetString(PyExc_TypeError, "Rotation or placement expected");
    return nullptr;
}

PyObject* CoordinateSystemPy::setPlacement(PyObject * args)
{
    PyObject *plm{};
    if (!PyArg_ParseTuple(args, "O!", &(PlacementPy::Type), &plm))
        return nullptr;
    getCoordinateSystemPtr()->setPlacement(*static_cast<PlacementPy*>(plm)->getPlacementPtr());
    Py_Return;
}

Py::Object CoordinateSystemPy::getAxis() const
{
    const Axis& axis = getCoordinateSystemPtr()->getAxis();
    return Py::asObject(new AxisPy(new Axis(axis)));
}

void CoordinateSystemPy::setAxis(Py::Object arg)
{
    if (PyObject_TypeCheck(arg.ptr(), &(Base::AxisPy::Type))) {
        AxisPy *axis = static_cast<AxisPy*>(arg.ptr());
        getCoordinateSystemPtr()->setAxis(*axis->getAxisPtr());
        return;
    }

    throw Py::TypeError("not an Axis");
}

Py::Object CoordinateSystemPy::getXDirection() const
{
    return Py::Vector(getCoordinateSystemPtr()->getXDirection()); // NOLINT
}

void CoordinateSystemPy::setXDirection(Py::Object arg)
{
    getCoordinateSystemPtr()->setXDirection(Py::Vector(arg).toVector());
}

Py::Object CoordinateSystemPy::getYDirection() const
{
    return Py::Vector(getCoordinateSystemPtr()->getYDirection()); // NOLINT
}

void CoordinateSystemPy::setYDirection(Py::Object arg)
{
    getCoordinateSystemPtr()->setYDirection(Py::Vector(arg).toVector());
}

Py::Object CoordinateSystemPy::getZDirection() const
{
    return Py::Vector(getCoordinateSystemPtr()->getZDirection()); // NOLINT
}

void CoordinateSystemPy::setZDirection(Py::Object arg)
{
    getCoordinateSystemPtr()->setZDirection(Py::Vector(arg).toVector());
}

Py::Object CoordinateSystemPy::getPosition() const
{
    return Py::Vector(getCoordinateSystemPtr()->getPosition()); // NOLINT
}

void CoordinateSystemPy::setPosition(Py::Object arg)
{
    getCoordinateSystemPtr()->setPosition(Py::Vector(arg).toVector());
}

PyObject *CoordinateSystemPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int CoordinateSystemPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
