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
# include <Geom_Conic.hxx>
#endif

#include <Base/GeometryPyCXX.h>
#include <Base/VectorPy.h>

#include "ConicPy.h"
#include "ConicPy.cpp"
#include "OCCError.h"


using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string ConicPy::representation() const
{
    return "<Conic object>";
}

PyObject *ConicPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // never create such objects with the constructor
    PyErr_SetString(PyExc_RuntimeError,
        "You cannot create an instance of the abstract class 'Conic'.");
    return nullptr;
}

// constructor method
int ConicPy::PyInit(PyObject* /*args*/, PyObject* /*kwds*/)
{
    return 0;
}

Py::Object ConicPy::getCenter() const
{
    Handle(Geom_Conic) conic = Handle(Geom_Conic)::DownCast(getGeomConicPtr()->handle());
    gp_Pnt loc = conic->Location();
    return Py::Vector(Base::Vector3d(loc.X(), loc.Y(), loc.Z()));
}

Py::Object ConicPy::getLocation() const
{
    Handle(Geom_Conic) conic = Handle(Geom_Conic)::DownCast(getGeomConicPtr()->handle());
    gp_Pnt loc = conic->Location();
    return Py::Vector(Base::Vector3d(loc.X(), loc.Y(), loc.Z()));
}

void  ConicPy::setCenter(Py::Object arg)
{
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        Base::Vector3d loc = static_cast<Base::VectorPy*>(p)->value();
        getGeomConicPtr()->setLocation(loc);
    }
    else if (PyObject_TypeCheck(p, &PyTuple_Type)) {
        Base::Vector3d loc = Base::getVectorFromTuple<double>(p);
        getGeomConicPtr()->setLocation(loc);
    } else {
        std::string error = std::string("type must be 'Vector', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

void  ConicPy::setLocation(Py::Object arg)
{
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        Base::Vector3d loc = static_cast<Base::VectorPy*>(p)->value();
        getGeomConicPtr()->setLocation(loc);
    }
    else if (PyObject_TypeCheck(p, &PyTuple_Type)) {
        Base::Vector3d loc = Base::getVectorFromTuple<double>(p);
        getGeomConicPtr()->setLocation(loc);
    } else {
        std::string error = std::string("type must be 'Vector', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

Py::Float ConicPy::getEccentricity() const
{
    Handle(Geom_Conic) conic = Handle(Geom_Conic)::DownCast(getGeomConicPtr()->handle());
    return Py::Float(conic->Eccentricity());
}

Py::Float ConicPy::getAngleXU() const
{
    return Py::Float(getGeomConicPtr()->getAngleXU());
}

void ConicPy::setAngleXU(Py::Float arg)
{
    getGeomConicPtr()->setAngleXU((double)arg);
}

Py::Object ConicPy::getAxis() const
{
    Handle(Geom_Conic) conic = Handle(Geom_Conic)::DownCast(getGeomConicPtr()->handle());
    gp_Ax1 axis = conic->Axis();
    gp_Dir dir = axis.Direction();
    return Py::Vector(Base::Vector3d(dir.X(), dir.Y(), dir.Z()));
}

void  ConicPy::setAxis(Py::Object arg)
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

    Handle(Geom_Conic) conic = Handle(Geom_Conic)::DownCast(getGeomConicPtr()->handle());
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

Py::Object ConicPy::getXAxis() const
{
    Handle(Geom_Conic) conic = Handle(Geom_Conic)::DownCast(getGeomConicPtr()->handle());
    gp_Ax1 axis = conic->XAxis();
    gp_Dir dir = axis.Direction();
    return Py::Vector(Base::Vector3d(dir.X(), dir.Y(), dir.Z()));
}

void  ConicPy::setXAxis(Py::Object arg)
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

    Handle(Geom_Conic) conic = Handle(Geom_Conic)::DownCast(getGeomConicPtr()->handle());
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

Py::Object ConicPy::getYAxis() const
{
    Handle(Geom_Conic) conic = Handle(Geom_Conic)::DownCast(getGeomConicPtr()->handle());
    gp_Ax1 axis = conic->YAxis();
    gp_Dir dir = axis.Direction();
    return Py::Vector(Base::Vector3d(dir.X(), dir.Y(), dir.Z()));
}

void  ConicPy::setYAxis(Py::Object arg)
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

    Handle(Geom_Conic) conic = Handle(Geom_Conic)::DownCast(getGeomConicPtr()->handle());
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

PyObject *ConicPy::getCustomAttributes(const char* ) const
{
    return nullptr;
}

int ConicPy::setCustomAttributes(const char* , PyObject *)
{
    return 0;
}
