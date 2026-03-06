// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#pragma once

#include <Gui/WorkbenchManipulator.h>

namespace MatGui {

class WorkbenchManipulator: public Gui::WorkbenchManipulator
{
protected:
    /*!
     * \brief modifyMenuBar
     * Adds the commands Std_SetMaterial and Std_SetAppearance to the View menu
     */
    void modifyMenuBar(Gui::MenuItem* menuBar) override;
    /*!
     * \brief modifyContextMenu
     * Adds the commands Std_SetMaterial and Std_SetAppearance to the contex-menu
     */
    void modifyContextMenu(const char* recipient, Gui::MenuItem* menuBar) override;

private:
    static void addCommands(Gui::MenuItem* menuBar, const char* reference);
    static void addCommandsToTree(Gui::MenuItem* menuBar);
};

} // namespace MatGui