// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2014 Jürgen Riegel <juergen.riegel@web.de>                             *
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


// clang-format off
// inclusion of the generated files (generated out of StepShapePy.xml)
#include "StepShapePy.h"
#include "StepShapePy.cpp"
// clang-format on

using namespace Import;

// returns a string which represents the object e.g. when printed in python
std::string StepShapePy::representation() const
{
    return {"<StepShape object>"};
}

PyObject* StepShapePy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    // create a new instance of StepShapePy and the Twin object
    return new StepShapePy(new StepShape);
}

// constructor method
int StepShapePy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    char* fileName;
    if (PyArg_ParseTuple(args, "s", &fileName)) {
        getStepShapePtr()->read(fileName);
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "StepShape needs a file name\n");
    return -1;
}

PyObject* StepShapePy::read(PyObject* /*args*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return nullptr;
}

PyObject* StepShapePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int StepShapePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
