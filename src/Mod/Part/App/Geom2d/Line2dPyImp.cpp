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
# include <GCE2d_MakeLine.hxx>
# include <Geom2d_Line.hxx>
# include <gp_Lin2d.hxx>
#endif

#include <Base/GeometryPyCXX.h>

#include "Geom2d/Line2dPy.h"
#include "Geom2d/Line2dPy.cpp"
#include "OCCError.h"


using namespace Part;

extern const char* gce_ErrorStatusText(gce_ErrorType et);

// returns a string which represents the object e.g. when printed in python
std::string Line2dPy::representation() const
{
    return "<Line2d object>";
}

PyObject *Line2dPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of Line2dPy and the Twin object
    return new Line2dPy(new Geom2dLine);
}

// constructor method
int Line2dPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    if (PyArg_ParseTuple(args, "")) {
        // default line
        return 0;
    }

    PyErr_Clear();
    PyObject *pLine;
    if (PyArg_ParseTuple(args, "O!", &(Line2dPy::Type), &pLine)) {
        // Copy line
        Line2dPy* pcLine = static_cast<Line2dPy*>(pLine);
        // get Geom_Line of line
        Handle(Geom2d_Line) that_line = Handle(Geom2d_Line)::DownCast
            (pcLine->getGeom2dLinePtr()->handle());
        // get Geom_Line of line
        Handle(Geom2d_Line) this_line = Handle(Geom2d_Line)::DownCast
            (this->getGeom2dLinePtr()->handle());

        // Assign the lines
        this_line->SetLin2d(that_line->Lin2d());
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
            GCE2d_MakeLine ms(gp_Pnt2d(v1.x,v1.y),
                              gp_Pnt2d(v2.x,v2.y));
            if (!ms.IsDone()) {
                PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(ms.Status()));
                return -1;
            }

            // get Geom_Line of line
            Handle(Geom2d_Line) this_line = Handle(Geom2d_Line)::DownCast
                (this->getGeom2dLinePtr()->handle());
            Handle(Geom2d_Line) that_line = ms.Value();
            this_line->SetLin2d(that_line->Lin2d());
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
        "-- Line\n"
        "-- Point, Point");
    return -1;
}

Py::Object Line2dPy::getLocation() const
{
    Handle(Geom2d_Line) this_curve = Handle(Geom2d_Line)::DownCast
        (this->getGeom2dLinePtr()->handle());
    gp_Pnt2d pnt = this_curve->Location();
    return Base::Vector2dPy::create(pnt.X(), pnt.Y());
}

void Line2dPy::setLocation(Py::Object arg)
{
    gp_Pnt2d pnt;
    gp_Dir2d dir;
    Handle(Geom2d_Line) this_line = Handle(Geom2d_Line)::DownCast
        (this->getGeom2dLinePtr()->handle());
    dir = this_line->Direction();

    PyObject *p = arg.ptr();
    if (PyObject_TypeCheck(p, Base::Vector2dPy::type_object())) {
        Base::Vector2d v = Py::toVector2d(p);
        pnt.SetX(v.x);
        pnt.SetY(v.y);
    }
    else if (PyTuple_Check(p)) {
        Py::Tuple tuple(arg);
        pnt.SetX((double)Py::Float(tuple.getItem(0)));
        pnt.SetY((double)Py::Float(tuple.getItem(1)));
    }
    else {
        std::string error = std::string("type must be 'Vector2d' or tuple, not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }

    try {
        GCE2d_MakeLine ms(pnt, dir);
        if (!ms.IsDone()) {
            throw Py::RuntimeError(gce_ErrorStatusText(ms.Status()));
        }

        // get Geom_Line of line
        Handle(Geom2d_Line) that_line = ms.Value();
        this_line->SetLin2d(that_line->Lin2d());
    }
    catch (Standard_Failure& e) {
        throw Py::RuntimeError(e.GetMessageString());
    }
}

Py::Object Line2dPy::getDirection() const
{
    Handle(Geom2d_Line) this_curve = Handle(Geom2d_Line)::DownCast
        (this->getGeom2dLinePtr()->handle());
    gp_Dir2d dir = this_curve->Direction();
    return Base::Vector2dPy::create(dir.X(), dir.Y());
}

void Line2dPy::setDirection(Py::Object arg)
{
    gp_Pnt2d pnt;
    gp_Dir2d dir;
    Handle(Geom2d_Line) this_line = Handle(Geom2d_Line)::DownCast
        (this->getGeom2dLinePtr()->handle());
    pnt = this_line->Location();

    PyObject *p = arg.ptr();
    if (PyObject_TypeCheck(p, Base::Vector2dPy::type_object())) {
        Base::Vector2d v = Py::toVector2d(p);
        dir = gp_Dir2d(v.x,v.y);
    }
    else if (PyTuple_Check(p)) {
        Py::Tuple tuple(arg);
        double x = (double)Py::Float(tuple.getItem(0));
        double y = (double)Py::Float(tuple.getItem(1));
        dir = gp_Dir2d(x,y);
    }
    else {
        std::string error = std::string("type must be 'Vector2d' or tuple, not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }

    try {
        GCE2d_MakeLine ms(pnt, dir);
        if (!ms.IsDone()) {
            throw Py::RuntimeError(gce_ErrorStatusText(ms.Status()));
        }

        // get Geom_Line of line
        Handle(Geom2d_Line) that_line = ms.Value();
        this_line->SetLin2d(that_line->Lin2d());
    }
    catch (Standard_Failure& e) {
        throw Py::RuntimeError(e.GetMessageString());
    }
}

PyObject *Line2dPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int Line2dPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
