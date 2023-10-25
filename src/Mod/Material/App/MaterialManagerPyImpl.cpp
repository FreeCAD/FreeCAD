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

#ifndef _PreComp_
#include <boost/uuid/uuid_io.hpp>
#endif

#include "Exceptions.h"
#include "MaterialManager.h"
#include "MaterialManagerPy.h"
#include "MaterialPy.h"
#include "Materials.h"

#include "MaterialManagerPy.cpp"

using namespace Materials;

// returns a string which represents the object e.g. when printed in python
std::string MaterialManagerPy::representation() const
{
    std::stringstream str;
    str << "<MaterialManager object at " << getMaterialManagerPtr() << ">";

    return str.str();
}

PyObject* MaterialManagerPy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    // never create such objects with the constructor
    return new MaterialManagerPy(new MaterialManager());
}

// constructor method
int MaterialManagerPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

PyObject* MaterialManagerPy::getMaterial(PyObject* args)
{
    char* uuid;
    if (!PyArg_ParseTuple(args, "s", &uuid)) {
        return nullptr;
    }

    try {
        const Material& material =
            getMaterialManagerPtr()->getMaterial(QString::fromStdString(uuid));
        return new MaterialPy(new Material(material));
    }
    catch (const MaterialNotFound&) {
        PyErr_SetString(PyExc_LookupError, "Material not found");
        return nullptr;
    }
}

PyObject* MaterialManagerPy::getMaterialByPath(PyObject* args)
{
    char* path;
    char* lib = "";
    if (!PyArg_ParseTuple(args, "s|s", &path, &lib)) {
        return nullptr;
    }

    QString libPath(QString::fromStdString(lib));
    if (!libPath.isEmpty()) {
        try {
            const Material& material =
                getMaterialManagerPtr()->getMaterialByPath(QString::fromStdString(path), libPath);
            return new MaterialPy(new Material(material));
        }
        catch (const MaterialNotFound&) {
            PyErr_SetString(PyExc_LookupError, "Material not found");
            return nullptr;
        }
        catch (const LibraryNotFound&) {
            PyErr_SetString(PyExc_LookupError, "Library not found");
            return nullptr;
        }
    }

    try {
        const Material& material =
            getMaterialManagerPtr()->getMaterialByPath(QString::fromStdString(path));
        return new MaterialPy(new Material(material));
    }
    catch (const MaterialNotFound&) {
        PyErr_SetString(PyExc_LookupError, "Material not found");
        return nullptr;
    }
}

Py::List MaterialManagerPy::getMaterialLibraries() const
{
    auto libraries = getMaterialManagerPtr()->getMaterialLibraries();
    Py::List list;

    for (auto it = libraries->begin(); it != libraries->end(); it++) {
        MaterialLibrary* lib = *it;
        Py::Tuple libTuple(3);
        libTuple.setItem(0, Py::String(lib->getName().toStdString()));
        libTuple.setItem(1, Py::String(lib->getDirectoryPath().toStdString()));
        libTuple.setItem(2, Py::String(lib->getIconPath().toStdString()));

        list.append(libTuple);
    }

    return list;
}

Py::Dict MaterialManagerPy::getMaterials() const
{
    Py::Dict dict;

    auto materials = getMaterialManagerPtr()->getMaterials();

    for (auto it = materials->begin(); it != materials->end(); it++) {
        QString key = it->first;
        Material* material = it->second;

        PyObject* materialPy = new MaterialPy(new Material(*material));
        dict.setItem(Py::String(key.toStdString()), Py::Object(materialPy, true));
    }

    // return Py::new_reference_to(dict);
    return dict;
}

PyObject* MaterialManagerPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int MaterialManagerPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

PyObject* MaterialManagerPy::materialsWithModel(PyObject* args)
{
    char* uuid;
    if (!PyArg_ParseTuple(args, "s", &uuid)) {
        return nullptr;
    }

    auto materials = getMaterialManagerPtr()->materialsWithModel(QString::fromStdString(uuid));
    PyObject* dict = PyDict_New();

    for (auto it = materials->begin(); it != materials->end(); it++) {
        QString key = it->first;
        Material* material = it->second;

        PyObject* materialPy = new MaterialPy(new Material(*material));
        PyDict_SetItem(dict, PyUnicode_FromString(key.toStdString().c_str()), materialPy);
    }

    return dict;
}

PyObject* MaterialManagerPy::materialsWithModelComplete(PyObject* args)
{
    char* uuid;
    if (!PyArg_ParseTuple(args, "s", &uuid)) {
        return nullptr;
    }

    auto materials =
        getMaterialManagerPtr()->materialsWithModelComplete(QString::fromStdString(uuid));
    PyObject* dict = PyDict_New();

    for (auto it = materials->begin(); it != materials->end(); it++) {
        QString key = it->first;
        Material* material = it->second;

        PyObject* materialPy = new MaterialPy(new Material(*material));
        PyDict_SetItem(dict, PyUnicode_FromString(key.toStdString().c_str()), materialPy);
    }

    return dict;
}
