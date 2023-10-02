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
# include <GC_MakeArcOfCircle.hxx>
# include <Geom_Circle.hxx>
# include <Geom_TrimmedCurve.hxx>
#endif

#include <Base/VectorPy.h>

#include "ArcOfCirclePy.h"
#include "ArcOfCirclePy.cpp"
#include "CirclePy.h"
#include "OCCError.h"


using namespace Part;

extern const char* gce_ErrorStatusText(gce_ErrorType et);

// returns a string which represents the object e.g. when printed in python
std::string ArcOfCirclePy::representation() const
{
    Handle(Geom_TrimmedCurve) trim = Handle(Geom_TrimmedCurve)::DownCast
        (getGeomArcOfCirclePtr()->handle());
    Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast(trim->BasisCurve());

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
int ArcOfCirclePy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
    PyObject* o;
    double u1, u2;
    PyObject *sense=Py_True;
    if (PyArg_ParseTuple(args, "O!dd|O!", &(Part::CirclePy::Type), &o, &u1, &u2, &PyBool_Type, &sense)) {
        try {
            Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast
                (static_cast<CirclePy*>(o)->getGeomCirclePtr()->handle());
            GC_MakeArcOfCircle arc(circle->Circ(), u1, u2, Base::asBoolean(sense));
            if (!arc.IsDone()) {
                PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(arc.Status()));
                return -1;
            }

            getGeomArcOfCirclePtr()->setHandle(arc.Value());
            return 0;
        }
        catch (Standard_Failure& e) {
            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return -1;
        }
        catch (...) {
            PyErr_SetString(PartExceptionOCCError, "creation of arc failed");
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
            PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(arc.Status()));
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

Py::Float ArcOfCirclePy::getRadius() const
{
    return Py::Float(getGeomArcOfCirclePtr()->getRadius());
}

void  ArcOfCirclePy::setRadius(Py::Float arg)
{
    getGeomArcOfCirclePtr()->setRadius((double)arg);
}

Py::Object ArcOfCirclePy::getCircle() const
{
    Handle(Geom_TrimmedCurve) trim = Handle(Geom_TrimmedCurve)::DownCast
        (getGeomArcOfCirclePtr()->handle());
    Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast(trim->BasisCurve());
    return Py::Object(new CirclePy(new GeomCircle(circle)), true);
}

PyObject *ArcOfCirclePy::getCustomAttributes(const char* ) const
{
    return nullptr;
}

int ArcOfCirclePy::setCustomAttributes(const char* , PyObject *)
{
    return 0;
}
