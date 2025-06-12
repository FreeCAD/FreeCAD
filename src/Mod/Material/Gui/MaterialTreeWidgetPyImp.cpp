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

#include <Gui/PythonWrapper.h>

#include "MaterialTreeWidget.h"
#include "MaterialTreeWidgetPy.h"

#include "MaterialTreeWidgetPy.cpp"

using namespace MatGui;

// returns a string which represents the object e.g. when printed in python
std::string MaterialTreeWidgetPy::representation() const
{
    std::ostringstream str;
    str << "<MaterialTreeWidget at " << getMaterialTreeWidgetPtr() << ">";
    return str.str();
}

PyObject* MaterialTreeWidgetPy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    // never create such objects with the constructor
    return new MaterialTreeWidgetPy(new MaterialTreeWidget());
}

// constructor method
int MaterialTreeWidgetPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    PyObject* obj {};
    if (PyArg_ParseTuple(args, "")) {
        return 0;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!", &(MatGui::MaterialTreeWidgetPy::Type), &obj)) {
        auto widget = static_cast<MatGui::MaterialTreeWidgetPy*>(obj)->getMaterialTreeWidgetPtr();
        _pcTwinPointer = widget;
        return 0;
    }

    // PyErr_Clear();
    // if (PyArg_ParseTuple(args, "O!", &(QWidget::Type), &obj)) {
    //     auto widget = static_cast<MatGui::MaterialTreeWidget*>(obj);
    //     _pcTwinPointer = widget;
    //     return 0;
    // }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O", &obj)) {
        if ((QLatin1String(obj->ob_type->tp_name) == QLatin1String("PySide2.QtWidgets.QWidget")) ||
            (QLatin1String(obj->ob_type->tp_name) == QLatin1String("PySide6.QtWidgets.QWidget"))) {
            Gui::PythonWrapper wrap;
            wrap.loadWidgetsModule();
            auto qObject = wrap.toQObject(Py::Object(obj));
            auto widget = static_cast<MatGui::MaterialTreeWidget*>(qObject);
            _pcTwinPointer = widget;
            return 0;
        }
        else {
            PyErr_Format(PyExc_TypeError,
                         "empty parameter list, or MaterialTreeWidget expected not '%s'",
                         obj->ob_type->tp_name);
            return -1;
        }
    }

    PyErr_SetString(PyExc_TypeError, "empty parameter list, or MaterialTreeWidget expected");
    // PyErr_Format(PyExc_TypeError,
    //              "empty parameter list, or MaterialTreeWidget expected not '%s'",
    //              obj->ob_type->tp_name);
    return -1;
}

Py::String MaterialTreeWidgetPy::getUUID() const
{
    return Py::String(getMaterialTreeWidgetPtr()->getMaterialUUID().toStdString());
}

void MaterialTreeWidgetPy::setUUID(const Py::String value)
{
    getMaterialTreeWidgetPtr()->setMaterial(QString::fromStdString(value));
}

Py::Boolean MaterialTreeWidgetPy::getexpanded() const
{
    return {getMaterialTreeWidgetPtr()->getExpanded()};
}

void MaterialTreeWidgetPy::setexpanded(const Py::Boolean value)
{
    getMaterialTreeWidgetPtr()->setExpanded(value.isTrue());
}

Py::Boolean MaterialTreeWidgetPy::getIncludeFavorites() const
{
    return {getMaterialTreeWidgetPtr()->includeFavorites()};
}

void MaterialTreeWidgetPy::setIncludeFavorites(const Py::Boolean value)
{
    getMaterialTreeWidgetPtr()->setIncludeFavorites(value.isTrue());
}

Py::Boolean MaterialTreeWidgetPy::getIncludeRecent() const
{
    return {getMaterialTreeWidgetPtr()->includeRecent()};
}

void MaterialTreeWidgetPy::setIncludeRecent(const Py::Boolean value)
{
    getMaterialTreeWidgetPtr()->setIncludeRecent(value.isTrue());
}

Py::Boolean MaterialTreeWidgetPy::getIncludeEmptyFolders() const
{
    return {getMaterialTreeWidgetPtr()->includeEmptyFolders()};
}

void MaterialTreeWidgetPy::setIncludeEmptyFolders(const Py::Boolean value)
{
    getMaterialTreeWidgetPtr()->setIncludeEmptyFolders(value.isTrue());
}

Py::Boolean MaterialTreeWidgetPy::getIncludeEmptyLibraries() const
{
    return {getMaterialTreeWidgetPtr()->includeEmptyLibraries()};
}

void MaterialTreeWidgetPy::setIncludeEmptyLibraries(const Py::Boolean value)
{
    getMaterialTreeWidgetPtr()->setIncludeEmptyLibraries(value.isTrue());
}

Py::Boolean MaterialTreeWidgetPy::getIncludeLegacy() const
{
    return {getMaterialTreeWidgetPtr()->includeLegacy()};
}

void MaterialTreeWidgetPy::setIncludeLegacy(const Py::Boolean value)
{
    getMaterialTreeWidgetPtr()->setIncludeLegacy(value.isTrue());
}

PyObject* MaterialTreeWidgetPy::setFilter(PyObject* args)
{
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return nullptr;
    }
    if (PyObject_TypeCheck(obj, &(Materials::MaterialFilterPy::Type))) {
        auto filter = static_cast<Materials::MaterialFilterPy*>(obj)->getMaterialFilterPtr();
        getMaterialTreeWidgetPtr()->setFilter(*filter);
    }
    else if (PyList_Check(obj)) {
        // The argument is a list of filters
        Py_ssize_t n = PyList_Size(obj);
        if (n < 0) {
            Py_Return;
        }
        PyObject* item;
        auto filterList = std::make_shared<std::list<std::shared_ptr<Materials::MaterialFilter>>>();
        for (int i = 0; i < n; i++) {
            item = PyList_GetItem(obj, i);
            if (PyObject_TypeCheck(item, &(Materials::MaterialFilterPy::Type))) {
                auto filter =
                    static_cast<Materials::MaterialFilterPy*>(item)->getMaterialFilterPtr();
                auto filterPtr = std::make_shared<Materials::MaterialFilter>(*filter);
                filterList->push_back(filterPtr);
            }
            else {
                PyErr_Format(PyExc_TypeError,
                             "List entry must be of type 'MaterialFilter' not '%s'",
                             obj->ob_type->tp_name);
                return nullptr;
            }
        }
        getMaterialTreeWidgetPtr()->setFilter(filterList);
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "Type must be 'MaterialFilter' or list of 'MaterialFilter' not '%s'",
                     obj->ob_type->tp_name);
        return nullptr;
    }

    Py_Return;
}

PyObject* MaterialTreeWidgetPy::selectFilter(PyObject* args)
{
    char* name;
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return nullptr;
    }

    Py_Return;
}

PyObject* MaterialTreeWidgetPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int MaterialTreeWidgetPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
