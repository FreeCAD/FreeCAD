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

#include "Model.h"
#include "PyVariants.h"
#include "ModelPropertyPy.h"
#include "MaterialPropertyPy.h"

#include "MaterialPropertyPy.cpp"

using namespace Materials;

// returns a string which represents the object e.g. when printed in python
std::string MaterialPropertyPy::representation() const
{
    std::stringstream str;
    str << "<MaterialProperty object at " << getMaterialPropertyPtr() << ">";

    return str.str();
}

PyObject* MaterialPropertyPy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    // never create such objects with the constructor
    return new MaterialPropertyPy(new MaterialProperty());
}

// constructor method
int MaterialPropertyPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

Py::Object MaterialPropertyPy::getValue() const
{
    auto value = getMaterialPropertyPtr()->getValue();

    return Py::Object(_pyObjectFromVariant(value), true);
}

Py::Boolean MaterialPropertyPy::getEmpty() const
{
    return getMaterialPropertyPtr()->isEmpty();
}

PyObject* MaterialPropertyPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int MaterialPropertyPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}