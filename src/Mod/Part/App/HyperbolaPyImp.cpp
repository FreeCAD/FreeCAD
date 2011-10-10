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
# include <Geom_Hyperbola.hxx>
# include <GC_MakeHyperbola.hxx>
# include <gp_Hypr.hxx>
#endif

#include <Base/VectorPy.h>
#include <Base/GeometryPyCXX.h>

#include "Geometry.h"
#include "HyperbolaPy.h"
#include "HyperbolaPy.cpp"

using namespace Part;

extern const char* gce_ErrorStatusText(gce_ErrorType et);

// returns a string which represents the object e.g. when printed in python
std::string HyperbolaPy::representation(void) const
{
    return "<Hyperbola object>";
}

PyObject *HyperbolaPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of HyperbolaPy and the Twin object 
    return new HyperbolaPy(new GeomHyperbola);
}

// constructor method
int HyperbolaPy::PyInit(PyObject* args, PyObject* kwds)
{
    char* keywords_n[] = {NULL};
    if (PyArg_ParseTupleAndKeywords(args, kwds, "", keywords_n)) {
        Handle_Geom_Hyperbola hypr = Handle_Geom_Hyperbola::DownCast(getGeometryPtr()->handle());
        hypr->SetMajorRadius(2.0);
        hypr->SetMinorRadius(1.0);
        return 0;
    }

    char* keywords_e[] = {"Hyperbola",NULL};
    PyErr_Clear();
    PyObject *pHypr;
    if (PyArg_ParseTupleAndKeywords(args, kwds, "O!",keywords_e, &(HyperbolaPy::Type), &pHypr)) {
        HyperbolaPy* pH = static_cast<HyperbolaPy*>(pHypr);
        Handle_Geom_Hyperbola Hypr1 = Handle_Geom_Hyperbola::DownCast
            (pH->getGeometryPtr()->handle());
        Handle_Geom_Hyperbola Hypr2 = Handle_Geom_Hyperbola::DownCast
            (this->getGeometryPtr()->handle());
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
        GC_MakeHyperbola mh(gp_Pnt(v1.x,v1.y,v1.z),
                            gp_Pnt(v2.x,v2.y,v2.z),
                            gp_Pnt(v3.x,v3.y,v3.z));
        if (!mh.IsDone()) {
            PyErr_SetString(PyExc_Exception, gce_ErrorStatusText(mh.Status()));
            return -1;
        }

        Handle_Geom_Hyperbola hyperb = Handle_Geom_Hyperbola::DownCast(getGeometryPtr()->handle());
        hyperb->SetHypr(mh.Value()->Hypr());
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
        GC_MakeHyperbola mh(gp_Ax2(gp_Pnt(c.x,c.y,c.z), gp_Dir(0.0,0.0,1.0)),
                          major, minor);
        if (!mh.IsDone()) {
            PyErr_SetString(PyExc_Exception, gce_ErrorStatusText(mh.Status()));
            return -1;
        }

        Handle_Geom_Hyperbola hyperb = Handle_Geom_Hyperbola::DownCast(getGeometryPtr()->handle());
        hyperb->SetHypr(mh.Value()->Hypr());
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "Hyperbola constructor accepts:\n"
        "-- empty parameter list\n"
        "-- Hyperbola\n"
        "-- Point, double, double\n"
        "-- Point, Point, Point");
    return -1;
}

Py::Float HyperbolaPy::getEccentricity(void) const
{
    Handle_Geom_Hyperbola curve = Handle_Geom_Hyperbola::DownCast(getGeometryPtr()->handle());
    return Py::Float(curve->Eccentricity()); 
}

Py::Float HyperbolaPy::getFocal(void) const
{
    Handle_Geom_Hyperbola curve = Handle_Geom_Hyperbola::DownCast(getGeometryPtr()->handle());
    return Py::Float(curve->Focal()); 
}

Py::Object HyperbolaPy::getFocus1(void) const
{
    Handle_Geom_Hyperbola c = Handle_Geom_Hyperbola::DownCast
        (getGeometryPtr()->handle());
    gp_Pnt loc = c->Focus1();
    return Py::Vector(Base::Vector3d(loc.X(), loc.Y(), loc.Z()));
}

Py::Object HyperbolaPy::getFocus2(void) const
{
    Handle_Geom_Hyperbola c = Handle_Geom_Hyperbola::DownCast
        (getGeometryPtr()->handle());
    gp_Pnt loc = c->Focus2();
    return Py::Vector(Base::Vector3d(loc.X(), loc.Y(), loc.Z()));
}

Py::Float HyperbolaPy::getParameter(void) const
{
    Handle_Geom_Hyperbola curve = Handle_Geom_Hyperbola::DownCast(getGeometryPtr()->handle());
    return Py::Float(curve->Parameter()); 
}

Py::Float HyperbolaPy::getMajorRadius(void) const
{
    Handle_Geom_Hyperbola curve = Handle_Geom_Hyperbola::DownCast(getGeometryPtr()->handle());
    return Py::Float(curve->MajorRadius()); 
}

void HyperbolaPy::setMajorRadius(Py::Float arg)
{
    Handle_Geom_Hyperbola curve = Handle_Geom_Hyperbola::DownCast(getGeometryPtr()->handle());
    curve->SetMajorRadius((double)arg); 
}

Py::Float HyperbolaPy::getMinorRadius(void) const
{
    Handle_Geom_Hyperbola curve = Handle_Geom_Hyperbola::DownCast(getGeometryPtr()->handle());
    return Py::Float(curve->MinorRadius()); 
}

void HyperbolaPy::setMinorRadius(Py::Float arg)
{
    Handle_Geom_Hyperbola curve = Handle_Geom_Hyperbola::DownCast(getGeometryPtr()->handle());
    curve->SetMinorRadius((double)arg); 
}

Py::Object HyperbolaPy::getLocation(void) const
{
    Handle_Geom_Hyperbola c = Handle_Geom_Hyperbola::DownCast
        (getGeometryPtr()->handle());
    gp_Pnt loc = c->Location();
    return Py::Vector(Base::Vector3d(loc.X(), loc.Y(), loc.Z()));
}

void HyperbolaPy::setLocation(Py::Object arg)
{
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        Base::Vector3d loc = static_cast<Base::VectorPy*>(p)->value();
        Handle_Geom_Hyperbola c = Handle_Geom_Hyperbola::DownCast
            (getGeometryPtr()->handle());
        c->SetLocation(gp_Pnt(loc.x, loc.y, loc.z));
    }
    else if (PyTuple_Check(p)) {
        gp_Pnt loc;
        Py::Tuple tuple(arg);
        loc.SetX((double)Py::Float(tuple.getItem(0)));
        loc.SetY((double)Py::Float(tuple.getItem(1)));
        loc.SetZ((double)Py::Float(tuple.getItem(2)));
        Handle_Geom_Hyperbola c = Handle_Geom_Hyperbola::DownCast
            (getGeometryPtr()->handle());
        c->SetLocation(loc);
    }
    else {
        std::string error = std::string("type must be 'Vector', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

Py::Object HyperbolaPy::getAxis(void) const
{
    Handle_Geom_Hyperbola c = Handle_Geom_Hyperbola::DownCast
        (getGeometryPtr()->handle());
    gp_Dir dir = c->Axis().Direction();
    return Py::Vector(Base::Vector3d(dir.X(), dir.Y(), dir.Z()));
}

void HyperbolaPy::setAxis(Py::Object arg)
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
        Handle_Geom_Hyperbola this_curv = Handle_Geom_Hyperbola::DownCast
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

PyObject *HyperbolaPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int HyperbolaPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


