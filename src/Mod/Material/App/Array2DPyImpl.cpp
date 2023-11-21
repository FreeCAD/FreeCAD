/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"

#include "Array2DPy.h"
#include "Model.h"
#include "ModelLibrary.h"
#include "ModelPropertyPy.h"
#include "ModelUuids.h"

#include "Array2DPy.cpp"

using namespace Materials;

// returns a string which represents the object e.g. when printed in python
std::string Array2DPy::representation() const
{
    std::stringstream str;
    str << "<Array2D object at " << getMaterial2DArrayPtr() << ">";

    return str.str();
}

PyObject* Array2DPy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    // never create such objects with the constructor
    return new Array2DPy(new Material2DArray());
}

// constructor method
int Array2DPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

Py::Int Array2DPy::getRows() const
{
    return Py::Int(getMaterial2DArrayPtr()->rows());
}

Py::Int Array2DPy::getColumns() const
{
    return Py::Int(getMaterial2DArrayPtr()->columns());
}

PyObject* Array2DPy::getDefaultValue(PyObject* args)
{
    char* name;
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return nullptr;
    }

    // QVariant value = getMaterial2DArrayPtr()->getPhysicalValue(QString::fromStdString(name));
    // return _pyObjectFromVariant(value);
    return nullptr;
}

PyObject* Array2DPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int Array2DPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
