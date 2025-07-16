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

#include <QMetaType>

#include <Base/Quantity.h>
#include <Base/QuantityPy.h>
#include <CXX/Objects.hxx>
#include <Gui/MetaTypes.h>

#include "Materials.h"

#include "Array2DPy.h"
#include "Array3DPy.h"
#include "Exceptions.h"
#include "MaterialLibrary.h"
#include "MaterialPy.h"
#include "MaterialValue.h"
#include "PyVariants.h"

#include "ModelPropertyPy.h"
#include "MaterialPropertyPy.h"

#include "MaterialPy.cpp"

using namespace Materials;

// returns a string which represents the object e.g. when printed in python
std::string MaterialPy::representation() const
{
    std::ostringstream str;
    str << "<Material at " << getMaterialPtr() << ">";
    return str.str();
}

PyObject* MaterialPy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    // never create such objects with the constructor
    return new MaterialPy(new Material());
}

// constructor method
int MaterialPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

Py::String MaterialPy::getLibraryName() const
{
    auto library = getMaterialPtr()->getLibrary();
    if (library->isLocal()) {
        auto materialLibrary =
            reinterpret_cast<const std::shared_ptr<Materials::MaterialLibraryLocal>&>(library);
        return {materialLibrary ? materialLibrary->getName().toStdString() : ""};
    }
    return "";
}

Py::String MaterialPy::getLibraryRoot() const
{
    auto library = getMaterialPtr()->getLibrary();
    if (library->isLocal()) {
        auto materialLibrary =
            reinterpret_cast<const std::shared_ptr<Materials::MaterialLibraryLocal>&>(library);
        return {materialLibrary ? materialLibrary->getDirectoryPath().toStdString() : ""};
    }
    return "";
}

Py::Object MaterialPy::getLibraryIcon() const
{
    auto library = getMaterialPtr()->getLibrary();
    if (library->isLocal()) {
        auto materialLibrary =
            reinterpret_cast<const std::shared_ptr<Materials::MaterialLibraryLocal>&>(library);
        auto icon = materialLibrary->getIcon();
        if (icon.isNull()) {
            return Py::Bytes();
        }
        return Py::Bytes(icon.data(), icon.size());
    }
    return Py::Bytes();
}

Py::String MaterialPy::getName() const
{
    return {getMaterialPtr()->getName().toStdString()};
}

void MaterialPy::setName(Py::String arg)
{
    getMaterialPtr()->setName(QString::fromStdString(arg));
}

Py::String MaterialPy::getDirectory() const
{
    return {getMaterialPtr()->getDirectory().toStdString()};
}

void MaterialPy::setDirectory(Py::String arg)
{
    getMaterialPtr()->setDirectory(QString::fromStdString(arg));
}

Py::String MaterialPy::getUUID() const
{
    return {getMaterialPtr()->getUUID().toStdString()};
}

Py::String MaterialPy::getDescription() const
{
    return {getMaterialPtr()->getDescription().toStdString()};
}

void MaterialPy::setDescription(Py::String arg)
{
    getMaterialPtr()->setDescription(QString::fromStdString(arg));
}

Py::String MaterialPy::getURL() const
{
    return {getMaterialPtr()->getURL().toStdString()};
}

void MaterialPy::setURL(Py::String arg)
{
    getMaterialPtr()->setURL(QString::fromStdString(arg));
}

Py::String MaterialPy::getReference() const
{
    return {getMaterialPtr()->getReference().toStdString()};
}

void MaterialPy::setReference(Py::String arg)
{
    getMaterialPtr()->setReference(QString::fromStdString(arg));
}

Py::String MaterialPy::getParent() const
{
    return {getMaterialPtr()->getParentUUID().toStdString()};
}

void MaterialPy::setParent(Py::String arg)
{
    getMaterialPtr()->setParentUUID(QString::fromStdString(arg));
}

Py::String MaterialPy::getAuthorAndLicense() const
{
    return {getMaterialPtr()->getAuthorAndLicense().toStdString()};
}

Py::String MaterialPy::getAuthor() const
{
    return {getMaterialPtr()->getAuthor().toStdString()};
}

void MaterialPy::setAuthor(Py::String arg)
{
    getMaterialPtr()->setAuthor(QString::fromStdString(arg));
}

Py::String MaterialPy::getLicense() const
{
    return {getMaterialPtr()->getLicense().toStdString()};
}

void MaterialPy::setLicense(Py::String arg)
{
    getMaterialPtr()->setLicense(QString::fromStdString(arg));
}

Py::List MaterialPy::getPhysicalModels() const
{
    auto models = getMaterialPtr()->getPhysicalModels();
    Py::List list;

    for (auto it : *models) {
        list.append(Py::String(it.toStdString()));
    }

    return list;
}

Py::List MaterialPy::getAppearanceModels() const
{
    auto models = getMaterialPtr()->getAppearanceModels();
    Py::List list;

    for (auto it : *models) {
        list.append(Py::String(it.toStdString()));
    }

    return list;
}

Py::List MaterialPy::getTags() const
{
    auto& tags = getMaterialPtr()->getTags();
    Py::List list;

    for (auto it : tags) {
        list.append(Py::String(it.toStdString()));
    }

    return list;
}

PyObject* MaterialPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int MaterialPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

PyObject* MaterialPy::addPhysicalModel(PyObject* args)
{
    char* uuid;
    if (!PyArg_ParseTuple(args, "s", &uuid)) {
        return nullptr;
    }

    getMaterialPtr()->addPhysical(QString::fromStdString(uuid));
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* MaterialPy::removePhysicalModel(PyObject* args)
{
    char* uuid;
    if (!PyArg_ParseTuple(args, "s", &uuid)) {
        return nullptr;
    }

    getMaterialPtr()->removePhysical(QString::fromStdString(uuid));
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* MaterialPy::hasPhysicalModel(PyObject* args)
{
    char* uuid;
    if (!PyArg_ParseTuple(args, "s", &uuid)) {
        return nullptr;
    }

    bool hasProperty = getMaterialPtr()->hasPhysicalModel(QString::fromStdString(uuid));
    return PyBool_FromLong(hasProperty ? 1 : 0);
}

PyObject* MaterialPy::addAppearanceModel(PyObject* args)
{
    char* uuid;
    if (!PyArg_ParseTuple(args, "s", &uuid)) {
        return nullptr;
    }

    getMaterialPtr()->addAppearance(QString::fromStdString(uuid));
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* MaterialPy::removeAppearanceModel(PyObject* args)
{
    char* uuid;
    if (!PyArg_ParseTuple(args, "s", &uuid)) {
        return nullptr;
    }

    getMaterialPtr()->removeAppearance(QString::fromStdString(uuid));
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* MaterialPy::hasAppearanceModel(PyObject* args)
{
    char* uuid;
    if (!PyArg_ParseTuple(args, "s", &uuid)) {
        return nullptr;
    }

    bool hasProperty = getMaterialPtr()->hasAppearanceModel(QString::fromStdString(uuid));
    return PyBool_FromLong(hasProperty ? 1 : 0);
}

PyObject* MaterialPy::isPhysicalModelComplete(PyObject* args)
{
    char* name;
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return nullptr;
    }

    bool isComplete = getMaterialPtr()->isPhysicalModelComplete(QString::fromStdString(name));
    return PyBool_FromLong(isComplete ? 1 : 0);
}

PyObject* MaterialPy::isAppearanceModelComplete(PyObject* args)
{
    char* name;
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return nullptr;
    }

    bool isComplete = getMaterialPtr()->isAppearanceModelComplete(QString::fromStdString(name));
    return PyBool_FromLong(isComplete ? 1 : 0);
}

PyObject* MaterialPy::hasPhysicalProperty(PyObject* args)
{
    char* name;
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return nullptr;
    }

    bool hasProperty = getMaterialPtr()->hasPhysicalProperty(QString::fromStdString(name));
    return PyBool_FromLong(hasProperty ? 1 : 0);
}

PyObject* MaterialPy::hasAppearanceProperty(PyObject* args)
{
    char* name;
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return nullptr;
    }

    bool hasProperty = getMaterialPtr()->hasAppearanceProperty(QString::fromStdString(name));
    return PyBool_FromLong(hasProperty ? 1 : 0);
}

PyObject* MaterialPy::hasLegacyProperties(PyObject* args)
{
    if (!PyArg_ParseTuple(args, "")) {
        return nullptr;
    }

    bool hasProperty = getMaterialPtr()->hasLegacyProperties();
    return PyBool_FromLong(hasProperty ? 1 : 0);
}

Py::Dict MaterialPy::getProperties() const
{
    Py::Dict dict;

    // Maintain backwards compatibility
    dict.setItem(Py::String("CardName"), Py::String(getMaterialPtr()->getName().toStdString()));
    dict.setItem(Py::String("AuthorAndLicense"),
                 Py::String(getMaterialPtr()->getAuthorAndLicense().toStdString()));
    dict.setItem(Py::String("Author"), Py::String(getMaterialPtr()->getAuthor().toStdString()));
    dict.setItem(Py::String("License"), Py::String(getMaterialPtr()->getLicense().toStdString()));
    dict.setItem(Py::String("Name"), Py::String(getMaterialPtr()->getName().toStdString()));
    dict.setItem(Py::String("Description"),
                 Py::String(getMaterialPtr()->getDescription().toStdString()));
    dict.setItem(Py::String("ReferenceSource"),
                 Py::String(getMaterialPtr()->getReference().toStdString()));
    dict.setItem(Py::String("SourceURL"), Py::String(getMaterialPtr()->getURL().toStdString()));

    auto properties = getMaterialPtr()->getPhysicalProperties();
    for (auto& it : properties) {
        QString key = it.first;
        auto materialProperty = it.second;

        if (!materialProperty->isNull()) {
            auto value = materialProperty->getDictionaryString();
            dict.setItem(Py::String(key.toStdString()), Py::String(value.toStdString()));
        }
    }

    properties = getMaterialPtr()->getAppearanceProperties();
    for (auto& it : properties) {
        QString key = it.first;
        auto materialProperty = it.second;

        if (!materialProperty->isNull()) {
            auto value = materialProperty->getDictionaryString();
            dict.setItem(Py::String(key.toStdString()), Py::String(value.toStdString()));
        }
    }

    auto legacy = getMaterialPtr()->getLegacyProperties();
    for (auto& it : legacy) {
        auto key = it.first;
        auto value = it.second;

        if (!value.isEmpty()) {
            dict.setItem(Py::String(key.toStdString()), Py::String(value.toStdString()));
        }
    }

    return dict;
}

Py::Dict MaterialPy::getPhysicalProperties() const
{
    Py::Dict dict;

    auto properties = getMaterialPtr()->getPhysicalProperties();
    for (auto& it : properties) {
        QString key = it.first;
        auto materialProperty = it.second;

        if (!materialProperty->isNull()) {
            auto value = materialProperty->getDictionaryString();
            dict.setItem(Py::String(key.toStdString()), Py::String(value.toStdString()));
        }
    }

    return dict;
}

Py::Dict MaterialPy::getAppearanceProperties() const
{
    Py::Dict dict;

    auto properties = getMaterialPtr()->getAppearanceProperties();
    for (auto& it : properties) {
        QString key = it.first;
        auto materialProperty = it.second;

        if (!materialProperty->isNull()) {
            auto value = materialProperty->getDictionaryString();
            dict.setItem(Py::String(key.toStdString()), Py::String(value.toStdString()));
        }
    }

    return dict;
}

Py::Dict MaterialPy::getLegacyProperties() const
{
    Py::Dict dict;

    auto legacy = getMaterialPtr()->getLegacyProperties();
    for (auto& it : legacy) {
        auto key = it.first;
        auto value = it.second;

        if (!value.isEmpty()) {
            dict.setItem(Py::String(key.toStdString()), Py::String(value.toStdString()));
        }
    }

    return dict;
}

PyObject* MaterialPy::getPhysicalValue(PyObject* args)
{
    char* name;
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return nullptr;
    }

    if (!getMaterialPtr()->hasPhysicalProperty(QString::fromStdString(name))) {
        Py_RETURN_NONE;
    }

    auto property = getMaterialPtr()->getPhysicalProperty(QString::fromStdString(name));
    if (!property) {
        Py_RETURN_NONE;
    }

    if (property->getType() == MaterialValue::Array2D) {
        auto value =
            std::static_pointer_cast<Materials::Array2D>(property->getMaterialValue());
        return new Array2DPy(new Array2D(*value));
    }
    if (property->getType() == MaterialValue::Array3D) {
        auto value =
            std::static_pointer_cast<Materials::Array3D>(property->getMaterialValue());
        return new Array3DPy(new Array3D(*value));
    }

    QVariant value = property->getValue();
    return _pyObjectFromVariant(value);
}

PyObject* MaterialPy::setPhysicalValue(PyObject* args)
{
    char* name;
    char* value;
    if (!PyArg_ParseTuple(args, "ss", &name, &value)) {
        return nullptr;
    }

    getMaterialPtr()->setPhysicalValue(QString::fromStdString(name), QString::fromStdString(value));
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* MaterialPy::getAppearanceValue(PyObject* args)
{
    char* name;
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return nullptr;
    }

    if (!getMaterialPtr()->hasAppearanceProperty(QString::fromStdString(name))) {
        Py_RETURN_NONE;
    }

    auto property = getMaterialPtr()->getAppearanceProperty(QString::fromStdString(name));
    if (!property) {
        Py_RETURN_NONE;
    }

    if (property->getType() == MaterialValue::Array2D) {
        auto value =
            std::static_pointer_cast<Materials::Array2D>(property->getMaterialValue());
        return new Array2DPy(new Array2D(*value));
    }
    if (property->getType() == MaterialValue::Array3D) {
        auto value =
            std::static_pointer_cast<Materials::Array3D>(property->getMaterialValue());
        return new Array3DPy(new Array3D(*value));
    }

    QVariant value = property->getValue();
    return _pyObjectFromVariant(value);
}

PyObject* MaterialPy::setAppearanceValue(PyObject* args)
{
    char* name;
    char* value;
    if (!PyArg_ParseTuple(args, "ss", &name, &value)) {
        return nullptr;
    }

    getMaterialPtr()->setAppearanceValue(QString::fromStdString(name),
                                         QString::fromStdString(value));
    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* MaterialPy::setValue(PyObject* args)
{
    char* name;
    char* value;
    PyObject* listObj;
    PyObject* arrayObj;
    if (PyArg_ParseTuple(args, "ss", &name, &value)) {
        getMaterialPtr()->setValue(QString::fromStdString(name), QString::fromStdString(value));
        Py_Return;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "sO!", &name, &PyList_Type, &listObj)) {
        QList<QVariant> variantList;
        Py::List list(listObj);
        for (auto itemObj : list) {
            Py::String item(itemObj);
            QString value(QString::fromStdString(item.as_string()));
            QVariant variant = QVariant::fromValue(value);
            variantList.append(variant);
        }

        getMaterialPtr()->setValue(QString::fromStdString(name), variantList);
        Py_Return;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "sO!", &name, &(Array2DPy::Type), &arrayObj)) {
        auto array = static_cast<Array2DPy*>(arrayObj);
        auto shared = std::make_shared<Array2D>(*array->getArray2DPtr());

        getMaterialPtr()->setValue(QString::fromStdString(name), shared);
        Py_Return;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "sO!", &name, &(Array3DPy::Type), &arrayObj)) {
        auto array = static_cast<Array3DPy*>(arrayObj);
        auto shared = std::make_shared<Array3D>(*array->getArray3DPtr());

        getMaterialPtr()->setValue(QString::fromStdString(name), shared);
        Py_Return;
    }

    PyErr_SetString(PyExc_TypeError,
                    "Either a string, a list, or an array are expected");
    return nullptr;
}

Py::Dict MaterialPy::getPropertyObjects() const
{
    Py::Dict dict;

    auto properties = getMaterialPtr()->getPhysicalProperties();
    for (auto& it : properties) {
        QString key = it.first;
        auto materialProperty = it.second;

        // if (materialProperty->getType() == MaterialValue::Array2D) {
        //     auto value = std::static_pointer_cast<Materials::Array2D>(
        //         materialProperty->getMaterialValue());
        //     dict.setItem(Py::String(key.toStdString()),
        //                  Py::Object(new Array2DPy(new Array2D(*value)), true));
        // }
        // else if (materialProperty->getType() == MaterialValue::Array3D) {
        //     auto value = std::static_pointer_cast<Materials::Array3D>(
        //         materialProperty->getMaterialValue());
        //     dict.setItem(Py::String(key.toStdString()),
        //                  Py::Object(new Array3DPy(new Array3D(*value)), true));
        // }
        // else {
        dict.setItem(
            Py::String(key.toStdString()),
            Py::Object(new MaterialPropertyPy(new MaterialProperty(materialProperty)), true));
        // }
    }

    properties = getMaterialPtr()->getAppearanceProperties();
    for (auto& it : properties) {
        QString key = it.first;
        auto materialProperty = it.second;
        dict.setItem(
            Py::String(key.toStdString()),
            Py::Object(new MaterialPropertyPy(new MaterialProperty(materialProperty)), true));
    }

    return dict;
}

PyObject* MaterialPy::keys()
{
    return Py::new_reference_to(this->getProperties().keys());
}

PyObject* MaterialPy::values()
{
    return Py::new_reference_to(this->getProperties().values());
}

Py_ssize_t MaterialPy::sequence_length(PyObject *self)
{
    return static_cast<MaterialPy*>(self)->getProperties().size();
}

PyObject* MaterialPy::sequence_item(PyObject* self, Py_ssize_t item)
{
    Py::List keys = static_cast<MaterialPy*>(self)->getProperties().keys();
    return Py::new_reference_to(keys.getItem(item));
}

int MaterialPy::sequence_contains(PyObject* self, PyObject* key)
{
    return PyDict_Contains(static_cast<MaterialPy*>(self)->getProperties().ptr(), key);
}

PyObject* MaterialPy::mapping_subscript(PyObject* self, PyObject* key)
{
    Py::Dict dict = static_cast<MaterialPy*>(self)->getProperties();
    return Py::new_reference_to(dict.getItem(Py::Object(key)));
}
