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
# include <gp_Hypr2d.hxx>
# include <Geom2d_Hyperbola.hxx>
# include <GCE2d_MakeHyperbola.hxx>
#endif

#include <Base/GeometryPyCXX.h>

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
    char* keywords_n[] = {NULL};
    if (PyArg_ParseTupleAndKeywords(args, kwds, "", keywords_n)) {
        Handle(Geom2d_Hyperbola) hyperbola = Handle(Geom2d_Hyperbola)::DownCast(getGeom2dHyperbolaPtr()->handle());
        hyperbola->SetMajorRadius(2.0);
        hyperbola->SetMinorRadius(1.0);
        return 0;
    }

    char* keywords_e[] = {"Hyperbola",NULL};
    PyErr_Clear();
    PyObject *pHypr;
    if (PyArg_ParseTupleAndKeywords(args, kwds, "O!",keywords_e, &(Hyperbola2dPy::Type), &pHypr)) {
        Hyperbola2dPy* pHyperbola = static_cast<Hyperbola2dPy*>(pHypr);
        Handle(Geom2d_Hyperbola) Hypr1 = Handle(Geom2d_Hyperbola)::DownCast
            (pHyperbola->getGeom2dHyperbolaPtr()->handle());
        Handle(Geom2d_Hyperbola) Hypr2 = Handle(Geom2d_Hyperbola)::DownCast
            (this->getGeom2dHyperbolaPtr()->handle());
        Hypr2->SetHypr2d(Hypr1->Hypr2d());
        return 0;
    }

    char* keywords_ssc[] = {"S1","S2","Center",NULL};
    PyErr_Clear();
    PyObject *pV1, *pV2, *pV3;
    if (PyArg_ParseTupleAndKeywords(args, kwds, "O!O!O!", keywords_ssc,
                                         Base::Vector2dPy::type_object(), &pV1,
                                         Base::Vector2dPy::type_object(), &pV2,
                                         Base::Vector2dPy::type_object(), &pV3)) {
        Base::Vector2d v1 = Py::toVector2d(pV1);
        Base::Vector2d v2 = Py::toVector2d(pV2);
        Base::Vector2d v3 = Py::toVector2d(pV3);
        GCE2d_MakeHyperbola me(gp_Pnt2d(v1.x,v1.y),
                               gp_Pnt2d(v2.x,v2.y),
                               gp_Pnt2d(v3.x,v3.y));
        if (!me.IsDone()) {
            PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(me.Status()));
            return -1;
        }

        Handle(Geom2d_Hyperbola) hyperbola = Handle(Geom2d_Hyperbola)::DownCast(getGeom2dHyperbolaPtr()->handle());
        hyperbola->SetHypr2d(me.Value()->Hypr2d());
        return 0;
    }

    char* keywords_cmm[] = {"Center","MajorRadius","MinorRadius",NULL};
    PyErr_Clear();
    PyObject *pV;
    double major, minor;
    if (PyArg_ParseTupleAndKeywords(args, kwds, "O!dd", keywords_cmm,
                                        Base::Vector2dPy::type_object(), &pV,
                                        &major, &minor)) {
        Base::Vector2d c = Py::toVector2d(pV);
        GCE2d_MakeHyperbola me(gp_Ax2d(gp_Pnt2d(c.x,c.y), gp_Dir2d(0.0,1.0)),
                               major, minor);
        if (!me.IsDone()) {
            PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(me.Status()));
            return -1;
        }

        Handle(Geom2d_Hyperbola) hyperbola = Handle(Geom2d_Hyperbola)::DownCast(getGeom2dHyperbolaPtr()->handle());
        hyperbola->SetHypr2d(me.Value()->Hypr2d());
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "Hyperbola constructor accepts:\n"
        "-- empty parameter list\n"
        "-- Hyperbola\n"
        "-- Point, double, double\n"
        "-- Point, Point, Point");
    return -1;
}

Py::Float Hyperbola2dPy::getMajorRadius(void) const
{
    Handle(Geom2d_Hyperbola) hyperbola = Handle(Geom2d_Hyperbola)::DownCast(getGeom2dHyperbolaPtr()->handle());
    return Py::Float(hyperbola->MajorRadius()); 
}

void Hyperbola2dPy::setMajorRadius(Py::Float arg)
{
    Handle(Geom2d_Hyperbola) hyperbola = Handle(Geom2d_Hyperbola)::DownCast(getGeom2dHyperbolaPtr()->handle());
    hyperbola->SetMajorRadius((double)arg);
}

Py::Float Hyperbola2dPy::getMinorRadius(void) const
{
    Handle(Geom2d_Hyperbola) hyperbola = Handle(Geom2d_Hyperbola)::DownCast(getGeom2dHyperbolaPtr()->handle());
    return Py::Float(hyperbola->MinorRadius()); 
}

void Hyperbola2dPy::setMinorRadius(Py::Float arg)
{
    Handle(Geom2d_Hyperbola) hyperbola = Handle(Geom2d_Hyperbola)::DownCast(getGeom2dHyperbolaPtr()->handle());
    hyperbola->SetMinorRadius((double)arg);
}

Py::Float Hyperbola2dPy::getFocal(void) const
{
    Handle(Geom2d_Hyperbola) hyperbola = Handle(Geom2d_Hyperbola)::DownCast(getGeom2dHyperbolaPtr()->handle());
    return Py::Float(hyperbola->Focal()); 
}

Py::Object Hyperbola2dPy::getFocus1(void) const
{
    Handle(Geom2d_Hyperbola) hyperbola = Handle(Geom2d_Hyperbola)::DownCast(getGeom2dHyperbolaPtr()->handle());
    gp_Pnt2d loc = hyperbola->Focus1();

    Py::Module module("__FreeCADBase__");
    Py::Callable method(module.getAttr("Vector2d"));
    Py::Tuple arg(2);
    arg.setItem(0, Py::Float(loc.X()));
    arg.setItem(1, Py::Float(loc.Y()));
    return method.apply(arg);
}

Py::Object Hyperbola2dPy::getFocus2(void) const
{
    Handle(Geom2d_Hyperbola) hyperbola = Handle(Geom2d_Hyperbola)::DownCast(getGeom2dHyperbolaPtr()->handle());
    gp_Pnt2d loc = hyperbola->Focus2();

    Py::Module module("__FreeCADBase__");
    Py::Callable method(module.getAttr("Vector2d"));
    Py::Tuple arg(2);
    arg.setItem(0, Py::Float(loc.X()));
    arg.setItem(1, Py::Float(loc.Y()));
    return method.apply(arg);
}

PyObject *Hyperbola2dPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int Hyperbola2dPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
