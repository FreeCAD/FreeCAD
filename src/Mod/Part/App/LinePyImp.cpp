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
# include <gp.hxx>
# include <gp_Lin.hxx>
# include <Geom_Line.hxx>
# include <Geom_TrimmedCurve.hxx>
# include <GC_MakeLine.hxx>
# include <GC_MakeSegment.hxx>
# include <Precision.hxx>
#endif

#include <Base/VectorPy.h>
#include <Base/GeometryPyCXX.h>

#include "Geometry.h"
#include "LinePy.h"
#include "LinePy.cpp"

using namespace Part;

extern const char* gce_ErrorStatusText(gce_ErrorType et);

// returns a string which represents the object e.g. when printed in python
std::string LinePy::representation(void) const
{
    std::stringstream str;
    //if(Infinite)
    //   str << "<Line infinite>";
    //else {
        Base::Vector3d start = getGeomLineSegmentPtr()->getStartPoint();
        Base::Vector3d end   = getGeomLineSegmentPtr()->getEndPoint();
        str << "<Line (" 
            << start.x << "," <<start.y << "," <<start.z << ") (" 
            << end.x   << "," <<end.y   << "," <<end.z   << ") >"; 
    //}

    return str.str();
}

PyObject *LinePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of LinePy and the Twin object 
    return new LinePy(new GeomLineSegment);
}

// constructor method
int LinePy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    
    if (PyArg_ParseTuple(args, "")) {
        // default line
        Infinite=false;
        return 0;
    }

    PyErr_Clear();
    PyObject *pLine;
    if (PyArg_ParseTuple(args, "O!", &(LinePy::Type), &pLine)) {
        // Copy line
        LinePy* pcLine = static_cast<LinePy*>(pLine);
        // get Geom_Line of line segment
        Handle_Geom_TrimmedCurve that_curv = Handle_Geom_TrimmedCurve::DownCast
            (pcLine->getGeomLineSegmentPtr()->handle());
        Handle_Geom_Line that_line = Handle_Geom_Line::DownCast
            (that_curv->BasisCurve());
        // get Geom_Line of line segment
        Handle_Geom_TrimmedCurve this_curv = Handle_Geom_TrimmedCurve::DownCast
            (this->getGeomLineSegmentPtr()->handle());
        Handle_Geom_Line this_line = Handle_Geom_Line::DownCast
            (this_curv->BasisCurve());

        Infinite = pcLine->Infinite;

        // Assign the lines
        this_line->SetLin(that_line->Lin());
        this_curv->SetTrim(that_curv->FirstParameter(), that_curv->LastParameter());
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
                PyErr_SetString(PyExc_Exception, gce_ErrorStatusText(ms.Status()));
                return -1;
            }

            // get Geom_Line of line segment
            Handle_Geom_TrimmedCurve this_curv = Handle_Geom_TrimmedCurve::DownCast
                (this->getGeomLineSegmentPtr()->handle());
            Handle_Geom_Line this_line = Handle_Geom_Line::DownCast
                (this_curv->BasisCurve());
            Handle_Geom_TrimmedCurve that_curv = ms.Value();
            Handle_Geom_Line that_line = Handle_Geom_Line::DownCast(that_curv->BasisCurve());
            this_line->SetLin(that_line->Lin());
            this_curv->SetTrim(that_curv->FirstParameter(), that_curv->LastParameter());

            Infinite = false;
            return 0;
        }
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            PyErr_SetString(PyExc_Exception, e->GetMessageString());
            return -1;
        }
        catch (...) {
            PyErr_SetString(PyExc_Exception, "creation of line failed");
            return -1;
        }
    }

    PyErr_SetString(PyExc_TypeError, "Line constructor accepts:\n"
        "-- empty parameter list\n"
        "-- Line\n"
        "-- Point, Point");
    return -1;
}

PyObject* LinePy::setParameterRange(PyObject *args)
{
    double first, last;
    if (!PyArg_ParseTuple(args, "dd", &first, &last))
        return NULL;

    try {
        Handle_Geom_TrimmedCurve this_curve = Handle_Geom_TrimmedCurve::DownCast
            (this->getGeomLineSegmentPtr()->handle());
        this_curve->SetTrim(first, last);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return NULL;
    }

    Py_Return; 
}

Py::Object LinePy::getStartPoint(void) const
{
    Handle_Geom_TrimmedCurve this_curve = Handle_Geom_TrimmedCurve::DownCast
        (this->getGeomLineSegmentPtr()->handle());
    gp_Pnt pnt = this_curve->StartPoint();
    return Py::Vector(Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z()));
}

void LinePy::setStartPoint(Py::Object arg)
{
    gp_Pnt p1, p2;
    Handle_Geom_TrimmedCurve this_curv = Handle_Geom_TrimmedCurve::DownCast
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
            throw Py::Exception(gce_ErrorStatusText(ms.Status()));
        }

        // get Geom_Line of line segment
        Handle_Geom_Line this_line = Handle_Geom_Line::DownCast
            (this_curv->BasisCurve());
        Handle_Geom_TrimmedCurve that_curv = ms.Value();
        Handle_Geom_Line that_line = Handle_Geom_Line::DownCast(that_curv->BasisCurve());
        this_line->SetLin(that_line->Lin());
        this_curv->SetTrim(that_curv->FirstParameter(), that_curv->LastParameter());
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        throw Py::Exception(e->GetMessageString());
    }
}

Py::Object LinePy::getEndPoint(void) const
{
    Handle_Geom_TrimmedCurve this_curve = Handle_Geom_TrimmedCurve::DownCast
        (this->getGeomLineSegmentPtr()->handle());
    gp_Pnt pnt = this_curve->EndPoint();
    return Py::Vector(Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z()));
}

void LinePy::setEndPoint(Py::Object arg)
{
    gp_Pnt p1, p2;
    Handle_Geom_TrimmedCurve this_curv = Handle_Geom_TrimmedCurve::DownCast
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
            throw Py::Exception(gce_ErrorStatusText(ms.Status()));
        }

        // get Geom_Line of line segment
        Handle_Geom_Line this_line = Handle_Geom_Line::DownCast
            (this_curv->BasisCurve());
        Handle_Geom_TrimmedCurve that_curv = ms.Value();
        Handle_Geom_Line that_line = Handle_Geom_Line::DownCast(that_curv->BasisCurve());
        this_line->SetLin(that_line->Lin());
        this_curv->SetTrim(that_curv->FirstParameter(), that_curv->LastParameter());
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        throw Py::Exception(e->GetMessageString());
    }
}

Py::Boolean LinePy::getInfinite(void) const
{
    return Py::Boolean(Infinite);
}

void  LinePy::setInfinite(Py::Boolean arg)
{
    Infinite = arg;
}


PyObject *LinePy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int LinePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
