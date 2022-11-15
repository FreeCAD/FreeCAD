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
# include <GC_MakeArcOfEllipse.hxx>
# include <Geom_Ellipse.hxx>
# include <Geom_TrimmedCurve.hxx>
#endif

#include "ArcOfEllipsePy.h"
#include "ArcOfEllipsePy.cpp"
#include "EllipsePy.h"
#include "OCCError.h"


using namespace Part;

extern const char* gce_ErrorStatusText(gce_ErrorType et);

// returns a string which represents the object e.g. when printed in python
std::string ArcOfEllipsePy::representation() const
{
    Handle(Geom_TrimmedCurve) trim = Handle(Geom_TrimmedCurve)::DownCast
        (getGeomArcOfEllipsePtr()->handle());
    Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(trim->BasisCurve());

    gp_Ax1 axis = ellipse->Axis();
    gp_Dir dir = axis.Direction();
    gp_Pnt loc = axis.Location();
    Standard_Real fMajRad = ellipse->MajorRadius();
    Standard_Real fMinRad = ellipse->MinorRadius();
    Standard_Real u1 = trim->FirstParameter();
    Standard_Real u2 = trim->LastParameter();

    gp_Dir normal = ellipse->Axis().Direction();
    gp_Dir xdir = ellipse->XAxis().Direction();

    gp_Ax2 xdirref(loc, normal); // this is a reference XY for the ellipse

    Standard_Real fAngleXU = -xdir.AngleWithRef(xdirref.XDirection(),normal);


    std::stringstream str;
    str << "ArcOfEllipse (";
    str << "MajorRadius : " << fMajRad << ", ";
    str << "MinorRadius : " << fMinRad << ", ";
    str << "AngleXU : " << fAngleXU << ", ";
    str << "Position : (" << loc.X() << ", "<< loc.Y() << ", "<< loc.Z() << "), ";
    str << "Direction : (" << dir.X() << ", "<< dir.Y() << ", "<< dir.Z() << "), ";
    str << "Parameter : (" << u1 << ", " << u2 << ")";
    str << ")";

    return str.str();
}

PyObject *ArcOfEllipsePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of ArcOfEllipsePy and the Twin object
    return new ArcOfEllipsePy(new GeomArcOfEllipse);
}

// constructor method
int ArcOfEllipsePy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
    PyObject* o;
    double u1, u2;
    PyObject *sense=Py_True;
    if (PyArg_ParseTuple(args, "O!dd|O!", &(Part::EllipsePy::Type), &o, &u1, &u2, &PyBool_Type, &sense)) {
        try {
            Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast
                (static_cast<EllipsePy*>(o)->getGeomEllipsePtr()->handle());
            GC_MakeArcOfEllipse arc(ellipse->Elips(), u1, u2, Base::asBoolean(sense));
            if (!arc.IsDone()) {
                PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(arc.Status()));
                return -1;
            }

            getGeomArcOfEllipsePtr()->setHandle(arc.Value());
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
        "ArcOfEllipse constructor expects an ellipse curve and a parameter range");
    return -1;
}

Py::Float ArcOfEllipsePy::getMajorRadius() const
{
    return Py::Float(getGeomArcOfEllipsePtr()->getMajorRadius());
}

void  ArcOfEllipsePy::setMajorRadius(Py::Float arg)
{
    getGeomArcOfEllipsePtr()->setMajorRadius((double)arg);
}

Py::Float ArcOfEllipsePy::getMinorRadius() const
{
    return Py::Float(getGeomArcOfEllipsePtr()->getMinorRadius());
}

void  ArcOfEllipsePy::setMinorRadius(Py::Float arg)
{
    getGeomArcOfEllipsePtr()->setMinorRadius((double)arg);
}

Py::Object ArcOfEllipsePy::getEllipse() const
{
    Handle(Geom_TrimmedCurve) trim = Handle(Geom_TrimmedCurve)::DownCast
        (getGeomArcOfEllipsePtr()->handle());
    Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(trim->BasisCurve());
    return Py::Object(new EllipsePy(new GeomEllipse(ellipse)), true);
}

PyObject *ArcOfEllipsePy::getCustomAttributes(const char* ) const
{
    return nullptr;
}

int ArcOfEllipsePy::setCustomAttributes(const char* , PyObject *)
{
    return 0;
}
