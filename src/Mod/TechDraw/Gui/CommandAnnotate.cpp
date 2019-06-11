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
#include <Mod/TechDraw/Gui/QGVPage.h>

#include "DrawGuiUtil.h"
#include "MDIViewPage.h"
#include "TaskLeaderLine.h"
#include "TaskRichAnno.h"
#include "TaskCosVertex.h"
#include "TaskCenterLine.h"
#include "ViewProviderPage.h"

using namespace TechDrawGui;
using namespace std;


//internal functions
bool _checkSelectionHatch(Gui::Command* cmd);

void execCosmeticVertex(Gui::Command* cmd);
void execMidpoints(Gui::Command* cmd);
void execQuadrant(Gui::Command* cmd);


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
    arc1->setToolTip(QApplication::translate("TechDraw_CosmeticVertex","Insert a Cosmetic Vertix into a View"));
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

    //combine 2 loops?
    const std::vector<TechDrawGeometry::BaseGeom*> edges = objFeat->getEdgeGeometry();
    double scale = objFeat->getScale();
    for (auto& s: SubNames) {
        int GeoId(TechDraw::DrawUtil::getIndexFromName(s));
        TechDrawGeometry::BaseGeom* geom = edges.at(GeoId);
        Base::Vector2d mid = geom->getMidPoint();
        Base::Vector3d mid3(mid.x / scale, - mid.y / scale, 0.0);
        objFeat->addRandomVertex(mid3);
    }
    cmd->updateActive();
//    Base::Console().Message("execMidpoints - exits\n");
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

    //combine 2 loops?
    const std::vector<TechDrawGeometry::BaseGeom*> edges = objFeat->getEdgeGeometry();
    double scale = objFeat->getScale();
    bool nonCircles = false;
    for (auto& s: SubNames) {
        int GeoId(TechDraw::DrawUtil::getIndexFromName(s));
        TechDrawGeometry::BaseGeom* geom = edges.at(GeoId);
        //TODO: should this be restricted to circles??
//        if (geom->geomType == TechDrawGeometry::CIRCLE) {
            std::vector<Base::Vector2d> quads = geom->getQuads();
            for (auto& q: quads) {
                Base::Vector3d q3(q.x / scale, - q.y / scale, 0.0);
                objFeat->addRandomVertex(q3);
            }
//        } else {
//            nonCircles = true;
//        }
    }
    if (nonCircles) {
        std::stringstream edgeMsg;
        edgeMsg << "Non circular edges found in selection.";
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Incorrect Selection"),
                             QObject::tr(edgeMsg.str().c_str()));
    }
    cmd->updateActive();
//    Base::Console().Message("execQuadrant - exits\n");
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
    if (SubNames.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Selection Error"),
                             QObject::tr("You must select a Face(s) for the center line."));
        return;
    }

    Gui::Control().showDialog(new TaskDlgCenterLine(baseFeat,
                                                    page,
                                                    SubNames));
}

bool CmdTechDrawFaceCenterLine::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
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
                TechDraw::CosmeticEdge* ce = objFeat->getCosmeticEdgeByLink(idx);
                if (ce != nullptr) {
                    objFeat->removeRandomEdge(ce);
                }
            } else if (geomType == "Vertex") {
                TechDraw::CosmeticVertex* cv = objFeat->getCosmeticVertexByLink(idx);
                if (cv != nullptr) {
                    objFeat->removeRandomVertex(cv);
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

void CreateTechDrawCommandsAnnotate(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdTechDrawLeaderLine());
    rcCmdMgr.addCommand(new CmdTechDrawRichAnno());
    rcCmdMgr.addCommand(new CmdTechDrawCosmeticVertexGrp());
    rcCmdMgr.addCommand(new CmdTechDrawCosmeticVertex());
    rcCmdMgr.addCommand(new CmdTechDrawMidpoints());
    rcCmdMgr.addCommand(new CmdTechDrawQuadrant());
    rcCmdMgr.addCommand(new CmdTechDrawAnnotation());
    rcCmdMgr.addCommand(new CmdTechDrawFaceCenterLine());
    rcCmdMgr.addCommand(new CmdTechDrawCosmeticEraser());
}

//===========================================================================
// Selection Validation Helpers
//===========================================================================


