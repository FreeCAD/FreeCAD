/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

// generated out of PythonWorkbench.pyi
#include "PythonWorkbenchPy.h"
#include "PythonWorkbenchPy.cpp"

#include <array>

#include <Base/PyWrapParseTupleAndKeywords.h>

using namespace Gui;

namespace
{
bool isValidToolbarScopeValue(long value)
{
    switch (static_cast<ToolBarManager::Scope>(value)) {
        case ToolBarManager::Scope::Legacy:
        case ToolBarManager::Scope::Shared:
        case ToolBarManager::Scope::Workbench:
        case ToolBarManager::Scope::Contextual:
            return true;
    }

    return false;
}

bool isValidToolbarTierValue(long value)
{
    switch (static_cast<ToolBarItem::Tier>(value)) {
        case ToolBarItem::Tier::Recommended:
        case ToolBarItem::Tier::Secondary:
        case ToolBarItem::Tier::Advanced:
        case ToolBarItem::Tier::Contextual:
            return true;
    }

    return false;
}

bool isValidToolbarVisibilityValue(long value)
{
    switch (static_cast<ToolBarItem::DefaultVisibility>(value)) {
        case ToolBarItem::DefaultVisibility::Visible:
        case ToolBarItem::DefaultVisibility::Hidden:
        case ToolBarItem::DefaultVisibility::Unavailable:
            return true;
    }

    return false;
}

bool parseStringList(PyObject* object, std::list<std::string>& items, const char* errorMessage)
{
    if (PyList_Check(object)) {
        int nItems = PyList_Size(object);
        for (int i = 0; i < nItems; ++i) {
            PyObject* item = PyList_GetItem(object, i);
            if (PyUnicode_Check(item)) {
                const char* pItem = PyUnicode_AsUTF8(item);
                items.emplace_back(pItem);
            }
        }
        return true;
    }

    if (PyUnicode_Check(object)) {
        const char* pItem = PyUnicode_AsUTF8(object);
        items.emplace_back(pItem);
        return true;
    }

    PyErr_SetString(PyExc_TypeError, errorMessage);
    return false;
}

PyObject* getOptionalAttr(PyObject* object, const char* name)
{
    if (!object || object == Py_None) {
        return nullptr;
    }

    PyObject* attr = PyObject_GetAttrString(object, name);
    if (!attr) {
        if (PyErr_ExceptionMatches(PyExc_AttributeError)) {
            PyErr_Clear();
        }
        return nullptr;
    }

    return attr;
}

template<typename Validator>
bool parseEnumValue(PyObject* object, const char* name, Validator validator, int* value, bool* present)
{
    PyObject* attr = getOptionalAttr(object, name);
    if (!attr) {
        if (PyErr_Occurred()) {
            return false;
        }
        if (present) {
            *present = false;
        }
        return true;
    }

    if (attr == Py_None) {
        Py_DECREF(attr);
        if (present) {
            *present = false;
        }
        return true;
    }

    PyObject* number = PyNumber_Long(attr);
    Py_DECREF(attr);
    if (!number) {
        PyErr_Format(PyExc_TypeError, "Expected %s to be an integer enum value", name);
        return false;
    }

    const long parsedValue = PyLong_AsLong(number);
    Py_DECREF(number);
    if (PyErr_Occurred()) {
        return false;
    }
    if (!validator(parsedValue)) {
        PyErr_Format(PyExc_ValueError, "Invalid %s enum value", name);
        return false;
    }

    if (present) {
        *present = true;
    }
    if (value) {
        *value = static_cast<int>(parsedValue);
    }
    return true;
}

bool parseUnicodeValue(PyObject* object, const char* name, QString* value)
{
    PyObject* attr = getOptionalAttr(object, name);
    if (!attr) {
        if (PyErr_Occurred()) {
            return false;
        }
        return true;
    }

    if (attr == Py_None) {
        Py_DECREF(attr);
        return true;
    }

    if (!PyUnicode_Check(attr)) {
        Py_DECREF(attr);
        PyErr_Format(PyExc_TypeError, "Expected %s to be a string", name);
        return false;
    }

    if (value) {
        *value = QString::fromUtf8(PyUnicode_AsUTF8(attr));
    }
    Py_DECREF(attr);
    return true;
}

bool parseToolbarScopeId(PyObject* object, std::optional<ToolBarManager::ToolbarScopeId>* scopeId)
{
    PyObject* scopeObject = getOptionalAttr(object, "scope");
    if (!scopeObject) {
        return !PyErr_Occurred();
    }

    if (scopeObject == Py_None) {
        Py_DECREF(scopeObject);
        return true;
    }

    int scopeValue = 0;
    bool hasScopeValue = false;
    if (!parseEnumValue(scopeObject, "scope", isValidToolbarScopeValue, &scopeValue, &hasScopeValue)) {
        Py_DECREF(scopeObject);
        return false;
    }
    if (!hasScopeValue) {
        Py_DECREF(scopeObject);
        PyErr_SetString(PyExc_ValueError, "ToolbarScopeId.scope is required");
        return false;
    }

    QString workbench;
    if (!parseUnicodeValue(scopeObject, "workbench", &workbench)) {
        Py_DECREF(scopeObject);
        return false;
    }

    QString context;
    if (!parseUnicodeValue(scopeObject, "context", &context)) {
        Py_DECREF(scopeObject);
        return false;
    }

    if (scopeId) {
        *scopeId = ToolBarManager::ToolbarScopeId {
            static_cast<ToolBarManager::Scope>(scopeValue),
            workbench,
            context
        };
    }

    Py_DECREF(scopeObject);
    return true;
}

bool parseToolbarOptions(
    PythonWorkbenchPy* self,
    PyObject* optionsObject,
    PythonBaseWorkbench::ToolBarOptions* options
)
{
    if (!optionsObject || optionsObject == Py_None) {
        return true;
    }

    QString toolbarId;
    if (!parseUnicodeValue(optionsObject, "id", &toolbarId)) {
        return false;
    }

    int tierValue = 0;
    bool hasTier = false;
    if (!parseEnumValue(optionsObject, "tier", isValidToolbarTierValue, &tierValue, &hasTier)) {
        return false;
    }
    if (hasTier) {
        options->tier = static_cast<ToolBarItem::Tier>(tierValue);
    }

    int visibilityValue = 0;
    bool hasVisibility = false;
    if (!parseEnumValue(
            optionsObject,
            "visibility",
            isValidToolbarVisibilityValue,
            &visibilityValue,
            &hasVisibility
        )) {
        return false;
    }
    if (hasVisibility) {
        options->visibility = static_cast<ToolBarItem::DefaultVisibility>(visibilityValue);
    }

    std::optional<ToolBarManager::ToolbarScopeId> scopeId;
    if (!parseToolbarScopeId(optionsObject, &scopeId)) {
        return false;
    }

    if (!toolbarId.isEmpty()) {
        if (scopeId) {
            options->persistenceId = ToolBarManager::PersistenceId(*scopeId, std::move(toolbarId));
        }
        else {
            options->persistenceId = ToolBarManager::PersistenceId(
                ToolBarManager::Scope::Workbench,
                std::move(toolbarId),
                QString::fromStdString(self->getPythonBaseWorkbenchPtr()->name())
            );
        }

        if (ToolBarManager::makeToolBarPersistenceKey(*options->persistenceId).isEmpty()) {
            PyErr_SetString(PyExc_ValueError, "Invalid ToolbarOptions id/scope combination");
            return false;
        }
    }
    else if (scopeId) {
        PyErr_SetString(
            PyExc_ValueError,
            "ToolbarOptions.scope requires ToolbarOptions.id to define a stable toolbar identity"
        );
        return false;
    }

    return true;
}
}  // namespace

/** @class PythonWorkbenchPy
 * The workbench Python class provides additional methods for manipulation of python
 * workbench objects.
 * From the view of Python PythonWorkbenchPy is also derived from WorkbenchPy as in C++.
 * @see Workbench
 * @see WorkbenchPy
 * @see PythonWorkbench
 * @author Werner Mayer
 */

// returns a string which represent the object e.g. when printed in python
std::string PythonWorkbenchPy::representation() const
{
    return {"<Workbench object>"};
}

/** Appends a new menu */
PyObject* PythonWorkbenchPy::appendMenu(PyObject* args)
{
    PY_TRY
    {
        PyObject* pPath;
        PyObject* pItems;
        if (!PyArg_ParseTuple(args, "OO", &pPath, &pItems)) {
            return nullptr;
        }

        // menu path
        std::list<std::string> path;
        if (PyList_Check(pPath)) {
            int nDepth = PyList_Size(pPath);
            for (int j = 0; j < nDepth; ++j) {
                PyObject* item = PyList_GetItem(pPath, j);
                if (PyUnicode_Check(item)) {
                    const char* pItem = PyUnicode_AsUTF8(item);
                    path.emplace_back(pItem);
                }
                else {
                    continue;
                }
            }
        }
        else if (PyUnicode_Check(pPath)) {
            const char* pItem = PyUnicode_AsUTF8(pPath);
            path.emplace_back(pItem);
        }
        else {
            PyErr_SetString(
                PyExc_AssertionError,
                "Expected either a string or a stringlist as first argument"
            );
            return nullptr;
        }

        // menu items
        std::list<std::string> items;
        if (PyList_Check(pItems)) {
            int nItems = PyList_Size(pItems);
            for (int i = 0; i < nItems; ++i) {
                PyObject* item = PyList_GetItem(pItems, i);
                if (PyUnicode_Check(item)) {
                    const char* pItem = PyUnicode_AsUTF8(item);
                    items.emplace_back(pItem);
                }
                else {
                    continue;
                }
            }
        }
        else if (PyUnicode_Check(pItems)) {
            const char* pItem = PyUnicode_AsUTF8(pItems);
            items.emplace_back(pItem);
        }
        else {
            PyErr_SetString(
                PyExc_AssertionError,
                "Expected either a string or a stringlist as first argument"
            );
            return nullptr;
        }

        getPythonBaseWorkbenchPtr()->appendMenu(path, items);

        Py_Return;
    }
    PY_CATCH;
}

/** Removes a menu */
PyObject* PythonWorkbenchPy::removeMenu(PyObject* args)
{
    PY_TRY
    {
        char* psMenu;
        if (!PyArg_ParseTuple(args, "s", &psMenu)) {
            return nullptr;
        }

        getPythonBaseWorkbenchPtr()->removeMenu(psMenu);
        Py_Return;
    }
    PY_CATCH;
}

/** Appends new context menu items */
PyObject* PythonWorkbenchPy::appendContextMenu(PyObject* args)
{
    PY_TRY
    {
        PyObject* pPath;
        PyObject* pItems;
        if (!PyArg_ParseTuple(args, "OO", &pPath, &pItems)) {
            return nullptr;
        }

        // menu path
        std::list<std::string> path;
        if (PyList_Check(pPath)) {
            int nDepth = PyList_Size(pPath);
            for (int j = 0; j < nDepth; ++j) {
                PyObject* item = PyList_GetItem(pPath, j);
                if (PyUnicode_Check(item)) {
                    const char* pItem = PyUnicode_AsUTF8(item);
                    path.emplace_back(pItem);
                }
                else {
                    continue;
                }
            }
        }
        else if (PyUnicode_Check(pPath)) {
            const char* pItem = PyUnicode_AsUTF8(pPath);
            path.emplace_back(pItem);
        }
        else {
            PyErr_SetString(
                PyExc_AssertionError,
                "Expected either a string or a stringlist as first argument"
            );
            return nullptr;
        }

        // menu items
        std::list<std::string> items;
        if (PyList_Check(pItems)) {
            int nItems = PyList_Size(pItems);
            for (int i = 0; i < nItems; ++i) {
                PyObject* item = PyList_GetItem(pItems, i);
                if (PyUnicode_Check(item)) {
                    const char* pItem = PyUnicode_AsUTF8(item);
                    items.emplace_back(pItem);
                }
                else {
                    continue;
                }
            }
        }
        else if (PyUnicode_Check(pItems)) {
            const char* pItem = PyUnicode_AsUTF8(pItems);
            items.emplace_back(pItem);
        }
        else {
            PyErr_SetString(
                PyExc_AssertionError,
                "Expected either a string or a stringlist as first argument"
            );
            return nullptr;
        }

        getPythonBaseWorkbenchPtr()->appendContextMenu(path, items);

        Py_Return;
    }
    PY_CATCH;
}

/** Removes a context menu */
PyObject* PythonWorkbenchPy::removeContextMenu(PyObject* args)
{
    PY_TRY
    {
        char* psMenu;
        if (!PyArg_ParseTuple(args, "s", &psMenu)) {
            return nullptr;
        }

        getPythonBaseWorkbenchPtr()->removeContextMenu(psMenu);
        Py_Return;
    }
    PY_CATCH;
}

/** Appends a new toolbar */
PyObject* PythonWorkbenchPy::appendToolbar(PyObject* args, PyObject* kwd)
{
    PY_TRY
    {
        PyObject* pObject;
        PyObject* pOptions = Py_None;
        char* psToolBar;
        static const std::array<const char*, 4> kwlist {"name", "cmds", "options", nullptr};
        if (
            !Base::Wrapped_ParseTupleAndKeywords(args, kwd, "sO|O", kwlist, &psToolBar, &pObject, &pOptions)
        ) {
            return nullptr;
        }

        std::list<std::string> items;
        if (!parseStringList(pObject, items, "Expected a list or string as second argument")) {
            return nullptr;
        }

        PythonBaseWorkbench::ToolBarOptions options;
        if (!parseToolbarOptions(this, pOptions, &options)) {
            return nullptr;
        }

        getPythonBaseWorkbenchPtr()->appendToolbar(psToolBar, items, options);

        Py_Return;
    }
    PY_CATCH;
}

/** Removes a toolbar */
PyObject* PythonWorkbenchPy::removeToolbar(PyObject* args)
{
    PY_TRY
    {
        char* psToolBar;
        if (!PyArg_ParseTuple(args, "s", &psToolBar)) {
            return nullptr;
        }

        getPythonBaseWorkbenchPtr()->removeToolbar(psToolBar);
        Py_Return;
    }
    PY_CATCH;
}

/** Appends a new command bar */
PyObject* PythonWorkbenchPy::appendCommandbar(PyObject* args)
{
    PY_TRY
    {
        PyObject* pObject;
        char* psToolBar;
        if (!PyArg_ParseTuple(args, "sO", &psToolBar, &pObject)) {
            return nullptr;
        }
        if (!PyList_Check(pObject)) {
            PyErr_SetString(PyExc_AssertionError, "Expected a list as second argument");
            return nullptr;
        }

        std::list<std::string> items;
        int nSize = PyList_Size(pObject);
        for (int i = 0; i < nSize; ++i) {
            PyObject* item = PyList_GetItem(pObject, i);
            if (PyUnicode_Check(item)) {
                const char* pItem = PyUnicode_AsUTF8(item);
                items.emplace_back(pItem);
            }
            else {
                continue;
            }
        }

        getPythonBaseWorkbenchPtr()->appendCommandbar(psToolBar, items);

        Py_Return;
    }
    PY_CATCH;
}

/** Removes a command bar */
PyObject* PythonWorkbenchPy::removeCommandbar(PyObject* args)
{
    PY_TRY
    {
        char* psToolBar;
        if (!PyArg_ParseTuple(args, "s", &psToolBar)) {
            return nullptr;
        }

        getPythonBaseWorkbenchPtr()->removeCommandbar(psToolBar);
        Py_Return;
    }
    PY_CATCH;
}

PyObject* PythonWorkbenchPy::getCustomAttributes(const char*) const
{
    return nullptr;
}

int PythonWorkbenchPy::setCustomAttributes(const char*, PyObject*)
{
    return 0;
}

// deprecated methods

PyObject* PythonWorkbenchPy::AppendMenu(PyObject* args)
{
    return appendMenu(args);
}

PyObject* PythonWorkbenchPy::RemoveMenu(PyObject* args)
{
    return removeMenu(args);
}

PyObject* PythonWorkbenchPy::ListMenus(PyObject* args)
{
    return listMenus(args);
}

PyObject* PythonWorkbenchPy::AppendContextMenu(PyObject* args)
{
    return appendContextMenu(args);
}

PyObject* PythonWorkbenchPy::RemoveContextMenu(PyObject* args)
{
    return removeContextMenu(args);
}

PyObject* PythonWorkbenchPy::AppendToolbar(PyObject* args)
{
    return appendToolbar(args, nullptr);
}

PyObject* PythonWorkbenchPy::RemoveToolbar(PyObject* args)
{
    return removeToolbar(args);
}

PyObject* PythonWorkbenchPy::ListToolbars(PyObject* args)
{
    return listToolbars(args);
}

PyObject* PythonWorkbenchPy::AppendCommandbar(PyObject* args)
{
    return appendCommandbar(args);
}

PyObject* PythonWorkbenchPy::RemoveCommandbar(PyObject* args)
{
    return removeCommandbar(args);
}

PyObject* PythonWorkbenchPy::ListCommandbars(PyObject* args)
{
    return listCommandbars(args);
}
