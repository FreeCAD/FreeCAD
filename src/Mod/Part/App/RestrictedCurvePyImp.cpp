// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2025 Ajinkya P. Dahale <dahale.a.p[at]gmail.com>        *
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

#include <Base/GeometryPyCXX.h>
#include <Base/Vector3D.h>
#include <Base/VectorPy.h>

#include "RestrictedCurvePy.h"
#include "RestrictedCurvePy.cpp"
#include "Geometry.h"
#include "OCCError.h"


using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string RestrictedCurvePy::representation() const
{
    return "<RestrictedCurve object>";
}

PyObject* RestrictedCurvePy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    // create a new instance of RestrictedCurvePy and the Twin object
    return new RestrictedCurvePy(new GeomRestrictedCurve);
}

// constructor method
int RestrictedCurvePy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    PyObject* pGeom;
    double firstParam;
    double lastParam;
    if (!PyArg_ParseTuple(args, "O!dd", &(GeometryPy::Type), &pGeom, &firstParam, &lastParam)) {
        return -1;
    }

    GeometryPy* pcGeo = static_cast<GeometryPy*>(pGeom);
    auto* curve = dynamic_cast<GeomCurve*>(pcGeo->getGeometryPtr());
    if (curve == nullptr) {
        PyErr_SetString(PyExc_TypeError, "geometry is not a curve");
        return -1;
    }

    try {
        Handle(Geom_Curve) curveHandle = Handle(Geom_Curve)::DownCast(curve->handle());
        Handle(Geom_TrimmedCurve) curve2 = new Geom_TrimmedCurve(curveHandle, firstParam, lastParam);
        getGeomRestrictedCurvePtr()->setHandle(curve2);
        return 0;
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return -1;
    }
}

Py::Float RestrictedCurvePy::getFirstParam() const
{
    auto* curve = dynamic_cast<GeomRestrictedCurve*>(getGeometryPtr());
    return Py::Float(curve->getFirstParameter());
}

void RestrictedCurvePy::setFirstParam(Py::Float arg)
{
    auto* curve = dynamic_cast<GeomRestrictedCurve*>(getGeometryPtr());
    double newFirst;
    double newLast;
    curve->getRange(newFirst, newLast);
    newFirst = (double)arg;
    curve->setRange(newFirst, newLast);
}

Py::Float RestrictedCurvePy::getLastParam() const
{
    auto* curve = dynamic_cast<GeomRestrictedCurve*>(getGeometryPtr());
    return Py::Float(curve->getLastParameter());
}

void RestrictedCurvePy::setLastParam(Py::Float arg)
{
    auto* curve = dynamic_cast<GeomRestrictedCurve*>(getGeometryPtr());
    double newFirst;
    double newLast;
    curve->getRange(newFirst, newLast);
    newLast = (double)arg;
    curve->setRange(newFirst, newLast);
}

Py::Object RestrictedCurvePy::getBasisCurve() const
{
    Handle(Geom_TrimmedCurve) curve = Handle(Geom_TrimmedCurve)::DownCast(getGeometryPtr()->handle());
    Handle(Geom_Curve) basis = curve->BasisCurve();
    std::unique_ptr<GeomCurve> ptr(Part::makeFromCurve(basis));
    return Py::asObject(ptr->getPyObject());
}

void RestrictedCurvePy::setBasisCurve(Py::Object arg)
{
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(GeometryPy::Type))) {
        GeometryPy* pcGeo = static_cast<GeometryPy*>(p);
        Handle(Geom_Curve) curve = Handle(Geom_Curve)::DownCast(pcGeo->getGeometryPtr()->handle());
        if (curve.IsNull()) {
            throw Py::TypeError("geometry is not a curve");
        }

        try {
            auto* myCurve = dynamic_cast<GeomRestrictedCurve*>(getGeometryPtr());
            double firstParam;
            double lastParam;
            myCurve->getRange(firstParam, lastParam);
            Handle(Geom_TrimmedCurve) curve2 = new Geom_TrimmedCurve(curve, firstParam, lastParam);

            myCurve->setHandle(curve2);
        }
        catch (Standard_Failure& e) {
            throw Py::RuntimeError(e.GetMessageString());
        }
    }
}

PyObject* RestrictedCurvePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int RestrictedCurvePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
