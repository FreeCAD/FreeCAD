// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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


#include <string>

#include <QApplication>


#include <App/Part.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MDIView.h>


//===========================================================================
// Utils
//===========================================================================
namespace
{
QString getAutoGroupCommandStr()
// Helper function to get the python code to add the newly created object to the active Part object
// if present
{
    App::Part* activePart = Gui::Application::Instance->activeView()->getActiveObject<App::Part*>(
        "part"
    );
    if (activePart) {
        QString activePartName = QString::fromLatin1(activePart->getNameInDocument());
        return QStringLiteral(
                   "App.ActiveDocument.getObject('%1\')."
                   "addObject(App.ActiveDocument.ActiveObject)\n"
        )
            .arg(activePartName);
    }
    return QStringLiteral("# Object created at document root.");
}
}  // namespace

//===========================================================================
// Part_Cylinder
//===========================================================================
DEF_STD_CMD_A(CmdPartCylinder)

CmdPartCylinder::CmdPartCylinder()
    : Command("Part_Cylinder")
{
    sAppModule = "Part";
    sGroup = QT_TR_NOOP("Part");
    sMenuText = QT_TR_NOOP("Cylinder");
    sToolTipText = QT_TR_NOOP("Creates a solid cylinder");
    sWhatsThis = "Part_Cylinder";
    sStatusTip = sToolTipText;
    sPixmap = "Part_Cylinder_Parametric";
}

void CmdPartCylinder::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::string name = qApp->translate("CmdPartCylinder", "Cylinder").toUtf8().toStdString();
    name = Base::Tools::escapeEncodeString(name);
    openCommand(name.c_str());

    runCommand(Doc, "App.ActiveDocument.addObject(\"Part::Cylinder\",\"Cylinder\")");
    QString cmd = QStringLiteral("App.ActiveDocument.ActiveObject.Label = \"%1\"")
                      .arg(QString::fromUtf8(name.c_str()));
    runCommand(Doc, cmd.toUtf8());
    runCommand(Doc, getAutoGroupCommandStr().toUtf8());
    commitCommand();
    updateActive();
    runCommand(Gui, "Gui.SendMsgToActiveView(\"ViewFit\")");
}

bool CmdPartCylinder::isActive()
{
    if (getActiveGuiDocument()) {
        return true;
    }
    else {
        return false;
    }
}

//===========================================================================
// Part_Box
//===========================================================================
DEF_STD_CMD_A(CmdPartBox)

CmdPartBox::CmdPartBox()
    : Command("Part_Box")
{
    sAppModule = "Part";
    sGroup = QT_TR_NOOP("Part");
    sMenuText = QT_TR_NOOP("Cube");
    sToolTipText = QT_TR_NOOP("Creates a solid cube");
    sWhatsThis = "Part_Box";
    sStatusTip = sToolTipText;
    sPixmap = "Part_Box_Parametric";
}

void CmdPartBox::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::string name = qApp->translate("CmdPartBox", "Cube").toUtf8().toStdString();
    name = Base::Tools::escapeEncodeString(name);
    openCommand(name.c_str());

    runCommand(Doc, "App.ActiveDocument.addObject(\"Part::Box\",\"Box\")");
    QString cmd = QStringLiteral("App.ActiveDocument.ActiveObject.Label = \"%1\"")
                      .arg(QString::fromUtf8(name.c_str()));
    runCommand(Doc, cmd.toUtf8());
    runCommand(Doc, getAutoGroupCommandStr().toUtf8());
    commitCommand();
    updateActive();
    runCommand(Gui, "Gui.SendMsgToActiveView(\"ViewFit\")");
}

bool CmdPartBox::isActive()
{
    if (getActiveGuiDocument()) {
        return true;
    }
    else {
        return false;
    }
}

//===========================================================================
// Part_Sphere
//===========================================================================
DEF_STD_CMD_A(CmdPartSphere)

CmdPartSphere::CmdPartSphere()
    : Command("Part_Sphere")
{
    sAppModule = "Part";
    sGroup = QT_TR_NOOP("Part");
    sMenuText = QT_TR_NOOP("Sphere");
    sToolTipText = QT_TR_NOOP("Creates a solid sphere");
    sWhatsThis = "Part_Sphere";
    sStatusTip = sToolTipText;
    sPixmap = "Part_Sphere_Parametric";
}

void CmdPartSphere::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::string name = qApp->translate("CmdPartSphere", "Sphere").toUtf8().toStdString();
    name = Base::Tools::escapeEncodeString(name);
    openCommand(name.c_str());

    runCommand(Doc, "App.ActiveDocument.addObject(\"Part::Sphere\",\"Sphere\")");
    QString cmd = QStringLiteral("App.ActiveDocument.ActiveObject.Label = \"%1\"")
                      .arg(QString::fromUtf8(name.c_str()));
    runCommand(Doc, cmd.toUtf8());
    runCommand(Doc, getAutoGroupCommandStr().toUtf8());
    commitCommand();
    updateActive();
    runCommand(Gui, "Gui.SendMsgToActiveView(\"ViewFit\")");
}

bool CmdPartSphere::isActive()
{
    if (getActiveGuiDocument()) {
        return true;
    }
    else {
        return false;
    }
}

//===========================================================================
// Part_Cone
//===========================================================================
DEF_STD_CMD_A(CmdPartCone)

CmdPartCone::CmdPartCone()
    : Command("Part_Cone")
{
    sAppModule = "Part";
    sGroup = QT_TR_NOOP("Part");
    sMenuText = QT_TR_NOOP("Cone");
    sToolTipText = QT_TR_NOOP("Creates a solid cone");
    sWhatsThis = "Part_Cone";
    sStatusTip = sToolTipText;
    sPixmap = "Part_Cone_Parametric";
}

void CmdPartCone::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::string name = qApp->translate("CmdPartCone", "Cone").toUtf8().toStdString();
    name = Base::Tools::escapeEncodeString(name);
    openCommand(name.c_str());

    runCommand(Doc, "App.ActiveDocument.addObject(\"Part::Cone\",\"Cone\")");
    QString cmd = QStringLiteral("App.ActiveDocument.ActiveObject.Label = \"%1\"")
                      .arg(QString::fromUtf8(name.c_str()));
    runCommand(Doc, cmd.toUtf8());
    runCommand(Doc, getAutoGroupCommandStr().toUtf8());
    commitCommand();
    updateActive();
    runCommand(Gui, "Gui.SendMsgToActiveView(\"ViewFit\")");
}

bool CmdPartCone::isActive()
{
    if (getActiveGuiDocument()) {
        return true;
    }
    else {
        return false;
    }
}

//===========================================================================
// Part_Torus
//===========================================================================
DEF_STD_CMD_A(CmdPartTorus)

CmdPartTorus::CmdPartTorus()
    : Command("Part_Torus")
{
    sAppModule = "Part";
    sGroup = QT_TR_NOOP("Part");
    sMenuText = QT_TR_NOOP("Torus");
    sToolTipText = QT_TR_NOOP("Creates a solid torus");
    sWhatsThis = "Part_Torus";
    sStatusTip = sToolTipText;
    sPixmap = "Part_Torus_Parametric";
}

void CmdPartTorus::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::string name = qApp->translate("CmdPartTorus", "Torus").toUtf8().toStdString();
    name = Base::Tools::escapeEncodeString(name);
    openCommand(name.c_str());

    runCommand(Doc, "App.ActiveDocument.addObject(\"Part::Torus\",\"Torus\")");
    QString cmd = QStringLiteral("App.ActiveDocument.ActiveObject.Label = \"%1\"")
                      .arg(QString::fromUtf8(name.c_str()));
    runCommand(Doc, cmd.toUtf8());
    runCommand(Doc, getAutoGroupCommandStr().toUtf8());
    commitCommand();
    updateActive();
    runCommand(Gui, "Gui.SendMsgToActiveView(\"ViewFit\")");
}

bool CmdPartTorus::isActive()
{
    if (getActiveGuiDocument()) {
        return true;
    }
    else {
        return false;
    }
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void CreateParamPartCommands()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommand(new CmdPartCylinder());
    rcCmdMgr.addCommand(new CmdPartBox());
    rcCmdMgr.addCommand(new CmdPartSphere());
    rcCmdMgr.addCommand(new CmdPartCone());
    rcCmdMgr.addCommand(new CmdPartTorus());
}
