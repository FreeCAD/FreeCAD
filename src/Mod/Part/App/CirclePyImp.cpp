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
# include <GC_MakeCircle.hxx>
# include <Geom_Circle.hxx>
# include <gp_Circ.hxx>
#endif

#include <Base/PyWrapParseTupleAndKeywords.h>
#include <Base/VectorPy.h>

#include "CirclePy.h"
#include "CirclePy.cpp"
#include "OCCError.h"


using namespace Part;

extern const char* gce_ErrorStatusText(gce_ErrorType et);

// returns a string which represents the object e.g. when printed in python
std::string CirclePy::representation() const
{
    Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast(getGeomCirclePtr()->handle());
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
}

PyObject *CirclePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of CirclePy and the Twin object
    Handle(Geom_Circle) circle = new Geom_Circle(gp_Circ());
    return new CirclePy(new GeomCircle(circle));
}

// constructor method
int CirclePy::PyInit(PyObject* args, PyObject* kwds)
{
    // circle and distance for offset
    PyObject *pCirc;
    double dist;
    static const std::array<const char *, 3> keywords_cd {"Circle", "Distance", nullptr};
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!d", keywords_cd, &(CirclePy::Type), &pCirc, &dist)) {
        CirclePy* pcCircle = static_cast<CirclePy*>(pCirc);
        Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast
            (pcCircle->getGeomCirclePtr()->handle());
        GC_MakeCircle mc(circle->Circ(), dist);
        if (!mc.IsDone()) {
            PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(mc.Status()));
            return -1;
        }

        Handle(Geom_Circle) circ = Handle(Geom_Circle)::DownCast(getGeomCirclePtr()->handle());
        circ->SetCirc(mc.Value()->Circ());
        return 0;
    }

    // center, normal and radius
    PyObject *pV1, *pV2, *pV3;
    static const std::array<const char *, 4> keywords_cnr {"Center", "Normal", "Radius", nullptr};
    PyErr_Clear();
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!O!d", keywords_cnr,
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

        Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast(getGeomCirclePtr()->handle());
        circle->SetCirc(mc.Value()->Circ());
        return 0;
    }

    static const std::array<const char *, 2> keywords_c {"Circle", nullptr};
    PyErr_Clear();
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!", keywords_c, &(CirclePy::Type), &pCirc)) {
        CirclePy* pcCircle = static_cast<CirclePy*>(pCirc);
        Handle(Geom_Circle) circ1 = Handle(Geom_Circle)::DownCast
            (pcCircle->getGeomCirclePtr()->handle());
        Handle(Geom_Circle) circ2 = Handle(Geom_Circle)::DownCast
            (this->getGeomCirclePtr()->handle());
        circ2->SetCirc(circ1->Circ());
        return 0;
    }

    static const std::array<const char *, 4> keywords_ppp {"Point1", "Point2", "Point3", nullptr};
    PyErr_Clear();
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!O!O!", keywords_ppp,
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

        Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast(getGeomCirclePtr()->handle());
        circle->SetCirc(mc.Value()->Circ());
        return 0;
    }

    // default circle
    static const std::array<const char *, 1> keywords_n {nullptr};
    PyErr_Clear();
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "", keywords_n)) {
        Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast(getGeomCirclePtr()->handle());
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
}

Py::Float CirclePy::getRadius() const
{
    Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast(getGeomCirclePtr()->handle());
    return Py::Float(circle->Radius());
}

void  CirclePy::setRadius(Py::Float arg)
{
    Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast(getGeomCirclePtr()->handle());
    circle->SetRadius((double)arg);
}

PyObject *CirclePy::getCustomAttributes(const char* ) const
{
    return nullptr;
}

int CirclePy::setCustomAttributes(const char* , PyObject *)
{
    return 0;
}
