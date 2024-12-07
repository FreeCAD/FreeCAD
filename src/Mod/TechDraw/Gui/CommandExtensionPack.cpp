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
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#endif

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
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
#include <Mod/TechDraw/App/DrawViewDimension.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawViewSection.h>
#include <Mod/TechDraw/App/Preferences.h>
#include <Mod/TechDraw/App/LineFormat.h>
#include <Mod/TechDraw/App/LineGenerator.h>
#include <Mod/TechDraw/App/LineGroup.h>

#include "DrawGuiUtil.h"
#include "QGSPage.h"
#include "TaskSelectLineAttributes.h"
#include "ViewProviderBalloon.h"
#include "ViewProviderDimension.h"
#include "ViewProviderPage.h"


using namespace TechDrawGui;
using namespace TechDraw;
using DU = DrawUtil;


namespace TechDrawGui
{
//TechDraw::LineFormat activeAttributes; // container holding global line attributes

//internal helper functions
TechDraw::LineFormat& _getActiveLineAttributes();
Base::Vector3d _circleCenter(Base::Vector3d p1, Base::Vector3d p2, Base::Vector3d p3);
void _createThreadCircle(const std::string Name, TechDraw::DrawViewPart* objFeat, double factor);
void _createThreadLines(const std::vector<std::string>& SubNames, TechDraw::DrawViewPart* objFeat,
                        double factor);
void _setLineAttributes(TechDraw::CosmeticEdge* cosEdge);
void _setLineAttributes(TechDraw::CenterLine* cosEdge);
void _setLineAttributes(TechDraw::CosmeticEdge* cosEdge, int style, float weight, App::Color color);
void _setLineAttributes(TechDraw::CenterLine* cosEdge, int style, float weight, App::Color color);
double _getAngle(Base::Vector3d center, Base::Vector3d point);
std::vector<Base::Vector3d> _getVertexPoints(const std::vector<std::string>& SubNames,
                                             TechDraw::DrawViewPart* objFeat);
bool _checkSel(Gui::Command* cmd, std::vector<Gui::SelectionObject>& selection,
               TechDraw::DrawViewPart*& objFeat, const std::string& message);
std::string _createBalloon(Gui::Command* cmd, TechDraw::DrawViewPart* objFeat);

//===========================================================================
// TechDraw_ExtensionHoleCircle
//===========================================================================

void execHoleCircle(Gui::Command* cmd)
{
    //create centerlines of a hole/bolt circle
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat{nullptr};
    if (!_checkSel(cmd, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw Hole Circle"))) {
        return;
    }
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

    // make the bolt hole circle from 3 scaled and rotated points
    Base::Vector3d bigCenter =
        _circleCenter(Circles[0]->center, Circles[1]->center, Circles[2]->center);
    double bigRadius = (Circles[0]->center - bigCenter).Length();
    // now convert the center & radius to canonical form
    bigCenter = CosmeticVertex::makeCanonicalPointInverted(objFeat, bigCenter);
    bigRadius = bigRadius / objFeat->getScale();
    TechDraw::BaseGeomPtr bigCircle =
        std::make_shared<TechDraw::Circle>(bigCenter, bigRadius);
    std::string bigCircleTag = objFeat->addCosmeticEdge(bigCircle);
    TechDraw::CosmeticEdge* ceCircle = objFeat->getCosmeticEdge(bigCircleTag);
    _setLineAttributes(ceCircle);

    // make the center lines for the individual bolt holes
    constexpr double ExtendFactor{1.1};
    for (const TechDraw::CirclePtr& oneCircle : Circles) {
        // convert the center to canonical form
        Base::Vector3d oneCircleCenter = CosmeticVertex::makeCanonicalPointInverted(objFeat, oneCircle->center);
        // oneCircle->radius is scaled.
        double oneRadius = oneCircle->radius / objFeat->getScale();
        // what is magic number 2 (now ExtendFactor)?  just a fudge factor to extend the line beyond the bolt
        // hole circle?  should it be a function of hole diameter? maybe 110% of oneRadius?
        Base::Vector3d delta = (oneCircleCenter - bigCenter).Normalize() * (oneRadius * ExtendFactor);
        Base::Vector3d startPt = oneCircleCenter + delta;
        Base::Vector3d endPt = oneCircleCenter - delta;
        std::string oneLineTag = objFeat->addCosmeticEdge(startPt, endPt);
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
    TechDraw::DrawViewPart* objFeat{nullptr};
    if (!_checkSel(cmd, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw Circle Centerlines"))) {
        return;
    }
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Circle Centerlines"));
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    for (const std::string& Name : SubNames) {
        int GeoId = TechDraw::DrawUtil::getIndexFromName(Name);
        TechDraw::BaseGeomPtr geom = objFeat->getGeomByIndex(GeoId);
        std::string GeoType = TechDraw::DrawUtil::getGeomTypeFromName(Name);
        if (GeoType == "Edge") {
            if (geom->getGeomType() == TechDraw::CIRCLE || geom->getGeomType() == TechDraw::ARCOFCIRCLE) {
                TechDraw::CirclePtr cgen = std::static_pointer_cast<TechDraw::Circle>(geom);
                // cgen->center is a scaled, rotated and inverted point
                Base::Vector3d center = CosmeticVertex::makeCanonicalPointInverted(objFeat, cgen->center);
                double radius = cgen->radius / objFeat->getScale();
                // right, left, top, bottom are formed from a canonical point (center)
                // so they do not need to be changed to canonical form.
                Base::Vector3d right(center.x + radius + 2.0, center.y, 0.0);
                Base::Vector3d top(center.x, center.y + radius + 2.0, 0.0);
                Base::Vector3d left(center.x - radius - 2.0, center.y, 0.0);
                Base::Vector3d bottom(center.x, center.y - radius - 2.0, 0.0);
                std::string line1tag = objFeat->addCosmeticEdge(right, left);
                std::string line2tag = objFeat->addCosmeticEdge(top, bottom);
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

    auto pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
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

    if (!_pcAction) {
        return;
    }
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> action = pcAction->actions();

    QAction* arc1 = action[0];
    arc1->setText(
        QApplication::translate("CmdTechDrawExtensionCircleCenterLines", "Add Circle Centerlines"));
    arc1->setToolTip(QApplication::translate("CmdTechDrawExtensionCircleCenterLines",
                                             "Add centerlines to circles and arcs:<br>\
- Specify the line attributes (optional)<br>\
- Select one or more circles or arcs<br>\
- Click this tool"));
    arc1->setStatusTip(arc1->text());
    QAction* arc2 = action[1];
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
    constexpr double ThreadFactor{1.176};
    // add cosmetic thread to side view of hole
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat{nullptr};
    if (!_checkSel(cmd, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw Thread Hole Side"))) {
        return;
    }
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Cosmetic Thread Hole Side"));
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    if (SubNames.size() >= 2) {
        _createThreadLines(SubNames, objFeat, ThreadFactor);
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
    constexpr double ThreadFactor{0.85};
    // add cosmetic thread to side view of bolt
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat{nullptr};
    if (!_checkSel(cmd, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw Thread Bolt Side")))  {
        return;
    }
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Cosmetic Thread Bolt Side"));
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    if (SubNames.size() >= 2) {
        _createThreadLines(SubNames, objFeat, ThreadFactor);
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
    constexpr double ThreadFactor{1.177};           // factor above is 1.176. should they be the same?
    // add cosmetic thread to bottom view of hole
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat{nullptr};
    if (!_checkSel(cmd, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw Thread Hole Bottom"))) {
        return;
    }
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Cosmetic Thread Hole Bottom"));
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    for (const std::string& Name : SubNames) {
        _createThreadCircle(Name, objFeat, ThreadFactor);
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
    constexpr double ThreadFactor{0.85};
    // add cosmetic thread to bottom view of bolt
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat{nullptr};
    if (!_checkSel(cmd, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw Thread Bolt Bottom")))  {
        return;
    }
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Cosmetic Thread Bolt Bottom"));
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    for (const std::string& Name : SubNames) {
        _createThreadCircle(Name, objFeat, ThreadFactor);
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

    auto pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
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

    if (!_pcAction)  {
        return;
    }
    auto pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> action = pcAction->actions();

    QAction* arc1 = action[0];
    arc1->setText(QApplication::translate("CmdTechDrawExtensionThreadHoleSide",
                                          "Add Cosmetic Thread Hole Side View"));
    arc1->setToolTip(QApplication::translate("CmdTechDrawExtensionThreadHoleSide",
                                             "Add a cosmetic thread to the side view of a hole:<br>\
- Specify the line attributes (optional)<br>\
- Select two parallel lines<br>\
- Click this tool"));
    arc1->setStatusTip(arc1->text());
    QAction* arc2 = action[1];
    arc2->setText(QApplication::translate("CmdTechDrawExtensionThreadHoleBottom",
                                          "Add Cosmetic Thread Hole Bottom View"));
    arc2->setToolTip(
        QApplication::translate("CmdTechDrawExtensionThreadHoleBottom",
                                "Add a cosmetic thread to the top or bottom view of holes:<br>\
- Specify the line attributes (optional)<br>\
- Select one or more circles<br>\
- Click this tool"));
    arc2->setStatusTip(arc2->text());
    QAction* arc3 = action[2];
    arc3->setText(QApplication::translate("CmdTechDrawExtensionThreadBoltSide",
                                          "Add Cosmetic Thread Bolt Side View"));
    arc3->setToolTip(
        QApplication::translate("CmdTechDrawExtensionThreadBoltSide",
                                "Add a cosmetic thread to the side view of a bolt/screw/rod:<br>\
- Specify the line attributes (optional)<br>\
- Select two parallel lines<br>\
- Click this tool"));
    arc3->setStatusTip(arc3->text());
    QAction* arc4 = action[3];
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
    Gui::Control().showDialog(new TaskDlgSelectLineAttributes());
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
    TechDraw::DrawViewPart* objFeat{nullptr};
    if (!_checkSel(this, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw Change Line Attributes"))) {
        return;
    }
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
- Select two edges<br>\
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
    TechDraw::DrawViewPart* objFeat{nullptr};
    if (!_checkSel(this, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw Cosmetic Intersection Vertex(es)")))  {
        return;
    }
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Cosmetic Intersection Vertex(es)"));
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    if (SubNames.size() >= 2) {
        std::string GeoType1 = TechDraw::DrawUtil::getGeomTypeFromName(SubNames[0]);
        std::string GeoType2 = TechDraw::DrawUtil::getGeomTypeFromName(SubNames[1]);
        if (GeoType1 == "Edge" && GeoType2 == "Edge") {
            int GeoId1 = TechDraw::DrawUtil::getIndexFromName(SubNames[0]);
            TechDraw::BaseGeomPtr geom1 = objFeat->getGeomByIndex(GeoId1);
            int GeoId2 = TechDraw::DrawUtil::getIndexFromName(SubNames[1]);
            TechDraw::BaseGeomPtr geom2 = objFeat->getGeomByIndex(GeoId2);

            std::vector<Base::Vector3d> interPoints = geom1->intersection(geom2);
            for (auto pt : interPoints) {
                // geometry points are inverted
                Base::Vector3d temp = CosmeticVertex::makeCanonicalPointInverted(objFeat, pt);
                objFeat->addCosmeticVertex(temp, false);
            }
        }
    }
    getSelection().clearSelection();
    objFeat->refreshCVGeoms();
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

//! adds an anti-clockwise arc based on a center point, a radius/start angle point and an end angle
//! point.  Selection order is significant - center, start end.
void execDrawCosmArc(Gui::Command* cmd)
{
    //draw a cosmetic arc of circle
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat{nullptr};
    if (!_checkSel(cmd, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw Cosmetic Arc")))  {
        return;
    }
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Cosmetic Arc"));
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    std::vector<Base::Vector3d> vertexPoints;
    vertexPoints = _getVertexPoints(SubNames, objFeat);
    if (vertexPoints.size() >= 3) {
        // vertexPoints come from stored geometry, so are centered, scaled, rotated and inverted (CSRIz).
        // because the points are inverted, the start and end angles will be mirrored unless we invert the points
        // before calculating the angle.
        Base::Vector3d center = CosmeticVertex::makeCanonicalPoint(objFeat, DU::invertY(vertexPoints[0]));
        Base::Vector3d end1 = CosmeticVertex::makeCanonicalPoint(objFeat, DU::invertY(vertexPoints[1]));
        Base::Vector3d end2 = CosmeticVertex::makeCanonicalPoint(objFeat, DU::invertY(vertexPoints[2]));
        double arcRadius = (end1 - center).Length();
        double angle1 = _getAngle(center, end1);
        double angle2 = _getAngle(center, end2);
        TechDraw::BaseGeomPtr baseGeo = std::make_shared<TechDraw::AOC>(
            center, arcRadius, angle1, angle2);
        TechDraw::AOCPtr aoc = std::static_pointer_cast<TechDraw::AOC>(baseGeo);
        // having done our calculations in sensible coordinates, we convert to inverted coords
        std::string arcTag = objFeat->addCosmeticEdge(baseGeo->inverted());
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
    TechDraw::DrawViewPart* objFeat{nullptr};
    if (!_checkSel(cmd, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw Cosmetic Circle"))) {
        return;
    }
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Cosmetic Circle"));
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    std::vector<Base::Vector3d> vertexPoints;
    vertexPoints = _getVertexPoints(SubNames, objFeat);
    if (vertexPoints.size() >= 2) {
        double circleRadius = (vertexPoints[1] - vertexPoints[0]).Length() / objFeat->getScale();
        auto center = CosmeticVertex::makeCanonicalPointInverted(objFeat, vertexPoints[0]);
        TechDraw::BaseGeomPtr baseGeo =
            std::make_shared<TechDraw::Circle>(center, circleRadius);
        std::string circleTag = objFeat->addCosmeticEdge(baseGeo);
        TechDraw::CosmeticEdge* circleEdge = objFeat->getCosmeticEdge(circleTag);
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
    TechDraw::DrawViewPart* objFeat{nullptr};
    if (!_checkSel(cmd, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw Cosmetic Circle 3 Points")))  {
        return;
    }
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Cosmetic Circle 3 Points"));
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    std::vector<Base::Vector3d> vertexPoints;
    vertexPoints = _getVertexPoints(SubNames, objFeat);
    if (vertexPoints.size() >= 3) {
        Base::Vector3d circleCenter = _circleCenter(vertexPoints[0],
                                                    vertexPoints[1],
                                                    vertexPoints[2]);
        double circleRadius = (vertexPoints[0] - circleCenter).Length() / objFeat->getScale();
        circleCenter = CosmeticVertex::makeCanonicalPointInverted(objFeat, circleCenter);
        TechDraw::BaseGeomPtr theCircle =
            std::make_shared<TechDraw::Circle>(circleCenter, circleRadius);
        std::string circleTag = objFeat->addCosmeticEdge(theCircle);
        TechDraw::CosmeticEdge* circleEdge = objFeat->getCosmeticEdge(circleTag);
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

    auto pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
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

    if (!_pcAction)  {
        return;
    }
    auto pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> action = pcAction->actions();

    QAction* arc1 = action[0];
    arc1->setText(
        QApplication::translate("CmdTechDrawExtensionDrawCosmCircle", "Add Cosmetic Circle"));
    arc1->setToolTip(QApplication::translate("CmdTechDrawExtensionDrawCosmCircle",
                                             "Add a cosmetic circle based on two vertexes:<br>\
- Specify the line attributes (optional)<br>\
- Select vertex 1 (center point)<br>\
- Select vertex 2 (radius)<br>\
- Click this tool"));
    arc1->setStatusTip(arc1->text());
    QAction* arc2 = action[1];
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
    QAction* arc3 = action[2];
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
    TechDraw::DrawViewPart* objFeat{nullptr};
    if (!_checkSel(cmd, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw Cosmetic Line Parallel/Perpendicular"))) {
        return;
    }
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Cosmetic Line Parallel/Perpendicular"));
    const std::vector<std::string> SubNames = selection[0].getSubNames();
    if (SubNames.size() >= 2) {
        std::string GeoType1 = TechDraw::DrawUtil::getGeomTypeFromName(SubNames[0]);
        std::string GeoType2 = TechDraw::DrawUtil::getGeomTypeFromName(SubNames[1]);
        int EdgeId{-1};
        int VertId{-1};
        if (GeoType1 == "Edge" && GeoType2 == "Vertex") {
            EdgeId = TechDraw::DrawUtil::getIndexFromName(SubNames[0]);
            VertId = TechDraw::DrawUtil::getIndexFromName(SubNames[1]);
        } else if (GeoType2 == "Edge" && GeoType1 == "Vertex") {
            EdgeId = TechDraw::DrawUtil::getIndexFromName(SubNames[1]);
            VertId = TechDraw::DrawUtil::getIndexFromName(SubNames[0]);
        } else {
            // we don't have an edge + vertex as selection
            return;
        }
        TechDraw::BaseGeomPtr geom1 = objFeat->getGeomByIndex(EdgeId);
        TechDraw::GenericPtr lineGen = std::static_pointer_cast<TechDraw::Generic>(geom1);
        // ends are scaled and rotated
        Base::Vector3d lineStart = lineGen->points.at(0);
        lineStart = CosmeticVertex::makeCanonicalPointInverted(objFeat, lineStart);
        Base::Vector3d lineEnd = lineGen->points.at(1);
        lineEnd = CosmeticVertex::makeCanonicalPointInverted(objFeat, lineEnd);
        TechDraw::VertexPtr vert = objFeat->getProjVertexByIndex(VertId);
        Base::Vector3d vertexPoint(vert->point().x, vert->point().y, 0.0);
        vertexPoint = CosmeticVertex::makeCanonicalPointInverted(objFeat, vertexPoint);

        Base::Vector3d halfVector = (lineEnd - lineStart) / 2.0;
        if (!isParallel) {
            float dummy = halfVector.x;
            halfVector.x = -halfVector.y;
            halfVector.y = dummy;
        }
        Base::Vector3d startPoint = vertexPoint + halfVector;
        Base::Vector3d endPoint = vertexPoint - halfVector;
        TechDraw::BaseGeomPtr cLine = CosmeticEdge::makeLineFromCanonicalPoints(startPoint, endPoint);
        std::string lineTag = objFeat->addCosmeticEdge(cLine);
        TechDraw::CosmeticEdge* lineEdge = objFeat->getCosmeticEdge(lineTag);
        _setLineAttributes(lineEdge);
        objFeat->refreshCEGeoms();
        objFeat->requestPaint();
        cmd->getSelection().clearSelection();
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

    auto pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
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

    if (!_pcAction) {
        return;
    }
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> action = pcAction->actions();

    QAction* arc1 = action[0];
    arc1->setText(
        QApplication::translate("CmdTechDrawExtensionLineParallel", "Add Cosmetic Parallel Line"));
    arc1->setToolTip(
        QApplication::translate("CmdTechDrawExtensionLineParallel",
                                "Add a cosmetic line parallel to another line through a vertex:<br>\
- Select a line<br>\
- Select a vertex<br>\
- Click this tool"));
    arc1->setStatusTip(arc1->text());
    QAction* arc2 = action[1];
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
    TechDraw::DrawViewPart* objFeat{nullptr};
    if (!_checkSel(this, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw Lock/Unlock View")))  {
        return;
    }
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
// TechDraw_ExtensionExtendLine
//===========================================================================

void execExtendShortenLine(Gui::Command* cmd, bool extend)
{
    // extend or shorten a cosmetic line or a centerline
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat{nullptr};
    if (!_checkSel(cmd, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw Extend/Shorten Line"))) {
        return;
    }
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
                    // start and end points are geometry points and are scaled, rotated and inverted
                    // convert start and end to unscaled, unrotated.
                    Base::Vector3d P0 = CosmeticVertex::makeCanonicalPointInverted(objFeat, baseGeo->getStartPoint());
                    Base::Vector3d P1 = CosmeticVertex::makeCanonicalPointInverted(objFeat, baseGeo->getEndPoint());
                    bool isCenterLine = false;
                    TechDraw::CenterLine* centerEdge = nullptr;
                    if (baseGeo->getCosmetic()) {
                        std::string uniTag = baseGeo->getCosmeticTag();
                        int oldStyle = 1;
                        float oldWeight = 1.0;
                        App::Color oldColor;
                        std::vector<std::string> toDelete;
                        toDelete.push_back(uniTag);
                        if (baseGeo->source() == 1) {
                            // cosmetic edge
                            auto cosEdge = objFeat->getCosmeticEdge(uniTag);
                            oldStyle = cosEdge->m_format.getLineNumber();
                            oldWeight = cosEdge->m_format.getWidth();
                            oldColor = cosEdge->m_format.getColor();
                            objFeat->removeCosmeticEdge(toDelete);
                        }
                        else if (baseGeo->source() == 2) {
                            // centerline
                            isCenterLine = true;
                            centerEdge = objFeat->getCenterLine(uniTag);
                        }
                        Base::Vector3d direction = (P1 - P0).Normalize();
                        Base::Vector3d delta = direction * activeDimAttributes.getLineStretch();
                        Base::Vector3d startPt, endPt;
                        if (extend) {
                            // make it longer
                            startPt = P0 - delta;
                            endPt = P1 + delta;
                        }
                        else {
                            // make it shorter
                            startPt = P0 + delta;
                            endPt = P1 - delta;
                        }
                        // startPt.y = -startPt.y;
                        // endPt.y = -endPt.y;
                        if (isCenterLine) {
                            if (extend) {
                                centerEdge->m_extendBy += activeDimAttributes.getLineStretch();
                            } else {
                                centerEdge->m_extendBy -= activeDimAttributes.getLineStretch();
                            }
                            objFeat->refreshCLGeoms();
                        }
                        else {
                            std::string lineTag =
                                objFeat->addCosmeticEdge(startPt, endPt);
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

    auto pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
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

    if (!_pcAction) {
        return;
    }
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> action = pcAction->actions();

    QAction* arc1 = action[0];
    arc1->setText(QApplication::translate("CmdTechDrawExtensionExtendLine", "Extend Line"));
    arc1->setToolTip(QApplication::translate(
        "CmdTechDrawExtensionExtendLine", "Extend a cosmetic line or centerline at both ends:<br>\
- Specify the delta distance (optional)<br>\
- Select a single line<br>\
- Click this tool"));
    arc1->setStatusTip(arc1->text());
    QAction* arc2 = action[1];
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
    sToolTipText = QT_TR_NOOP("Select several faces then click this tool");
    sWhatsThis = "TechDraw_ExtensionAreaAnnotation";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_ExtensionAreaAnnotation";
}

void CmdTechDrawExtensionAreaAnnotation::activated(int iMsg)
// calculate the area of selected faces, create output in a balloon
{
    Q_UNUSED(iMsg);
    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart* objFeat{nullptr};
    if (!_checkSel(this, selection, objFeat, QT_TRANSLATE_NOOP("Command","TechDraw calculate selected area")))  {
        return;
    }

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
    Base::Vector3d center;
    double totalArea = 0.0;
    for (const std::string& name : subNames) {
        TechDraw::FacePtr face = objFeat->getFace(name);
        if (!face) {
            continue;
        }

        GProp_GProps faceProps;
        BRepGProp::SurfaceProperties(face->toOccFace(), faceProps);

        double faceArea = faceProps.Mass();
        totalArea += faceArea;
        center += faceArea*DrawUtil::toVector3d(faceProps.CentreOfMass());
    }
    if (totalArea > 0.0) {
        center /= totalArea;
    }

    // if area calculation was successful, start the command
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Calculate Face Area"));
    // at first we create the balloon
    std::string balloonName = _createBalloon(this, objFeat);
    TechDraw::DrawViewBalloon* balloon = nullptr;
    balloon = dynamic_cast<TechDraw::DrawViewBalloon*>(
        this->getDocument()->getObject(balloonName.c_str()));
    if (!balloon) {
        throw Base::TypeError("CmdTechDrawNewBalloon - balloon not found\n");
    }
    // the balloon has been created successfully

    // calculate needed variables
    double scale = objFeat->getScale();
    center = DrawUtil::invertY(center/scale);
    double scale2 = scale * scale;
    totalArea = totalArea / scale2;//convert from view scale to internal mm2

    //make area unit-aware
    Base::Quantity asQuantity;
    asQuantity.setValue(totalArea);
    asQuantity.setUnit(Base::Unit::Area);

    QString qUserString = asQuantity.getUserString();
    if (qUserString.endsWith(QString::fromUtf8("^2"))) {
        qUserString.chop(2);
        qUserString.append(QString::fromUtf8("²"));
    }
    std::string sUserString = qUserString.toStdString();

    // set the attributes in the data tab's fields
    //    balloon->SourceView.setValue(objFeat);
    balloon->BubbleShape.setValue("Rectangle");
    balloon->EndType.setValue("None");
    balloon->KinkLength.setValue(0.0);
    balloon->X.setValue(center.x);
    balloon->Y.setValue(center.y);
    balloon->OriginX.setValue(center.x);
    balloon->OriginY.setValue(center.y);
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
        viewProvider->LineWidth.setValue(TechDraw::LineGroup::getDefaultWidth("Graphic"));
        viewProvider->LineVisible.setValue(false);
        viewProvider->Color.setValue(App::Color(1.0, 0.0, 0.0));
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
// TechDraw_ExtensionArcLengthAnnotation
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawExtensionArcLengthAnnotation)

CmdTechDrawExtensionArcLengthAnnotation::CmdTechDrawExtensionArcLengthAnnotation()
    : Command("TechDraw_ExtensionArcLengthAnnotation")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Calculate the arc length of selected edges");
    sToolTipText = QT_TR_NOOP("Select several edges<br>\
    - click this tool");
    sWhatsThis = "TechDraw_ExtensionArcLengthAnnotation";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_ExtensionArcLengthAnnotation";
}

void CmdTechDrawExtensionArcLengthAnnotation::activated(int iMsg)
// Calculate the arc length of selected edge and create a balloon holding the datum
{
    Q_UNUSED(iMsg);

    std::vector<Gui::SelectionObject> selection;
    TechDraw::DrawViewPart *objFeat{nullptr};
    if (!_checkSel(this, selection, objFeat, QT_TRANSLATE_NOOP("Command", "TechDraw calculate selected arc length"))) {
        return;
    }

    // Collect all edges in the selection
    std::vector<std::string> subNames;
    for (auto &name : selection[0].getSubNames()) {
        if (DrawUtil::getGeomTypeFromName(name) == "Edge") {
            subNames.push_back(name);
        }
    }

    if (subNames.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect selection"),
                             QObject::tr("No edges in selection."));
        return;
    }

    // Now we have at least one edge
    std::vector<double> lengths(subNames.size());
    double totalLength = 0.0;
    for (size_t iName = 0; iName < subNames.size(); ++iName) {
        lengths[iName] = totalLength;
        TechDraw::BaseGeomPtr edge = objFeat->getEdge(subNames[iName]);
        if (!edge) {
            continue;
        }

        GProp_GProps edgeProps;
        BRepGProp::LinearProperties(edge->getOCCEdge(), edgeProps);

        totalLength += edgeProps.Mass();
        lengths[iName] = totalLength;
    }

    // We have calculated the length, let's start the command
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Calculate Edge Length"));

    // First we need to create the balloon
    std::string balloonName = _createBalloon(this, objFeat);
    auto balloon = dynamic_cast<TechDraw::DrawViewBalloon *>(getDocument()->getObject(balloonName.c_str()));
    if (!balloon) {
        throw Base::TypeError("CmdTechDrawNewBalloon - balloon not found\n");
    }

    // Find the edge halving the selected path and the offset from its starting point
    double anchorLength = totalLength*0.5;
    size_t iLength = 0;
    while (iLength < lengths.size() && lengths[iLength] < anchorLength) {
        ++iLength;
    }
    if (iLength > 0) {
        anchorLength -= lengths[iLength - 1];
    }

    // As reasonable anchor base point seems the "halving" edge endpoint
    BRepAdaptor_Curve curve(objFeat->getEdge(subNames[iLength])->getOCCEdge());
    gp_Pnt midPoint;
    curve.D0(curve.LastParameter(), midPoint);

    // Now try to get the real path center which lies anchorLength from edge start point
    GCPnts_AbscissaPoint abscissa(Precision::Confusion(), curve, anchorLength, curve.FirstParameter());
    if (abscissa.IsDone()) {
        curve.D0(abscissa.Parameter(), midPoint);
    }

    double scale = objFeat->getScale();
    Base::Vector3d anchor = DrawUtil::invertY(DrawUtil::toVector3d(midPoint)/scale);
    totalLength /= scale;

    // Use virtual dimension view helper to format resulting value
    TechDraw::DrawViewDimension helperDim;
    std::string valueStr = helperDim.formatValue(totalLength,
                                                 QString::fromUtf8(helperDim.FormatSpec.getStrValue().data()),
                                                 helperDim.isMultiValueSchema() ? 0 : 1);
    balloon->Text.setValue("◠ " + valueStr);

    // Set balloon format to be referencing dimension-like
    int stdStyle = Preferences::getPreferenceGroup("Dimensions")->GetInt("StandardAndStyle",
                       ViewProviderDimension::STD_STYLE_ISO_ORIENTED);
    bool asmeStyle = stdStyle == ViewProviderDimension::STD_STYLE_ASME_INLINED
                     || stdStyle == ViewProviderDimension::STD_STYLE_ASME_REFERENCING;
    balloon->BubbleShape.setValue(asmeStyle ? "None" : "Line");
    balloon->EndType.setValue(Preferences::getPreferenceGroup("Dimensions")->GetInt("ArrowStyle", 0));
    balloon->OriginX.setValue(anchor.x);
    balloon->OriginY.setValue(anchor.y);

    // Set balloon label position a bit upwards and to the right, as QGSPage::createBalloon does
    double textOffset = 20.0/scale;
    balloon->X.setValue(anchor.x + textOffset);
    balloon->Y.setValue(anchor.y + textOffset);

    // Adjust the kink length accordingly to the standard used
    auto viewProvider = dynamic_cast<ViewProviderBalloon *>(Gui::Application::Instance->getViewProvider(balloon));
    if (viewProvider) {
        balloon->KinkLength.setValue((asmeStyle ? 12.0 : 1.0)*viewProvider->LineWidth.getValue());
    }

    // Close the command and update the view
    Gui::Command::commitCommand();
    objFeat->touch(true);
    Gui::Command::updateActive();
}

bool CmdTechDrawExtensionArcLengthAnnotation::isActive()
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

LineFormat& _getActiveLineAttributes()
{
    return LineFormat::getCurrentLineFormat();
}

std::string _createBalloon(Gui::Command* cmd, TechDraw::DrawViewPart* objFeat)
// create a new balloon, return its name as string
{
    std::string featName;
    TechDraw::DrawPage* page = objFeat->findParentPage();
    Gui::Document* guiDoc = Gui::Application::Instance->getDocument(page->getDocument());
    auto pageVP = dynamic_cast<ViewProviderPage*>(guiDoc->getViewProvider(page));
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
               TechDraw::DrawViewPart*& objFeat, const std::string& message)
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

//! return the vertices in the selection as [Base::Vector3d]
std::vector<Base::Vector3d> _getVertexPoints(const std::vector<std::string>& SubNames,
                                             TechDraw::DrawViewPart* objFeat)
{
    std::vector<Base::Vector3d> vertexPoints;
    for (const std::string& Name : SubNames) {
        std::string GeoType = TechDraw::DrawUtil::getGeomTypeFromName(Name);
        if (GeoType == "Vertex") {
            int GeoId = TechDraw::DrawUtil::getIndexFromName(Name);
            TechDraw::VertexPtr vert = objFeat->getProjVertexByIndex(GeoId);
            vertexPoints.push_back(vert->point());
        }
    }
    return vertexPoints;
}

//! get angle between x-axis and the vector from center to point.
//! result is [0, 360]
double _getAngle(Base::Vector3d center, Base::Vector3d point)
{
    constexpr double DegreesHalfCircle{180.0};
    Base::Vector3d vecCP = point - center;
    double angle = DU::angleWithX(vecCP) * DegreesHalfCircle / M_PI;
    return angle;
}

Base::Vector3d _circleCenter(Base::Vector3d p1, Base::Vector3d p2, Base::Vector3d p3)
{
    Base::Vector2d v1(p1.x, p1.y);
    Base::Vector2d v2(p2.x, p2.y);
    Base::Vector2d v3(p3.x, p3.y);
    Base::Vector2d center = Part::Geom2dCircle::getCircleCenter(v1, v2, v3);
    return Base::Vector3d(center.x, center.y, 0.0);
}

void _createThreadCircle(const std::string Name, TechDraw::DrawViewPart* objFeat, double factor)
{
    constexpr double ArcStartDegree{255.0};
    constexpr double ArcEndDegree{165.0};
    // create the 3/4 arc symbolizing a thread from top seen
    double scale = objFeat->getScale();
    int GeoId = TechDraw::DrawUtil::getIndexFromName(Name);
    TechDraw::BaseGeomPtr geom = objFeat->getGeomByIndex(GeoId);
    std::string GeoType = TechDraw::DrawUtil::getGeomTypeFromName(Name);

    if (GeoType == "Edge" && geom->getGeomType() == TechDraw::CIRCLE) {
        TechDraw::CirclePtr cgen = std::static_pointer_cast<TechDraw::Circle>(geom);
        // center is rotated and scaled
        Base::Vector3d center = CosmeticVertex::makeCanonicalPointInverted(objFeat, cgen->center);
        // radius is scaled
        float radius = cgen->radius * factor / scale;
        TechDraw::BaseGeomPtr threadArc =
            std::make_shared<TechDraw::AOC>(center, radius, ArcStartDegree, ArcEndDegree);
        std::string arcTag = objFeat->addCosmeticEdge(threadArc);
        TechDraw::CosmeticEdge* arc = objFeat->getCosmeticEdge(arcTag);
        _setLineAttributes(arc);
    }
}

void _createThreadLines(const std::vector<std::string>& SubNames, TechDraw::DrawViewPart* objFeat,
                        double factor)
{
    // create symbolizing lines of a thread from the side seen
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
        // start and end points are scaled,rotated and inverted (CSRIx).
        // convert start and end to unscaled, unrotated.
        Base::Vector3d start0 = CosmeticVertex::makeCanonicalPointInverted(objFeat, line0->getStartPoint());
        Base::Vector3d start1 = CosmeticVertex::makeCanonicalPointInverted(objFeat, line1->getStartPoint());
        Base::Vector3d end0 = CosmeticVertex::makeCanonicalPointInverted(objFeat, line0->getEndPoint());
        Base::Vector3d end1 = CosmeticVertex::makeCanonicalPointInverted(objFeat, line1->getEndPoint());
        if (DrawUtil::circulation(start0, end0, start1)
            != DrawUtil::circulation(end0, end1, start1)) {
            Base::Vector3d help1 = start1;
            Base::Vector3d help2 = end1;
            start1 = help2;
            end1 = help1;
        }
        float kernelDiam = (start1 - start0).Length();
        float kernelFactor = (kernelDiam * factor - kernelDiam) / 2;
        Base::Vector3d delta = (start1 - start0).Normalize() * kernelFactor;
        std::string line0Tag =
            objFeat->addCosmeticEdge(start0 - delta, end0 - delta);
        std::string line1Tag =
            objFeat->addCosmeticEdge(start1 + delta, end1 + delta);
        TechDraw::CosmeticEdge* cosTag0 = objFeat->getCosmeticEdge(line0Tag);
        TechDraw::CosmeticEdge* cosTag1 = objFeat->getCosmeticEdge(line1Tag);
        _setLineAttributes(cosTag0);
        _setLineAttributes(cosTag1);
    }
}

void _setLineAttributes(TechDraw::CosmeticEdge* cosEdge)
{
    // set line attributes of a cosmetic edge
    cosEdge->m_format.setStyle(_getActiveLineAttributes().getStyle());
    cosEdge->m_format.setWidth(_getActiveLineAttributes().getWidth());
    cosEdge->m_format.setColor(_getActiveLineAttributes().getColor());
    cosEdge->m_format.setVisible(_getActiveLineAttributes().getVisible());
    cosEdge->m_format.setLineNumber(_getActiveLineAttributes().getLineNumber());
}

void _setLineAttributes(TechDraw::CenterLine* cosEdge)
{
    // set line attributes of a cosmetic edge
    cosEdge->m_format.setStyle(_getActiveLineAttributes().getStyle());
    cosEdge->m_format.setWidth(_getActiveLineAttributes().getWidth());
    cosEdge->m_format.setColor(_getActiveLineAttributes().getColor());
    cosEdge->m_format.setVisible(_getActiveLineAttributes().getVisible());
    cosEdge->m_format.setLineNumber(_getActiveLineAttributes().getLineNumber());
}

void _setLineAttributes(TechDraw::CosmeticEdge* cosEdge, int style, float weight, App::Color color)
{
    // set line attributes of a cosmetic edge
    cosEdge->m_format.setStyle(style);
    cosEdge->m_format.setWidth(weight);
    cosEdge->m_format.setColor(color);
    cosEdge->m_format.setVisible(_getActiveLineAttributes().getVisible());
    cosEdge->m_format.setLineNumber(LineGenerator::fromQtStyle((Qt::PenStyle)style));
}

void _setLineAttributes(TechDraw::CenterLine* cosEdge, int style, float weight, App::Color color)
{
    // set line attributes of a centerline
    cosEdge->m_format.setStyle(style);
    cosEdge->m_format.setWidth(weight);
    cosEdge->m_format.setColor(color);
    cosEdge->m_format.setVisible(_getActiveLineAttributes().getVisible());
    cosEdge->m_format.setLineNumber(style);}
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
    rcCmdMgr.addCommand(new CmdTechDrawExtensionArcLengthAnnotation());
}
