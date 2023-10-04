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


#ifndef GUI_WORKBENCHMANIPULATOR_PYTHON_H
#define GUI_WORKBENCHMANIPULATOR_PYTHON_H

#include <Gui/WorkbenchManipulator.h>
#include <CXX/Objects.hxx>

namespace Gui {

/**
 * The WorkbenchManipulatorPython class accepts an instance of a Python class
 * that is supposed to implement any of the virtual functions.
 * @author Werner Mayer
 */
class GuiExport WorkbenchManipulatorPython : public WorkbenchManipulator
{
public:
    static void installManipulator(const Py::Object& obj);
    static void removeManipulator(const Py::Object& obj);
    explicit WorkbenchManipulatorPython(const Py::Object& obj);
    ~WorkbenchManipulatorPython() override;

protected:
    /*!
     * \brief modifyMenuBar
     * Method to manipulate the menu structure of a workbench.
     */
    void modifyMenuBar([[maybe_unused]] MenuItem* menuBar) override;
    /*!
     * \brief modifyContextMenu
     * Method to manipulate the contextmenu structure of a workbench.
     */
    void modifyContextMenu([[maybe_unused]] const char* recipient,
                           [[maybe_unused]] MenuItem* menuBar) override;
    /*!
     * \brief modifyToolBars
     * Method to manipulate the toolbar structure of a workbench
     */
    void modifyToolBars([[maybe_unused]] ToolBarItem* toolBar) override;
    /*!
     * \brief modifyDockWindows
     * Method to manipulate the dock window structure of a workbench
     */
    void modifyDockWindows([[maybe_unused]] DockWindowItems* dockWindow) override;

public:
    WorkbenchManipulatorPython(const WorkbenchManipulatorPython&) = delete;
    WorkbenchManipulatorPython(WorkbenchManipulatorPython&&) = delete;
    WorkbenchManipulatorPython& operator = (const WorkbenchManipulatorPython&) = delete;
    WorkbenchManipulatorPython& operator = (WorkbenchManipulatorPython&&) = delete;

private:
    Py::Object object;
};

} // namespace Gui


#endif // GUI_WORKBENCHMANIPULATOR_PYTHON_H
