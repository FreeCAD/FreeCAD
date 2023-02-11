/***************************************************************************
 *   Copyright (c) 2014 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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
# include <Geom_TrimmedCurve.hxx>
#endif

#include <Base/GeometryPyCXX.h>
#include <Base/VectorPy.h>

#include "ArcOfConicPy.h"
#include "ArcOfConicPy.cpp"
#include "OCCError.h"


using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string ArcOfConicPy::representation() const
{
    return "<ArcOfConic object>";
}

PyObject *ArcOfConicPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // never create such objects with the constructor
    PyErr_SetString(PyExc_RuntimeError,
        "You cannot create an instance of the abstract class 'ArcOfConic'.");
    return nullptr;
}

// constructor method
int ArcOfConicPy::PyInit(PyObject* /*args*/, PyObject* /*kwds*/)
{
    return -1;
}

Py::Object ArcOfConicPy::getLocation() const
{
    return Py::Vector(getGeomArcOfConicPtr()->getLocation());
}

Py::Object ArcOfConicPy::getCenter() const
{
    return Py::Vector(getGeomArcOfConicPtr()->getCenter());
}

void  ArcOfConicPy::setLocation(Py::Object arg)
{
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        Base::Vector3d loc = static_cast<Base::VectorPy*>(p)->value();
        getGeomArcOfConicPtr()->setLocation(loc);
    }
    else if (PyObject_TypeCheck(p, &PyTuple_Type)) {
        Base::Vector3d loc = Base::getVectorFromTuple<double>(p);
        getGeomArcOfConicPtr()->setLocation(loc);
    }
    else {
        std::string error = std::string("type must be 'Vector', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

void  ArcOfConicPy::setCenter(Py::Object arg)
{
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        Base::Vector3d loc = static_cast<Base::VectorPy*>(p)->value();
        getGeomArcOfConicPtr()->setCenter(loc);
    }
    else if (PyObject_TypeCheck(p, &PyTuple_Type)) {
        Base::Vector3d loc = Base::getVectorFromTuple<double>(p);
        getGeomArcOfConicPtr()->setCenter(loc);
    }
    else {
        std::string error = std::string("type must be 'Vector', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

Py::Float ArcOfConicPy::getAngleXU() const
{
    return Py::Float(getGeomArcOfConicPtr()->getAngleXU());
}

void ArcOfConicPy::setAngleXU(Py::Float arg)
{
    getGeomArcOfConicPtr()->setAngleXU((double)arg);
}

Py::Object ArcOfConicPy::getAxis() const
{
    Handle(Geom_TrimmedCurve) trim = Handle(Geom_TrimmedCurve)::DownCast
        (getGeomArcOfConicPtr()->handle());
    Handle(Geom_Conic) conic = Handle(Geom_Conic)::DownCast(trim->BasisCurve());
    gp_Ax1 axis = conic->Axis();
    gp_Dir dir = axis.Direction();
    return Py::Vector(Base::Vector3d(dir.X(), dir.Y(), dir.Z()));
}

void  ArcOfConicPy::setAxis(Py::Object arg)
{
    PyObject* p = arg.ptr();
    Base::Vector3d val;
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        val = static_cast<Base::VectorPy*>(p)->value();
    }
    else if (PyTuple_Check(p)) {
        val = Base::getVectorFromTuple<double>(p);
    }
    else {
        std::string error = std::string("type must be 'Vector', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }

    Handle(Geom_TrimmedCurve) trim = Handle(Geom_TrimmedCurve)::DownCast
        (getGeomArcOfConicPtr()->handle());
    Handle(Geom_Conic) conic = Handle(Geom_Conic)::DownCast(trim->BasisCurve());
    try {
        gp_Ax1 axis;
        axis.SetLocation(conic->Location());
        axis.SetDirection(gp_Dir(val.x, val.y, val.z));
        conic->SetAxis(axis);
    }
    catch (Standard_Failure&) {
        throw Py::RuntimeError("cannot set axis");
    }
}

Py::Object ArcOfConicPy::getXAxis() const
{
    Handle(Geom_TrimmedCurve) trim = Handle(Geom_TrimmedCurve)::DownCast
        (getGeomArcOfConicPtr()->handle());
    Handle(Geom_Conic) conic = Handle(Geom_Conic)::DownCast(trim->BasisCurve());
    gp_Ax1 axis = conic->XAxis();
    gp_Dir dir = axis.Direction();
    return Py::Vector(Base::Vector3d(dir.X(), dir.Y(), dir.Z()));
}

void  ArcOfConicPy::setXAxis(Py::Object arg)
{
    PyObject* p = arg.ptr();
    Base::Vector3d val;
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        val = static_cast<Base::VectorPy*>(p)->value();
    }
    else if (PyTuple_Check(p)) {
        val = Base::getVectorFromTuple<double>(p);
    }
    else {
        std::string error = std::string("type must be 'Vector', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }

    Handle(Geom_TrimmedCurve) trim = Handle(Geom_TrimmedCurve)::DownCast
        (getGeomArcOfConicPtr()->handle());
    Handle(Geom_Conic) conic = Handle(Geom_Conic)::DownCast(trim->BasisCurve());
    try {
        gp_Ax2 pos;
        pos = conic->Position();
        pos.SetXDirection(gp_Dir(val.x, val.y, val.z));
        conic->SetPosition(pos);
    }
    catch (Standard_Failure&) {
        throw Py::RuntimeError("cannot set X axis");
    }
}

Py::Object ArcOfConicPy::getYAxis() const
{
    Handle(Geom_TrimmedCurve) trim = Handle(Geom_TrimmedCurve)::DownCast
        (getGeomArcOfConicPtr()->handle());
    Handle(Geom_Conic) conic = Handle(Geom_Conic)::DownCast(trim->BasisCurve());
    gp_Ax1 axis = conic->YAxis();
    gp_Dir dir = axis.Direction();
    return Py::Vector(Base::Vector3d(dir.X(), dir.Y(), dir.Z()));
}

void  ArcOfConicPy::setYAxis(Py::Object arg)
{
    PyObject* p = arg.ptr();
    Base::Vector3d val;
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        val = static_cast<Base::VectorPy*>(p)->value();
    }
    else if (PyTuple_Check(p)) {
        val = Base::getVectorFromTuple<double>(p);
    }
    else {
        std::string error = std::string("type must be 'Vector', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }

    Handle(Geom_TrimmedCurve) trim = Handle(Geom_TrimmedCurve)::DownCast
        (getGeomArcOfConicPtr()->handle());
    Handle(Geom_Conic) conic = Handle(Geom_Conic)::DownCast(trim->BasisCurve());
    try {
        gp_Ax2 pos;
        pos = conic->Position();
        pos.SetYDirection(gp_Dir(val.x, val.y, val.z));
        conic->SetPosition(pos);
    }
    catch (Standard_Failure&) {
        throw Py::RuntimeError("cannot set Y axis");
    }
}

PyObject *ArcOfConicPy::getCustomAttributes(const char* ) const
{
    return nullptr;
}

int ArcOfConicPy::setCustomAttributes(const char* , PyObject *)
{
    return 0;
}
