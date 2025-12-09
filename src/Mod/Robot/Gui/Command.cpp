/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <QMessageBox>


#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionFilter.h>
#include <Mod/Robot/App/RobotObject.h>
#include <Mod/Robot/App/TrajectoryObject.h>

#include "TaskDlgSimulate.h"
#include "TrajectorySimulate.h"


using namespace std;
using namespace RobotGui;

#include <QFileDialog>

namespace
{

std::string getWrl(const QString& hint_directory)
{
    QString fileName = QFileDialog::getOpenFileName(
        Gui::getMainWindow(),
        QObject::tr("Select VRML file for Robot"),
        hint_directory,
        QObject::tr("VRML Files (*.wrl *.vrml)")
    );

    return fileName.toStdString();
}

std::string getCsv(const std::string& wrl_path)
{
    QFileInfo wrlInfo(QString::fromStdString(wrl_path));
    QString hintDir = wrlInfo.absolutePath();
    QString fileName = QFileDialog::getOpenFileName(
        Gui::getMainWindow(),
        QObject::tr("Select Kinematic CSV file for Robot"),
        hintDir,
        QObject::tr("CSV Files (*.csv)")
    );
    return fileName.toStdString();
}

}  // namespace

DEF_STD_CMD_A(CmdRobotSetHomePos)

CmdRobotSetHomePos::CmdRobotSetHomePos()
    : Command("Robot_SetHomePos")
{
    sAppModule = "Robot";
    sGroup = QT_TR_NOOP("Robot");
    sMenuText = QT_TR_NOOP("Set Home Position");
    sToolTipText = QT_TR_NOOP("Sets the home position");
    sWhatsThis = "Robot_SetHomePos";
    sStatusTip = sToolTipText;
    sPixmap = "Robot_SetHomePos";
}


void CmdRobotSetHomePos::activated(int)
{
    const char* SelFilter = "SELECT Robot::RobotObject COUNT 1 ";

    Gui::SelectionFilter filter(SelFilter);
    Robot::RobotObject* pcRobotObject;
    if (filter.match()) {
        pcRobotObject = static_cast<Robot::RobotObject*>(filter.Result[0][0].getObject());
    }
    else {
        QMessageBox::warning(
            Gui::getMainWindow(),
            QObject::tr("Wrong selection"),
            QObject::tr("Select one Robot to set home position")
        );
        return;
    }


    std::string FeatName = pcRobotObject->getNameInDocument();

    const char* n = FeatName.c_str();
    openCommand("Set home");
    doCommand(
        Doc,
        "App.activeDocument().%s.Home = "
        "[App.activeDocument().%s.Axis1,App.activeDocument().%s.Axis2,App.activeDocument().%"
        "s.Axis3,App.activeDocument().%s.Axis4,App.activeDocument().%s.Axis5,App."
        "activeDocument().%s.Axis6]",
        n,
        n,
        n,
        n,
        n,
        n,
        n
    );
    updateActive();
    commitCommand();
}

bool CmdRobotSetHomePos::isActive()
{
    return hasActiveDocument();
}


// #####################################################################################################
DEF_STD_CMD_A(CmdRobotRestoreHomePos)

CmdRobotRestoreHomePos::CmdRobotRestoreHomePos()
    : Command("Robot_RestoreHomePos")
{
    sAppModule = "Robot";
    sGroup = QT_TR_NOOP("Robot");
    sMenuText = QT_TR_NOOP("Move to Home");
    sToolTipText = QT_TR_NOOP("Moves to the home position");
    sWhatsThis = "Robot_RestoreHomePos";
    sStatusTip = sToolTipText;
    sPixmap = "Robot_RestoreHomePos";
}


void CmdRobotRestoreHomePos::activated(int)
{
    const char* SelFilter = "SELECT Robot::RobotObject COUNT 1 ";

    Gui::SelectionFilter filter(SelFilter);
    Robot::RobotObject* pcRobotObject;
    if (filter.match()) {
        pcRobotObject = static_cast<Robot::RobotObject*>(filter.Result[0][0].getObject());
    }
    else {
        QMessageBox::warning(
            Gui::getMainWindow(),
            QObject::tr("Wrong selection"),
            QObject::tr("Select one Robot")
        );
        return;
    }


    std::string FeatName = pcRobotObject->getNameInDocument();

    const char* n = FeatName.c_str();
    openCommand("Move to home");
    doCommand(Doc, "App.activeDocument().%s.Axis1 = App.activeDocument().%s.Home[0]", n, n);
    doCommand(Doc, "App.activeDocument().%s.Axis2 = App.activeDocument().%s.Home[1]", n, n);
    doCommand(Doc, "App.activeDocument().%s.Axis3 = App.activeDocument().%s.Home[2]", n, n);
    doCommand(Doc, "App.activeDocument().%s.Axis4 = App.activeDocument().%s.Home[3]", n, n);
    doCommand(Doc, "App.activeDocument().%s.Axis5 = App.activeDocument().%s.Home[4]", n, n);
    doCommand(Doc, "App.activeDocument().%s.Axis6 = App.activeDocument().%s.Home[5]", n, n);
    updateActive();
    commitCommand();
}

bool CmdRobotRestoreHomePos::isActive()
{
    return hasActiveDocument();
}


// #####################################################################################################
DEF_STD_CMD_A(CmdRobotConstraintAxle)

CmdRobotConstraintAxle::CmdRobotConstraintAxle()
    : Command("Robot_Create")
{
    sAppModule = "Robot";
    sGroup = QT_TR_NOOP("Robot");
    sMenuText = QT_TR_NOOP("Place Robot");
    sToolTipText = QT_TR_NOOP("Places a robot in the scene");

    sWhatsThis = "Robot_Create";
    sStatusTip = sToolTipText;
    sPixmap = "Robot_CreateRobot";
}


void CmdRobotConstraintAxle::activated([[maybe_unused]] int msg)
{
    const std::string FeatName = getUniqueObjectName("Robot");
    const std::string WrlPath = getWrl(QString());
    const std::string KinematicPath = getCsv(WrlPath);

    openCommand("Place robot");
    doCommand(Doc, "App.activeDocument().addObject(\"Robot::RobotObject\",\"%s\")", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.RobotVrmlFile = \"%s\"", FeatName.c_str(), WrlPath.c_str());
    doCommand(
        Doc,
        "App.activeDocument().%s.RobotKinematicFile = \"%s\"",
        FeatName.c_str(),
        KinematicPath.c_str()
    );
    updateActive();
    commitCommand();
}

bool CmdRobotConstraintAxle::isActive()
{
    return hasActiveDocument();
}


// #####################################################################################################

DEF_STD_CMD_A(CmdRobotSimulate)

CmdRobotSimulate::CmdRobotSimulate()
    : Command("Robot_Simulate")
{
    sAppModule = "Robot";
    sGroup = QT_TR_NOOP("Robot");
    sMenuText = QT_TR_NOOP("Simulate Trajectory");
    sToolTipText = QT_TR_NOOP("Simulates robot movement along a selected trajectory");
    sWhatsThis = "Robot_Simulate";
    sStatusTip = sToolTipText;
    sPixmap = "Robot_Simulate";
}


void CmdRobotSimulate::activated(int)
{
    const char* SelFilter = "SELECT Robot::RobotObject  \n"
                            "SELECT Robot::TrajectoryObject  ";

    Gui::SelectionFilter filter(SelFilter);
    Robot::RobotObject* pcRobotObject;
    Robot::TrajectoryObject* pcTrajectoryObject;

    if (filter.match()) {
        pcRobotObject = static_cast<Robot::RobotObject*>(filter.Result[0][0].getObject());
        pcTrajectoryObject = static_cast<Robot::TrajectoryObject*>(filter.Result[1][0].getObject());
    }
    else {
        QMessageBox::warning(
            Gui::getMainWindow(),
            QObject::tr("Wrong selection"),
            QObject::tr("Select one Robot and one Trajectory object.")
        );
        return;
    }

    if (pcTrajectoryObject->Trajectory.getValue().getSize() < 2) {
        QMessageBox::warning(
            Gui::getMainWindow(),
            QObject::tr("Trajectory not valid"),
            QObject::tr("You need at least two waypoints in a trajectory to simulate.")
        );
        return;
    }

    Gui::TaskView::TaskDialog* dlg = new TaskDlgSimulate(pcRobotObject, pcTrajectoryObject);
    Gui::Control().showDialog(dlg);
}

bool CmdRobotSimulate::isActive()
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}


// #####################################################################################################


void CreateRobotCommands()
{
    Gui::CommandManager& command_manager = Gui::Application::Instance->commandManager();

    command_manager.addCommand(new CmdRobotRestoreHomePos());
    command_manager.addCommand(new CmdRobotSetHomePos());
    command_manager.addCommand(new CmdRobotConstraintAxle());
    command_manager.addCommand(new CmdRobotSimulate());
}
