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
#include <Base/Interpreter.h>

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

void WorkbenchManipulatorPython::modifyMenuBar([[maybe_unused]] MenuItem* menuBar)
{
    Base::PyGILStateLocker lock;
    try {
        if (object.hasAttr(std::string("modifyMenuBar"))) {
            Py::Callable method(object.getAttr(std::string("modifyMenuBar")));
            Py::Tuple args(1);
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException exc; // extract the Python error text
        exc.ReportException();
    }
}

void WorkbenchManipulatorPython::modifyContextMenu([[maybe_unused]] const char* recipient,
                                                   [[maybe_unused]] MenuItem* menuBar)
{
    Base::PyGILStateLocker lock;
    try {
        if (object.hasAttr(std::string("modifyContextMenu"))) {
            Py::Callable method(object.getAttr(std::string("modifyContextMenu")));
            Py::Tuple args(2);
            args.setItem(0, Py::String(recipient));
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException exc; // extract the Python error text
        exc.ReportException();
    }
}

void WorkbenchManipulatorPython::modifyToolBars([[maybe_unused]] ToolBarItem* toolBar)
{
    Base::PyGILStateLocker lock;
    try {
        if (object.hasAttr(std::string("modifyToolBars"))) {
            Py::Callable method(object.getAttr(std::string("modifyToolBars")));
            Py::Tuple args(1);
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException exc; // extract the Python error text
        exc.ReportException();
    }
}

void WorkbenchManipulatorPython::modifyDockWindows([[maybe_unused]] DockWindowItems* dockWindow)
{
    Base::PyGILStateLocker lock;
    try {
        if (object.hasAttr(std::string("modifyDockWindows"))) {
            Py::Callable method(object.getAttr(std::string("modifyDockWindows")));
            Py::Tuple args(1);
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException exc; // extract the Python error text
        exc.ReportException();
    }
}
