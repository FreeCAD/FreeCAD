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
# include <gp_Elips2d.hxx>
# include <Geom2d_Ellipse.hxx>
# include <GCE2d_MakeEllipse.hxx>
#endif

#include <Base/GeometryPyCXX.h>

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
    char* keywords_n[] = {NULL};
    if (PyArg_ParseTupleAndKeywords(args, kwds, "", keywords_n)) {
        Handle(Geom2d_Ellipse) ellipse = Handle(Geom2d_Ellipse)::DownCast(getGeom2dEllipsePtr()->handle());
        ellipse->SetMajorRadius(2.0);
        ellipse->SetMinorRadius(1.0);
        return 0;
    }

    char* keywords_e[] = {"Ellipse",NULL};
    PyErr_Clear();
    PyObject *pElips;
    if (PyArg_ParseTupleAndKeywords(args, kwds, "O!",keywords_e, &(Ellipse2dPy::Type), &pElips)) {
        Ellipse2dPy* pEllipse = static_cast<Ellipse2dPy*>(pElips);
        Handle(Geom2d_Ellipse) Elips1 = Handle(Geom2d_Ellipse)::DownCast
            (pEllipse->getGeom2dEllipsePtr()->handle());
        Handle(Geom2d_Ellipse) Elips2 = Handle(Geom2d_Ellipse)::DownCast
            (this->getGeom2dEllipsePtr()->handle());
        Elips2->SetElips2d(Elips1->Elips2d());
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
        GCE2d_MakeEllipse me(gp_Pnt2d(v1.x,v1.y),
                             gp_Pnt2d(v2.x,v2.y),
                             gp_Pnt2d(v3.x,v3.y));
        if (!me.IsDone()) {
            PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(me.Status()));
            return -1;
        }

        Handle(Geom2d_Ellipse) ellipse = Handle(Geom2d_Ellipse)::DownCast(getGeom2dEllipsePtr()->handle());
        ellipse->SetElips2d(me.Value()->Elips2d());
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
        GCE2d_MakeEllipse me(gp_Ax2d(gp_Pnt2d(c.x,c.y), gp_Dir2d(0.0,1.0)),
                          major, minor);
        if (!me.IsDone()) {
            PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(me.Status()));
            return -1;
        }

        Handle(Geom2d_Ellipse) ellipse = Handle(Geom2d_Ellipse)::DownCast(getGeom2dEllipsePtr()->handle());
        ellipse->SetElips2d(me.Value()->Elips2d());
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "Ellipse constructor accepts:\n"
        "-- empty parameter list\n"
        "-- Ellipse\n"
        "-- Point, double, double\n"
        "-- Point, Point, Point");
    return -1;
}

Py::Float Ellipse2dPy::getMajorRadius(void) const
{
    Handle(Geom2d_Ellipse) ellipse = Handle(Geom2d_Ellipse)::DownCast(getGeom2dEllipsePtr()->handle());
    return Py::Float(ellipse->MajorRadius()); 
}

void Ellipse2dPy::setMajorRadius(Py::Float arg)
{
    Handle(Geom2d_Ellipse) ellipse = Handle(Geom2d_Ellipse)::DownCast(getGeom2dEllipsePtr()->handle());
    ellipse->SetMajorRadius((double)arg);
}

Py::Float Ellipse2dPy::getMinorRadius(void) const
{
    Handle(Geom2d_Ellipse) ellipse = Handle(Geom2d_Ellipse)::DownCast(getGeom2dEllipsePtr()->handle());
    return Py::Float(ellipse->MinorRadius()); 
}

void Ellipse2dPy::setMinorRadius(Py::Float arg)
{
    Handle(Geom2d_Ellipse) ellipse = Handle(Geom2d_Ellipse)::DownCast(getGeom2dEllipsePtr()->handle());
    ellipse->SetMinorRadius((double)arg);
}

Py::Float Ellipse2dPy::getFocal(void) const
{
    Handle(Geom2d_Ellipse) ellipse = Handle(Geom2d_Ellipse)::DownCast(getGeom2dEllipsePtr()->handle());
    return Py::Float(ellipse->Focal()); 
}

Py::Object Ellipse2dPy::getFocus1(void) const
{
    Handle(Geom2d_Ellipse) ellipse = Handle(Geom2d_Ellipse)::DownCast(getGeom2dEllipsePtr()->handle());
    gp_Pnt2d loc = ellipse->Focus1();

    Py::Module module("__FreeCADBase__");
    Py::Callable method(module.getAttr("Vector2d"));
    Py::Tuple arg(2);
    arg.setItem(0, Py::Float(loc.X()));
    arg.setItem(1, Py::Float(loc.Y()));
    return method.apply(arg);
}

Py::Object Ellipse2dPy::getFocus2(void) const
{
    Handle(Geom2d_Ellipse) ellipse = Handle(Geom2d_Ellipse)::DownCast(getGeom2dEllipsePtr()->handle());
    gp_Pnt2d loc = ellipse->Focus2();

    Py::Module module("__FreeCADBase__");
    Py::Callable method(module.getAttr("Vector2d"));
    Py::Tuple arg(2);
    arg.setItem(0, Py::Float(loc.X()));
    arg.setItem(1, Py::Float(loc.Y()));
    return method.apply(arg);
}

PyObject *Ellipse2dPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int Ellipse2dPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
