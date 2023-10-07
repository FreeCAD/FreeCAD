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


#ifndef PARTGUI_WORKBENCHMANIPULATOR_H
#define PARTGUI_WORKBENCHMANIPULATOR_H

#include <Gui/WorkbenchManipulator.h>

namespace PartGui {

class WorkbenchManipulator: public Gui::WorkbenchManipulator
{
protected:
    /*!
     * \brief modifyMenuBar
     * Method to manipulate the menu structure of a workbench.
     * The default implementation doesn't change anything.SectionCut
     */
    void modifyMenuBar(Gui::MenuItem* menuBar) override;
    /*!
     * \brief modifyContextMenu
     * Method to manipulate the contextmenu structure of a workbench.
     * The default implementation doesn't change anything.
     */
    void modifyContextMenu(const char* recipient, Gui::MenuItem* menuBar) override;
    /*!
     * \brief modifyToolBars
     * Method to manipulate the toolbar structure of a workbench
     * The default implementation doesn't change anything.
     */
    void modifyToolBars([[maybe_unused]] Gui::ToolBarItem* toolBar) override;
    /*!
     * \brief modifyDockWindows
     * Method to manipulate the dock window structure of a workbench
     * The default implementation doesn't change anything.
     */
    void modifyDockWindows([[maybe_unused]] Gui::DockWindowItems* dockWindow) override;

private:
    static void addSectionCut(Gui::MenuItem* menuBar);
    static void addSelectionFilter(Gui::ToolBarItem* toolBar);
    static void addSelectionFilter(Gui::MenuItem* menuBar);
};

} // namespace PartGui


#endif // PARTGUI_WORKBENCHMANIPULATOR_H
