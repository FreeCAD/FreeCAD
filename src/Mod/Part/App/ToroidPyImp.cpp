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
# include <Geom_ToroidalSurface.hxx>
# include <Standard_Failure.hxx>
#endif

#include <Base/GeometryPyCXX.h>
#include <Base/VectorPy.h>

#include "OCCError.h"
#include "ToroidPy.h"
#include "ToroidPy.cpp"


using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string ToroidPy::representation() const
{
    return "<Toroid object>";
}

PyObject *ToroidPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of ToroidPy and the Twin object
    return new ToroidPy(new GeomToroid);
}

// constructor method
int ToroidPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    if (PyArg_ParseTuple(args, "")) {
        Handle(Geom_ToroidalSurface) torus = Handle(Geom_ToroidalSurface)::DownCast
            (getGeomToroidPtr()->handle());
        torus->SetMajorRadius(5.0);
        torus->SetMinorRadius(1.0);
        return 0;
    }

    return -1;
}

Py::Float ToroidPy::getMajorRadius() const
{
    Handle(Geom_ToroidalSurface) torus = Handle(Geom_ToroidalSurface)::DownCast
        (getGeomToroidPtr()->handle());
    return Py::Float(torus->MajorRadius());
}

void ToroidPy::setMajorRadius(Py::Float arg)
{
    try {
        Handle(Geom_ToroidalSurface) torus = Handle(Geom_ToroidalSurface)::DownCast
            (getGeomToroidPtr()->handle());
        torus->SetMajorRadius((double)arg);
    }
    catch (Standard_Failure&) {
        throw Py::RuntimeError("Major radius must be positive and higher than minor radius");
    }
}

Py::Float ToroidPy::getMinorRadius() const
{
    Handle(Geom_ToroidalSurface) torus = Handle(Geom_ToroidalSurface)::DownCast
        (getGeomToroidPtr()->handle());
    return Py::Float(torus->MinorRadius());
}

void ToroidPy::setMinorRadius(Py::Float arg)
{
    try {
        Handle(Geom_ToroidalSurface) torus = Handle(Geom_ToroidalSurface)::DownCast
            (getGeomToroidPtr()->handle());
        torus->SetMinorRadius((double)arg);
    }
    catch (Standard_Failure&) {
        throw Py::RuntimeError("Minor radius must be positive and lower than major radius");
    }
}

Py::Object ToroidPy::getCenter() const
{
    Handle(Geom_ToroidalSurface) torus = Handle(Geom_ToroidalSurface)::DownCast
        (getGeomToroidPtr()->handle());
    gp_Pnt loc = torus->Location();
    return Py::Vector(Base::Vector3d(loc.X(), loc.Y(), loc.Z()));
}

void ToroidPy::setCenter(Py::Object arg)
{
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        Base::Vector3d loc = static_cast<Base::VectorPy*>(p)->value();
        Handle(Geom_ToroidalSurface) torus = Handle(Geom_ToroidalSurface)::DownCast
            (getGeomToroidPtr()->handle());
        torus->SetLocation(gp_Pnt(loc.x, loc.y, loc.z));
    }
    else {
        std::string error = std::string("type must be 'Vector', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

Py::Object ToroidPy::getAxis() const
{
    Handle(Geom_ElementarySurface) s = Handle(Geom_ElementarySurface)::DownCast
        (getGeometryPtr()->handle());
    gp_Dir dir = s->Axis().Direction();
    return Py::Vector(Base::Vector3d(dir.X(), dir.Y(), dir.Z()));
}

void ToroidPy::setAxis(Py::Object arg)
{
    Standard_Real dir_x, dir_y, dir_z;
    PyObject *p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        Base::Vector3d v = static_cast<Base::VectorPy*>(p)->value();
        dir_x = v.x;
        dir_y = v.y;
        dir_z = v.z;
    }
    else if (PyTuple_Check(p)) {
        Py::Tuple tuple(arg);
        dir_x = (double)Py::Float(tuple.getItem(0));
        dir_y = (double)Py::Float(tuple.getItem(1));
        dir_z = (double)Py::Float(tuple.getItem(2));
    }
    else {
        std::string error = std::string("type must be 'Vector' or tuple, not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }

    try {
        Handle(Geom_ElementarySurface) this_surf = Handle(Geom_ElementarySurface)::DownCast
            (this->getGeometryPtr()->handle());
        gp_Ax1 axis;
        axis.SetLocation(this_surf->Location());
        axis.SetDirection(gp_Dir(dir_x, dir_y, dir_z));
        this_surf->SetAxis(axis);
    }
    catch (Standard_Failure&) {
        throw Py::RuntimeError("cannot set axis");
    }
}

Py::Float ToroidPy::getArea() const
{
    Handle(Geom_ToroidalSurface) torus = Handle(Geom_ToroidalSurface)::DownCast
        (getGeomToroidPtr()->handle());
    return Py::Float(torus->Area());
}

Py::Float ToroidPy::getVolume() const
{
    Handle(Geom_ToroidalSurface) torus = Handle(Geom_ToroidalSurface)::DownCast
        (getGeomToroidPtr()->handle());
    return Py::Float(torus->Volume());
}

PyObject *ToroidPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ToroidPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
