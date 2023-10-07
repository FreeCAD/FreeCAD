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
#include "WorkbenchManipulator.h"

using namespace Gui;

std::set<WorkbenchManipulator::Ptr> WorkbenchManipulator::manipulators; // NOLINT

void WorkbenchManipulator::installManipulator(const WorkbenchManipulator::Ptr& ptr)
{
    manipulators.insert(ptr);
}

void WorkbenchManipulator::removeManipulator(const WorkbenchManipulator::Ptr& ptr)
{
    auto it = manipulators.find(ptr);
    if (it != manipulators.end()) {
        manipulators.erase(it);
    }
}

void WorkbenchManipulator::changeMenuBar(MenuItem* menuBar)
{
    for (auto& it : manipulators) {
        it->modifyMenuBar(menuBar);
    }
}

void WorkbenchManipulator::changeContextMenu(const char* recipient, MenuItem* menuBar)
{
    for (auto& it : manipulators) {
        it->modifyContextMenu(recipient, menuBar);
    }
}

void WorkbenchManipulator::changeToolBars(ToolBarItem* toolBar)
{
    for (auto& it : manipulators) {
        it->modifyToolBars(toolBar);
    }
}

void WorkbenchManipulator::changeDockWindows(DockWindowItems* dockWindow)
{
    for (auto& it : manipulators) {
        it->modifyDockWindows(dockWindow);
    }
}

void WorkbenchManipulator::modifyMenuBar([[maybe_unused]] MenuItem* menuBar)
{
}

void WorkbenchManipulator::modifyContextMenu([[maybe_unused]] const char* recipient,
                                             [[maybe_unused]] MenuItem* menuBar)
{
}

void WorkbenchManipulator::modifyToolBars([[maybe_unused]] ToolBarItem* toolBar)
{
}

void WorkbenchManipulator::modifyDockWindows([[maybe_unused]] DockWindowItems* dockWindow)
{
}
