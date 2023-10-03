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
#include <Gui/MenuManager.h>
#include <Gui/ToolBarManager.h>

using namespace PartGui;

void WorkbenchManipulator::modifyMenuBar([[maybe_unused]] Gui::MenuItem* menuBar)
{
    addSectionCut(menuBar);
}

void WorkbenchManipulator::modifyContextMenu(const char* recipient, Gui::MenuItem* menuBar)
{
    if (strcmp(recipient, "View") == 0) {
        addSelectionFilter(menuBar);
    }
}

void WorkbenchManipulator::modifyToolBars(Gui::ToolBarItem* toolBar)
{
    addSelectionFilter(toolBar);
}

void WorkbenchManipulator::modifyDockWindows([[maybe_unused]] Gui::DockWindowItems* dockWindow)
{
}

void WorkbenchManipulator::addSectionCut(Gui::MenuItem* menuBar)
{
    const char* toggleClipPlane = "Std_ToggleClipPlane";
    auto par = menuBar->findParentOf(toggleClipPlane);
    if (par) {
        auto item = par->findItem(toggleClipPlane);
        item = par->afterItem(item);

        auto add = new Gui::MenuItem(); // NOLINT
        add->setCommand("Part_SectionCut");
        par->insertItem(item, add);
    }
}

void WorkbenchManipulator::addSelectionFilter(Gui::ToolBarItem* toolBar)
{
    if (auto view = toolBar->findItem("View")) {
        auto add = new Gui::ToolBarItem(); // NOLINT
        add->setCommand("Part_SelectFilter");
        auto item = view->findItem("Std_TreeViewActions");
        if (item) {
            view->insertItem(item, add);
        }
        else {
            view->appendItem(add);
        }
    }
}

void WorkbenchManipulator::addSelectionFilter(Gui::MenuItem* menuBar)
{
    if (auto measure = menuBar->findItem("Measure")) {
        auto add = new Gui::MenuItem(); // NOLINT
        add->setCommand("Part_SelectFilter");
        menuBar->insertItem(measure, add);
    }
}
