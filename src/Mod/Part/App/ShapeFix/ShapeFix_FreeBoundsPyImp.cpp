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

#include "ShapeFix/ShapeFix_FreeBoundsPy.h"
#include "ShapeFix/ShapeFix_FreeBoundsPy.cpp"
#include "TopoShapePy.h"


using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string ShapeFix_FreeBoundsPy::representation() const
{
    return "<ShapeFix_FreeBounds object>";
}

PyObject *ShapeFix_FreeBoundsPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    return new ShapeFix_FreeBoundsPy(nullptr);
}

// constructor method
int ShapeFix_FreeBoundsPy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
    if (PyArg_ParseTuple(args, "")) {
        setTwinPointer(new ShapeFix_FreeBounds);
        return 0;
    }

    PyErr_Clear();
    PyObject* shape;
    PyObject* splitclosed;
    PyObject* splitopen;
    double sewtoler;
    double closetoler;
    if (PyArg_ParseTuple(args, "O!ddO!O!", &TopoShapePy::Type, &shape, &sewtoler, &closetoler,
                                           &PyBool_Type, &splitclosed, &PyBool_Type, &splitopen)) {
        TopoDS_Shape sh = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        setTwinPointer(new ShapeFix_FreeBounds(sh, sewtoler, closetoler,
                                               Base::asBoolean(splitclosed),
                                               Base::asBoolean(splitopen)));
        return 0;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!dO!O!", &TopoShapePy::Type, &shape, &closetoler,
                                          &PyBool_Type, &splitclosed, &PyBool_Type, &splitopen)) {
        TopoDS_Shape sh = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        setTwinPointer(new ShapeFix_FreeBounds(sh, closetoler,
                                               Base::asBoolean(splitclosed),
                                               Base::asBoolean(splitopen)));
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "ShapeFix_FreeBounds()\n"
                                     "ShapeFix_FreeBounds(shape, sewtolerance, closetolerance, splitClosed, splitOpen)\n"
                                     "ShapeFix_FreeBounds(shape, closetolerance, splitClosed, splitOpen)");
    return -1;
}

PyObject* ShapeFix_FreeBoundsPy::closedWires(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    TopoShape comp = getShapeFix_FreeBoundsPtr()->GetClosedWires();
    return comp.getPyObject();
}

PyObject* ShapeFix_FreeBoundsPy::openWires(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    TopoShape comp = getShapeFix_FreeBoundsPtr()->GetOpenWires();
    return comp.getPyObject();
}

PyObject* ShapeFix_FreeBoundsPy::shape(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    TopoShape shape = getShapeFix_FreeBoundsPtr()->GetShape();
    return shape.getPyObject();
}

PyObject *ShapeFix_FreeBoundsPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ShapeFix_FreeBoundsPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
