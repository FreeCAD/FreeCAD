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
# include <GCE2d_MakeArcOfCircle.hxx>
# include <Geom2d_Circle.hxx>
# include <Geom2d_TrimmedCurve.hxx>
# include <gp_Circ2d.hxx>
#endif

#include <Base/GeometryPyCXX.h>

#include "Geom2d/ArcOfCircle2dPy.h"
#include "Geom2d/ArcOfCircle2dPy.cpp"
#include "Geom2d/Circle2dPy.h"
#include "OCCError.h"


using namespace Part;

extern const char* gce_ErrorStatusText(gce_ErrorType et);

// returns a string which represents the object e.g. when printed in python
std::string ArcOfCircle2dPy::representation() const
{
    return "<Arc of circle2d object>";
}

PyObject *ArcOfCircle2dPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of ArcOfCirclePy and the Twin object
    return new ArcOfCircle2dPy(new Geom2dArcOfCircle);
}

// constructor method
int ArcOfCircle2dPy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
    PyObject* o;
    double u1, u2;
    PyObject *sense=Py_True;
    if (PyArg_ParseTuple(args, "O!dd|O!", &(Part::Circle2dPy::Type), &o, &u1, &u2, &PyBool_Type, &sense)) {
        try {
            Handle(Geom2d_Circle) circle = Handle(Geom2d_Circle)::DownCast
                (static_cast<Circle2dPy*>(o)->getGeom2dCirclePtr()->handle());
            GCE2d_MakeArcOfCircle arc(circle->Circ2d(), u1, u2, Base::asBoolean(sense));
            if (!arc.IsDone()) {
                PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(arc.Status()));
                return -1;
            }

            getGeom2dArcOfCirclePtr()->setHandle(arc.Value());
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
    if (PyArg_ParseTuple(args, "O!O!O!", Base::Vector2dPy::type_object(), &pV1,
                                         Base::Vector2dPy::type_object(), &pV2,
                                         Base::Vector2dPy::type_object(), &pV3)) {
        Base::Vector2d v1 = Py::toVector2d(pV1);
        Base::Vector2d v2 = Py::toVector2d(pV2);
        Base::Vector2d v3 = Py::toVector2d(pV3);

        GCE2d_MakeArcOfCircle arc(gp_Pnt2d(v1.x,v1.y),
                                  gp_Pnt2d(v2.x,v2.y),
                                  gp_Pnt2d(v3.x,v3.y));
        if (!arc.IsDone()) {
            PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(arc.Status()));
            return -1;
        }

        getGeom2dArcOfCirclePtr()->setHandle(arc.Value());
        return 0;
    }

    // All checks failed
    PyErr_SetString(PyExc_TypeError,
        "ArcOfCircle2d constructor expects a circle curve and a parameter range or three points");
    return -1;
}

Py::Float ArcOfCircle2dPy::getRadius() const
{
    return Py::Float(getGeom2dArcOfCirclePtr()->getRadius());
}

void  ArcOfCircle2dPy::setRadius(Py::Float arg)
{
    getGeom2dArcOfCirclePtr()->setRadius((double)arg);
}

Py::Object ArcOfCircle2dPy::getCircle() const
{
    Handle(Geom2d_TrimmedCurve) curve = Handle(Geom2d_TrimmedCurve)::DownCast(getGeom2dArcOfConicPtr()->handle());
    Handle(Geom2d_Circle) circle = Handle(Geom2d_Circle)::DownCast(curve->BasisCurve());
    return Py::asObject(new Circle2dPy(new Geom2dCircle(circle)));
}

PyObject *ArcOfCircle2dPy::getCustomAttributes(const char* ) const
{
    return nullptr;
}

int ArcOfCircle2dPy::setCustomAttributes(const char* , PyObject *)
{
    return 0;
}
