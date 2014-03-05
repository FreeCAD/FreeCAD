/***************************************************************************
 *   Copyright (c) J�rgen Riegel          (juergen.riegel@web.de) 2002     *
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
# include <QApplication>
# include <QDir>
# include <QFileInfo>
# include <QLineEdit>
#endif

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>

//===========================================================================
// Part_Cylinder
//===========================================================================
DEF_STD_CMD_A(CmdPartCylinder);

CmdPartCylinder::CmdPartCylinder()
  : Command("Part_Cylinder")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Cylinder");
    sToolTipText  = QT_TR_NOOP("Create a Cylinder");
    sWhatsThis    = sToolTipText;
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Cylinder";
}

void CmdPartCylinder::activated(int iMsg)
{
    QString cmd;
    cmd = qApp->translate("CmdPartCylinder","Cylinder");
    openCommand((const char*)cmd.toUtf8());

    doCommand(Doc,"App.ActiveDocument.addObject(\"Part::Cylinder\",\"Cylinder\")");
    cmd = QString::fromAscii("App.ActiveDocument.ActiveObject.Label = \"%1\"")
        .arg(qApp->translate("CmdPartCylinder","Cylinder"));
    doCommand(Doc,(const char*)cmd.toUtf8());
    commitCommand();
    updateActive();
    doCommand(Gui, "Gui.SendMsgToActiveView(\"ViewFit\")");
}

bool CmdPartCylinder::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

//===========================================================================
// Part_Box
//===========================================================================
DEF_STD_CMD_A(CmdPartBox);

CmdPartBox::CmdPartBox()
  : Command("Part_Box")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Cube");
    sToolTipText  = QT_TR_NOOP("Create a cube solid");
    sWhatsThis    = "Part_Box";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Box";
}

void CmdPartBox::activated(int iMsg)
{
    QString cmd;
    cmd = qApp->translate("CmdPartBox","Cube");
    openCommand((const char*)cmd.toUtf8());

    doCommand(Doc,"App.ActiveDocument.addObject(\"Part::Box\",\"Box\")");
    cmd = QString::fromAscii("App.ActiveDocument.ActiveObject.Label = \"%1\"")
        .arg(qApp->translate("CmdPartBox","Cube"));
    doCommand(Doc,(const char*)cmd.toUtf8());
    commitCommand();
    updateActive();
    doCommand(Gui, "Gui.SendMsgToActiveView(\"ViewFit\")");
}

bool CmdPartBox::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

//===========================================================================
// Part_Sphere
//===========================================================================
DEF_STD_CMD_A(CmdPartSphere);

CmdPartSphere::CmdPartSphere()
  : Command("Part_Sphere")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Sphere");
    sToolTipText  = QT_TR_NOOP("Create a sphere solid");
    sWhatsThis    = "Part_Sphere";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Sphere";
}

void CmdPartSphere::activated(int iMsg)
{
    QString cmd;
    cmd = qApp->translate("CmdPartSphere","Sphere");
    openCommand((const char*)cmd.toUtf8());

    doCommand(Doc,"App.ActiveDocument.addObject(\"Part::Sphere\",\"Sphere\")");
    cmd = QString::fromAscii("App.ActiveDocument.ActiveObject.Label = \"%1\"")
        .arg(qApp->translate("CmdPartSphere","Sphere"));
    doCommand(Doc,(const char*)cmd.toUtf8());
    commitCommand();
    updateActive();
    doCommand(Gui, "Gui.SendMsgToActiveView(\"ViewFit\")");
}

bool CmdPartSphere::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

//===========================================================================
// Part_Cone
//===========================================================================
DEF_STD_CMD_A(CmdPartCone);

CmdPartCone::CmdPartCone()
  : Command("Part_Cone")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Cone");
    sToolTipText  = QT_TR_NOOP("Create a cone solid");
    sWhatsThis    = "Part_Cone";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Cone";
}

void CmdPartCone::activated(int iMsg)
{
    QString cmd;
    cmd = qApp->translate("CmdPartCone","Cone");
    openCommand((const char*)cmd.toUtf8());

    doCommand(Doc,"App.ActiveDocument.addObject(\"Part::Cone\",\"Cone\")");
    cmd = QString::fromAscii("App.ActiveDocument.ActiveObject.Label = \"%1\"")
        .arg(qApp->translate("CmdPartCone","Cone"));
    doCommand(Doc,(const char*)cmd.toUtf8());
    commitCommand();
    updateActive();
    doCommand(Gui, "Gui.SendMsgToActiveView(\"ViewFit\")");
}

bool CmdPartCone::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

//===========================================================================
// Part_Torus
//===========================================================================
DEF_STD_CMD_A(CmdPartTorus);

CmdPartTorus::CmdPartTorus()
  : Command("Part_Torus")
{
    sAppModule    = "Part";
    sGroup        = QT_TR_NOOP("Part");
    sMenuText     = QT_TR_NOOP("Torus");
    sToolTipText  = QT_TR_NOOP("Create a torus solid");
    sWhatsThis    = "Part_Torus";
    sStatusTip    = sToolTipText;
    sPixmap       = "Part_Torus";
}

void CmdPartTorus::activated(int iMsg)
{
    QString cmd;
    cmd = qApp->translate("CmdPartTorus","Torus");
    openCommand((const char*)cmd.toUtf8());

    doCommand(Doc,"App.ActiveDocument.addObject(\"Part::Torus\",\"Torus\")");
    cmd = QString::fromAscii("App.ActiveDocument.ActiveObject.Label = \"%1\"")
        .arg(qApp->translate("CmdPartTorus","Torus"));
    doCommand(Doc,(const char*)cmd.toUtf8());
    commitCommand();
    updateActive();
    doCommand(Gui, "Gui.SendMsgToActiveView(\"ViewFit\")");
}

bool CmdPartTorus::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void CreateParamPartCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new CmdPartCylinder());
    rcCmdMgr.addCommand(new CmdPartBox());
    rcCmdMgr.addCommand(new CmdPartSphere());
    rcCmdMgr.addCommand(new CmdPartCone());
    rcCmdMgr.addCommand(new CmdPartTorus());
}
