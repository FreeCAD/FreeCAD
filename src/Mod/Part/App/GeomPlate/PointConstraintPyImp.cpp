/***************************************************************************
 *   Copyright (c) 2020 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#ifndef _PreComp_
# include <memory>
# include <Standard_Failure.hxx>
#endif

#include <Base/VectorPy.h>
#include <Base/PyWrapParseTupleAndKeywords.h>

#include "GeomPlate/PointConstraintPy.h"
#include "GeomPlate/PointConstraintPy.cpp"


using namespace Part;

PyObject *PointConstraintPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of PointConstraintPy
    return new PointConstraintPy(nullptr);
}

// constructor method
int PointConstraintPy::PyInit(PyObject* args, PyObject* kwds)
{
    PyObject *pt;
    int order = 0;
    double tolDist = 0.0001;

    static const std::array<const char *, 4> keywords {"Point", "Order", "TolDist", nullptr};
    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!|id", keywords, &(Base::VectorPy::Type), &pt, &order,
                                             &tolDist)) {
        return -1;
    }

    try {
        std::unique_ptr<GeomPlate_PointConstraint> ptr;
        Base::Vector3d v = static_cast<Base::VectorPy*>(pt)->value();

        ptr = std::make_unique<GeomPlate_PointConstraint>(gp_Pnt(v.x, v.y, v.z), order, tolDist);
        setTwinPointer(ptr.release());

        return 0;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return -1;
    }
}

// returns a string which represents the object e.g. when printed in python
std::string PointConstraintPy::representation() const
{
    return {"<GeomPlate_PointConstraint object>"};
}

PyObject* PointConstraintPy::setOrder(PyObject *args)
{
    int order;
    if (!PyArg_ParseTuple(args, "i", &order))
        return nullptr;

    try {
        getGeomPlate_PointConstraintPtr()->SetOrder(order);
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* PointConstraintPy::order(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        Standard_Integer v = getGeomPlate_PointConstraintPtr()->Order();
        return PyLong_FromLong(v);
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* PointConstraintPy::G0Criterion(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        Standard_Real v = getGeomPlate_PointConstraintPtr()->G0Criterion();
        return PyFloat_FromDouble(v);
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* PointConstraintPy::G1Criterion(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        Standard_Real v = getGeomPlate_PointConstraintPtr()->G1Criterion();
        return PyFloat_FromDouble(v);
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* PointConstraintPy::G2Criterion(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        Standard_Real v = getGeomPlate_PointConstraintPtr()->G2Criterion();
        return PyFloat_FromDouble(v);
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* PointConstraintPy::setG0Criterion(PyObject *args)
{
    double tolDist;
    if (!PyArg_ParseTuple(args, "d", &tolDist))
        return nullptr;

    try {
        getGeomPlate_PointConstraintPtr()->SetG0Criterion(tolDist);
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* PointConstraintPy::setG1Criterion(PyObject *args)
{
    double tolAng;
    if (!PyArg_ParseTuple(args, "d", &tolAng))
        return nullptr;

    try {
        getGeomPlate_PointConstraintPtr()->SetG1Criterion(tolAng);
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* PointConstraintPy::setG2Criterion(PyObject *args)
{
    double tolCurv;
    if (!PyArg_ParseTuple(args, "d", &tolCurv))
        return nullptr;

    try {
        getGeomPlate_PointConstraintPtr()->SetG2Criterion(tolCurv);
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* PointConstraintPy::hasPnt2dOnSurf(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        Standard_Boolean ok = getGeomPlate_PointConstraintPtr()->HasPnt2dOnSurf();
        return Py_BuildValue("O", (ok ? Py_True : Py_False));
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* PointConstraintPy::setPnt2dOnSurf(PyObject *args)
{
    double x, y;
    if (!PyArg_ParseTuple(args, "dd", &x, &y))
        return nullptr;

    try {
        getGeomPlate_PointConstraintPtr()->SetPnt2dOnSurf(gp_Pnt2d(x, y));
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* PointConstraintPy::pnt2dOnSurf(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        gp_Pnt2d pt = getGeomPlate_PointConstraintPtr()->Pnt2dOnSurf();
        Py::Tuple coord(2);
        coord.setItem(0, Py::Float(pt.X()));
        coord.setItem(1, Py::Float(pt.Y()));
        return Py::new_reference_to(coord);
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject *PointConstraintPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int PointConstraintPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
