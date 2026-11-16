// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2022 Matteo Grellier
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


// clang-format off
#include "Blending/BlendCurvePy.h"
#include "Blending/BlendCurvePy.cpp"
#include "Blending/BlendPointPy.h"
// clang-format on
#include <Base/VectorPy.h>
#include <Mod/Part/App/BezierCurvePy.h>

using namespace Surface;

std::string BlendCurvePy::representation() const
{
    return "BlendCurve";
}

PyObject* BlendCurvePy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    // create a new instance of BlendCurvePy
    return new BlendCurvePy(new BlendCurve);
}

int BlendCurvePy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
    PyObject* b1;
    PyObject* b2;

    if (!PyArg_ParseTuple(
            args,
            "O!O!",
            &(Surface::BlendPointPy::Type),
            &b1,
            &(Surface::BlendPointPy::Type),
            &b2
        )) {
        return -1;
    }

    std::vector<BlendPoint> bpList;
    BlendPoint* geom1 = static_cast<BlendPointPy*>(b1)->getBlendPointPtr();
    BlendPoint* geom2 = static_cast<BlendPointPy*>(b2)->getBlendPointPtr();
    bpList.emplace_back(*geom1);
    bpList.emplace_back(*geom2);
    this->getBlendCurvePtr()->blendPoints = bpList;
    return 0;
}

PyObject* BlendCurvePy::compute(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    BlendCurve* bc = getBlendCurvePtr();
    Handle(Geom_BezierCurve) gc = bc->compute();
    return new Part::BezierCurvePy(new Part::GeomBezierCurve(gc));
}

PyObject* BlendCurvePy::setSize(PyObject* args)
{
    int i;
    double size;
    PyObject* relative = Py_True;
    if (!PyArg_ParseTuple(args, "idO!", &i, &size, &PyBool_Type, &relative)) {
        return nullptr;
    }
    try {
        getBlendCurvePtr()->setSize(i, size, Base::asBoolean(relative));
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(Base::PyExc_FC_CADKernelError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BlendCurvePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int BlendCurvePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
