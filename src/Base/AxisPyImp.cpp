/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include "GeometryPyCXX.h"

// inclusion of the generated files (generated out of AxisPy.xml)
#include "AxisPy.h"
#include "AxisPy.cpp"

#include "VectorPy.h"
#include "PlacementPy.h"

using namespace Base;

// returns a string which represents the object e.g. when printed in python
std::string AxisPy::representation() const
{
    AxisPy::PointerType ptr = getAxisPtr();
    std::stringstream str;
    str << "Axis [Base=(";
    str << ptr->getBase().x << ","<< ptr->getBase().y << "," << ptr->getBase().z;
    str << "), Direction=(";
    str << ptr->getDirection().x << ","<< ptr->getDirection().y << "," << ptr->getDirection().z << ")]";

    return str.str();
}

PyObject *AxisPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of AxisPy and the Twin object
    return new AxisPy(new Axis);
}

// constructor method
int AxisPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    PyObject* o{};
    if (PyArg_ParseTuple(args, "")) {
        return 0;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!", &(Base::AxisPy::Type), &o)) {
        Base::Axis *a = static_cast<Base::AxisPy*>(o)->getAxisPtr();
        *(getAxisPtr()) = *a;
        return 0;
    }

    PyErr_Clear();
    PyObject* d{};
    if (PyArg_ParseTuple(args, "O!O!", &(Base::VectorPy::Type), &o,
                                      &(Base::VectorPy::Type), &d)) {
        // NOTE: The first parameter defines the base (origin) and the second the direction.
        *getAxisPtr() = Base::Axis(static_cast<Base::VectorPy*>(o)->value(),
                                   static_cast<Base::VectorPy*>(d)->value());
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "empty parameter list, axis or base and direction expected");
    return -1;
}

PyObject* AxisPy::move(PyObject * args)
{
    PyObject *vec{};
    if (!PyArg_ParseTuple(args, "O!", &(VectorPy::Type), &vec))
        return nullptr;
    getAxisPtr()->move(static_cast<VectorPy*>(vec)->value());
    Py_Return;
}

PyObject* AxisPy::multiply(PyObject * args)
{
    PyObject *plm{};
    if (!PyArg_ParseTuple(args, "O!", &(PlacementPy::Type), &plm))
        return nullptr;
    Axis mult = (*getAxisPtr()) * (*static_cast<PlacementPy*>(plm)->getPlacementPtr());
    return new AxisPy(new Axis(mult));
}

PyObject* AxisPy::copy(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    return new AxisPy(new Axis(*getAxisPtr()));
}

PyObject* AxisPy::reversed(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    Base::Axis a = getAxisPtr()->reversed();
    return new AxisPy(new Axis(a));
}

Py::Object AxisPy::getBase() const
{
    return Py::Vector(getAxisPtr()->getBase()); // NOLINT
}

void AxisPy::setBase(Py::Object arg)
{
    getAxisPtr()->setBase(Py::Vector(arg).toVector());
}

Py::Object AxisPy::getDirection() const
{
    return Py::Vector(getAxisPtr()->getDirection()); // NOLINT
}

void AxisPy::setDirection(Py::Object arg)
{
    getAxisPtr()->setDirection(Py::Vector(arg).toVector());
}

PyObject *AxisPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int AxisPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

