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
# include <GeomAdaptor_Curve.hxx>
# include <Geom2dAdaptor_Curve.hxx>
# include <Standard_Failure.hxx>
# include <Standard_Version.hxx>
# if OCC_VERSION_HEX < 0x070600
# include <GeomAdaptor_HCurve.hxx>
# include <Geom2dAdaptor_HCurve.hxx>
# endif
#endif

#include "GeomPlate/CurveConstraintPy.h"
#include "GeomPlate/CurveConstraintPy.cpp"
#include "Geom2d/Curve2dPy.h"
#include "GeometryCurvePy.h"
#include <Base/PyWrapParseTupleAndKeywords.h>


using namespace Part;

PyObject *CurveConstraintPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of CurveConstraintPy and the Twin object
    return new CurveConstraintPy(nullptr);
}

// constructor method
int CurveConstraintPy::PyInit(PyObject* args, PyObject* kwds)
{
    PyObject *bound = nullptr;
    int order = 0;
    int nbPts = 10;
    double tolDist = 0.0001;
    double tolAng = 0.01;
    double tolCurv = 0.1;

    // GeomPlate_CurveConstraint has a default constructor but OCCT doesn't check
    // if neither a 2d, 3d or curve on surface is set when accessing the functions
    // Length(), FirstParameter(), LastParameter(), ...
    // Thus, we don't allow to create an empty GeomPlate_CurveConstraint instance

    static const std::array<const char *, 7> keywords{"Boundary", "Order", "NbPts", "TolDist", "TolAng", "TolCurv",
                                                      nullptr};
    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!|iiddd", keywords,
                                             &(GeometryCurvePy::Type), &bound, &order,
                                             &nbPts, &tolDist, &tolAng, &tolCurv)) {
        return -1;
    }

    try {
        std::unique_ptr<GeomPlate_CurveConstraint> ptr;
        if (bound) {
            GeomCurve* curve = static_cast<GeometryCurvePy*>(bound)->getGeomCurvePtr();
            Handle(Geom_Curve) handle = Handle(Geom_Curve)::DownCast(curve->handle());
            if (handle.IsNull()) {
                PyErr_SetString(PyExc_ReferenceError, "No valid curve handle");
                return -1;
            }

#if OCC_VERSION_HEX >= 0x070600
            Handle(Adaptor3d_Curve) hCurve;
            if (curve->getTypeId().isDerivedFrom(GeomTrimmedCurve::getClassTypeId())) {
                GeomTrimmedCurve* trim = static_cast<GeomTrimmedCurve*>(curve);
                hCurve = new GeomAdaptor_Curve(handle, trim->getFirstParameter(), trim->getLastParameter());
            }
            else {
                hCurve = new GeomAdaptor_Curve(handle);
            }
#else
            Handle(Adaptor3d_HCurve) hCurve;
            if (curve->getTypeId().isDerivedFrom(GeomTrimmedCurve::getClassTypeId())) {
                GeomTrimmedCurve* trim = static_cast<GeomTrimmedCurve*>(curve);
                GeomAdaptor_Curve adapt(handle, trim->getFirstParameter(), trim->getLastParameter());
                hCurve = new GeomAdaptor_HCurve(adapt);
            }
            else {
                GeomAdaptor_Curve adapt(handle);
                hCurve = new GeomAdaptor_HCurve(adapt);
            }
#endif

            ptr = std::make_unique<GeomPlate_CurveConstraint>(hCurve, order, nbPts, tolDist, tolAng, tolCurv);
        }
        else {
            ptr = std::make_unique<GeomPlate_CurveConstraint>();
        }

        setTwinPointer(ptr.release());

        return 0;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return -1;
    }
}

// returns a string which represents the object e.g. when printed in python
std::string CurveConstraintPy::representation() const
{
    return {"<GeomPlate_CurveConstraint object>"};
}

PyObject* CurveConstraintPy::setOrder(PyObject *args)
{
    int order;
    if (!PyArg_ParseTuple(args, "i", &order))
        return nullptr;

    try {
        getGeomPlate_CurveConstraintPtr()->SetOrder(order);
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* CurveConstraintPy::order(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        Standard_Integer v = getGeomPlate_CurveConstraintPtr()->Order();
        return PyLong_FromLong(v);
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* CurveConstraintPy::G0Criterion(PyObject *args)
{
    double u;
    if (!PyArg_ParseTuple(args, "d", &u))
        return nullptr;

    try {
        Standard_Real v = getGeomPlate_CurveConstraintPtr()->G0Criterion(u);
        return PyFloat_FromDouble(v);
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* CurveConstraintPy::G1Criterion(PyObject *args)
{
    double u;
    if (!PyArg_ParseTuple(args, "d", &u))
        return nullptr;

    try {
        Standard_Real v = getGeomPlate_CurveConstraintPtr()->G1Criterion(u);
        return PyFloat_FromDouble(v);
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* CurveConstraintPy::G2Criterion(PyObject *args)
{
    double u;
    if (!PyArg_ParseTuple(args, "d", &u))
        return nullptr;

    try {
        Standard_Real v = getGeomPlate_CurveConstraintPtr()->G2Criterion(u);
        return PyFloat_FromDouble(v);
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* CurveConstraintPy::setG0Criterion(PyObject *)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return nullptr;
}

PyObject* CurveConstraintPy::setG1Criterion(PyObject *)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return nullptr;
}

PyObject* CurveConstraintPy::setG2Criterion(PyObject *)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return nullptr;
}

PyObject* CurveConstraintPy::curve3d(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        auto hAdapt = getGeomPlate_CurveConstraintPtr()->Curve3d();
        if (hAdapt.IsNull())
            Py_Return;

#if OCC_VERSION_HEX >= 0x070600
        const Adaptor3d_Curve& a3d = *hAdapt;
#else
        const Adaptor3d_Curve& a3d = hAdapt->Curve();
#endif
        std::unique_ptr<GeomCurve> ptr(Part::makeFromCurveAdaptor(a3d));
        return ptr->getPyObject();
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* CurveConstraintPy::setCurve2dOnSurf(PyObject *args)
{
    PyObject* c;
    if (!PyArg_ParseTuple(args, "O!", &Part::Curve2dPy::Type, &c))
        return nullptr;

    try {
        Handle(Geom2d_Curve) curve2 = Handle(Geom2d_Curve)::DownCast(static_cast<Geometry2dPy*>(c)->getGeometry2dPtr()->handle());
        if (curve2.IsNull()) {
            PyErr_SetString(PyExc_ReferenceError, "No valid curve handle");
            return nullptr;
        }

        getGeomPlate_CurveConstraintPtr()->SetCurve2dOnSurf(curve2);
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* CurveConstraintPy::curve2dOnSurf(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        Handle(Geom2d_Curve) curve2 = getGeomPlate_CurveConstraintPtr()->Curve2dOnSurf();
        if (curve2.IsNull())
            Py_Return;

        std::unique_ptr<Part::Geom2dCurve> ptr(Part::makeFromCurve2d(curve2));
        return ptr->getPyObject();
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* CurveConstraintPy::setProjectedCurve(PyObject *args)
{
    PyObject* c;
    double tolU, tolV;
    if (!PyArg_ParseTuple(args, "O!dd", &Part::Curve2dPy::Type, &c, &tolU, &tolV))
        return nullptr;

    try {
        Geom2dCurve* curve2 = static_cast<Curve2dPy*>(c)->getGeom2dCurvePtr();
        Handle(Geom2d_Curve) handle = Handle(Geom2d_Curve)::DownCast(curve2->handle());
        if (handle.IsNull()) {
            PyErr_SetString(PyExc_ReferenceError, "No valid curve handle");
            return nullptr;
        }

#if OCC_VERSION_HEX >= 0x070600
        Handle(Adaptor2d_Curve2d) hCurve;
        if (handle->IsKind(STANDARD_TYPE(Geom2d_TrimmedCurve))) {
            Handle(Geom2d_TrimmedCurve) aTC (Handle(Geom2d_TrimmedCurve)::DownCast (handle));
            hCurve = new Geom2dAdaptor_Curve(handle, aTC->FirstParameter(), aTC->LastParameter());
        }
        else {
            hCurve = new Geom2dAdaptor_Curve(handle);
        }
#else
        Handle(Adaptor2d_HCurve2d) hCurve;
        if (handle->IsKind(STANDARD_TYPE(Geom2d_TrimmedCurve))) {
            Handle(Geom2d_TrimmedCurve) aTC (Handle(Geom2d_TrimmedCurve)::DownCast (handle));
            Geom2dAdaptor_Curve adapt(handle, aTC->FirstParameter(), aTC->LastParameter());
            hCurve = new Geom2dAdaptor_HCurve(adapt);
        }
        else {
            Geom2dAdaptor_Curve adapt(handle);
            hCurve = new Geom2dAdaptor_HCurve(adapt);
        }
#endif

        getGeomPlate_CurveConstraintPtr()->SetProjectedCurve(hCurve, tolU, tolV);
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* CurveConstraintPy::projectedCurve(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        auto hAdapt = getGeomPlate_CurveConstraintPtr()->ProjectedCurve();
        if (hAdapt.IsNull())
            Py_Return;

#if OCC_VERSION_HEX >= 0x070600
        const Adaptor2d_Curve2d& a2d = *hAdapt;
#else
        const Adaptor2d_Curve2d& a2d = hAdapt->Curve2d();
#endif
        std::unique_ptr<Geom2dCurve> ptr(Part::makeFromCurveAdaptor2d(a2d));
        return ptr->getPyObject();
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

Py::Long CurveConstraintPy::getNbPoints() const
{
    try {
        Standard_Integer v = getGeomPlate_CurveConstraintPtr()->NbPoints();
        return Py::Long(v);
    }
    catch (const Standard_Failure& e) {
        throw Py::RuntimeError(e.GetMessageString());
    }
}

void  CurveConstraintPy::setNbPoints(Py::Long arg)
{
    try {
        getGeomPlate_CurveConstraintPtr()->SetNbPoints(static_cast<long>(arg));
    }
    catch (const Standard_Failure& e) {
        throw Py::RuntimeError(e.GetMessageString());
    }
}

Py::Float CurveConstraintPy::getFirstParameter() const
{
    try {
        Standard_Real v = getGeomPlate_CurveConstraintPtr()->FirstParameter();
        return Py::Float(v);
    }
    catch (const Standard_Failure& e) {
        throw Py::RuntimeError(e.GetMessageString());
    }
}

Py::Float CurveConstraintPy::getLastParameter() const
{
    try {
        Standard_Real v = getGeomPlate_CurveConstraintPtr()->LastParameter();
        return Py::Float(v);
    }
    catch (const Standard_Failure& e) {
        throw Py::RuntimeError(e.GetMessageString());
    }
}

Py::Float CurveConstraintPy::getLength() const
{
    try {
        Standard_Real v = getGeomPlate_CurveConstraintPtr()->Length();
        return Py::Float(v);
    }
    catch (const Standard_Failure& e) {
        throw Py::RuntimeError(e.GetMessageString());
    }
}

PyObject *CurveConstraintPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int CurveConstraintPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
