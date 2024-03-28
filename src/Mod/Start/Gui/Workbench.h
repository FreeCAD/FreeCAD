// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 The FreeCAD Project Association AISBL               *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/


#ifndef LAUNCHERGUI_WORKBENCH_H
#define LAUNCHERGUI_WORKBENCH_H

#include <Gui/Workbench.h>
#include <memory>

namespace Gui
{
class MDIView;
}

namespace StartGui
{

class Workbench: public Gui::StdWorkbench
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();  // NOLINT

public:
    Workbench() = default;

    /** Run some actions when the workbench gets activated. */
    void activated() override;

    static void loadStart();

protected:
    /** Defines the standard menus. */
    Gui::MenuItem* setupMenuBar() const override;
    /** Defines the standard toolbars. */
    Gui::ToolBarItem* setupToolBars() const override;
    /** Defines the standard command bars. */
    Gui::ToolBarItem* setupCommandBars() const override;
    /** Returns a DockWindowItems structure of dock windows this workbench. */
    Gui::DockWindowItems* setupDockWindows() const override;

};  // class Workbench

}  // namespace StartGui
#endif  // LAUNCHERGUI_WORKBENCH_H
