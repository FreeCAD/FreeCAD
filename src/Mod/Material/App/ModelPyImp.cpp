// SPDX-License-Identifier: LGPL-2.1-or-later

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


#include "Model.h"
#include "ModelLibrary.h"
#include "ModelPropertyPy.h"
#include "ModelPy.h"
#include "ModelUuids.h"

#include "ModelPy.cpp"

using namespace Materials;

// returns a string which represents the object e.g. when printed in python
std::string ModelPy::representation() const
{
    std::stringstream str;
    str << "<Model object at " << getModelPtr() << ">";

    return str.str();
}

PyObject* ModelPy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    // never create such objects with the constructor
    return new ModelPy(new Model());
}

// constructor method
int ModelPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

Py::String ModelPy::getLibraryName() const
{
    auto library = getModelPtr()->getLibrary();
    return Py::String(library ? library->getName() : "");
}

Py::String ModelPy::getLibraryRoot() const
{
    auto library = getModelPtr()->getLibrary();
    if (!library->isLocal()) {
        return "";
    }
    return Py::String(library ? library->getDirectoryPath() : "");
}

Py::Object ModelPy::getLibraryIcon() const
{
    auto library = getModelPtr()->getLibrary();
    return Py::Bytes(library->getIcon().data(), library->getIcon().size());
}

Py::String ModelPy::getName() const
{
    return Py::String(getModelPtr()->getName());
}

void ModelPy::setName(Py::String arg)
{
    getModelPtr()->setName(arg);
}

Py::String ModelPy::getType() const
{
    auto type = (getModelPtr()->getType() == Model::ModelType_Physical)
        ? "Physical"
        : "Appearance";

    return Py::String(type);
}

void ModelPy::setType(Py::String arg)
{
    if (arg.as_std_string() == "Appearance") {
        getModelPtr()->setType(Model::ModelType_Appearance);
    }
    else {
        getModelPtr()->setType(Model::ModelType_Physical);
    }
}

Py::String ModelPy::getDirectory() const
{
    return Py::String(getModelPtr()->getDirectory());
}

void ModelPy::setDirectory(Py::String arg)
{
    getModelPtr()->setDirectory(arg);
}

Py::String ModelPy::getUUID() const
{
    return Py::String(getModelPtr()->getUUID());
}

Py::String ModelPy::getDescription() const
{
    return Py::String(getModelPtr()->getDescription());
}

void ModelPy::setDescription(Py::String arg)
{
    getModelPtr()->setDescription(arg);
}

Py::String ModelPy::getURL() const
{
    return Py::String(getModelPtr()->getURL());
}

void ModelPy::setURL(Py::String arg)
{
    getModelPtr()->setURL(arg);
}

Py::String ModelPy::getDOI() const
{
    return Py::String(getModelPtr()->getDOI());
}

void ModelPy::setDOI(Py::String arg)
{
    getModelPtr()->setDOI(arg);
}

Py::List ModelPy::getInherited() const
{
    auto& inherited = getModelPtr()->getInheritance();
    Py::List list;

    for (auto it = inherited.begin(); it != inherited.end(); it++) {
        list.append(Py::String(*it));
    }

    return list;
}

Py::Dict ModelPy::getProperties() const
{
    Py::Dict dict;

    for (auto it = getModelPtr()->begin(); it != getModelPtr()->end(); it++) {
        const std::string& key = it->first;
        ModelProperty& modelProperty = it->second;

        PyObject* modelPropertyPy = new ModelPropertyPy(new ModelProperty(modelProperty));
        dict.setItem(Py::String(key), Py::Object(modelPropertyPy, true));
    }

    return dict;
}

PyObject* ModelPy::addInheritance(PyObject* args)
{
    char* uuid;
    if (!PyArg_ParseTuple(args, "s", &uuid)) {
        return nullptr;
    }

    getModelPtr()->addInheritance(uuid);
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* ModelPy::addProperty(PyObject* args)
{
    PyObject* object;
    if (!PyArg_ParseTuple(args, "O!", &ModelPropertyPy::Type, &object)) {
        return nullptr;
    }
    ModelProperty* property = static_cast<ModelPropertyPy*>(object)->getModelPropertyPtr();

    getModelPtr()->addProperty(*property);
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* ModelPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ModelPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
