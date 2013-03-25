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
# include <Geom_Parabola.hxx>
#endif

#include <Base/VectorPy.h>
#include <Base/GeometryPyCXX.h>

#include "Geometry.h"
#include "ParabolaPy.h"
#include "ParabolaPy.cpp"

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string ParabolaPy::representation(void) const
{
    return "<Parabola object>";
}

PyObject *ParabolaPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of ParabolaPy and the Twin object 
    return new ParabolaPy(new GeomParabola);
}

// constructor method
int ParabolaPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    if (PyArg_ParseTuple(args, "")) {
        Handle_Geom_Parabola c = Handle_Geom_Parabola::DownCast
            (getGeometryPtr()->handle());
        c->SetFocal(1.0);
        return 0;
    }

    return -1;
}

PyObject* ParabolaPy::compute(PyObject *args)
{
    PyObject *p1, *p2, *p3;
    if (!PyArg_ParseTuple(args, "O!O!O!",
        &Base::VectorPy::Type,&p1,
        &Base::VectorPy::Type,&p2,
        &Base::VectorPy::Type,&p3))
        return 0;
    Base::Vector3d v1 = Py::Vector(p1,false).toVector();
    Base::Vector3d v2 = Py::Vector(p2,false).toVector();
    Base::Vector3d v3 = Py::Vector(p3,false).toVector();
    Base::Vector3d c = (v1-v2) % (v3-v2);
    double zValue = v1.z;
    if (fabs(c.Length()) < 0.0001) {
        PyErr_SetString(PyExc_Exception, "Points are collinear");
        return 0;
    }

    Base::Matrix4D m;
    Base::Vector3d v;
    m[0][0] = v1.y * v1.y;
    m[0][1] = v1.y;
    m[0][2] = 1;
    m[1][0] = v2.y * v2.y;
    m[1][1] = v2.y;
    m[1][2] = 1;
    m[2][0] = v3.y * v3.y;
    m[2][1] = v3.y;
    m[2][2] = 1.0;
    v.x = v1.x;
    v.y = v2.x;
    v.z = v3.x;
    m.inverseGauss();
    v = m * v;
    double a22 = v.x;
    double a10 = -0.5;
    double a20 = v.y/2.0;
    double a00 = v.z;
    Handle_Geom_Parabola curve = Handle_Geom_Parabola::DownCast(getGeometryPtr()->handle());
    curve->SetFocal(0.5*fabs(a10/a22));
    curve->SetLocation(gp_Pnt((a20*a20-a22*a00)/(2*a22*a10), -a20/a22, zValue));

    Py_Return;
}

Py::Float ParabolaPy::getEccentricity(void) const
{
    Handle_Geom_Parabola curve = Handle_Geom_Parabola::DownCast(getGeometryPtr()->handle());
    return Py::Float(curve->Eccentricity()); 
}

Py::Float ParabolaPy::getFocal(void) const
{
    Handle_Geom_Parabola curve = Handle_Geom_Parabola::DownCast(getGeometryPtr()->handle());
    return Py::Float(curve->Focal()); 
}

void ParabolaPy::setFocal(Py::Float arg)
{
    Handle_Geom_Parabola curve = Handle_Geom_Parabola::DownCast(getGeometryPtr()->handle());
    curve->SetFocal((double)arg); 
}

Py::Object ParabolaPy::getFocus(void) const
{
    Handle_Geom_Parabola c = Handle_Geom_Parabola::DownCast
        (getGeometryPtr()->handle());
    gp_Pnt loc = c->Focus();
    return Py::Vector(Base::Vector3d(loc.X(), loc.Y(), loc.Z()));
}

Py::Float ParabolaPy::getParameter(void) const
{
    Handle_Geom_Parabola curve = Handle_Geom_Parabola::DownCast(getGeometryPtr()->handle());
    return Py::Float(curve->Parameter()); 
}

Py::Object ParabolaPy::getLocation(void) const
{
    Handle_Geom_Parabola c = Handle_Geom_Parabola::DownCast
        (getGeometryPtr()->handle());
    gp_Pnt loc = c->Location();
    return Py::Vector(Base::Vector3d(loc.X(), loc.Y(), loc.Z()));
}

void ParabolaPy::setLocation(Py::Object arg)
{
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        Base::Vector3d loc = static_cast<Base::VectorPy*>(p)->value();
        Handle_Geom_Parabola c = Handle_Geom_Parabola::DownCast
            (getGeometryPtr()->handle());
        c->SetLocation(gp_Pnt(loc.x, loc.y, loc.z));
    }
    else if (PyTuple_Check(p)) {
        Py::Tuple tuple(arg);
        gp_Pnt loc;
        loc.SetX((double)Py::Float(tuple.getItem(0)));
        loc.SetY((double)Py::Float(tuple.getItem(1)));
        loc.SetZ((double)Py::Float(tuple.getItem(2)));
        Handle_Geom_Parabola c = Handle_Geom_Parabola::DownCast
            (getGeometryPtr()->handle());
        c->SetLocation(loc);
    }
    else {
        std::string error = std::string("type must be 'Vector', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

Py::Object ParabolaPy::getAxis(void) const
{
    Handle_Geom_Parabola c = Handle_Geom_Parabola::DownCast
        (getGeometryPtr()->handle());
    gp_Dir dir = c->Axis().Direction();
    return Py::Vector(Base::Vector3d(dir.X(), dir.Y(), dir.Z()));
}

void ParabolaPy::setAxis(Py::Object arg)
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
        Handle_Geom_Parabola this_curv = Handle_Geom_Parabola::DownCast
            (this->getGeometryPtr()->handle());
        gp_Ax1 axis;
        axis.SetLocation(this_curv->Location());
        axis.SetDirection(gp_Dir(dir_x, dir_y, dir_z));
        this_curv->SetAxis(axis);
    }
    catch (Standard_Failure) {
        throw Py::Exception("cannot set axis");
    }
}

PyObject *ParabolaPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int ParabolaPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


