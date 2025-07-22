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

#include "Workbench.h"
#include "WorkbenchManager.h"

// inclusion of the generated files (generated out of WorkbenchPy.xml)
#include "WorkbenchPy.h"
#include "WorkbenchPy.cpp"

using namespace Gui;

/** @class WorkbenchPy
 * The workbench Python base class doesn't allow you to manipulate the C++ workbench class behind.
 * You're only allowed either to activate the workbench class or get its name.
 * The WorkbenchPy class is associated to all C++ workbench classes except of PythonWorkbench.
 * @see Workbench
 * @see PythonWorkbench
 * @see PythonWorkbenchPy
 * @author Werner Mayer
 */

// returns a string which represent the object e.g. when printed in python
std::string WorkbenchPy::representation() const
{
    return {"<Workbench object>"};
}

/** Retrieves the workbench name */
PyObject*  WorkbenchPy::name(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    PY_TRY {
        Py::String name(getWorkbenchPtr()->name());
        return Py::new_reference_to(name);
    }PY_CATCH;
}

/** Activates the workbench object */
PyObject*  WorkbenchPy::activate(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    PY_TRY {
        std::string name = getWorkbenchPtr()->name();
        WorkbenchManager::instance()->activate( name, getWorkbenchPtr()->getTypeId().getName() );
        Py_Return;
    }PY_CATCH;
}

/** Shows a list of all menus */
PyObject* WorkbenchPy::listMenus(PyObject *args)
{
    PY_TRY {
        if (!PyArg_ParseTuple(args, ""))
            return nullptr;

        std::list<std::string> menus = getWorkbenchPtr()->listMenus();

        Py::List list;
        for (const auto & menu : menus) {
            list.append(Py::String(menu));
        }
        return Py::new_reference_to(list);
    } PY_CATCH;
}

/** Shows a list of all toolbars */
PyObject* WorkbenchPy::listToolbars(PyObject *args)
{
    PY_TRY {
        if (!PyArg_ParseTuple(args, ""))
            return nullptr;

        std::list<std::string> bars = getWorkbenchPtr()->listToolbars();

        Py::List list;
        for (const auto & bar : bars) {
            list.append(Py::String(bar));
        }
        return Py::new_reference_to(list);
    } PY_CATCH;
}

/** Shows a dict of all toolbars and their commands*/
PyObject* WorkbenchPy::getToolbarItems(PyObject *args)
{
    PY_TRY {
        if (!PyArg_ParseTuple(args, ""))
            return nullptr;

        std::list<std::pair<std::string, std::list<std::string>>> bars = getWorkbenchPtr()->getToolbarItems();

        Py::Dict dict;
        for (const auto& it : bars) {
            Py::List list;
            for (const auto& jt : it.second) {
                list.append(Py::String(jt));
            }
            dict.setItem(it.first, list);
        }
        return Py::new_reference_to(dict);
    } PY_CATCH;
}

/** Shows a list of all command bars */
PyObject* WorkbenchPy::listCommandbars(PyObject *args)
{
    PY_TRY {
        if (!PyArg_ParseTuple(args, ""))
            return nullptr;

        std::list<std::string> bars = getWorkbenchPtr()->listCommandbars();

        Py::List list;
        for (const auto & bar : bars) {
            list.append(Py::String(bar));
        }
        return Py::new_reference_to(list);
    } PY_CATCH;
}

/** Reload the workbench */
PyObject*  WorkbenchPy::reloadActive(PyObject *args)
{
    PY_TRY {
        if (!PyArg_ParseTuple(args, ""))
            return nullptr;

        Workbench* active = Gui::WorkbenchManager::instance()->active();
        if (active)
            active->activate();
        Py_Return;
    } PY_CATCH;
}

PyObject* WorkbenchPy::getCustomAttributes(const char*) const
{
    return nullptr;
}

int WorkbenchPy::setCustomAttributes(const char*, PyObject *)
{
    return 0;
}
