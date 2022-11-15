/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <Geom_OffsetCurve.hxx>
#endif

#include <Base/GeometryPyCXX.h>
#include <Base/Vector3D.h>
#include <Base/VectorPy.h>

#include "OffsetCurvePy.h"
#include "OffsetCurvePy.cpp"
#include "Geometry.h"
#include "OCCError.h"


using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string OffsetCurvePy::representation() const
{
    return "<OffsetCurve object>";
}

PyObject *OffsetCurvePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of OffsetCurvePy and the Twin object
    return new OffsetCurvePy(new GeomOffsetCurve);
}

// constructor method
int OffsetCurvePy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    PyObject* pGeom;
    PyObject* pDir;
    double offset;
    if (!PyArg_ParseTuple(args, "O!dO!",
                            &(GeometryPy::Type), &pGeom,
                            &offset,
                            &(Base::VectorPy::Type),&pDir))
        return -1;

    GeometryPy* pcGeo = static_cast<GeometryPy*>(pGeom);
    Handle(Geom_Curve) curve = Handle(Geom_Curve)::DownCast
        (pcGeo->getGeometryPtr()->handle());
    if (curve.IsNull()) {
        PyErr_SetString(PyExc_TypeError, "geometry is not a curve");
        return -1;
    }

    try {
        Base::Vector3d dir = static_cast<Base::VectorPy*>(pDir)->value();
        Handle(Geom_OffsetCurve) curve2 = new Geom_OffsetCurve(curve, offset, gp_Dir(dir.x,dir.y,dir.z));
        getGeomOffsetCurvePtr()->setHandle(curve2);
        return 0;
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return -1;
    }
}

Py::Float OffsetCurvePy::getOffsetValue() const
{
    Handle(Geom_OffsetCurve) curve = Handle(Geom_OffsetCurve)::DownCast(getGeometryPtr()->handle());
    return Py::Float(curve->Offset());
}

void OffsetCurvePy::setOffsetValue(Py::Float arg)
{
    Handle(Geom_OffsetCurve) curve = Handle(Geom_OffsetCurve)::DownCast(getGeometryPtr()->handle());
    curve->SetOffsetValue((double)arg);
}

Py::Object OffsetCurvePy::getOffsetDirection() const
{
    Handle(Geom_OffsetCurve) curve = Handle(Geom_OffsetCurve)::DownCast(getGeometryPtr()->handle());
    const gp_Dir& dir = curve->Direction();
    return Py::Vector(Base::Vector3d(dir.X(),dir.Y(),dir.Z()));
}

void OffsetCurvePy::setOffsetDirection(Py::Object arg)
{
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        Base::Vector3d dir = static_cast<Base::VectorPy*>(p)->value();
        Handle(Geom_OffsetCurve) curve = Handle(Geom_OffsetCurve)::DownCast(getGeometryPtr()->handle());
        curve->SetDirection(gp_Dir(dir.x,dir.y,dir.z));
    }
    else if (PyObject_TypeCheck(p, &PyTuple_Type)) {
        Base::Vector3d dir = Base::getVectorFromTuple<double>(p);
        Handle(Geom_OffsetCurve) curve = Handle(Geom_OffsetCurve)::DownCast(getGeometryPtr()->handle());
        curve->SetDirection(gp_Dir(dir.x,dir.y,dir.z));
    }
    else {
        std::string error = std::string("type must be 'Vector', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

Py::Object OffsetCurvePy::getBasisCurve() const
{
    Handle(Geom_OffsetCurve) curve = Handle(Geom_OffsetCurve)::DownCast(getGeometryPtr()->handle());
    Handle(Geom_Curve) basis = curve->BasisCurve();
    std::unique_ptr<GeomCurve> ptr(Part::makeFromCurve(basis));
    return Py::asObject(ptr->getPyObject());
}

void OffsetCurvePy::setBasisCurve(Py::Object arg)
{
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(GeometryPy::Type))) {
        GeometryPy* pcGeo = static_cast<GeometryPy*>(p);
        Handle(Geom_Curve) curve = Handle(Geom_Curve)::DownCast
            (pcGeo->getGeometryPtr()->handle());
        if (curve.IsNull()) {
            throw Py::TypeError("geometry is not a curve");
        }

        try {
            Handle(Geom_OffsetCurve) curve2 = Handle(Geom_OffsetCurve)::DownCast
                (getGeometryPtr()->handle());
            curve2->SetBasisCurve(curve);
        }
        catch (Standard_Failure& e) {
                throw Py::RuntimeError(e.GetMessageString());
        }
    }
}

PyObject *OffsetCurvePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int OffsetCurvePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
