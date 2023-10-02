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
# include <Geom2d_Conic.hxx>
#endif

#include <Base/GeometryPyCXX.h>

#include "Geom2d/Conic2dPy.h"
#include "Geom2d/Conic2dPy.cpp"
#include "OCCError.h"


using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string Conic2dPy::representation() const
{
    return "<Conic2d object>";
}

PyObject *Conic2dPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // never create such objects with the constructor
    PyErr_SetString(PyExc_RuntimeError,
        "You cannot create an instance of the abstract class 'Conic2d'.");
    return nullptr;
}

// constructor method
int Conic2dPy::PyInit(PyObject* /*args*/, PyObject* /*kwds*/)
{
    return 0;
}

Py::Object Conic2dPy::getLocation() const
{
    Base::Vector2d loc = getGeom2dConicPtr()->getLocation();
    return Base::Vector2dPy::create(loc);
}

void  Conic2dPy::setLocation(Py::Object arg)
{
    Base::Vector2d loc = Py::toVector2d(arg.ptr());
    getGeom2dConicPtr()->setLocation(loc);
}

Py::Float Conic2dPy::getEccentricity() const
{
    Handle(Geom2d_Conic) conic = Handle(Geom2d_Conic)::DownCast(getGeom2dConicPtr()->handle());
    return Py::Float(conic->Eccentricity());
}

Py::Object Conic2dPy::getXAxis() const
{
    Handle(Geom2d_Conic) conic = Handle(Geom2d_Conic)::DownCast(getGeom2dConicPtr()->handle());
    gp_Dir2d xdir = conic->XAxis().Direction();
    return Base::Vector2dPy::create(xdir.X(), xdir.Y());
}

void  Conic2dPy::setXAxis(Py::Object arg)
{
    Base::Vector2d dir = Py::toVector2d(arg.ptr());
    Handle(Geom2d_Conic) conic = Handle(Geom2d_Conic)::DownCast(getGeom2dConicPtr()->handle());
    gp_Ax2d xaxis = conic->XAxis();
    xaxis.SetDirection(gp_Dir2d(dir.x, dir.y));
    conic->SetXAxis(xaxis);
}

Py::Object Conic2dPy::getYAxis() const
{
    Handle(Geom2d_Conic) conic = Handle(Geom2d_Conic)::DownCast(getGeom2dConicPtr()->handle());
    gp_Dir2d ydir = conic->YAxis().Direction();
    return Base::Vector2dPy::create(ydir.X(), ydir.Y());
}

void  Conic2dPy::setYAxis(Py::Object arg)
{
    Base::Vector2d dir = Py::toVector2d(arg.ptr());
    Handle(Geom2d_Conic) conic = Handle(Geom2d_Conic)::DownCast(getGeom2dConicPtr()->handle());
    gp_Ax2d yaxis = conic->YAxis();
    yaxis.SetDirection(gp_Dir2d(dir.x, dir.y));
    conic->SetYAxis(yaxis);
}

PyObject *Conic2dPy::getCustomAttributes(const char* ) const
{
    return nullptr;
}

int Conic2dPy::setCustomAttributes(const char* , PyObject *)
{
    return 0;
}
