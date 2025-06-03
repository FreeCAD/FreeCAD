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

#include "Exceptions.h"
#include "MaterialFilter.h"
#include "MaterialFilterPy.h"
#include "MaterialManager.h"
#include "MaterialManagerPy.h"
#include "MaterialPy.h"
#include "Materials.h"

#include "MaterialManagerPy.cpp"

#include <Base/PyWrapParseTupleAndKeywords.h>

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
    return new MaterialManagerPy(&(MaterialManager::getManager()));
}

// constructor method
int MaterialManagerPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

PyObject* MaterialManagerPy::getMaterial(PyObject* args)
{
    char* uuid {};
    if (!PyArg_ParseTuple(args, "s", &uuid)) {
        return nullptr;
    }

    try {
        auto material = getMaterialManagerPtr()->getMaterial(QString::fromStdString(uuid));
        return new MaterialPy(new Material(*material));
    }
    catch (const MaterialNotFound&) {
        PyErr_SetString(PyExc_LookupError, "Material not found");
        return nullptr;
    }
}

PyObject* MaterialManagerPy::getMaterialByPath(PyObject* args)
{
    char* path {};
    const char* lib = "";
    if (!PyArg_ParseTuple(args, "et|s", "utf-8", &path, &lib)) {
        return nullptr;
    }

    std::string utf8Path = std::string(path);
    PyMem_Free(path);

    QString libPath(QString::fromStdString(lib));
    if (!libPath.isEmpty()) {
        try {
            auto material =
                getMaterialManagerPtr()->getMaterialByPath(QString::fromUtf8(utf8Path.c_str()),
                                                           libPath);
            return new MaterialPy(new Material(*material));
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
        auto material =
            getMaterialManagerPtr()->getMaterialByPath(QString::fromUtf8(utf8Path.c_str()));
        return new MaterialPy(new Material(*material));
    }
    catch (const MaterialNotFound&) {
        PyErr_SetString(PyExc_LookupError, "Material not found");
        return nullptr;
    }
}

PyObject* MaterialManagerPy::inheritMaterial(PyObject* args)
{
    char* uuid {};
    if (!PyArg_ParseTuple(args, "s", &uuid)) {
        return nullptr;
    }

    try {
        auto parent = getMaterialManagerPtr()->getMaterial(QString::fromStdString(uuid));

        // Found the parent. Create a new material with this as parent
        auto material = new Material();
        material->setParentUUID(QString::fromLatin1(uuid));
        return new MaterialPy(material); // Transfers ownership
    }
    catch (const MaterialNotFound&) {
        PyErr_SetString(PyExc_LookupError, "Material not found");
        return nullptr;
    }
}

Py::List MaterialManagerPy::getMaterialLibraries() const
{
    auto libraries = getMaterialManagerPtr()->getLibraries();
    Py::List list;

    for (auto it = libraries->begin(); it != libraries->end(); it++) {
        auto lib = *it;
        Py::Tuple libTuple(3);
        if (lib->isLocal()) {
            auto materialLibrary =
                reinterpret_cast<const std::shared_ptr<Materials::MaterialLibraryLocal>&>(lib);
            libTuple.setItem(0, Py::String(materialLibrary->getName().toStdString()));
            libTuple.setItem(1, Py::String(materialLibrary->getDirectoryPath().toStdString()));
            libTuple.setItem(2,
                             Py::Bytes(Py::Bytes(materialLibrary->getIcon().data(),
                                                 materialLibrary->getIcon().size())));
        }
        else
        {
            libTuple.setItem(0, Py::String());
            libTuple.setItem(1, Py::String());
            libTuple.setItem(2, Py::Bytes());
        }

        list.append(libTuple);
    }

    return list;
}

Py::Dict MaterialManagerPy::getMaterials() const
{
    Py::Dict dict;

    auto materials = getMaterialManagerPtr()->getLocalMaterials();

    for (auto it = materials->begin(); it != materials->end(); it++) {
        QString key = it->first;
        auto material = it->second;

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
    char* uuid {};
    if (!PyArg_ParseTuple(args, "s", &uuid)) {
        return nullptr;
    }

    auto materials = getMaterialManagerPtr()->materialsWithModel(QString::fromStdString(uuid));
    Py::Dict dict;

    for (auto it = materials->begin(); it != materials->end(); it++) {
        QString key = it->first;
        auto material = it->second;

        PyObject* materialPy = new MaterialPy(new Material(*material));
        dict.setItem(key.toStdString(), Py::asObject(materialPy));
    }

    return Py::new_reference_to(dict);
}

PyObject* MaterialManagerPy::materialsWithModelComplete(PyObject* args)
{
    char* uuid {};
    if (!PyArg_ParseTuple(args, "s", &uuid)) {
        return nullptr;
    }

    auto materials =
        getMaterialManagerPtr()->materialsWithModelComplete(QString::fromStdString(uuid));
    Py::Dict dict;

    for (auto it = materials->begin(); it != materials->end(); it++) {
        QString key = it->first;
        auto material = it->second;

        PyObject* materialPy = new MaterialPy(new Material(*material));
        dict.setItem(key.toStdString(), Py::asObject(materialPy));
    }

    return Py::new_reference_to(dict);
}

PyObject* MaterialManagerPy::save(PyObject* args, PyObject* kwds)
{
    char* libraryName {};
    PyObject* obj {};
    char* path {};
    PyObject* overwrite = Py_False;
    PyObject* saveAsCopy = Py_False;
    PyObject* saveInherited = Py_False;
    static const std::array<const char *, 7> kwlist { "library", "material", "path", "overwrite", "saveAsCopy", "saveInherited", nullptr };
    if (!Base::Wrapped_ParseTupleAndKeywords(args,
                                     kwds,
                                     "etOet|O!O!O!",
                                     kwlist,
                                     "utf-8", &libraryName,
                                     &obj,
                                     "utf-8", &path,
                                     &PyBool_Type, &overwrite,
                                     &PyBool_Type, &saveAsCopy,
                                     &PyBool_Type, &saveInherited)) {
        return nullptr;
    }

    Base::Console().log("library name %s\n", libraryName);
    Base::Console().log("path %s\n", path);

    MaterialPy* material;
    if (QLatin1String(obj->ob_type->tp_name) == QLatin1String("Materials.Material")) {
        material = static_cast<MaterialPy*>(obj);
    }
    else {
        PyErr_Format(PyExc_TypeError, "Material expected not '%s'", obj->ob_type->tp_name);
        return nullptr;
    }
    if (!material) {
        PyErr_SetString(PyExc_TypeError, "Invalid material object");
        return nullptr;
    }
    auto sharedMaterial = std::make_shared<Material>(*(material->getMaterialPtr()));

    std::shared_ptr<MaterialLibrary> library;
    try {
        library = getMaterialManagerPtr()->getLibrary(QString::fromUtf8(libraryName));
    }
    catch (const LibraryNotFound&) {
        PyErr_SetString(PyExc_LookupError, "Unknown library");
        return nullptr;
    }


    getMaterialManagerPtr()->saveMaterial(library,
                                          sharedMaterial,
                                          QString::fromUtf8(path),
                                          PyObject_IsTrue(overwrite),
                                          PyObject_IsTrue(saveAsCopy),
                                          PyObject_IsTrue(saveInherited));
    material->getMaterialPtr()->setUUID(sharedMaterial->getUUID()); // Make sure they match

    Py_INCREF(Py_None);
    return Py_None;
}

void addMaterials(MaterialManager *manager,
                  Py::List& list,
                  const std::shared_ptr<std::map<QString, std::shared_ptr<MaterialTreeNode>>>& tree)
{
    for (auto& node : *tree) {
        if (node.second->getType() == MaterialTreeNode::NodeType::DataNode) {
            auto uuid = node.second->getUUID();
            auto material = manager->getMaterial(uuid);
            PyObject* materialPy = new MaterialPy(new Material(*material));
            list.append(Py::Object(materialPy, true));
        }
        else {
            addMaterials(manager, list, node.second->getFolder());
        }
    }
}

PyObject* MaterialManagerPy::filterMaterials(PyObject* args, PyObject* kwds)
{
    PyObject* filterPy {};
    PyObject* includeLegacy = Py_False;
    static const std::array<const char*, 3> kwds_save{ "filter",
                                                       "includeLegacy",
                                                       nullptr };
    if (!Base::Wrapped_ParseTupleAndKeywords(args,
                                             kwds,
                                             //  "O|O!",
                                             "O!|O!",
                                             kwds_save,
                                             &MaterialFilterPy::Type,
                                             &filterPy,
                                             &PyBool_Type,
                                             &includeLegacy)) {
        return nullptr;
    }

    MaterialFilterOptions options;
    options.setIncludeFavorites(false);
    options.setIncludeRecent(false);
    options.setIncludeEmptyFolders(false);
    options.setIncludeEmptyLibraries(false);
    options.setIncludeLegacy(PyObject_IsTrue(includeLegacy));

    auto filter = std::make_shared<MaterialFilter>(*(static_cast<MaterialFilterPy*>(filterPy)->getMaterialFilterPtr()));

    auto libraries = getMaterialManagerPtr()->getLibraries();
    Py::List list;

    for (auto lib : *libraries) {
        auto tree = getMaterialManagerPtr()->getMaterialTree(*lib, *filter, options);
        if (tree->size() > 0) {
            addMaterials(getMaterialManagerPtr(), list, tree);
        }
    }

    Py_INCREF(*list);
    return *list;
}

PyObject* MaterialManagerPy::refresh(PyObject* /*args*/)
{
    getMaterialManagerPtr()->refresh();

    Py_INCREF(Py_None);
    return Py_None;
}
