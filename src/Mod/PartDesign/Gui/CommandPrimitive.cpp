/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger (stefantroeger@gmx.net)              *
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
#include "Workbench.h"
#include <Mod/PartDesign/App/Body.h>
#ifndef _PreComp_
# include <Inventor/nodes/SoPickStyle.h>
# include <QApplication>
#endif

#include <Gui/Command.h>
#include <Gui/Action.h>
#include <Gui/MainWindow.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Application.h>
#include <Base/Console.h>

using namespace std;

DEF_STD_CMD_ACL(CmdPrimtiveCompAdditive);

CmdPrimtiveCompAdditive::CmdPrimtiveCompAdditive()
  : Command("PartDesign_CompPrimitiveAdditive")
{
    sAppModule      = "PartDesign";
    sGroup          = QT_TR_NOOP("PartDesign");
    sMenuText       = QT_TR_NOOP("Create an additive primitive");
    sToolTipText    = QT_TR_NOOP("Create an additive primitive");
    sWhatsThis      = "Sketcher_CompPrimitiveAdditive";
    sStatusTip      = sToolTipText;
    eType           = ForEdit;
}

void CmdPrimtiveCompAdditive::activated(int iMsg)
{
    Base::Console().Message("activated msg %i\n", iMsg);
    
    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(/*messageIfNot = */true);
    if (!pcActiveBody) return;
    
    if(iMsg == 0) {
    
        std::string FeatName = getUniqueObjectName("Box");
        
        Gui::Command::openCommand("Make additive box");
        Gui::Command::doCommand(Gui::Command::Doc,"App.activeDocument().addObject(\'PartDesign::AdditiveBox\',\'%s\')",
            FeatName.c_str());
        Gui::Command::doCommand(Doc,"App.activeDocument().%s.addFeature(App.activeDocument().%s)"
                        ,pcActiveBody->getNameInDocument(), FeatName.c_str());
        Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeDocument().setEdit(\'%s\')", FeatName.c_str());       
    }
}

Gui::Action * CmdPrimtiveCompAdditive::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().pixmap("Part_Box"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().pixmap("Part_Cylinder"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(p1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdPrimtiveCompAdditive::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* arc1 = a[0];
    arc1->setText(QApplication::translate("CmdSketcherCompCreateArc","Center and end points"));
    arc1->setToolTip(QApplication::translate("Sketcher_CreateArc","Create an arc by its center and by its end points"));
    arc1->setStatusTip(QApplication::translate("Sketcher_CreateArc","Create an arc by its center and by its end points"));
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("CmdSketcherCompCreateArc","End points and rim point"));
    arc2->setToolTip(QApplication::translate("Sketcher_Create3PointArc","Create an arc by its end points and a point along the arc"));
    arc2->setStatusTip(QApplication::translate("Sketcher_Create3PointArc","Create an arc by its end points and a point along the arc"));
}

bool CmdPrimtiveCompAdditive::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

DEF_STD_CMD_ACL(CmdPrimtiveCompSubtractive);

CmdPrimtiveCompSubtractive::CmdPrimtiveCompSubtractive()
  : Command("PartDesign_CompPrimitiveSubtractive")
{
    sAppModule      = "PartDesign";
    sGroup          = QT_TR_NOOP("PartDesign");
    sMenuText       = QT_TR_NOOP("Create an subtractive primitive");
    sToolTipText    = QT_TR_NOOP("Create an subtractive primitive");
    sWhatsThis      = "PartDesign_CompPrimitiveSubtractive";
    sStatusTip      = sToolTipText;
    eType           = ForEdit;
}

void CmdPrimtiveCompSubtractive::activated(int iMsg)
{
    Base::Console().Message("activated msg %i\n", iMsg);
    return;

    // Since the default icon is reset when enabing/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action * CmdPrimtiveCompSubtractive::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().pixmap("Part_Box"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().pixmap("Part_Cylinder"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(p1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdPrimtiveCompSubtractive::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* arc1 = a[0];
    arc1->setText(QApplication::translate("CmdSketcherCompCreateArc","Center and end points"));
    arc1->setToolTip(QApplication::translate("Sketcher_CreateArc","Create an arc by its center and by its end points"));
    arc1->setStatusTip(QApplication::translate("Sketcher_CreateArc","Create an arc by its center and by its end points"));
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("CmdSketcherCompCreateArc","End points and rim point"));
    arc2->setToolTip(QApplication::translate("Sketcher_Create3PointArc","Create an arc by its end points and a point along the arc"));
    arc2->setStatusTip(QApplication::translate("Sketcher_Create3PointArc","Create an arc by its end points and a point along the arc"));
}

bool CmdPrimtiveCompSubtractive::isActive(void)
{
    if (getActiveGuiDocument())
        return true;
    else
        return false;
}

//===========================================================================
// Initialization
//===========================================================================

void CreatePartDesignPrimitiveCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdPrimtiveCompAdditive);
    rcCmdMgr.addCommand(new CmdPrimtiveCompSubtractive);
 }
