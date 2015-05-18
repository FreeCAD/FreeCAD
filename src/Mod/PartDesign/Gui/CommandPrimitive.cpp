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
    
    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(/*messageIfNot = */true);
    if (!pcActiveBody) return;
    
    std::string FeatName;
    std::string CSName = getUniqueObjectName("CoordinateSystem");;
    if(iMsg == 0) {
    
        FeatName = getUniqueObjectName("Box");
      
        Gui::Command::openCommand("Make additive box");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.addObject(\'PartDesign::AdditiveBox\',\'%s\')",
            FeatName.c_str());
    }
    else if(iMsg == 1) {
    
        FeatName = getUniqueObjectName("Cylinder");
         
        Gui::Command::openCommand("Make additive cylinder");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.addObject(\'PartDesign::AdditiveCylinder\',\'%s\')",
            FeatName.c_str());
    }
    else if(iMsg == 3) {
    
        FeatName = getUniqueObjectName("Sphere");
         
        Gui::Command::openCommand("Make additive sphere");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.addObject(\'PartDesign::AdditiveSphere\',\'%s\')",
            FeatName.c_str());
    }
    
    Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.addFeature(App.activeDocument().%s)"
                    ,pcActiveBody->getNameInDocument(), FeatName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.addObject(\'PartDesign::CoordinateSystem\',\'%s\')",
        CSName.c_str());
    Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.addFeature(App.activeDocument().%s)"
                    ,pcActiveBody->getNameInDocument(), CSName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.CoordinateSystem=(App.ActiveDocument.%s)",
        FeatName.c_str(), CSName.c_str());
    Gui::Command::updateActive();
    
    if (isActiveObjectValid() && (pcActiveBody != NULL)) {
        App::DocumentObject* prevSolidFeature = pcActiveBody->getPrevSolidFeature(NULL, false);
        if (prevSolidFeature != NULL && strcmp(prevSolidFeature->getNameInDocument(), FeatName.c_str())!=0)
            doCommand(Gui,"Gui.activeDocument().hide(\"%s\")", prevSolidFeature->getNameInDocument());
    }

    Gui::Command::doCommand(Gui, "Gui.activeDocument().hide(\'%s\')", CSName.c_str());
    Gui::Command::doCommand(Gui, "Gui.activeDocument().setEdit(\'%s\')", FeatName.c_str());    
}

Gui::Action * CmdPrimtiveCompAdditive::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().pixmap("PartDesign_Additive_Box"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().pixmap("PartDesign_Additive_Cylinder"));
    QAction* p3 = pcAction->addAction(QString());
    p3->setIcon(Gui::BitmapFactory().pixmap("PartDesign_Additive_Sphere"));

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
    arc1->setText(QApplication::translate("CmdPrimtiveCompAdditive","Additive Box"));
    arc1->setToolTip(QApplication::translate("PartDesign_CompPrimitiveAdditive","Create an additive box by its with, height and length"));
    arc1->setStatusTip(arc1->toolTip());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("CmdPrimtiveCompAdditive","Additive Cylinder"));
    arc2->setToolTip(QApplication::translate("PartDesign_CompPrimitiveAdditive","Create an additive cylinder by its radius, height and angle"));
    arc2->setStatusTip(arc2->toolTip());
    QAction* arc3 = a[2];
    arc3->setText(QApplication::translate("CmdPrimtiveCompAdditive","Additive Sphere"));
    arc3->setToolTip(QApplication::translate("PartDesign_CompPrimitiveAdditive","Create an additive sphere by its radius and varius angles"));
    arc3->setStatusTip(arc3->toolTip());
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
    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(/*messageIfNot = */true);
    if (!pcActiveBody) return;
    
    std::string FeatName;
    std::string CSName = getUniqueObjectName("CoordinateSystem");
    if(iMsg == 0) {
    
        FeatName = getUniqueObjectName("Box");
                
        Gui::Command::openCommand("Make subtractive box");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.addObject(\'PartDesign::SubtractiveBox\',\'%s\')",
            FeatName.c_str());
    }
    else if(iMsg == 1) {
    
        FeatName = getUniqueObjectName("Cylinder");
                
        Gui::Command::openCommand("Make subtractive cylinder");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.addObject(\'PartDesign::SubtractiveCylinder\',\'%s\')",
            FeatName.c_str());
    }
    else if(iMsg == 2) {
    
        FeatName = getUniqueObjectName("Sphere");
                
        Gui::Command::openCommand("Make subtractive sphere");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.addObject(\'PartDesign::SubtractiveSphere\',\'%s\')",
            FeatName.c_str());
    }
    
    Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.addFeature(App.activeDocument().%s)"
                    ,pcActiveBody->getNameInDocument(), FeatName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.addObject(\'PartDesign::CoordinateSystem\',\'%s\')",
        CSName.c_str());
    Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.addFeature(App.activeDocument().%s)"
                    ,pcActiveBody->getNameInDocument(), CSName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.CoordinateSystem=(App.ActiveDocument.%s)",
        FeatName.c_str(), CSName.c_str());
    Gui::Command::updateActive();
    
    if (isActiveObjectValid() && (pcActiveBody != NULL)) {
        App::DocumentObject* prevSolidFeature = pcActiveBody->getPrevSolidFeature(NULL, false);
        if (prevSolidFeature != NULL && strcmp(prevSolidFeature->getNameInDocument(), FeatName.c_str())!=0)
            doCommand(Gui,"Gui.activeDocument().hide(\"%s\")", prevSolidFeature->getNameInDocument());
    }

    Gui::Command::doCommand(Gui, "Gui.activeDocument().hide(\'%s\')", CSName.c_str());
    Gui::Command::doCommand(Gui, "Gui.activeDocument().setEdit(\'%s\')", FeatName.c_str()); 
}

Gui::Action * CmdPrimtiveCompSubtractive::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().pixmap("PartDesign_Subtractive_Box"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().pixmap("PartDesign_Subtractive_Cylinder"));
    QAction* p3 = pcAction->addAction(QString());
    p3->setIcon(Gui::BitmapFactory().pixmap("PartDesign_Subtractive_Sphere"));

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
    arc1->setText(QApplication::translate("CmdPrimtiveCompSubtractive","Subtractive Box"));
    arc1->setToolTip(QApplication::translate("PartDesign_CompPrimitiveSubtractive","Create an subtractive box by its with, height and length"));
    arc1->setStatusTip(arc1->toolTip());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("CmdPrimtiveCompSubtractive","Subtractive Cylinder"));
    arc2->setToolTip(QApplication::translate("PartDesign_CompPrimitiveSubtractive","Create an subtractive cylinder by its radius, height and angle"));
    arc2->setStatusTip(arc2->toolTip());
    QAction* arc3 = a[2];
    arc3->setText(QApplication::translate("CmdPrimtiveCompSubtractive","Subtractive Sphere"));
    arc3->setToolTip(QApplication::translate("PartDesign_CompPrimitiveSubtractive","Create an subtractive sphere by its radius and varius angles"));
    arc3->setStatusTip(arc3->toolTip());
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
