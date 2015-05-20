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
#include <Mod/PartDesign/App/FeaturePrimitive.h>
#ifndef _PreComp_
# include <Inventor/nodes/SoPickStyle.h>
# include <QApplication>
# include <QMessageBox>
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
    else if(iMsg == 2) {
    
        FeatName = getUniqueObjectName("Sphere");
         
        Gui::Command::openCommand("Make additive sphere");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.addObject(\'PartDesign::AdditiveSphere\',\'%s\')",
            FeatName.c_str());
    }
    else if(iMsg == 3) {
    
        FeatName = getUniqueObjectName("Cone");
         
        Gui::Command::openCommand("Make additive cone");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.addObject(\'PartDesign::AdditiveCone\',\'%s\')",
            FeatName.c_str());
    }
    else if(iMsg == 4) {
    
        FeatName = getUniqueObjectName("Ellipsoid");
         
        Gui::Command::openCommand("Make additive ellipsoid");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.addObject(\'PartDesign::AdditiveEllipsoid\',\'%s\')",
            FeatName.c_str());
    }
    else if(iMsg == 5) {
    
        FeatName = getUniqueObjectName("Torus");
         
        Gui::Command::openCommand("Make additive torus");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.addObject(\'PartDesign::AdditiveTorus\',\'%s\')",
            FeatName.c_str());
    }
    else if(iMsg == 6) {
    
        FeatName = getUniqueObjectName("Prism");
         
        Gui::Command::openCommand("Make additive prism");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.addObject(\'PartDesign::AdditivePrism\',\'%s\')",
            FeatName.c_str());
    }
    else if(iMsg == 7) {
    
        FeatName = getUniqueObjectName("Wedge");
         
        Gui::Command::openCommand("Make additive wedge");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.addObject(\'PartDesign::AdditiveWedge\',\'%s\')",
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
    
    auto* prm = static_cast<PartDesign::FeaturePrimitive*>(getDocument()->getObject(FeatName.c_str()));
    if (prm->BaseFeature.getValue())
       doCommand(Gui,"Gui.activeDocument().hide(\"%s\")", prm->BaseFeature.getValue()->getNameInDocument());

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
    QAction* p4 = pcAction->addAction(QString());
    p4->setIcon(Gui::BitmapFactory().pixmap("PartDesign_Additive_Cone"));
    QAction* p5 = pcAction->addAction(QString());
    p5->setIcon(Gui::BitmapFactory().pixmap("PartDesign_Additive_Ellipsoid"));
    QAction* p6 = pcAction->addAction(QString());
    p6->setIcon(Gui::BitmapFactory().pixmap("PartDesign_Additive_Torus"));
    QAction* p7 = pcAction->addAction(QString());
    p7->setIcon(Gui::BitmapFactory().pixmap("PartDesign_Additive_Prism"));
    QAction* p8 = pcAction->addAction(QString());
    p8->setIcon(Gui::BitmapFactory().pixmap("PartDesign_Additive_Wedge"));

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
    QAction* arc4 = a[3];
    arc4->setText(QApplication::translate("CmdPrimtiveCompAdditive","Additive Cone"));
    arc4->setToolTip(QApplication::translate("PartDesign_CompPrimitiveAdditive","Create an additive cone"));
    arc4->setStatusTip(arc4->toolTip());
    QAction* arc5 = a[4];
    arc5->setText(QApplication::translate("CmdPrimtiveCompAdditive","Additive Ellipsoid"));
    arc5->setToolTip(QApplication::translate("PartDesign_CompPrimitiveAdditive","Create an additive ellipsoid"));
    arc5->setStatusTip(arc5->toolTip());
    QAction* arc6 = a[5];
    arc6->setText(QApplication::translate("CmdPrimtiveCompAdditive","Additive Torus"));
    arc6->setToolTip(QApplication::translate("PartDesign_CompPrimitiveAdditive","Create an additive torus"));
    arc6->setStatusTip(arc6->toolTip());
    QAction* arc7 = a[6];
    arc7->setText(QApplication::translate("CmdPrimtiveCompAdditive","Additive Prism"));
    arc7->setToolTip(QApplication::translate("PartDesign_CompPrimitiveAdditive","Create an additive prism"));
    arc7->setStatusTip(arc7->toolTip());
    QAction* arc8 = a[7];
    arc8->setText(QApplication::translate("CmdPrimtiveCompAdditive","Additive Wedge"));
    arc8->setToolTip(QApplication::translate("PartDesign_CompPrimitiveAdditive","Create an additive wedge"));
    arc8->setStatusTip(arc8->toolTip());
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
    
    //check if we already have a feature as subtractive ones work only if we have 
    //something to subtract from.
    if(!pcActiveBody->getPrevSolidFeature()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No previous feature found"),
                QObject::tr("It is not possible to create a subtractive feature without a base feature available"));
            return;
    }
    
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
    else if(iMsg == 3) {
    
        FeatName = getUniqueObjectName("Cone");
         
        Gui::Command::openCommand("Make subtractive cone");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.addObject(\'PartDesign::SubtractiveCone\',\'%s\')",
            FeatName.c_str());
    }
    else if(iMsg == 4) {
    
        FeatName = getUniqueObjectName("Ellipsoid");
         
        Gui::Command::openCommand("Make subtractive ellipsoid");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.addObject(\'PartDesign::SubtractiveEllipsoid\',\'%s\')",
            FeatName.c_str());
    }
    else if(iMsg == 5) {
    
        FeatName = getUniqueObjectName("Torus");
         
        Gui::Command::openCommand("Make subtractive torus");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.addObject(\'PartDesign::SubtractiveTorus\',\'%s\')",
            FeatName.c_str());
    }
    else if(iMsg == 6) {
    
        FeatName = getUniqueObjectName("Prism");
         
        Gui::Command::openCommand("Make subtractive prism");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.addObject(\'PartDesign::SubtractivePrism\',\'%s\')",
            FeatName.c_str());
    }
    else if(iMsg == 7) {
    
        FeatName = getUniqueObjectName("Wedge");
         
        Gui::Command::openCommand("Make subtractive wedge");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.addObject(\'PartDesign::SubtractiveWedge\',\'%s\')",
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
    QAction* p4 = pcAction->addAction(QString());
    p4->setIcon(Gui::BitmapFactory().pixmap("PartDesign_Subtractive_Cone"));
    QAction* p5 = pcAction->addAction(QString());
    p5->setIcon(Gui::BitmapFactory().pixmap("PartDesign_Subtractive_Ellipsoid"));
    QAction* p6 = pcAction->addAction(QString());
    p6->setIcon(Gui::BitmapFactory().pixmap("PartDesign_Subtractive_Torus"));
    QAction* p7 = pcAction->addAction(QString());
    p7->setIcon(Gui::BitmapFactory().pixmap("PartDesign_Subtractive_Prism"));
    QAction* p8 = pcAction->addAction(QString());
    p8->setIcon(Gui::BitmapFactory().pixmap("PartDesign_Subtractive_Wedge"));

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
    QAction* arc4 = a[3];
    arc4->setText(QApplication::translate("CmdPrimtiveCompSubtractive","Subtractive Cone"));
    arc4->setToolTip(QApplication::translate("PartDesign_CompPrimitiveSubtractive","Create an subtractive cone"));
    arc4->setStatusTip(arc4->toolTip());
    QAction* arc5 = a[4];
    arc5->setText(QApplication::translate("CmdPrimtiveCompSubtractive","Subtractive Ellipsoid"));
    arc5->setToolTip(QApplication::translate("PartDesign_CompPrimitiveSubtractive","Create an subtractive ellipsoid"));
    arc5->setStatusTip(arc5->toolTip());
    QAction* arc6 = a[5];
    arc6->setText(QApplication::translate("CmdPrimtiveCompSubtractive","Subtractive Torus"));
    arc6->setToolTip(QApplication::translate("PartDesign_CompPrimitiveSubtractive","Create an subtractive torus"));
    arc6->setStatusTip(arc6->toolTip());
    QAction* arc7 = a[6];
    arc7->setText(QApplication::translate("CmdPrimtiveCompSubtractive","Subtractive Prism"));
    arc7->setToolTip(QApplication::translate("PartDesign_CompPrimitiveSubtractive","Create an subtractive prism"));
    arc7->setStatusTip(arc7->toolTip());
    QAction* arc8 = a[7];
    arc8->setText(QApplication::translate("CmdPrimtiveCompSubtractive","Subtractive Wedge"));
    arc8->setToolTip(QApplication::translate("PartDesign_CompPrimitiveSubtractive","Create an subtractive wedge"));
    arc8->setStatusTip(arc8->toolTip());
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
