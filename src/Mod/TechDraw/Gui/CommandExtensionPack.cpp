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
#include <QApplication>
#include <QMessageBox>
#include <sstream>
#endif

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Base/Type.h>
#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/SelectionObject.h>
#include <Gui/ViewProvider.h>
#include <Mod/Part/App/Geometry2d.h>
#include <Mod/TechDraw/App/CenterLine.h>
#include <Mod/TechDraw/App/Cosmetic.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawProjGroup.h>
#include <Mod/TechDraw/App/DrawProjGroupItem.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawViewBalloon.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawViewSection.h>

#include "DrawGuiUtil.h"
#include "QGSPage.h"
#include "TaskSelectLineAttributes.h"
#include "ViewProviderBalloon.h"
#include "ViewProviderPage.h"


using namespace TechDrawGui;
using namespace TechDraw;


namespace TechDrawGui
{
//LineAttributes activeAttributes; // container holding global line attributes

//internal helper functions
lineAttributes& _getActiveLineAttributes();
Base::Vector3d _circleCenter(Base::Vector3d p1, Base::Vector3d p2, Base::Vector3d p3);
void _createThreadCircle(std::string Name, TechDraw::DrawViewPart* objFeat, float factor);
void _createThreadLines(std::vector<std::string> SubNames, TechDraw::DrawViewPart* objFeat,
                        float factor);
void _setLineAttributes(TechDraw::CosmeticEdge* cosEdge);
void _setLineAttributes(TechDraw::CenterLine* cosEdge);
void _setLineAttributes(TechDraw::CosmeticEdge* cosEdge, int style, float weight, App::Color color);
void _setLineAttributes(TechDraw::CenterLine* cosEdge, int style, float weight, App::Color color);
float _getAngle(Base::Vector3d center, Base::Vector3d point);
std::vector<Base::Vector3d> _getVertexPoints(std::vector<std::string> SubNames,
                                             TechDraw::DrawViewPart* objFeat);
bool _checkSel(Gui::Command* cmd, std::vector<Gui::SelectionObject>& selection,
               TechDraw::DrawViewPart*& objFeat, std::string message);
std::string _createBalloon(Gui::Command* cmd, TechDraw::DrawViewPart* objFeat);

//===========================================================================
// TechDraw_ExtensionHoleCircle
//===========================================================================

void execHoleCircle(Gui::Command* cmd)
{
    //create centerlines of a hole/bolt circle
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSel(cmd, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw Hole Circle")))
        return;
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    std::vector<TechDraw::CirclePtr> Circles;
    for (const std::string& Name : SubNames) {
        int GeoId = TechDraw::DrawUtil::getIndexFromName(Name);
        std::string GeoType = TechDraw::DrawUtil::getGeomTypeFromName(Name);
        TechDraw::BaseGeomPtr geom = objFeat->getGeomByIndex(GeoId);
        if (GeoType == "Edge") {
            if (geom->getGeomType() == TechDraw::CIRCLE || geom->getGeomType() == TechDraw::ARCOFCIRCLE) {
                TechDraw::CirclePtr cgen = std::static_pointer_cast<TechDraw::Circle>(geom);
                Circles.push_back(cgen);
            }
        }
    }
    if (Circles.size() <= 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("TechDraw Hole Circle"),
                             QObject::tr("Fewer than three circles selected"));
        return;
    }
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Bolt Circle Centerlines"));
    double scale = objFeat->getScale();
    Base::Vector3d bigCenter =
        _circleCenter(Circles[0]->center, Circles[1]->center, Circles[2]->center);
    float bigRadius = (Circles[0]->center - bigCenter).Length();
    TechDraw::BaseGeomPtr bigCircle =
        std::make_shared<TechDraw::Circle>(bigCenter / scale, bigRadius / scale);
    std::string bigCircleTag = objFeat->addCosmeticEdge(bigCircle);
    TechDraw::CosmeticEdge* ceCircle = objFeat->getCosmeticEdge(bigCircleTag);
    _setLineAttributes(ceCircle);
    for (const TechDraw::CirclePtr& oneCircle : Circles) {
        Base::Vector3d oneCircleCenter = oneCircle->center;
        float oneRadius = oneCircle->radius;
        Base::Vector3d delta = (oneCircle->center - bigCenter).Normalize() * (oneRadius + 2);
        Base::Vector3d startPt = oneCircleCenter + delta;
        Base::Vector3d endPt = oneCircleCenter - delta;
        startPt.y = -startPt.y;
        endPt.y = -endPt.y;
        std::string oneLineTag = objFeat->addCosmeticEdge(startPt / scale, endPt / scale);
        TechDraw::CosmeticEdge* ceLine = objFeat->getCosmeticEdge(oneLineTag);
        _setLineAttributes(ceLine);
    }
    cmd->getSelection().clearSelection();
    objFeat->refreshCEGeoms();
    objFeat->requestPaint();
    Gui::Command::commitCommand();
}
}// namespace TechDrawGui

DEF_STD_CMD_A(CmdTechDrawExtensionHoleCircle)

CmdTechDrawExtensionHoleCircle::CmdTechDrawExtensionHoleCircle()
    : Command("TechDraw_ExtensionHoleCircle")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Add Bolt Circle Centerlines");
    sToolTipText = QT_TR_NOOP("Add centerlines to a circular pattern of circles:<br>\
- Specify the line attributes (optional)<br>\
- Select three or more circles forming a circular pattern<br>\
- Click this tool");
    sWhatsThis = "TechDraw_ExtensionHoleCircle";
    sStatusTip = sMenuText;
    sPixmap = "TechDraw_ExtensionHoleCircle";
}

void CmdTechDrawExtensionHoleCircle::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execHoleCircle(this);
    //Base::Console().Message("HoleCircle started\n");
}

bool CmdTechDrawExtensionHoleCircle::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionCircleCenterLines
//===========================================================================

void execCircleCenterLines(Gui::Command* cmd)
{
    // create circle centerlines
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSel(cmd, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw Circle Centerlines")))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Circle Centerlines"));
    double scale = objFeat->getScale();
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    for (const std::string& Name : SubNames) {
        int GeoId = TechDraw::DrawUtil::getIndexFromName(Name);
        TechDraw::BaseGeomPtr geom = objFeat->getGeomByIndex(GeoId);
        std::string GeoType = TechDraw::DrawUtil::getGeomTypeFromName(Name);
        if (GeoType == "Edge") {
            if (geom->getGeomType() == TechDraw::CIRCLE || geom->getGeomType() == TechDraw::ARCOFCIRCLE) {
                TechDraw::CirclePtr cgen = std::static_pointer_cast<TechDraw::Circle>(geom);
                Base::Vector3d center = cgen->center;
                center.y = -center.y;
                float radius = cgen->radius;
                Base::Vector3d right(center.x + radius + 2.0, center.y, 0.0);
                Base::Vector3d top(center.x, center.y + radius + 2.0, 0.0);
                Base::Vector3d left(center.x - radius - 2.0, center.y, 0.0);
                Base::Vector3d bottom(center.x, center.y - radius - 2.0, 0.0);
                std::string line1tag = objFeat->addCosmeticEdge(right / scale, left / scale);
                std::string line2tag = objFeat->addCosmeticEdge(top / scale, bottom / scale);
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
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Add Circle Centerlines");
    sToolTipText = QT_TR_NOOP("Add centerlines to circles and arcs:<br>\
- Specify the line attributes (optional)<br>\
- Select one or more circles or arcs<br>\
- Click this tool");
    sWhatsThis = "TechDraw_ExtensionCircleCenterLines";
    sStatusTip = sMenuText;
    sPixmap = "TechDraw_ExtensionCircleCenterLines";
}

void CmdTechDrawExtensionCircleCenterLines::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execCircleCenterLines(this);
}

bool CmdTechDrawExtensionCircleCenterLines::isActive()
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
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Add Circle Centerlines");
    sToolTipText = QT_TR_NOOP("Add centerlines to circles and arcs:<br>\
- Specify the line attributes (optional)<br>\
- Select one or more circles or arcs<br>\
- Click this tool");
    sWhatsThis = "TechDraw_ExtensionCircleCenterLinesGroup";
    sStatusTip = sMenuText;
}

void CmdTechDrawExtensionCircleCenterLinesGroup::activated(int iMsg)
{
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
                             QObject::tr("Close active task dialog and try again."));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch (iMsg) {
        case 0://circle centerlines
            execCircleCenterLines(this);
            break;
        case 1://bolt circle centerlines
            execHoleCircle(this);
            break;
        default:
            Base::Console().Message("CMD::CVGrp - invalid iMsg: %d\n", iMsg);
    };
}

Gui::Action* CmdTechDrawExtensionCircleCenterLinesGroup::createAction()
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
    arc1->setText(
        QApplication::translate("CmdTechDrawExtensionCircleCenterLines", "Add Circle Centerlines"));
    arc1->setToolTip(QApplication::translate("CmdTechDrawExtensionCircleCenterLines",
                                             "Add centerlines to circles and arcs:<br>\
- Specify the line attributes (optional)<br>\
- Select one or more circles or arcs<br>\
- Click this tool"));
    arc1->setStatusTip(arc1->text());
    QAction* arc2 = a[1];
    arc2->setText(
        QApplication::translate("CmdTechDrawExtensionHoleCircle", "Add Bolt Circle Centerlines"));
    arc2->setToolTip(QApplication::translate("CmdTechDrawExtensionHoleCircle",
                                             "Add centerlines to a circular pattern of circles:<br>\
- Specify the line attributes (optional)<br>\
- Select three or more circles forming a circular pattern<br>\
- Click this tool"));
    arc2->setStatusTip(arc2->text());
}

bool CmdTechDrawExtensionCircleCenterLinesGroup::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionThreadHoleSide
//===========================================================================

void execThreadHoleSide(Gui::Command* cmd)
{
    // add cosmetic thread to side view of hole
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSel(cmd, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw Thread Hole Side")))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Cosmetic Thread Hole Side"));
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
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Add Cosmetic Thread Hole Side View");
    sToolTipText = QT_TR_NOOP("Add a cosmetic thread to the side view of a hole:<br>\
- Specify the line attributes (optional)<br>\
- Select two parallel lines<br>\
- Click this tool");
    sWhatsThis = "TechDraw_ExtensionThreadHoleSide";
    sStatusTip = sMenuText;
    sPixmap = "TechDraw_ExtensionThreadHoleSide";
}

void CmdTechDrawExtensionThreadHoleSide::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execThreadHoleSide(this);
}

bool CmdTechDrawExtensionThreadHoleSide::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionThreadBoltSide
//===========================================================================

void execThreadBoltSide(Gui::Command* cmd)
{
    // add cosmetic thread to side view of bolt
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSel(cmd, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw Thread Bolt Side")))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Cosmetic Thread Bolt Side"));
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
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Add Cosmetic Thread Bolt Side View");
    sToolTipText = QT_TR_NOOP("Add a cosmetic thread to the side view of a bolt/screw/rod:<br>\
- Specify the line attributes (optional)<br>\
- Select two parallel lines<br>\
- Click this tool");
    sWhatsThis = "TechDraw_ExtensionThreadBoltSide";
    sStatusTip = sMenuText;
    sPixmap = "TechDraw_ExtensionThreadBoltSide";
}

void CmdTechDrawExtensionThreadBoltSide::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execThreadBoltSide(this);
}

bool CmdTechDrawExtensionThreadBoltSide::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionThreadHoleBottom
//===========================================================================

void execThreadHoleBottom(Gui::Command* cmd)
{
    // add cosmetic thread to bottom view of hole
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSel(cmd, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw Thread Hole Bottom")))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Cosmetic Thread Hole Bottom"));
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    for (const std::string& Name : SubNames) {
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
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Add Cosmetic Thread Hole Bottom View");
    sToolTipText = QT_TR_NOOP("Add a cosmetic thread to the top or bottom view of holes:<br>\
- Specify the line attributes (optional)<br>\
- Select one or more circles<br>\
- Click this tool");
    sWhatsThis = "TechDraw_ExtensionThreadHoleBottom";
    sStatusTip = sMenuText;
    sPixmap = "TechDraw_ExtensionThreadHoleBottom";
}

void CmdTechDrawExtensionThreadHoleBottom::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execThreadHoleBottom(this);
}

bool CmdTechDrawExtensionThreadHoleBottom::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionThreadBoltBottom
//===========================================================================

void execThreadBoltBottom(Gui::Command* cmd)
{
    // add cosmetic thread to bottom view of bolt
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSel(cmd, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw Thread Bolt Bottom")))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Cosmetic Thread Bolt Bottom"));
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    for (const std::string& Name : SubNames) {
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
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Add Cosmetic Thread Bolt Bottom View");
    sToolTipText =
        QT_TR_NOOP("Add a cosmetic thread to the top or bottom view of bolts/screws/rods:<br>\
- Specify the line attributes (optional)<br>\
- Select one or more circles<br>\
- Click this tool");
    sWhatsThis = "TechDraw_ExtensionThreadBoltBottom";
    sStatusTip = sMenuText;
    sPixmap = "TechDraw_ExtensionThreadBoltBottom";
}

void CmdTechDrawExtensionThreadBoltBottom::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execThreadBoltBottom(this);
}

bool CmdTechDrawExtensionThreadBoltBottom::isActive()
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
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Add Cosmetic Thread Hole Side View");
    sToolTipText = QT_TR_NOOP("Add a cosmetic thread to the side view of a hole:<br>\
- Specify the line attributes (optional)<br>\
- Select two parallel lines<br>\
- Click this tool");
    sWhatsThis = "TechDraw_ExtensionThreadsGroup";
    sStatusTip = sMenuText;
}

void CmdTechDrawExtensionThreadsGroup::activated(int iMsg)
{
    //    Base::Console().Message("CMD::TechDrawExtensionThreadsGroup - activated(%d)\n", iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
                             QObject::tr("Close active task dialog and try again."));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch (iMsg) {
        case 0://thread hole side view
            execThreadHoleSide(this);
            break;
        case 1://thread hole bottom view
            execThreadHoleBottom(this);
            break;
        case 2://thread bolt side view
            execThreadBoltSide(this);
            break;
        case 3://thread bolt bottom view
            execThreadBoltBottom(this);
            break;
        default:
            Base::Console().Message("CMD::CVGrp - invalid iMsg: %d\n", iMsg);
    };
}

Gui::Action* CmdTechDrawExtensionThreadsGroup::createAction()
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
    arc1->setText(QApplication::translate("CmdTechDrawExtensionThreadHoleSide",
                                          "Add Cosmetic Thread Hole Side View"));
    arc1->setToolTip(QApplication::translate("CmdTechDrawExtensionThreadHoleSide",
                                             "Add a cosmetic thread to the side view of a hole:<br>\
- Specify the line attributes (optional)<br>\
- Select two parallel lines<br>\
- Click this tool"));
    arc1->setStatusTip(arc1->text());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("CmdTechDrawExtensionThreadHoleBottom",
                                          "Add Cosmetic Thread Hole Bottom View"));
    arc2->setToolTip(
        QApplication::translate("CmdTechDrawExtensionThreadHoleBottom",
                                "Add a cosmetic thread to the top or bottom view of holes:<br>\
- Specify the line attributes (optional)<br>\
- Select one or more circles<br>\
- Click this tool"));
    arc2->setStatusTip(arc2->text());
    QAction* arc3 = a[2];
    arc3->setText(QApplication::translate("CmdTechDrawExtensionThreadBoltSide",
                                          "Add Cosmetic Thread Bolt Side View"));
    arc3->setToolTip(
        QApplication::translate("CmdTechDrawExtensionThreadBoltSide",
                                "Add a cosmetic thread to the side view of a bolt/screw/rod:<br>\
- Specify the line attributes (optional)<br>\
- Select two parallel lines<br>\
- Click this tool"));
    arc3->setStatusTip(arc3->text());
    QAction* arc4 = a[3];
    arc4->setText(QApplication::translate("CmdTechDrawExtensionThreadBoltBottom",
                                          "Add Cosmetic Thread Bolt Bottom View"));
    arc4->setToolTip(QApplication::translate(
        "CmdTechDrawExtensionThreadBoltBottom",
        "Add a cosmetic thread to the top or bottom view of bolts/screws/rods:<br>\
- Specify the line attributes (optional)<br>\
- Select one or more circles<br>\
- Click this tool"));
    arc4->setStatusTip(arc4->text());
}

bool CmdTechDrawExtensionThreadsGroup::isActive()
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
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Select Line Attributes, Cascade Spacing and Delta Distance");
    sToolTipText = QT_TR_NOOP(
        "Select the attributes for new cosmetic lines and centerlines, and specify the cascade spacing and delta distance:<br>\
- Click this tool<br>\
- Specify the attributes, spacing and distance in the dialog box<br>\
- Press OK");
    sWhatsThis = "TechDraw_ExtensionSelectLineAttributes";
    sStatusTip = sMenuText;
    sPixmap = "TechDraw_ExtensionSelectLineAttributes";
}

void CmdTechDrawExtensionSelectLineAttributes::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::Control().showDialog(new TaskDlgSelectLineAttributes(&_getActiveLineAttributes()));
}

bool CmdTechDrawExtensionSelectLineAttributes::isActive()
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
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Change Line Attributes");
    sToolTipText = QT_TR_NOOP("Change the attributes of cosmetic lines and centerlines:<br>\
- Specify the line attributes (optional)<br>\
- Select one or more lines<br>\
- Click this tool");
    sWhatsThis = "TechDraw_ExtensionChangeLineAttributes";
    sStatusTip = sMenuText;
    sPixmap = "TechDraw_ExtensionChangeLineAttributes";
}

void CmdTechDrawExtensionChangeLineAttributes::activated(int iMsg)
{
    // change attributes (type, width, color) of a cosmetic or centerline
    Q_UNUSED(iMsg);
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSel(this, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw Change Line Attributes")))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Change Line Attributes"));
    const std::vector<std::string> subNames = selection[0].getSubNames();
    for (const std::string& name : subNames) {
        int num = DrawUtil::getIndexFromName(name);
        BaseGeomPtr baseGeo = objFeat->getGeomByIndex(num);
        if (baseGeo) {
            if (baseGeo->getCosmetic()) {
                if (baseGeo->source() == 1) {
                    TechDraw::CosmeticEdge* cosEdgeTag = objFeat->getCosmeticEdgeBySelection(name);
                    _setLineAttributes(cosEdgeTag);
                }
                else if (baseGeo->source() == 2) {
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

bool CmdTechDrawExtensionChangeLineAttributes::isActive()
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
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Add Cosmetic Intersection Vertex(es)");
    sToolTipText =
        QT_TR_NOOP("Add cosmetic vertex(es) at the intersection(s) of selected edges:<br>\
- Select two edges (lines, circles and/or arcs)<br>\
- Click this tool");
    sWhatsThis = "TechDraw_ExtensionVertexAtIntersection";
    sStatusTip = sMenuText;
    sPixmap = "TechDraw_ExtensionVertexAtIntersection";
}

void CmdTechDrawExtensionVertexAtIntersection::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    //Base::Console().Message("VertexAtIntersection started\n");
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSel(this, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw Cosmetic Intersection Vertex(es)")))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Cosmetic Intersection Vertex(es)"));
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    std::vector<Base::Vector3d> interPoints;
    if (SubNames.size() >= 2) {
        std::string GeoType1 = TechDraw::DrawUtil::getGeomTypeFromName(SubNames[0]);
        std::string GeoType2 = TechDraw::DrawUtil::getGeomTypeFromName(SubNames[1]);
        if (GeoType1 == "Edge" && GeoType2 == "Edge") {
            int GeoId1 = TechDraw::DrawUtil::getIndexFromName(SubNames[0]);
            TechDraw::BaseGeomPtr geom1 = objFeat->getGeomByIndex(GeoId1);
            int GeoId2 = TechDraw::DrawUtil::getIndexFromName(SubNames[1]);
            TechDraw::BaseGeomPtr geom2 = objFeat->getGeomByIndex(GeoId2);
            interPoints = geom1->intersection(geom2);
            if (!interPoints.empty()) {
                double scale = objFeat->getScale();
                std::string id1 = objFeat->addCosmeticVertex(interPoints[0] / scale);
                objFeat->add1CVToGV(id1);
                if (interPoints.size() >= 2) {
                    std::string id2 = objFeat->addCosmeticVertex(interPoints[1] / scale);
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

bool CmdTechDrawExtensionVertexAtIntersection::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionDrawCosmArc
//===========================================================================

void execDrawCosmArc(Gui::Command* cmd)
{
    //draw a cosmetic arc of circle
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSel(cmd, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw Cosmetic Arc")))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Cosmetic Arc"));
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    std::vector<Base::Vector3d> vertexPoints;
    vertexPoints = _getVertexPoints(SubNames, objFeat);
    if (vertexPoints.size() >= 3) {
        double scale = objFeat->getScale();
        float arcRadius = (vertexPoints[1] - vertexPoints[0]).Length();
        float angle1 = _getAngle(vertexPoints[0], vertexPoints[1]);
        float angle2 = _getAngle(vertexPoints[0], vertexPoints[2]);
        TechDraw::BaseGeomPtr baseGeo = std::make_shared<TechDraw::AOC>(
            vertexPoints[0] / scale, arcRadius / scale, -angle2, -angle1);
        std::string arcTag = objFeat->addCosmeticEdge(baseGeo);
        TechDraw::CosmeticEdge* arcEdge = objFeat->getCosmeticEdge(arcTag);
        _setLineAttributes(arcEdge);
        objFeat->refreshCEGeoms();
        objFeat->requestPaint();
        cmd->getSelection().clearSelection();
        Gui::Command::commitCommand();
    }
}

DEF_STD_CMD_A(CmdTechDrawExtensionDrawCosmArc)

CmdTechDrawExtensionDrawCosmArc::CmdTechDrawExtensionDrawCosmArc()
    : Command("TechDraw_ExtensionDrawCosmArc")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Add Cosmetic Arc");
    sToolTipText = QT_TR_NOOP("Add a cosmetic counter clockwise arc based on three vertexes:<br>\
- Specify the line attributes (optional)<br>\
- Select vertex 1 (center point)<br>\
- Select vertex 2 (radius and start angle)<br>\
- Select vertex 3 (end angle)<br>\
- Click this tool");
    sWhatsThis = "TechDraw_ExtensionDrawCosmArc";
    sStatusTip = sMenuText;
    sPixmap = "TechDraw_ExtensionDrawCosmArc";
}

void CmdTechDrawExtensionDrawCosmArc::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    //Base::Console().Message("Cosmetic Arc started\n");
    execDrawCosmArc(this);
}

bool CmdTechDrawExtensionDrawCosmArc::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionDrawCosmCircle
//===========================================================================

void execDrawCosmCircle(Gui::Command* cmd)
{
    //draw a cosmetic circle
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSel(cmd, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw Cosmetic Circle")))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Cosmetic Circle"));
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    std::vector<Base::Vector3d> vertexPoints;
    vertexPoints = _getVertexPoints(SubNames, objFeat);
    if (vertexPoints.size() >= 2) {
        double scale = objFeat->getScale();
        float circleRadius = (vertexPoints[1] - vertexPoints[0]).Length();
        TechDraw::BaseGeomPtr baseGeo =
            std::make_shared<TechDraw::Circle>(vertexPoints[0] / scale, circleRadius / scale);
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
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Add Cosmetic Circle");
    sToolTipText = QT_TR_NOOP("Add a cosmetic circle based on two vertexes:<br>\
- Specify the line attributes (optional)<br>\
- Select vertex 1 (center point)<br>\
- Select vertex 2 (radius)<br>\
- Click this tool");
    sWhatsThis = "TechDraw_ExtensionDrawCosmCircle";
    sStatusTip = sMenuText;
    sPixmap = "TechDraw_ExtensionDrawCosmCircle";
}

void CmdTechDrawExtensionDrawCosmCircle::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    //Base::Console().Message("Cosmetic Circle started\n");
    execDrawCosmCircle(this);
}

bool CmdTechDrawExtensionDrawCosmCircle::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionDrawCosmCircle3Points
//===========================================================================

void execDrawCosmCircle3Points(Gui::Command* cmd)
{
    //draw a cosmetic circle through 3 points
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSel(cmd, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw Cosmetic Circle 3 Points")))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Cosmetic Circle 3 Points"));
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    std::vector<Base::Vector3d> vertexPoints;
    vertexPoints = _getVertexPoints(SubNames, objFeat);
    if (vertexPoints.size() >= 3) {
        double scale = objFeat->getScale();
        Base::Vector3d circleCenter =
            _circleCenter(vertexPoints[0], vertexPoints[1], vertexPoints[2]);
        float circleRadius = (vertexPoints[0] - circleCenter).Length();
        TechDraw::BaseGeomPtr theCircle =
            std::make_shared<TechDraw::Circle>(circleCenter / scale, circleRadius / scale);
        std::string cicleTag = objFeat->addCosmeticEdge(theCircle);
        TechDraw::CosmeticEdge* circleEdge = objFeat->getCosmeticEdge(cicleTag);
        _setLineAttributes(circleEdge);
        objFeat->refreshCEGeoms();
        objFeat->requestPaint();
        cmd->getSelection().clearSelection();
        Gui::Command::commitCommand();
    }
}

DEF_STD_CMD_A(CmdTechDrawExtensionDrawCosmCircle3Points)

CmdTechDrawExtensionDrawCosmCircle3Points::CmdTechDrawExtensionDrawCosmCircle3Points()
    : Command("TechDraw_ExtensionDrawCosmCircle3Points")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Add Cosmetic Circle 3 Points");
    sToolTipText = QT_TR_NOOP("Add a cosmetic circle based on three vertexes:<br>\
- Specify the line attributes (optional)<br>\
- Select 3 vertexes<br>\
- Click this tool");
    sWhatsThis = "TechDraw_ExtensionDrawCosmCircle3Points";
    sStatusTip = sMenuText;
    sPixmap = "TechDraw_ExtensionDrawCosmCircle3Points";
}

void CmdTechDrawExtensionDrawCosmCircle3Points::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    //Base::Console().Message("Cosmetic Circle 3 Points started\n");
    execDrawCosmCircle3Points(this);
}

bool CmdTechDrawExtensionDrawCosmCircle3Points::isActive()
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
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Add Cosmetic Circle");
    sToolTipText = QT_TR_NOOP("Add a cosmetic circle based on two vertexes:<br>\
- Specify the line attributes (optional)<br>\
- Select vertex 1 (center point)<br>\
- Select vertex 2 (radius)<br>\
- Click this tool");
    sWhatsThis = "TechDraw_ExtensionDrawCirclesGroup";
    sStatusTip = sMenuText;
}

void CmdTechDrawExtensionDrawCirclesGroup::activated(int iMsg)
{
    //    Base::Console().Message("CMD::ExtensionDrawCirclesGroup - activated(%d)\n", iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
                             QObject::tr("Close active task dialog and try again."));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch (iMsg) {
        case 0://draw cosmetic circle
            execDrawCosmCircle(this);
            break;
        case 1://draw cosmetic arc
            execDrawCosmArc(this);
            break;
        case 2://draw cosmetic circle 3 points
            execDrawCosmCircle3Points(this);
            break;
        default:
            Base::Console().Message("CMD::CVGrp - invalid iMsg: %d\n", iMsg);
    };
}

Gui::Action* CmdTechDrawExtensionDrawCirclesGroup::createAction()
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionDrawCosmCircle"));
    p1->setObjectName(QString::fromLatin1("TechDraw_ExtensionDrawCosmCircle"));
    p1->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionDrawCosmCircle"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionDrawCosmArc"));
    p2->setObjectName(QString::fromLatin1("TechDraw_ExtensionDrawCosmArc"));
    p2->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionDrawCosmArc"));
    QAction* p3 = pcAction->addAction(QString());
    p3->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_ExtensionDrawCosmCircle3Points"));
    p3->setObjectName(QString::fromLatin1("TechDraw_ExtensionDrawCosmCircle3Points"));
    p3->setWhatsThis(QString::fromLatin1("TechDraw_ExtensionDrawCosmCircle3Points"));

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
    arc1->setText(
        QApplication::translate("CmdTechDrawExtensionDrawCosmCircle", "Add Cosmetic Circle"));
    arc1->setToolTip(QApplication::translate("CmdTechDrawExtensionDrawCosmCircle",
                                             "Add a cosmetic circle based on two vertexes:<br>\
- Specify the line attributes (optional)<br>\
- Select vertex 1 (center point)<br>\
- Select vertex 2 (radius)<br>\
- Click this tool"));
    arc1->setStatusTip(arc1->text());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("CmdTechDrawExtensionDrawCosmArc", "Add Cosmetic Arc"));
    arc2->setToolTip(
        QApplication::translate("CmdTechDrawExtensionDrawCosmArc",
                                "Add a cosmetic counter clockwise arc based on three vertexes:<br>\
- Specify the line attributes (optional)<br>\
- Select vertex 1 (center point)<br>\
- Select vertex 2 (radius and start angle)<br>\
- Select vertex 3 (end angle)<br>\
- Click this tool"));
    arc2->setStatusTip(arc2->text());
    QAction* arc3 = a[2];
    arc3->setText(QApplication::translate("CmdTechDrawExtensionDrawCosmCircle3Points",
                                          "Add Cosmetic Circle 3 Points"));
    arc3->setToolTip(QApplication::translate("CmdTechDrawExtensionDrawCosmCircle3Points",
                                             "Add a cosmetic circle based on three vertexes:<br>\
- Specify the line attributes (optional)<br>\
- Select three vertexes<br>\
- Click this tool"));
    arc3->setStatusTip(arc3->text());
}

bool CmdTechDrawExtensionDrawCirclesGroup::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionLineParallel
//===========================================================================

void execLineParallelPerpendicular(Gui::Command* cmd, bool isParallel)
{
    // create a line parallel or perpendicular to another line
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSel(cmd, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw Cosmetic Line Parallel/Perpendicular")))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Cosmetic Line Parallel/Perpendicular"));
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    if (SubNames.size() >= 2) {
        std::string GeoType1 = TechDraw::DrawUtil::getGeomTypeFromName(SubNames[0]);
        std::string GeoType2 = TechDraw::DrawUtil::getGeomTypeFromName(SubNames[1]);
        if (GeoType1 == "Edge" && GeoType2 == "Vertex") {
            double scale = objFeat->getScale();
            int GeoId1 = TechDraw::DrawUtil::getIndexFromName(SubNames[0]);
            TechDraw::BaseGeomPtr geom1 = objFeat->getGeomByIndex(GeoId1);
            int GeoId2 = TechDraw::DrawUtil::getIndexFromName(SubNames[1]);
            TechDraw::GenericPtr lineGen = std::static_pointer_cast<TechDraw::Generic>(geom1);
            Base::Vector3d lineStart = lineGen->points.at(0);
            Base::Vector3d lineEnd = lineGen->points.at(1);
            TechDraw::VertexPtr vert = objFeat->getProjVertexByIndex(GeoId2);
            Base::Vector3d vertexPoint(vert->point().x, vert->point().y, 0.0);
            Base::Vector3d halfVector = (lineEnd - lineStart) / 2.0;
            if (!isParallel) {
                float dummy = halfVector.x;
                halfVector.x = -halfVector.y;
                halfVector.y = dummy;
            }
            Base::Vector3d startPoint = vertexPoint + halfVector;
            Base::Vector3d endPoint = vertexPoint - halfVector;
            startPoint.y = -startPoint.y;
            endPoint.y = -endPoint.y;
            std::string lineTag = objFeat->addCosmeticEdge(startPoint / scale, endPoint / scale);
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
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Add Cosmetic Parallel Line");
    sToolTipText = QT_TR_NOOP("Add a cosmetic line parallel to another line through a vertex:<br>\
- Select a line<br>\
- Select a vertex<br>\
- Click this tool");
    sWhatsThis = "TechDraw_ExtensionLineParallel";
    sStatusTip = sMenuText;
    sPixmap = "TechDraw_ExtensionLineParallel";
}

void CmdTechDrawExtensionLineParallel::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execLineParallelPerpendicular(this, true);
}

bool CmdTechDrawExtensionLineParallel::isActive()
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
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Add Cosmetic Perpendicular Line");
    sToolTipText =
        QT_TR_NOOP("Add a cosmetic line perpendicular to another line through a vertex:<br>\
- Select a line<br>\
- Select a vertex<br>\
- Click this tool");
    sWhatsThis = "TechDraw_ExtensionLinePerpendicular";
    sStatusTip = sMenuText;
    sPixmap = "TechDraw_ExtensionLinePerpendicular";
}

void CmdTechDrawExtensionLinePerpendicular::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execLineParallelPerpendicular(this, false);
}

bool CmdTechDrawExtensionLinePerpendicular::isActive()
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
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Add Cosmetic Parallel Line");
    sToolTipText = QT_TR_NOOP("Add a cosmetic line parallel to another line through a vertex:<br>\
- Select a line<br>\
- Select a vertex<br>\
- Click this tool");
    sWhatsThis = "TechDraw_ExtensionLinePPGroup";
    sStatusTip = sMenuText;
}

void CmdTechDrawExtensionLinePPGroup::activated(int iMsg)
{
    //    Base::Console().Message("CMD::ExtensionLinePPGroup - activated(%d)\n", iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
                             QObject::tr("Close active task dialog and try again."));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch (iMsg) {
        case 0://create parallel line
            execLineParallelPerpendicular(this, true);
            break;
        case 1://create perpendicular line
            execLineParallelPerpendicular(this, false);
            break;
        default:
            Base::Console().Message("CMD::CVGrp - invalid iMsg: %d\n", iMsg);
    };
}

Gui::Action* CmdTechDrawExtensionLinePPGroup::createAction()
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
    arc1->setText(
        QApplication::translate("CmdTechDrawExtensionLineParallel", "Add Cosmetic Parallel Line"));
    arc1->setToolTip(
        QApplication::translate("CmdTechDrawExtensionLineParallel",
                                "Add a cosmetic line parallel to another line through a vertex:<br>\
- Select a line<br>\
- Select a vertex<br>\
- Click this tool"));
    arc1->setStatusTip(arc1->text());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("CmdTechDrawExtensionLinePerpendicular",
                                          "Add Cosmetic Perpendicular Line"));
    arc2->setToolTip(QApplication::translate(
        "CmdTechDrawExtensionLinePerpendicular",
        "Add a cosmetic line perpendicular to another line through a vertex:<br>\
- Select a line<br>\
- Select a vertex<br>\
- Click this tool"));
    arc2->setStatusTip(arc2->text());
}

bool CmdTechDrawExtensionLinePPGroup::isActive()
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
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Lock/Unlock View");
    sToolTipText = QT_TR_NOOP("Lock or unlock the position of a view:<br>\
- Select a single view<br>\
- Click this tool");
    sWhatsThis = "TechDraw_ExtensionLockUnlockView";
    sStatusTip = sMenuText;
    sPixmap = "TechDraw_ExtensionLockUnlockView";
}

void CmdTechDrawExtensionLockUnlockView::activated(int iMsg)
{
    // lock/unlock a selected view
    Q_UNUSED(iMsg);
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSel(this, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw Lock/Unlock View")))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Lock/Unlock View"));
    if (objFeat->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId())) {
        bool lockPosition = objFeat->LockPosition.getValue();
        lockPosition = !lockPosition;
        objFeat->LockPosition.setValue(lockPosition);
    }
    Gui::Command::commitCommand();
}

bool CmdTechDrawExtensionLockUnlockView::isActive()
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
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Position Section View");
    sToolTipText = QT_TR_NOOP("Orthogonally align a section view with its source view:<br>\
- Select a single section view<br>\
- Click this tool");
    sWhatsThis = "TechDraw_ExtensionPositionSectionView";
    sStatusTip = sMenuText;
    sPixmap = "TechDraw_ExtensionPositionSectionView";
}

void CmdTechDrawExtensionPositionSectionView::activated(int iMsg)
{
    // position a section view
    Q_UNUSED(iMsg);
    //Base::Console().Message("PositionSectionView started\n");
    auto selection = getSelection().getSelectionEx();
    if (selection.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("TechDraw Position Section View"),
                             QObject::tr("Selection is empty"));
        return;
    }

    double xPos = 0.0, yPos = 0.0;
    TechDraw::DrawViewPart* baseView;
    auto objFeat = selection[0].getObject();
    if (objFeat && objFeat->isDerivedFrom(TechDraw::DrawViewSection::getClassTypeId())) {
        TechDraw::DrawViewSection* sectionView = static_cast<TechDraw::DrawViewSection*>(objFeat);
        baseView = sectionView->getBaseDVP();
        if (baseView && baseView->isDerivedFrom(TechDraw::DrawProjGroupItem::getClassTypeId())) {
            std::vector<App::DocumentObject*> parentViews = baseView->getInList();
            if (!parentViews.empty()) {
                TechDraw::DrawProjGroup* groupBase =
                    dynamic_cast<TechDraw::DrawProjGroup*>(parentViews[0]);
                if (groupBase) {
                    xPos = groupBase->X.getValue();
                    yPos = groupBase->Y.getValue();
                }
            }
        }
        else if (baseView) {
            xPos = baseView->X.getValue();
            yPos = baseView->Y.getValue();
        }
        std::string direction = sectionView->SectionDirection.getValueAsString();
        if ((direction == "Right") || (direction == "Left"))
            sectionView->Y.setValue(yPos);
        else if ((direction == "Up") || (direction == "Down"))
            sectionView->X.setValue(xPos);
        else if (direction == "Aligned")
        {
            Base::Vector3d pBase(xPos,yPos,0.0);
            Base::Vector3d dirView(sectionView->Direction.getValue());
            Base::Vector3d pSection(sectionView->X.getValue(),sectionView->Y.getValue(),0.0);
            Base::Vector3d newPos = DrawUtil::getTrianglePoint(pBase, dirView, pSection);
            sectionView->X.setValue(newPos.x);
            sectionView->Y.setValue(newPos.y);
        }
    }
}

bool CmdTechDrawExtensionPositionSectionView::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionExtendLine
//===========================================================================

void execExtendShortenLine(Gui::Command* cmd, bool extend)
{
    // extend or shorten a cosmetic line or a centerline
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSel(cmd, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw Extend/Shorten Line")))
        return;
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Extend/Shorten Line"));
    const std::vector<std::string> subNames = selection[0].getSubNames();
    if (!subNames.empty()) {
        std::string name = subNames[0];
        int num = DrawUtil::getIndexFromName(name);
        std::string geoType = TechDraw::DrawUtil::getGeomTypeFromName(name);
        if (geoType == "Edge") {
            TechDraw::BaseGeomPtr baseGeo = objFeat->getGeomByIndex(num);
            if (baseGeo) {
                if (baseGeo->getGeomType() == TechDraw::GENERIC) {
                    TechDraw::GenericPtr genLine =
                        std::static_pointer_cast<TechDraw::Generic>(baseGeo);
                    Base::Vector3d P0 = genLine->points.at(0);
                    Base::Vector3d P1 = genLine->points.at(1);
                    bool isCenterLine = false;
                    TechDraw::CenterLine* centerEdge = nullptr;
                    if (baseGeo->getCosmetic()) {
                        std::string uniTag = baseGeo->getCosmeticTag();
                        int oldStyle = 1;
                        float oldWeight = 1.0f;
                        App::Color oldColor;
                        std::vector<std::string> toDelete;
                        toDelete.push_back(uniTag);
                        if (baseGeo->source() == 1) {
                            auto cosEdge = objFeat->getCosmeticEdge(uniTag);
                            oldStyle = cosEdge->m_format.m_style;
                            oldWeight = cosEdge->m_format.m_weight;
                            oldColor = cosEdge->m_format.m_color;
                            objFeat->removeCosmeticEdge(toDelete);
                        }
                        else if (baseGeo->source() == 2) {
                            isCenterLine = true;
                            centerEdge = objFeat->getCenterLine(uniTag);
                        }
                        double scale = objFeat->getScale();
                        Base::Vector3d direction = (P1 - P0).Normalize();
                        Base::Vector3d delta = direction * activeDimAttributes.getLineStretch();
                        Base::Vector3d startPt, endPt;
                        if (extend) {
                            startPt = P0 - delta;
                            endPt = P1 + delta;
                        }
                        else {
                            startPt = P0 + delta;
                            endPt = P1 - delta;
                        }
                        startPt.y = -startPt.y;
                        endPt.y = -endPt.y;
                        if (isCenterLine) {
                            centerEdge->m_extendBy += activeDimAttributes.getLineStretch();
                            objFeat->refreshCLGeoms();
                        }
                        else {
                            std::string lineTag =
                                objFeat->addCosmeticEdge(startPt / scale, endPt / scale);
                            TechDraw::CosmeticEdge* lineEdge = objFeat->getCosmeticEdge(lineTag);
                            _setLineAttributes(lineEdge, oldStyle, oldWeight, oldColor);
                            objFeat->refreshCEGeoms();
                        }
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
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Extend Line");
    sToolTipText = QT_TR_NOOP("Extend a cosmetic line or centerline at both ends:<br>\
- Specify the delta distance (optional)<br>\
- Select a single line<br>\
- Click this tool");
    sWhatsThis = "TechDraw_ExtensionExtendLine";
    sStatusTip = sMenuText;
    sPixmap = "TechDraw_ExtensionExtendLine";
}

void CmdTechDrawExtensionExtendLine::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execExtendShortenLine(this, true);
    ///Base::Console().Message("ExtendLine started\n");
}

bool CmdTechDrawExtensionExtendLine::isActive()
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
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Shorten Line");
    sToolTipText = QT_TR_NOOP("Shorten a cosmetic line or centerline at both ends:<br>\
- Specify the delta distance (optional)<br>\
- Select a single line<br>\
- Click this tool");
    sWhatsThis = "TechDraw_ExtensionShortenLine";
    sStatusTip = sMenuText;
    sPixmap = "TechDraw_ExtensionShortenLine";
}

void CmdTechDrawExtensionShortenLine::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    execExtendShortenLine(this, false);
    ///Base::Console().Message("ShortenLine started\n");
}

bool CmdTechDrawExtensionShortenLine::isActive()
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
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Extend Line");
    sToolTipText = QT_TR_NOOP("Extend a cosmetic line or centerline at both ends:<br>\
- Specify the delta distance (optional)<br>\
- Select a single line<br>\
- Click this tool");
    sWhatsThis = "TechDraw_ExtensionExtendShortenLineGroup";
    sStatusTip = sMenuText;
}

void CmdTechDrawExtendShortenLineGroup::activated(int iMsg)
{
    // Base::Console().Message("CMD::ExtendShortenLineGroup - activated(%d)\n", iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Task In Progress"),
                             QObject::tr("Close active task dialog and try again."));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch (iMsg) {
        case 0://extend a line
            execExtendShortenLine(this, true);
            break;
        case 1://shorten line
            execExtendShortenLine(this, false);
            break;
        default:
            Base::Console().Message("CMD::CVGrp - invalid iMsg: %d\n", iMsg);
    };
}

Gui::Action* CmdTechDrawExtendShortenLineGroup::createAction()
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
    arc1->setText(QApplication::translate("CmdTechDrawExtensionExtendLine", "Extend Line"));
    arc1->setToolTip(QApplication::translate(
        "CmdTechDrawExtensionExtendLine", "Extend a cosmetic line or centerline at both ends:<br>\
- Specify the delta distance (optional)<br>\
- Select a single line<br>\
- Click this tool"));
    arc1->setStatusTip(arc1->text());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("CmdTechDrawExtensionShortenLine", "Shorten Line"));
    arc2->setToolTip(QApplication::translate(
        "CmdTechDrawExtensionShortenLine", "Shorten a cosmetic line or centerline at both ends:<br>\
- Specify the delta distance (optional)<br>\
- Select a single line<br>\
- Click this tool"));
    arc2->setStatusTip(arc2->text());
}

bool CmdTechDrawExtendShortenLineGroup::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, true);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_ExtensionAreaAnnotation
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawExtensionAreaAnnotation)

CmdTechDrawExtensionAreaAnnotation::CmdTechDrawExtensionAreaAnnotation()
    : Command("TechDraw_ExtensionAreaAnnotation")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Calculate the area of selected faces");
    sToolTipText = QT_TR_NOOP("Select several faces<br>\
    - click this tool");
    sWhatsThis = "TechDraw_ExtensionAreaAnnotation";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_ExtensionAreaAnnotation";
}

void CmdTechDrawExtensionAreaAnnotation::activated(int iMsg)
// calculate the area of selected faces, create output in a balloon
{
    Q_UNUSED(iMsg);
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat;
    if (!_checkSel(this, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw calculate selected area")))
        return;
    double faceArea(0.0), totalArea(0.0), xCenter(0.0), yCenter(0.0);
    int totalPoints(0);

    // we must have at least 1 face in the selection
    const std::vector<std::string> subNamesAll = selection[0].getSubNames();
    std::vector<std::string> subNames;
    for (auto& name : subNamesAll) {
        std::string geomType = DrawUtil::getGeomTypeFromName(name);
        if (geomType == "Face") {
            subNames.push_back(name);
        }
    }

    if (subNames.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect selection"),
                             QObject::tr("No faces in selection."));
        return;
    }

    // we have at least 1 face
    for (const std::string& name : subNames) {
        int idx = TechDraw::DrawUtil::getIndexFromName(name);
        std::vector<TechDraw::BaseGeomPtr> faceEdges = objFeat->getFaceEdgesByIndex(idx);
        // We filter arcs, circles etc. which are not allowed.
        for (const TechDraw::BaseGeomPtr& geoPtr : faceEdges)
            if (geoPtr->getGeomType() != TechDraw::GENERIC)
                throw Base::TypeError(
                    "CmdTechDrawAreaAnnotation - forbidden border element found\n");
        // We create a list of all points along the boundary of the face.
        // The edges form a closed polygon, but their start- and endpoints may be interchanged.
        std::vector<Base::Vector3d> facePoints;
        TechDraw::GenericPtr firstEdge =
            std::static_pointer_cast<TechDraw::Generic>(faceEdges[0]);
        facePoints.push_back(firstEdge->points.at(0));
        facePoints.push_back(firstEdge->points.at(1));
        for (long unsigned int n = 1; n < faceEdges.size() - 1; n++) {
            TechDraw::GenericPtr nextEdge =
                std::static_pointer_cast<TechDraw::Generic>(faceEdges[n]);
            if ((nextEdge->points.at(0) - facePoints.back()).Length() < 0.01)
                facePoints.push_back(nextEdge->points.at(1));
            else
                facePoints.push_back(nextEdge->points.at(0));
        }
        facePoints.push_back(facePoints.front());
        // We calculate the area, using triangles. Each having one point at (0/0).
        faceArea = 0.0;
        xCenter = xCenter + facePoints[0].x;
        yCenter = yCenter + facePoints[0].y;
        for (long unsigned int n = 0; n < facePoints.size() - 1; n++) {
            faceArea = faceArea + facePoints[n].x * facePoints[n + 1].y
                - facePoints[n].y * facePoints[n + 1].x;
            xCenter = xCenter + facePoints[n + 1].x;
            yCenter = yCenter + facePoints[n + 1].y;
        }
        faceArea = abs(faceArea) / 2.0;
        totalArea = totalArea + faceArea;
        totalPoints = totalPoints + facePoints.size();
    }

    // if area calculation was successful, start the command
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Calculate Face Area"));
    // at first we create the balloon
    std::string balloonName = _createBalloon(this, objFeat);
    TechDraw::DrawViewBalloon* balloon = nullptr;
    balloon = dynamic_cast<TechDraw::DrawViewBalloon*>(
        this->getDocument()->getObject(balloonName.c_str()));
    if (!balloon)
        throw Base::TypeError("CmdTechDrawNewBalloon - balloon not found\n");
    // the balloon has been created successfully
    // calculate needed variables
    double scale = objFeat->getScale();
    double scale2 = scale * scale;
    totalArea = totalArea / scale2;//convert from view scale to internal mm2

    //make area unit-aware
    Base::Quantity asQuantity;
    asQuantity.setValue(totalArea);
    asQuantity.setUnit(Base::Unit::Area);
    QString qUserString = asQuantity.getUserString();
    std::string sUserString = Base::Tools::toStdString(qUserString);

    if (totalPoints != 0 && scale != 0.0) {
        xCenter = (xCenter / totalPoints) / scale;
        yCenter = (yCenter / totalPoints) / scale;
    }

    // set the attributes in the data tab's fields
    //    balloon->SourceView.setValue(objFeat);
    balloon->BubbleShape.setValue("Rectangle");
    balloon->EndType.setValue("None");
    balloon->KinkLength.setValue(0.0);
    balloon->X.setValue(xCenter);
    balloon->Y.setValue(-yCenter);
    balloon->OriginX.setValue(xCenter);
    balloon->OriginY.setValue(-yCenter);
    balloon->ScaleType.setValue("Page");
    balloon->Text.setValue(sUserString);
    // look for the ballons's view provider
    TechDraw::DrawPage* page = objFeat->findParentPage();
    Gui::Document* guiDoc = Gui::Application::Instance->getDocument(page->getDocument());
    auto viewProvider = static_cast<ViewProviderBalloon*>(guiDoc->getViewProvider(balloon));
    if (viewProvider) {
        // view provider successfully found,
        // set the attributes in the view tab's fields
        viewProvider->Fontsize.setValue(2.0);
        viewProvider->LineWidth.setValue(0.75);
        viewProvider->LineVisible.setValue(false);
        viewProvider->Color.setValue(App::Color(1.0f, 0.0f, 0.0f));
    }
    Gui::Command::commitCommand();
    objFeat->touch(true);
    Gui::Command::updateActive();
}

bool CmdTechDrawExtensionAreaAnnotation::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// internal helper routines
//===========================================================================
namespace TechDrawGui
{

lineAttributes& _getActiveLineAttributes()
{
    static lineAttributes attributes;
    return attributes;
}

std::string _createBalloon(Gui::Command* cmd, TechDraw::DrawViewPart* objFeat)
// create a new balloon, return it's name as string
{
    std::string featName;
    TechDraw::DrawPage* page = objFeat->findParentPage();
    Gui::Document* guiDoc = Gui::Application::Instance->getDocument(page->getDocument());
    ViewProviderPage* pageVP = dynamic_cast<ViewProviderPage*>(guiDoc->getViewProvider(page));
    if (pageVP) {
        QGSPage* scenePage = pageVP->getQGSPage();
        featName = scenePage->getDrawPage()->getDocument()->getUniqueObjectName("Balloon");
        std::string pageName = scenePage->getDrawPage()->getNameInDocument();
        cmd->doCommand(cmd->Doc,
                       "App.activeDocument().addObject('TechDraw::DrawViewBalloon', '%s')",
                       featName.c_str());
        cmd->doCommand(cmd->Doc, "App.activeDocument().%s.SourceView = (App.activeDocument().%s)",
                       featName.c_str(), objFeat->getNameInDocument());

        cmd->doCommand(cmd->Doc, "App.activeDocument().%s.addView(App.activeDocument().%s)",
                       pageName.c_str(), featName.c_str());
    }
    return featName;
}

bool _checkSel(Gui::Command* cmd, std::vector<Gui::SelectionObject>& selection,
               TechDraw::DrawViewPart*& objFeat, std::string message)
{
    // check selection of getSelectionEx() and selection[0].getObject()
    selection = cmd->getSelection().getSelectionEx();
    if (selection.empty()) {
        // message is translated in caller
        QMessageBox::warning(Gui::getMainWindow(), QString::fromUtf8(message.c_str()),
                             QObject::tr("Selection is empty"));
        return false;
    }

    objFeat = dynamic_cast<TechDraw::DrawViewPart*>(selection[0].getObject());
    if (!objFeat) {
        QMessageBox::warning(Gui::getMainWindow(), QString::fromUtf8(message.c_str()),
                             QObject::tr("No object selected"));
        return false;
    }

    return true;
}

std::vector<Base::Vector3d> _getVertexPoints(std::vector<std::string> SubNames,
                                             TechDraw::DrawViewPart* objFeat)
{
    // get vertex points as Vector3d
    std::vector<Base::Vector3d> vertexPoints;
    for (const std::string& Name : SubNames) {
        std::string GeoType = TechDraw::DrawUtil::getGeomTypeFromName(Name);
        if (GeoType == "Vertex") {
            int GeoId = TechDraw::DrawUtil::getIndexFromName(Name);
            TechDraw::VertexPtr vert = objFeat->getProjVertexByIndex(GeoId);
            Base::Vector3d onePoint(vert->point().x, vert->point().y, 0);
            vertexPoints.push_back(onePoint);
        }
    }
    return vertexPoints;
}

float _getAngle(Base::Vector3d center, Base::Vector3d point)
{
    // get angle between x-axis and the vector from center to point
    const auto Pi180 = 180.0 / M_PI;
    Base::Vector3d vecCP = point - center;
    float dy = vecCP.y;
    float sign = -1.0;
    if (dy < 0.0)
        sign = -sign;
    float angle = acos(vecCP.Normalize().x) * Pi180 * sign;
    if (angle < 0.0)
        angle = 360 + angle;
    return angle;
}

Base::Vector3d _circleCenter(Base::Vector3d p1, Base::Vector3d p2, Base::Vector3d p3)
{
    Base::Vector2d v1(p1.x, p1.y);
    Base::Vector2d v2(p2.x, p2.y);
    Base::Vector2d v3(p3.x, p3.y);
    Base::Vector2d c = Part::Geom2dCircle::getCircleCenter(v1, v2, v3);
    return Base::Vector3d(c.x, c.y, 0.0);
}

void _createThreadCircle(std::string Name, TechDraw::DrawViewPart* objFeat, float factor)
{
    // create the 3/4 arc symbolizing a thread from top seen
    double scale = objFeat->getScale();
    int GeoId = TechDraw::DrawUtil::getIndexFromName(Name);
    TechDraw::BaseGeomPtr geom = objFeat->getGeomByIndex(GeoId);
    std::string GeoType = TechDraw::DrawUtil::getGeomTypeFromName(Name);

    if (GeoType == "Edge" && geom->getGeomType() == TechDraw::CIRCLE) {
        TechDraw::CirclePtr cgen = std::static_pointer_cast<TechDraw::Circle>(geom);
        Base::Vector3d center = cgen->center;
        float radius = cgen->radius;
        TechDraw::BaseGeomPtr threadArc =
            std::make_shared<TechDraw::AOC>(center / scale, radius * factor / scale, 255.0, 165.0);
        std::string arcTag = objFeat->addCosmeticEdge(threadArc);
        TechDraw::CosmeticEdge* arc = objFeat->getCosmeticEdge(arcTag);
        _setLineAttributes(arc);
    }
}

void _createThreadLines(std::vector<std::string> SubNames, TechDraw::DrawViewPart* objFeat,
                        float factor)
{
    // create symbolizing lines of a thread from the side seen
    double scale = objFeat->getScale();
    std::string GeoType0 = TechDraw::DrawUtil::getGeomTypeFromName(SubNames[0]);
    std::string GeoType1 = TechDraw::DrawUtil::getGeomTypeFromName(SubNames[1]);
    if ((GeoType0 == "Edge") && (GeoType1 == "Edge")) {
        int GeoId0 = TechDraw::DrawUtil::getIndexFromName(SubNames[0]);
        int GeoId1 = TechDraw::DrawUtil::getIndexFromName(SubNames[1]);
        TechDraw::BaseGeomPtr geom0 = objFeat->getGeomByIndex(GeoId0);
        TechDraw::BaseGeomPtr geom1 = objFeat->getGeomByIndex(GeoId1);
        if (geom0->getGeomType() != TechDraw::GENERIC || geom1->getGeomType() != TechDraw::GENERIC) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("TechDraw Thread Hole Side"),
                                 QObject::tr("Please select two straight lines"));
            return;
        }

        TechDraw::GenericPtr line0 = std::static_pointer_cast<TechDraw::Generic>(geom0);
        TechDraw::GenericPtr line1 = std::static_pointer_cast<TechDraw::Generic>(geom1);
        Base::Vector3d start0 = line0->points.at(0);
        Base::Vector3d end0 = line0->points.at(1);
        Base::Vector3d start1 = line1->points.at(0);
        Base::Vector3d end1 = line1->points.at(1);
        if (DrawUtil::circulation(start0, end0, start1)
            != DrawUtil::circulation(end0, end1, start1)) {
            Base::Vector3d help1 = start1;
            Base::Vector3d help2 = end1;
            start1 = help2;
            end1 = help1;
        }
        start0.y = -start0.y;
        end0.y = -end0.y;
        start1.y = -start1.y;
        end1.y = -end1.y;
        float kernelDiam = (start1 - start0).Length();
        float kernelFactor = (kernelDiam * factor - kernelDiam) / 2;
        Base::Vector3d delta = (start1 - start0).Normalize() * kernelFactor;
        std::string line0Tag =
            objFeat->addCosmeticEdge((start0 - delta) / scale, (end0 - delta) / scale);
        std::string line1Tag =
            objFeat->addCosmeticEdge((start1 + delta) / scale, (end1 + delta) / scale);
        TechDraw::CosmeticEdge* cosTag0 = objFeat->getCosmeticEdge(line0Tag);
        TechDraw::CosmeticEdge* cosTag1 = objFeat->getCosmeticEdge(line1Tag);
        _setLineAttributes(cosTag0);
        _setLineAttributes(cosTag1);
    }
}

void _setLineAttributes(TechDraw::CosmeticEdge* cosEdge)
{
    // set line attributes of a cosmetic edge
    cosEdge->m_format.m_style = _getActiveLineAttributes().getStyle();
    cosEdge->m_format.m_weight = _getActiveLineAttributes().getWidthValue();
    cosEdge->m_format.m_color = _getActiveLineAttributes().getColorValue();
}

void _setLineAttributes(TechDraw::CenterLine* cosEdge)
{
    // set line attributes of a cosmetic edge
    cosEdge->m_format.m_style = _getActiveLineAttributes().getStyle();
    cosEdge->m_format.m_weight = _getActiveLineAttributes().getWidthValue();
    cosEdge->m_format.m_color = _getActiveLineAttributes().getColorValue();
}

void _setLineAttributes(TechDraw::CosmeticEdge* cosEdge, int style, float weight, App::Color color)
{
    // set line attributes of a cosmetic edge
    cosEdge->m_format.m_style = style;
    cosEdge->m_format.m_weight = weight;
    cosEdge->m_format.m_color = color;
}

void _setLineAttributes(TechDraw::CenterLine* cosEdge, int style, float weight, App::Color color)
{
    // set line attributes of a centerline
    cosEdge->m_format.m_style = style;
    cosEdge->m_format.m_weight = weight;
    cosEdge->m_format.m_color = color;
}
}// namespace TechDrawGui

//------------------------------------------------------------------------------
void CreateTechDrawCommandsExtensions()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

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
    rcCmdMgr.addCommand(new CmdTechDrawExtensionDrawCosmArc());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionDrawCosmCircle3Points());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionLinePPGroup());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionLineParallel());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionLinePerpendicular());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionThreadsGroup());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionThreadHoleSide());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionThreadBoltSide());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionThreadHoleBottom());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionThreadBoltBottom());
    rcCmdMgr.addCommand(new CmdTechDrawExtensionAreaAnnotation());
}
