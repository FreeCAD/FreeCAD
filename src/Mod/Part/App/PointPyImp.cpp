/***************************************************************************
 *   Copyright (c) 2012 Konstantinos Poulios <logari81[at]gmail.com>       *
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
# include <BRepBuilderAPI_MakeVertex.hxx>
# include <GC_MakeLine.hxx>
# include <Geom_CartesianPoint.hxx>
# include <TopoDS_Vertex.hxx>
#endif

#include <Base/VectorPy.h>

#include "PointPy.h"
#include "PointPy.cpp"
#include "OCCError.h"
#include "TopoShapeVertexPy.h"


using namespace Part;

extern const char* gce_ErrorStatusText(gce_ErrorType et);

// returns a string which represents the object e.g. when printed in python
std::string PointPy::representation() const
{
    std::stringstream str;
    Base::Vector3d coords = getGeomPointPtr()->getPoint();
    str << "<Point (" << coords.x << "," << coords.y << "," << coords.z << ") >";
    return str.str();
}

PyObject *PointPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of PointPy and the Twin object
    return new PointPy(new GeomPoint);
}

// constructor method
int PointPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{

    if (PyArg_ParseTuple(args, "")) {
        // default point
        return 0;
    }

    PyErr_Clear();
    PyObject *pPoint;
    if (PyArg_ParseTuple(args, "O!", &(PointPy::Type), &pPoint)) {
        // Copy point
        PointPy* pcPoint = static_cast<PointPy*>(pPoint);
        // get Geom_CartesianPoint of that point
        Handle(Geom_CartesianPoint) that_point = Handle(Geom_CartesianPoint)::DownCast
            (pcPoint->getGeomPointPtr()->handle());
        // get Geom_CartesianPoint of this point
        Handle(Geom_CartesianPoint) this_point = Handle(Geom_CartesianPoint)::DownCast
            (this->getGeomPointPtr()->handle());

        // Assign the coordinates
        this_point->SetPnt(that_point->Pnt());
        return 0;
    }

    PyErr_Clear();
    PyObject *pV;
    if (PyArg_ParseTuple(args, "O!", &(Base::VectorPy::Type), &pV)) {
        Base::Vector3d v = static_cast<Base::VectorPy*>(pV)->value();
        Handle(Geom_CartesianPoint) this_point = Handle(Geom_CartesianPoint)::DownCast
            (this->getGeomPointPtr()->handle());
        this_point->SetCoord(v.x,v.y,v.z);
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "Point constructor accepts:\n"
        "-- empty parameter list\n"
        "-- Point\n"
        "-- Coordinates vector");
    return -1;
}

PyObject* PointPy::toShape(PyObject *args)
{
    Handle(Geom_CartesianPoint) this_point = Handle(Geom_CartesianPoint)::DownCast
        (this->getGeomPointPtr()->handle());
    try {
        if (!this_point.IsNull()) {
            if (!PyArg_ParseTuple(args, ""))
                return nullptr;

            BRepBuilderAPI_MakeVertex mkBuilder(this_point->Pnt());
            const TopoDS_Vertex& sh = mkBuilder.Vertex();
            return new TopoShapeVertexPy(new TopoShape(sh));
        }
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    PyErr_SetString(PartExceptionOCCError, "Geometry is not a point");
    return nullptr;
}

Py::Float PointPy::getX() const
{
    Handle(Geom_CartesianPoint) this_point = Handle(Geom_CartesianPoint)::DownCast
        (this->getGeomPointPtr()->handle());
    return Py::Float(this_point->X());
}

void PointPy::setX(Py::Float X)
{
    Handle(Geom_CartesianPoint) this_point = Handle(Geom_CartesianPoint)::DownCast
        (this->getGeomPointPtr()->handle());

    try {
        this_point->SetX(double(X));
    }
    catch (Standard_Failure& e) {
        throw Py::RuntimeError(e.GetMessageString());
    }
}

Py::Float PointPy::getY() const
{
    Handle(Geom_CartesianPoint) this_point = Handle(Geom_CartesianPoint)::DownCast
        (this->getGeomPointPtr()->handle());
    return Py::Float(this_point->Y());
}

void PointPy::setY(Py::Float Y)
{
    Handle(Geom_CartesianPoint) this_point = Handle(Geom_CartesianPoint)::DownCast
        (this->getGeomPointPtr()->handle());

    try {
        this_point->SetY(double(Y));
    }
    catch (Standard_Failure& e) {
        throw Py::RuntimeError(e.GetMessageString());
    }
}

Py::Float PointPy::getZ() const
{
    Handle(Geom_CartesianPoint) this_point = Handle(Geom_CartesianPoint)::DownCast
        (this->getGeomPointPtr()->handle());
    return Py::Float(this_point->Z());
}

void PointPy::setZ(Py::Float Z)
{
    Handle(Geom_CartesianPoint) this_point = Handle(Geom_CartesianPoint)::DownCast
        (this->getGeomPointPtr()->handle());

    try {
        this_point->SetZ(double(Z));
    }
    catch (Standard_Failure& e) {
        throw Py::RuntimeError(e.GetMessageString());
    }
}


PyObject *PointPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int PointPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
