// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Mario Passaglia <mpassaglia[at]cbc.uba.ar>         *
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

#include <Python.h>

#include "FemPostObjectPy.h"
#include "FemPostObjectPy.cpp"

#ifdef FC_USE_VTK_PYTHON
# include <vtkDataSet.h>
# include <vtkPythonUtil.h>
#endif  // BUILD_FEM_VTK

using namespace Fem;

// returns a string which represent the object e.g. when printed in python
std::string FemPostObjectPy::representation() const
{
    std::stringstream str;
    str << "<FemPostObject object at " << getFemPostObjectPtr() << ">";

    return str.str();
}

PyObject* FemPostObjectPy::writeVTK(PyObject* args)
{
    char* filename;
    if (!PyArg_ParseTuple(args, "et", "utf-8", &filename)) {
        return nullptr;
    }

    std::string utf8Name(filename);
    PyMem_Free(filename);
    getFemPostObjectPtr()->writeVTK(utf8Name.c_str());

    Py_Return;
}

PyObject* FemPostObjectPy::getDataSet([[maybe_unused]] PyObject* args)
{
#ifdef FC_USE_VTK_PYTHON
    // we take no arguments
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    // return python object for the dataset
    auto dataset = getFemPostObjectPtr()->getDataSet();
    if (dataset) {
        PyObject* py_algorithm = vtkPythonUtil::GetObjectFromPointer(dataset);
        return py_algorithm;
    }

    Py_Return;
#else
    PyErr_SetString(PyExc_NotImplementedError, "VTK python wrapper not available");
    return nullptr;
#endif
}

PyObject* FemPostObjectPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int FemPostObjectPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
