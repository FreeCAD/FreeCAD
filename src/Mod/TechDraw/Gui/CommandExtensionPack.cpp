/***************************************************************************
 *   Copyright (c) 2021 edi                                                *
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
# include <QMessageBox>
# include <iostream>
# include <string>
# include <sstream>
# include <cstdlib>
# include <exception>
#endif  //#ifndef _PreComp_

#include <QGraphicsView>

# include <App/DocumentObject.h>
# include <Base/Exception.h>
#include <Base/Console.h>
#include <Base/Type.h>
# include <Gui/Action.h>
# include <Gui/Application.h>
# include <Gui/BitmapFactory.h>
# include <Gui/Command.h>
# include <Gui/Control.h>
# include <Gui/Document.h>
# include <Gui/Selection.h>
# include <Gui/MainWindow.h>
# include <Gui/FileDialog.h>
# include <Gui/ViewProvider.h>

# include <Mod/Part/App/PartFeature.h>

# include <Mod/TechDraw/App/DrawViewPart.h>
# include <Mod/TechDraw/App/DrawProjGroupItem.h>
# include <Mod/TechDraw/App/DrawProjGroup.h>
# include <Mod/TechDraw/App/DrawViewDimension.h>
# include <Mod/TechDraw/App/DrawDimHelper.h>
# include <Mod/TechDraw/App/LandmarkDimension.h>
# include <Mod/TechDraw/App/DrawPage.h>
# include <Mod/TechDraw/App/DrawUtil.h>
# include <Mod/TechDraw/App/Geometry.h>

#include <Mod/TechDraw/Gui/QGVPage.h>


#include "DrawGuiUtil.h"
#include "MDIViewPage.h"
#include "ViewProviderPage.h"
#include "TaskLinkDim.h"

#include "TaskSelectLineAttributes.h"

/////////////////////////////
#include <Mod/TechDraw/App/DrawViewSection.h>  // needed
#include <Mod/TechDraw/App/DrawProjGroupItem.h>
/////////////////////////////

using namespace TechDrawGui;
using namespace TechDraw;
using namespace std;

lineAttributes activeAttributes; // container holding global line attributes

//internal helper functions
bool _circulation(Base::Vector3d A, Base::Vector3d B, Base::Vector3d C);
Base::Vector3d _circleCenter(Base::Vector3d p1, Base::Vector3d p2, Base::Vector3d p3);
void _createThreadCircle(std::string Name, TechDraw::DrawViewPart* objFeat, float factor);
void _createThreadLines(std::vector<std::string> SubNames, TechDraw::DrawViewPart* objFeat, float factor);
void _setLineAttributes(TechDraw::CosmeticEdge* cosEdge);
void _setLineAttributes(TechDraw::CenterLine* cosEdge);
void _setLineAttributes(TechDraw::CosmeticEdge* cosEdge, int style, float weight, App::Color color);
void _intersection(TechDraw::BaseGeom* geom1, TechDraw::BaseGeom* geom2, std::vector<Base::Vector3d>& interPoints);
void _intersectionLL(TechDraw::BaseGeom* geom1, TechDraw::BaseGeom* geom2, std::vector<Base::Vector3d>& interPoints);
void _intersectionCL(TechDraw::BaseGeom* geom1, TechDraw::BaseGeom* geom2, std::vector<Base::Vector3d>& interPoints);
void _intersectionCC(TechDraw::BaseGeom* geom1, TechDraw::BaseGeom* geom2, std::vector<Base::Vector3d>& interPoints);
float _getAngle(Base::Vector3d center, Base::Vector3d point);
std::vector<Base::Vector3d> _getVertexPoints(std::vector<std::string> SubNames,TechDraw::DrawViewPart* objFeat);
bool _checkSel(Gui::Command* cmd,
               std::vector<Gui::SelectionObject>& selection,
               TechDraw::DrawViewPart*& objFeat,
               std::string message);

//===========================================================================
// TechDraw_ExtensionHoleCircle
//===========================================================================

void execHoleCircle(Gui::Command* cmd){
    //create centerlines of a hole/bolt circle
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSel(cmd,selection,objFeat,"TechDraw Hole Circle"))
        return;
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    std::vector<TechDraw::Circle*> Circles;
    for (std::string Name : SubNames){
        int GeoId = TechDraw::DrawUtil::getIndexFromName(Name);
        std::string GeoType = TechDraw::DrawUtil::getGeomTypeFromName(Name);
        TechDraw::BaseGeom* geom = objFeat->getGeomByIndex(GeoId);
        if (GeoType == "Edge"){
            if (geom->geomType == TechDraw::CIRCLE ||
                geom->geomType == TechDraw::ARCOFCIRCLE){
                TechDraw::Circle* cgen = static_cast<TechDraw::Circle *>(geom);
                Circles.push_back(cgen);
            }
        }
    }
    if (Circles.size() <= 2) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("TechDraw Hole Circle"),
                             QObject::tr("Less then three circles selected"));
        return;
    }
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Draw bolt circle centerlines"));
    double scale = objFeat->getScale();
    Base::Vector3d bigCenter = _circleCenter(Circles[0]->center,
                                             Circles[1]->center,
                                             Circles[2]->center);
    float bigRadius = (Circles[0]->center-bigCenter).Length();
    TechDraw::BaseGeom* bigCircle = new TechDraw::Circle(bigCenter/scale,bigRadius/scale);
    std::string bigCircleTag = objFeat->addCosmeticEdge(bigCircle);
    TechDraw::CosmeticEdge* ceCircle = objFeat->getCosmeticEdge(bigCircleTag);
    _setLineAttributes(ceCircle);
    for (TechDraw::Circle* oneCircle : Circles){
        Base::Vector3d oneCircleCenter = oneCircle->center;
        float oneRadius = oneCircle->radius;
        Base::Vector3d delta = (oneCircle->center-bigCenter).Normalize()*(oneRadius+2);
        Base::Vector3d startPt = oneCircleCenter+delta;
        Base::Vector3d endPt = oneCircleCenter-delta;
        startPt.y = -startPt.y;
        endPt.y = -endPt.y;
        std::string oneLineTag = objFeat->addCosmeticEdge(startPt/scale,endPt/scale);
        TechDraw::CosmeticEdge* ceLine = objFeat->getCosmeticEdge(oneLineTag);
        _setLineAttributes(ceLine);
    }
    cmd->getSelection().clearSelection();
    objFeat->refreshCEGeoms();
    objFeat->requestPaint();
    Gui::Command::commitCommand();
}

DEF_STD_CMD_A(CmdTechDrawExtensionHoleCircle)

CmdTechDrawExtensionHoleCircle::CmdTechDrawExtensionHoleCircle()
  : Command("TechDraw_ExtensionHoleCircle")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Draw bolt circle centerlines");
    sToolTipText    = QT_TR_NOOP("Draw the centerlines of a bolt circle\n\
    - pick favoured line attributes\n\
    - select at least three cirles of a bolt circle\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionHoleCircle";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionHoleCircle";
}

void CmdTechDrawExtensionHoleCircle::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execHoleCircle(this);
    //Base::Console().Message("HoleCircle gestartet\n");
}

bool CmdTechDrawExtensionHoleCircle::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCircleCenterLines
//===========================================================================

void execCircleCenterLines(Gui::Command* cmd){
    // create centerline cross at circles
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSel(cmd,selection,objFeat,"TechDraw Circle Centerlines"))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Draw Circle Centerlines"));
    double scale = objFeat->getScale();
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    for (std::string Name : SubNames) {
        int GeoId = TechDraw::DrawUtil::getIndexFromName(Name);
        TechDraw::BaseGeom* geom = objFeat->getGeomByIndex(GeoId);
        std::string GeoType = TechDraw::DrawUtil::getGeomTypeFromName(Name);
        if (GeoType == "Edge"){
            if (geom->geomType == TechDraw::CIRCLE ||
                geom->geomType == TechDraw::ARCOFCIRCLE){
                TechDraw::Circle* cgen = static_cast<TechDraw::Circle *>(geom);
                Base::Vector3d center = cgen->center;
                center.y = -center.y;
                float radius = cgen->radius;
                Base::Vector3d right(center.x+radius+2.0,center.y,0.0);
                Base::Vector3d top(center.x,center.y+radius+2.0,0.0);
                Base::Vector3d left(center.x-radius-2.0,center.y,0.0);
                Base::Vector3d bottom(center.x,center.y-radius-2.0,0.0);
                std::string line1tag = objFeat->addCosmeticEdge(right/scale, left/scale);
                std::string line2tag = objFeat->addCosmeticEdge(top/scale, bottom/scale);
                TechDraw::CosmeticEdge* horiz = objFeat->getCosmeticEdge(line1tag);
                _setLineAttributes(horiz);
                TechDraw::CosmeticEdge* vert = objFeat->getCosmeticEdge(line2tag);
                _setLineAttributes(vert);
            }
        }
    }
    cmd->getSelection().clearSelection();
    objFeat->refreshCEGeoms();
    objFeat->requestPaint();
    Gui::Command::commitCommand();
}

DEF_STD_CMD_A(CmdTechDrawExtensionCircleCenterLines)

CmdTechDrawExtensionCircleCenterLines::CmdTechDrawExtensionCircleCenterLines()
  : Command("TechDraw_ExtensionCircleCenterLines")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Draw circle center lines");
    sToolTipText    = QT_TR_NOOP("Draw circle center line cross at circles\n\
    - pick favoured line attributes\n\
    - select many circles or arcs\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionCircleCenterLines";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionCircleCenterLines";
}

void CmdTechDrawExtensionCircleCenterLines::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execCircleCenterLines(this);
}

bool CmdTechDrawExtensionCircleCenterLines::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCircleCenterLinesGroup
//===========================================================================

DEF_STD_CMD_ACL(CmdTechDrawExtensionCircleCenterLinesGroup)

CmdTechDrawExtensionCircleCenterLinesGroup::CmdTechDrawExtensionCircleCenterLinesGroup()
  : Command("TechDraw_ExtensionCircleCenterLinesGroup")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Insert Circle Center Line");
    sToolTipText    = QT_TR_NOOP("Draw circle center line cross at circles\n\
    - pick favoured line attributes\n\
    - select many circles or arcs\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionCircleCenterLinesGroup";
    sStatusTip      = sToolTipText;

}

void CmdTechDrawExtensionCircleCenterLinesGroup::activated(int iMsg){
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg != nullptr) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch(iMsg) {
        case 0:                 //circle center lines
            execCircleCenterLines(this);
            break;
        case 1:                 //bolt circle center lines
            execHoleCircle(this);
            break;
        default:
            Base::Console().Message("CMD::CVGrp - invalid iMsg: %d\n",iMsg);
    };
}

Gui::Action * CmdTechDrawExtensionCircleCenterLinesGroup::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionCircleCenterLines"));
    p1->setObjectName(QString::fromLatin1("TechDraw_ExtensionCircleCenterLines"));
    p1->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionCircleCenterLines"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionHoleCircle"));
    p2->setObjectName(QString::fromLatin1("TechDraw_ExtensionHoleCircle"));
    p2->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionHoleCircle"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(p1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdTechDrawExtensionCircleCenterLinesGroup::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* arc1 = a[0];
    arc1->setText(QApplication::translate("TechDraw_Extension","Add Centerlines to Circles"));
    arc1->setToolTip(QApplication::translate("TechDraw_Extension",
    "Draw circle center line cross at circles\n\
    - pick favoured line attributes\n\
    - select many circles or arcs\n\
    - click this button"));
    arc1->setStatusTip(arc1->toolTip());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("TechDraw_Extension","Add Centerlines to Boltcircle"));
    arc2->setToolTip(QApplication::translate("TechDraw_Extension",
    "Draw the centerlines of a bolt circle\n\
    - pick favoured line attributes\n\
    - select at least three cirles of a bolt circle\n\
    - click this buttone"));
    arc2->setStatusTip(arc2->toolTip());
}

bool CmdTechDrawExtensionCircleCenterLinesGroup::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionThreadHoleSide
//===========================================================================

void execThreadHoleSide(Gui::Command* cmd){
    // create symbolic thread in a hole seen from side
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSel(cmd,selection,objFeat,"TechDraw Thread Hole Side"))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Thread Hole Side"));
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    if (SubNames.size() >= 2) {
        _createThreadLines(SubNames, objFeat, 1.176f);
    }
    cmd->getSelection().clearSelection();
    objFeat->refreshCEGeoms();
    objFeat->requestPaint();
    Gui::Command::commitCommand();
}

DEF_STD_CMD_A(CmdTechDrawExtensionThreadHoleSide)

CmdTechDrawExtensionThreadHoleSide::CmdTechDrawExtensionThreadHoleSide()
  : Command("TechDraw_ExtensionThreadHoleSide")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Cosmetic thread hole side view");
    sToolTipText    = QT_TR_NOOP("Draw cosmetic thread hole side view\n\
    - pick favoured line attributes\n\
    - select two parallel lines\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionThreadHoleSide";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionThreadHoleSide";
}

void CmdTechDrawExtensionThreadHoleSide::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execThreadHoleSide(this);
}

bool CmdTechDrawExtensionThreadHoleSide::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionThreadBoltSide
//===========================================================================

void execThreadBoltSide(Gui::Command* cmd){
    // create symbolic thread at a bolt seen from side
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSel(cmd,selection,objFeat,"TechDraw Thread Bolt Side"))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Thread Bolt Side"));
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    if (SubNames.size() >= 2) {
        _createThreadLines(SubNames, objFeat, 0.85f);
    }
    cmd->getSelection().clearSelection();
    objFeat->refreshCEGeoms();
    objFeat->requestPaint();
    Gui::Command::commitCommand();
}

DEF_STD_CMD_A(CmdTechDrawExtensionThreadBoltSide)

CmdTechDrawExtensionThreadBoltSide::CmdTechDrawExtensionThreadBoltSide()
  : Command("TechDraw_ExtensionThreadBoltSide")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Cosmetic thread bolt side view");
    sToolTipText    = QT_TR_NOOP("Thread Screw/pin/shaft side view/section\n\
    - pick favoured line attributes\n\
    - select two parallel lines\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionThreadBoltSide";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionThreadBoltSide";
}

void CmdTechDrawExtensionThreadBoltSide::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execThreadBoltSide(this);
}

bool CmdTechDrawExtensionThreadBoltSide::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionThreadHoleBottom
//===========================================================================

void execThreadHoleBottom(Gui::Command* cmd){
    // create symbolic thread in a hole seen from bottom
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSel(cmd,selection,objFeat,"TechDraw Thread Hole Bottom"))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Thread Hole Bottom"));
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    for (std::string Name : SubNames) {
        _createThreadCircle(Name, objFeat, 1.177f);
    }
    cmd->getSelection().clearSelection();
    objFeat->refreshCEGeoms();
    objFeat->requestPaint();
    Gui::Command::commitCommand();  
}

DEF_STD_CMD_A(CmdTechDrawExtensionThreadHoleBottom)

CmdTechDrawExtensionThreadHoleBottom::CmdTechDrawExtensionThreadHoleBottom()
  : Command("TechDraw_ExtensionThreadHoleBottom")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Cosmetic hole thread ground view");
    sToolTipText    = QT_TR_NOOP("Draw cosmetic hole threads ground view\n\
    - pick favoured line attributes\n\
    - select many circles\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionThreadHoleBottom";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionThreadHoleBottom";
}

void CmdTechDrawExtensionThreadHoleBottom::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execThreadHoleBottom(this);
}

bool CmdTechDrawExtensionThreadHoleBottom::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionThreadBoltBottom
//===========================================================================

void execThreadBoltBottom(Gui::Command* cmd){
    // create symbolic thread at a bolt seen from bottom
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSel(cmd,selection,objFeat,"TechDraw Thread Bolt Bottom"))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Thread Bolt Bottom"));
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    for (std::string Name : SubNames) {
        _createThreadCircle(Name, objFeat, 0.85f);
    }
    cmd->getSelection().clearSelection();
    objFeat->refreshCEGeoms();
    objFeat->requestPaint();
    Gui::Command::commitCommand();  
}

DEF_STD_CMD_A(CmdTechDrawExtensionThreadBoltBottom)

CmdTechDrawExtensionThreadBoltBottom::CmdTechDrawExtensionThreadBoltBottom()
  : Command("TechDraw_ExtensionThreadBoltBottom")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Screw/pin/shaft thread in plan");
    sToolTipText    = QT_TR_NOOP("Draw the technical symbol of the thread in the screw/pin/shaft plant\n\
    - pick favoured line attributes\n\
    - select many circles\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionThreadBoltBottom";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionThreadBoltBottom";
}

void CmdTechDrawExtensionThreadBoltBottom::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execThreadBoltBottom(this);
}

bool CmdTechDrawExtensionThreadBoltBottom::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionThreadsGroup
//===========================================================================

DEF_STD_CMD_ACL(CmdTechDrawExtensionThreadsGroup)

CmdTechDrawExtensionThreadsGroup::CmdTechDrawExtensionThreadsGroup()
  : Command("TechDraw_ExtensionThreadsGroup")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Cosmetic thread hole side view");
    sToolTipText    = QT_TR_NOOP("Draw cosmetic thread hole side view\n\
    - pick favoured line attributes\n\
    - select two parallel lines\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionThreadsGroup";
    sStatusTip      = sToolTipText;
}

void CmdTechDrawExtensionThreadsGroup::activated(int iMsg)
{
//    Base::Console().Message("CMD::TechDrawExtensionThreadsGroup - activated(%d)\n", iMsg);
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg != nullptr) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch(iMsg) {
        case 0:                 //thread hole side view
            execThreadHoleSide(this);
            break;
        case 1:                 //thread hole bottom view
            execThreadHoleBottom(this);
            break;
        case 2:                 //thread bolt side view
            execThreadBoltSide(this);
            break;
        case 3:                 //thread bolt bottom view
            execThreadBoltBottom(this);
            break;
        default:
            Base::Console().Message("CMD::CVGrp - invalid iMsg: %d\n",iMsg);
    };
}

Gui::Action * CmdTechDrawExtensionThreadsGroup::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionThreadHoleSide"));
    p1->setObjectName(QString::fromLatin1("TechDraw_ExtensionThreadHoleSide"));
    p1->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionThreadHoleSide"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionThreadHoleBottom"));
    p2->setObjectName(QString::fromLatin1("TechDraw_ExtensionThreadHoleBottom"));
    p2->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionThreadHoleBottom"));
    QAction* p3 = pcAction->addAction(QString());
    p3->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionThreadBoltSide"));
    p3->setObjectName(QString::fromLatin1("TechDraw_ExtensionThreadBoltSide"));
    p3->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionThreadBoltSide"));
    QAction* p4 = pcAction->addAction(QString());
    p4->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionThreadBoltBottom"));
    p4->setObjectName(QString::fromLatin1("TechDraw_ExtensionThreadBoltBottom"));
    p4->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionThreadBoltBottom"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(p1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdTechDrawExtensionThreadsGroup::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* arc1 = a[0];
    arc1->setText(QApplication::translate("TechDraw_Extension","Create Thread Hole Side View"));
    arc1->setToolTip(QApplication::translate("TechDraw_Extension",
    "Draw cosmetic thread hole side view\n\
    - pick favoured line attributes\n\
    - select two parallel lines\n\
    - click this button"));
    arc1->setStatusTip(arc1->toolTip());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("TechDraw_Extension","Create Thread Hole Bottom View"));
    arc2->setToolTip(QApplication::translate("TechDraw_Extension",
    "Draw cosmetic hole threads ground view\n\
    - pick favoured line attributes\n\
    - select many circles\n\
    - click this button"));
    arc2->setStatusTip(arc2->toolTip());
    QAction* arc3 = a[2];
    arc3->setText(QApplication::translate("TechDraw_Extension","Create Thread Bolt Side View"));
    arc3->setToolTip(QApplication::translate("TechDraw_Extension",
    "Thread Screw/pin/shaft side view/section\n\
    - pick favoured line attributes\n\
    - select two parallel lines\n\
    - click this button"));
    arc3->setStatusTip(arc3->toolTip());
    QAction* arc4 = a[3];
    arc4->setText(QApplication::translate("TechDraw_Extension","Create Thread Bolt Bottom View"));
    arc4->setToolTip(QApplication::translate("TechDraw_Extension",
    "Draw the technical symbol of the thread in the screw/pin/shaft plant\n\
    - pick favoured line attributes\n\
    - select many circles\n\
    - click this button"));
    arc4->setStatusTip(arc4->toolTip());
}

bool CmdTechDrawExtensionThreadsGroup::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionSelectLineAttributes
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawExtensionSelectLineAttributes)

CmdTechDrawExtensionSelectLineAttributes::CmdTechDrawExtensionSelectLineAttributes()
  : Command("TechDraw_ExtensionSelectLineAttributes")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Select line attributes");
    sToolTipText    = QT_TR_NOOP("Select the line attributes\n\
    - click this button\n\
    - select line attributes in opened window");
    sWhatsThis      = "TechDraw_ExtensionSelectLineAttributes";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionSelectLineAttributes";
}

void CmdTechDrawExtensionSelectLineAttributes::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Control().showDialog(new TaskDlgSelectLineAttributes(& activeAttributes));
}

bool CmdTechDrawExtensionSelectLineAttributes::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionChangeLineAttributes
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawExtensionChangeLineAttributes)

CmdTechDrawExtensionChangeLineAttributes::CmdTechDrawExtensionChangeLineAttributes()
  : Command("TechDraw_ExtensionChangeLineAttributes")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Change the line attributes");
    sToolTipText    = QT_TR_NOOP("Change the attributes of selected lines\n\
    - pick favoured line attributes\n\
    - select many cosmetic or center lines\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionChangeLineAttributes";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionChangeLineAttributes";
}

void CmdTechDrawExtensionChangeLineAttributes::activated(int iMsg){
    // change attributes (type, width, color) of a cosmetic or centerline
    Q_UNUSED(iMsg);
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSel(this,selection,objFeat,"TechDraw Change Line Attributes"))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Change Line Attributes"));
    const std::vector<std::string> subNames = selection[0].getSubNames();
    for (std::string name : subNames){
        int num = DrawUtil::getIndexFromName(name);
        BaseGeom* baseGeo = objFeat->getGeomByIndex(num);
        if (baseGeo != nullptr){
            if (baseGeo->cosmetic){
                if (baseGeo->source() == 1){
                    TechDraw::CosmeticEdge* cosEdgeTag = objFeat->getCosmeticEdgeBySelection(name);
                    _setLineAttributes(cosEdgeTag);
                } else if (baseGeo->source() == 2){
                    TechDraw::CenterLine* centerLineTag = objFeat->getCenterLineBySelection(name);
                    _setLineAttributes(centerLineTag);
                }
            }
        }
    }
    getSelection().clearSelection();
    objFeat->refreshCEGeoms();
    objFeat->requestPaint();
    Gui::Command::commitCommand(); 
}

bool CmdTechDrawExtensionChangeLineAttributes::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionVertexAtIntersection
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawExtensionVertexAtIntersection)

CmdTechDrawExtensionVertexAtIntersection::CmdTechDrawExtensionVertexAtIntersection()
  : Command("TechDraw_ExtensionVertexAtIntersection")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Create Vertex(es) at Intersection");
    sToolTipText    = QT_TR_NOOP("Create the vertexes at intersection of lines\n\
    - select two lines/circles/arcs\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionVertexAtIntersection";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionVertexAtIntersection";
}

void CmdTechDrawExtensionVertexAtIntersection::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    //Base::Console().Message("VertexAtIntersection started\n");
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSel(this,selection,objFeat,"TechDraw Create Vertex at Intersection"))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create Vertex at Intersection"));
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    std::vector<Base::Vector3d> interPoints;
    if (SubNames.size() >=2){
        std::string GeoType1 = TechDraw::DrawUtil::getGeomTypeFromName(SubNames[0]);
        std::string GeoType2 = TechDraw::DrawUtil::getGeomTypeFromName(SubNames[1]);
        if (GeoType1 == "Edge" && GeoType2 == "Edge"){
            int GeoId1 = TechDraw::DrawUtil::getIndexFromName(SubNames[0]);
            TechDraw::BaseGeom* geom1 = objFeat->getGeomByIndex(GeoId1);
            int GeoId2 = TechDraw::DrawUtil::getIndexFromName(SubNames[1]);
            TechDraw::BaseGeom* geom2 = objFeat->getGeomByIndex(GeoId2);
            _intersection(geom1, geom2, interPoints);
            if (!interPoints.empty()){
                double scale = objFeat->getScale();
                std::string id1 = objFeat->addCosmeticVertex(interPoints[0]/scale);
                objFeat->add1CVToGV(id1);
                if (interPoints.size() >= 2){
                    std::string id2 = objFeat->addCosmeticVertex(interPoints[1]/scale);
                    objFeat->add1CVToGV(id2);
                }
            }
        }
    }
    getSelection().clearSelection();
    objFeat->refreshCEGeoms();
    objFeat->requestPaint();
    Gui::Command::commitCommand();
}

bool CmdTechDrawExtensionVertexAtIntersection::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionDrawArc
//===========================================================================

void execDrawCosmArc(Gui::Command* cmd){
    //draw a cosmetic arc of circle
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSel(cmd,selection,objFeat,"TechDraw Draw Cosmetic Arc of Circle"))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Draw Cosmetic Arc"));
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    std::vector<Base::Vector3d> vertexPoints;
    vertexPoints = _getVertexPoints(SubNames,objFeat);
    if (vertexPoints.size() >= 3){
        double scale = objFeat->getScale();
        float arcRadius = (vertexPoints[1]-vertexPoints[0]).Length();
        float angle1 = _getAngle(vertexPoints[0],vertexPoints[1]);
        float angle2 = _getAngle(vertexPoints[0],vertexPoints[2]);
        TechDraw::BaseGeom* baseGeo = new TechDraw::AOC(vertexPoints[0]/scale, arcRadius/scale, -angle2, -angle1);
        std::string arcTag = objFeat->addCosmeticEdge(baseGeo);
        TechDraw::CosmeticEdge* arcEdge = objFeat->getCosmeticEdge(arcTag);
        _setLineAttributes(arcEdge);
        objFeat->refreshCEGeoms();
        objFeat->requestPaint();
        cmd->getSelection().clearSelection();
        Gui::Command::commitCommand();
    }
}

DEF_STD_CMD_A(CmdTechDrawExtensionArc)

CmdTechDrawExtensionArc::CmdTechDrawExtensionArc()
  : Command("TechDraw_ExtensionArc")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Draw an cosmetic arc (center and two vertexes)");
    sToolTipText    = QT_TR_NOOP("Draw an arc rotating math. positive\n\
    - select three vertexes:\n\
    - center, start, end\n\
    - start defines the radius\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionArc";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionArc";
}

void CmdTechDrawExtensionArc::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    //Base::Console().Message("Cosmetic Arc gestartet\n");
    execDrawCosmArc(this);
}

bool CmdTechDrawExtensionArc::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionDrawCosmCircle
//===========================================================================

void execDrawCosmCircle(Gui::Command* cmd){
    //draw a cosmetic circle
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSel(cmd,selection,objFeat,"TechDraw Draw Cosmetic Circle"))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Draw Cosmetic Circle"));
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    std::vector<Base::Vector3d> vertexPoints;
    vertexPoints = _getVertexPoints(SubNames,objFeat);
    if (vertexPoints.size() >= 2){
        double scale = objFeat->getScale();
        float circleRadius = (vertexPoints[1]-vertexPoints[0]).Length();
        TechDraw::BaseGeom* baseGeo = new TechDraw::Circle(vertexPoints[0]/scale, circleRadius/scale);
        std::string cicleTag = objFeat->addCosmeticEdge(baseGeo);
        TechDraw::CosmeticEdge* circleEdge = objFeat->getCosmeticEdge(cicleTag);
        _setLineAttributes(circleEdge);
        objFeat->refreshCEGeoms();
        objFeat->requestPaint();
        cmd->getSelection().clearSelection();
        Gui::Command::commitCommand();
    }
}

DEF_STD_CMD_A(CmdTechDrawExtensionDrawCosmCircle)

CmdTechDrawExtensionDrawCosmCircle::CmdTechDrawExtensionDrawCosmCircle()
  : Command("TechDraw_ExtensionDrawCosmCircle")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Draw an cosmetic circumference (center and 1 vertex)");
    sToolTipText    = QT_TR_NOOP("Draw a cosmetic circumference using two vertices\n\
    - choose the line attributes\n\
    - select the first vertex (center) -> in sequence the second (radius)\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionDrawCosmCircle";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionDrawCosmCircle";
}

void CmdTechDrawExtensionDrawCosmCircle::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    //Base::Console().Message("Cosmetic Circle gestartet\n");
    execDrawCosmCircle(this);
}

bool CmdTechDrawExtensionDrawCosmCircle::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionDrawCirclesGroup
//===========================================================================

DEF_STD_CMD_ACL(CmdTechDrawExtensionDrawCirclesGroup)

CmdTechDrawExtensionDrawCirclesGroup::CmdTechDrawExtensionDrawCirclesGroup()
  : Command("TechDraw_ExtensionDrawCirclesGroup")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Draw an cosmetic circumference (center and 1 vertex)");
    sToolTipText    = QT_TR_NOOP("Draw a cosmetic circumference using two vertices\n\
    - choose the line attributes\n\
    - select the first vertex (center) -> in sequence the second (radius)\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionDrawCirclesGroup";
    sStatusTip      = sToolTipText;
}

void CmdTechDrawExtensionDrawCirclesGroup::activated(int iMsg)
{
//    Base::Console().Message("CMD::ExtensionDrawCirclesGroup - activated(%d)\n", iMsg);
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg != nullptr) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch(iMsg) {
        case 0:                 //draw cosmetic circle
            execDrawCosmCircle(this);
            break;
        case 1:                 //draw cosmetic arc
            execDrawCosmArc(this);
            break;
        default:
            Base::Console().Message("CMD::CVGrp - invalid iMsg: %d\n",iMsg);
    };
}

Gui::Action * CmdTechDrawExtensionDrawCirclesGroup::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionDrawCosmCircle"));
    p1->setObjectName(QString::fromLatin1("TechDraw_ExtensionDrawCosmCircle"));
    p1->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionDrawCosmCircle"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionArc"));
    p2->setObjectName(QString::fromLatin1("TechDraw_ExtensionArc"));
    p2->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionArc"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(p1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdTechDrawExtensionDrawCirclesGroup::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* arc1 = a[0];
    arc1->setText(QApplication::translate("TechDraw_Extension","Draw Cosmetic Circle"));
    arc1->setToolTip(QApplication::translate("TechDraw_Extension",
    "Draw a cosmetic circumference using two vertices\n\
    - choose the line attributes\n\
    - select the first vertex (center) -> in sequence the second (radius)\n\
    - click this button"));
    arc1->setStatusTip(arc1->toolTip());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("TechDraw_Extension","Draw Cosmetic Arc"));
    arc2->setToolTip(QApplication::translate("TechDraw_Extension",
    "Draw an arc rotating math. positive\n\
    - select three vertexes:\n\
    - center, start, end\n\
    - start defines the radius\n\
    - click this buttonc"));
    arc2->setStatusTip(arc2->toolTip());
}

bool CmdTechDrawExtensionDrawCirclesGroup::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionLineParallel
//===========================================================================

void execLineParallelPerpendicular(Gui::Command* cmd, bool isParallel){
    // create a line parallel or perpendicular to another line
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSel(cmd,selection,objFeat,"TechDraw Create Line Parallel/Perpendicular"))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create Line Parallel/Perpendicular"));
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    if (SubNames.size() >= 2){
        std::string GeoType1 = TechDraw::DrawUtil::getGeomTypeFromName(SubNames[0]);
        std::string GeoType2 = TechDraw::DrawUtil::getGeomTypeFromName(SubNames[1]);
        if (GeoType1 == "Edge" && GeoType2 == "Vertex"){
            double scale = objFeat->getScale();
            int GeoId1 = TechDraw::DrawUtil::getIndexFromName(SubNames[0]);
            TechDraw::BaseGeom* geom1 = objFeat->getGeomByIndex(GeoId1);
            int GeoId2 = TechDraw::DrawUtil::getIndexFromName(SubNames[1]);
            TechDraw::Generic* lineGen = static_cast<TechDraw::Generic *>(geom1);
            Base::Vector3d lineStart = lineGen->points.at(0);
            Base::Vector3d lineEnd = lineGen->points.at(1);
            TechDraw::VertexPtr vert = objFeat->getProjVertexByIndex(GeoId2);
            Base::Vector3d vertexPoint(vert->point().x,vert->point().y,0.0);
            Base::Vector3d halfVector = (lineEnd-lineStart)/2.0;
            if (!isParallel){
                float dummy = halfVector.x;
                halfVector.x = -halfVector.y;
                halfVector.y = dummy;
            }
            Base::Vector3d startPoint = vertexPoint+halfVector;
            Base::Vector3d endPoint = vertexPoint-halfVector;
            startPoint.y = -startPoint.y;
            endPoint.y = -endPoint.y;
            std::string lineTag = objFeat->addCosmeticEdge(startPoint/scale, endPoint/scale);
            TechDraw::CosmeticEdge* lineEdge = objFeat->getCosmeticEdge(lineTag);
            _setLineAttributes(lineEdge);
            objFeat->refreshCEGeoms();
            objFeat->requestPaint();
            cmd->getSelection().clearSelection();
        }
    }
    Gui::Command::commitCommand();
}

DEF_STD_CMD_A(CmdTechDrawExtensionLineParallel)

CmdTechDrawExtensionLineParallel::CmdTechDrawExtensionLineParallel()
  : Command("TechDraw_ExtensionLineParallel")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Create a line parallel to another line");
    sToolTipText    = QT_TR_NOOP("Create a line parallel to another line through a vertex\n\
    - choose the line attributes\n\
    - select one line\n\
    - select one vertex\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionLineParallel";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionLineParallel";
}

void CmdTechDrawExtensionLineParallel::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execLineParallelPerpendicular(this,true);
}

bool CmdTechDrawExtensionLineParallel::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionLinePerpendicular
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawExtensionLinePerpendicular)

CmdTechDrawExtensionLinePerpendicular::CmdTechDrawExtensionLinePerpendicular()
  : Command("TechDraw_ExtensionLinePerpendicular")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Create a line perpendicular to another line");
    sToolTipText    = QT_TR_NOOP("Create a line perpendicular to another line through a vertex\n\
    - choose the line attributes\n\
    - select one line\n\
    - select one vertex\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionLinePerpendicular";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionLinePerpendicular";
}

void CmdTechDrawExtensionLinePerpendicular::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execLineParallelPerpendicular(this,false);
    ///Base::Console().Message("Create perpendiculararallel line started\n");
}

bool CmdTechDrawExtensionLinePerpendicular::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionLinePPGroup
//===========================================================================

DEF_STD_CMD_ACL(CmdTechDrawExtensionLinePPGroup)

CmdTechDrawExtensionLinePPGroup::CmdTechDrawExtensionLinePPGroup()
  : Command("TechDraw_ExtensionLinePPGroup")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Create a line parallel to another line");
    sToolTipText    = QT_TR_NOOP("Create a line parallel to another line through a vertex\n\
    - choose the line attributes\n\
    - select one line\n\
    - select one vertex\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionLinePPGroup";
    sStatusTip      = sToolTipText;
}

void CmdTechDrawExtensionLinePPGroup::activated(int iMsg)
{
//    Base::Console().Message("CMD::ExtensionLinePPGroup - activated(%d)\n", iMsg);
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg != nullptr) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch(iMsg) {
        case 0:                 //create parallel line
            execLineParallelPerpendicular(this,true);
            break;
        case 1:                 //create perpendicular line
            execLineParallelPerpendicular(this,false);
            break;
        default:
            Base::Console().Message("CMD::CVGrp - invalid iMsg: %d\n",iMsg);
    };
}

Gui::Action * CmdTechDrawExtensionLinePPGroup::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionLineParallel"));
    p1->setObjectName(QString::fromLatin1("TechDraw_ExtensionLineParallel"));
    p1->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionLineParallel"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionLinePerpendicular"));
    p2->setObjectName(QString::fromLatin1("TechDraw_ExtensionLinePerpendicular"));
    p2->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionLinePerpendicular"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(p1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdTechDrawExtensionLinePPGroup::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* arc1 = a[0];
    arc1->setText(QApplication::translate("TechDraw_Extension","Create parallel Line"));
    arc1->setToolTip(QApplication::translate("TechDraw_Extension",
    "Create a line parallel to another line through a vertex\n\
    - choose the line attributes\n\
    - select one line\n\
    - select one vertex\n\
    - click this button"));
    arc1->setStatusTip(arc1->toolTip());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("TechDraw_Extension","Create perpendicular Line"));
    arc2->setToolTip(QApplication::translate("TechDraw_Extension",
    "Create a line perpendicular to another line through a vertex\n\
    - choose the line attributes\n\
    - select one line\n\
    - select one vertex\n\
    - click this button"));
    arc2->setStatusTip(arc2->toolTip());
}

bool CmdTechDrawExtensionLinePPGroup::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionLockUnlockView
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawExtensionLockUnlockView)

CmdTechDrawExtensionLockUnlockView::CmdTechDrawExtensionLockUnlockView()
  : Command("TechDraw_ExtensionLockUnlockView")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Lock/Unlock a View");
    sToolTipText    = QT_TR_NOOP("Lock/Unlock a View\n\
    - select a view\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionLockUnlockView";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionLockUnlockView";
}

void CmdTechDrawExtensionLockUnlockView::activated(int iMsg){
    // lock/unlock a selected view
    Q_UNUSED(iMsg);
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSel(this,selection,objFeat,"TechDraw Lock/Unlock View"))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Lock/Unlock View"));
    if (objFeat->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())){
        auto objFeat = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
        bool lockPosition = objFeat->LockPosition.getValue();
        lockPosition = !lockPosition;
        objFeat->LockPosition.setValue(lockPosition);
    }
    Gui::Command::commitCommand();
}

bool CmdTechDrawExtensionLockUnlockView::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionPositionSectionView
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawExtensionPositionSectionView)

CmdTechDrawExtensionPositionSectionView::CmdTechDrawExtensionPositionSectionView()
  : Command("TechDraw_ExtensionPositionSectionView")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Orthogonal projection group: section view positioning");
    sToolTipText    = QT_TR_NOOP("Position a section view at same x or y as its base view\n\
    - select a section view\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionPositionSectionView";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionPositionSectionView";
}

void CmdTechDrawExtensionPositionSectionView::activated(int iMsg){
    // position a section view
    Q_UNUSED(iMsg);
    //Base::Console().Message("PositionSectionView started\n");
    auto selection = getSelection().getSelectionEx();
    if( selection.empty() ) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("TechDraw Lock/Unlock View"),
                             QObject::tr("Selection is empty"));
        return;
    }
    float xPos, yPos;
    TechDraw::DrawViewPart* baseView;
    auto objFeat = selection[0].getObject();
    if (objFeat->isDerivedFrom(TechDraw::DrawViewSection::getClassTypeId())){
        TechDraw::DrawViewSection* sectionView = dynamic_cast<TechDraw::DrawViewSection*>(objFeat);
        baseView = sectionView->getBaseDVP();
        if (baseView->isDerivedFrom(TechDraw::DrawProjGroupItem::getClassTypeId())){
            std::vector<App::DocumentObject*> parentViews = baseView->getInList();
            if (!parentViews.empty()){
                TechDraw::DrawProjGroup* groupBase = dynamic_cast<TechDraw::DrawProjGroup*>(parentViews[0]);
                xPos = groupBase->X.getValue();
                yPos = groupBase->Y.getValue();
            } 
        }
        else {
            xPos = baseView->X.getValue();
            yPos = baseView->Y.getValue();
        }
        std::string direction = sectionView->SectionDirection.getValueAsString();
        if ((direction == "Right") || (direction == "Left"))
            sectionView->Y.setValue(yPos);
        else if ((direction == "Up") || (direction == "Down"))
            sectionView->X.setValue(xPos);
    }
}

bool CmdTechDrawExtensionPositionSectionView::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionExtendLine
//===========================================================================

void execExtendShortenLine(Gui::Command* cmd, bool extend){
    // extend or shorten a cosmetic or a center line
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSel(cmd,selection,objFeat,"TechDraw Extend/shorten a Line"))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Extend/shorten a Line"));
    const std::vector<std::string> subNames = selection[0].getSubNames();
    if (!subNames.empty()){
        std::string name = subNames[0];
        int num = DrawUtil::getIndexFromName(name);
        std::string geoType = TechDraw::DrawUtil::getGeomTypeFromName(name);
        if (geoType == "Edge"){
            TechDraw::BaseGeom* baseGeo = objFeat->getGeomByIndex(num);
            if (baseGeo != nullptr){
                if (baseGeo->geomType == TechDraw::GENERIC){ 
                    TechDraw::Generic* genLine = static_cast<TechDraw::Generic*>(baseGeo);
                    Base::Vector3d P0 = genLine->points.at(0);
                    Base::Vector3d P1 = genLine->points.at(1);
                    if (baseGeo->cosmetic){
                        std::string uniTag = baseGeo->getCosmeticTag();
                        int oldStyle;
                        float oldWeight;
                        App::Color oldColor;
                        std::vector<std::string> toDelete;
                        toDelete.push_back(uniTag);
                        if (baseGeo->source() == 1){
                            auto cosEdge = objFeat->getCosmeticEdge(uniTag);
                            oldStyle = cosEdge->m_format.m_style; 
                            oldWeight = cosEdge->m_format.m_weight;
                            oldColor = cosEdge->m_format.m_color;
                            objFeat->removeCosmeticEdge(toDelete);
                        }  
                        else if (baseGeo->source() == 2){
                            auto centerEdge = objFeat->getCenterLine(uniTag);
                            oldStyle = centerEdge->m_format.m_style; 
                            oldWeight = centerEdge->m_format.m_weight;
                            oldColor = centerEdge->m_format.m_color;
                            objFeat->removeCenterLine(toDelete);
                        }
                        double scale = objFeat->getScale();
                        Base::Vector3d direction = (P1-P0).Normalize();
                        Base::Vector3d delta = direction*2.0;
                        Base::Vector3d startPt, endPt;
                        if (extend){
                            startPt = P0-delta;
                            endPt = P1+delta;
                        } else {
                            startPt = P0+delta;
                            endPt = P1-delta;
                        }
                        startPt.y = -startPt.y;
                        endPt.y = -endPt.y;
                        std::string lineTag = objFeat->addCosmeticEdge(startPt/scale,endPt/scale);
                        TechDraw::CosmeticEdge* lineEdge = objFeat->getCosmeticEdge(lineTag);
                        _setLineAttributes(lineEdge,oldStyle,oldWeight,oldColor);
                        cmd->getSelection().clearSelection();
                        objFeat->refreshCEGeoms();
                        objFeat->refreshCLGeoms();
                        objFeat->requestPaint();
                    }
                }
            }
        }
    }
    Gui::Command::commitCommand();
}

DEF_STD_CMD_A(CmdTechDrawExtensionExtendLine)

CmdTechDrawExtensionExtendLine::CmdTechDrawExtensionExtendLine()
  : Command("TechDraw_ExtensionExtendLine")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Extend a Line");
    sToolTipText    = QT_TR_NOOP("Extend a line at both ends\n\
    - select one cosmetic or centerline\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionExtendLine";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionExtendLine";
}

void CmdTechDrawExtensionExtendLine::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execExtendShortenLine(this,true);
    ///Base::Console().Message("ExtendLine started\n");
}

bool CmdTechDrawExtensionExtendLine::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionShortenLine
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawExtensionShortenLine)

CmdTechDrawExtensionShortenLine::CmdTechDrawExtensionShortenLine()
  : Command("TechDraw_ExtensionShortenLine")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Shorten a Line");
    sToolTipText    = QT_TR_NOOP("Shorten a line at both ends\n\
    - select one cosmetic or centerline\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionShortenLine";
    sStatusTip      = sToolTipText;
    sPixmap         = "TechDraw_ExtensionShortenLine";
}

void CmdTechDrawExtensionShortenLine::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execExtendShortenLine(this,false);
    ///Base::Console().Message("ShortenLine started\n");
}

bool CmdTechDrawExtensionShortenLine::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionExtendShortenLineGroup
//===========================================================================

DEF_STD_CMD_ACL(CmdTechDrawExtendShortenLineGroup)

CmdTechDrawExtendShortenLineGroup::CmdTechDrawExtendShortenLineGroup()
  : Command("TechDraw_ExtensionExtendShortenLineGroup")
{
    sAppModule      = "TechDraw";
    sGroup          = QT_TR_NOOP("TechDraw");
    sMenuText       = QT_TR_NOOP("Extend a Line");
    sToolTipText    = QT_TR_NOOP("Extend a line at both ends\n\
    - select one cosmetic or centerline\n\
    - click this button");
    sWhatsThis      = "TechDraw_ExtensionExtendShortenLineGroup";
    sStatusTip      = sToolTipText;
}

void CmdTechDrawExtendShortenLineGroup::activated(int iMsg)
{
//    Base::Console().Message("CMD::ExtendShortenLineGroup - activated(%d)\n", iMsg);
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    if (dlg != nullptr) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
            QObject::tr("Close active task dialog and try again."));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch(iMsg) {
        case 0:                 //extend a line
            execExtendShortenLine(this,true);
            break;
        case 1:                 //shorten line
            execExtendShortenLine(this,false);
            break;
        default:
            Base::Console().Message("CMD::CVGrp - invalid iMsg: %d\n",iMsg);
    };
}

Gui::Action * CmdTechDrawExtendShortenLineGroup::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionExtendLine"));
    p1->setObjectName(QString::fromLatin1("TechDraw_ExtensionExtendLine"));
    p1->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionExtendLine"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionShortenLine"));
    p2->setObjectName(QString::fromLatin1("TechDraw_ExtensionShortenLine"));
    p2->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionShortenLine"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(p1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdTechDrawExtendShortenLineGroup::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* arc1 = a[0];
    arc1->setText(QApplication::translate("TechDraw_Extension","Extend a Line"));
    arc1->setToolTip(QApplication::translate("TechDraw_Extension",
    "Extend a line at both ends\n\
    - select one cosmetic or centerline\n\
    - click this button"));
    arc1->setStatusTip(arc1->toolTip());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("TechDraw_Extension","Shorten a Line"));
    arc2->setToolTip(QApplication::translate("TechDraw_Extension",
    "Shorten a line at both ends\n\
    - select one cosmetic or centerline\n\
    - click this button"));
    arc2->setStatusTip(arc2->toolTip());
}

bool CmdTechDrawExtendShortenLineGroup::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

//===========================================================================
// internal helper routines
//===========================================================================

bool _checkSel(Gui::Command* cmd,
               std::vector<Gui::SelectionObject>& selection,
               TechDraw::DrawViewPart*& objFeat,
               std::string message){
    // check selection of getSelectionEx() and selection[0].getObject()
    bool OK = true;
    selection = cmd->getSelection().getSelectionEx();
    if( selection.empty() ) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr(message.c_str()),
                             QObject::tr("Selection is empty"));
        OK = false;
    }
    if (OK) {
        objFeat = dynamic_cast<TechDraw::DrawViewPart *>(selection[0].getObject());
        if( objFeat == nullptr ) {
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr(message.c_str()),
                                 QObject::tr("No object selected"));
            OK = false;
        }
    }
    return OK;
}

std::vector<Base::Vector3d> _getVertexPoints(std::vector<std::string> SubNames,TechDraw::DrawViewPart* objFeat){
    // get vertex points as Vector3d
    std::vector<Base::Vector3d> vertexPoints;
    for (std::string Name : SubNames){    
        std::string GeoType = TechDraw::DrawUtil::getGeomTypeFromName(Name);
        if (GeoType == "Vertex"){
            int GeoId = TechDraw::DrawUtil::getIndexFromName(Name);
            TechDraw::VertexPtr vert = objFeat->getProjVertexByIndex(GeoId);
            Base::Vector3d onePoint(vert->point().x,vert->point().y,0);
            vertexPoints.push_back(onePoint);
        }
    }
    return vertexPoints;
}

float _getAngle(Base::Vector3d center, Base::Vector3d point){
    // get angle between x-axis and the vector from center to point
    const float Pi180 = 180.0/3.14159;
    Base::Vector3d vecCP = point-center;
    float dy = vecCP.y;
    float sign = -1.0;
    if (dy < 0.0)
        sign = -sign;
    float angle = acos(vecCP.Normalize().x)*Pi180*sign;
    if (angle < 0.0)
        angle = 360+angle;
    return angle;
}

void _intersection(TechDraw::BaseGeom* geom1, TechDraw::BaseGeom* geom2, std::vector<Base::Vector3d>& interPoints){
    // find intersection vertex(es) between two edges
    #define unknown 0
    #define isLine 1
    #define isCircle 2
    int edge1(unknown), edge2(unknown);
    if (geom1->geomType == TechDraw::CIRCLE ||
        geom1->geomType == TechDraw::ARCOFCIRCLE)
            edge1 = isCircle;
    else if (geom1->geomType == TechDraw::GENERIC)
            edge1 = isLine;
    if (geom2->geomType == TechDraw::CIRCLE ||
        geom2->geomType == TechDraw::ARCOFCIRCLE)
            edge2 = isCircle;
    else if (geom2->geomType == TechDraw::GENERIC)
            edge2 = isLine;
    if (edge1 == isLine && edge2 == isLine)
        _intersectionLL(geom1,geom2,interPoints);
    else if (edge1 == isCircle && edge2 == isLine)
        _intersectionCL(geom1,geom2,interPoints);
    else if (edge1 == isLine && edge2 == isCircle)
        _intersectionCL(geom2,geom1,interPoints);
    else if (edge1 == isCircle && edge2 == isCircle)
        _intersectionCC(geom2,geom1,interPoints);
}

void _intersectionLL(TechDraw::BaseGeom* geom1, TechDraw::BaseGeom* geom2, std::vector<Base::Vector3d>& interPoints){
    // find intersection vertex of two lines
    // Taken from: <http://de.wikipedia.org/wiki/Schnittpunkt>
    TechDraw::Generic* gen1 = static_cast<TechDraw::Generic *>(geom1);
    TechDraw::Generic* gen2 = static_cast<TechDraw::Generic *>(geom2);
    Base::Vector3d startPnt1 = gen1->points.at(0);
    Base::Vector3d endPnt1 = gen1->points.at(1);
    Base::Vector3d startPnt2 = gen2->points.at(0);
    Base::Vector3d endPnt2 = gen2->points.at(1);
    Base::Vector3d dir1 = endPnt1-startPnt1;
    Base::Vector3d dir2 = endPnt2-startPnt2;
    float a1 = -dir1.y;
    float b1 = dir1.x;
    float c1 = -startPnt1.x*dir1.y+startPnt1.y*dir1.x;
    float a2 = -dir2.y;
    float b2 = dir2.x;
    float c2 = -startPnt2.x*dir2.y+startPnt2.y*dir2.x;
    float denom = a1*b2-a2*b1;
    if (abs(denom)>=0.01){
        float xIntersect = (c1*b2-c2*b1)/denom;
        float yIntersect = (a1*c2-a2*c1)/denom;
        yIntersect = -yIntersect;
        Base::Vector3d interPoint(xIntersect,yIntersect,0.0);
        interPoints.push_back(interPoint);
    }
}

void _intersectionCL(TechDraw::BaseGeom* geom1, TechDraw::BaseGeom* geom2, std::vector<Base::Vector3d>& interPoints){
    // find intersection vertex(es) between one circle and one line
    // Taken from: <http://de.wikipedia.org/wiki/Schnittpunkt>
    TechDraw::Circle* gen1 = static_cast<TechDraw::Circle *>(geom1);
    TechDraw::Generic* gen2 = static_cast<TechDraw::Generic *>(geom2);
    Base::Vector3d cirleCenter = gen1->center;
    Base::Vector3d startPnt = gen2->points.at(0);
    Base::Vector3d endPnt = gen2->points.at(1);
    Base::Vector3d dir = endPnt-startPnt;
    float r0 = gen1->radius;
    float x0 = cirleCenter.x;
    float y0 = cirleCenter.y;
    float a = -dir.y;
    float b = dir.x;
    float c = -startPnt.x*dir.y+startPnt.y*dir.x;
    float d = c-a*x0-b*y0;
    float ab = a*a+b*b;
    float rootArg = r0*r0*ab-d*d;
    if (rootArg > 0){
        if (rootArg < 0.01){
            float x1 = x0+a*d/ab;
            float y1 = -y0+b*d/ab;
            Base::Vector3d interPoint1(x1,y1,0.0);
            interPoints.push_back(interPoint1);
        }
        else {
            float root = sqrt(rootArg);
            float x1 = x0+(a*d+b*root)/ab;
            float y1 = -y0-(b*d-a*root)/ab;
            float x2 = x0+(a*d-b*root)/ab;
            float y2 = -y0-(b*d+a*root)/ab;
            Base::Vector3d interPoint1(x1,y1,0.0);
            interPoints.push_back(interPoint1);
            Base::Vector3d interPoint2(x2,y2,0.0);
            interPoints.push_back(interPoint2);
        }
    }
}

void _intersectionCC(TechDraw::BaseGeom* geom1, TechDraw::BaseGeom* geom2, std::vector<Base::Vector3d>& interPoints){
    // find intersection vertex(es) between two circles
    // Taken from: <http://de.wikipedia.org/wiki/Schnittpunkt>
    TechDraw::Circle* gen1 = static_cast<TechDraw::Circle *>(geom1);
    TechDraw::Circle* gen2 = static_cast<TechDraw::Circle *>(geom2);
    Base::Vector3d Center1 = gen1->center;
    Base::Vector3d Center2 = gen2->center;
    float r1 = gen1->radius;
    float r2 = gen2->radius;
    float d12 = (Center2-Center1).Length();
    Base::Vector3d m = (Center2-Center1).Normalize();
    Base::Vector3d n(-m.y,m.x,0.0);
    float d0 = (r1*r1-r2*r2+d12*d12)/(2*d12);
    float rootArg = r1*r1-d0*d0;
    if (rootArg > 0){
        if (rootArg < 0.1){
            Base::Vector3d interPoint1 = -Center1+m*d0;
            interPoint1.y = -interPoint1.y;
            interPoints.push_back(interPoint1);
        }
        else {
            float e0 = sqrt(rootArg);
            Base::Vector3d interPoint1 = Center1+m*d0+n*e0;
            interPoint1.y = -interPoint1.y;
            interPoints.push_back(interPoint1);
            Base::Vector3d interPoint2 = Center1+m*d0-n*e0;
            interPoint2.y = -interPoint2.y;
            interPoints.push_back(interPoint2);
        }
    }
}

Base::Vector3d _circleCenter(Base::Vector3d p1, Base::Vector3d p2, Base::Vector3d p3){
    // Circle through 3 points, calculate center point
    // copied from ...Sketcher/Gui/CommandCreatGeo.cpp
    Base::Vector3d u = p2-p1;
    Base::Vector3d v = p3-p2;
    Base::Vector3d w = p1-p3;

    double uu =  u*u;
    double vv =  v*v;
    double ww =  w*w;

    double uv = -(u*v);
    double vw = -(v*w);
    double uw = -(u*w);

    double w0 = (2 * sqrt(uu * ww - uw * uw) * uw / (uu * ww));
    double w1 = (2 * sqrt(uu * vv - uv * uv) * uv / (uu * vv));
    double w2 = (2 * sqrt(vv * ww - vw * vw) * vw / (vv * ww));

    double wx = w0 + w1 + w2;

    if( wx == 0)
        THROWM(Base::ValueError,"Points are collinear");

    double x = (w0*p1.x + w1*p2.x + w2*p3.x)/wx;
    double y = (w0*p1.y + w1*p2.y + w2*p3.y)/wx;

    return Base::Vector3d(x, y, 0.0);
}

bool _circulation(Base::Vector3d A, Base::Vector3d B, Base::Vector3d C){
    // test the circulation of the triangle A-B-C
    if (A.x*B.y+A.y*C.x+B.x*C.y-C.x*B.y-C.y*A.x-B.x*A.y > 0.0)
        return true;
    else
        return false;
}

void _createThreadCircle(std::string Name, TechDraw::DrawViewPart* objFeat, float factor){
    // create the 3/4 arc symbolizing a thread from top seen
    double scale = objFeat->getScale();
    int GeoId = TechDraw::DrawUtil::getIndexFromName(Name);
    TechDraw::BaseGeom* geom = objFeat->getGeomByIndex(GeoId);
    std::string GeoType = TechDraw::DrawUtil::getGeomTypeFromName(Name);
    if (GeoType == "Edge"){
        if (geom->geomType == TechDraw::CIRCLE){
            TechDraw::Circle* cgen = static_cast<TechDraw::Circle *>(geom);
            Base::Vector3d center = cgen->center;
            float radius = cgen->radius;
            TechDraw::BaseGeom* threadArc = new TechDraw::AOC(center/scale, radius*factor/scale, 255.0, 165.0);
            std::string arcTag = objFeat->addCosmeticEdge(threadArc);
            TechDraw::CosmeticEdge* arc = objFeat->getCosmeticEdge(arcTag);
            _setLineAttributes(arc);
        }
    }
}

void _createThreadLines(std::vector<std::string> SubNames, TechDraw::DrawViewPart* objFeat, float factor){
    // create symbolizing lines of a thread from the side seen
    double scale = objFeat->getScale();
    std::string GeoType0 = TechDraw::DrawUtil::getGeomTypeFromName(SubNames[0]);
    std::string GeoType1 = TechDraw::DrawUtil::getGeomTypeFromName(SubNames[1]);
    if ((GeoType0 == "Edge") && (GeoType1 == "Edge")) {
        int GeoId0 = TechDraw::DrawUtil::getIndexFromName(SubNames[0]);
        int GeoId1 = TechDraw::DrawUtil::getIndexFromName(SubNames[1]);
        TechDraw::BaseGeom* geom0 = objFeat->getGeomByIndex(GeoId0);
        TechDraw::BaseGeom* geom1 = objFeat->getGeomByIndex(GeoId1);
        if ((geom0->geomType == TechDraw::GENERIC) && (geom1->geomType == TechDraw::GENERIC)) {
            TechDraw::Generic* line0 = static_cast<TechDraw::Generic *>(geom0);
            TechDraw::Generic* line1 = static_cast<TechDraw::Generic *>(geom1);
            Base::Vector3d start0 = line0->points.at(0);
            Base::Vector3d end0 = line0->points.at(1);
            Base::Vector3d start1 = line1->points.at(0);
            Base::Vector3d end1 = line1->points.at(1);
            if (_circulation(start0,end0,start1) != _circulation(end0,end1,start1)) {
                Base::Vector3d help1 = start1;
                Base::Vector3d help2 = end1;
                start1 = help2;
                end1 = help1;
            }
            start0.y = -start0.y;
            end0.y = -end0.y;
            start1.y = -start1.y;
            end1.y = -end1.y;
            float kernelDiam = (start1-start0).Length();
            float kernelFactor = (kernelDiam*factor-kernelDiam)/2;
            Base::Vector3d delta = (start1-start0).Normalize()*kernelFactor;
            std::string line0Tag = objFeat->addCosmeticEdge((start0-delta)/scale, (end0-delta)/scale);
            std::string line1Tag = objFeat->addCosmeticEdge((start1+delta)/scale, (end1+delta)/scale);
            TechDraw::CosmeticEdge* cosTag0 = objFeat->getCosmeticEdge(line0Tag);
            TechDraw::CosmeticEdge* cosTag1 = objFeat->getCosmeticEdge(line1Tag);
            _setLineAttributes(cosTag0);
            _setLineAttributes(cosTag1);
        } else {
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr("TechDraw Thread Hole Side"),
                                 QObject::tr("Please select two straight lines"));
            return;
        }
    }
}

void _setLineAttributes(TechDraw::CosmeticEdge* cosEdge) {
    // set line attributes of a cosmetic edge
    cosEdge->m_format.m_style = activeAttributes.getStyle();
    cosEdge->m_format.m_weight = activeAttributes.getWidthValue();
    cosEdge->m_format.m_color = activeAttributes.getColorValue();
}

void _setLineAttributes(TechDraw::CenterLine* cosEdge) {
    // set line attributes of a cosmetic edge
    cosEdge->m_format.m_style = activeAttributes.getStyle();
    cosEdge->m_format.m_weight = activeAttributes.getWidthValue();
    cosEdge->m_format.m_color = activeAttributes.getColorValue();
}

void _setLineAttributes(TechDraw::CosmeticEdge* cosEdge, int style, float weight, App::Color color) {
    // set line attributes of a cosmetic edge
    cosEdge->m_format.m_style = style;
    cosEdge->m_format.m_weight = weight;
    cosEdge->m_format.m_color = color;
}

//------------------------------------------------------------------------------
void CreateTechDrawCommandsExtensions(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdTechDrawExtensionSelectLineAttributes());
    rcCmdMgr.addCommand(new CmdTechDrawExtendShortenLineGroup());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionExtendLine());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionShortenLine());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionLockUnlockView());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionPositionSectionView());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionChangeLineAttributes());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionCircleCenterLinesGroup());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionCircleCenterLines());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionHoleCircle());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionVertexAtIntersection());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionDrawCirclesGroup());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionDrawCosmCircle());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionArc());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionLinePPGroup());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionLineParallel());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionLinePerpendicular());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionThreadsGroup());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionThreadHoleSide());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionThreadBoltSide());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionThreadHoleBottom());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionThreadBoltBottom());
}
