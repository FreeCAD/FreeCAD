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
# include <Geom2d_Parabola.hxx>
#endif

#include <Base/GeometryPyCXX.h>

#include "Geom2d/Parabola2dPy.h"
#include "Geom2d/Parabola2dPy.cpp"
#include "OCCError.h"


using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string Parabola2dPy::representation() const
{
    return "<Parabola2d object>";
}

PyObject *Parabola2dPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of Parabola2dPy and the Twin object
    return new Parabola2dPy(new Geom2dParabola);
}

// constructor method
int Parabola2dPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    if (PyArg_ParseTuple(args, "")) {
        Handle(Geom2d_Parabola) c = Handle(Geom2d_Parabola)::DownCast
            (getGeometry2dPtr()->handle());
        c->SetFocal(1.0);
        return 0;
    }

    return -1;
}

Py::Float Parabola2dPy::getFocal() const
{
    Handle(Geom2d_Parabola) curve = Handle(Geom2d_Parabola)::DownCast(getGeometry2dPtr()->handle());
    return Py::Float(curve->Focal());
}

void Parabola2dPy::setFocal(Py::Float arg)
{
    Handle(Geom2d_Parabola) curve = Handle(Geom2d_Parabola)::DownCast(getGeometry2dPtr()->handle());
    curve->SetFocal((double)arg);
}

Py::Object Parabola2dPy::getFocus() const
{
    Handle(Geom2d_Parabola) curve = Handle(Geom2d_Parabola)::DownCast(getGeometry2dPtr()->handle());
    gp_Pnt2d loc = curve->Focus();
    return Base::Vector2dPy::create(loc.X(), loc.Y());
}

Py::Float Parabola2dPy::getParameter() const
{
    Handle(Geom2d_Parabola) curve = Handle(Geom2d_Parabola)::DownCast(getGeometry2dPtr()->handle());
    return Py::Float(curve->Parameter());
}

PyObject *Parabola2dPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int Parabola2dPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
