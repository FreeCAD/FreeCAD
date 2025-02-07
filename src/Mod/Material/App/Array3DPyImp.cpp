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

#include <QList>
#include <QMetaType>

#include <Base/Quantity.h>
#include <Base/QuantityPy.h>
#include <CXX/Objects.hxx>
#include <Gui/MetaTypes.h>

#include "Array3DPy.h"
#include "Exceptions.h"
#include "Model.h"
#include "ModelLibrary.h"
#include "ModelPropertyPy.h"
#include "ModelUuids.h"

#include "Array3DPy.cpp"

using namespace Materials;

// returns a string which represents the object e.g. when printed in python
std::string Array3DPy::representation() const
{
    std::stringstream str;
    str << "<Array3D object at " << getMaterial3DArrayPtr() << ">";

    return str.str();
}

PyObject* Array3DPy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    // never create such objects with the constructor
    return new Array3DPy(new Material3DArray());
}

// constructor method
int Array3DPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

Py::List Array3DPy::getArray() const
{
    Py::List list;
    auto array = getMaterial3DArrayPtr()->getArray();

    for (auto& depth : array) {
        Py::List depthList;
        for (auto& row : *std::get<1>(depth)) {
            Py::List rowList;
            for (auto& column : *row) {
                auto quantity = new Base::QuantityPy(new Base::Quantity(column));
                rowList.append(Py::asObject(quantity));
            }

            depthList.append(rowList);
        }
        list.append(depthList);
    }

    return list;
}

Py::Long Array3DPy::getColumns() const
{
    return Py::Long(getMaterial3DArrayPtr()->columns());
}

Py::Long Array3DPy::getDepth() const
{
    return Py::Long(getMaterial3DArrayPtr()->depth());
}

PyObject* Array3DPy::getRows(PyObject* args)
{
    int depth = getMaterial3DArrayPtr()->currentDepth();
    if (!PyArg_ParseTuple(args, "|i", &depth)) {
        return nullptr;
    }

    return PyLong_FromLong(getMaterial3DArrayPtr()->rows(depth));
}

PyObject* Array3DPy::getValue(PyObject* args)
{
    int depth;
    int row;
    int column;
    if (!PyArg_ParseTuple(args, "iii", &depth, &row, &column)) {
        return nullptr;
    }

    try {
        auto value = getMaterial3DArrayPtr()->getValue(depth, row, column);
        return new Base::QuantityPy(new Base::Quantity(value));
    }
    catch (const InvalidIndex&) {
    }

    PyErr_SetString(PyExc_IndexError, "Invalid array index");
    return nullptr;
}

PyObject* Array3DPy::getDepthValue(PyObject* args)
{
    int depth;
    if (!PyArg_ParseTuple(args, "i", &depth)) {
        return nullptr;
    }

    try {
        auto value = getMaterial3DArrayPtr()->getDepthValue(depth);
        return new Base::QuantityPy(new Base::Quantity(value));
    }
    catch (const InvalidIndex&) {
    }

    PyErr_SetString(PyExc_IndexError, "Invalid array index");
    return nullptr;
}

PyObject* Array3DPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int Array3DPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
