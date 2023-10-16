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


#ifndef GUI_WORKBENCHMANIPULATOR_H
#define GUI_WORKBENCHMANIPULATOR_H

#include <memory>
#include <set>
#include <map>
#include <string>
#include <FCGlobal.h>

namespace Gui {

class DockWindowItems;
class MenuItem;
class ToolBarItem;

/**
 * The WorkbenchManipulator is a class that allows to modify the workbench
 * by adding or removing commands.
 * WorkbenchManipulator provides methods to manipulate the MenuItem, ToolBarItem or
 * DockWindowItems structure before setting up the workbench.
 * @author Werner Mayer
 */
class GuiExport WorkbenchManipulator
{
public:
    using Ptr = std::shared_ptr<WorkbenchManipulator>;
    /*!
     * \brief installManipulator
     * Installs a new instance of WorkbenchManipulator
     */
    static void installManipulator(const WorkbenchManipulator::Ptr&);
    /*!
     * \brief removeManipulator
     * Removes an installed instance of WorkbenchManipulator
     */
    static void removeManipulator(const WorkbenchManipulator::Ptr&);
    /*!
     * \brief changeMenuBar
     * Calls \ref modifyMenuBar for every installed WorkbenchManipulator
     */
    static void changeMenuBar(MenuItem* menuBar);
    /*!
     * \brief changeContextMenu
     * Calls \ref modifyContextMenu for every installed WorkbenchManipulator
     */
    static void changeContextMenu(const char* recipient, MenuItem* menuBar);
    /*!
     * \brief changeToolBars
     * Calls \ref modifyToolBars for every installed WorkbenchManipulator
     */
    static void changeToolBars(ToolBarItem* toolBar);
    /*!
     * \brief changeDockWindows
     * Calls \ref modifyDockWindows for every installed WorkbenchManipulator
     */
    static void changeDockWindows(DockWindowItems* dockWindow);

    WorkbenchManipulator() = default;
    virtual ~WorkbenchManipulator() = default;

protected:
    /*!
     * \brief modifyMenuBar
     * Method to manipulate the menu structure of a workbench.
     * The default implementation doesn't change anything.
     */
    virtual void modifyMenuBar([[maybe_unused]] MenuItem* menuBar);
    /*!
     * \brief modifyContextMenu
     * Method to manipulate the contextmenu structure of a workbench.
     * The default implementation doesn't change anything.
     */
    virtual void modifyContextMenu([[maybe_unused]] const char* recipient,
                                   [[maybe_unused]] MenuItem* menuBar);
    /*!
     * \brief modifyToolBars
     * Method to manipulate the toolbar structure of a workbench
     * The default implementation doesn't change anything.
     */
    virtual void modifyToolBars([[maybe_unused]] ToolBarItem* toolBar);
    /*!
     * \brief modifyDockWindows
     * Method to manipulate the dock window structure of a workbench
     * The default implementation doesn't change anything.
     */
    virtual void modifyDockWindows([[maybe_unused]] DockWindowItems* dockWindow);

public:
    WorkbenchManipulator(const WorkbenchManipulator&) = delete;
    WorkbenchManipulator(WorkbenchManipulator&&) = delete;
    WorkbenchManipulator& operator = (const WorkbenchManipulator&) = delete;
    WorkbenchManipulator& operator = (WorkbenchManipulator&&) = delete;

private:
    static std::set<WorkbenchManipulator::Ptr> manipulators; // NOLINT
};

} // namespace Gui


#endif // GUI_WORKBENCHMANIPULATOR_H
