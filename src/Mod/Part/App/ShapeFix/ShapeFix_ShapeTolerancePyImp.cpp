// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/***************************************************************************
 *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Mod/Part/PartGlobal.h>

#include <TopoDS.hxx>


#include "ShapeFix/ShapeFix_ShapeTolerancePy.h"
#include "ShapeFix/ShapeFix_ShapeTolerancePy.cpp"
#include "TopoShapePy.h"


using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string ShapeFix_ShapeTolerancePy::representation() const
{
    return "<ShapeFix_ShapeTolerance object>";
}

PyObject* ShapeFix_ShapeTolerancePy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    return new ShapeFix_ShapeTolerancePy(new ShapeFix_ShapeTolerance);
}

// constructor method
int ShapeFix_ShapeTolerancePy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
    if (!PyArg_ParseTuple(args, "")) {
        return -1;
    }
    return 0;
}

PyObject* ShapeFix_ShapeTolerancePy::limitTolerance(PyObject* args)
{
    PyObject* shape;
    double tmin;
    double tmax = 0.0;
    TopAbs_ShapeEnum styp = TopAbs_SHAPE;
    if (!PyArg_ParseTuple(args, "O!d|di", &TopoShapePy::Type, &shape, &tmin, &tmax, &styp)) {
        return nullptr;
    }

    TopoDS_Shape sh = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
    bool ok = getShapeFix_ShapeTolerancePtr()->LimitTolerance(sh, tmin, tmax, styp);
    return Py::new_reference_to(Py::Boolean(ok));
}

PyObject* ShapeFix_ShapeTolerancePy::setTolerance(PyObject* args)
{
    PyObject* shape;
    double prec;
    TopAbs_ShapeEnum styp = TopAbs_SHAPE;
    if (!PyArg_ParseTuple(args, "O!d|i", &TopoShapePy::Type, &shape, &prec, &styp)) {
        return nullptr;
    }

    TopoDS_Shape sh = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
    getShapeFix_ShapeTolerancePtr()->SetTolerance(sh, prec, styp);
    Py_Return;
}

PyObject* ShapeFix_ShapeTolerancePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ShapeFix_ShapeTolerancePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
