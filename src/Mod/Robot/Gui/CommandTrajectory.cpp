/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel (juergen.riegel@web.de)              *
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
# include <QInputDialog>
# include <QMessageBox>
#endif

#include <App/Application.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/FileDialog.h>
#include <Gui/Selection.h>
#include <Gui/Document.h>
#include <Gui/Placement.h>
#include <Gui/Control.h>


#include <Mod/Part/App/PartFeature.h>
#include <Mod/Robot/App/RobotObject.h>
#include <Mod/Robot/App/TrajectoryObject.h>
#include <Mod/Robot/App/Edge2TracObject.h>
#include <Mod/Robot/App/TrajectoryDressUpObject.h>
#include <Mod/Robot/App/TrajectoryCompound.h>
#include "TaskDlgEdge2Trac.h"

#include "TrajectorySimulate.h"

using namespace std;
using namespace RobotGui;

// #####################################################################################################

DEF_STD_CMD_A(CmdRobotCreateTrajectory);

CmdRobotCreateTrajectory::CmdRobotCreateTrajectory()
	:Command("Robot_CreateTrajectory")
{
    sAppModule      = "Robot";
    sGroup          = QT_TR_NOOP("Robot");
    sMenuText       = QT_TR_NOOP("Create trajectory");
    sToolTipText    = QT_TR_NOOP("Create a new empty trajectory ");
    sWhatsThis      = "Robot_CreateTrajectory";
    sStatusTip      = sToolTipText;
    sPixmap         = "Robot_CreateTrajectory";
}


void CmdRobotCreateTrajectory::activated(int)
{
    std::string FeatName = getUniqueObjectName("Trajectory");
 
    openCommand("Create trajectory");
    doCommand(Doc,"App.activeDocument().addObject(\"Robot::TrajectoryObject\",\"%s\")",FeatName.c_str());
    updateActive();
    commitCommand();
      
}

bool CmdRobotCreateTrajectory::isActive(void)
{
    return hasActiveDocument();
}

// #####################################################################################################

DEF_STD_CMD_A(CmdRobotInsertWaypoint);

CmdRobotInsertWaypoint::CmdRobotInsertWaypoint()
	:Command("Robot_InsertWaypoint")
{
    sAppModule      = "Robot";
    sGroup          = QT_TR_NOOP("Robot");
    sMenuText       = QT_TR_NOOP("Insert in trajectory");
    sToolTipText    = QT_TR_NOOP("Insert robot Tool location into trajectory");
    sWhatsThis      = "Robot_InsertWaypoint";
    sStatusTip      = sToolTipText;
    sPixmap         = "Robot_InsertWaypoint";
    sAccel          = "A";
}


void CmdRobotInsertWaypoint::activated(int)
{
    unsigned int n1 = getSelection().countObjectsOfType(Robot::RobotObject::getClassTypeId());
    unsigned int n2 = getSelection().countObjectsOfType(Robot::TrajectoryObject::getClassTypeId());
 
    if (n1 != 1 || n2 != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select one Robot and one Trajectory object."));
        return;
    }

    std::vector<Gui::SelectionSingleton::SelObj> Sel = getSelection().getSelection();

    Robot::RobotObject *pcRobotObject=0;
    if(Sel[0].pObject->getTypeId() == Robot::RobotObject::getClassTypeId())
        pcRobotObject = static_cast<Robot::RobotObject*>(Sel[0].pObject);
    else if(Sel[1].pObject->getTypeId() == Robot::RobotObject::getClassTypeId())
        pcRobotObject = static_cast<Robot::RobotObject*>(Sel[1].pObject);
    std::string RoboName = pcRobotObject->getNameInDocument();

    Robot::TrajectoryObject *pcTrajectoryObject=0;
    if(Sel[0].pObject->getTypeId() == Robot::TrajectoryObject::getClassTypeId())
        pcTrajectoryObject = static_cast<Robot::TrajectoryObject*>(Sel[0].pObject);
    else if(Sel[1].pObject->getTypeId() == Robot::TrajectoryObject::getClassTypeId())
        pcTrajectoryObject = static_cast<Robot::TrajectoryObject*>(Sel[1].pObject);
    std::string TrakName = pcTrajectoryObject->getNameInDocument();

    openCommand("Insert waypoint");
    doCommand(Doc,"App.activeDocument().%s.Trajectory = App.activeDocument().%s.Trajectory.insertWaypoints(Robot.Waypoint(App.activeDocument().%s.Tcp.multiply(App.activeDocument().%s.Tool),type='LIN',name='Pt',vel=_DefSpeed,cont=_DefCont,acc=_DefAccelaration,tool=1))",TrakName.c_str(),TrakName.c_str(),RoboName.c_str(),RoboName.c_str());
    updateActive();
    commitCommand();
      
}

bool CmdRobotInsertWaypoint::isActive(void)
{
    return hasActiveDocument();
}

// #####################################################################################################

DEF_STD_CMD_A(CmdRobotInsertWaypointPreselect);

CmdRobotInsertWaypointPreselect::CmdRobotInsertWaypointPreselect()
	:Command("Robot_InsertWaypointPreselect")
{
    sAppModule      = "Robot";
    sGroup          = QT_TR_NOOP("Robot");
    sMenuText       = QT_TR_NOOP("Insert in trajectory");
    sToolTipText    = QT_TR_NOOP("Insert preselection position into trajectory (W)");
    sWhatsThis      = "Robot_InsertWaypointPreselect";
    sStatusTip      = sToolTipText;
    sPixmap         = "Robot_InsertWaypointPre";
    sAccel          = "W";

}


void CmdRobotInsertWaypointPreselect::activated(int)
{
    
    if (getSelection().size() != 1 ) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select one Trajectory object."));
        return;
    }

    std::vector<Gui::SelectionSingleton::SelObj> Sel = getSelection().getSelection();

    const Gui::SelectionChanges & PreSel = getSelection().getPreselection();
    float x = PreSel.x;
    float y = PreSel.y;
    float z = PreSel.z;


    Robot::TrajectoryObject *pcTrajectoryObject;
    if(Sel[0].pObject->getTypeId() == Robot::TrajectoryObject::getClassTypeId())
        pcTrajectoryObject = static_cast<Robot::TrajectoryObject*>(Sel[0].pObject);
    else  {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select one Trajectory object."));
        return;
    }
    std::string TrakName = pcTrajectoryObject->getNameInDocument();

    if(PreSel.pDocName == 0){
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No preselection"),
            QObject::tr("You have to hover above a geometry (Preselection) with the mouse to use this command. See documentation for details."));
        return;
    }

    openCommand("Insert waypoint");
    doCommand(Doc,"App.activeDocument().%s.Trajectory = App.activeDocument().%s.Trajectory.insertWaypoints(Robot.Waypoint(FreeCAD.Placement(FreeCAD.Vector(%f,%f,%f)+_DefDisplacement,_DefOrientation),type='LIN',name='Pt',vel=_DefSpeed,cont=_DefCont,acc=_DefAccelaration,tool=1))",TrakName.c_str(),TrakName.c_str(),x,y,z);
    updateActive();
    commitCommand();
      
}

bool CmdRobotInsertWaypointPreselect::isActive(void)
{
    return hasActiveDocument();
}

// #####################################################################################################

DEF_STD_CMD_A(CmdRobotSetDefaultOrientation);

CmdRobotSetDefaultOrientation::CmdRobotSetDefaultOrientation()
	:Command("Robot_SetDefaultOrientation")
{
    sAppModule      = "Robot";
    sGroup          = QT_TR_NOOP("Robot");
    sMenuText       = QT_TR_NOOP("Set default orientation");
    sToolTipText    = QT_TR_NOOP("Set the default orientation for subsequent commands for waypoint creation");
    sWhatsThis      = "Robot_SetDefaultOrientation";
    sStatusTip      = sToolTipText;
    sPixmap         = 0;

}


void CmdRobotSetDefaultOrientation::activated(int)
{
    // create placement dialog 
    Gui::Dialog::Placement *Dlg = new Gui::Dialog::Placement();
    Base::Placement place;
    Dlg->setPlacement(place);
    if(Dlg->exec() == QDialog::Accepted ){
        place = Dlg->getPlacement();
        Base::Rotation rot = place.getRotation();
        Base::Vector3d disp = place.getPosition();
        doCommand(Doc,"_DefOrientation = FreeCAD.Rotation(%f,%f,%f,%f)",rot[0],rot[1],rot[2],rot[3]);
        doCommand(Doc,"_DefDisplacement = FreeCAD.Vector(%f,%f,%f)",disp[0],disp[1],disp[2]);
    }
      
}

bool CmdRobotSetDefaultOrientation::isActive(void)
{
    return true;
}

// #####################################################################################################

DEF_STD_CMD_A(CmdRobotSetDefaultValues);

CmdRobotSetDefaultValues::CmdRobotSetDefaultValues()
	:Command("Robot_SetDefaultValues")
{
    sAppModule      = "Robot";
    sGroup          = QT_TR_NOOP("Robot");
    sMenuText       = QT_TR_NOOP("Set default values");
    sToolTipText    = QT_TR_NOOP("Set the default values for speed, acceleration and continuity for subsequent commands of waypoint creation");
    sWhatsThis      = "Robot_SetDefaultValues";
    sStatusTip      = sToolTipText;
    sPixmap         = 0;

}


void CmdRobotSetDefaultValues::activated(int)
{

    bool ok;
    QString text = QInputDialog::getText(0, QObject::tr("Set default speed"),
                                          QObject::tr("speed: (e.g. 1 m/s or 3 cm/s)"), QLineEdit::Normal,
                                          QString::fromLatin1("1 m/s"), &ok);
    if ( ok && !text.isEmpty() ) {
        doCommand(Doc,"_DefSpeed = '%s'",text.toLatin1().constData());
    } 

    QStringList items;
    items  << QString::fromLatin1("False") << QString::fromLatin1("True");

    QString item = QInputDialog::getItem(0, QObject::tr("Set default continuity"),
                                          QObject::tr("continuous ?"), items, 0, false, &ok);
    if (ok && !item.isEmpty())
        doCommand(Doc,"_DefCont = %s",item.toLatin1().constData());

    text.clear();

    text = QInputDialog::getText(0, QObject::tr("Set default acceleration"),
                                          QObject::tr("acceleration: (e.g. 1 m/s^2 or 3 cm/s^2)"), QLineEdit::Normal,
                                          QString::fromLatin1("1 m/s^2"), &ok);
    if ( ok && !text.isEmpty() ) {
        doCommand(Doc,"_DefAccelaration = '%s'",text.toLatin1().constData());
    } 


    // create placement dialog 
    //Gui::Dialog::Placement *Dlg = new Gui::Dialog::Placement();
    //Base::Placement place;
    //Dlg->setPlacement(place);
    //if(Dlg->exec() == QDialog::Accepted ){
    //    place = Dlg->getPlacement();
    //    Base::Rotation rot = place.getRotation();
    //    Base::Vector3d disp = place.getPosition();
    //    doCommand(Doc,"_DefOrientation = FreeCAD.Rotation(%f,%f,%f,%f)",rot[0],rot[1],rot[2],rot[3]);
    //    doCommand(Doc,"_DefDisplacement = FreeCAD.Vector(%f,%f,%f)",disp[0],disp[1],disp[2]);
    //}
      
}

bool CmdRobotSetDefaultValues::isActive(void)
{
    return true;
}

// #####################################################################################################

DEF_STD_CMD_A(CmdRobotEdge2Trac);

CmdRobotEdge2Trac::CmdRobotEdge2Trac()
	:Command("Robot_Edge2Trac")
{
    sAppModule      = "Robot";
    sGroup          = QT_TR_NOOP("Robot");
    sMenuText       = QT_TR_NOOP("Edge to Trajectory...");
    sToolTipText    = QT_TR_NOOP("Generate a Trajectory from a set of edges");
    sWhatsThis      = "Robot_Edge2Trac";
    sStatusTip      = sToolTipText;
    sPixmap         = "Robot_Edge2Trac";

}


void CmdRobotEdge2Trac::activated(int)
{
     
 /*   App::DocumentObject *obj = this->getDocument()->getObject(FeatName.c_str());
    App::Property *prop = &(dynamic_cast<Robot::Edge2TracObject *>(obj)->Source); 

    Gui::TaskView::TaskDialog* dlg = new TaskDlgEdge2Trac(dynamic_cast<Robot::Edge2TracObject *>(obj));
    Gui::Control().showDialog(dlg);*/


    Gui::SelectionFilter ObjectFilter("SELECT Robot::Edge2TracObject COUNT 1");
    Gui::SelectionFilter EdgeFilter  ("SELECT Part::Feature SUBELEMENT Edge COUNT 1..");

    if (ObjectFilter.match()) {
        Robot::Edge2TracObject *EdgeObj = static_cast<Robot::Edge2TracObject*>(ObjectFilter.Result[0][0].getObject());
        openCommand("Edit Edge2TracObject");
        doCommand(Gui,"Gui.activeDocument().setEdit('%s')",EdgeObj->getNameInDocument());
    }else if (EdgeFilter.match()) {
        // get the selected object
        //Part::Feature *part = static_cast<Part::Feature*>(EdgeFilter.Result[0][0].getObject());
        std::string obj_sub = EdgeFilter.Result[0][0].getAsPropertyLinkSubString();

        std::string FeatName = getUniqueObjectName("Edge2Trac");

        openCommand("Create a new Edge2TracObject");
        doCommand(Doc,"App.activeDocument().addObject('Robot::Edge2TracObject','%s')",FeatName.c_str());
        doCommand(Gui,"App.activeDocument().%s.Source = %s",FeatName.c_str(),obj_sub.c_str());
        doCommand(Gui,"Gui.activeDocument().setEdit('%s')",FeatName.c_str());

    }else {
        std::string FeatName = getUniqueObjectName("Edge2Trac");

        openCommand("Create a new Edge2TracObject");
        doCommand(Doc,"App.activeDocument().addObject('Robot::Edge2TracObject','%s')",FeatName.c_str());
        doCommand(Gui,"Gui.activeDocument().setEdit('%s')",FeatName.c_str());
    }
 


}

bool CmdRobotEdge2Trac::isActive(void)
{
    return true;
}

// #####################################################################################################

DEF_STD_CMD_A(CmdRobotTrajectoryDressUp);

CmdRobotTrajectoryDressUp::CmdRobotTrajectoryDressUp()
	:Command("Robot_TrajectoryDressUp")
{
    sAppModule      = "Robot";
    sGroup          = QT_TR_NOOP("Robot");
    sMenuText       = QT_TR_NOOP("Dress-up trajectory...");
    sToolTipText    = QT_TR_NOOP("Create a dress-up object which overrides some aspects of a trajectory");
    sWhatsThis      = "Robot_TrajectoryDressUp";
    sStatusTip      = sToolTipText;
    sPixmap         = "Robot_TrajectoryDressUp";

}


void CmdRobotTrajectoryDressUp::activated(int)
{
    Gui::SelectionFilter ObjectFilterDressUp("SELECT Robot::TrajectoryDressUpObject COUNT 1");
    Gui::SelectionFilter ObjectFilter("SELECT Robot::TrajectoryObject COUNT 1");

    if (ObjectFilterDressUp.match()) {
        Robot::TrajectoryDressUpObject *Object = static_cast<Robot::TrajectoryDressUpObject*>(ObjectFilterDressUp.Result[0][0].getObject());
        openCommand("Edit Sketch");
        doCommand(Gui,"Gui.activeDocument().setEdit('%s')",Object->getNameInDocument());
    }else if (ObjectFilter.match()) {
        std::string FeatName = getUniqueObjectName("DressUpObject");
        Robot::TrajectoryObject *Object = static_cast<Robot::TrajectoryObject*>(ObjectFilter.Result[0][0].getObject());
        openCommand("Create a new TrajectoryDressUp");
        doCommand(Doc,"App.activeDocument().addObject('Robot::TrajectoryDressUpObject','%s')",FeatName.c_str());
        doCommand(Gui,"App.activeDocument().%s.Source = App.activeDocument().%s",FeatName.c_str(),Object->getNameInDocument());
        doCommand(Gui,"Gui.activeDocument().hide(\"%s\")",Object->getNameInDocument());
        doCommand(Gui,"Gui.activeDocument().setEdit('%s')",FeatName.c_str());
    }else {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select the Trajectory which you want to dress up."));
        return;
    }
 }

bool CmdRobotTrajectoryDressUp::isActive(void)
{
    return true;
}

// #####################################################################################################

DEF_STD_CMD_A(CmdRobotTrajectoryCompound);

CmdRobotTrajectoryCompound::CmdRobotTrajectoryCompound()
	:Command("Robot_TrajectoryCompound")
{
    sAppModule      = "Robot";
    sGroup          = QT_TR_NOOP("Robot");
    sMenuText       = QT_TR_NOOP("Trajectory compound...");
    sToolTipText    = QT_TR_NOOP("Group and connect some trajectories to one");
    sWhatsThis      = "Robot_TrajectoryCompound";
    sStatusTip      = sToolTipText;
    sPixmap         = "Robot_TrajectoryCompound";

}


void CmdRobotTrajectoryCompound::activated(int)
{
    Gui::SelectionFilter ObjectFilter("SELECT Robot::TrajectoryCompound COUNT 1");

    if (ObjectFilter.match()) {
        Robot::TrajectoryCompound *Object = static_cast<Robot::TrajectoryCompound*>(ObjectFilter.Result[0][0].getObject());
        openCommand("Edit TrajectoryCompound");
        doCommand(Gui,"Gui.activeDocument().setEdit('%s')",Object->getNameInDocument());
    }else {
        std::string FeatName = getUniqueObjectName("TrajectoryCompound");

        openCommand("Create a new TrajectoryDressUp");
        doCommand(Doc,"App.activeDocument().addObject('Robot::TrajectoryCompound','%s')",FeatName.c_str());
        doCommand(Gui,"Gui.activeDocument().setEdit('%s')",FeatName.c_str());
    }
}

bool CmdRobotTrajectoryCompound::isActive(void)
{
    return true;
}



// #####################################################################################################



void CreateRobotCommandsTrajectory(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdRobotCreateTrajectory());
    rcCmdMgr.addCommand(new CmdRobotInsertWaypoint());
    rcCmdMgr.addCommand(new CmdRobotInsertWaypointPreselect());
    rcCmdMgr.addCommand(new CmdRobotSetDefaultOrientation());
    rcCmdMgr.addCommand(new CmdRobotSetDefaultValues());
    rcCmdMgr.addCommand(new CmdRobotEdge2Trac());
    rcCmdMgr.addCommand(new CmdRobotTrajectoryDressUp());
    rcCmdMgr.addCommand(new CmdRobotTrajectoryCompound());
 }
