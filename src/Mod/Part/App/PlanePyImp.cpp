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
# include <gp_Ax1.hxx>
# include <gp_Dir.hxx>
# include <gp_Pnt.hxx>
# include <gp_Lin.hxx>
# include <gp_Pln.hxx>
# include <Geom_Line.hxx>
# include <Geom_Plane.hxx>
# include <Geom_TrimmedCurve.hxx>
# include <GC_MakePlane.hxx>
# include <Standard_Failure.hxx>
#endif

#include <Base/VectorPy.h>
#include <Base/GeometryPyCXX.h>

#include "OCCError.h"
#include "Geometry.h"
#include <Mod/Part/App/LinePy.h>
#include <Mod/Part/App/PlanePy.h>
#include <Mod/Part/App/PlanePy.cpp>

using namespace Part;

extern const char* gce_ErrorStatusText(gce_ErrorType et);

// returns a string which represents the object e.g. when printed in python
std::string PlanePy::representation(void) const
{
    return "<Plane object>";
}

PyObject *PlanePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of PlanePy and the Twin object 
    return new PlanePy(new GeomPlane);
}

// constructor method
int PlanePy::PyInit(PyObject* args, PyObject* kwds)
{
    // plane and distance for offset
    PyObject *pPlane;
    double dist;
    static char* keywords_pd[] = {"Plane","Distance",NULL};
    if (PyArg_ParseTupleAndKeywords(args, kwds, "O!d", keywords_pd, &(PlanePy::Type), &pPlane, &dist)) {
        PlanePy* pcPlane = static_cast<PlanePy*>(pPlane);
        Handle(Geom_Plane) plane = Handle(Geom_Plane)::DownCast
            (pcPlane->getGeometryPtr()->handle());
        GC_MakePlane mc(plane->Pln(), dist);
        if (!mc.IsDone()) {
            PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(mc.Status()));
            return -1;
        }

        Handle(Geom_Plane) plan = Handle(Geom_Plane)::DownCast(getGeometryPtr()->handle());
        plan->SetPln(mc.Value()->Pln());
        return 0;
    }

    // plane from equation
    double a,b,c,d;
    static char* keywords_abcd[] = {"A","B","C","D",NULL};
    PyErr_Clear();
    if (PyArg_ParseTupleAndKeywords(args, kwds, "dddd", keywords_abcd,
                                        &a,&b,&c,&d)) {
        GC_MakePlane mc(a,b,c,d);
        if (!mc.IsDone()) {
            PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(mc.Status()));
            return -1;
        }

        Handle(Geom_Plane) plane = Handle(Geom_Plane)::DownCast(getGeometryPtr()->handle());
        plane->SetPln(mc.Value()->Pln());
        return 0;
    }

    PyObject *pV1, *pV2, *pV3;
    static char* keywords_ppp[] = {"Point1","Point2","Point3",NULL};
    PyErr_Clear();
    if (PyArg_ParseTupleAndKeywords(args, kwds, "O!O!O!", keywords_ppp,
                                         &(Base::VectorPy::Type), &pV1,
                                         &(Base::VectorPy::Type), &pV2,
                                         &(Base::VectorPy::Type), &pV3)) {
        Base::Vector3d v1 = static_cast<Base::VectorPy*>(pV1)->value();
        Base::Vector3d v2 = static_cast<Base::VectorPy*>(pV2)->value();
        Base::Vector3d v3 = static_cast<Base::VectorPy*>(pV3)->value();
        GC_MakePlane mc(gp_Pnt(v1.x,v1.y,v1.z),
                        gp_Pnt(v2.x,v2.y,v2.z),
                        gp_Pnt(v3.x,v3.y,v3.z));
        if (!mc.IsDone()) {
            PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(mc.Status()));
            return -1;
        }

        Handle(Geom_Plane) plane = Handle(Geom_Plane)::DownCast(getGeometryPtr()->handle());
        plane->SetPln(mc.Value()->Pln());
        return 0;
    }

    // location and normal
    static char* keywords_cnr[] = {"Location","Normal",NULL};
    PyErr_Clear();
    if (PyArg_ParseTupleAndKeywords(args, kwds, "O!O!", keywords_cnr,
                                        &(Base::VectorPy::Type), &pV1,
                                        &(Base::VectorPy::Type), &pV2)) {
        Base::Vector3d v1 = static_cast<Base::VectorPy*>(pV1)->value();
        Base::Vector3d v2 = static_cast<Base::VectorPy*>(pV2)->value();
        GC_MakePlane mc(gp_Pnt(v1.x,v1.y,v1.z),
                        gp_Dir(v2.x,v2.y,v2.z));
        if (!mc.IsDone()) {
            PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(mc.Status()));
            return -1;
        }

        Handle(Geom_Plane) plane = Handle(Geom_Plane)::DownCast(getGeometryPtr()->handle());
        plane->SetPln(mc.Value()->Pln());
        return 0;
    }

    static char* keywords_p[] = {"Plane",NULL};
    PyErr_Clear();
    if (PyArg_ParseTupleAndKeywords(args, kwds, "O!", keywords_p, &(PlanePy::Type), &pPlane)) {
        PlanePy* pcPlane = static_cast<PlanePy*>(pPlane);
        Handle(Geom_Plane) plane1 = Handle(Geom_Plane)::DownCast
            (pcPlane->getGeometryPtr()->handle());
        Handle(Geom_Plane) plane2 = Handle(Geom_Plane)::DownCast
            (this->getGeometryPtr()->handle());
        plane2->SetPln(plane1->Pln());
        return 0;
    }

    static char* keywords_n[] = {NULL};
    PyErr_Clear();
    if (PyArg_ParseTupleAndKeywords(args, kwds, "", keywords_n)) {
        // do nothing
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "Plane constructor accepts:\n"
        "-- empty parameter list\n"
        "-- Plane\n"
        "-- Plane, Distance\n"
        "-- Location, Normal\n"
        "-- Point1, Point2, Point3\n"
        "-- A, B, C, D\n"
        "   (as equation: Ax + By + Cz + D = 0.0)");
    return -1;
}

Py::Object PlanePy::getPosition(void) const
{
    Handle(Geom_Plane) this_surf = Handle(Geom_Plane)::DownCast
        (this->getGeomPlanePtr()->handle());
    gp_Pnt pnt = this_surf->Location();
    return Py::Vector(Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z()));
}

void PlanePy::setPosition(Py::Object arg)
{
    gp_Pnt loc;
    PyObject *p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        Base::Vector3d v = static_cast<Base::VectorPy*>(p)->value();
        loc.SetX(v.x);
        loc.SetY(v.y);
        loc.SetZ(v.z);
    }
    else if (PyTuple_Check(p)) {
        Py::Tuple tuple(arg);
        loc.SetX((double)Py::Float(tuple.getItem(0)));
        loc.SetY((double)Py::Float(tuple.getItem(1)));
        loc.SetZ((double)Py::Float(tuple.getItem(2)));
    }
    else {
        std::string error = std::string("type must be 'Vector' or tuple, not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }

    try {
        Handle(Geom_Plane) this_surf = Handle(Geom_Plane)::DownCast
            (this->getGeomPlanePtr()->handle());
        this_surf->SetLocation(loc);
    }
    catch (Standard_Failure& e) {
        throw Py::RuntimeError(e.GetMessageString());
    }
}

Py::Object PlanePy::getAxis(void) const
{
    Handle(Geom_ElementarySurface) s = Handle(Geom_ElementarySurface)::DownCast
        (getGeometryPtr()->handle());
    gp_Dir dir = s->Axis().Direction();
    return Py::Vector(Base::Vector3d(dir.X(), dir.Y(), dir.Z()));
}

void PlanePy::setAxis(Py::Object arg)
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
        Handle(Geom_ElementarySurface) this_surf = Handle(Geom_ElementarySurface)::DownCast
            (this->getGeometryPtr()->handle());
        gp_Ax1 axis;
        axis.SetLocation(this_surf->Location());
        axis.SetDirection(gp_Dir(dir_x, dir_y, dir_z));
        this_surf->SetAxis(axis);
    }
    catch (Standard_Failure&) {
        throw Py::RuntimeError("cannot set axis");
    }
}

PyObject* PlanePy::uIso(PyObject * args)
{
    double u;
    if (!PyArg_ParseTuple(args, "d", &u))
        return 0;

    try {
        Handle(Geom_Plane) plane = Handle(Geom_Plane)::DownCast
            (getGeomPlanePtr()->handle());
        Handle(Geom_Line) c = Handle(Geom_Line)::DownCast(plane->UIso(u));
        GeomLine* line = new GeomLine();
        Handle(Geom_Line) this_curv = Handle(Geom_Line)::DownCast
            (line->handle());
        this_curv->SetLin(c->Lin());
        return new LinePy(line);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* PlanePy::vIso(PyObject * args)
{
    double v;
    if (!PyArg_ParseTuple(args, "d", &v))
        return 0;

    try {
        Handle(Geom_Plane) plane = Handle(Geom_Plane)::DownCast
            (getGeomPlanePtr()->handle());
        Handle(Geom_Line) c = Handle(Geom_Line)::DownCast(plane->VIso(v));
        GeomLine* line = new GeomLine();
        Handle(Geom_Line) this_curv = Handle(Geom_Line)::DownCast
            (line->handle());
        this_curv->SetLin(c->Lin());
        return new LinePy(line);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject *PlanePy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int PlanePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
