/***************************************************************************
 *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "PrecisionPy.h"
#include "PrecisionPy.cpp"


using Base::Precision;
using Base::PrecisionPy;


// returns a string which represents the object e.g. when printed in python
std::string PrecisionPy::representation() const
{
    return {"<Precision object>"};
}

PyObject* PrecisionPy::angular(PyObject *args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    Py::Float v(Precision::Angular());
    return Py::new_reference_to(v);
}

PyObject* PrecisionPy::confusion(PyObject *args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    Py::Float v(Precision::Confusion());
    return Py::new_reference_to(v);
}

PyObject* PrecisionPy::squareConfusion(PyObject *args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    Py::Float v(Precision::SquareConfusion());
    return Py::new_reference_to(v);
}

PyObject* PrecisionPy::intersection(PyObject *args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    Py::Float v(Precision::Intersection());
    return Py::new_reference_to(v);
}

PyObject* PrecisionPy::approximation(PyObject *args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    Py::Float v(Precision::Approximation());
    return Py::new_reference_to(v);
}

PyObject* PrecisionPy::parametric(PyObject *args)
{
    double p{};
    if (PyArg_ParseTuple(args, "d", &p)) {
        Py::Float v(Precision::Parametric(p));
        return Py::new_reference_to(v);
    }

    PyErr_Clear();
    double t{};
    if (PyArg_ParseTuple(args, "dd", &p, &t)) {
        Py::Float v(Precision::Parametric(p, t));
        return Py::new_reference_to(v);
    }

    PyErr_SetString(PyExc_ValueError, "one or two floats expected");
    return nullptr;
}

PyObject* PrecisionPy::isInfinite(PyObject *args)
{
    double v{};
    if (!PyArg_ParseTuple(args, "d", &v)) {
        return nullptr;
    }

    Py::Boolean b(Precision::IsInfinite(v));
    return Py::new_reference_to(b);
}

PyObject* PrecisionPy::isPositiveInfinite(PyObject *args)
{
    double v{};
    if (!PyArg_ParseTuple(args, "d", &v)) {
        return nullptr;
    }

    Py::Boolean b(Precision::IsPositiveInfinite(v));
    return Py::new_reference_to(b);
}

PyObject* PrecisionPy::isNegativeInfinite(PyObject *args)
{
    double v{};
    if (!PyArg_ParseTuple(args, "d", &v)) {
        return nullptr;
    }

    Py::Boolean b(Precision::IsNegativeInfinite(v));
    return Py::new_reference_to(b);
}

PyObject* PrecisionPy::infinite(PyObject *args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    Py::Float v(Precision::Infinite());
    return Py::new_reference_to(v);
}

PyObject *PrecisionPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int PrecisionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
