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
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeaturePrimitive.h>
#ifndef _PreComp_
# include <Inventor/nodes/SoPickStyle.h>
# include <QApplication>
# include <QMessageBox>
#endif

#include <App/Document.h>
#include <Gui/Command.h>
#include <Gui/Action.h>
#include <Gui/Control.h>
#include <Gui/MainWindow.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Application.h>
#include <Base/Console.h>

#include "Utils.h"
#include "WorkflowManager.h"

using namespace std;

DEF_STD_CMD_ACL(CmdPrimtiveCompAdditive);

static const char * primitiveIntToName(int id)
{
    switch(id) {
        case 0:  return "Box";
        case 1:  return "Cylinder";
        case 2:  return "Sphere";
        case 3:  return "Cone";
        case 4:  return "Ellipsoid";
        case 5:  return "Torus";
        case 6:  return "Prism";
        case 7:  return "Wedge";
        default: return nullptr;
    };
};

CmdPrimtiveCompAdditive::CmdPrimtiveCompAdditive()
  : Command("PartDesign_CompPrimitiveAdditive")
{
    sAppModule      = "PartDesign";
    sGroup          = QT_TR_NOOP("PartDesign");
    sMenuText       = QT_TR_NOOP("Create an additive primitive");
    sToolTipText    = QT_TR_NOOP("Create an additive primitive");
    sWhatsThis      = "PartDesign_CompPrimitiveAdditive";
    sStatusTip      = sToolTipText;
    eType           = ForEdit;
}

void CmdPrimtiveCompAdditive::activated(int iMsg)
{
    App::Document *doc = getDocument();
    if (!PartDesignGui::assureModernWorkflow(doc))
        return;

    // We need either an active Body, or for there to be no Body objects
    // (in which case, just make one) to make a new additive shape.

    PartDesign::Body *pcActiveBody = PartDesignGui::getBody( /* messageIfNot = */ false );

    auto shouldMakeBody( false );
    if (pcActiveBody == nullptr) {
        if ( doc->getObjectsOfType(PartDesign::Body::getClassTypeId()).empty() ) {
            shouldMakeBody = true;
        } else {
            PartDesignGui::needActiveBodyError();
            return;
        }
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());

    auto shapeType( primitiveIntToName(iMsg) );

    Gui::Command::openCommand( (std::string("Make additive ") + shapeType).c_str() );
    if (shouldMakeBody) {
        pcActiveBody = PartDesignGui::makeBody(doc);
    }

    if (pcActiveBody == nullptr) {
        return;
    }

    auto FeatName( getUniqueObjectName(shapeType, pcActiveBody) );

    FCMD_OBJ_DOC_CMD(pcActiveBody,"addObject('PartDesign::Additive"<<shapeType<<"','"<<FeatName<<"')");

    auto* prm = static_cast<PartDesign::FeaturePrimitive*>(
            pcActiveBody->getDocument()->getObject(FeatName.c_str()));

    if(!prm) return;
    FCMD_OBJ_CMD(pcActiveBody,"addObject("<<getObjectCmd(prm)<<")");
    Gui::Command::updateActive();

    auto base = prm->BaseFeature.getValue();
    FCMD_OBJ_HIDE(base);

    if(!base)
        base = pcActiveBody;
    copyVisual(prm, "ShapeColor", base);
    copyVisual(prm, "LineColor", base);
    copyVisual(prm, "PointColor", base);
    copyVisual(prm, "Transparency", base);
    copyVisual(prm, "DisplayMode", base);
    
    PartDesignGui::setEdit(prm,pcActiveBody);
}

Gui::Action * CmdPrimtiveCompAdditive::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("PartDesign_Additive_Box"));
    p1->setObjectName(QString::fromLatin1("PartDesign_AdditiveBox"));
    p1->setWhatsThis(QString::fromLatin1("PartDesign_AdditiveBox"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("PartDesign_Additive_Cylinder"));
    p2->setObjectName(QString::fromLatin1("PartDesign_AdditiveCylinder"));
    p2->setWhatsThis(QString::fromLatin1("PartDesign_AdditiveCylinder"));
    QAction* p3 = pcAction->addAction(QString());
    p3->setIcon(Gui::BitmapFactory().iconFromTheme("PartDesign_Additive_Sphere"));
    p3->setObjectName(QString::fromLatin1("PartDesign_AdditiveSphere"));
    p3->setWhatsThis(QString::fromLatin1("PartDesign_AdditiveSphere"));
    QAction* p4 = pcAction->addAction(QString());
    p4->setIcon(Gui::BitmapFactory().iconFromTheme("PartDesign_Additive_Cone"));
    p4->setObjectName(QString::fromLatin1("PartDesign_AdditiveCone"));
    p4->setWhatsThis(QString::fromLatin1("PartDesign_AdditiveCone"));
    QAction* p5 = pcAction->addAction(QString());
    p5->setIcon(Gui::BitmapFactory().iconFromTheme("PartDesign_Additive_Ellipsoid"));
    p5->setObjectName(QString::fromLatin1("PartDesign_AdditiveEllipsoid"));
    p5->setWhatsThis(QString::fromLatin1("PartDesign_AdditiveEllipsoid"));
    QAction* p6 = pcAction->addAction(QString());
    p6->setIcon(Gui::BitmapFactory().iconFromTheme("PartDesign_Additive_Torus"));
    p6->setObjectName(QString::fromLatin1("PartDesign_AdditiveTorus"));
    p6->setWhatsThis(QString::fromLatin1("PartDesign_AdditiveTorus"));
    QAction* p7 = pcAction->addAction(QString());
    p7->setIcon(Gui::BitmapFactory().iconFromTheme("PartDesign_Additive_Prism"));
    p7->setObjectName(QString::fromLatin1("PartDesign_AdditivePrism"));
    p7->setWhatsThis(QString::fromLatin1("PartDesign_AdditivePrism"));
    QAction* p8 = pcAction->addAction(QString());
    p8->setIcon(Gui::BitmapFactory().iconFromTheme("PartDesign_Additive_Wedge"));
    p8->setObjectName(QString::fromLatin1("PartDesign_AdditiveWedge"));
    p8->setWhatsThis(QString::fromLatin1("PartDesign_AdditiveWedge"));

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
    arc1->setToolTip(QApplication::translate("PartDesign_CompPrimitiveAdditive","Create an additive box by its width, height, and length"));
    arc1->setStatusTip(arc1->toolTip());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("CmdPrimtiveCompAdditive","Additive Cylinder"));
    arc2->setToolTip(QApplication::translate("PartDesign_CompPrimitiveAdditive","Create an additive cylinder by its radius, height, and angle"));
    arc2->setStatusTip(arc2->toolTip());
    QAction* arc3 = a[2];
    arc3->setText(QApplication::translate("CmdPrimtiveCompAdditive","Additive Sphere"));
    arc3->setToolTip(QApplication::translate("PartDesign_CompPrimitiveAdditive","Create an additive sphere by its radius and various angles"));
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
    return (hasActiveDocument() && !Gui::Control().activeDialog());
}

DEF_STD_CMD_ACL(CmdPrimtiveCompSubtractive);

CmdPrimtiveCompSubtractive::CmdPrimtiveCompSubtractive()
  : Command("PartDesign_CompPrimitiveSubtractive")
{
    sAppModule      = "PartDesign";
    sGroup          = QT_TR_NOOP("PartDesign");
    sMenuText       = QT_TR_NOOP("Create a subtractive primitive");
    sToolTipText    = QT_TR_NOOP("Create a subtractive primitive");
    sWhatsThis      = "PartDesign_CompPrimitiveSubtractive";
    sStatusTip      = sToolTipText;
    eType           = ForEdit;
}

void CmdPrimtiveCompSubtractive::activated(int iMsg)
{
    App::Document *doc = getDocument();
    if (!PartDesignGui::assureModernWorkflow(doc))
        return;

    PartDesign::Body *pcActiveBody = PartDesignGui::getBody(true);

    if (!pcActiveBody)
        return;

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());

    //check if we already have a feature as subtractive ones work only if we have
    //something to subtract from.
    App::DocumentObject *prevSolid = pcActiveBody->Tip.getValue();
    if(!prevSolid) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No previous feature found"),
                QObject::tr("It is not possible to create a subtractive feature without a base feature available"));
            return;
    }

    auto shapeType( primitiveIntToName(iMsg) );
    auto FeatName( getUniqueObjectName(shapeType, pcActiveBody) );

    Gui::Command::openCommand( (std::string("Make subtractive ") + shapeType).c_str() );
    FCMD_OBJ_CMD(pcActiveBody,"newObject('PartDesign::Subtractive"<<shapeType<<"','"<<FeatName<<"')");
    Gui::Command::updateActive();

    auto Feat = pcActiveBody->getDocument()->getObject(FeatName.c_str());
    copyVisual(Feat, "ShapeColor", prevSolid);
    copyVisual(Feat, "LineColor", prevSolid);
    copyVisual(Feat, "PointColor", prevSolid);
    copyVisual(Feat, "Transparency", prevSolid);
    copyVisual(Feat, "DisplayMode", prevSolid);

    if ( isActiveObjectValid() ) {
        // TODO  (2015-08-05, Fat-Zer)
        FCMD_OBJ_HIDE(prevSolid);
    }

    PartDesignGui::setEdit(Feat,pcActiveBody);
}

Gui::Action * CmdPrimtiveCompSubtractive::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("PartDesign_Subtractive_Box"));
    p1->setObjectName(QString::fromLatin1("PartDesign_SubtractiveBox"));
    p1->setWhatsThis(QString::fromLatin1("PartDesign_SubtractiveBox"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("PartDesign_Subtractive_Cylinder"));
    p2->setObjectName(QString::fromLatin1("PartDesign_SubtractiveCylinder"));
    p2->setWhatsThis(QString::fromLatin1("PartDesign_SubtractiveCylinder"));
    QAction* p3 = pcAction->addAction(QString());
    p3->setIcon(Gui::BitmapFactory().iconFromTheme("PartDesign_Subtractive_Sphere"));
    p3->setObjectName(QString::fromLatin1("PartDesign_SubtractiveSphere"));
    p3->setWhatsThis(QString::fromLatin1("PartDesign_SubtractiveSphere"));
    QAction* p4 = pcAction->addAction(QString());
    p4->setIcon(Gui::BitmapFactory().iconFromTheme("PartDesign_Subtractive_Cone"));
    p4->setObjectName(QString::fromLatin1("PartDesign_SubtractiveCone"));
    p4->setWhatsThis(QString::fromLatin1("PartDesign_SubtractiveCone"));
    QAction* p5 = pcAction->addAction(QString());
    p5->setIcon(Gui::BitmapFactory().iconFromTheme("PartDesign_Subtractive_Ellipsoid"));
    p5->setObjectName(QString::fromLatin1("PartDesign_SubtractiveEllipsoid"));
    p5->setWhatsThis(QString::fromLatin1("PartDesign_SubtractiveEllipsoid"));
    QAction* p6 = pcAction->addAction(QString());
    p6->setIcon(Gui::BitmapFactory().iconFromTheme("PartDesign_Subtractive_Torus"));
    p6->setObjectName(QString::fromLatin1("PartDesign_SubtractiveTorus"));
    p6->setWhatsThis(QString::fromLatin1("PartDesign_SubtractiveTorus"));
    QAction* p7 = pcAction->addAction(QString());
    p7->setIcon(Gui::BitmapFactory().iconFromTheme("PartDesign_Subtractive_Prism"));
    p7->setObjectName(QString::fromLatin1("PartDesign_SubtractivePrism"));
    p7->setWhatsThis(QString::fromLatin1("PartDesign_SubtractivePrism"));
    QAction* p8 = pcAction->addAction(QString());
    p8->setIcon(Gui::BitmapFactory().iconFromTheme("PartDesign_Subtractive_Wedge"));
    p8->setObjectName(QString::fromLatin1("PartDesign_SubtractiveWedge"));
    p8->setWhatsThis(QString::fromLatin1("PartDesign_SubtractiveWedge"));

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
    arc1->setToolTip(QApplication::translate("PartDesign_CompPrimitiveSubtractive","Create a subtractive box by its width, height and length"));
    arc1->setStatusTip(arc1->toolTip());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("CmdPrimtiveCompSubtractive","Subtractive Cylinder"));
    arc2->setToolTip(QApplication::translate("PartDesign_CompPrimitiveSubtractive","Create a subtractive cylinder by its radius, height and angle"));
    arc2->setStatusTip(arc2->toolTip());
    QAction* arc3 = a[2];
    arc3->setText(QApplication::translate("CmdPrimtiveCompSubtractive","Subtractive Sphere"));
    arc3->setToolTip(QApplication::translate("PartDesign_CompPrimitiveSubtractive","Create a subtractive sphere by its radius and various angles"));
    arc3->setStatusTip(arc3->toolTip());
    QAction* arc4 = a[3];
    arc4->setText(QApplication::translate("CmdPrimtiveCompSubtractive","Subtractive Cone"));
    arc4->setToolTip(QApplication::translate("PartDesign_CompPrimitiveSubtractive","Create a subtractive cone"));
    arc4->setStatusTip(arc4->toolTip());
    QAction* arc5 = a[4];
    arc5->setText(QApplication::translate("CmdPrimtiveCompSubtractive","Subtractive Ellipsoid"));
    arc5->setToolTip(QApplication::translate("PartDesign_CompPrimitiveSubtractive","Create a subtractive ellipsoid"));
    arc5->setStatusTip(arc5->toolTip());
    QAction* arc6 = a[5];
    arc6->setText(QApplication::translate("CmdPrimtiveCompSubtractive","Subtractive Torus"));
    arc6->setToolTip(QApplication::translate("PartDesign_CompPrimitiveSubtractive","Create a subtractive torus"));
    arc6->setStatusTip(arc6->toolTip());
    QAction* arc7 = a[6];
    arc7->setText(QApplication::translate("CmdPrimtiveCompSubtractive","Subtractive Prism"));
    arc7->setToolTip(QApplication::translate("PartDesign_CompPrimitiveSubtractive","Create a subtractive prism"));
    arc7->setStatusTip(arc7->toolTip());
    QAction* arc8 = a[7];
    arc8->setText(QApplication::translate("CmdPrimtiveCompSubtractive","Subtractive Wedge"));
    arc8->setToolTip(QApplication::translate("PartDesign_CompPrimitiveSubtractive","Create a subtractive wedge"));
    arc8->setStatusTip(arc8->toolTip());
}

bool CmdPrimtiveCompSubtractive::isActive(void)
{
    return (hasActiveDocument() && !Gui::Control().activeDialog());
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
