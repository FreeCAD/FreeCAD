/***************************************************************************
 *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <gp_Pln.hxx>
# include <Standard_Failure.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
#endif

#include <Base/VectorPy.h>

#include "ChFi2d/ChFi2d_FilletAlgoPy.h"
#include "ChFi2d/ChFi2d_FilletAlgoPy.cpp"
#include "TopoShapeEdgePy.h"
#include "TopoShapeWirePy.h"
#include "PlanePy.h"
#include "Tools.h"


using namespace Part;

PyObject *ChFi2d_FilletAlgoPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of ChFi2d_FilletAlgoPy and the Twin object
    return new ChFi2d_FilletAlgoPy(new ChFi2d_FilletAlgo);
}

// constructor method
int ChFi2d_FilletAlgoPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    if (PyArg_ParseTuple(args, ""))
        return 0;

    PyErr_Clear();
    PyObject* wire;
    PyObject* plane;
    if (PyArg_ParseTuple(args, "O!O!", &TopoShapeWirePy::Type, &wire, &PlanePy::Type, &plane)) {
        TopoDS_Shape shape = static_cast<TopoShapeWirePy*>(wire)->getTopoShapePtr()->getShape();
        Handle(Geom_Plane) hPlane = Handle(Geom_Plane)::DownCast(static_cast<PlanePy*>(plane)->getGeomPlanePtr()->handle());
        getChFi2d_FilletAlgoPtr()->Init(TopoDS::Wire(shape), hPlane->Pln());
        return 0;
    }

    PyErr_Clear();
    PyObject* edge1;
    PyObject* edge2;
    if (PyArg_ParseTuple(args, "O!O!O!", &TopoShapeEdgePy::Type, &edge1,
                                         &TopoShapeEdgePy::Type, &edge2,
                                         &PlanePy::Type, &plane)) {
        TopoDS_Shape shape1 = static_cast<TopoShapeEdgePy*>(edge1)->getTopoShapePtr()->getShape();
        TopoDS_Shape shape2 = static_cast<TopoShapeEdgePy*>(edge2)->getTopoShapePtr()->getShape();
        Handle(Geom_Plane) hPlane = Handle(Geom_Plane)::DownCast(static_cast<PlanePy*>(plane)->getGeomPlanePtr()->handle());
        getChFi2d_FilletAlgoPtr()->Init(TopoDS::Edge(shape1), TopoDS::Edge(shape2), hPlane->Pln());
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "Wrong arguments:\n"
                                     "-- FilletAlgo()\n"
                                     "-- FilletAlgo(wire, plane)"
                                     "-- FilletAlgo(edge, edge, plane)\n");
    return -1;
}

// returns a string which represents the object e.g. when printed in python
std::string ChFi2d_FilletAlgoPy::representation() const
{
    return {"<FilletAlgo object>"};
}

PyObject* ChFi2d_FilletAlgoPy::init(PyObject *args)
{
    PyObject* wire;
    PyObject* plane;
    if (PyArg_ParseTuple(args, "O!O!", &TopoShapeWirePy::Type, &wire, &PlanePy::Type, &plane)) {
        TopoDS_Shape shape = static_cast<TopoShapeWirePy*>(wire)->getTopoShapePtr()->getShape();
        Handle(Geom_Plane) hPlane = Handle(Geom_Plane)::DownCast(static_cast<PlanePy*>(plane)->getGeomPlanePtr()->handle());
        getChFi2d_FilletAlgoPtr()->Init(TopoDS::Wire(shape), hPlane->Pln());
        Py_Return;
    }

    PyErr_Clear();
    PyObject* edge1;
    PyObject* edge2;
    if (PyArg_ParseTuple(args, "O!O!O!", &TopoShapeEdgePy::Type, &edge1,
                                         &TopoShapeEdgePy::Type, &edge2,
                                         &PlanePy::Type, &plane)) {
        TopoDS_Shape shape1 = static_cast<TopoShapeEdgePy*>(edge1)->getTopoShapePtr()->getShape();
        TopoDS_Shape shape2 = static_cast<TopoShapeEdgePy*>(edge2)->getTopoShapePtr()->getShape();
        Handle(Geom_Plane) hPlane = Handle(Geom_Plane)::DownCast(static_cast<PlanePy*>(plane)->getGeomPlanePtr()->handle());
        getChFi2d_FilletAlgoPtr()->Init(TopoDS::Edge(shape1), TopoDS::Edge(shape2), hPlane->Pln());
        Py_Return;
    }

    PyErr_SetString(PyExc_TypeError, "Wrong arguments:\n"
                                     "-- init(wire, plane)"
                                     "-- init(edge, edge, plane)\n");
    return nullptr;
}

PyObject* ChFi2d_FilletAlgoPy::perform(PyObject *args)
{
    double radius;
    if (!PyArg_ParseTuple(args, "d", &radius))
        return nullptr;

    try {
        bool ok = getChFi2d_FilletAlgoPtr()->Perform(radius);
        return Py::new_reference_to(Py::Boolean(ok));
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(Base::PyExc_FC_CADKernelError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* ChFi2d_FilletAlgoPy::numberOfResults(PyObject *args)
{
    PyObject* pnt;
    if (!PyArg_ParseTuple(args, "O!", &Base::VectorPy::Type, &pnt))
        return nullptr;

    try {
        Base::Vector3d* vec = static_cast<Base::VectorPy*>(pnt)->getVectorPtr();
        Standard_Integer num = getChFi2d_FilletAlgoPtr()->NbResults(gp_Pnt(vec->x, vec->y, vec->z));
        return Py::new_reference_to(Py::Long(num));
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(Base::PyExc_FC_CADKernelError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* ChFi2d_FilletAlgoPy::result(PyObject *args)
{
    PyObject* pnt;
    int solution = -1;
    if (!PyArg_ParseTuple(args, "O!|i", &Base::VectorPy::Type, &pnt, &solution))
        return nullptr;

    Base::Vector3d* vec = static_cast<Base::VectorPy*>(pnt)->getVectorPtr();

    try {
        TopoDS_Edge theEdge1, theEdge2;
        TopoDS_Shape res_edge = getChFi2d_FilletAlgoPtr()->Result(Base::convertTo<gp_Pnt>(*vec), theEdge1, theEdge2, solution);

        Py::TupleN tuple(Py::asObject(TopoShape(res_edge).getPyObject()),
                         Py::asObject(TopoShape(theEdge1).getPyObject()),
                         Py::asObject(TopoShape(theEdge2).getPyObject()));
        return Py::new_reference_to(tuple);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(Base::PyExc_FC_CADKernelError, e.GetMessageString());
        return nullptr;
    }
}

PyObject *ChFi2d_FilletAlgoPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ChFi2d_FilletAlgoPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
