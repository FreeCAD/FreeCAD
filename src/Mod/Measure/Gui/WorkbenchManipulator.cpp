/***************************************************************************
 *   Copyright (c) 2024 David Friedli <david[at]friedli-be.ch>             *
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

using namespace MeasureGui;

void WorkbenchManipulator::modifyMenuBar([[maybe_unused]] Gui::MenuItem* menuBar)
{
    auto menuTools = menuBar->findItem("&Tools");
    if (!menuTools) {
        return;
    }
    auto itemMeasure = new Gui::MenuItem();
    itemMeasure->setCommand("Std_Measure");
    menuTools->appendItem(itemMeasure);
}

void WorkbenchManipulator::modifyToolBars(Gui::ToolBarItem* toolBar)
{
    auto tbView = toolBar->findItem("View");
    if (!tbView) {
        return;
    }

    auto itemMeasure = new Gui::ToolBarItem();
    itemMeasure->setCommand("Std_Measure");
    tbView->appendItem(itemMeasure);
}
