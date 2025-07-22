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

#include "Geom2d/OffsetCurve2dPy.h"
#include "Geom2d/OffsetCurve2dPy.cpp"
#include "OCCError.h"


using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string OffsetCurve2dPy::representation() const
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
    Handle(Geom2d_Curve) curve = Handle(Geom2d_Curve)::DownCast
        (pcGeo->getGeometry2dPtr()->handle());
    if (curve.IsNull()) {
        PyErr_SetString(PyExc_TypeError, "geometry is not a curve");
        return -1;
    }

    try {
        Handle(Geom2d_OffsetCurve) curve2 = new Geom2d_OffsetCurve(curve, offset);
        getGeom2dOffsetCurvePtr()->setHandle(curve2);
        return 0;
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return -1;
    }
}

Py::Float OffsetCurve2dPy::getOffsetValue() const
{
    Handle(Geom2d_OffsetCurve) curve = Handle(Geom2d_OffsetCurve)::DownCast(getGeometry2dPtr()->handle());
    return Py::Float(curve->Offset());
}

void OffsetCurve2dPy::setOffsetValue(Py::Float arg)
{
    Handle(Geom2d_OffsetCurve) curve = Handle(Geom2d_OffsetCurve)::DownCast(getGeometry2dPtr()->handle());
    curve->SetOffsetValue((double)arg);
}

Py::Object OffsetCurve2dPy::getBasisCurve() const
{
    Handle(Geom2d_OffsetCurve) curve = Handle(Geom2d_OffsetCurve)::DownCast(getGeometry2dPtr()->handle());
    Handle(Geom2d_Curve) basis = curve->BasisCurve();
    if (basis.IsNull())
        return Py::None();
    std::unique_ptr<Geom2dCurve> geo2d = makeFromCurve2d(basis);
    if (!geo2d)
        throw Py::RuntimeError("Unknown curve type");
    return Py::asObject(geo2d->getPyObject());
}

void OffsetCurve2dPy::setBasisCurve(Py::Object arg)
{
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Curve2dPy::Type))) {
        Curve2dPy* pcGeo = static_cast<Curve2dPy*>(p);
        Handle(Geom2d_Curve) curve = Handle(Geom2d_Curve)::DownCast
            (pcGeo->getGeometry2dPtr()->handle());
        if (curve.IsNull()) {
            throw Py::TypeError("geometry is not a curve");
        }

        Handle(Geom2d_OffsetCurve) curve2 = Handle(Geom2d_OffsetCurve)::DownCast
            (getGeometry2dPtr()->handle());
        if (curve == curve2) {
            throw Py::RuntimeError("cannot set this curve as basis");
        }

        try {
            curve2->SetBasisCurve(curve);
        }
        catch (Standard_Failure& e) {
            throw Py::RuntimeError(e.GetMessageString());
        }
    }
}

PyObject *OffsetCurve2dPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int OffsetCurve2dPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
