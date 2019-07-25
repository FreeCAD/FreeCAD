/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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
# include <QGraphicsView>
# include <QMessageBox>
# include <iostream>
# include <string>
# include <sstream>
# include <cstdlib>
# include <exception>
#endif  //#ifndef _PreComp_

#include <App/DocumentObject.h>
#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/MainWindow.h>
#include <Gui/FileDialog.h>
#include <Gui/ViewProvider.h>

#include <Mod/Part/App/PartFeature.h>

#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawViewAnnotation.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/Geometry.h>
#include <Mod/TechDraw/App/Cosmetic.h>

#include "DrawGuiUtil.h"
#include "MDIViewPage.h"
#include "TaskLeaderLine.h"
#include "TaskRichAnno.h"
#include "TaskCosVertex.h"
#include "TaskCenterLine.h"
#include "TaskLineDecor.h"
#include "ViewProviderPage.h"
#include "ViewProviderViewPart.h"
#include "QGVPage.h"

using namespace TechDrawGui;
using namespace TechDraw;
using namespace std;


//internal functions
bool _checkSelectionHatch(Gui::Command* cmd);

void execCosmeticVertex(Gui::Command* cmd);
void execMidpoints(Gui::Command* cmd);
void execQuadrant(Gui::Command* cmd);
void execCenterLine(Gui::Command* cmd);
void exec2LineCenterLine(Gui::Command* cmd);
void exec2PointCenterLine(Gui::Command* cmd);


//===========================================================================
// TechDraw_Leader
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawLeaderLine);

CmdTechDrawLeaderLine::CmdTechDrawLeaderLine()
  : Command("TechDraw_LeaderLine")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Add a line to a view");
    sToolTipText    = QT_TR_NOOP("Add a line to a view");
    sWhatsThis      = "TechDraw_LeaderLine";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-mline";
}

void CmdTechDrawLeaderLine::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg != nullptr) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawView* baseFeat = nullptr;
    if (!selection.empty()) {
        baseFeat =  dynamic_cast<TechDraw::DrawView *>(selection[0].getObject());
        if( baseFeat == nullptr ) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection Error"),
                                 QObject::tr("Can not attach leader.  No base View selected."));
            return;
        }
    } else {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection Error"),
                                 QObject::tr("You must select a base View for the line."));
            return;
    }

    Gui::Control().showDialog(new TechDrawGui::TaskDlgLeaderLine(baseFeat,
                                                    page));
}

bool CmdTechDrawLeaderLine::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_RichAnno
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawRichAnno);

CmdTechDrawRichAnno::CmdTechDrawRichAnno()
  : Command("TechDraw_RichAnno")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Add a rich text annotation");
    sToolTipText    = QT_TR_NOOP("Add a rich text annotation");
    sWhatsThis      = "TechDraw_RichAnno";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-textleader";
}

void CmdTechDrawRichAnno::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg != nullptr) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawView* baseFeat = nullptr;
    if (!selection.empty()) {
        baseFeat =  dynamic_cast<TechDraw::DrawView *>(selection[0].getObject());
    }

    Gui::Control().showDialog(new TaskDlgRichAnno(baseFeat,
                                                  page));
}

bool CmdTechDrawRichAnno::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
}


//===========================================================================
// CosmeticVertexGroup
//===========================================================================

DEF_STD_CMD_ACL(CmdTechDrawCosmeticVertexGrp);

CmdTechDrawCosmeticVertexGrp::CmdTechDrawCosmeticVertexGrp()
  : Command("TechDraw_CosmeticVertexGrp")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Cosmetic Vertex");
    sToolTipText    = QT_TR_NOOP("Insert Cosmetic Vertex");
    sWhatsThis      = "TechDraw_CosmeticVertexGrp";
    sStatusTip      = sToolTipText;
//    eType           = ForEdit;
}

void CmdTechDrawCosmeticVertexGrp::activated(int iMsg)
{
//    Base::Console().Message("CMD::CosmeticVertexGrp - activated(%d)\n", iMsg);
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg != nullptr) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch(iMsg) {
        case 0:
            execCosmeticVertex(this);
            break;
        case 1:
            execMidpoints(this);
            break;
        case 2:
            execQuadrant(this);
            break;
        default:
            Base::Console().Message("CMD::CVGrp - invalid iMsg: %d\n",iMsg);
    };
//    Base::Console().Message("CMD::CosmeticVertexGrp - activated - exits\n");
}

Gui::Action * CmdTechDrawCosmeticVertexGrp::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("actions/techdraw-point"));
    p1->setObjectName(QString::fromLatin1("TechDraw_CosmeticVertex"));
    p1->setWhatsThis(QString::fromLatin1("TechDraw_CosmeticVertx"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("actions/techdraw-midpoint"));
    p2->setObjectName(QString::fromLatin1("TechDraw_Midpoints"));
    p2->setWhatsThis(QString::fromLatin1("TechDraw_Midpoints"));
    QAction* p3 = pcAction->addAction(QString());
    p3->setIcon(Gui::BitmapFactory().iconFromTheme("actions/techdraw-quadrant"));
    p3->setObjectName(QString::fromLatin1("TechDraw_Quadrant"));
    p3->setWhatsThis(QString::fromLatin1("TechDraw_Quadrant"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(p1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdTechDrawCosmeticVertexGrp::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* arc1 = a[0];
    arc1->setText(QApplication::translate("CmdTechDrawCosmeticVertexGrp","Cosmetic Vertex"));
    arc1->setToolTip(QApplication::translate("TechDraw_CosmeticVertex","Insert a Cosmetic Vertex into a View"));
    arc1->setStatusTip(arc1->toolTip());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("CmdMidpoints","Midpoints"));
    arc2->setToolTip(QApplication::translate("TechDraw_Midpoints","Insert Cosmetic Vertex at midpoint of Edge(s)"));
    arc2->setStatusTip(arc2->toolTip());
    QAction* arc3 = a[2];
    arc3->setText(QApplication::translate("CmdQuadrant","Quadrant"));
    arc3->setToolTip(QApplication::translate("TechDraw_Quadrant","Insert Cosmetic Vertex at quadrant points of Circle(s)"));
    arc3->setStatusTip(arc3->toolTip());
}

bool CmdTechDrawCosmeticVertexGrp::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
}


//===========================================================================
// TechDraw_CosmeticVertex
//===========================================================================

void execCosmeticVertex(Gui::Command* cmd)
{
//    Base::Console().Message("execCosmeticVertex()\n");
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(cmd);
    if (!page) {
        return;
    }

    std::vector<App::DocumentObject*> shapes = cmd->getSelection().
                                       getObjectsOfType(TechDraw::DrawViewPart::getClassTypeId());
    if (shapes.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("No DrawViewPart objects in this selection"));
        return;
    }

    TechDraw::DrawViewPart* baseFeat = nullptr;
    baseFeat =  dynamic_cast<TechDraw::DrawViewPart*>((*shapes.begin()));
    if (baseFeat == nullptr) {
        Base::Console().Message("CMD::CosmeticVertex - 1st shape is not DVP.  WTF?\n");
        return;
    }

    Gui::Control().showDialog(new TaskDlgCosVertex(baseFeat,
                                                   page));
//    Base::Console().Message("execCosmeticVertex - exits\n");
}

void execMidpoints(Gui::Command* cmd)
{
//    Base::Console().Message("execMidpoints()\n");
    TechDraw::DrawViewPart * objFeat = 0;
    std::vector<std::string> SubNames;

    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            objFeat = static_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            SubNames = (*itSel).getSubNames();
        }
    }

    for (auto& s: SubNames) {
        if (TechDraw::DrawUtil::getGeomTypeFromName(s) == "Edge") {
            continue;
        } else {
            std::stringstream edgeMsg;
            edgeMsg << "Please select only Edges for this function";
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
                                                   QObject::tr(edgeMsg.str().c_str()));
            return;
        }
    }

    const std::vector<TechDraw::BaseGeom*> edges = objFeat->getEdgeGeometry();
    double scale = objFeat->getScale();
    for (auto& s: SubNames) {
        int GeoId(TechDraw::DrawUtil::getIndexFromName(s));
        TechDraw::BaseGeom* geom = edges.at(GeoId);
        Base::Vector3d mid = geom->getMidPoint();
        mid = DrawUtil::invertY(mid);
        objFeat->addCosmeticVertex(mid / scale);
    }
    cmd->updateActive();
}

void execQuadrant(Gui::Command* cmd)
{
//    Base::Console().Message("execQuadrant()\n");
    TechDraw::DrawViewPart * objFeat = 0;
    std::vector<std::string> SubNames;

    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            objFeat = static_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            SubNames = (*itSel).getSubNames();
        }
    }

    for (auto& s: SubNames) {
        if (TechDraw::DrawUtil::getGeomTypeFromName(s) == "Edge") {
            continue;
        } else {
            std::stringstream edgeMsg;
            edgeMsg << "Please select only Edges for this function";
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
                                                   QObject::tr(edgeMsg.str().c_str()));
            return;
        }
    }

    const std::vector<TechDraw::BaseGeom*> edges = objFeat->getEdgeGeometry();
    double scale = objFeat->getScale();
    for (auto& s: SubNames) {
        int GeoId(TechDraw::DrawUtil::getIndexFromName(s));
        TechDraw::BaseGeom* geom = edges.at(GeoId);
            std::vector<Base::Vector3d> quads = geom->getQuads();
            for (auto& q: quads) {
                Base::Vector3d iq = DrawUtil::invertY(q);
                objFeat->addCosmeticVertex(iq / scale);
            }
    }
    cmd->updateActive();
}

DEF_STD_CMD_A(CmdTechDrawCosmeticVertex);

CmdTechDrawCosmeticVertex::CmdTechDrawCosmeticVertex()
  : Command("TechDraw_CosmeticVertex")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Add a cosmetic vertex");
    sToolTipText    = QT_TR_NOOP("Add a cosmetic vertex");
    sWhatsThis      = "TechDraw_CosmeticVertex";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-point";
}

void CmdTechDrawCosmeticVertex::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg != nullptr) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

    std::vector<App::DocumentObject*> shapes = getSelection().
                                       getObjectsOfType(TechDraw::DrawViewPart::getClassTypeId());
    if (shapes.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("No DrawViewPart objects in this selection"));
        return;
    }

    TechDraw::DrawViewPart* baseFeat = nullptr;
    baseFeat =  dynamic_cast<TechDraw::DrawViewPart*>((*shapes.begin()));
    if (baseFeat == nullptr) {
        Base::Console().Message("CMD::CosmeticVertex - 1st shape is not DVP.  WTF?\n");
        return;
    }

    Gui::Control().showDialog(new TaskDlgCosVertex(baseFeat,
                                                   page));
}

bool CmdTechDrawCosmeticVertex::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_Midpoints
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawMidpoints);

CmdTechDrawMidpoints::CmdTechDrawMidpoints()
  : Command("TechDraw_Midpoints")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Add midpoint vertices");
    sToolTipText    = QT_TR_NOOP("Add midpoint vertices");
    sWhatsThis      = "TechDraw_Midpoints";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-midpoint";
}

void CmdTechDrawMidpoints::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg != nullptr) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }
    execMidpoints(this);
}

bool CmdTechDrawMidpoints::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_Quadrant
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawQuadrant);

CmdTechDrawQuadrant::CmdTechDrawQuadrant()
  : Command("TechDraw_Quadrant")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Add quadrant vertices");
    sToolTipText    = QT_TR_NOOP("Add quadrant vertices");
    sWhatsThis      = "TechDraw_Quadrant";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-quadrant";
}

void CmdTechDrawQuadrant::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg != nullptr) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }
    execQuadrant(this);
}

bool CmdTechDrawQuadrant::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_Annotation
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawAnnotation);

CmdTechDrawAnnotation::CmdTechDrawAnnotation()
  : Command("TechDraw_Annotation")
{
    // setting the Gui eye-candy
    sGroup        = QT_TR_NOOP("TechDraw");
    sMenuText     = QT_TR_NOOP("Insert Annotation");
    sToolTipText  = QT_TR_NOOP("Insert Annotation");
    sWhatsThis    = "TechDraw_NewAnnotation";
    sStatusTip    = sToolTipText;
    sPixmap       = "actions/techdraw-annotation";
}

void CmdTechDrawAnnotation::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }
    std::string PageName = page->getNameInDocument();

    std::string FeatName = getUniqueObjectName("Annotation");
    openCommand("Create Annotation");
    doCommand(Doc,"App.activeDocument().addObject('TechDraw::DrawViewAnnotation','%s')",FeatName.c_str());
    doCommand(Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());
    updateActive();
    commitCommand();
}

bool CmdTechDrawAnnotation::isActive(void)
{
    return DrawGuiUtil::needPage(this);
}

//===========================================================================
// CenterLineGroup
//===========================================================================

DEF_STD_CMD_ACL(CmdTechDrawCenterLineGrp);

CmdTechDrawCenterLineGrp::CmdTechDrawCenterLineGrp()
  : Command("TechDraw_CenterLineGrp")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Center Line");
    sToolTipText    = QT_TR_NOOP("Insert Center Line");
    sWhatsThis      = "TechDraw_CenterLineGrp";
    sStatusTip      = sToolTipText;
//    eType           = ForEdit;
}

void CmdTechDrawCenterLineGrp::activated(int iMsg)
{
//    Base::Console().Message("CMD::CenterLineGrp - activated(%d)\n", iMsg);
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg != nullptr) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch(iMsg) {
        case 0:
            execCenterLine(this);
            break;
        case 1:
            exec2LineCenterLine(this);
            break;
        case 2:
            exec2PointCenterLine(this);
            break;
        default:
            Base::Console().Message("CMD::CVGrp - invalid iMsg: %d\n",iMsg);
    };
}

Gui::Action * CmdTechDrawCenterLineGrp::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("actions/techdraw-facecenterline"));
    p1->setObjectName(QString::fromLatin1("TechDraw_FaceCenterLine"));
    p1->setWhatsThis(QString::fromLatin1("TechDraw_FaceCenterLine"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("actions/techdraw-2linecenterline"));
    p2->setObjectName(QString::fromLatin1("TechDraw_2LineCenterLine"));
    p2->setWhatsThis(QString::fromLatin1("TechDraw_2LineCenterLine"));
    QAction* p3 = pcAction->addAction(QString());
    p3->setIcon(Gui::BitmapFactory().iconFromTheme("actions/techdraw-2pointcenterline"));
    p3->setObjectName(QString::fromLatin1("TechDraw_2PointCenterLine"));
    p3->setWhatsThis(QString::fromLatin1("TechDraw_2PointCenterLine"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(p1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdTechDrawCenterLineGrp::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* arc1 = a[0];
    arc1->setText(QApplication::translate("CmdTechDrawCenterLineGrp","Center Line"));
    arc1->setToolTip(QApplication::translate("TechDraw_FaceCenterLine","Insert a CenterLine into a Face(s)"));
    arc1->setStatusTip(arc1->toolTip());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("Cmd2LineCenterLine","2 Line CenterLine"));
    arc2->setToolTip(QApplication::translate("TechDraw_2LineCenterLine","Insert CenterLine between 2 lines"));
    arc2->setStatusTip(arc2->toolTip());
    QAction* arc3 = a[2];
    arc3->setText(QApplication::translate("Cmd2PointCenterLine","2 Point CenterLine"));
    arc3->setToolTip(QApplication::translate("TechDraw_2PointCenterLine","Insert CenterLine between 2 points"));
    arc3->setStatusTip(arc3->toolTip());
}

bool CmdTechDrawCenterLineGrp::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
}
//===========================================================================
// TechDraw_Centerline
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawFaceCenterLine);

CmdTechDrawFaceCenterLine::CmdTechDrawFaceCenterLine()
  : Command("TechDraw_FaceCenterLine")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Add a centerline to a Face(s)");
    sToolTipText    = QT_TR_NOOP("Add a centerline to a Face(s)");
    sWhatsThis      = "TechDraw_FaceCenterLine";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-facecenterline";
}

void CmdTechDrawFaceCenterLine::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg != nullptr) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    execCenterLine(this);
}

bool CmdTechDrawFaceCenterLine::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
}

void execCenterLine(Gui::Command* cmd)
{
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(cmd);
    if (!page) {
        return;
    }

    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    TechDraw::DrawViewPart* baseFeat = nullptr;
    if (!selection.empty()) {
        baseFeat =  dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
        if( baseFeat == nullptr ) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection Error"),
                                 QObject::tr("No base View in Selection."));
            return;
        }
    } else {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection Error"),
                                 QObject::tr("You must select a base View for the line."));
            return;
    }

    std::vector<std::string> SubNames;

    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            baseFeat = static_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            SubNames = (*itSel).getSubNames();
        }
    }
    std::vector<std::string> faceNames;
    std::vector<std::string> edgeNames;
    for (auto& s: SubNames) {
        std::string geomType = DrawUtil::getGeomTypeFromName(s);
        if (geomType == "Face") {
            faceNames.push_back(s);
        } else if (geomType == "Edge") {
            edgeNames.push_back(s);
        }
    }

    if ( (faceNames.empty()) && 
         (edgeNames.empty()) ) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection Error"),
                             QObject::tr("You must select a Face(s) or an existing CenterLine."));
        return;
    }
    if (!faceNames.empty()) {
        Gui::Control().showDialog(new TaskDlgCenterLine(baseFeat,
                                                        page,
                                                        faceNames));
    } else if (edgeNames.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection Error"),
                             QObject::tr("No CenterLine in selection."));
        return;
    } else {
        std::string edgeName = edgeNames.front();
        int geomIdx = DrawUtil::getIndexFromName(edgeName);
        const std::vector<TechDraw::BaseGeom  *> &geoms = baseFeat->getEdgeGeometry();
        BaseGeom* bg = geoms.at(geomIdx);
        int clIdx = bg->sourceIndex();
        TechDraw::CenterLine* cl = baseFeat->getCenterLineByIndex(clIdx);
        if (cl == nullptr) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection Error"),
                                 QObject::tr("No CenterLine in selection."));
            return;
        }

        Gui::Control().showDialog(new TaskDlgCenterLine(baseFeat,
                                                        page,
                                                        edgeNames.front()));
    }
}

//===========================================================================
// TechDraw_2LineCenterline
//===========================================================================

DEF_STD_CMD_A(CmdTechDraw2LineCenterLine);

CmdTechDraw2LineCenterLine::CmdTechDraw2LineCenterLine()
  : Command("TechDraw_2LineCenterLine")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Add a centerline between 2 lines");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_2LineCenterLine";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-2linecenterline";
}

void CmdTechDraw2LineCenterLine::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg != nullptr) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    exec2LineCenterLine(this);
}

bool CmdTechDraw2LineCenterLine::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
}

void exec2LineCenterLine(Gui::Command* cmd)
{
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(cmd);
    if (!page) {
        return;
    }

    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    TechDraw::DrawViewPart* baseFeat = nullptr;
    if (!selection.empty()) {
        baseFeat =  dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
        if( baseFeat == nullptr ) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection Error"),
                                 QObject::tr("No base View in Selection."));
            return;
        }
    } else {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection Error"),
                                 QObject::tr("You must select a base View for the line."));
            return;
    }

    std::vector<std::string> SubNames;

    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            baseFeat = static_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            SubNames = (*itSel).getSubNames();
        }
    }
    std::vector<std::string> faceNames;
    std::vector<std::string> edgeNames;
    for (auto& s: SubNames) {
        std::string geomType = DrawUtil::getGeomTypeFromName(s);
        if (geomType == "Edge") {
            edgeNames.push_back(s);
        }
    }

    if (edgeNames.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection Error"),
                             QObject::tr("You must select 2 lines or an existing CenterLine."));
        return;
    }
    if (!edgeNames.empty() && (edgeNames.size() == 2)) {
        Gui::Control().showDialog(new TaskDlgCenterLine(baseFeat,
                                                        page,
                                                        edgeNames));
    } else if (!edgeNames.empty() && (edgeNames.size() == 1)) {
        std::string edgeName = edgeNames.front();
        int geomIdx = DrawUtil::getIndexFromName(edgeName);
        const std::vector<TechDraw::BaseGeom  *> &geoms = baseFeat->getEdgeGeometry();
        BaseGeom* bg = geoms.at(geomIdx);
        int clIdx = bg->sourceIndex();
        TechDraw::CenterLine* cl = baseFeat->getCenterLineByIndex(clIdx);
        if (cl == nullptr) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection Error"),
                                 QObject::tr("No CenterLine in selection."));
            return;
        } else {
            Base::Console().Message("CMD::2LineCenter - show edit dialog here\n");
            Gui::Control().showDialog(new TaskDlgCenterLine(baseFeat,
                                                            page,
                                                            edgeNames.front()));
        }
    } else if (edgeNames.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection Error"),
                             QObject::tr("No CenterLine in selection."));
        return;
    }
}

//===========================================================================
// TechDraw_2PointCenterline
//===========================================================================

DEF_STD_CMD_A(CmdTechDraw2PointCenterLine);

CmdTechDraw2PointCenterLine::CmdTechDraw2PointCenterLine()
  : Command("TechDraw_2PointCenterLine")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Add a centerline between 2 points");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_2PointCenterLine";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-2pointcenterline";
}

void CmdTechDraw2PointCenterLine::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg != nullptr) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    exec2PointCenterLine(this);
}

bool CmdTechDraw2PointCenterLine::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
}

void exec2PointCenterLine(Gui::Command* cmd)
{
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(cmd);
    if (!page) {
        return;
    }

    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    TechDraw::DrawViewPart* baseFeat = nullptr;
    if (!selection.empty()) {
        baseFeat =  dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
        if( baseFeat == nullptr ) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection Error"),
                                 QObject::tr("No base View in Selection."));
            return;
        }
    } else {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection Error"),
                                 QObject::tr("You must select a base View for the line."));
            return;
    }

    std::vector<std::string> SubNames;

    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            baseFeat = static_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            SubNames = (*itSel).getSubNames();
        }
    }
    std::vector<std::string> edgeNames;
    std::vector<std::string> vertexNames;
    for (auto& s: SubNames) {
        std::string geomType = DrawUtil::getGeomTypeFromName(s);
        if (geomType == "Vertex") {
            vertexNames.push_back(s);
        } else if (geomType == "Edge") {
            edgeNames.push_back(s);
        }
    }

    if (vertexNames.empty() &&
        edgeNames.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection Error"),
                             QObject::tr("You must select 2 Vertexes or an existing CenterLine."));
        return;
    }
    if (!vertexNames.empty() && (vertexNames.size() == 2)) {
        Gui::Control().showDialog(new TaskDlgCenterLine(baseFeat,
                                                        page,
                                                        vertexNames));
    } else if (!edgeNames.empty() && (edgeNames.size() == 1)) {
        Gui::Control().showDialog(new TaskDlgCenterLine(baseFeat,
                                                        page,
                                                        edgeNames.front()));
    } else if (vertexNames.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection Error"),
                             QObject::tr("No CenterLine in selection."));
        return;
    }
}


//===========================================================================
// TechDraw_CosmeticEraser
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawCosmeticEraser);

CmdTechDrawCosmeticEraser::CmdTechDrawCosmeticEraser()
  : Command("TechDraw_CosmeticEraser")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Remove a cosmetic object");
    sToolTipText    = QT_TR_NOOP("Remove a cosmetic object");
    sWhatsThis      = "TechDraw_CosmeticEraser";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-eraser";
}

void CmdTechDrawCosmeticEraser::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg != nullptr) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    if (selection.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                         QObject::tr("Nothing selected"));
        return;
    }

    bool selectionError = false;
    for (auto& s: selection) {
        TechDraw::DrawViewPart * objFeat = static_cast<TechDraw::DrawViewPart*> (s.getObject());
        if (!objFeat->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            selectionError = true;
            break;
        }
    }
    if (selectionError) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                         QObject::tr("At least 1 object in selection is not a part view"));
        return;
    }

    TechDraw::DrawViewPart * objFeat = nullptr;
    std::vector<std::string> SubNames;
    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            objFeat = static_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            SubNames = (*itSel).getSubNames();
        }
        for (auto& s: SubNames) {
            int idx = TechDraw::DrawUtil::getIndexFromName(s);
            std::string geomType = TechDraw::DrawUtil::getGeomTypeFromName(s);
            if (geomType == "Edge") {
                TechDraw::BaseGeom* bg = objFeat->getGeomByIndex(idx);
                if ((bg != nullptr) &&
                    (bg->cosmetic) ) {
                    int source = bg->source();
                    int sourceIndex = bg->sourceIndex();
                    if (source == 1) {  //this is a "CosmeticEdge"
                        objFeat->removeCosmeticEdge(sourceIndex);
                    } else if (source == 2) { //this is a "CenterLine"
                        objFeat->removeCenterLine(sourceIndex);
                    } else {
                        Base::Console().Message(
                            "CMD::CosmeticEraserP - edge: %d is confused - source: %d\n",idx,source);
                    }
                }
            } else if (geomType == "Vertex") {
                TechDraw::CosmeticVertex* cv = objFeat->getCosmeticVertexByGeom(idx);
                if (cv != nullptr) {
                    objFeat->removeCosmeticVertex(cv);
                }
            } else {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                           QObject::tr("Unknown object type in selection"));
                return;
            }
        }
    }
}

bool CmdTechDrawCosmeticEraser::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_DecorateLine
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawDecorateLine);

CmdTechDrawDecorateLine::CmdTechDrawDecorateLine()
  : Command("TechDraw_DecorateLine")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Change the appearance of a line");
    sToolTipText    = QT_TR_NOOP("Change the appearance of a line");
    sWhatsThis      = "TechDraw_DecorateLine";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-linedecor";
}

void CmdTechDrawDecorateLine::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg != nullptr) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawViewPart* baseFeat = nullptr;
    if (!selection.empty()) {
        baseFeat =  dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
        if( baseFeat == nullptr ) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection Error"),
                                 QObject::tr("No View in Selection."));
            return;
        }
    } else {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection Error"),
                                 QObject::tr("You must select a View and/or line(s)."));
            return;
    }

    std::vector<std::string> SubNames;

    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            baseFeat = static_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            SubNames = (*itSel).getSubNames();
        }
    }
    std::vector<std::string> edgeNames;
    for (auto& s: SubNames) {
        std::string geomType = DrawUtil::getGeomTypeFromName(s);
        if (geomType == "Edge") {
            edgeNames.push_back(s);
        }
    }

    Gui::Control().showDialog(new TaskDlgLineDecor(baseFeat,
                                                   edgeNames));
}

bool CmdTechDrawDecorateLine::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ShowAll
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawShowAll);

CmdTechDrawShowAll::CmdTechDrawShowAll()
  : Command("TechDraw_ShowAll")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Show/Hide invisible edges in a View");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_ShowAll";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/techdraw-showall";
}

void CmdTechDrawShowAll::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg != nullptr) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    TechDraw::DrawView* baseFeat = nullptr;
    if (!selection.empty()) {
        baseFeat =  dynamic_cast<TechDraw::DrawView *>(selection[0].getObject());
    }

    Gui::ViewProvider* vp = QGIView::getViewProvider(baseFeat);
    auto partVP = dynamic_cast<ViewProviderViewPart*>(vp);
    if ( vp != nullptr ) {
        bool state = partVP->ShowAllEdges.getValue();
        state = !state;
        partVP->ShowAllEdges.setValue(state);
        baseFeat->requestPaint();
    }

}

bool CmdTechDrawShowAll::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
}

void CreateTechDrawCommandsAnnotate(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdTechDrawLeaderLine());
    rcCmdMgr.addCommand(new CmdTechDrawRichAnno());
    rcCmdMgr.addCommand(new CmdTechDrawCosmeticVertexGrp());
    rcCmdMgr.addCommand(new CmdTechDrawCosmeticVertex());
    rcCmdMgr.addCommand(new CmdTechDrawMidpoints());
    rcCmdMgr.addCommand(new CmdTechDrawQuadrant());
    rcCmdMgr.addCommand(new CmdTechDrawCenterLineGrp());
    rcCmdMgr.addCommand(new CmdTechDrawFaceCenterLine());
    rcCmdMgr.addCommand(new CmdTechDraw2LineCenterLine());
    rcCmdMgr.addCommand(new CmdTechDraw2PointCenterLine());
    rcCmdMgr.addCommand(new CmdTechDrawAnnotation());
    rcCmdMgr.addCommand(new CmdTechDrawCosmeticEraser());
    rcCmdMgr.addCommand(new CmdTechDrawDecorateLine());
    rcCmdMgr.addCommand(new CmdTechDrawShowAll());
}

//===========================================================================
// Selection Validation Helpers
//===========================================================================
