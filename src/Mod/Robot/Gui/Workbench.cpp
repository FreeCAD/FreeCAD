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

#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <qobject.h>


#include <App/Application.h>
#include <Gui/Control.h>
#include <Gui/MainWindow.h>
#include <Gui/MenuManager.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/TaskView/TaskWatcher.h>
#include <Gui/ToolBarManager.h>
#include <Gui/WaitCursor.h>

#include "TaskWatcher.h"
#include "Workbench.h"


using namespace RobotGui;

#if 0  // needed for Qt's lupdate utility
    qApp->translate("Workbench", "Robot");
    qApp->translate("Workbench", "Insert Robot");
    qApp->translate("Workbench", "&Robot");
    qApp->translate("Workbench", "Export Trajectory");
    qApp->translate("Gui::TaskView::TaskWatcherCommands", "Trajectory Tools");
    qApp->translate("Gui::TaskView::TaskWatcherCommands", "Robot Tools");
    qApp->translate("Gui::TaskView::TaskWatcherCommands", "Insert Robot");
#endif

/// @namespace RobotGui @class Workbench
TYPESYSTEM_SOURCE(RobotGui::Workbench, Gui::StdWorkbench)

Workbench::Workbench() = default;

Workbench::~Workbench() = default;

void Workbench::activated()
{
    Gui::Workbench::activated();

    const char* RobotAndTrac[] = {"Robot_InsertWaypoint", "Robot_InsertWaypointPreselect", nullptr};

    const char* Robot[] = {"Robot_AddToolShape", "Robot_SetHomePos", "Robot_RestoreHomePos", nullptr};

    const char* TracSingle[] = {"Robot_TrajectoryDressUp", nullptr};

    const char* TracMore[] = {"Robot_TrajectoryCompound", nullptr};

    std::vector<Gui::TaskView::TaskWatcher*> Watcher;

    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT Robot::TrajectoryObject COUNT 1"
        "SELECT Robot::RobotObject COUNT 1",
        RobotAndTrac,
        "Trajectory Tools",
        "Robot_InsertWaypoint"
    ));

    Watcher.push_back(new TaskWatcherRobot);

    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT Robot::RobotObject COUNT 1",
        Robot,
        "Robot Tools",
        "Robot_CreateRobot"
    ));

    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT Robot::TrajectoryObject COUNT 1",
        TracSingle,
        "Trajectory Tools",
        "Robot_CreateRobot"
    ));

    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT Robot::TrajectoryObject COUNT 2..",
        TracMore,
        "Trajectory Tools",
        "Robot_CreateRobot"
    ));

    addTaskWatcher(Watcher);
    Gui::Control().showTaskView();
}


void Workbench::deactivated()
{
    Gui::Workbench::deactivated();
    removeTaskWatcher();
}


Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();
    Gui::ToolBarItem* part = new Gui::ToolBarItem(root);
    part->setCommand("Robot");
    *part << "Robot_Create";
    *part << "Separator";
    *part << "Robot_CreateTrajectory";
    *part << "Robot_InsertWaypoint";
    *part << "Robot_InsertWaypointPreselect";
    *part << "Separator";
    *part << "Robot_Edge2Trac";
    *part << "Robot_TrajectoryDressUp";
    *part << "Robot_TrajectoryCompound";
    *part << "Separator";
    *part << "Robot_SetHomePos";
    *part << "Robot_RestoreHomePos";
    *part << "Separator";
    *part << "Robot_Simulate";
    return root;
}

Gui::MenuItem* Workbench::setupMenuBar() const
{
    Gui::MenuItem* root = StdWorkbench::setupMenuBar();
    Gui::MenuItem* item = root->findItem("&Windows");
    Gui::MenuItem* robot = new Gui::MenuItem;
    root->insertItem(item, robot);

    // analyze
    Gui::MenuItem* insertRobots = new Gui::MenuItem;
    insertRobots->setCommand("Insert Robot");
    *insertRobots << "Robot_AddToolShape";

    // boolean
    Gui::MenuItem* exportM = new Gui::MenuItem;
    exportM->setCommand("Export Trajectory");
    *exportM << "Robot_ExportKukaCompact"
             << "Robot_ExportKukaFull";

    robot->setCommand("&Robot");
    *robot << insertRobots << "Robot_CreateTrajectory"
           << "Separator"
           << "Robot_CreateTrajectory"
           << "Robot_InsertWaypoint"
           << "Robot_InsertWaypointPreselect"
           << "Robot_Edge2Trac"
           << "Separator"
           << "Robot_SetHomePos"
           << "Robot_RestoreHomePos"
           << "Separator"
           << "Robot_SetDefaultOrientation"
           << "Robot_SetDefaultValues"
           << "Separator"
           << "Robot_Simulate" << exportM;
    return root;
}
