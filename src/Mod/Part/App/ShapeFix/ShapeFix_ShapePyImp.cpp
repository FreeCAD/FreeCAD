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

#include "ShapeFix/ShapeFix_ShapePy.h"
#include "ShapeFix/ShapeFix_ShapePy.cpp"
#include "ShapeFix/ShapeFix_EdgePy.h"
#include "ShapeFix/ShapeFix_FacePy.h"
#include "ShapeFix/ShapeFix_ShellPy.h"
#include "ShapeFix/ShapeFix_SolidPy.h"
#include "ShapeFix/ShapeFix_WirePy.h"
#include "TopoShapePy.h"


using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string ShapeFix_ShapePy::representation() const
{
    return "<ShapeFix_Shape object>";
}

PyObject *ShapeFix_ShapePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of ShapeFix_ShapePy
    return new ShapeFix_ShapePy(nullptr);
}

// constructor method
int ShapeFix_ShapePy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
    PyObject* shape = nullptr;
    if (!PyArg_ParseTuple(args, "|O!", &TopoShapePy::Type, &shape))
        return -1;

    setHandle(new ShapeFix_Shape);
    if (shape) {
        getShapeFix_ShapePtr()->Init(static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape());
    }

    return 0;
}

PyObject* ShapeFix_ShapePy::init(PyObject *args)
{
    PyObject* shape;
    if (!PyArg_ParseTuple(args, "O!", &TopoShapePy::Type, &shape))
        return nullptr;

    getShapeFix_ShapePtr()->Init(static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape());
    Py_Return;
}

PyObject* ShapeFix_ShapePy::perform(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Standard_Boolean ok = getShapeFix_ShapePtr()->Perform();
    return Py::new_reference_to(Py::Boolean(ok ? true : false));
}

PyObject* ShapeFix_ShapePy::shape(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    TopoShape shape = getShapeFix_ShapePtr()->Shape();
    return shape.getPyObject();
}

PyObject* ShapeFix_ShapePy::fixSolidTool(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Handle(ShapeFix_Solid) tool = getShapeFix_ShapePtr()->FixSolidTool();
    ShapeFix_SolidPy* solid = new ShapeFix_SolidPy(nullptr);
    solid->setHandle(tool);
    return solid;
}

PyObject* ShapeFix_ShapePy::fixShellTool(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Handle(ShapeFix_Shell) tool = getShapeFix_ShapePtr()->FixShellTool();
    ShapeFix_ShellPy* shell = new ShapeFix_ShellPy(nullptr);
    shell->setHandle(tool);
    return shell;
}

PyObject* ShapeFix_ShapePy::fixFaceTool(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Handle(ShapeFix_Face) tool = getShapeFix_ShapePtr()->FixFaceTool();
    ShapeFix_FacePy* face = new ShapeFix_FacePy(nullptr);
    face->setHandle(tool);
    return face;
}

PyObject* ShapeFix_ShapePy::fixWireTool(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Handle(ShapeFix_Wire) tool = getShapeFix_ShapePtr()->FixWireTool();
    ShapeFix_WirePy* wire = new ShapeFix_WirePy(nullptr);
    wire->setHandle(tool);
    return wire;
}

PyObject* ShapeFix_ShapePy::fixEdgeTool(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Handle(ShapeFix_Edge) tool = getShapeFix_ShapePtr()->FixEdgeTool();
    ShapeFix_EdgePy* edge = new ShapeFix_EdgePy(nullptr);
    edge->setHandle(tool);
    return edge;
}

Py::Boolean ShapeFix_ShapePy::getFixSolidMode() const
{
    return Py::Boolean(getShapeFix_ShapePtr()->FixSolidMode());
}

void ShapeFix_ShapePy::setFixSolidMode(Py::Boolean arg)
{
    getShapeFix_ShapePtr()->FixSolidMode() = arg;
}

Py::Boolean ShapeFix_ShapePy::getFixFreeShellMode() const
{
    return Py::Boolean(getShapeFix_ShapePtr()->FixFreeShellMode());
}

void ShapeFix_ShapePy::setFixFreeShellMode(Py::Boolean arg)
{
    getShapeFix_ShapePtr()->FixFreeShellMode() = arg;
}

Py::Boolean ShapeFix_ShapePy::getFixFreeFaceMode() const
{
    return Py::Boolean(getShapeFix_ShapePtr()->FixFreeFaceMode());
}

void ShapeFix_ShapePy::setFixFreeFaceMode(Py::Boolean arg)
{
    getShapeFix_ShapePtr()->FixFreeFaceMode() = arg;
}

Py::Boolean ShapeFix_ShapePy::getFixFreeWireMode() const
{
    return Py::Boolean(getShapeFix_ShapePtr()->FixFreeWireMode());
}

void ShapeFix_ShapePy::setFixFreeWireMode(Py::Boolean arg)
{
    getShapeFix_ShapePtr()->FixFreeWireMode() = arg;
}

Py::Boolean ShapeFix_ShapePy::getFixSameParameterMode() const
{
    return Py::Boolean(getShapeFix_ShapePtr()->FixSameParameterMode());
}

void ShapeFix_ShapePy::setFixSameParameterMode(Py::Boolean arg)
{
    getShapeFix_ShapePtr()->FixSameParameterMode() = arg;
}

Py::Boolean ShapeFix_ShapePy::getFixVertexPositionMode() const
{
    return Py::Boolean(getShapeFix_ShapePtr()->FixVertexPositionMode());
}

void ShapeFix_ShapePy::setFixVertexPositionMode(Py::Boolean arg)
{
    getShapeFix_ShapePtr()->FixVertexPositionMode() = arg;
}

Py::Boolean ShapeFix_ShapePy::getFixVertexTolMode() const
{
    return Py::Boolean(getShapeFix_ShapePtr()->FixVertexTolMode());
}

void ShapeFix_ShapePy::setFixVertexTolMode(Py::Boolean arg)
{
    getShapeFix_ShapePtr()->FixVertexTolMode() = arg;
}

PyObject *ShapeFix_ShapePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ShapeFix_ShapePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
