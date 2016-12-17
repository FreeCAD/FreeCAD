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
# include <gp_Elips.hxx>
# include <Geom_Ellipse.hxx>
# include <GC_MakeArcOfEllipse.hxx>
# include <GC_MakeEllipse.hxx>
# include <Geom_TrimmedCurve.hxx>
#endif

#include <Mod/Part/App/Geometry2d.h>
#include <Mod/Part/App/Geom2d/ArcOfEllipse2dPy.h>
#include <Mod/Part/App/Geom2d/ArcOfEllipse2dPy.cpp>
#include <Mod/Part/App/EllipsePy.h>
#include <Mod/Part/App/OCCError.h>

#include <Base/GeometryPyCXX.h>
#include <Base/VectorPy.h>

using namespace Part;

extern const char* gce_ErrorStatusText(gce_ErrorType et);

// returns a string which represents the object e.g. when printed in python
std::string ArcOfEllipse2dPy::representation(void) const
{
#if 0
    Handle_Geom_TrimmedCurve trim = Handle_Geom_TrimmedCurve::DownCast
        (getGeomArcOfEllipsePtr()->handle());
    Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(trim->BasisCurve());

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
#else
    return "";
#endif
}

PyObject *ArcOfEllipse2dPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of ArcOfEllipse2dPy and the Twin object
    return new ArcOfEllipse2dPy(new Geom2dArcOfEllipse);
}

// constructor method
int ArcOfEllipse2dPy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
#if 1
    return 0;
#else
    PyObject* o;
    double u1, u2;
    PyObject *sense=Py_True;
    if (PyArg_ParseTuple(args, "O!dd|O!", &(Part::EllipsePy::Type), &o, &u1, &u2, &PyBool_Type, &sense)) {
        try {
            Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast
                (static_cast<EllipsePy*>(o)->getGeomEllipsePtr()->handle());
            GC_MakeArcOfEllipse arc(ellipse->Elips(), u1, u2, PyObject_IsTrue(sense) ? Standard_True : Standard_False);
            if (!arc.IsDone()) {
                PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(arc.Status()));
                return -1;
            }

            getGeomArcOfEllipsePtr()->setHandle(arc.Value());
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
        "ArcOfEllipse constructor expects an ellipse curve and a parameter range");
    return -1;
#endif
}
#if 0
Py::Float ArcOfEllipse2dPy::getMajorRadius(void) const
{
    return Py::Float(getGeomArcOfEllipsePtr()->getMajorRadius()); 
}

void  ArcOfEllipse2dPy::setMajorRadius(Py::Float arg)
{
    getGeomArcOfEllipsePtr()->setMajorRadius((double)arg);
}

Py::Float ArcOfEllipse2dPy::getMinorRadius(void) const
{
    return Py::Float(getGeomArcOfEllipsePtr()->getMinorRadius()); 
}

void  ArcOfEllipse2dPy::setMinorRadius(Py::Float arg)
{
    getGeomArcOfEllipsePtr()->setMinorRadius((double)arg);
}

Py::Float ArcOfEllipse2dPy::getAngleXU(void) const
{
    return Py::Float(getGeomArcOfEllipsePtr()->getAngleXU()); 
}

void ArcOfEllipse2dPy::setAngleXU(Py::Float arg)
{
    getGeomArcOfEllipsePtr()->setAngleXU((double)arg);
}

Py::Object ArcOfEllipse2dPy::getCenter(void) const
{
    return Py::Vector(getGeomArcOfEllipsePtr()->getCenter());
}

void  ArcOfEllipse2dPy::setCenter(Py::Object arg)
{
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        Base::Vector3d loc = static_cast<Base::VectorPy*>(p)->value();
        getGeomArcOfEllipsePtr()->setCenter(loc);
    }
    else if (PyObject_TypeCheck(p, &PyTuple_Type)) {
        Base::Vector3d loc = Base::getVectorFromTuple<double>(p);
        getGeomArcOfEllipsePtr()->setCenter(loc);
    }
    else {
        std::string error = std::string("type must be 'Vector', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

Py::Object ArcOfEllipse2dPy::getAxis(void) const
{
    Handle_Geom_TrimmedCurve trim = Handle_Geom_TrimmedCurve::DownCast
        (getGeomArcOfEllipsePtr()->handle());
    Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(trim->BasisCurve());
    gp_Ax1 axis = ellipse->Axis();
    gp_Dir dir = axis.Direction();
    return Py::Vector(Base::Vector3d(dir.X(), dir.Y(), dir.Z()));
}

void  ArcOfEllipse2dPy::setAxis(Py::Object arg)
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

    Handle_Geom_TrimmedCurve trim = Handle_Geom_TrimmedCurve::DownCast
        (getGeomArcOfEllipsePtr()->handle());
    Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(trim->BasisCurve());
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

Py::Object ArcOfEllipse2dPy::getEllipse(void) const
{
    Handle_Geom_TrimmedCurve trim = Handle_Geom_TrimmedCurve::DownCast
        (getGeomArcOfEllipsePtr()->handle());
    Handle_Geom_Ellipse ellipse = Handle_Geom_Ellipse::DownCast(trim->BasisCurve());
    return Py::Object(new EllipsePy(new GeomEllipse(ellipse)), true);
}
#endif
PyObject *ArcOfEllipse2dPy::getCustomAttributes(const char* ) const
{
    return 0;
}

int ArcOfEllipse2dPy::setCustomAttributes(const char* , PyObject *)
{
    return 0; 
}
