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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QMessageBox>
#endif

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/FileDialog.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Mod/Robot/App/RobotObject.h>
#include <Mod/Robot/App/TrajectoryObject.h>


using namespace std;

DEF_STD_CMD_A(CmdRobotExportKukaCompact)

CmdRobotExportKukaCompact::CmdRobotExportKukaCompact()
    : Command("Robot_ExportKukaCompact")
{
    sAppModule = "Robot";
    sGroup = QT_TR_NOOP("Robot");
    sMenuText = QT_TR_NOOP("Kuka compact subroutine...");
    sToolTipText = QT_TR_NOOP("Export the trajectory as a compact KRL subroutine.");
    sWhatsThis = "Robot_ExportKukaCompact";
    sStatusTip = sToolTipText;
    sPixmap = "Robot_Export";
}


void CmdRobotExportKukaCompact::activated(int)
{
    unsigned int n1 = getSelection().countObjectsOfType(Robot::RobotObject::getClassTypeId());
    unsigned int n2 = getSelection().countObjectsOfType(Robot::TrajectoryObject::getClassTypeId());

    if (n1 != 1 || n2 != 1) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Wrong selection"),
                             QObject::tr("Select one Robot and one Trajectory object."));
        return;
    }

    std::vector<Gui::SelectionSingleton::SelObj> Sel = getSelection().getSelection();


    Robot::RobotObject* pcRobotObject = nullptr;
    if (Sel[0].pObject->getTypeId() == Robot::RobotObject::getClassTypeId()) {
        pcRobotObject = static_cast<Robot::RobotObject*>(Sel[0].pObject);
    }
    else if (Sel[1].pObject->getTypeId() == Robot::RobotObject::getClassTypeId()) {
        pcRobotObject = static_cast<Robot::RobotObject*>(Sel[1].pObject);
    }
    std::string RoboName = pcRobotObject->getNameInDocument();

    Robot::TrajectoryObject* pcTrajectoryObject = nullptr;
    if (Sel[0].pObject->getTypeId() == Robot::TrajectoryObject::getClassTypeId()) {
        pcTrajectoryObject = static_cast<Robot::TrajectoryObject*>(Sel[0].pObject);
    }
    else if (Sel[1].pObject->getTypeId() == Robot::TrajectoryObject::getClassTypeId()) {
        pcTrajectoryObject = static_cast<Robot::TrajectoryObject*>(Sel[1].pObject);
    }
    // std::string TrakName = pcTrajectoryObject->getNameInDocument();

    QStringList filter;
    filter << QString::fromLatin1("%1 (*.src)").arg(QObject::tr("KRL file"));
    filter << QString::fromLatin1("%1 (*.*)").arg(QObject::tr("All Files"));
    QString fn = Gui::FileDialog::getSaveFileName(Gui::getMainWindow(),
                                                  QObject::tr("Export program"),
                                                  QString(),
                                                  filter.join(QLatin1String(";;")));
    if (fn.isEmpty()) {
        return;
    }

    doCommand(Doc, "from KukaExporter import ExportCompactSub");
    doCommand(Doc,
              "ExportCompactSub(App.activeDocument().%s,App.activeDocument().%s,'%s')",
              pcRobotObject->getNameInDocument(),
              pcTrajectoryObject->getNameInDocument(),
              (const char*)fn.toLatin1());
}

bool CmdRobotExportKukaCompact::isActive()
{
    return hasActiveDocument();
}

// #####################################################################################################


DEF_STD_CMD_A(CmdRobotExportKukaFull)

CmdRobotExportKukaFull::CmdRobotExportKukaFull()
    : Command("Robot_ExportKukaFull")
{
    sAppModule = "Robot";
    sGroup = QT_TR_NOOP("Robot");
    sMenuText = QT_TR_NOOP("Kuka full subroutine...");
    sToolTipText = QT_TR_NOOP("Export the trajectory as a full KRL subroutine.");
    sWhatsThis = "Robot_ExportKukaFull";
    sStatusTip = sToolTipText;
    sPixmap = "Robot_Export";
}


void CmdRobotExportKukaFull::activated(int)
{
    unsigned int n1 = getSelection().countObjectsOfType(Robot::RobotObject::getClassTypeId());
    unsigned int n2 = getSelection().countObjectsOfType(Robot::TrajectoryObject::getClassTypeId());

    if (n1 != 1 || n2 != 1) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Wrong selection"),
                             QObject::tr("Select one Robot and one Trajectory object."));
        return;
    }

    std::vector<Gui::SelectionSingleton::SelObj> Sel = getSelection().getSelection();


    Robot::RobotObject* pcRobotObject = nullptr;
    if (Sel[0].pObject->getTypeId() == Robot::RobotObject::getClassTypeId()) {
        pcRobotObject = static_cast<Robot::RobotObject*>(Sel[0].pObject);
    }
    else if (Sel[1].pObject->getTypeId() == Robot::RobotObject::getClassTypeId()) {
        pcRobotObject = static_cast<Robot::RobotObject*>(Sel[1].pObject);
    }
    // std::string RoboName = pcRobotObject->getNameInDocument();

    Robot::TrajectoryObject* pcTrajectoryObject = nullptr;
    if (Sel[0].pObject->getTypeId() == Robot::TrajectoryObject::getClassTypeId()) {
        pcTrajectoryObject = static_cast<Robot::TrajectoryObject*>(Sel[0].pObject);
    }
    else if (Sel[1].pObject->getTypeId() == Robot::TrajectoryObject::getClassTypeId()) {
        pcTrajectoryObject = static_cast<Robot::TrajectoryObject*>(Sel[1].pObject);
    }
    // std::string TrakName = pcTrajectoryObject->getNameInDocument();

    QStringList filter;
    filter << QString::fromLatin1("%1 (*.src)").arg(QObject::tr("KRL file"));
    filter << QString::fromLatin1("%1 (*.*)").arg(QObject::tr("All Files"));
    QString fn = Gui::FileDialog::getSaveFileName(Gui::getMainWindow(),
                                                  QObject::tr("Export program"),
                                                  QString(),
                                                  filter.join(QLatin1String(";;")));
    if (fn.isEmpty()) {
        return;
    }

    doCommand(Doc, "from KukaExporter import ExportFullSub");
    doCommand(Doc,
              "ExportFullSub(App.activeDocument().%s,App.activeDocument().%s,'%s')",
              pcRobotObject->getNameInDocument(),
              pcTrajectoryObject->getNameInDocument(),
              (const char*)fn.toLatin1());
}

bool CmdRobotExportKukaFull::isActive()
{
    return hasActiveDocument();
}

// #####################################################################################################


void CreateRobotCommandsExport()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdRobotExportKukaFull());
    rcCmdMgr.addCommand(new CmdRobotExportKukaCompact());
}
