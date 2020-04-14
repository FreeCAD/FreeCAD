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
# include <gp.hxx>
# include <gp_Lin2d.hxx>
# include <Geom2d_Line.hxx>
# include <Geom2d_TrimmedCurve.hxx>
# include <GCE2d_MakeLine.hxx>
# include <GCE2d_MakeSegment.hxx>
# include <Precision.hxx>
#endif

#include <Base/GeometryPyCXX.h>

#include <Mod/Part/App/OCCError.h>
#include <Mod/Part/App/Geometry2d.h>
#include <Mod/Part/App/Geom2d/Line2dPy.h>
#include <Mod/Part/App/Geom2d/Line2dSegmentPy.h>
#include <Mod/Part/App/Geom2d/Line2dSegmentPy.cpp>

using namespace Part;

extern const char* gce_ErrorStatusText(gce_ErrorType et);

// returns a string which represents the object e.g. when printed in python
std::string Line2dSegmentPy::representation(void) const
{
    return "<Line2dSegment object>";
}

PyObject *Line2dSegmentPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of Line2dSegmentPy and the Twin object
    return new Line2dSegmentPy(new Geom2dLineSegment);
}

// constructor method
int Line2dSegmentPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    if (PyArg_ParseTuple(args, "")) {
        // default line
        return 0;
    }

    PyErr_Clear();
    PyObject *pLine;
    if (PyArg_ParseTuple(args, "O!", &(Line2dSegmentPy::Type), &pLine)) {
        // Copy line
        Line2dSegmentPy* pcLine = static_cast<Line2dSegmentPy*>(pLine);
        // get Geom_Line of line segment
        Handle(Geom2d_TrimmedCurve) that_curv = Handle(Geom2d_TrimmedCurve)::DownCast
            (pcLine->getGeom2dLineSegmentPtr()->handle());
        Handle(Geom2d_Line) that_line = Handle(Geom2d_Line)::DownCast
            (that_curv->BasisCurve());
        // get Geom_Line of line segment
        Handle(Geom2d_TrimmedCurve) this_curv = Handle(Geom2d_TrimmedCurve)::DownCast
            (this->getGeom2dLineSegmentPtr()->handle());
        Handle(Geom2d_Line) this_line = Handle(Geom2d_Line)::DownCast
            (this_curv->BasisCurve());

        // Assign the lines
        this_line->SetLin2d(that_line->Lin2d());
        this_curv->SetTrim(that_curv->FirstParameter(), that_curv->LastParameter());
        return 0;
    }

    PyErr_Clear();
    double first, last;
    if (PyArg_ParseTuple(args, "O!dd", &(Line2dSegmentPy::Type), &pLine, &first, &last)) {
        // Copy line
        Line2dSegmentPy* pcLine = static_cast<Line2dSegmentPy*>(pLine);
        // get Geom_Line of line segment
        Handle(Geom2d_TrimmedCurve) that_curv = Handle(Geom2d_TrimmedCurve)::DownCast
            (pcLine->getGeom2dLineSegmentPtr()->handle());
        Handle(Geom2d_Line) that_line = Handle(Geom2d_Line)::DownCast
            (that_curv->BasisCurve());
        // get Geom_Line of line segment
        Handle(Geom2d_TrimmedCurve) this_curv = Handle(Geom2d_TrimmedCurve)::DownCast
            (this->getGeom2dLineSegmentPtr()->handle());
        Handle(Geom2d_Line) this_line = Handle(Geom2d_Line)::DownCast
            (this_curv->BasisCurve());

        // Assign the lines
        this_line->SetLin2d(that_line->Lin2d());
        this_curv->SetTrim(first, last);
        return 0;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!dd", &(Line2dPy::Type), &pLine, &first, &last)) {
        // Copy line
        Line2dPy* pcLine = static_cast<Line2dPy*>(pLine);
        // get Geom_Line of line
        Handle(Geom2d_Line) that_line = Handle(Geom2d_Line)::DownCast
            (pcLine->getGeom2dLinePtr()->handle());
        // get Geom_Line of line segment
        Handle(Geom2d_TrimmedCurve) this_curv = Handle(Geom2d_TrimmedCurve)::DownCast
            (this->getGeom2dLineSegmentPtr()->handle());
        Handle(Geom2d_Line) this_line = Handle(Geom2d_Line)::DownCast
            (this_curv->BasisCurve());

        // Assign the lines
        this_line->SetLin2d(that_line->Lin2d());
        this_curv->SetTrim(first, last);
        return 0;
    }

    PyErr_Clear();
    PyObject *pV1, *pV2;
    if (PyArg_ParseTuple(args, "O!O!", Base::Vector2dPy::type_object(), &pV1,
                                       Base::Vector2dPy::type_object(), &pV2)) {
        Base::Vector2d v1 = Py::toVector2d(pV1);
        Base::Vector2d v2 = Py::toVector2d(pV2);
        try {
            // Create line out of two points
            double distance = (v1-v2).Length();
            if (distance < gp::Resolution())
                Standard_Failure::Raise("Both points are equal");
            GCE2d_MakeSegment ms(gp_Pnt2d(v1.x,v1.y),
                                 gp_Pnt2d(v2.x,v2.y));
            if (!ms.IsDone()) {
                PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(ms.Status()));
                return -1;
            }

            // get Geom_Line of line segment
            Handle(Geom2d_TrimmedCurve) this_curv = Handle(Geom2d_TrimmedCurve)::DownCast
                (this->getGeom2dLineSegmentPtr()->handle());
            Handle(Geom2d_Line) this_line = Handle(Geom2d_Line)::DownCast
                (this_curv->BasisCurve());
            Handle(Geom2d_TrimmedCurve) that_curv = ms.Value();
            Handle(Geom2d_Line) that_line = Handle(Geom2d_Line)::DownCast(that_curv->BasisCurve());
            this_line->SetLin2d(that_line->Lin2d());
            this_curv->SetTrim(that_curv->FirstParameter(), that_curv->LastParameter());
            return 0;
        }
        catch (Standard_Failure& e) {
    
            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return -1;
        }
        catch (...) {
            PyErr_SetString(PartExceptionOCCError, "creation of line failed");
            return -1;
        }
    }

    PyErr_SetString(PyExc_TypeError, "Line2dSegment constructor accepts:\n"
        "-- empty parameter list\n"
        "-- Line2dSegment\n"
        "-- Line2dSegment, float, float\n"
        "-- Line2d, float, float\n"
        "-- Point, Point");
    return -1;
}

PyObject* Line2dSegmentPy::setParameterRange(PyObject *args)
{
    double first, last;
    if (!PyArg_ParseTuple(args, "dd", &first, &last))
        return NULL;

    try {
        Handle(Geom2d_TrimmedCurve) this_curve = Handle(Geom2d_TrimmedCurve)::DownCast
            (this->getGeom2dLineSegmentPtr()->handle());
        this_curve->SetTrim(first, last);
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return NULL;
    }

    Py_Return; 
}

Py::Object Line2dSegmentPy::getStartPoint(void) const
{
    Handle(Geom2d_TrimmedCurve) this_curve = Handle(Geom2d_TrimmedCurve)::DownCast
        (this->getGeom2dLineSegmentPtr()->handle());
    gp_Pnt2d pnt = this_curve->StartPoint();

    Py::Module module("__FreeCADBase__");
    Py::Callable method(module.getAttr("Vector2d"));
    Py::Tuple arg(2);
    arg.setItem(0, Py::Float(pnt.X()));
    arg.setItem(1, Py::Float(pnt.Y()));
    return method.apply(arg);
}

void Line2dSegmentPy::setStartPoint(Py::Object arg)
{
    gp_Pnt2d p1, p2;
    Handle(Geom2d_TrimmedCurve) this_curv = Handle(Geom2d_TrimmedCurve)::DownCast
        (this->getGeom2dLineSegmentPtr()->handle());
    p2 = this_curv->EndPoint();

    PyObject *p = arg.ptr();
    if (PyObject_TypeCheck(p, Base::Vector2dPy::type_object())) {
        Base::Vector2d v = Py::toVector2d(p);
        p1.SetX(v.x);
        p1.SetY(v.y);
    }
    else if (PyTuple_Check(p)) {
        Py::Tuple tuple(arg);
        p1.SetX((double)Py::Float(tuple.getItem(0)));
        p1.SetY((double)Py::Float(tuple.getItem(1)));
    }
    else {
        std::string error = std::string("type must be 'Vector2d' or tuple, not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }

    try {
        // Create line out of two points
        if (p1.Distance(p2) < gp::Resolution())
            Standard_Failure::Raise("Both points are equal");
        GCE2d_MakeSegment ms(p1, p2);
        if (!ms.IsDone()) {
            throw Py::RuntimeError(gce_ErrorStatusText(ms.Status()));
        }

        // get Geom_Line of line segment
        Handle(Geom2d_Line) this_line = Handle(Geom2d_Line)::DownCast
            (this_curv->BasisCurve());
        Handle(Geom2d_TrimmedCurve) that_curv = ms.Value();
        Handle(Geom2d_Line) that_line = Handle(Geom2d_Line)::DownCast(that_curv->BasisCurve());
        this_line->SetLin2d(that_line->Lin2d());
        this_curv->SetTrim(that_curv->FirstParameter(), that_curv->LastParameter());
    }
    catch (Standard_Failure& e) {
        throw Py::RuntimeError(e.GetMessageString());
    }
}

Py::Object Line2dSegmentPy::getEndPoint(void) const
{
    Handle(Geom2d_TrimmedCurve) this_curve = Handle(Geom2d_TrimmedCurve)::DownCast
        (this->getGeom2dLineSegmentPtr()->handle());
    gp_Pnt2d pnt = this_curve->EndPoint();

    Py::Module module("__FreeCADBase__");
    Py::Callable method(module.getAttr("Vector2d"));
    Py::Tuple arg(2);
    arg.setItem(0, Py::Float(pnt.X()));
    arg.setItem(1, Py::Float(pnt.Y()));
    return method.apply(arg);
}

void Line2dSegmentPy::setEndPoint(Py::Object arg)
{
    gp_Pnt2d p1, p2;
    Handle(Geom2d_TrimmedCurve) this_curv = Handle(Geom2d_TrimmedCurve)::DownCast
        (this->getGeom2dLineSegmentPtr()->handle());
    p1 = this_curv->StartPoint();

    PyObject *p = arg.ptr();
    if (PyObject_TypeCheck(p, Base::Vector2dPy::type_object())) {
        Base::Vector2d v = Py::toVector2d(p);
        p2.SetX(v.x);
        p2.SetY(v.y);
    }
    else if (PyTuple_Check(p)) {
        Py::Tuple tuple(arg);
        p2.SetX((double)Py::Float(tuple.getItem(0)));
        p2.SetY((double)Py::Float(tuple.getItem(1)));
    }
    else {
        std::string error = std::string("type must be 'Vector' or tuple, not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }

    try {
        // Create line out of two points
        if (p1.Distance(p2) < gp::Resolution())
            Standard_Failure::Raise("Both points are equal");
        GCE2d_MakeSegment ms(p1, p2);
        if (!ms.IsDone()) {
            throw Py::RuntimeError(gce_ErrorStatusText(ms.Status()));
        }

        // get Geom_Line of line segment
        Handle(Geom2d_Line) this_line = Handle(Geom2d_Line)::DownCast
            (this_curv->BasisCurve());
        Handle(Geom2d_TrimmedCurve) that_curv = ms.Value();
        Handle(Geom2d_Line) that_line = Handle(Geom2d_Line)::DownCast(that_curv->BasisCurve());
        this_line->SetLin2d(that_line->Lin2d());
        this_curv->SetTrim(that_curv->FirstParameter(), that_curv->LastParameter());
    }
    catch (Standard_Failure& e) {
        throw Py::RuntimeError(e.GetMessageString());
    }
}

PyObject *Line2dSegmentPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int Line2dSegmentPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
