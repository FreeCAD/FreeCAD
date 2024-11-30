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
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
#endif

#include "ShapeFix/ShapeFix_SplitToolPy.h"
#include "ShapeFix/ShapeFix_SplitToolPy.cpp"
#include "TopoShapeEdgePy.h"
#include "TopoShapeFacePy.h"
#include "TopoShapeVertexPy.h"


using namespace Part;


std::string ShapeFix_SplitToolPy::representation() const
{
    return "<ShapeFix_SplitTool object>";
}

PyObject *ShapeFix_SplitToolPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    return new ShapeFix_SplitToolPy(new ShapeFix_SplitTool);
}

// constructor method
int ShapeFix_SplitToolPy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
    if (PyArg_ParseTuple(args, ""))
        return -1;
    return 0;
}

PyObject* ShapeFix_SplitToolPy::splitEdge(PyObject *args)
{
    TopoDS_Edge e1, e2;
    bool ok = false;

    do {
        PyObject* edge;
        PyObject* vert;
        PyObject* face;
        double param1, param2;
        double tol3d;
        double tol2d;

        if (PyArg_ParseTuple(args, "O!dO!O!dd", &TopoShapeEdgePy::Type, &edge, &param1,
                                                &TopoShapeVertexPy::Type, &vert,
                                                &TopoShapeFacePy::Type, &face,
                                                &tol3d, &tol2d)) {
            TopoDS_Shape e = static_cast<TopoShapePy*>(edge)->getTopoShapePtr()->getShape();
            TopoDS_Shape v = static_cast<TopoShapePy*>(vert)->getTopoShapePtr()->getShape();
            TopoDS_Shape f = static_cast<TopoShapePy*>(face)->getTopoShapePtr()->getShape();

            ok = getShapeFix_SplitToolPtr()->SplitEdge(TopoDS::Edge(e), param1, TopoDS::Vertex(v), TopoDS::Face(f), e1, e2, tol3d, tol2d);
            break;
        }

        PyErr_Clear();
        if (PyArg_ParseTuple(args, "O!ddO!O!dd", &TopoShapeEdgePy::Type, &edge,
                                                 &param1, &param2,
                                                 &TopoShapeVertexPy::Type, &vert,
                                                 &TopoShapeFacePy::Type, &face,
                                                 &tol3d, &tol2d)) {
            TopoDS_Shape e = static_cast<TopoShapePy*>(edge)->getTopoShapePtr()->getShape();
            TopoDS_Shape v = static_cast<TopoShapePy*>(vert)->getTopoShapePtr()->getShape();
            TopoDS_Shape f = static_cast<TopoShapePy*>(face)->getTopoShapePtr()->getShape();

            ok = getShapeFix_SplitToolPtr()->SplitEdge(TopoDS::Edge(e), param1, param2, TopoDS::Vertex(v), TopoDS::Face(f), e1, e2, tol3d, tol2d);
            break;
        }

        PyErr_SetString(PyExc_TypeError, "splitEdge(edge, param, vertex, face, tol3d, tol2d)\n"
                                         "splitEdge(edge, param1, param2, vertex, face, tol3d, tol2d)");
        return nullptr;
    }
    while(false);

    Py::Tuple tuple(2);
    if (ok && !e1.IsNull() && !e2.IsNull()) {
        tuple.setItem(0, Py::asObject(TopoShape(e1).getPyObject()));
        tuple.setItem(1, Py::asObject(TopoShape(e2).getPyObject()));
    }
    return Py::new_reference_to(tuple);

}

PyObject* ShapeFix_SplitToolPy::cutEdge(PyObject *args)
{
    PyObject* edge;
    PyObject* face;
    double pend;
    double cut;
    if (!PyArg_ParseTuple(args, "O!ddO!", &TopoShapeEdgePy::Type, &edge, &pend, &cut,
                                          &TopoShapeFacePy::Type, &face))
        return nullptr;

    TopoDS_Shape e = static_cast<TopoShapePy*>(edge)->getTopoShapePtr()->getShape();
    TopoDS_Shape f = static_cast<TopoShapePy*>(face)->getTopoShapePtr()->getShape();
    Standard_Boolean iscutline;
    bool ok = getShapeFix_SplitToolPtr()->CutEdge(TopoDS::Edge(e), pend, cut, TopoDS::Face(f), iscutline);
    return Py::new_reference_to(Py::Boolean(ok));
}

PyObject *ShapeFix_SplitToolPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ShapeFix_SplitToolPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
