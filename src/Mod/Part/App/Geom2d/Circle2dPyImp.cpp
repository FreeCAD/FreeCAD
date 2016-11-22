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
# include <gp_Circ.hxx>
# include <Geom_Circle.hxx>
# include <GC_MakeCircle.hxx>
#endif

#include <Mod/Part/App/OCCError.h>
#include <Mod/Part/App/Geom2d/Circle2dPy.h>
#include <Mod/Part/App/Geom2d/Circle2dPy.cpp>

#include <Base/GeometryPyCXX.h>
#include <Base/VectorPy.h>

using namespace Part;

extern const char* gce_ErrorStatusText(gce_ErrorType et);

// returns a string which represents the object e.g. when printed in python
std::string Circle2dPy::representation(void) const
{
#if 0
    Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(getGeomCirclePtr()->handle());
    gp_Ax1 axis = circle->Axis();
    gp_Dir dir = axis.Direction();
    gp_Pnt loc = axis.Location();
    Standard_Real fRad = circle->Radius();

    std::stringstream str;
    str << "Circle (";
    str << "Radius : " << fRad << ", "; 
    str << "Position : (" << loc.X() << ", "<< loc.Y() << ", "<< loc.Z() << "), "; 
    str << "Direction : (" << dir.X() << ", "<< dir.Y() << ", "<< dir.Z() << ")"; 
    str << ")";

    return str.str();
#else
    return "";
#endif
}

PyObject *Circle2dPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
#if 1
    return 0;
#else
    // create a new instance of Circle2dPy and the Twin object
    Handle_Geom_Circle circle = new Geom_Circle(gp_Circ());
    return new Circle2dPy(new GeomCircle(circle));
#endif
}

// constructor method
int Circle2dPy::PyInit(PyObject* args, PyObject* kwds)
{
    return 0;
#if 0
    // circle and distance for offset
    PyObject *pCirc;
    double dist;
    static char* keywords_cd[] = {"Circle","Distance",NULL};
    if (PyArg_ParseTupleAndKeywords(args, kwds, "O!d", keywords_cd, &(Circle2dPy::Type), &pCirc, &dist)) {
        Circle2dPy* pcCircle = static_cast<Circle2dPy*>(pCirc);
        Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast
            (pcCircle->getGeomCirclePtr()->handle());
        GC_MakeCircle mc(circle->Circ(), dist);
        if (!mc.IsDone()) {
            PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(mc.Status()));
            return -1;
        }

        Handle_Geom_Circle circ = Handle_Geom_Circle::DownCast(getGeomCirclePtr()->handle());
        circ->SetCirc(mc.Value()->Circ());
        return 0;
    }

    // center, normal and radius
    PyObject *pV1, *pV2, *pV3;
    static char* keywords_cnr[] = {"Center","Normal","Radius",NULL};
    PyErr_Clear();
    if (PyArg_ParseTupleAndKeywords(args, kwds, "O!O!d", keywords_cnr,
                                        &(Base::VectorPy::Type), &pV1,
                                        &(Base::VectorPy::Type), &pV2,
                                        &dist)) {
        Base::Vector3d v1 = static_cast<Base::VectorPy*>(pV1)->value();
        Base::Vector3d v2 = static_cast<Base::VectorPy*>(pV2)->value();
        GC_MakeCircle mc(gp_Pnt(v1.x,v1.y,v1.z),
                         gp_Dir(v2.x,v2.y,v2.z),
                         dist);
        if (!mc.IsDone()) {
            PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(mc.Status()));
            return -1;
        }

        Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(getGeomCirclePtr()->handle());
        circle->SetCirc(mc.Value()->Circ());
        return 0;
    }

    static char* keywords_c[] = {"Circle",NULL};
    PyErr_Clear();
    if (PyArg_ParseTupleAndKeywords(args, kwds, "O!", keywords_c, &(Circle2dPy::Type), &pCirc)) {
        Circle2dPy* pcCircle = static_cast<Circle2dPy*>(pCirc);
        Handle_Geom_Circle circ1 = Handle_Geom_Circle::DownCast
            (pcCircle->getGeomCirclePtr()->handle());
        Handle_Geom_Circle circ2 = Handle_Geom_Circle::DownCast
            (this->getGeomCirclePtr()->handle());
        circ2->SetCirc(circ1->Circ());
        return 0;
    }

    static char* keywords_ppp[] = {"Point1","Point2","Point3",NULL};
    PyErr_Clear();
    if (PyArg_ParseTupleAndKeywords(args, kwds, "O!O!O!", keywords_ppp,
                                         &(Base::VectorPy::Type), &pV1,
                                         &(Base::VectorPy::Type), &pV2,
                                         &(Base::VectorPy::Type), &pV3)) {
        Base::Vector3d v1 = static_cast<Base::VectorPy*>(pV1)->value();
        Base::Vector3d v2 = static_cast<Base::VectorPy*>(pV2)->value();
        Base::Vector3d v3 = static_cast<Base::VectorPy*>(pV3)->value();
        GC_MakeCircle mc(gp_Pnt(v1.x,v1.y,v1.z),
                         gp_Pnt(v2.x,v2.y,v2.z),
                         gp_Pnt(v3.x,v3.y,v3.z));
        if (!mc.IsDone()) {
            PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(mc.Status()));
            return -1;
        }

        Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(getGeomCirclePtr()->handle());
        circle->SetCirc(mc.Value()->Circ());
        return 0;
    }

    // default circle
    static char* keywords_n[] = {NULL};
    PyErr_Clear();
    if (PyArg_ParseTupleAndKeywords(args, kwds, "", keywords_n)) {
        Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(getGeomCirclePtr()->handle());
        circle->SetRadius(1.0);
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "Circle constructor accepts:\n"
        "-- empty parameter list\n"
        "-- Circle\n"
        "-- Circle, Distance\n"
        "-- Center, Normal, Radius\n"
        "-- Point1, Point2, Point3");
    return -1;
#endif
}
#if 0
Py::Float Circle2dPy::getRadius(void) const
{
    Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(getGeomCirclePtr()->handle());
    return Py::Float(circle->Radius()); 
}

void  Circle2dPy::setRadius(Py::Float arg)
{
    Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(getGeomCirclePtr()->handle());
    circle->SetRadius((double)arg);
}

Py::Object Circle2dPy::getCenter(void) const
{
    Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(getGeomCirclePtr()->handle());
    gp_Pnt loc = circle->Location();
    return Py::Vector(Base::Vector3d(loc.X(), loc.Y(), loc.Z()));
}

void  Circle2dPy::setCenter(Py::Object arg)
{
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        Base::Vector3d loc = static_cast<Base::VectorPy*>(p)->value();
        getGeomCirclePtr()->setCenter(loc);
    }
    else if (PyObject_TypeCheck(p, &PyTuple_Type)) {
        Base::Vector3d loc = Base::getVectorFromTuple<double>(p);
        getGeomCirclePtr()->setCenter(loc);
    } else {
        std::string error = std::string("type must be 'Vector', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

Py::Object Circle2dPy::getAxis(void) const
{
    Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(getGeomCirclePtr()->handle());
    gp_Ax1 axis = circle->Axis();
    gp_Dir dir = axis.Direction();
    return Py::Vector(Base::Vector3d(dir.X(), dir.Y(), dir.Z()));
}

void  Circle2dPy::setAxis(Py::Object arg)
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

    Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(getGeomCirclePtr()->handle());
    try {
        gp_Ax1 axis;
        axis.SetLocation(circle->Location());
        axis.SetDirection(gp_Dir(val.x, val.y, val.z));
        circle->SetAxis(axis);
    }
    catch (Standard_Failure) {
        throw Py::Exception("cannot set axis");
    }
}

Py::Object Circle2dPy::getXAxis(void) const
{
    Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(getGeomCirclePtr()->handle());
    gp_Ax1 axis = circle->XAxis();
    gp_Dir dir = axis.Direction();
    return Py::Vector(Base::Vector3d(dir.X(), dir.Y(), dir.Z()));
}

void  Circle2dPy::setXAxis(Py::Object arg)
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

    Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(getGeomCirclePtr()->handle());
    try {
        gp_Ax2 pos;
        pos = circle->Position();
        pos.SetXDirection(gp_Dir(val.x, val.y, val.z));
        circle->SetPosition(pos);
    }
    catch (Standard_Failure) {
        throw Py::Exception("cannot set X axis");
    }
}

Py::Object Circle2dPy::getYAxis(void) const
{
    Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(getGeomCirclePtr()->handle());
    gp_Ax1 axis = circle->YAxis();
    gp_Dir dir = axis.Direction();
    return Py::Vector(Base::Vector3d(dir.X(), dir.Y(), dir.Z()));
}

void  Circle2dPy::setYAxis(Py::Object arg)
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

    Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(getGeomCirclePtr()->handle());
    try {
        gp_Ax2 pos;
        pos = circle->Position();
        pos.SetYDirection(gp_Dir(val.x, val.y, val.z));
        circle->SetPosition(pos);
    }
    catch (Standard_Failure) {
        throw Py::Exception("cannot set Y axis");
    }
}
#endif
PyObject *Circle2dPy::getCustomAttributes(const char* ) const
{
    return 0;
}

int Circle2dPy::setCustomAttributes(const char* , PyObject *)
{
    return 0; 
}
