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

#include "PreCompiled.h"

// inclusion of the generated files (generated out of PythonWorkbenchPy.xml)
#include "PythonWorkbenchPy.h"
#include "PythonWorkbenchPy.cpp"


using namespace Gui;

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
PyObject*  PythonWorkbenchPy::appendMenu(PyObject *args)
{
    PY_TRY {
        PyObject* pPath;
        PyObject* pItems;
        if ( !PyArg_ParseTuple(args, "OO", &pPath, &pItems) )
            return nullptr;

        // menu path
        std::list<std::string> path;
        if (PyList_Check(pPath)) {
            int nDepth = PyList_Size(pPath);
            for (int j=0; j<nDepth;++j) {
                PyObject* item = PyList_GetItem(pPath, j);
                if (PyUnicode_Check(item)) {
                    const char* pItem = PyUnicode_AsUTF8(item);
                    path.emplace_back(pItem);
                } else {
                    continue;
                }
            }
        } else if (PyUnicode_Check(pPath)) {
            const char* pItem = PyUnicode_AsUTF8(pPath);
            path.emplace_back(pItem);
        } else {
            PyErr_SetString(PyExc_AssertionError, "Expected either a string or a stringlist as first argument");
            return nullptr;
        }

        // menu items
        std::list<std::string> items;
        if (PyList_Check(pItems)) {
            int nItems = PyList_Size(pItems);
            for (int i=0; i<nItems;++i) {
                PyObject* item = PyList_GetItem(pItems, i);
                if (PyUnicode_Check(item)) {
                    const char* pItem = PyUnicode_AsUTF8(item);
                    items.emplace_back(pItem);
                } else {
                    continue;
                }
            }
        } else if (PyUnicode_Check(pItems)) {
            const char* pItem = PyUnicode_AsUTF8(pItems);
            items.emplace_back(pItem);
        } else {
            PyErr_SetString(PyExc_AssertionError, "Expected either a string or a stringlist as first argument");
            return nullptr;
        }

        getPythonBaseWorkbenchPtr()->appendMenu( path, items );

        Py_Return;
    } PY_CATCH;
}

/** Removes a menu */
PyObject*  PythonWorkbenchPy::removeMenu(PyObject *args)
{
    PY_TRY {
        char *psMenu;
        if (!PyArg_ParseTuple(args, "s", &psMenu))
            return nullptr;

        getPythonBaseWorkbenchPtr()->removeMenu( psMenu );
        Py_Return;
    } PY_CATCH;
}

/** Appends new context menu items */
PyObject*  PythonWorkbenchPy::appendContextMenu(PyObject *args)
{
    PY_TRY {
        PyObject* pPath;
        PyObject* pItems;
        if ( !PyArg_ParseTuple(args, "OO", &pPath, &pItems) )
            return nullptr;

        // menu path
        std::list<std::string> path;
        if (PyList_Check(pPath)) {
            int nDepth = PyList_Size(pPath);
            for (int j=0; j<nDepth;++j) {
                PyObject* item = PyList_GetItem(pPath, j);
                if (PyUnicode_Check(item)) {
                    const char* pItem = PyUnicode_AsUTF8(item);
                    path.emplace_back(pItem);
                } else {
                    continue;
                }
            }
        } else if (PyUnicode_Check(pPath)) {
            const char* pItem = PyUnicode_AsUTF8(pPath);
            path.emplace_back(pItem);
        } else {
            PyErr_SetString(PyExc_AssertionError, "Expected either a string or a stringlist as first argument");
            return nullptr;
        }

        // menu items
        std::list<std::string> items;
        if (PyList_Check(pItems)) {
            int nItems = PyList_Size(pItems);
            for (int i=0; i<nItems;++i) {
                PyObject* item = PyList_GetItem(pItems, i);
                if (PyUnicode_Check(item)) {
                    const char* pItem = PyUnicode_AsUTF8(item);
                    items.emplace_back(pItem);
                } else {
                    continue;
                }
            }
        } else if (PyUnicode_Check(pItems)) {
            const char* pItem = PyUnicode_AsUTF8(pItems);
            items.emplace_back(pItem);
        } else {
            PyErr_SetString(PyExc_AssertionError, "Expected either a string or a stringlist as first argument");
            return nullptr;
        }

        getPythonBaseWorkbenchPtr()->appendContextMenu( path, items );

        Py_Return;
    } PY_CATCH;
}

/** Removes a context menu */
PyObject*  PythonWorkbenchPy::removeContextMenu(PyObject *args)
{
    PY_TRY {
        char *psMenu;
        if (!PyArg_ParseTuple(args, "s", &psMenu))
            return nullptr;

        getPythonBaseWorkbenchPtr()->removeContextMenu( psMenu );
        Py_Return;
    } PY_CATCH;
}

/** Appends a new toolbar */
PyObject*  PythonWorkbenchPy::appendToolbar(PyObject *args)
{
    PY_TRY {
        PyObject* pObject;
        char* psToolBar;
        if ( !PyArg_ParseTuple(args, "sO", &psToolBar, &pObject) )
            return nullptr;
        if (!PyList_Check(pObject)) {
            PyErr_SetString(PyExc_AssertionError, "Expected a list as second argument");
            return nullptr;
        }

        std::list<std::string> items;
        int nSize = PyList_Size(pObject);
        for (int i=0; i<nSize;++i) {
            PyObject* item = PyList_GetItem(pObject, i);
            if (PyUnicode_Check(item)) {
                const char* pItem = PyUnicode_AsUTF8(item);
                items.emplace_back(pItem);
            } else {
                continue;
            }
        }
        getPythonBaseWorkbenchPtr()->appendToolbar( psToolBar, items );

        Py_Return;
    } PY_CATCH;
}

/** Removes a toolbar */
PyObject*  PythonWorkbenchPy::removeToolbar(PyObject *args)
{
    PY_TRY {
        char *psToolBar;
        if (!PyArg_ParseTuple(args, "s", &psToolBar))
            return nullptr;

        getPythonBaseWorkbenchPtr()->removeToolbar( psToolBar );
        Py_Return;
    } PY_CATCH;
}

/** Appends a new command bar */
PyObject*  PythonWorkbenchPy::appendCommandbar(PyObject *args)
{
    PY_TRY {
        PyObject* pObject;
        char* psToolBar;
        if ( !PyArg_ParseTuple(args, "sO", &psToolBar, &pObject) )
            return nullptr;
        if (!PyList_Check(pObject)) {
            PyErr_SetString(PyExc_AssertionError, "Expected a list as second argument");
            return nullptr;
        }

        std::list<std::string> items;
        int nSize = PyList_Size(pObject);
        for (int i=0; i<nSize;++i) {
            PyObject* item = PyList_GetItem(pObject, i);
            if (PyUnicode_Check(item)) {
                const char* pItem = PyUnicode_AsUTF8(item);
                items.emplace_back(pItem);
            } else {
                continue;
            }
        }

        getPythonBaseWorkbenchPtr()->appendCommandbar( psToolBar, items );

        Py_Return;
    } PY_CATCH;
}

/** Removes a command bar */
PyObject*  PythonWorkbenchPy::removeCommandbar(PyObject *args)
{
    PY_TRY {
        char *psToolBar;
        if (!PyArg_ParseTuple(args, "s", &psToolBar))
            return nullptr;

        getPythonBaseWorkbenchPtr()->removeCommandbar( psToolBar );
        Py_Return;
    } PY_CATCH;
}

PyObject* PythonWorkbenchPy::getCustomAttributes(const char* ) const
{
    return nullptr;
}

int PythonWorkbenchPy::setCustomAttributes(const char* , PyObject *)
{
    return 0;
}

// deprecated methods

PyObject*  PythonWorkbenchPy::AppendMenu(PyObject *args)
{
    return appendMenu(args);
}

PyObject*  PythonWorkbenchPy::RemoveMenu(PyObject *args)
{
    return removeMenu(args);
}

PyObject*  PythonWorkbenchPy::ListMenus(PyObject *args)
{
    return listMenus(args);
}

PyObject*  PythonWorkbenchPy::AppendContextMenu(PyObject *args)
{
    return appendContextMenu(args);
}

PyObject*  PythonWorkbenchPy::RemoveContextMenu(PyObject *args)
{
    return removeContextMenu(args);
}

PyObject*  PythonWorkbenchPy::AppendToolbar(PyObject *args)
{
    return appendToolbar(args);
}

PyObject*  PythonWorkbenchPy::RemoveToolbar(PyObject *args)
{
    return removeToolbar(args);
}

PyObject*  PythonWorkbenchPy::ListToolbars(PyObject *args)
{
    return listToolbars(args);
}

PyObject*  PythonWorkbenchPy::AppendCommandbar(PyObject *args)
{
    return appendCommandbar(args);
}

PyObject*  PythonWorkbenchPy::RemoveCommandbar(PyObject *args)
{
    return removeCommandbar(args);
}

PyObject*  PythonWorkbenchPy::ListCommandbars(PyObject *args)
{
    return listCommandbars(args);
}
