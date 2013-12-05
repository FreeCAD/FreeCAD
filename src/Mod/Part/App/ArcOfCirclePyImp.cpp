/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <GC_MakeArcOfCircle.hxx>
# include <GC_MakeCircle.hxx>
# include <Geom_TrimmedCurve.hxx>
#endif

#include "Mod/Part/App/Geometry.h"
#include "ArcOfCirclePy.h"
#include "ArcOfCirclePy.cpp"
#include "CirclePy.h"

#include <Base/GeometryPyCXX.h>
#include <Base/VectorPy.h>

using namespace Part;

extern const char* gce_ErrorStatusText(gce_ErrorType et);

// returns a string which represents the object e.g. when printed in python
std::string ArcOfCirclePy::representation(void) const
{
    Handle_Geom_TrimmedCurve trim = Handle_Geom_TrimmedCurve::DownCast
        (getGeomArcOfCirclePtr()->handle());
    Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(trim->BasisCurve());

    gp_Ax1 axis = circle->Axis();
    gp_Dir dir = axis.Direction();
    gp_Pnt loc = axis.Location();
    Standard_Real fRad = circle->Radius();
    Standard_Real u1 = trim->FirstParameter();
    Standard_Real u2 = trim->LastParameter();

    std::stringstream str;
    str << "ArcOfCircle (";
    str << "Radius : " << fRad << ", "; 
    str << "Position : (" << loc.X() << ", "<< loc.Y() << ", "<< loc.Z() << "), "; 
    str << "Direction : (" << dir.X() << ", "<< dir.Y() << ", "<< dir.Z() << "), "; 
    str << "Parameter : (" << u1 << ", " << u2 << ")"; 
    str << ")";

    return str.str();
}

PyObject *ArcOfCirclePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of ArcOfCirclePy and the Twin object 
    return new ArcOfCirclePy(new GeomArcOfCircle);
}

// constructor method
int ArcOfCirclePy::PyInit(PyObject* args, PyObject* kwds)
{
    PyObject* o;
    double u1, u2;
    PyObject *sense=Py_True;
    if (PyArg_ParseTuple(args, "O!dd|O!", &(Part::CirclePy::Type), &o, &u1, &u2, &PyBool_Type, &sense)) {
        try {
            Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast
                (static_cast<CirclePy*>(o)->getGeomCirclePtr()->handle());
            GC_MakeArcOfCircle arc(circle->Circ(), u1, u2, PyObject_IsTrue(sense) ? Standard_True : Standard_False);
            if (!arc.IsDone()) {
                PyErr_SetString(PyExc_Exception, gce_ErrorStatusText(arc.Status()));
                return -1;
            }

            getGeomArcOfCirclePtr()->setHandle(arc.Value());
            return 0;
        }
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            PyErr_SetString(PyExc_Exception, e->GetMessageString());
            return -1;
        }
        catch (...) {
            PyErr_SetString(PyExc_Exception, "creation of arc failed");
            return -1;
        }
    }

    PyErr_Clear();
    PyObject *pV1, *pV2, *pV3;
    if (PyArg_ParseTuple(args, "O!O!O!", &(Base::VectorPy::Type), &pV1,
                                         &(Base::VectorPy::Type), &pV2,
                                         &(Base::VectorPy::Type), &pV3)) {
        Base::Vector3d v1 = static_cast<Base::VectorPy*>(pV1)->value();
        Base::Vector3d v2 = static_cast<Base::VectorPy*>(pV2)->value();
        Base::Vector3d v3 = static_cast<Base::VectorPy*>(pV3)->value();

        GC_MakeArcOfCircle arc(gp_Pnt(v1.x,v1.y,v1.z),
                               gp_Pnt(v2.x,v2.y,v2.z),
                               gp_Pnt(v3.x,v3.y,v3.z));
        if (!arc.IsDone()) {
            PyErr_SetString(PyExc_Exception, gce_ErrorStatusText(arc.Status()));
            return -1;
        }

        getGeomArcOfCirclePtr()->setHandle(arc.Value());
        return 0;
    }

    // All checks failed
    PyErr_SetString(PyExc_TypeError,
        "ArcOfCircle constructor expects a circle curve and a parameter range or three points");
    return -1;
}

Py::Float ArcOfCirclePy::getRadius(void) const
{
    return Py::Float(getGeomArcOfCirclePtr()->getRadius()); 
}

void  ArcOfCirclePy::setRadius(Py::Float arg)
{
    getGeomArcOfCirclePtr()->setRadius((double)arg);
}

Py::Object ArcOfCirclePy::getCenter(void) const
{
    return Py::Vector(getGeomArcOfCirclePtr()->getCenter());
}

void  ArcOfCirclePy::setCenter(Py::Object arg)
{
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        Base::Vector3d loc = static_cast<Base::VectorPy*>(p)->value();
        getGeomArcOfCirclePtr()->setCenter(loc);
    }
    else if (PyObject_TypeCheck(p, &PyTuple_Type)) {
        Base::Vector3d loc = Base::getVectorFromTuple<double>(p);
        getGeomArcOfCirclePtr()->setCenter(loc);
    }
    else {
        std::string error = std::string("type must be 'Vector', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

Py::Object ArcOfCirclePy::getAxis(void) const
{
    Handle_Geom_TrimmedCurve trim = Handle_Geom_TrimmedCurve::DownCast
        (getGeomArcOfCirclePtr()->handle());
    Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(trim->BasisCurve());
    gp_Ax1 axis = circle->Axis();
    gp_Dir dir = axis.Direction();
    return Py::Vector(Base::Vector3d(dir.X(), dir.Y(), dir.Z()));
}

void  ArcOfCirclePy::setAxis(Py::Object arg)
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

    Handle_Geom_TrimmedCurve trim = Handle_Geom_TrimmedCurve::DownCast
        (getGeomArcOfCirclePtr()->handle());
    Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(trim->BasisCurve());
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

Py::Object ArcOfCirclePy::getCircle(void) const
{
    Handle_Geom_TrimmedCurve trim = Handle_Geom_TrimmedCurve::DownCast
        (getGeomArcOfCirclePtr()->handle());
    Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(trim->BasisCurve());
    return Py::Object(new CirclePy(new GeomCircle(circle)), true);
}

PyObject *ArcOfCirclePy::getCustomAttributes(const char* attr) const
{
    return 0;
}

int ArcOfCirclePy::setCustomAttributes(const char* attr, PyObject *obj)
{
    return 0; 
}
