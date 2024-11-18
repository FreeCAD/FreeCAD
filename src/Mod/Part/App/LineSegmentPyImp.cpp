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
# include <GC_MakeSegment.hxx>
# include <Geom_Line.hxx>
# include <Geom_TrimmedCurve.hxx>
#endif

#include <Base/GeometryPyCXX.h>
#include <Base/VectorPy.h>

#include "LineSegmentPy.h"
#include "LineSegmentPy.cpp"
#include "LinePy.h"
#include "OCCError.h"


using namespace Part;

extern const char* gce_ErrorStatusText(gce_ErrorType et);

// returns a string which represents the object e.g. when printed in python
std::string LineSegmentPy::representation() const
{
    std::stringstream str;
    Base::Vector3d start = getGeomLineSegmentPtr()->getStartPoint();
    Base::Vector3d end   = getGeomLineSegmentPtr()->getEndPoint();
    str << "<Line segment ("
        << start.x << "," <<start.y << "," <<start.z << ") ("
        << end.x   << "," <<end.y   << "," <<end.z   << ") >";

    return str.str();
}

PyObject *LineSegmentPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of LineSegmentPy and the Twin object
    return new LineSegmentPy(new GeomLineSegment);
}

// constructor method
int LineSegmentPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    if (PyArg_ParseTuple(args, "")) {
        return 0;
    }

    PyErr_Clear();
    PyObject *pLine;
    if (PyArg_ParseTuple(args, "O!", &(LineSegmentPy::Type), &pLine)) {
        // Copy line
        LineSegmentPy* pcLine = static_cast<LineSegmentPy*>(pLine);
        // get Geom_Line of line segment
        Handle(Geom_TrimmedCurve) that_curv = Handle(Geom_TrimmedCurve)::DownCast
            (pcLine->getGeomLineSegmentPtr()->handle());
        Handle(Geom_Line) that_line = Handle(Geom_Line)::DownCast
            (that_curv->BasisCurve());
        // get Geom_Line of line segment
        Handle(Geom_TrimmedCurve) this_curv = Handle(Geom_TrimmedCurve)::DownCast
            (this->getGeomLineSegmentPtr()->handle());
        Handle(Geom_Line) this_line = Handle(Geom_Line)::DownCast
            (this_curv->BasisCurve());

        // Assign the lines
        this_line->SetLin(that_line->Lin());
        this_curv->SetTrim(that_curv->FirstParameter(), that_curv->LastParameter());
        return 0;
    }

    PyErr_Clear();
    double first, last;
    if (PyArg_ParseTuple(args, "O!dd", &(LineSegmentPy::Type), &pLine, &first, &last)) {
        // Copy line
        LineSegmentPy* pcLine = static_cast<LineSegmentPy*>(pLine);
        // get Geom_Line of line segment
        Handle(Geom_TrimmedCurve) that_curv = Handle(Geom_TrimmedCurve)::DownCast
            (pcLine->getGeomLineSegmentPtr()->handle());
        Handle(Geom_Line) that_line = Handle(Geom_Line)::DownCast
            (that_curv->BasisCurve());
        // get Geom_Line of line segment
        Handle(Geom_TrimmedCurve) this_curv = Handle(Geom_TrimmedCurve)::DownCast
            (this->getGeomLineSegmentPtr()->handle());
        Handle(Geom_Line) this_line = Handle(Geom_Line)::DownCast
            (this_curv->BasisCurve());

        // Assign the lines
        this_line->SetLin(that_line->Lin());
        this_curv->SetTrim(first, last);
        return 0;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!dd", &(LinePy::Type), &pLine, &first, &last)) {
        // Copy line
        LinePy* pcLine = static_cast<LinePy*>(pLine);
        // get Geom_Line of line segment
        Handle(Geom_Line) that_line = Handle(Geom_Line)::DownCast
            (pcLine->getGeomLinePtr()->handle());
        // get Geom_Line of line segment
        Handle(Geom_TrimmedCurve) this_curv = Handle(Geom_TrimmedCurve)::DownCast
            (this->getGeomLineSegmentPtr()->handle());
        Handle(Geom_Line) this_line = Handle(Geom_Line)::DownCast
            (this_curv->BasisCurve());

        // Assign the lines
        this_line->SetLin(that_line->Lin());
        this_curv->SetTrim(first, last);
        return 0;
    }

    PyErr_Clear();
    PyObject *pV1, *pV2;
    if (PyArg_ParseTuple(args, "O!O!", &(Base::VectorPy::Type), &pV1,
                                       &(Base::VectorPy::Type), &pV2)) {
        Base::Vector3d v1 = static_cast<Base::VectorPy*>(pV1)->value();
        Base::Vector3d v2 = static_cast<Base::VectorPy*>(pV2)->value();
        try {
            // Create line out of two points
            double distance = Base::Distance(v1, v2);
            if (distance < gp::Resolution())
                Standard_Failure::Raise("Both points are equal");
            GC_MakeSegment ms(gp_Pnt(v1.x,v1.y,v1.z),
                              gp_Pnt(v2.x,v2.y,v2.z));
            if (!ms.IsDone()) {
                PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(ms.Status()));
                return -1;
            }

            // get Geom_Line of line segment
            Handle(Geom_TrimmedCurve) this_curv = Handle(Geom_TrimmedCurve)::DownCast
                (this->getGeomLineSegmentPtr()->handle());
            Handle(Geom_Line) this_line = Handle(Geom_Line)::DownCast
                (this_curv->BasisCurve());
            Handle(Geom_TrimmedCurve) that_curv = ms.Value();
            Handle(Geom_Line) that_line = Handle(Geom_Line)::DownCast(that_curv->BasisCurve());
            this_line->SetLin(that_line->Lin());
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

    PyErr_SetString(PyExc_TypeError, "Line constructor accepts:\n"
        "-- empty parameter list\n"
        "-- LineSegment\n"
        "-- LineSegment,double,double\n"
        "-- Line,double,double\n"
        "-- Point, Point");
    return -1;
}

PyObject* LineSegmentPy::setParameterRange(PyObject *args)
{
    double first, last;
    if (!PyArg_ParseTuple(args, "dd", &first, &last))
        return nullptr;

    try {
        Handle(Geom_TrimmedCurve) this_curve = Handle(Geom_TrimmedCurve)::DownCast
            (this->getGeomLineSegmentPtr()->handle());
        this_curve->SetTrim(first, last);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    Py_Return;
}

Py::Object LineSegmentPy::getStartPoint() const
{
    Handle(Geom_TrimmedCurve) this_curve = Handle(Geom_TrimmedCurve)::DownCast
        (this->getGeomLineSegmentPtr()->handle());
    gp_Pnt pnt = this_curve->StartPoint();
    return Py::Vector(Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z()));
}

void LineSegmentPy::setStartPoint(Py::Object arg)
{
    gp_Pnt p1, p2;
    Handle(Geom_TrimmedCurve) this_curv = Handle(Geom_TrimmedCurve)::DownCast
        (this->getGeomLineSegmentPtr()->handle());
    p2 = this_curv->EndPoint();

    PyObject *p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        Base::Vector3d v = static_cast<Base::VectorPy*>(p)->value();
        p1.SetX(v.x);
        p1.SetY(v.y);
        p1.SetZ(v.z);
    }
    else if (PyTuple_Check(p)) {
        Py::Tuple tuple(arg);
        p1.SetX((double)Py::Float(tuple.getItem(0)));
        p1.SetY((double)Py::Float(tuple.getItem(1)));
        p1.SetZ((double)Py::Float(tuple.getItem(2)));
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
        GC_MakeSegment ms(p1, p2);
        if (!ms.IsDone()) {
            throw Py::RuntimeError(gce_ErrorStatusText(ms.Status()));
        }

        // get Geom_Line of line segment
        Handle(Geom_Line) this_line = Handle(Geom_Line)::DownCast
            (this_curv->BasisCurve());
        Handle(Geom_TrimmedCurve) that_curv = ms.Value();
        Handle(Geom_Line) that_line = Handle(Geom_Line)::DownCast(that_curv->BasisCurve());
        this_line->SetLin(that_line->Lin());
        this_curv->SetTrim(that_curv->FirstParameter(), that_curv->LastParameter());
    }
    catch (Standard_Failure& e) {
        throw Py::RuntimeError(e.GetMessageString());
    }
}

Py::Object LineSegmentPy::getEndPoint() const
{
    Handle(Geom_TrimmedCurve) this_curve = Handle(Geom_TrimmedCurve)::DownCast
        (this->getGeomLineSegmentPtr()->handle());
    gp_Pnt pnt = this_curve->EndPoint();
    return Py::Vector(Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z()));
}

void LineSegmentPy::setEndPoint(Py::Object arg)
{
    gp_Pnt p1, p2;
    Handle(Geom_TrimmedCurve) this_curv = Handle(Geom_TrimmedCurve)::DownCast
        (this->getGeomLineSegmentPtr()->handle());
    p1 = this_curv->StartPoint();

    PyObject *p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        Base::Vector3d v = static_cast<Base::VectorPy*>(p)->value();
        p2.SetX(v.x);
        p2.SetY(v.y);
        p2.SetZ(v.z);
    }
    else if (PyTuple_Check(p)) {
        Py::Tuple tuple(arg);
        p2.SetX((double)Py::Float(tuple.getItem(0)));
        p2.SetY((double)Py::Float(tuple.getItem(1)));
        p2.SetZ((double)Py::Float(tuple.getItem(2)));
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
        GC_MakeSegment ms(p1, p2);
        if (!ms.IsDone()) {
            throw Py::RuntimeError(gce_ErrorStatusText(ms.Status()));
        }

        // get Geom_Line of line segment
        Handle(Geom_Line) this_line = Handle(Geom_Line)::DownCast
            (this_curv->BasisCurve());
        Handle(Geom_TrimmedCurve) that_curv = ms.Value();
        Handle(Geom_Line) that_line = Handle(Geom_Line)::DownCast(that_curv->BasisCurve());
        this_line->SetLin(that_line->Lin());
        this_curv->SetTrim(that_curv->FirstParameter(), that_curv->LastParameter());
    }
    catch (Standard_Failure& e) {
        throw Py::RuntimeError(e.GetMessageString());
    }
}

PyObject *LineSegmentPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int LineSegmentPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
