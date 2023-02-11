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
# include <Geom_Line.hxx>
# include <GC_MakeLine.hxx>
#endif

#include <Base/GeometryPyCXX.h>
#include <Base/VectorPy.h>

#include "LinePy.h"
#include "LinePy.cpp"
#include "OCCError.h"


using namespace Part;

extern const char* gce_ErrorStatusText(gce_ErrorType et);

// returns a string which represents the object e.g. when printed in python
std::string LinePy::representation() const
{
    return "<Line object>";
}

PyObject *LinePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of LinePy and the Twin object
    return new LinePy(new GeomLine);
}

// constructor method
int LinePy::PyInit(PyObject* args, PyObject* /*kwd*/)
{

    if (PyArg_ParseTuple(args, "")) {
        // default line
        return 0;
    }

    PyErr_Clear();
    PyObject *pLine;
    if (PyArg_ParseTuple(args, "O!", &(LinePy::Type), &pLine)) {
        // Copy line
        LinePy* pcLine = static_cast<LinePy*>(pLine);
        // get Geom_Line of line
        Handle(Geom_Line) that_line = Handle(Geom_Line)::DownCast
            (pcLine->getGeomLinePtr()->handle());
        // get Geom_Line of line
        Handle(Geom_Line) this_line = Handle(Geom_Line)::DownCast
            (this->getGeomLinePtr()->handle());

        // Assign the lines
        this_line->SetLin(that_line->Lin());
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
            GC_MakeLine ms(gp_Pnt(v1.x,v1.y,v1.z),
                           gp_Pnt(v2.x,v2.y,v2.z));
            if (!ms.IsDone()) {
                PyErr_SetString(PartExceptionOCCError, gce_ErrorStatusText(ms.Status()));
                return -1;
            }

            // get Geom_Line of line
            Handle(Geom_Line) this_curv = Handle(Geom_Line)::DownCast
                (this->getGeomLinePtr()->handle());
            Handle(Geom_Line) that_curv = ms.Value();
            this_curv->SetLin(that_curv->Lin());
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

Py::Object LinePy::getLocation() const
{
    Handle(Geom_Line) this_curve = Handle(Geom_Line)::DownCast
        (this->getGeomLinePtr()->handle());
    gp_Pnt pnt = this_curve->Position().Location();
    return Py::Vector(Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z()));
}

void LinePy::setLocation(Py::Object arg)
{
    gp_Pnt pnt;
    gp_Dir dir;
    Handle(Geom_Line) this_curv = Handle(Geom_Line)::DownCast
        (this->getGeomLinePtr()->handle());
    dir = this_curv->Position().Direction();

    PyObject *p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        Base::Vector3d v = static_cast<Base::VectorPy*>(p)->value();
        pnt.SetX(v.x);
        pnt.SetY(v.y);
        pnt.SetZ(v.z);
    }
    else if (PyTuple_Check(p)) {
        Py::Tuple tuple(arg);
        pnt.SetX((double)Py::Float(tuple.getItem(0)));
        pnt.SetY((double)Py::Float(tuple.getItem(1)));
        pnt.SetZ((double)Py::Float(tuple.getItem(2)));
    }
    else {
        std::string error = std::string("type must be 'Vector' or tuple, not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }

    try {
        GC_MakeLine ms(pnt, dir);
        if (!ms.IsDone()) {
            throw Py::RuntimeError(gce_ErrorStatusText(ms.Status()));
        }

        // get Geom_Line of line
        Handle(Geom_Line) that_curv = ms.Value();
        this_curv->SetLin(that_curv->Lin());
    }
    catch (Standard_Failure& e) {
        throw Py::RuntimeError(e.GetMessageString());
    }
}

Py::Object LinePy::getDirection() const
{
    Handle(Geom_Line) this_curve = Handle(Geom_Line)::DownCast
        (this->getGeomLinePtr()->handle());
    gp_Dir dir = this_curve->Position().Direction();
    return Py::Vector(Base::Vector3d(dir.X(), dir.Y(), dir.Z()));
}

void LinePy::setDirection(Py::Object arg)
{
    gp_Pnt pnt;
    gp_Dir dir;
    Handle(Geom_Line) this_curv = Handle(Geom_Line)::DownCast
        (this->getGeomLinePtr()->handle());
    pnt = this_curv->Position().Location();

    PyObject *p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Base::VectorPy::Type))) {
        Base::Vector3d v = static_cast<Base::VectorPy*>(p)->value();
        dir = gp_Dir(v.x,v.y,v.z);
    }
    else if (PyTuple_Check(p)) {
        Py::Tuple tuple(arg);
        double x = (double)Py::Float(tuple.getItem(0));
        double y = (double)Py::Float(tuple.getItem(1));
        double z = (double)Py::Float(tuple.getItem(2));
        dir = gp_Dir(x,y,z);
    }
    else {
        std::string error = std::string("type must be 'Vector' or tuple, not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }

    try {
        GC_MakeLine ms(pnt, dir);
        if (!ms.IsDone()) {
            throw Py::RuntimeError(gce_ErrorStatusText(ms.Status()));
        }

        // get Geom_Line of line
        Handle(Geom_Line) that_curv = ms.Value();
        this_curv->SetLin(that_curv->Lin());
    }
    catch (Standard_Failure& e) {
        throw Py::RuntimeError(e.GetMessageString());
    }
}

PyObject *LinePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int LinePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
