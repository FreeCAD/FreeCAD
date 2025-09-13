/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david@friedli-be.ch>                *
 *   Copyright (c) 2023 Wandererfan <wandererfan@gmail.com>                *
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


// inclusion of the generated files (generated out of MeasureManagerPy.xml)
#include "MeasureManagerPy.h"
#include "MeasureManagerPy.cpp"


using namespace App;


// returns a string which represents the object e.g. when printed in python
std::string MeasureManagerPy::representation() const
{
    return "<App::MeasureManager>";
}

PyObject* MeasureManagerPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int MeasureManagerPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}


PyObject* MeasureManagerPy::addMeasureType(PyObject* args)
{
    PyObject* pyobj = Py_None;
    char *id, *label;

    if (!PyArg_ParseTuple(args, "ssO", &id, &label, &pyobj)) {
        return nullptr;
    }

    MeasureManager::addMeasureType(
        new App::MeasureType {id, label, "", nullptr, nullptr, true, pyobj});

    Py_Return;
}


PyObject* MeasureManagerPy::getMeasureTypes()
{
    Py::List types;
    for (auto& it : MeasureManager::getMeasureTypes()) {
        Py::Tuple type(3);
        type.setItem(0, Py::String(it->identifier));
        type.setItem(1, Py::String(it->label));
        type.setItem(2, Py::Object(it->pythonClass));

        types.append(type);
    }

    return Py::new_reference_to(types);
}
