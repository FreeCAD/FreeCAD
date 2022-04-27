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
# include <Precision.hxx>
# include <Standard_Failure.hxx>
# include <TopoDS.hxx>
#endif

#include "ShapeFix/ShapeFix_ShellPy.h"
#include "ShapeFix/ShapeFix_ShellPy.cpp"
#include <Mod/Part/App/TopoShapeShellPy.h>

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string ShapeFix_ShellPy::representation() const
{
    return "<ShapeFix_Shell object>";
}

PyObject *ShapeFix_ShellPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of ShapeFix_RootPy
    return new ShapeFix_ShellPy(new ShapeFix_Shell);
}

// constructor method
int ShapeFix_ShellPy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
    PyObject* shell = nullptr;
    if (!PyArg_ParseTuple(args, "|O!", &TopoShapeShellPy::Type, &shell))
        return -1;

    if (shell) {
        getShapeFix_ShellPtr()->Init(TopoDS::Shell(static_cast<TopoShapePy*>(shell)->getTopoShapePtr()->getShape()));
    }

    return 0;
}

PyObject* ShapeFix_ShellPy::init(PyObject *args)
{
    PyObject* shell;
    if (!PyArg_ParseTuple(args, "O!", &TopoShapeShellPy::Type, &shell))
        return nullptr;

    getShapeFix_ShellPtr()->Init(TopoDS::Shell(static_cast<TopoShapePy*>(shell)->getTopoShapePtr()->getShape()));
    Py_Return;
}

PyObject* ShapeFix_ShellPy::perform(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Standard_Boolean ok = getShapeFix_ShellPtr()->Perform();
    return Py::new_reference_to(Py::Boolean(ok ? true : false));
}

PyObject* ShapeFix_ShellPy::shell(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    TopoShape shape = getShapeFix_ShellPtr()->Shell();
    return shape.getPyObject();
}

PyObject* ShapeFix_ShellPy::shape(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    TopoShape shape = getShapeFix_ShellPtr()->Shape();
    return shape.getPyObject();
}

PyObject *ShapeFix_ShellPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ShapeFix_ShellPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
