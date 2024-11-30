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
# include <Standard_Version.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Solid.hxx>

# if OCC_VERSION_HEX >= 0x070500
#  include <Message_ProgressRange.hxx>
# endif
#endif

#include "ShapeFix/ShapeFix_SolidPy.h"
#include "ShapeFix/ShapeFix_SolidPy.cpp"
#include "ShapeFix/ShapeFix_ShellPy.h"
#include "TopoShapeShellPy.h"
#include "TopoShapeSolidPy.h"


using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string ShapeFix_SolidPy::representation() const
{
    return "<ShapeFix_Solid object>";
}

PyObject *ShapeFix_SolidPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of ShapeFix_SolidPy
    return new ShapeFix_SolidPy(nullptr);
}

// constructor method
int ShapeFix_SolidPy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
    PyObject* solid = nullptr;
    if (!PyArg_ParseTuple(args, "|O!", &TopoShapeSolidPy::Type, &solid))
        return -1;

    setHandle(new ShapeFix_Solid);
    if (solid) {
        getShapeFix_SolidPtr()->Init(TopoDS::Solid(static_cast<TopoShapePy*>(solid)->getTopoShapePtr()->getShape()));
    }

    return 0;
}

PyObject* ShapeFix_SolidPy::init(PyObject *args)
{
    PyObject* solid;
    if (!PyArg_ParseTuple(args, "O!", &TopoShapeSolidPy::Type, &solid))
        return nullptr;

    getShapeFix_SolidPtr()->Init(TopoDS::Solid(static_cast<TopoShapePy*>(solid)->getTopoShapePtr()->getShape()));
    Py_Return;
}

PyObject* ShapeFix_SolidPy::perform(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Standard_Boolean ok = getShapeFix_SolidPtr()->Perform();
    return Py::new_reference_to(Py::Boolean(ok ? true : false));
}

PyObject* ShapeFix_SolidPy::solidFromShell(PyObject *args)
{
    PyObject* shell;
    if (!PyArg_ParseTuple(args, "O!", &TopoShapeShellPy::Type, &shell))
        return nullptr;

    TopoDS_Shape shape = static_cast<TopoShapePy*>(shell)->getTopoShapePtr()->getShape();
    TopoShape solid = getShapeFix_SolidPtr()->SolidFromShell(TopoDS::Shell(shape));
    return solid.getPyObject();
}

PyObject* ShapeFix_SolidPy::solid(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    TopoShape shape = getShapeFix_SolidPtr()->Solid();
    return shape.getPyObject();
}

PyObject* ShapeFix_SolidPy::shape(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    TopoShape shape = getShapeFix_SolidPtr()->Shape();
    return shape.getPyObject();
}

PyObject* ShapeFix_SolidPy::fixShellTool(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Handle(ShapeFix_Shell) tool = getShapeFix_SolidPtr()->FixShellTool();
    ShapeFix_ShellPy* shell = new ShapeFix_ShellPy(nullptr);
    shell->setHandle(tool);
    return shell;
}

Py::Boolean ShapeFix_SolidPy::getFixShellMode() const
{
    return Py::Boolean(getShapeFix_SolidPtr()->FixShellMode());
}

void ShapeFix_SolidPy::setFixShellMode(Py::Boolean arg)
{
    getShapeFix_SolidPtr()->FixShellMode() = arg;
}

Py::Boolean ShapeFix_SolidPy::getFixShellOrientationMode() const
{
    return Py::Boolean(getShapeFix_SolidPtr()->FixShellOrientationMode());
}

void ShapeFix_SolidPy::setFixShellOrientationMode(Py::Boolean arg)
{
    getShapeFix_SolidPtr()->FixShellOrientationMode() = arg;
}

Py::Boolean ShapeFix_SolidPy::getCreateOpenSolidMode() const
{
    return Py::Boolean(getShapeFix_SolidPtr()->CreateOpenSolidMode());
}

void ShapeFix_SolidPy::setCreateOpenSolidMode(Py::Boolean arg)
{
    getShapeFix_SolidPtr()->CreateOpenSolidMode() = arg;
}

PyObject *ShapeFix_SolidPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ShapeFix_SolidPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
