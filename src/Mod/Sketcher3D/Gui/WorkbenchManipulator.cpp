// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Yash Suthar <yashsuthar983@gmail.com>              *
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

#include <Gui/MenuManager.h>
#include <Gui/ToolBarManager.h>

#include "WorkbenchManipulator.h"


using namespace Sketcher3DGui;

void WorkbenchManipulator::modifyMenuBar(Gui::MenuItem* menuBar)
{
    addCreateSketchToMenu(menuBar);
}

void WorkbenchManipulator::modifyToolBars(Gui::ToolBarItem* toolBar)
{
    setupCreateSketchToolbar(toolBar);
    setupEditModeToolbar(toolBar);
}

void WorkbenchManipulator::setupCreateSketchToolbar(Gui::ToolBarItem* toolBar)
{
    auto sketcher = toolBar->findItem("Sketcher");
    if (!sketcher) {
        return;
    }

    sketcher->clear();
    *sketcher << "Sketcher_NewSketch"
              << "Sketcher3D_CreateSketch";
}

void WorkbenchManipulator::addCreateSketchToMenu(Gui::MenuItem* menuBar)
{
    auto sketch = menuBar->findItem("S&ketch");
    if (!sketch) {
        return;
    }

    auto add = new Gui::MenuItem();
    add->setCommand("Sketcher3D_CreateSketch");
    sketch->appendItem(add);
}

void WorkbenchManipulator::setupEditModeToolbar(Gui::ToolBarItem* toolBar)
{
    if (!toolBar->findItem("Sketcher")) {
        return;
    }

    auto* editTb = new Gui::ToolBarItem(toolBar, Gui::ToolBarItem::DefaultVisibility::Unavailable);
    editTb->setCommand("Sketcher3D Edit");
    *editTb << "Sketcher3D_CreatePoint"
            << "Sketcher3D_CreateLine"
            << "Sketcher3D_CreatePolyline"
            << "Sketcher3D_ToggleConstruction"
            << "Separator"
            << "Sketcher3D_CompDimensionTools"
            << "Sketcher3D_ConstrainCoincident"
            << "Sketcher3D_CompParallel"
            << "Sketcher3D_ConstrainEqualLength"
            << "Sketcher3D_ConstrainPointOnLine"
            << "Sketcher3D_ConstrainPointAtLineMidpoint"
            << "Sketcher3D_ConstrainCollinear";
}
