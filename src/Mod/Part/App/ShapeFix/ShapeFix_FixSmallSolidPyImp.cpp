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
# include <ShapeBuild_ReShape.hxx>
#endif

#include "ShapeFix/ShapeFix_FixSmallSolidPy.h"
#include "ShapeFix/ShapeFix_FixSmallSolidPy.cpp"
#include "TopoShapePy.h"


using namespace Part;

std::string ShapeFix_FixSmallSolidPy::representation() const
{
    return "<ShapeFix_FixSmallSolid object>";
}

PyObject *ShapeFix_FixSmallSolidPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    return new ShapeFix_FixSmallSolidPy(new ShapeFix_FixSmallSolid);
}

// constructor method
int ShapeFix_FixSmallSolidPy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
    if (!PyArg_ParseTuple(args, ""))
        return -1;
    return 0;
}

PyObject* ShapeFix_FixSmallSolidPy::setFixMode(PyObject *args)
{
    int mode;
    if (!PyArg_ParseTuple(args, "i", &mode))
        return nullptr;

    getShapeFix_FixSmallSolidPtr()->SetFixMode(mode);
    Py_Return;
}

PyObject* ShapeFix_FixSmallSolidPy::setVolumeThreshold(PyObject *args)
{
    double value = -1.0;
    if (!PyArg_ParseTuple(args, "|d", &value))
        return nullptr;

    getShapeFix_FixSmallSolidPtr()->SetVolumeThreshold(value);
    Py_Return;
}

PyObject* ShapeFix_FixSmallSolidPy::setWidthFactorThreshold(PyObject *args)
{
    double value = -1.0;
    if (!PyArg_ParseTuple(args, "|d", &value))
        return nullptr;

    getShapeFix_FixSmallSolidPtr()->SetWidthFactorThreshold(value);
    Py_Return;
}

PyObject* ShapeFix_FixSmallSolidPy::remove(PyObject *args)
{
    PyObject* shape;
    if (!PyArg_ParseTuple(args, "O!", &TopoShapePy::Type, &shape))
        return nullptr;

    Handle(ShapeBuild_ReShape) context = new ShapeBuild_ReShape();
    TopoShape sh = getShapeFix_FixSmallSolidPtr()->Remove(static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape(), context);
    return sh.getPyObject();
}

PyObject* ShapeFix_FixSmallSolidPy::merge(PyObject *args)
{
    PyObject* shape;
    if (!PyArg_ParseTuple(args, "O!", &TopoShapePy::Type, &shape))
        return nullptr;

    Handle(ShapeBuild_ReShape) context = new ShapeBuild_ReShape();
    TopoShape sh = getShapeFix_FixSmallSolidPtr()->Merge(static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape(), context);
    return sh.getPyObject();
}

PyObject *ShapeFix_FixSmallSolidPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ShapeFix_FixSmallSolidPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
