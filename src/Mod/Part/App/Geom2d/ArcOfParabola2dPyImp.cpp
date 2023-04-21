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
# include <GCE2d_MakeArcOfParabola.hxx>
# include <Geom2d_Parabola.hxx>
# include <Geom2d_TrimmedCurve.hxx>
# include <gp_Parab2d.hxx>
#endif

#include "Geom2d/ArcOfParabola2dPy.h"
#include "Geom2d/ArcOfParabola2dPy.cpp"
#include "Geom2d/Parabola2dPy.h"
#include "OCCError.h"


using namespace Part;

extern const char* gce_ErrorStatusText(gce_ErrorType et);

// returns a string which represents the object e.g. when printed in python
std::string ArcOfParabola2dPy::representation() const
{
    return "<ArcOfParabola2d object>";
}

PyObject *ArcOfParabola2dPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of ArcOfParabola2dPy and the Twin object
    return new ArcOfParabola2dPy(new Geom2dArcOfParabola);
}

// constructor method
int ArcOfParabola2dPy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
    PyObject* o;
    double u1, u2;
    PyObject *sense=Py_True;
    if (PyArg_ParseTuple(args, "O!dd|O!", &(Part::Parabola2dPy::Type), &o, &u1, &u2, &PyBool_Type, &sense)) {
        try {
            Handle(Geom2d_Parabola) parabola = Handle(Geom2d_Parabola)::DownCast
                (static_cast<Parabola2dPy*>(o)->getGeom2dParabolaPtr()->handle());
            GCE2d_MakeArcOfParabola arc(parabola->Parab2d(), u1, u2, Base::asBoolean(sense));
            if (!arc.IsDone()) {
                PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(arc.Status()));
                return -1;
            }

            getGeom2dArcOfParabolaPtr()->setHandle(arc.Value());
            return 0;
        }
        catch (Standard_Failure& e) {

            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return -1;
        }
        catch (...) {
            PyErr_SetString(PartExceptionOCCError, "creation of arc failed");
            return -1;
        }
    }

    // All checks failed
    PyErr_SetString(PyExc_TypeError,
        "ArcOfParabola2d constructor expects an parabola curve and a parameter range");
    return -1;
}

Py::Float ArcOfParabola2dPy::getFocal() const
{
    return Py::Float(getGeom2dArcOfParabolaPtr()->getFocal());
}

void  ArcOfParabola2dPy::setFocal(Py::Float arg)
{
    getGeom2dArcOfParabolaPtr()->setFocal((double)arg);
}

Py::Object ArcOfParabola2dPy::getParabola() const
{
    Handle(Geom2d_TrimmedCurve) trim = Handle(Geom2d_TrimmedCurve)::DownCast
        (getGeom2dArcOfParabolaPtr()->handle());
    Handle(Geom2d_Parabola) parabola = Handle(Geom2d_Parabola)::DownCast(trim->BasisCurve());
    return Py::asObject(new Parabola2dPy(new Geom2dParabola(parabola)));
}

PyObject *ArcOfParabola2dPy::getCustomAttributes(const char* ) const
{
    return nullptr;
}

int ArcOfParabola2dPy::setCustomAttributes(const char* , PyObject *)
{
    return 0;
}
