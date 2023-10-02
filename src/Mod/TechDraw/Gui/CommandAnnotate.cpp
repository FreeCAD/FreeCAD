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
# include <QApplication>
# include <QMessageBox>
# include <sstream>
#endif

#include <App/DocumentObject.h>
#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/SelectionObject.h>
#include <Gui/ViewProvider.h>
#include <Mod/TechDraw/App/Cosmetic.h>
#include <Mod/TechDraw/App/Geometry.h>
#include <Mod/TechDraw/App/DrawLeaderLine.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawViewAnnotation.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawWeldSymbol.h>
#include <Mod/TechDraw/App/DrawUtil.h>

#include "DrawGuiUtil.h"
#include "QGIView.h"
#include "TaskCenterLine.h"
#include "TaskCosmeticLine.h"
#include "TaskCosVertex.h"
#include "TaskLeaderLine.h"
#include "TaskLineDecor.h"
#include "TaskRichAnno.h"
#include "TaskSurfaceFinishSymbols.h"
#include "TaskWeldingSymbol.h"
#include "TaskCosmeticCircle.h"
#include "ViewProviderViewPart.h"


using namespace TechDrawGui;
using namespace TechDraw;

//internal functions
bool _checkSelectionHatch(Gui::Command* cmd);

void execCosmeticVertex(Gui::Command* cmd);
void execMidpoints(Gui::Command* cmd);
void execQuadrants(Gui::Command* cmd);
void execCenterLine(Gui::Command* cmd);
void exec2LineCenterLine(Gui::Command* cmd);
void exec2PointCenterLine(Gui::Command* cmd);
void execLine2Points(Gui::Command* cmd);
void execCosmeticCircle(Gui::Command* cmd);
std::vector<std::string> getSelectedSubElements(Gui::Command* cmd,
                                                TechDraw::DrawViewPart* &dvp,
                                                std::string subType = "Edge");

//===========================================================================
// TechDraw_Leader
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawLeaderLine)

CmdTechDrawLeaderLine::CmdTechDrawLeaderLine()
  : Command("TechDraw_LeaderLine")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Add Leaderline to View");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_LeaderLine";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_LeaderLine";
}

void CmdTechDrawLeaderLine::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg) {
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
        if (!baseFeat) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong Selection"),
                                 QObject::tr("Can not attach leader.  No base View selected."));
            return;
        }
    } else {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong Selection"),
                                 QObject::tr("You must select a base View for the line."));
            return;
    }

    Gui::Control().showDialog(new TechDrawGui::TaskDlgLeaderLine(baseFeat,
                                                                 page));
    updateActive();
    Gui::Selection().clearSelection();
}

bool CmdTechDrawLeaderLine::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_RichTextAnnotation
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawRichTextAnnotation)

CmdTechDrawRichTextAnnotation::CmdTechDrawRichTextAnnotation()
  : Command("TechDraw_RichTextAnnotation")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Rich Text Annotation");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_RichTextAnnotation";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_RichTextAnnotation";
}

void CmdTechDrawRichTextAnnotation::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg) {
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
    updateActive();
    Gui::Selection().clearSelection();
}

bool CmdTechDrawRichTextAnnotation::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    return havePage;
}


//===========================================================================
// TechDraw_CosmeticVertexGroup
//===========================================================================

DEF_STD_CMD_ACL(CmdTechDrawCosmeticVertexGroup)

CmdTechDrawCosmeticVertexGroup::CmdTechDrawCosmeticVertexGroup()
  : Command("TechDraw_CosmeticVertexGroup")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Cosmetic Vertex");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_CosmeticVertexGroup";
    sStatusTip      = sToolTipText;
//    eType           = ForEdit;
}

void CmdTechDrawCosmeticVertexGroup::activated(int iMsg)
{
//    Base::Console().Message("CMD::CosmeticVertexGroup - activated(%d)\n", iMsg);
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg) {
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
            execQuadrants(this);
            break;
        default:
            Base::Console().Message("CMD::CVGrp - invalid iMsg: %d\n", iMsg);
    };
    updateActive();
    Gui::Selection().clearSelection();
}

Gui::Action * CmdTechDrawCosmeticVertexGroup::createAction()
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("actions/TechDraw_CosmeticVertex"));
    p1->setObjectName(QString::fromLatin1("TechDraw_CosmeticVertex"));
    p1->setWhatsThis(QString::fromLatin1("TechDraw_CosmeticVertex"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("actions/TechDraw_Midpoints"));
    p2->setObjectName(QString::fromLatin1("TechDraw_Midpoints"));
    p2->setWhatsThis(QString::fromLatin1("TechDraw_Midpoints"));
    QAction* p3 = pcAction->addAction(QString());
    p3->setIcon(Gui::BitmapFactory().iconFromTheme("actions/TechDraw_Quadrants"));
    p3->setObjectName(QString::fromLatin1("TechDraw_Quadrants"));
    p3->setWhatsThis(QString::fromLatin1("TechDraw_Quadrants"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(p1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdTechDrawCosmeticVertexGroup::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* arc1 = a[0];
    arc1->setText(QApplication::translate("CmdTechDrawCosmeticVertexGroup", "Add Cosmetic Vertex"));
    arc1->setToolTip(QApplication::translate("TechDraw_CosmeticVertex", "Inserts a Cosmetic Vertex into a View"));
    arc1->setStatusTip(arc1->toolTip());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("CmdMidpoints", "Add Midpoint Vertices"));
    arc2->setToolTip(QApplication::translate("TechDraw_Midpoints", "Inserts Cosmetic Vertices at Midpoint of selected Edges"));
    arc2->setStatusTip(arc2->toolTip());
    QAction* arc3 = a[2];
    arc3->setText(QApplication::translate("CmdQuadrants", "Add Quadrant Vertices"));
    arc3->setToolTip(QApplication::translate("TechDraw_Quadrants", "Inserts Cosmetic Vertices at Quadrant Points of selected Circles"));
    arc3->setStatusTip(arc3->toolTip());
}

bool CmdTechDrawCosmeticVertexGroup::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
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

    //shapes not empty and only contains dvp
    TechDraw::DrawViewPart* baseFeat = nullptr;
    baseFeat =  dynamic_cast<TechDraw::DrawViewPart*>((*shapes.begin()));

    Gui::Control().showDialog(new TaskDlgCosVertex(baseFeat,
                                                   page));
}

void execMidpoints(Gui::Command* cmd)
{
//    Base::Console().Message("execMidpoints()\n");
    TechDraw::DrawViewPart * dvp = nullptr;
    std::vector<std::string> selectedEdges = getSelectedSubElements(cmd, dvp, "Edge");

    if (!dvp || selectedEdges.empty())
        return;

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add Midpoint Vertices"));

    const TechDraw::BaseGeomPtrVector edges = dvp->getEdgeGeometry();
    double scale = dvp->getScale();
    for (auto& s: selectedEdges) {
        int GeoId(TechDraw::DrawUtil::getIndexFromName(s));
        TechDraw::BaseGeomPtr geom = edges.at(GeoId);
        Base::Vector3d mid = geom->getMidPoint();
        mid = DrawUtil::invertY(mid);
        dvp->addCosmeticVertex(mid / scale);
    }

    Gui::Command::commitCommand();

    dvp->recomputeFeature();
}

void execQuadrants(Gui::Command* cmd)
{
//    Base::Console().Message("execQuadrants()\n");
    TechDraw::DrawViewPart* dvp = nullptr;
    std::vector<std::string> selectedEdges = getSelectedSubElements(cmd, dvp, "Edge");

    if (!dvp || selectedEdges.empty())
        return;

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add Quadrant Vertices"));

    const TechDraw::BaseGeomPtrVector edges = dvp->getEdgeGeometry();
    double scale = dvp->getScale();
    for (auto& s: selectedEdges) {
        int GeoId(TechDraw::DrawUtil::getIndexFromName(s));
        TechDraw::BaseGeomPtr geom = edges.at(GeoId);
            std::vector<Base::Vector3d> quads = geom->getQuads();
            for (auto& q: quads) {
                Base::Vector3d iq = DrawUtil::invertY(q);
                dvp->addCosmeticVertex(iq / scale);
            }
    }

    Gui::Command::commitCommand();

    dvp->recomputeFeature();
}

DEF_STD_CMD_A(CmdTechDrawCosmeticVertex)

CmdTechDrawCosmeticVertex::CmdTechDrawCosmeticVertex()
  : Command("TechDraw_CosmeticVertex")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Add Cosmetic Vertex");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_CosmeticVertex";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_CosmeticVertex";
}

void CmdTechDrawCosmeticVertex::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg) {
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
    if (!baseFeat) {
        Base::Console().Message("CMD::CosmeticVertex - 1st shape is not DVP.  WTF?\n");
        return;
    }

    Gui::Control().showDialog(new TaskDlgCosVertex(baseFeat,
                                                   page));
    updateActive();
    Gui::Selection().clearSelection();
}

bool CmdTechDrawCosmeticVertex::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_Midpoints
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawMidpoints)

CmdTechDrawMidpoints::CmdTechDrawMidpoints()
  : Command("TechDraw_Midpoints")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Add Midpoint Vertices");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_Midpoints";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_Midpoints";
}

void CmdTechDrawMidpoints::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }
    execMidpoints(this);
    updateActive();
    Gui::Selection().clearSelection();
}

bool CmdTechDrawMidpoints::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_Quadrants
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawQuadrants)

CmdTechDrawQuadrants::CmdTechDrawQuadrants()
  : Command("TechDraw_Quadrants")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Add Quadrant Vertices");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_Quadrants";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_Quadrants";
}

void CmdTechDrawQuadrants::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }
    execQuadrants(this);
    updateActive();
    Gui::Selection().clearSelection();
}

bool CmdTechDrawQuadrants::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_Annotation
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawAnnotation)

CmdTechDrawAnnotation::CmdTechDrawAnnotation()
  : Command("TechDraw_Annotation")
{
    // setting the Gui eye-candy
    sGroup        = QT_TR_NOOP("TechDraw");
    sMenuText     = QT_TR_NOOP("Insert Annotation");
    sToolTipText  = sMenuText;
    sWhatsThis    = "TechDraw_NewAnnotation";
    sStatusTip    = sToolTipText;
    sPixmap       = "actions/TechDraw_Annotation";
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
    openCommand(QT_TRANSLATE_NOOP("Command", "Create Annotation"));
    doCommand(Doc, "App.activeDocument().addObject('TechDraw::DrawViewAnnotation', '%s')", FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.translateLabel('DrawViewAnnotation', 'Annotation', '%s')",
              FeatName.c_str(), FeatName.c_str());

    doCommand(Doc, "App.activeDocument().%s.addView(App.activeDocument().%s)", PageName.c_str(), FeatName.c_str());
    updateActive();
    commitCommand();
}

bool CmdTechDrawAnnotation::isActive()
{
    return DrawGuiUtil::needPage(this);
}

//===========================================================================
// TechDraw_CenterLineGroup
//===========================================================================

DEF_STD_CMD_ACL(CmdTechDrawCenterLineGroup)

CmdTechDrawCenterLineGroup::CmdTechDrawCenterLineGroup()
  : Command("TechDraw_CenterLineGroup")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Center Line");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_CenterLineGroup";
    sStatusTip      = sToolTipText;
//    eType           = ForEdit;
}

void CmdTechDrawCenterLineGroup::activated(int iMsg)
{
//    Base::Console().Message("CMD::CenterLineGroup - activated(%d)\n", iMsg);
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch(iMsg) {
        case 0:                 //faces
            execCenterLine(this);
            break;
        case 1:                 //2 lines
            exec2LineCenterLine(this);
            break;
        case 2:                 //2 points
            exec2PointCenterLine(this);
            break;
        default:
            Base::Console().Message("CMD::CVGrp - invalid iMsg: %d\n", iMsg);
    };
}

Gui::Action * CmdTechDrawCenterLineGroup::createAction()
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("actions/TechDraw_FaceCenterLine"));
    p1->setObjectName(QString::fromLatin1("TechDraw_FaceCenterLine"));
    p1->setWhatsThis(QString::fromLatin1("TechDraw_FaceCenterLine"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("actions/TechDraw_2LineCenterline"));
    p2->setObjectName(QString::fromLatin1("TechDraw_2LineCenterLine"));
    p2->setWhatsThis(QString::fromLatin1("TechDraw_2LineCenterLine"));
    QAction* p3 = pcAction->addAction(QString());
    p3->setIcon(Gui::BitmapFactory().iconFromTheme("actions/TechDraw_2PointCenterline"));
    p3->setObjectName(QString::fromLatin1("TechDraw_2PointCenterLine"));
    p3->setWhatsThis(QString::fromLatin1("TechDraw_2PointCenterLine"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(p1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdTechDrawCenterLineGroup::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* arc1 = a[0];
    arc1->setText(QApplication::translate("CmdTechDrawCenterLineGroup", "Add Centerline to Faces"));
    arc1->setToolTip(QApplication::translate("TechDraw_FaceCenterLine", "Adds a Centerline to Faces"));
    arc1->setStatusTip(arc1->toolTip());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("Cmd2LineCenterLine", "Add Centerline between 2 Lines"));
    arc2->setToolTip(QApplication::translate("TechDraw_2LineCenterLine", "Adds a Centerline between 2 Lines"));
    arc2->setStatusTip(arc2->toolTip());
    QAction* arc3 = a[2];
    arc3->setText(QApplication::translate("Cmd2PointCenterLine", "Add Centerline between 2 Points"));
    arc3->setToolTip(QApplication::translate("TechDraw_2PointCenterLine", "Adds a Centerline between 2 Points"));
    arc3->setStatusTip(arc3->toolTip());
}

bool CmdTechDrawCenterLineGroup::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}
//===========================================================================
// TechDraw_Centerline
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawFaceCenterLine)

CmdTechDrawFaceCenterLine::CmdTechDrawFaceCenterLine()
  : Command("TechDraw_FaceCenterLine")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Add Centerline to Faces");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_FaceCenterLine";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_FaceCenterLine";
}

void CmdTechDrawFaceCenterLine::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    execCenterLine(this);
}

bool CmdTechDrawFaceCenterLine::isActive()
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
    TechDraw::DrawViewPart *baseFeat = nullptr;
    if (!selection.empty()) {
        baseFeat = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
        if (!baseFeat) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong Selection"),
                                 QObject::tr("No base View in Selection."));
            return;
        }
    }
    else {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong Selection"),
                             QObject::tr("You must select a base View for the line."));
        return;
    }

    std::vector<std::string> subNames;

    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            baseFeat = static_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            subNames = (*itSel).getSubNames();
        }
    }
    std::vector<std::string> faceNames;
    std::vector<std::string> edgeNames;
    for (auto& s: subNames) {
        std::string geomType = DrawUtil::getGeomTypeFromName(s);
        if (geomType == "Face") {
            faceNames.push_back(s);
        } else if (geomType == "Edge") {
            edgeNames.push_back(s);
        }
    }

    if ( (faceNames.empty()) &&
         (edgeNames.empty()) ) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong Selection"),
                             QObject::tr("You must select Faces or an existing CenterLine."));
        return;
    }
    if (!faceNames.empty()) {
        Gui::Control().showDialog(new TaskDlgCenterLine(baseFeat,
                                                        page,
                                                        faceNames,
                                                        false));
    } else if (edgeNames.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong Selection"),
                             QObject::tr("No CenterLine in selection."));
        return;
    } else {
        TechDraw::CenterLine* cl = baseFeat->getCenterLineBySelection(edgeNames.front());
        if (!cl) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong Selection"),
                             QObject::tr("Selection is not a CenterLine."));
            return;
        }
        Gui::Control().showDialog(new TaskDlgCenterLine(baseFeat,
                                                        page,
                                                        edgeNames.front(),
                                                        true));
    }
}

//===========================================================================
// TechDraw_2LineCenterline
//===========================================================================

DEF_STD_CMD_A(CmdTechDraw2LineCenterLine)

CmdTechDraw2LineCenterLine::CmdTechDraw2LineCenterLine()
  : Command("TechDraw_2LineCenterLine")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Add Centerline between 2 Lines");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_2LineCenterLine";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_2LineCenterline";
}

void CmdTechDraw2LineCenterLine::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    exec2LineCenterLine(this);
}

bool CmdTechDraw2LineCenterLine::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

void exec2LineCenterLine(Gui::Command* cmd)
{
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(cmd);
    if (!page) {
        return;
    }
    TechDraw::DrawViewPart* dvp = nullptr;
    std::vector<std::string> selectedEdges = getSelectedSubElements(cmd, dvp, "Edge");

    if (!dvp || selectedEdges.empty()) {
        return;
    }

    if (selectedEdges.size() == 2) {
        Gui::Control().showDialog(new TaskDlgCenterLine(dvp,
                                                        page,
                                                        selectedEdges,
                                                        false));
    } else if (selectedEdges.size() == 1) {
        TechDraw::CenterLine* cl = dvp->getCenterLineBySelection(selectedEdges.front());
        if (!cl) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong Selection"),
                             QObject::tr("Selection is not a CenterLine."));
            return;
        }
        Gui::Control().showDialog(new TaskDlgCenterLine(dvp,
                                                page,
                                                selectedEdges.front(),
                                                true));
    } else {  //not create, not edit, what is this???
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong Selection"),
                             QObject::tr("Selection not understood."));
        return;
    }
}

//===========================================================================
// TechDraw_2PointCenterline
//===========================================================================

DEF_STD_CMD_A(CmdTechDraw2PointCenterLine)

CmdTechDraw2PointCenterLine::CmdTechDraw2PointCenterLine()
  : Command("TechDraw_2PointCenterLine")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Add Centerline between 2 Points");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_2PointCenterLine";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_2PointCenterline";
}

void CmdTechDraw2PointCenterLine::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    exec2PointCenterLine(this);
    updateActive();
    Gui::Selection().clearSelection();
}

bool CmdTechDraw2PointCenterLine::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
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
    if (selection.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong Selection"),
                                QObject::tr("You must select a base View for the line."));
        return;
    }

    baseFeat =  dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    if (!baseFeat) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong Selection"),
                                QObject::tr("No base View in Selection."));
        return;
    }

    std::vector<std::string> subNames;

    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            baseFeat = static_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            subNames = (*itSel).getSubNames();
        }
    }
    std::vector<std::string> edgeNames;
    std::vector<std::string> vertexNames;
    for (auto& s: subNames) {
        std::string geomType = DrawUtil::getGeomTypeFromName(s);
        if (geomType == "Vertex") {
            vertexNames.push_back(s);
        } else if (geomType == "Edge") {
            edgeNames.push_back(s);
        }
    }

    if (vertexNames.empty() &&
        edgeNames.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong Selection"),
                             QObject::tr("You must select 2 Vertexes or an existing CenterLine."));
        return;
    }
    if (!vertexNames.empty() && (vertexNames.size() == 2)) {
        Gui::Control().showDialog(new TaskDlgCenterLine(baseFeat,
                                                        page,
                                                        vertexNames,
                                                        false));
    } else if (!edgeNames.empty() && (edgeNames.size() == 1)) {
        TechDraw::CenterLine* cl = baseFeat->getCenterLineBySelection(edgeNames.front());
        if (!cl) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong Selection"),
                             QObject::tr("Selection is not a CenterLine."));
            return;
        }

        Gui::Control().showDialog(new TaskDlgCenterLine(baseFeat,
                                                        page,
                                                        edgeNames.front(),
                                                        false));
    } else if (vertexNames.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong Selection"),
                             QObject::tr("Need 2 Vertices or 1 CenterLine."));
        return;
    }
}

//===========================================================================
// TechDraw_2PointCosmeticLine
//===========================================================================

DEF_STD_CMD_A(CmdTechDraw2PointCosmeticLine)

CmdTechDraw2PointCosmeticLine::CmdTechDraw2PointCosmeticLine()
  : Command("TechDraw_2PointCosmeticLine")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Add Cosmetic Line Through 2 Points");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_2PointCosmeticLine";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_Line2Points";
}

void CmdTechDraw2PointCosmeticLine::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    execLine2Points(this);

    updateActive();
    Gui::Selection().clearSelection();
}

bool CmdTechDraw2PointCosmeticLine::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

void execLine2Points(Gui::Command* cmd)
{
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(cmd);
    if (!page) {
        return;
    }

    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    TechDraw::DrawViewPart* baseFeat = nullptr;
    std::vector<std::string> subNames2D;
    std::vector< std::pair<Part::Feature*, std::string> > objs3D;
    if (selection.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong Selection"),
                             QObject::tr("Selection is empty."));
        return;
    }

    for (auto& so: selection) {
        if (so.getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            baseFeat = static_cast<TechDraw::DrawViewPart*> (so.getObject());
            subNames2D = so.getSubNames();
        } else if (so.getObject()->isDerivedFrom(Part::Feature::getClassTypeId())) {
            std::vector<std::string> subNames3D = so.getSubNames();
            for (auto& sub3D: subNames3D) {
                std::pair<Part::Feature*, std::string> temp;
                temp.first = static_cast<Part::Feature*>(so.getObject());
                temp.second = sub3D;
                objs3D.push_back(temp);
            }
        } else {
            //garbage
        }
    }

    if (!baseFeat) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong Selection"),
                             QObject::tr("You must select a base View for the line."));
        return;
    }

    //TODO: should be a smarter check
    if ( (subNames2D.empty()) &&
         (objs3D.empty()) )  {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong Selection"),
                             QObject::tr("Not enough points in selection."));
        return;
    }

    std::vector<std::string> edgeNames;
    std::vector<std::string> vertexNames;
    for (auto& s: subNames2D) {
        std::string geomType = DrawUtil::getGeomTypeFromName(s);
        if (geomType == "Vertex") {
            vertexNames.push_back(s);
        } else if (geomType == "Edge") {
            edgeNames.push_back(s);
        }
    }

    //check if editing existing edge
    if (!edgeNames.empty() && (edgeNames.size() == 1)) {
        TechDraw::CosmeticEdge* ce = baseFeat->getCosmeticEdgeBySelection(edgeNames.front());
        if (!ce) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong Selection"),
                             QObject::tr("Selection is not a Cosmetic Line."));
            return;
        }
        Gui::Control().showDialog(new TaskDlgCosmeticLine(baseFeat,
                                                          edgeNames.front()));
        return;
    }

    std::vector<Base::Vector3d> points;
    std::vector<bool> is3d;
    //get the 2D points
    if (!vertexNames.empty()) {
        for (auto& v2d: vertexNames) {
            int idx = DrawUtil::getIndexFromName(v2d);
            TechDraw::VertexPtr v = baseFeat->getProjVertexByIndex(idx);
            if (v) {
                points.push_back(v->point());
                is3d.push_back(false);
            }
        }
    }
    //get the 3D points
    if (!objs3D.empty()) {
        for (auto& o3D: objs3D) {
            int idx = DrawUtil::getIndexFromName(o3D.second);
            Part::TopoShape s = o3D.first->Shape.getShape();
            TopoDS_Vertex v = TopoDS::Vertex(s.getSubShape(TopAbs_VERTEX, idx));
            Base::Vector3d p = DrawUtil::vertex2Vector(v);
            points.push_back(p);
            is3d.push_back(true);
        }
    }

    if (points.size() != 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong Selection"),
                             QObject::tr("You must select 2 Vertexes."));
        return;
    }

    Gui::Control().showDialog(new TaskDlgCosmeticLine(baseFeat,
                                                      points,
                                                      is3d));
}

//===========================================================================
// TechDraw_CosmeticCircle
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawCosmeticCircle)

CmdTechDrawCosmeticCircle::CmdTechDrawCosmeticCircle()
  : Command("TechDraw_CosmeticCircle")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Add Cosmetic Circle");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_CosmeticCircle";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_CosmeticCircle";
}

void CmdTechDrawCosmeticCircle::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    execCosmeticCircle(this);

    updateActive();
    Gui::Selection().clearSelection();
}

bool CmdTechDrawCosmeticCircle::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

void execCosmeticCircle(Gui::Command* cmd)
{
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(cmd);
    if (!page) {
        return;
    }

    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    TechDraw::DrawViewPart* baseFeat = nullptr;
    std::vector<std::string> subNames2D;
    std::vector< std::pair<Part::Feature*, std::string> > objs3D;
    if (selection.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong Selection"),
                             QObject::tr("Selection is empty."));
        return;
    }

    for (auto& so: selection) {
        if (so.getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            baseFeat = static_cast<TechDraw::DrawViewPart*> (so.getObject());
            subNames2D = so.getSubNames();
        } else if (so.getObject()->isDerivedFrom(Part::Feature::getClassTypeId())) {
            std::vector<std::string> subNames3D = so.getSubNames();
            for (auto& sub3D: subNames3D) {
                std::pair<Part::Feature*, std::string> temp;
                temp.first = static_cast<Part::Feature*>(so.getObject());
                temp.second = sub3D;
                objs3D.push_back(temp);
            }
        } else {
            //garbage
        }
    }

    if (!baseFeat) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong Selection"),
                             QObject::tr("You must select a base View for the circle."));
        return;
    }

    std::vector<std::string> edgeNames;
    std::vector<std::string> vertexNames;
    for (auto& s: subNames2D) {
        std::string geomType = DrawUtil::getGeomTypeFromName(s);
        if (geomType == "Vertex") {
            vertexNames.push_back(s);
        } else if (geomType == "Edge") {
            edgeNames.push_back(s);
        }
    }

    //check if editing existing edge
    if (!edgeNames.empty() && (edgeNames.size() == 1)) {
        TechDraw::CosmeticEdge* ce = baseFeat->getCosmeticEdgeBySelection(edgeNames.front());
        if (!ce) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong Selection"),
                             QObject::tr("Selection is not a Cosmetic edge."));
            return;
        }
        Gui::Control().showDialog(new TaskDlgCosmeticCircle(baseFeat,
                                                          edgeNames.front()));
        return;
    }

    std::vector<Base::Vector3d> points;
    std::vector<bool> is3d;
    //get the 2D points
    if (!vertexNames.empty()) {
        for (auto& v2d: vertexNames) {
            int idx = DrawUtil::getIndexFromName(v2d);
            TechDraw::VertexPtr v = baseFeat->getProjVertexByIndex(idx);
            if (v) {
                points.push_back(v->point());
                is3d.push_back(false);
            }
        }
    }
    //get the 3D points
    if (!objs3D.empty()) {
        for (auto& o3D: objs3D) {
            int idx = DrawUtil::getIndexFromName(o3D.second);
            Part::TopoShape s = o3D.first->Shape.getShape();
            TopoDS_Vertex v = TopoDS::Vertex(s.getSubShape(TopAbs_VERTEX, idx));
            Base::Vector3d p = DrawUtil::vertex2Vector(v);
            points.push_back(p);
            is3d.push_back(true);
        }
    }

    if (points.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong Selection"),
                             QObject::tr("Please select a center for the circle."));
        return;
    }

    bool centerIs3d = false;
    if (!is3d.empty()) {
        centerIs3d = is3d.front();
    }

    Gui::Control().showDialog(new TaskDlgCosmeticCircle(baseFeat,
                                                      points.front(),
                                                      centerIs3d));
}

//===========================================================================
// TechDraw_CosmeticEraser
//===========================================================================

#define GEOMETRYEDGE 0
#define COSMETICEDGE 1
#define CENTERLINE 2

DEF_STD_CMD_A(CmdTechDrawCosmeticEraser)

CmdTechDrawCosmeticEraser::CmdTechDrawCosmeticEraser()
  : Command("TechDraw_CosmeticEraser")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Remove Cosmetic Object");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_CosmeticEraser";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_CosmeticEraser";
}

void CmdTechDrawCosmeticEraser::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg) {
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

    for (auto& s: selection) {
        TechDraw::DrawViewPart * objFeat = static_cast<TechDraw::DrawViewPart*> (s.getObject());
        if (!objFeat->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                            QObject::tr("At least 1 object in selection is not a part view"));
            return;
        }
    }

    TechDraw::DrawViewPart * objFeat = nullptr;
    std::vector<std::string> subNames;
    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            objFeat = static_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            subNames = (*itSel).getSubNames();
        }
        if (!objFeat) {
            break;
        }
        std::vector<std::string> cv2Delete;
        std::vector<std::string> ce2Delete;
        std::vector<std::string> cl2Delete;
        for (auto& s: subNames) {
            int idx = TechDraw::DrawUtil::getIndexFromName(s);
            std::string geomType = TechDraw::DrawUtil::getGeomTypeFromName(s);
            if (geomType == "Edge") {
                TechDraw::BaseGeomPtr bg = objFeat->getGeomByIndex(idx);
                if (bg && bg->getCosmetic()) {
                    int source = bg->source();
                    std::string tag = bg->getCosmeticTag();
                    if (source == COSMETICEDGE) {
                        ce2Delete.push_back(tag);
                    } else if (source == CENTERLINE) {
                        cl2Delete.push_back(tag);
                    } else {
                        Base::Console().Message(
                            "CMD::CosmeticEraser - edge: %d is confused - source: %d\n", idx, source);
                    }
                }
            } else if (geomType == "Vertex") {
                TechDraw::VertexPtr tdv = objFeat->getProjVertexByIndex(idx);
                if (!tdv)
                    Base::Console().Message("CMD::eraser - geom: %d not found!\n", idx);

                std::string delTag = tdv->getCosmeticTag();
                if (delTag.empty())
                    Base::Console().Warning("Vertex%d is not cosmetic! Can not erase.\n", idx);
                cv2Delete.push_back(delTag);
            } else {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                           QObject::tr("Unknown object type in selection"));
                return;
            }

        }
        if (!cv2Delete.empty()) {
            objFeat->removeCosmeticVertex(cv2Delete);
        }

        if (!ce2Delete.empty()) {
            objFeat->removeCosmeticEdge(ce2Delete);
        }
        if (!cl2Delete.empty()) {
            objFeat->removeCenterLine(cl2Delete);
        }
    objFeat->recomputeFeature();
    }
}

bool CmdTechDrawCosmeticEraser::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_DecorateLine
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawDecorateLine)

CmdTechDrawDecorateLine::CmdTechDrawDecorateLine()
  : Command("TechDraw_DecorateLine")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Change Appearance of Lines");
    sToolTipText    = QT_TR_NOOP("Change Appearance of selected Lines");
    sWhatsThis      = "TechDraw_DecorateLine";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_DecorateLine";
}

void CmdTechDrawDecorateLine::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg) {
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
    if (selection.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong Selection"),
                                QObject::tr("You must select a View and/or lines."));
        return;
    }

    baseFeat =  dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    if (!baseFeat) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong Selection"),
                                QObject::tr("No View in Selection."));
        return;
    }

    std::vector<std::string> subNames;

    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            baseFeat = static_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            subNames = (*itSel).getSubNames();
        }
    }
    std::vector<std::string> edgeNames;
    for (auto& s: subNames) {
        std::string geomType = DrawUtil::getGeomTypeFromName(s);
        if (geomType == "Edge") {
            edgeNames.push_back(s);
        }
    }

    Gui::Control().showDialog(new TaskDlgLineDecor(baseFeat,
                                                   edgeNames));
    updateActive();
    Gui::Selection().clearSelection();
}

bool CmdTechDrawDecorateLine::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ShowAll
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawShowAll)

CmdTechDrawShowAll::CmdTechDrawShowAll()
  : Command("TechDraw_ShowAll")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Show/Hide Invisible Edges");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_ShowAll";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_ShowAll";
}

void CmdTechDrawShowAll::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg) {
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
    if (selection.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("Nothing selected"));
        return;
    }

    baseFeat =  dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
    if (!baseFeat)  {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("No Part Views in this selection"));
        return;
    }

    Gui::ViewProvider* vp = QGIView::getViewProvider(baseFeat);
    auto partVP = dynamic_cast<ViewProviderViewPart*>(vp);
    if (partVP) {
        bool state = partVP->ShowAllEdges.getValue();
        state = !state;
        partVP->ShowAllEdges.setValue(state);
        baseFeat->requestPaint();
    }
}

bool CmdTechDrawShowAll::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_WeldSymbol
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawWeldSymbol)

CmdTechDrawWeldSymbol::CmdTechDrawWeldSymbol()
  : Command("TechDraw_WeldSymbol")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Add Welding Information to Leaderline");
    sToolTipText    = sMenuText;
    sWhatsThis      = "TechDraw_WeldSymbol";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_WeldSymbol";
}

void CmdTechDrawWeldSymbol::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

    std::vector<App::DocumentObject*> leaders = getSelection().
                                         getObjectsOfType(TechDraw::DrawLeaderLine::getClassTypeId());
    std::vector<App::DocumentObject*> welds = getSelection().
                                         getObjectsOfType(TechDraw::DrawWeldSymbol::getClassTypeId());
    TechDraw::DrawLeaderLine* leadFeat = nullptr;
    TechDraw::DrawWeldSymbol* weldFeat = nullptr;
    if ( (leaders.size() != 1) &&
         (welds.size() != 1) ) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select exactly one Leader line or one Weld symbol."));
        return;
    }
    if (!leaders.empty()) {
        leadFeat = static_cast<TechDraw::DrawLeaderLine*> (leaders.front());
        Gui::Control().showDialog(new TaskDlgWeldingSymbol(leadFeat));
    } else if (!welds.empty()) {
        weldFeat = static_cast<TechDraw::DrawWeldSymbol*> (welds.front());
        Gui::Control().showDialog(new TaskDlgWeldingSymbol(weldFeat));
    }
    updateActive();
    Gui::Selection().clearSelection();
}

bool CmdTechDrawWeldSymbol::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_SurfaceFinishSymbols
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawSurfaceFinishSymbols)

CmdTechDrawSurfaceFinishSymbols::CmdTechDrawSurfaceFinishSymbols()
  : Command("TechDraw_SurfaceFinishSymbols")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Create a Surface Finish Symbol");
    sToolTipText    = QT_TR_NOOP("Select a view<br>\
    - click this button<br>\
    - select surface finish symbol attributes in opened panel");
    sWhatsThis      = "TechDraw_SurfaceFinishSymbols";
    sStatusTip      = sToolTipText;
    sPixmap         = "actions/TechDraw_SurfaceFinishSymbols";
}

void CmdTechDrawSurfaceFinishSymbols::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<Gui::SelectionObject> selection = this->getSelection().getSelectionEx();
    if (selection.empty())
    {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("SurfaceFinishSymbols"), QObject::tr("Selection is empty"));
        return;
    }
    TechDraw::DrawViewPart* objFeat = dynamic_cast<TechDraw::DrawViewPart*> (selection[0].getObject());
    if(!objFeat)
    {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("SurfaceFinishSymbols"), QObject::tr("No object selected"));
        return;
    }
    Gui::Control().showDialog(new TechDrawGui::TaskDlgSurfaceFinishSymbols(objFeat));
}

bool CmdTechDrawSurfaceFinishSymbols::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return havePage && haveView;
}

void CreateTechDrawCommandsAnnotate()
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdTechDrawLeaderLine());
    rcCmdMgr.addCommand(new CmdTechDrawRichTextAnnotation());
    rcCmdMgr.addCommand(new CmdTechDrawCosmeticVertexGroup());
    rcCmdMgr.addCommand(new CmdTechDrawCosmeticVertex());
    rcCmdMgr.addCommand(new CmdTechDrawMidpoints());
    rcCmdMgr.addCommand(new CmdTechDrawQuadrants());
    rcCmdMgr.addCommand(new CmdTechDrawCenterLineGroup());
    rcCmdMgr.addCommand(new CmdTechDrawFaceCenterLine());
    rcCmdMgr.addCommand(new CmdTechDraw2LineCenterLine());
    rcCmdMgr.addCommand(new CmdTechDraw2PointCenterLine());
    rcCmdMgr.addCommand(new CmdTechDraw2PointCosmeticLine());
    rcCmdMgr.addCommand(new CmdTechDrawCosmeticCircle());
    rcCmdMgr.addCommand(new CmdTechDrawAnnotation());
    rcCmdMgr.addCommand(new CmdTechDrawCosmeticEraser());
    rcCmdMgr.addCommand(new CmdTechDrawDecorateLine());
    rcCmdMgr.addCommand(new CmdTechDrawShowAll());
    rcCmdMgr.addCommand(new CmdTechDrawWeldSymbol());
    rcCmdMgr.addCommand(new CmdTechDrawSurfaceFinishSymbols());
}

//===========================================================================
// Selection Validation Helpers
//===========================================================================

std::vector<std::string> getSelectedSubElements(Gui::Command* cmd,
                                                TechDraw::DrawViewPart* &dvp,
                                                std::string subType)
{
//    Base::Console().Message("getSelectedSubElements() - dvp: %X\n", dvp);
    std::vector<std::string> selectedSubs;
    std::vector<std::string> subNames;
    dvp = nullptr;
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++)  {
        if ((*itSel).getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
            dvp = static_cast<TechDraw::DrawViewPart*> ((*itSel).getObject());
            subNames = (*itSel).getSubNames();
            break;
        }
    }
    if (!dvp) {
      QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong Selection"),
                           QObject::tr("No Part View in Selection"));
      return selectedSubs;
    }

    for (auto& s: subNames) {
        if (TechDraw::DrawUtil::getGeomTypeFromName(s) == subType) {
            selectedSubs.push_back(s);
        }
    }

    if (selectedSubs.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Wrong Selection"),
                             QObject::tr("No %1 in Selection")
                                 .arg(QString::fromStdString(subType)));
        return selectedSubs;
    }

    return selectedSubs;
}
