/***************************************************************************
 *   Copyright (c) 2024 Martin Rodriguez Reboredo <yakoyoku@gmail.com>     *
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

#ifndef TEXTGUI_Workbench_H
#define TEXTGUI_Workbench_H

#include <Gui/MenuManager.h>
#include <Gui/ToolBarManager.h>
#include <Gui/Workbench.h>
#include <Mod/Text/TextGlobal.h>


namespace TextGui {

class TextGuiExport Workbench : public Gui::StdWorkbench
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    Workbench();
    ~Workbench() override;

    static void enterEditMode();
    static void leaveEditMode();

protected:
    Gui::MenuItem* setupMenuBar() const override;
    Gui::ToolBarItem* setupToolBars() const override;
    Gui::ToolBarItem* setupCommandBars() const override;
    void activated() override;
};

TextGuiExport void addTextWorkbenchTextActions(Gui::MenuItem& sketch);
TextGuiExport void addTextWorkbenchTextEditModeActions(Gui::MenuItem& sketch);

TextGuiExport void addTextWorkbenchTextActions(Gui::ToolBarItem& sketch);
TextGuiExport void addTextWorkbenchTextEditModeActions(Gui::ToolBarItem& sketch);

} //namespace TextGui


#endif // TEXTGUI_Workbench_H
