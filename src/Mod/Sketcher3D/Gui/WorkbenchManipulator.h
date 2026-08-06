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


#pragma once

#include <Gui/WorkbenchManipulator.h>


namespace Sketcher3DGui
{

// Injects Sketcher3D Tools into the Sketcher workbench.

class WorkbenchManipulator: public Gui::WorkbenchManipulator
{
protected:
    void modifyMenuBar(Gui::MenuItem* menuBar) override;
    void modifyToolBars(Gui::ToolBarItem* toolBar) override;

private:
    static void setupCreateSketchToolbar(Gui::ToolBarItem* toolBar);
    static void setupEditModeToolbar(Gui::ToolBarItem* toolBar);
    static void addCreateSketchToMenu(Gui::MenuItem* menuBar);
};

}  // namespace Sketcher3DGui
