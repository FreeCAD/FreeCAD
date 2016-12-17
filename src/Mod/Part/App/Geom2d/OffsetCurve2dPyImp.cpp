/***************************************************************************
 *   Copyright (c) 2016 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <Geom2d_OffsetCurve.hxx>
#endif

#include <Mod/Part/App/OCCError.h>
#include <Mod/Part/App/Geometry2d.h>
#include <Mod/Part/App/Geom2d/OffsetCurve2dPy.h>
#include <Mod/Part/App/Geom2d/OffsetCurve2dPy.cpp>

#include <Base/GeometryPyCXX.h>

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string OffsetCurve2dPy::representation(void) const
{
    return "<OffsetCurve2d object>";
}

PyObject *OffsetCurve2dPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of OffsetCurve2dPy and the Twin object
    return new OffsetCurve2dPy(new Geom2dOffsetCurve);
}

// constructor method
int OffsetCurve2dPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    PyObject* pGeom;
    double offset;
    if (!PyArg_ParseTuple(args, "O!d",
                            &(Curve2dPy::Type), &pGeom,
                            &offset))
        return -1;

    Curve2dPy* pcGeo = static_cast<Curve2dPy*>(pGeom);
    Handle_Geom2d_Curve curve = Handle_Geom2d_Curve::DownCast
        (pcGeo->getGeometry2dPtr()->handle());
    if (curve.IsNull()) {
        PyErr_SetString(PyExc_TypeError, "geometry is not a curve");
        return -1;
    }

    try {
        Handle_Geom2d_OffsetCurve curve2 = new Geom2d_OffsetCurve(curve, offset);
        getGeom2dOffsetCurvePtr()->setHandle(curve2);
        return 0;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PartExceptionOCCError, e->GetMessageString());
        return -1;
    }
}

Py::Float OffsetCurve2dPy::getOffsetValue(void) const
{
    Handle_Geom2d_OffsetCurve curve = Handle_Geom2d_OffsetCurve::DownCast(getGeometry2dPtr()->handle());
    return Py::Float(curve->Offset());
}

void OffsetCurve2dPy::setOffsetValue(Py::Float arg)
{
    Handle_Geom2d_OffsetCurve curve = Handle_Geom2d_OffsetCurve::DownCast(getGeometry2dPtr()->handle());
    curve->SetOffsetValue((double)arg);
}

Py::Object OffsetCurve2dPy::getBasisCurve(void) const
{
    Handle_Geom2d_OffsetCurve curve = Handle_Geom2d_OffsetCurve::DownCast(getGeometry2dPtr()->handle());
    Handle_Geom2d_Curve basis = curve->BasisCurve();
    if (basis.IsNull())
        return Py::None();
    if (basis->IsKind(STANDARD_TYPE (Geom2d_Parabola))) {
        Geom2dParabola c(Handle_Geom2d_Parabola::DownCast(basis));
        return Py::asObject(c.getPyObject());
    }
    if (basis->IsKind(STANDARD_TYPE (Geom2d_Hyperbola))) {
        Geom2dHyperbola c(Handle_Geom2d_Hyperbola::DownCast(basis));
        return Py::asObject(c.getPyObject());
    }
    if (basis->IsKind(STANDARD_TYPE (Geom2d_Ellipse))) {
        Geom2dEllipse c(Handle_Geom2d_Ellipse::DownCast(basis));
        return Py::asObject(c.getPyObject());
    }
    if (basis->IsKind(STANDARD_TYPE (Geom2d_Circle))) {
        Geom2dCircle c(Handle_Geom2d_Circle::DownCast(basis));
        return Py::asObject(c.getPyObject());
    }
    if (basis->IsKind(STANDARD_TYPE (Geom2d_Line))) {
        Geom2dLine c(Handle_Geom2d_Line::DownCast(basis));
        return Py::asObject(c.getPyObject());
    }
    if (basis->IsKind(STANDARD_TYPE (Geom2d_BSplineCurve))) {
        Geom2dBSplineCurve c(Handle_Geom2d_BSplineCurve::DownCast(basis));
        return Py::asObject(c.getPyObject());
    }
    if (basis->IsKind(STANDARD_TYPE (Geom2d_BezierCurve))) {
        Geom2dBezierCurve c(Handle_Geom2d_BezierCurve::DownCast(basis));
        return Py::asObject(c.getPyObject());
    }
    if (basis->IsKind(STANDARD_TYPE (Geom2d_TrimmedCurve))) {
        Geom2dTrimmedCurve c(Handle_Geom2d_TrimmedCurve::DownCast(basis));
        return Py::asObject(c.getPyObject());
    }
    throw Py::RuntimeError("Unknown curve type");
}

void OffsetCurve2dPy::setBasisCurve(Py::Object arg)
{
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Curve2dPy::Type))) {
        Curve2dPy* pcGeo = static_cast<Curve2dPy*>(p);
        Handle_Geom2d_Curve curve = Handle_Geom2d_Curve::DownCast
            (pcGeo->getGeometry2dPtr()->handle());
        if (curve.IsNull()) {
            throw Py::TypeError("geometry is not a curve");
        }

        Handle_Geom2d_OffsetCurve curve2 = Handle_Geom2d_OffsetCurve::DownCast
            (getGeometry2dPtr()->handle());
        if (curve == curve2) {
            throw Py::RuntimeError("cannot set this curve as basis");
        }

        try {
            curve2->SetBasisCurve(curve);
        }
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            throw Py::Exception(e->GetMessageString());
        }
    }
}

PyObject *OffsetCurve2dPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int OffsetCurve2dPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
