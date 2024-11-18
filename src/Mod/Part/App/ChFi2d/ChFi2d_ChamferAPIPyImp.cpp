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
# include <Standard_Failure.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
#endif

#include "ChFi2d/ChFi2d_ChamferAPIPy.h"
#include "ChFi2d/ChFi2d_ChamferAPIPy.cpp"
#include "TopoShapeEdgePy.h"
#include "TopoShapeWirePy.h"


using namespace Part;

PyObject *ChFi2d_ChamferAPIPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of ChFi2d_ChamferAPIPy and the Twin object
    return new ChFi2d_ChamferAPIPy(new ChFi2d_ChamferAPI);
}

// constructor method
int ChFi2d_ChamferAPIPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    // Note: Although the C++ class has a default constructor it would cause a crash
    // if the algorithm is not initialized with a wire/edges.
    // Thus, in the Python API it is disabled.
    PyObject* wire;
    if (PyArg_ParseTuple(args, "O!", &TopoShapeWirePy::Type, &wire)) {
        TopoDS_Shape shape = static_cast<TopoShapeWirePy*>(wire)->getTopoShapePtr()->getShape();
        getChFi2d_ChamferAPIPtr()->Init(TopoDS::Wire(shape));
        return 0;
    }

    PyErr_Clear();
    PyObject* edge1;
    PyObject* edge2;
    if (PyArg_ParseTuple(args, "O!O!", &TopoShapeEdgePy::Type, &edge1,
                                       &TopoShapeEdgePy::Type, &edge2)) {
        TopoDS_Shape shape1 = static_cast<TopoShapeEdgePy*>(edge1)->getTopoShapePtr()->getShape();
        TopoDS_Shape shape2 = static_cast<TopoShapeEdgePy*>(edge2)->getTopoShapePtr()->getShape();
        getChFi2d_ChamferAPIPtr()->Init(TopoDS::Edge(shape1), TopoDS::Edge(shape2));
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "Wrong arguments:\n"
                                     "-- ChamferAPI()\n"
                                     "-- ChamferAPI(wire)"
                                     "-- ChamferAPI(edge, edge)\n");
    return -1;
}

// returns a string which represents the object e.g. when printed in python
std::string ChFi2d_ChamferAPIPy::representation() const
{
    return {"<ChamferAPI object>"};
}

PyObject* ChFi2d_ChamferAPIPy::init(PyObject *args)
{
    PyObject* wire;
    if (PyArg_ParseTuple(args, "O!", &TopoShapeWirePy::Type, &wire)) {
        TopoDS_Shape shape = static_cast<TopoShapeWirePy*>(wire)->getTopoShapePtr()->getShape();
        getChFi2d_ChamferAPIPtr()->Init(TopoDS::Wire(shape));
        Py_Return;
    }

    PyErr_Clear();
    PyObject* edge1;
    PyObject* edge2;
    if (PyArg_ParseTuple(args, "O!O!", &TopoShapeEdgePy::Type, &edge1,
                                       &TopoShapeEdgePy::Type, &edge2)) {
        TopoDS_Shape shape1 = static_cast<TopoShapeEdgePy*>(edge1)->getTopoShapePtr()->getShape();
        TopoDS_Shape shape2 = static_cast<TopoShapeEdgePy*>(edge2)->getTopoShapePtr()->getShape();
        getChFi2d_ChamferAPIPtr()->Init(TopoDS::Edge(shape1), TopoDS::Edge(shape2));
        Py_Return;
    }

    PyErr_SetString(PyExc_TypeError, "Wrong arguments:\n"
                                     "-- init(wire)"
                                     "-- init(edge, edge)\n");
    return nullptr;
}

PyObject* ChFi2d_ChamferAPIPy::perform(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        bool ok = getChFi2d_ChamferAPIPtr()->Perform();
        return Py::new_reference_to(Py::Boolean(ok));
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(Base::PyExc_FC_CADKernelError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* ChFi2d_ChamferAPIPy::result(PyObject *args)
{
    double length1, length2;
    if (!PyArg_ParseTuple(args, "dd", &length1, &length2))
        return nullptr;

    try {
        TopoDS_Edge theEdge1, theEdge2;
        TopoDS_Shape res_edge = getChFi2d_ChamferAPIPtr()->Result(theEdge1, theEdge2, length1, length2);

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

PyObject *ChFi2d_ChamferAPIPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ChFi2d_ChamferAPIPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
