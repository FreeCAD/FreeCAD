/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <werner.wm.mayer@gmx.de>              *
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

#ifndef ROBOT_WORKBENCH_H
#define ROBOT_WORKBENCH_H

#include <Gui/TaskView/TaskWatcher.h>
#include <Gui/Workbench.h>
#include <Mod/Robot/RobotGlobal.h>


namespace RobotGui
{

/**
 * @author Werner Mayer
 */
class RobotGuiExport Workbench: public Gui::StdWorkbench
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    Workbench();
    ~Workbench() override;

    /** Run some actions when the workbench gets activated. */
    void activated() override;
    /** Run some actions when the workbench gets deactivated. */
    void deactivated() override;


protected:
    Gui::ToolBarItem* setupToolBars() const override;
    Gui::MenuItem* setupMenuBar() const override;

    std::vector<Gui::TaskView::TaskWatcher*> Watcher;
};

}// namespace RobotGui


#endif// ROBOT_WORKBENCH_H
