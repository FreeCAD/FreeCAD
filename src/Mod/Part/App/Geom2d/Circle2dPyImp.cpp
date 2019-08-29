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
# include <gp_Circ2d.hxx>
# include <Geom2d_Circle.hxx>
# include <GCE2d_MakeCircle.hxx>
#endif

#include <Mod/Part/App/OCCError.h>
#include <Mod/Part/App/Geom2d/Circle2dPy.h>
#include <Mod/Part/App/Geom2d/Circle2dPy.cpp>

#include <Base/GeometryPyCXX.h>

using namespace Part;

extern const char* gce_ErrorStatusText(gce_ErrorType et);

// returns a string which represents the object e.g. when printed in python
std::string Circle2dPy::representation(void) const
{
    return "<Circle2d object>";
}

PyObject *Circle2dPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of Circle2dPy and the Twin object
    return new Circle2dPy(new Geom2dCircle());
}

// constructor method
int Circle2dPy::PyInit(PyObject* args, PyObject* kwds)
{
    // circle and distance for offset
    PyObject *pCirc;
    double dist;
    static char* keywords_cd[] = {"Circle","Distance",NULL};
    if (PyArg_ParseTupleAndKeywords(args, kwds, "O!d", keywords_cd, &(Circle2dPy::Type), &pCirc, &dist)) {
        Circle2dPy* pcCircle = static_cast<Circle2dPy*>(pCirc);
        Handle(Geom2d_Circle) circle = Handle(Geom2d_Circle)::DownCast
            (pcCircle->getGeom2dCirclePtr()->handle());
        GCE2d_MakeCircle mc(circle->Circ2d(), dist);
        if (!mc.IsDone()) {
            PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(mc.Status()));
            return -1;
        }

        Handle(Geom2d_Circle) circ = Handle(Geom2d_Circle)::DownCast(getGeom2dCirclePtr()->handle());
        circ->SetCirc2d(mc.Value()->Circ2d());
        return 0;
    }

    // center and radius
    PyObject *pV1, *pV2, *pV3;
    static char* keywords_cnr[] = {"Center","Radius",NULL};
    PyErr_Clear();
    if (PyArg_ParseTupleAndKeywords(args, kwds, "O!d", keywords_cnr,
                                        Base::Vector2dPy::type_object(), &pV1,
                                        &dist)) {
        Base::Vector2d v1 = Py::toVector2d(pV1);
        GCE2d_MakeCircle mc(gp_Pnt2d(v1.x,v1.y), dist);
        if (!mc.IsDone()) {
            PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(mc.Status()));
            return -1;
        }

        Handle(Geom2d_Circle) circle = Handle(Geom2d_Circle)::DownCast(getGeom2dCirclePtr()->handle());
        circle->SetCirc2d(mc.Value()->Circ2d());
        return 0;
    }

    static char* keywords_c[] = {"Circle",NULL};
    PyErr_Clear();
    if (PyArg_ParseTupleAndKeywords(args, kwds, "O!", keywords_c, &(Circle2dPy::Type), &pCirc)) {
        Circle2dPy* pcCircle = static_cast<Circle2dPy*>(pCirc);
        Handle(Geom2d_Circle) circ1 = Handle(Geom2d_Circle)::DownCast
            (pcCircle->getGeom2dCirclePtr()->handle());
        Handle(Geom2d_Circle) circ2 = Handle(Geom2d_Circle)::DownCast
            (this->getGeom2dCirclePtr()->handle());
        circ2->SetCirc2d(circ1->Circ2d());
        return 0;
    }

    static char* keywords_ppp[] = {"Point1","Point2","Point3",NULL};
    PyErr_Clear();
    if (PyArg_ParseTupleAndKeywords(args, kwds, "O!O!O!", keywords_ppp,
                                         Base::Vector2dPy::type_object(), &pV1,
                                         Base::Vector2dPy::type_object(), &pV2,
                                         Base::Vector2dPy::type_object(), &pV3)) {
        Base::Vector2d v1 = Py::toVector2d(pV1);
        Base::Vector2d v2 = Py::toVector2d(pV2);
        Base::Vector2d v3 = Py::toVector2d(pV3);
        GCE2d_MakeCircle mc(gp_Pnt2d(v1.x,v1.y),
                            gp_Pnt2d(v2.x,v2.y),
                            gp_Pnt2d(v3.x,v3.y));
        if (!mc.IsDone()) {
            PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(mc.Status()));
            return -1;
        }

        Handle(Geom2d_Circle) circle = Handle(Geom2d_Circle)::DownCast(getGeom2dCirclePtr()->handle());
        circle->SetCirc2d(mc.Value()->Circ2d());
        return 0;
    }

    // default circle
    static char* keywords_n[] = {NULL};
    PyErr_Clear();
    if (PyArg_ParseTupleAndKeywords(args, kwds, "", keywords_n)) {
        Handle(Geom2d_Circle) circle = Handle(Geom2d_Circle)::DownCast(getGeom2dCirclePtr()->handle());
        circle->SetRadius(1.0);
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "Circle constructor accepts:\n"
        "-- empty parameter list\n"
        "-- Circle\n"
        "-- Circle, Distance\n"
        "-- Center, Radius\n"
        "-- Point1, Point2, Point3");
    return -1;
}

Py::Float Circle2dPy::getRadius(void) const
{
    Handle(Geom2d_Circle) circle = Handle(Geom2d_Circle)::DownCast(getGeom2dCirclePtr()->handle());
    return Py::Float(circle->Radius()); 
}

void  Circle2dPy::setRadius(Py::Float arg)
{
    Handle(Geom2d_Circle) circle = Handle(Geom2d_Circle)::DownCast(getGeom2dCirclePtr()->handle());
    circle->SetRadius((double)arg);
}

PyObject *Circle2dPy::getCustomAttributes(const char* ) const
{
    return 0;
}

int Circle2dPy::setCustomAttributes(const char* , PyObject *)
{
    return 0; 
}
