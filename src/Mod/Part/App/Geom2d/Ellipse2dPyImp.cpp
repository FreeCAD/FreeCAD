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

#include <Mod/Part/App/OCCError.h>
#include <Mod/Part/App/Geometry2d.h>
#include <Mod/Part/App/Geom2d/Ellipse2dPy.h>
#include <Mod/Part/App/Geom2d/Ellipse2dPy.cpp>

using namespace Part;

extern const char* gce_ErrorStatusText(gce_ErrorType et);

// returns a string which represents the object e.g. when printed in python
std::string Ellipse2dPy::representation(void) const
{
    return "<Ellipse2d object>";
}

PyObject *Ellipse2dPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of Ellipse2dPy and the Twin object
    return new Ellipse2dPy(new Geom2dEllipse);
}

// constructor method
int Ellipse2dPy::PyInit(PyObject* args, PyObject* kwds)
{
    return 0;
#if 0
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
    if (PyArg_ParseTupleAndKeywords(args, kwds, "O!",keywords_e, &(Ellipse2dPy::Type), &pElips)) {
        Ellipse2dPy* pEllipse = static_cast<Ellipse2dPy*>(pElips);
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
            PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(me.Status()));
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
            PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(me.Status()));
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
#endif
}
#if 0
Py::Float Ellipse2dPy::getMajorRadius(void) const
{
    Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(getGeomEllipsePtr()->handle());
    return Py::Float(ellipse->MajorRadius()); 
}

void Ellipse2dPy::setMajorRadius(Py::Float arg)
{
    Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(getGeomEllipsePtr()->handle());
    ellipse->SetMajorRadius((double)arg);
}

Py::Float Ellipse2dPy::getMinorRadius(void) const
{
    Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(getGeomEllipsePtr()->handle());
    return Py::Float(ellipse->MinorRadius()); 
}

void Ellipse2dPy::setMinorRadius(Py::Float arg)
{
    Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(getGeomEllipsePtr()->handle());
    ellipse->SetMinorRadius((double)arg);
}

Py::Float Ellipse2dPy::getFocal(void) const
{
    Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(getGeomEllipsePtr()->handle());
    return Py::Float(ellipse->Focal()); 
}

Py::Object Ellipse2dPy::getFocus1(void) const
{
    Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(getGeomEllipsePtr()->handle());
    gp_Pnt loc = ellipse->Focus1();
    return Py::Vector(Base::Vector3d(loc.X(), loc.Y(), loc.Z()));
}

Py::Object Ellipse2dPy::getFocus2(void) const
{
    Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(getGeomEllipsePtr()->handle());
    gp_Pnt loc = ellipse->Focus2();
    return Py::Vector(Base::Vector3d(loc.X(), loc.Y(), loc.Z()));
}
#endif
PyObject *Ellipse2dPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int Ellipse2dPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
