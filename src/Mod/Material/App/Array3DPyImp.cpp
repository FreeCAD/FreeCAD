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
    str << "<Array3D object at " << getArray3DPtr() << ">";

    return str.str();
}

PyObject* Array3DPy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    // never create such objects with the constructor
    return new Array3DPy(new Array3D());
}

// constructor method
int Array3DPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

Py::List Array3DPy::getArray() const
{
    Py::List list;
    auto array = getArray3DPtr()->getArray();

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

Py::Long Array3DPy::getDimensions() const
{
    return Py::Long(3);
}

Py::Long Array3DPy::getColumns() const
{
    return Py::Long(getArray3DPtr()->columns());
}

void Array3DPy::setColumns(Py::Long arg)
{
    getArray3DPtr()->setColumns(arg);
}

Py::Long Array3DPy::getDepth() const
{
    return Py::Long(getArray3DPtr()->depth());
}

void Array3DPy::setDepth(Py::Long arg)
{
    getArray3DPtr()->setDepth(arg);
}

PyObject* Array3DPy::getRows(PyObject* args) const
{
    int depth = getArray3DPtr()->currentDepth();
    if (!PyArg_ParseTuple(args, "|i", &depth)) {
        return nullptr;
    }

    return PyLong_FromLong(getArray3DPtr()->rows(depth));
}

PyObject* Array3DPy::getValue(PyObject* args) const
{
    int depth;
    int row;
    int column;
    if (!PyArg_ParseTuple(args, "iii", &depth, &row, &column)) {
        return nullptr;
    }

    try {
        auto value = getArray3DPtr()->getValue(depth, row, column);
        return new Base::QuantityPy(new Base::Quantity(value));
    }
    catch (const InvalidIndex&) {
    }

    PyErr_SetString(PyExc_IndexError, "Invalid array index");
    return nullptr;
}

PyObject* Array3DPy::getDepthValue(PyObject* args) const
{
    int depth;
    if (!PyArg_ParseTuple(args, "i", &depth)) {
        return nullptr;
    }

    try {
        auto value = getArray3DPtr()->getDepthValue(depth);
        return new Base::QuantityPy(new Base::Quantity(value));
    }
    catch (const InvalidIndex&) {
    }

    PyErr_SetString(PyExc_IndexError, "Invalid array index");
    return nullptr;
}

PyObject* Array3DPy::setDepthValue(PyObject* args)
{
    int depth;
    PyObject* valueObj;
    if (PyArg_ParseTuple(args, "iO!", &depth, &PyUnicode_Type, &valueObj)) {
        Py::String item(valueObj);
        try {
            auto quantity = Base::Quantity::parse(item.as_string());
            quantity.setFormat(MaterialValue::getQuantityFormat());
            getArray3DPtr()->setDepthValue(depth, quantity);
        }
        catch (const Base::ParserError& e) {
            PyErr_SetString(PyExc_ValueError, e.what());
            return nullptr;
        }
        catch (const InvalidIndex& e) {
            PyErr_SetString(PyExc_IndexError, e.what());
            return nullptr;
        }
        Py_Return;
    }

    PyErr_SetString(PyExc_TypeError, "Expected (integer, string) arguments");
    return nullptr;
}

PyObject* Array3DPy::setValue(PyObject* args)
{
    int depth;
    int row;
    int column;
    PyObject* valueObj;
    if (PyArg_ParseTuple(args, "iiiO!", &depth, &row, &column, &PyUnicode_Type, &valueObj)) {
        Py::String item(valueObj);
        try {
            auto quantity = Base::Quantity::parse(item.as_string());
            quantity.setFormat(MaterialValue::getQuantityFormat());
            getArray3DPtr()->setValue(depth, row, column, quantity);
        }
        catch (const Base::ParserError& e) {
            PyErr_SetString(PyExc_ValueError, e.what());
            return nullptr;
        }
        catch (const InvalidIndex& e) {
            PyErr_SetString(PyExc_IndexError, e.what());
            return nullptr;
        }
        Py_Return;
    }

    PyErr_SetString(PyExc_TypeError, "Expected (integer, integer, integer, string) arguments");
    return nullptr;
}

PyObject* Array3DPy::setRows(PyObject* args)
{
    int depth;
    int rows;
    if (!PyArg_ParseTuple(args, "ii", &depth, &rows)) {
        return nullptr;
    }

    getArray3DPtr()->setRows(depth, rows);
    Py_Return;
}

PyObject* Array3DPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int Array3DPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
