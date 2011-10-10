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
# include <gp_Elips.hxx>
# include <Geom_Ellipse.hxx>
# include <GC_MakeEllipse.hxx>
#endif

#include <Base/GeometryPyCXX.h>
#include <Base/VectorPy.h>

#include "Geometry.h"
#include "EllipsePy.h"
#include "EllipsePy.cpp"

using namespace Part;

extern const char* gce_ErrorStatusText(gce_ErrorType et);

// returns a string which represents the object e.g. when printed in python
std::string EllipsePy::representation(void) const
{
    return "<Ellipse object>";
}

PyObject *EllipsePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of EllipsePy and the Twin object 
    return new EllipsePy(new GeomEllipse);
}

// constructor method
int EllipsePy::PyInit(PyObject* args, PyObject* kwds)
{
    char* keywords_n[] = {NULL};
    if (PyArg_ParseTupleAndKeywords(args, kwds, "", keywords_n)) {
        Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(getGeomEllipsePtr()->handle());
        ellipse->SetMajorRadius(2.0);
        ellipse->SetMinorRadius(1.0);
        return 0;
    }

    char* keywords_e[] = {"Ellipse",NULL};
    PyErr_Clear();
    PyObject *pElips;
    if (PyArg_ParseTupleAndKeywords(args, kwds, "O!",keywords_e, &(EllipsePy::Type), &pElips)) {
        EllipsePy* pEllipse = static_cast<EllipsePy*>(pElips);
        Handle_Geom_Ellipse Elips1 = Handle_Geom_Ellipse::DownCast
            (pEllipse->getGeomEllipsePtr()->handle());
        Handle_Geom_Ellipse Elips2 = Handle_Geom_Ellipse::DownCast
            (this->getGeomEllipsePtr()->handle());
        Elips2->SetElips(Elips1->Elips());
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
        GC_MakeEllipse me(gp_Pnt(v1.x,v1.y,v1.z),
                          gp_Pnt(v2.x,v2.y,v2.z),
                          gp_Pnt(v3.x,v3.y,v3.z));
        if (!me.IsDone()) {
            PyErr_SetString(PyExc_Exception, gce_ErrorStatusText(me.Status()));
            return -1;
        }

        Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(getGeomEllipsePtr()->handle());
        ellipse->SetElips(me.Value()->Elips());
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
        GC_MakeEllipse me(gp_Ax2(gp_Pnt(c.x,c.y,c.z), gp_Dir(0.0,0.0,1.0)),
                          major, minor);
        if (!me.IsDone()) {
            PyErr_SetString(PyExc_Exception, gce_ErrorStatusText(me.Status()));
            return -1;
        }

        Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(getGeomEllipsePtr()->handle());
        ellipse->SetElips(me.Value()->Elips());
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "Ellipse constructor accepts:\n"
        "-- empty parameter list\n"
        "-- Ellipse\n"
        "-- Point, double, double\n"
        "-- Point, Point, Point");
    return -1;
}

Py::Float EllipsePy::getMajorRadius(void) const
{
    Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(getGeomEllipsePtr()->handle());
    return Py::Float(ellipse->MajorRadius()); 
}

void EllipsePy::setMajorRadius(Py::Float arg)
{
    Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(getGeomEllipsePtr()->handle());
    ellipse->SetMajorRadius((double)arg);
}

Py::Float EllipsePy::getMinorRadius(void) const
{
    Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(getGeomEllipsePtr()->handle());
    return Py::Float(ellipse->MinorRadius()); 
}

void EllipsePy::setMinorRadius(Py::Float arg)
{
    Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(getGeomEllipsePtr()->handle());
    ellipse->SetMinorRadius((double)arg);
}

Py::Float EllipsePy::getEccentricity(void) const
{
    Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(getGeomEllipsePtr()->handle());
    return Py::Float(ellipse->Eccentricity()); 
}

Py::Float EllipsePy::getFocal(void) const
{
    Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(getGeomEllipsePtr()->handle());
    return Py::Float(ellipse->Focal()); 
}

Py::Object EllipsePy::getFocus1(void) const
{
    Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(getGeomEllipsePtr()->handle());
    gp_Pnt loc = ellipse->Focus1();
    return Py::Vector(Base::Vector3d(loc.X(), loc.Y(), loc.Z()));
}

Py::Object EllipsePy::getFocus2(void) const
{
    Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(getGeomEllipsePtr()->handle());
    gp_Pnt loc = ellipse->Focus2();
    return Py::Vector(Base::Vector3d(loc.X(), loc.Y(), loc.Z()));
}

Py::Object EllipsePy::getCenter(void) const
{
    Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(getGeomEllipsePtr()->handle());
    gp_Pnt loc = ellipse->Location();
    return Py::Vector(Base::Vector3d(loc.X(), loc.Y(), loc.Z()));
}

void EllipsePy::setCenter(Py::Object arg)
{
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        Base::Vector3d loc = static_cast<Base::VectorPy*>(p)->value();
        Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(getGeomEllipsePtr()->handle());
        ellipse->SetLocation(gp_Pnt(loc.x, loc.y, loc.z));
    }
    else if (PyTuple_Check(p)) {
        Py::Tuple tuple(arg);
        gp_Pnt loc;
        loc.SetX((double)Py::Float(tuple.getItem(0)));
        loc.SetY((double)Py::Float(tuple.getItem(1)));
        loc.SetZ((double)Py::Float(tuple.getItem(2)));
        Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(getGeomEllipsePtr()->handle());
        ellipse->SetLocation(loc);
    }
    else {
        std::string error = std::string("type must be 'Vector', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

Py::Object EllipsePy::getAxis(void) const
{
    Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(getGeomEllipsePtr()->handle());
    gp_Ax1 axis = ellipse->Axis();
    gp_Dir dir = axis.Direction();
    return Py::Vector(Base::Vector3d(dir.X(), dir.Y(), dir.Z()));
}

void EllipsePy::setAxis(Py::Object arg)
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

    Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(getGeomEllipsePtr()->handle());
    try {
        gp_Ax1 axis;
        axis.SetLocation(ellipse->Location());
        axis.SetDirection(gp_Dir(val.x, val.y, val.z));
        ellipse->SetAxis(axis);
    }
    catch (Standard_Failure) {
        throw Py::Exception("cannot set axis");
    }
}

PyObject *EllipsePy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int EllipsePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
