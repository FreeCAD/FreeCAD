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
# include <gp_Hypr.hxx>
# include <Geom_Hyperbola.hxx>
# include <GC_MakeHyperbola.hxx>
#endif

#include <Base/GeometryPyCXX.h>
#include <Base/VectorPy.h>

#include <Mod/Part/App/OCCError.h>
#include <Mod/Part/App/Geometry2d.h>
#include <Mod/Part/App/Geom2d/Hyperbola2dPy.h>
#include <Mod/Part/App/Geom2d/Hyperbola2dPy.cpp>

using namespace Part;

extern const char* gce_ErrorStatusText(gce_ErrorType et);

// returns a string which represents the object e.g. when printed in python
std::string Hyperbola2dPy::representation(void) const
{
    return "<Hyperbola2d object>";
}

PyObject *Hyperbola2dPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of Hyperbola2dPy and the Twin object
    return new Hyperbola2dPy(new Geom2dHyperbola);
}

// constructor method
int Hyperbola2dPy::PyInit(PyObject* args, PyObject* kwds)
{
    return 0;
#if 0
    char* keywords_n[] = {NULL};
    if (PyArg_ParseTupleAndKeywords(args, kwds, "", keywords_n)) {
        Handle_Geom_Hyperbola hyperbola = Handle_Geom_Hyperbola::DownCast(getGeomHyperbolaPtr()->handle());
        hyperbola->SetMajorRadius(2.0);
        hyperbola->SetMinorRadius(1.0);
        return 0;
    }

    char* keywords_e[] = {"Hyperbola",NULL};
    PyErr_Clear();
    PyObject *pHypr;
    if (PyArg_ParseTupleAndKeywords(args, kwds, "O!",keywords_e, &(Hyperbola2dPy::Type), &pHypr)) {
        Hyperbola2dPy* pHyperbola = static_cast<Hyperbola2dPy*>(pHypr);
        Handle_Geom_Hyperbola Hypr1 = Handle_Geom_Hyperbola::DownCast
            (pHyperbola->getGeomHyperbolaPtr()->handle());
        Handle_Geom_Hyperbola Hypr2 = Handle_Geom_Hyperbola::DownCast
            (this->getGeomHyperbolaPtr()->handle());
        Hypr2->SetHypr(Hypr1->Hypr());
        return 0;
    }

    char* keywords_ssc[] = {"S1","S2","Center",NULL};
    PyErr_Clear();
    PyObject *pV1, *pV2, *pV3;
    if (PyArg_ParseTupleAndKeywords(args, kwds, "O!O!O!", keywords_ssc,
                                         &(Base::VectorPy::Type), &pV1,
                                         &(Base::VectorPy::Type), &pV2,
                                         &(Base::VectorPy::Type), &pV3)) {
        Base::Vector3d v1 = static_cast<Base::VectorPy*>(pV1)->value();
        Base::Vector3d v2 = static_cast<Base::VectorPy*>(pV2)->value();
        Base::Vector3d v3 = static_cast<Base::VectorPy*>(pV3)->value();
        GC_MakeHyperbola me(gp_Pnt(v1.x,v1.y,v1.z),
                          gp_Pnt(v2.x,v2.y,v2.z),
                          gp_Pnt(v3.x,v3.y,v3.z));
        if (!me.IsDone()) {
            PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(me.Status()));
            return -1;
        }

        Handle_Geom_Hyperbola hyperbola = Handle_Geom_Hyperbola::DownCast(getGeomHyperbolaPtr()->handle());
        hyperbola->SetHypr(me.Value()->Hypr());
        return 0;
    }

    char* keywords_cmm[] = {"Center","MajorRadius","MinorRadius",NULL};
    PyErr_Clear();
    PyObject *pV;
    double major, minor;
    if (PyArg_ParseTupleAndKeywords(args, kwds, "O!dd", keywords_cmm,
                                        &(Base::VectorPy::Type), &pV,
                                        &major, &minor)) {
        Base::Vector3d c = static_cast<Base::VectorPy*>(pV)->value();
        GC_MakeHyperbola me(gp_Ax2(gp_Pnt(c.x,c.y,c.z), gp_Dir(0.0,0.0,1.0)),
                          major, minor);
        if (!me.IsDone()) {
            PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(me.Status()));
            return -1;
        }

        Handle_Geom_Hyperbola hyperbola = Handle_Geom_Hyperbola::DownCast(getGeomHyperbolaPtr()->handle());
        hyperbola->SetHypr(me.Value()->Hypr());
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "Hyperbola constructor accepts:\n"
        "-- empty parameter list\n"
        "-- Hyperbola\n"
        "-- Point, double, double\n"
        "-- Point, Point, Point");
    return -1;
#endif
}
#if 0
Py::Float Hyperbola2dPy::getMajorRadius(void) const
{
    Handle_Geom_Hyperbola hyperbola = Handle_Geom_Hyperbola::DownCast(getGeomHyperbolaPtr()->handle());
    return Py::Float(hyperbola->MajorRadius()); 
}

void Hyperbola2dPy::setMajorRadius(Py::Float arg)
{
    Handle_Geom_Hyperbola hyperbola = Handle_Geom_Hyperbola::DownCast(getGeomHyperbolaPtr()->handle());
    hyperbola->SetMajorRadius((double)arg);
}

Py::Float Hyperbola2dPy::getMinorRadius(void) const
{
    Handle_Geom_Hyperbola hyperbola = Handle_Geom_Hyperbola::DownCast(getGeomHyperbolaPtr()->handle());
    return Py::Float(hyperbola->MinorRadius()); 
}

void Hyperbola2dPy::setMinorRadius(Py::Float arg)
{
    Handle_Geom_Hyperbola hyperbola = Handle_Geom_Hyperbola::DownCast(getGeomHyperbolaPtr()->handle());
    hyperbola->SetMinorRadius((double)arg);
}

Py::Float Hyperbola2dPy::getAngleXU(void) const
{
    Handle_Geom_Hyperbola hyperbola = Handle_Geom_Hyperbola::DownCast(getGeomHyperbolaPtr()->handle());
    
    gp_Pnt center = hyperbola->Axis().Location();
    gp_Dir normal = hyperbola->Axis().Direction();
    gp_Dir xdir = hyperbola->XAxis().Direction();
        
    gp_Ax2 xdirref(center, normal); // this is a reference system, might be CCW or CW depending on the creation method
    
    return Py::Float(-xdir.AngleWithRef(xdirref.XDirection(),normal));

}

void Hyperbola2dPy::setAngleXU(Py::Float arg)
{
    Handle_Geom_Hyperbola hyperbola = Handle_Geom_Hyperbola::DownCast(getGeomHyperbolaPtr()->handle());


    gp_Pnt center = hyperbola->Axis().Location();
    gp_Dir normal = hyperbola->Axis().Direction();
    
    gp_Ax1 normaxis(center, normal);
    
    gp_Ax2 xdirref(center, normal);
    
    xdirref.Rotate(normaxis,arg);
    
    hyperbola->SetPosition(xdirref);

}

Py::Float Hyperbola2dPy::getEccentricity(void) const
{
    Handle_Geom_Hyperbola hyperbola = Handle_Geom_Hyperbola::DownCast(getGeomHyperbolaPtr()->handle());
    return Py::Float(hyperbola->Eccentricity()); 
}

Py::Float Hyperbola2dPy::getFocal(void) const
{
    Handle_Geom_Hyperbola hyperbola = Handle_Geom_Hyperbola::DownCast(getGeomHyperbolaPtr()->handle());
    return Py::Float(hyperbola->Focal()); 
}

Py::Object Hyperbola2dPy::getFocus1(void) const
{
    Handle_Geom_Hyperbola hyperbola = Handle_Geom_Hyperbola::DownCast(getGeomHyperbolaPtr()->handle());
    gp_Pnt loc = hyperbola->Focus1();
    return Py::Vector(Base::Vector3d(loc.X(), loc.Y(), loc.Z()));
}

Py::Object Hyperbola2dPy::getFocus2(void) const
{
    Handle_Geom_Hyperbola hyperbola = Handle_Geom_Hyperbola::DownCast(getGeomHyperbolaPtr()->handle());
    gp_Pnt loc = hyperbola->Focus2();
    return Py::Vector(Base::Vector3d(loc.X(), loc.Y(), loc.Z()));
}

Py::Object Hyperbola2dPy::getCenter(void) const
{
    Handle_Geom_Hyperbola hyperbola = Handle_Geom_Hyperbola::DownCast(getGeomHyperbolaPtr()->handle());
    gp_Pnt loc = hyperbola->Location();
    return Py::Vector(Base::Vector3d(loc.X(), loc.Y(), loc.Z()));
}

void Hyperbola2dPy::setCenter(Py::Object arg)
{
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        Base::Vector3d loc = static_cast<Base::VectorPy*>(p)->value();
        Handle_Geom_Hyperbola hyperbola = Handle_Geom_Hyperbola::DownCast(getGeomHyperbolaPtr()->handle());
        hyperbola->SetLocation(gp_Pnt(loc.x, loc.y, loc.z));
    }
    else if (PyTuple_Check(p)) {
        Py::Tuple tuple(arg);
        gp_Pnt loc;
        loc.SetX((double)Py::Float(tuple.getItem(0)));
        loc.SetY((double)Py::Float(tuple.getItem(1)));
        loc.SetZ((double)Py::Float(tuple.getItem(2)));
        Handle_Geom_Hyperbola hyperbola = Handle_Geom_Hyperbola::DownCast(getGeomHyperbolaPtr()->handle());
        hyperbola->SetLocation(loc);
    }
    else {
        std::string error = std::string("type must be 'Vector', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

Py::Object Hyperbola2dPy::getAxis(void) const
{
    Handle_Geom_Hyperbola hyperbola = Handle_Geom_Hyperbola::DownCast(getGeomHyperbolaPtr()->handle());
    gp_Ax1 axis = hyperbola->Axis();
    gp_Dir dir = axis.Direction();
    return Py::Vector(Base::Vector3d(dir.X(), dir.Y(), dir.Z()));
}

void Hyperbola2dPy::setAxis(Py::Object arg)
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

    Handle_Geom_Hyperbola hyperbola = Handle_Geom_Hyperbola::DownCast(getGeomHyperbolaPtr()->handle());
    try {
        gp_Ax1 axis;
        axis.SetLocation(hyperbola->Location());
        axis.SetDirection(gp_Dir(val.x, val.y, val.z));
        hyperbola->SetAxis(axis);
    }
    catch (Standard_Failure) {
        throw Py::Exception("cannot set axis");
    }
}
#endif
PyObject *Hyperbola2dPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int Hyperbola2dPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
