/***************************************************************************
 *   Copyright (c) 2014 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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
# include <GC_MakeArcOfParabola.hxx>
# include <Geom_Parabola.hxx>
# include <Geom_TrimmedCurve.hxx>
#endif

#include "ArcOfParabolaPy.h"
#include "ArcOfParabolaPy.cpp"
#include "ParabolaPy.h"
#include "OCCError.h"


using namespace Part;

extern const char* gce_ErrorStatusText(gce_ErrorType et);

// returns a string which represents the object e.g. when printed in python
std::string ArcOfParabolaPy::representation() const
{
    Handle(Geom_TrimmedCurve) trim = Handle(Geom_TrimmedCurve)::DownCast
        (getGeomArcOfParabolaPtr()->handle());
    Handle(Geom_Parabola) parabola = Handle(Geom_Parabola)::DownCast(trim->BasisCurve());

    gp_Ax1 axis = parabola->Axis();
    gp_Dir dir = axis.Direction();
    gp_Pnt loc = axis.Location();
    Standard_Real fFocal = parabola->Focal();
    Standard_Real u1 = trim->FirstParameter();
    Standard_Real u2 = trim->LastParameter();

    gp_Dir normal = parabola->Axis().Direction();
    gp_Dir xdir = parabola->XAxis().Direction();

    gp_Ax2 xdirref(loc, normal); // this is a reference XY for the parabola

    Standard_Real fAngleXU = -xdir.AngleWithRef(xdirref.XDirection(),normal);


    std::stringstream str;
    str << "ArcOfParabola (";
    str << "Focal : " << fFocal << ", ";
    str << "AngleXU : " << fAngleXU << ", ";
    str << "Position : (" << loc.X() << ", "<< loc.Y() << ", "<< loc.Z() << "), ";
    str << "Direction : (" << dir.X() << ", "<< dir.Y() << ", "<< dir.Z() << "), ";
    str << "Parameter : (" << u1 << ", " << u2 << ")";
    str << ")";

    return str.str();
}

PyObject *ArcOfParabolaPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of ArcOfParabolaPy and the Twin object
    return new ArcOfParabolaPy(new GeomArcOfParabola);
}

// constructor method
int ArcOfParabolaPy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
    PyObject* o;
    double u1, u2;
    PyObject *sense=Py_True;
    if (PyArg_ParseTuple(args, "O!dd|O!", &(Part::ParabolaPy::Type), &o, &u1, &u2, &PyBool_Type, &sense)) {
        try {
            Handle(Geom_Parabola) parabola = Handle(Geom_Parabola)::DownCast
                (static_cast<ParabolaPy*>(o)->getGeomParabolaPtr()->handle());
            GC_MakeArcOfParabola arc(parabola->Parab(), u1, u2, Base::asBoolean(sense));
            if (!arc.IsDone()) {
                PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(arc.Status()));
                return -1;
            }

            getGeomArcOfParabolaPtr()->setHandle(arc.Value());
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
        "ArcOfParabola constructor expects an parabola curve and a parameter range");
    return -1;
}

Py::Float ArcOfParabolaPy::getFocal() const
{
    return Py::Float(getGeomArcOfParabolaPtr()->getFocal());
}

void  ArcOfParabolaPy::setFocal(Py::Float arg)
{
    getGeomArcOfParabolaPtr()->setFocal((double)arg);
}

Py::Object ArcOfParabolaPy::getParabola() const
{
    Handle(Geom_TrimmedCurve) trim = Handle(Geom_TrimmedCurve)::DownCast
        (getGeomArcOfParabolaPtr()->handle());
    Handle(Geom_Parabola) parabola = Handle(Geom_Parabola)::DownCast(trim->BasisCurve());
    return Py::Object(new ParabolaPy(new GeomParabola(parabola)), true);
}

PyObject *ArcOfParabolaPy::getCustomAttributes(const char* ) const
{
    return nullptr;
}

int ArcOfParabolaPy::setCustomAttributes(const char* , PyObject *)
{
    return 0;
}
