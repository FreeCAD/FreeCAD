// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include "WorkbenchManipulatorPython.h"
#include "MenuManager.h"
#include "ToolBarManager.h"
#include <Base/Console.h>
#include <Base/Interpreter.h>

FC_LOG_LEVEL_INIT("WorkbenchManipulatorPython", true, true)

using namespace Gui;

void WorkbenchManipulatorPython::installManipulator(const Py::Object& obj)
{
    auto manip = std::make_shared<WorkbenchManipulatorPython>(obj);
    WorkbenchManipulator::installManipulator(manip);
}

void WorkbenchManipulatorPython::removeManipulator(const Py::Object& obj)
{
    auto manip = getManipulators();
    for (const auto& it : manip) {
        auto ptr = std::dynamic_pointer_cast<WorkbenchManipulatorPython>(it);
        if (ptr && ptr->object == obj) {
            WorkbenchManipulator::removeManipulator(ptr);
            break;
        }
    }
}

WorkbenchManipulatorPython::WorkbenchManipulatorPython(const Py::Object& obj)
    : object(obj)
{
}

WorkbenchManipulatorPython::~WorkbenchManipulatorPython()
{
    Base::PyGILStateLocker lock;
    object = Py::None();
}

/*!
 * \brief WorkbenchManipulatorPython::modifyMenuBar
 * \param menuBar
 * The Python manipulator can be implemented as
 * \code
 * class Manipulator:
 *   def modifyMenuBar(self):
 *     return [{"remove" : "Std_Quit"},
 *             {"append" : "Std_About", "menuItem" : "Std_DlgMacroRecord"},
 *             {"insert" : "Std_About", "menuItem" : "Std_DlgParameter"}
 *             {"insert" : "Std_Windows", "menuItem" : "Std_DlgParameter", "after" : ""}]
 *
 * manip = Manipulator()
 * Gui.addWorkbenchManipulator(manip)
 * \endcode
 * This manipulator removes the Std_Quit command, appends the Std_About command
 * to the Macro menu, inserts it to the Tools menu before the Std_DlgParameter
 * and adds the Std_Windows after the Std_DlgParameter command.
 */
void WorkbenchManipulatorPython::modifyMenuBar(MenuItem* menuBar)
{
    Base::PyGILStateLocker lock;
    try {
        tryModifyMenuBar(menuBar);
    }
    catch (Py::Exception&) {
        Base::PyException exc; // extract the Python error text
        exc.reportException();
    }
}

void WorkbenchManipulatorPython::tryModifyMenuBar(MenuItem* menuBar)
{
    if (object.hasAttr(std::string("modifyMenuBar"))) {
        Py::Callable method(object.getAttr(std::string("modifyMenuBar")));
        Py::Tuple args;
        Py::Object result = method.apply(args);
        if (result.isDict()) {
            tryModifyMenuBar(Py::Dict(result), menuBar);
        }
        else if (result.isSequence()) {
            Py::Sequence list(result);
            for (const auto& it : list) {
                if (it.isDict()) {
                    tryModifyMenuBar(Py::Dict(it), menuBar);
                }
            }
        }
    }
}

// NOLINTNEXTLINE
void WorkbenchManipulatorPython::tryModifyMenuBar(const Py::Dict& dict, MenuItem* menuBar)
{
    std::string insert("insert");
    std::string append("append");
    std::string remove("remove");

    // insert a new command
    if (dict.hasKey(insert)) {
        std::string command = static_cast<std::string>(Py::String(dict.getItem(insert)));
        std::string itemName = static_cast<std::string>(Py::String(dict.getItem("menuItem")));
        bool after = dict.hasKey(std::string("after"));

        if (auto par = menuBar->findParentOf(itemName)) {
            if (MenuItem* item = par->findItem(itemName)) {
                if (after) {
                    item = par->afterItem(item);
                }

                if (item) {
                    auto add = new MenuItem();  // NOLINT
                    add->setCommand(command);
                    par->insertItem(item, add);
                }
            }
        }
    }
    // append a command
    else if (dict.hasKey(append)) {
        std::string command = static_cast<std::string>(Py::String(dict.getItem(append)));
        std::string itemName = static_cast<std::string>(Py::String(dict.getItem("menuItem")));

        if (auto par = menuBar->findParentOf(itemName)) {
            auto add = new MenuItem();  // NOLINT
            add->setCommand(command);
            par->appendItem(add);
        }
    }
    // remove a command
    else if (dict.hasKey(remove)) {
        std::string command = static_cast<std::string>(Py::String(dict.getItem(remove)));
        if (auto par = menuBar->findParentOf(command)) {
            if (MenuItem* item = par->findItem(command)) {
                if (item == menuBar) {
                    // Can't remove the menubar itself - Coverity issue 512853
                    FC_WARN("Cannot remove top-level menubar");
                    return;
                }
                par->removeItem(item);
                delete item;  // NOLINT
            }
        }

    }
}

/*!
 * \brief WorkbenchManipulatorPython::modifyContextMenu
 * \param menuBar
 * The Python manipulator can be implemented as
 * \code
 * class Manipulator:
 *   def modifyContextMenu(self, recipient):
 *     if recipient == "View":
 *       return [{"remove" : "Standard views"},
 *               {"insert" : "Std_Windows", "menuItem" : "View_Measure_Toggle_All"}]
 *
 * manip = Manipulator()
 * Gui.addWorkbenchManipulator(manip)
 * \endcode
 * This manipulator removes the "Standard views sub-menu and
 * adds the Std_Windows before the View_Measure_Toggle_All command.
 */
void WorkbenchManipulatorPython::modifyContextMenu(const char* recipient, MenuItem* menuBar)
{
    Base::PyGILStateLocker lock;
    try {
        tryModifyContextMenu(recipient, menuBar);
    }
    catch (Py::Exception&) {
        Base::PyException exc; // extract the Python error text
        exc.reportException();
    }
}

void WorkbenchManipulatorPython::tryModifyContextMenu(const char* recipient, MenuItem* menuBar)
{
    if (object.hasAttr(std::string("modifyContextMenu"))) {
        Py::Callable method(object.getAttr(std::string("modifyContextMenu")));
        Py::Tuple args(1);
        args.setItem(0, Py::String(recipient));
        Py::Object result = method.apply(args);
        if (result.isDict()) {
            tryModifyContextMenu(Py::Dict(result), menuBar);
        }
        else if (result.isSequence()) {
            Py::Sequence list(result);
            for (const auto& it : list) {
                if (it.isDict()) {
                    tryModifyContextMenu(Py::Dict(it), menuBar);
                }
            }
        }
    }
}

void WorkbenchManipulatorPython::tryModifyContextMenu(const Py::Dict& dict, MenuItem* menuBar)
{
    tryModifyMenuBar(dict, menuBar);
}

void WorkbenchManipulatorPython::modifyToolBars(ToolBarItem* toolBar)
{
    Base::PyGILStateLocker lock;
    try {
        tryModifyToolBar(toolBar);
    }
    catch (Py::Exception&) {
        Base::PyException exc; // extract the Python error text
        exc.reportException();
    }
}

/*!
 * \brief WorkbenchManipulatorPython::tryModifyToolBar
 * \param toolBar
 * The Python manipulator can be implemented as
 * \code
 * class Manipulator:
 *   def modifyToolBars(self):
       return [{"remove" : "Macro"},
               {"append" : "Std_Quit", "toolBar" : "File"},
 *             {"insert" : "Std_Cut", "toolItem" : "Std_New"}]
 *
 * manip = Manipulator()
 * Gui.addWorkbenchManipulator(manip)
 * \endcode
 * This manipulator removes the Macro toolbar, adds the
 * Std_Quit to the File toolbar and adds Std_Cut the
 * command to the toolbar where Std_New is part of.
 */
void WorkbenchManipulatorPython::tryModifyToolBar(ToolBarItem* toolBar)
{
    if (object.hasAttr(std::string("modifyToolBars"))) {
        Py::Callable method(object.getAttr(std::string("modifyToolBars")));
        Py::Tuple args;
        Py::Object result = method.apply(args);
        if (result.isDict()) {
            tryModifyToolBar(Py::Dict(result), toolBar);
        }
        else if (result.isSequence()) {
            Py::Sequence list(result);
            for (const auto& it : list) {
                if (it.isDict()) {
                    tryModifyToolBar(Py::Dict(it), toolBar);
                }
            }
        }
    }
}

// NOLINTNEXTLINE
void WorkbenchManipulatorPython::tryModifyToolBar(const Py::Dict& dict, ToolBarItem* toolBar)
{
    std::string insert("insert");
    std::string append("append");
    std::string remove("remove");

    // insert a new command
    if (dict.hasKey(insert)) {

        std::string command = static_cast<std::string>(Py::String(dict.getItem(insert)));
        std::string itemName = static_cast<std::string>(Py::String(dict.getItem("toolItem")));

        for (auto it : toolBar->getItems()) {
            if (ToolBarItem* item = it->findItem(itemName)) {
                auto add = new ToolBarItem();  // NOLINT
                add->setCommand(command);
                it->insertItem(item, add);
                break;
            }
        }
    }
    // append a command
    else if (dict.hasKey(append)) {
        std::string command = static_cast<std::string>(Py::String(dict.getItem(append)));
        std::string itemName = static_cast<std::string>(Py::String(dict.getItem("toolBar")));

        if (ToolBarItem* item = toolBar->findItem(itemName)) {
            auto add = new ToolBarItem();  // NOLINT
            add->setCommand(command);
            item->appendItem(add);
        }
    }
    // remove a command or toolbar
    else if (dict.hasKey(remove)) {
        std::string command = static_cast<std::string>(Py::String(dict.getItem(remove)));

        if (ToolBarItem* item = toolBar->findItem(command)) {
            toolBar->removeItem(item);
            delete item;  // NOLINT
        }
        else {
            for (auto it : toolBar->getItems()) {
                if (ToolBarItem* item = it->findItem(command)) {
                    if (item == toolBar) {
                        // Can't remove the toolBar itself - Coverity issue 513838
                        FC_WARN("Cannot remove top-level toolbar");
                        return;
                    }
                    it->removeItem(item);
                    delete item;  // NOLINT
                    break;
                }
            }
        }
    }
}

void WorkbenchManipulatorPython::modifyDockWindows(DockWindowItems* dockWindow)
{
    Base::PyGILStateLocker lock;
    try {
        tryModifyDockWindows(dockWindow);
    }
    catch (Py::Exception&) {
        Base::PyException exc; // extract the Python error text
        exc.reportException();
    }
}

void WorkbenchManipulatorPython::tryModifyDockWindows(DockWindowItems* dockWindow)
{
    if (object.hasAttr(std::string("modifyDockWindows"))) {
        Py::Callable method(object.getAttr(std::string("modifyDockWindows")));
        Py::Tuple args;
        Py::Object result = method.apply(args);
        if (result.isDict()) {
            tryModifyDockWindows(Py::Dict(result), dockWindow);
        }
        else if (result.isSequence()) {
            Py::Sequence list(result);
            for (const auto& it : list) {
                if (it.isDict()) {
                    tryModifyDockWindows(Py::Dict(it), dockWindow);
                }
            }
        }
    }
}

void WorkbenchManipulatorPython::tryModifyDockWindows([[maybe_unused]]const Py::Dict& dict,
                                                      [[maybe_unused]]DockWindowItems* dockWindow)
{
}
