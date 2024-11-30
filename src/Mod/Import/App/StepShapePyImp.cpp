/***************************************************************************
 *   Copyright (c) 2014 Jürgen Riegel <juergen.riegel@web.de>              *
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
