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
#
#include "ShapeFix/ShapeFix_SplitCommonVertexPy.h"
#include "ShapeFix/ShapeFix_SplitCommonVertexPy.cpp"
#include "TopoShapePy.h"


using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string ShapeFix_SplitCommonVertexPy::representation() const
{
    return "<ShapeFix_SplitCommonVertex object>";
}

PyObject *ShapeFix_SplitCommonVertexPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of ShapeFix_ShapePy
    return new ShapeFix_SplitCommonVertexPy(nullptr);
}

// constructor method
int ShapeFix_SplitCommonVertexPy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
    PyObject* shape = nullptr;
    if (!PyArg_ParseTuple(args, "|O!", &TopoShapePy::Type, &shape))
        return -1;

    setHandle(new ShapeFix_SplitCommonVertex);
    if (shape) {
        getShapeFix_SplitCommonVertexPtr()->Init(static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape());
    }

    return 0;
}

PyObject* ShapeFix_SplitCommonVertexPy::init(PyObject *args)
{
    PyObject* shape;
    if (!PyArg_ParseTuple(args, "O!", &TopoShapePy::Type, &shape))
        return nullptr;

    getShapeFix_SplitCommonVertexPtr()->Init(static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape());
    Py_Return;
}

PyObject* ShapeFix_SplitCommonVertexPy::perform(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    getShapeFix_SplitCommonVertexPtr()->Perform();
    Py_Return;
}

PyObject* ShapeFix_SplitCommonVertexPy::shape(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    TopoShape shape = getShapeFix_SplitCommonVertexPtr()->Shape();
    return shape.getPyObject();
}

PyObject *ShapeFix_SplitCommonVertexPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ShapeFix_SplitCommonVertexPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
