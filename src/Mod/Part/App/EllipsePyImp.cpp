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

#include "OCCError.h"
#include "Geometry.h"
#include <Mod/Part/App/EllipsePy.h>
#include <Mod/Part/App/EllipsePy.cpp>

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
        Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(getGeomEllipsePtr()->handle());
        ellipse->SetMajorRadius(2.0);
        ellipse->SetMinorRadius(1.0);
        return 0;
    }

    char* keywords_e[] = {"Ellipse",NULL};
    PyErr_Clear();
    PyObject *pElips;
    if (PyArg_ParseTupleAndKeywords(args, kwds, "O!",keywords_e, &(EllipsePy::Type), &pElips)) {
        EllipsePy* pEllipse = static_cast<EllipsePy*>(pElips);
        Handle(Geom_Ellipse) Elips1 = Handle(Geom_Ellipse)::DownCast
            (pEllipse->getGeomEllipsePtr()->handle());
        Handle(Geom_Ellipse) Elips2 = Handle(Geom_Ellipse)::DownCast
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

        Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(getGeomEllipsePtr()->handle());
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

        Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(getGeomEllipsePtr()->handle());
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
    Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(getGeomEllipsePtr()->handle());
    return Py::Float(ellipse->MajorRadius()); 
}

void EllipsePy::setMajorRadius(Py::Float arg)
{
    Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(getGeomEllipsePtr()->handle());
    ellipse->SetMajorRadius((double)arg);
}

Py::Float EllipsePy::getMinorRadius(void) const
{
    Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(getGeomEllipsePtr()->handle());
    return Py::Float(ellipse->MinorRadius()); 
}

void EllipsePy::setMinorRadius(Py::Float arg)
{
    Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(getGeomEllipsePtr()->handle());
    ellipse->SetMinorRadius((double)arg);
}

Py::Float EllipsePy::getFocal(void) const
{
    Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(getGeomEllipsePtr()->handle());
    return Py::Float(ellipse->Focal()); 
}

Py::Object EllipsePy::getFocus1(void) const
{
    Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(getGeomEllipsePtr()->handle());
    gp_Pnt loc = ellipse->Focus1();
    return Py::Vector(Base::Vector3d(loc.X(), loc.Y(), loc.Z()));
}

Py::Object EllipsePy::getFocus2(void) const
{
    Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(getGeomEllipsePtr()->handle());
    gp_Pnt loc = ellipse->Focus2();
    return Py::Vector(Base::Vector3d(loc.X(), loc.Y(), loc.Z()));
}

PyObject *EllipsePy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int EllipsePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
