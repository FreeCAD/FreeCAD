/***************************************************************************
 *   Copyright (c) 2014 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com      *
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
# include <gp_Parab.hxx>
# include <Geom_Parabola.hxx>
# include <GC_MakeArcOfParabola.hxx>
# include <gce_MakeParab.hxx>
# include <Geom_TrimmedCurve.hxx>
#endif

#include <Mod/Part/App/Geometry2d.h>
#include <Mod/Part/App/Geom2d/ArcOfParabola2dPy.h>
#include <Mod/Part/App/Geom2d/ArcOfParabola2dPy.cpp>
#include <Mod/Part/App/ParabolaPy.h>
#include <Mod/Part/App/OCCError.h>

#include <Base/GeometryPyCXX.h>
#include <Base/VectorPy.h>

using namespace Part;

extern const char* gce_ErrorStatusText(gce_ErrorType et);

// returns a string which represents the object e.g. when printed in python
std::string ArcOfParabola2dPy::representation(void) const
{
#if 0
    Handle_Geom_TrimmedCurve trim = Handle_Geom_TrimmedCurve::DownCast
        (getGeomArcOfParabolaPtr()->handle());
    Handle_Geom_Parabola parabola = Handle_Geom_Parabola::DownCast(trim->BasisCurve());

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
#else
    return "";
#endif
}

PyObject *ArcOfParabola2dPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of ArcOfParabola2dPy and the Twin object
    return new ArcOfParabola2dPy(new Geom2dArcOfParabola);
}

// constructor method
int ArcOfParabola2dPy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
#if 1
    return 0;
#else
    PyObject* o;
    double u1, u2;
    PyObject *sense=Py_True;
    if (PyArg_ParseTuple(args, "O!dd|O!", &(Part::ParabolaPy::Type), &o, &u1, &u2, &PyBool_Type, &sense)) {
        try {
            Handle_Geom_Parabola parabola = Handle_Geom_Parabola::DownCast
                (static_cast<ParabolaPy*>(o)->getGeomParabolaPtr()->handle());
            GC_MakeArcOfParabola arc(parabola->Parab(), u1, u2, PyObject_IsTrue(sense) ? Standard_True : Standard_False);
            if (!arc.IsDone()) {
                PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(arc.Status()));
                return -1;
            }

            getGeomArcOfParabolaPtr()->setHandle(arc.Value());
            return 0;
        }
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            PyErr_SetString(PartExceptionOCCError, e->GetMessageString());
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
#endif
}
#if 0
Py::Float ArcOfParabola2dPy::getFocal(void) const
{
    return Py::Float(getGeomArcOfParabolaPtr()->getFocal()); 
}

void  ArcOfParabola2dPy::setFocal(Py::Float arg)
{
    getGeomArcOfParabolaPtr()->setFocal((double)arg);
}

Py::Object ArcOfParabola2dPy::getParabola(void) const
{
    Handle_Geom_TrimmedCurve trim = Handle_Geom_TrimmedCurve::DownCast
        (getGeomArcOfParabolaPtr()->handle());
    Handle_Geom_Parabola parabola = Handle_Geom_Parabola::DownCast(trim->BasisCurve());
    return Py::Object(new ParabolaPy(new GeomParabola(parabola)), true);
}
#endif
PyObject *ArcOfParabola2dPy::getCustomAttributes(const char* ) const
{
    return 0;
}

int ArcOfParabola2dPy::setCustomAttributes(const char* , PyObject *)
{
    return 0; 
}
