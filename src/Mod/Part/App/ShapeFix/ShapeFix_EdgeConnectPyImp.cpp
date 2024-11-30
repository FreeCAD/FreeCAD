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
#endif

#include "ShapeFix/ShapeFix_EdgeConnectPy.h"
#include "ShapeFix/ShapeFix_EdgeConnectPy.cpp"
#include "TopoShapeEdgePy.h"


using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string ShapeFix_EdgeConnectPy::representation() const
{
    return "<ShapeFix_EdgeConnect object>";
}

PyObject *ShapeFix_EdgeConnectPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of ShapeFix_RootPy
    return new ShapeFix_EdgeConnectPy(new ShapeFix_EdgeConnect);
}

// constructor method
int ShapeFix_EdgeConnectPy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
    if (!PyArg_ParseTuple(args, ""))
        return -1;
    return 0;
}

PyObject* ShapeFix_EdgeConnectPy::add(PyObject *args)
{
    PyObject* edge1;
    PyObject* edge2;
    if (PyArg_ParseTuple(args, "O!O!", &TopoShapeEdgePy::Type, &edge1,
                                       &TopoShapeEdgePy::Type, &edge2)) {
        TopoDS_Shape e1 = static_cast<TopoShapeEdgePy*>(edge1)->getTopoShapePtr()->getShape();
        TopoDS_Shape e2 = static_cast<TopoShapeEdgePy*>(edge2)->getTopoShapePtr()->getShape();
        getShapeFix_EdgeConnectPtr()->Add(TopoDS::Edge(e1), TopoDS::Edge(e2));
        Py_Return;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!", &TopoShapePy::Type, &edge1)) {
        TopoDS_Shape shape = static_cast<TopoShapePy*>(edge1)->getTopoShapePtr()->getShape();
        getShapeFix_EdgeConnectPtr()->Add(shape);
        Py_Return;
    }

    PyErr_SetString(PyExc_TypeError, "add(edge, edge) or\n"
                                     "add(shape)");
    return nullptr;
}

PyObject* ShapeFix_EdgeConnectPy::build(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    getShapeFix_EdgeConnectPtr()->Build();
    Py_Return;
}

PyObject* ShapeFix_EdgeConnectPy::clear(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    getShapeFix_EdgeConnectPtr()->Clear();
    Py_Return;
}

PyObject *ShapeFix_EdgeConnectPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ShapeFix_EdgeConnectPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
