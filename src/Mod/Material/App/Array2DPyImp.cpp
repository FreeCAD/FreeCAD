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

#include "Array2DPy.h"
#include "Exceptions.h"
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

Py::List Array2DPy::getArray() const
{
    Py::List list;

    auto array = getMaterial2DArrayPtr()->getArray();

    for (auto& row : array) {
        Py::List rowList;
        for (auto& column : *row) {
            auto quantity =
                new Base::QuantityPy(new Base::Quantity(column.value<Base::Quantity>()));
            rowList.append(Py::asObject(quantity));
        }

        list.append(rowList);
    }

    return list;
}

Py::Long Array2DPy::getRows() const
{
    return Py::Long(getMaterial2DArrayPtr()->rows());
}

Py::Long Array2DPy::getColumns() const
{
    return Py::Long(getMaterial2DArrayPtr()->columns());
}

PyObject* Array2DPy::getRow(PyObject* args)
{
    int row;
    if (!PyArg_ParseTuple(args, "i", &row)) {
        return nullptr;
    }

    try {
        Py::List list;

        auto arrayRow = getMaterial2DArrayPtr()->getRow(row);
        for (auto& column : *arrayRow) {
            auto quantity =
                new Base::QuantityPy(new Base::Quantity(column.value<Base::Quantity>()));
            list.append(Py::asObject(quantity));
        }

        return Py::new_reference_to(list);
    }
    catch (const InvalidIndex&) {
    }

    PyErr_SetString(PyExc_IndexError, "Invalid array index");
    return nullptr;
}

PyObject* Array2DPy::getValue(PyObject* args)
{
    int row;
    int column;
    if (!PyArg_ParseTuple(args, "ii", &row, &column)) {
        return nullptr;
    }

    try {
        auto value = getMaterial2DArrayPtr()->getValue(row, column);
        return new Base::QuantityPy(new Base::Quantity(value.value<Base::Quantity>()));
    }
    catch (const InvalidIndex&) {
    }

    PyErr_SetString(PyExc_IndexError, "Invalid array index");
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
