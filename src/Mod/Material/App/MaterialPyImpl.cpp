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

#include "MaterialPy.cpp"

using namespace Materials;

// Forward declaration
static PyObject* _pyObjectFromVariant(const QVariant& value);
static Py::List getList(const QVariant& value);

// returns a string which represents the object e.g. when printed in python
std::string MaterialPy::representation() const
{
    MaterialPy::PointerType ptr = getMaterialPtr();
    std::stringstream str;
    str << "Property [Name=(";
    str << ptr->getName().toStdString();
    str << "), UUID=(";
    str << ptr->getUUID().toStdString();
    auto library = ptr->getLibrary();
    if (library) {
        str << "), Library Name=(";
        str << ptr->getLibrary()->getName().toStdString();
        str << "), Library Root=(";
        str << ptr->getLibrary()->getDirectoryPath().toStdString();
        str << "), Library Icon=(";
        str << ptr->getLibrary()->getIconPath().toStdString();
    }
    str << "), Directory=(";
    str << ptr->getDirectory().toStdString();
    // str << "), URL=(";
    // str << ptr->getURL();
    // str << "), DOI=(";
    // str << ptr->getDOI();
    // str << "), Description=(";
    // str << ptr->getDescription();
    // str << "), Inherits=[";
    // const std::vector<std::string> &inherited = getMaterialPtr()->getInheritance();
    // for (auto it = inherited.begin(); it != inherited.end(); it++)
    // {
    //     std::string uuid = *it;
    //     if (it != inherited.begin())
    //         str << "), UUID=(";
    //     else
    //         str << "UUID=(";
    //     str << uuid << ")";
    // }
    // str << "]]";
    str << ")]";

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
    return Py::String(library ? library->getName().toStdString() : "");
}

Py::String MaterialPy::getLibraryRoot() const
{
    auto library = getMaterialPtr()->getLibrary();
    return Py::String(library ? library->getDirectoryPath().toStdString() : "");
}

Py::String MaterialPy::getLibraryIcon() const
{
    auto library = getMaterialPtr()->getLibrary();
    return Py::String(library ? library->getIconPath().toStdString() : "");
}

Py::String MaterialPy::getName() const
{
    return Py::String(getMaterialPtr()->getName().toStdString());
}

Py::String MaterialPy::getDirectory() const
{
    return Py::String(getMaterialPtr()->getDirectory().toStdString());
}

Py::String MaterialPy::getUUID() const
{
    return Py::String(getMaterialPtr()->getUUID().toStdString());
}

Py::String MaterialPy::getDescription() const
{
    return Py::String(getMaterialPtr()->getDescription().toStdString());
}

Py::String MaterialPy::getURL() const
{
    return Py::String(getMaterialPtr()->getURL().toStdString());
}

Py::String MaterialPy::getReference() const
{
    return Py::String(getMaterialPtr()->getReference().toStdString());
}

Py::String MaterialPy::getParent() const
{
    return Py::String(getMaterialPtr()->getParentUUID().toStdString());
}

Py::String MaterialPy::getAuthorAndLicense() const
{
    return Py::String(getMaterialPtr()->getAuthorAndLicense().toStdString());
}

Py::String MaterialPy::getAuthor() const
{
    return Py::String(getMaterialPtr()->getAuthor().toStdString());
}

Py::String MaterialPy::getLicense() const
{
    return Py::String(getMaterialPtr()->getLicense().toStdString());
}

Py::List MaterialPy::getPhysicalModels() const
{
    auto models = getMaterialPtr()->getPhysicalModels();
    Py::List list;

    for (auto it = models->begin(); it != models->end(); it++) {
        list.append(Py::String(it->toStdString()));
    }

    return list;
}

Py::List MaterialPy::getAppearanceModels() const
{
    auto models = getMaterialPtr()->getAppearanceModels();
    Py::List list;

    for (auto it = models->begin(); it != models->end(); it++) {
        list.append(Py::String(it->toStdString()));
    }

    return list;
}

Py::List MaterialPy::getTags() const
{
    auto& tags = getMaterialPtr()->getTags();
    Py::List list;

    for (auto it = tags.begin(); it != tags.end(); it++) {
        list.append(Py::String(it->toStdString()));
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

PyObject* MaterialPy::hasPhysicalModel(PyObject* args)
{
    char* uuid;
    if (!PyArg_ParseTuple(args, "s", &uuid)) {
        return nullptr;
    }

    bool hasProperty = getMaterialPtr()->hasPhysicalModel(QString::fromStdString(uuid));
    return PyBool_FromLong(hasProperty ? 1 : 0);
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
    for (auto it = properties.begin(); it != properties.end(); it++) {
        QString key = it->first;
        auto materialProperty = it->second;

        if (!materialProperty->isNull()) {
            auto value = materialProperty->getDictionaryString();
            dict.setItem(Py::String(key.toStdString()), Py::String(value.toStdString()));
        }
    }

    properties = getMaterialPtr()->getAppearanceProperties();
    for (auto it = properties.begin(); it != properties.end(); it++) {
        QString key = it->first;
        auto materialProperty = it->second;

        if (!materialProperty->isNull()) {
            auto value = materialProperty->getDictionaryString();
            dict.setItem(Py::String(key.toStdString()), Py::String(value.toStdString()));
        }
    }

    return dict;
}

Py::Dict MaterialPy::getPhysicalProperties() const
{
    Py::Dict dict;

    auto properties = getMaterialPtr()->getPhysicalProperties();
    for (auto it = properties.begin(); it != properties.end(); it++) {
        QString key = it->first;
        auto materialProperty = it->second;

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
    for (auto it = properties.begin(); it != properties.end(); it++) {
        QString key = it->first;
        auto materialProperty = it->second;

        if (!materialProperty->isNull()) {
            auto value = materialProperty->getDictionaryString();
            dict.setItem(Py::String(key.toStdString()), Py::String(value.toStdString()));
        }
    }

    return dict;
}

static Py::List getList(const QVariant& value)
{
    auto listValue = value.value<QList<QVariant>>();
    Py::List list;

    for (auto& it : listValue) {
        list.append(Py::Object(_pyObjectFromVariant(it)));
    }

    return list;
}

static PyObject* _pyObjectFromVariant(const QVariant& value)
{
    if (value.isNull()) {
        Py_RETURN_NONE;
    }

    if (value.userType() == qMetaTypeId<Base::Quantity>()) {
        return new Base::QuantityPy(new Base::Quantity(value.value<Base::Quantity>()));
    }
    if (value.userType() == QMetaType::Double) {
        return PyFloat_FromDouble(value.toDouble());
    }
    if (value.userType() == QMetaType::Float) {
        return PyFloat_FromDouble(value.toFloat());
    }
    if (value.userType() == QMetaType::Int) {
        return PyLong_FromLong(value.toInt());
    }
    if (value.userType() == QMetaType::Long) {
        return PyLong_FromLong(value.toInt());
    }
    if (value.userType() == QMetaType::Bool) {
        return Py::new_reference_to(Py::Boolean(value.toBool()));
    }
    if (value.userType() == QMetaType::QString) {
        return PyUnicode_FromString(value.toString().toStdString().c_str());
    }
    if (value.userType() == qMetaTypeId<QList<QVariant>>()) {
        return Py::new_reference_to(getList(value));
    }

    throw UnknownValueType();
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
            std::static_pointer_cast<Materials::Material2DArray>(property->getMaterialValue());
        return new Array2DPy(new Material2DArray(*value));
    }
    else if (property->getType() == MaterialValue::Array3D) {
        auto value =
            std::static_pointer_cast<Materials::Material3DArray>(property->getMaterialValue());
        return new Array3DPy(new Material3DArray(*value));
    }

    QVariant value = property->getValue();
    return _pyObjectFromVariant(value);
}

PyObject* MaterialPy::getAppearanceValue(PyObject* args)
{
    char* name;
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return nullptr;
    }

    QVariant value = getMaterialPtr()->getAppearanceValue(QString::fromStdString(name));
    return _pyObjectFromVariant(value);
}
