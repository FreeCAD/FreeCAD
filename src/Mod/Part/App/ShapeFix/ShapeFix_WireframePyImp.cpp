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

#include "ShapeFix/ShapeFix_WireframePy.h"
#include "ShapeFix/ShapeFix_WireframePy.cpp"
#include "TopoShapePy.h"


using namespace Part;

std::string ShapeFix_WireframePy::representation() const
{
    return "<ShapeFix_Wireframe object>";
}

PyObject *ShapeFix_WireframePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    return new ShapeFix_WireframePy(nullptr);
}

// constructor method
int ShapeFix_WireframePy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
    PyObject* shape = nullptr;
    if (!PyArg_ParseTuple(args, "|O!", &TopoShapePy::Type, &shape))
        return -1;

    if (shape) {
        setHandle(new ShapeFix_Wireframe(static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape()));
    }
    else {
        setHandle(new ShapeFix_Wireframe());
    }

    return 0;
}

PyObject* ShapeFix_WireframePy::load(PyObject *args)
{
    PyObject* shape;
    if (!PyArg_ParseTuple(args, "O!", &TopoShapePy::Type, &shape))
        return nullptr;

    getShapeFix_WireframePtr()->Load(static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape());
    Py_Return;
}

PyObject* ShapeFix_WireframePy::clearStatuses(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    getShapeFix_WireframePtr()->ClearStatuses();
    Py_Return;
}

PyObject* ShapeFix_WireframePy::fixWireGaps(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    bool ok = getShapeFix_WireframePtr()->FixWireGaps();
    return Py::new_reference_to(Py::Boolean(ok));
}

PyObject* ShapeFix_WireframePy::fixSmallEdges(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    bool ok = getShapeFix_WireframePtr()->FixSmallEdges();
    return Py::new_reference_to(Py::Boolean(ok));
}

PyObject* ShapeFix_WireframePy::shape(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    TopoShape sh = getShapeFix_WireframePtr()->Shape();
    return sh.getPyObject();
}

Py::Boolean ShapeFix_WireframePy::getModeDropSmallEdges() const
{
    return Py::Boolean(getShapeFix_WireframePtr()->ModeDropSmallEdges());
}

void ShapeFix_WireframePy::setModeDropSmallEdges(Py::Boolean arg)
{
    getShapeFix_WireframePtr()->ModeDropSmallEdges() = arg;
}

Py::Float ShapeFix_WireframePy::getLimitAngle() const
{
    return Py::Float(getShapeFix_WireframePtr()->LimitAngle());
}

void ShapeFix_WireframePy::setLimitAngle(Py::Float arg)
{
    getShapeFix_WireframePtr()->SetLimitAngle(arg);
}

PyObject *ShapeFix_WireframePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ShapeFix_WireframePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
