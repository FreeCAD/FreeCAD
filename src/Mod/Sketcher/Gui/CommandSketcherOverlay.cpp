/***************************************************************************
 *   Copyright (c) 2023 Florian Foinant-Willig <flachyjoe@gmail.com>       *
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
#include <Inventor/SbString.h>
#include <QApplication>
#include <cfloat>
#endif

#include <App/Application.h>
#include <Base/Console.h>
#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Notifications.h>
#include <Gui/Selection.h>
#include <Gui/SelectionObject.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "DrawSketchHandler.h"
#include "Utils.h"
#include "ViewProviderSketch.h"

using namespace std;
using namespace SketcherGui;
using namespace Sketcher;


void ShowRestoreInformationLayer(const char* visibleelementname)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    bool status = hGrp->GetBool(visibleelementname, true);
    hGrp->SetBool(visibleelementname, !status);
}

// Show/Hide B-spline degree
DEF_STD_CMD_A(CmdSketcherBSplineDegree)

CmdSketcherBSplineDegree::CmdSketcherBSplineDegree()
    : Command("Sketcher_BSplineDegree")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Show/hide B-spline degree");
    sToolTipText = QT_TR_NOOP("Switches between showing and hiding the degree for all B-splines");
    sWhatsThis = "Sketcher_BSplineDegree";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_BSplineDegree";
    sAccel = "";
    eType = ForEdit;
}

void CmdSketcherBSplineDegree::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    ShowRestoreInformationLayer("BSplineDegreeVisible");
}

bool CmdSketcherBSplineDegree::isActive()
{
    return isSketcherBSplineActive(getActiveGuiDocument(), false);
}

// Show/Hide B-spline polygon
DEF_STD_CMD_A(CmdSketcherBSplinePolygon)

CmdSketcherBSplinePolygon::CmdSketcherBSplinePolygon()
    : Command("Sketcher_BSplinePolygon")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Show/hide B-spline control polygon");
    sToolTipText =
        QT_TR_NOOP("Switches between showing and hiding the control polygons for all B-splines");
    sWhatsThis = "Sketcher_BSplinePolygon";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_BSplinePolygon";
    sAccel = "";
    eType = ForEdit;
}

void CmdSketcherBSplinePolygon::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    ShowRestoreInformationLayer("BSplineControlPolygonVisible");
}

bool CmdSketcherBSplinePolygon::isActive()
{
    return isSketcherBSplineActive(getActiveGuiDocument(), false);
}

// Show/Hide B-spline comb
DEF_STD_CMD_A(CmdSketcherBSplineComb)

CmdSketcherBSplineComb::CmdSketcherBSplineComb()
    : Command("Sketcher_BSplineComb")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Show/hide B-spline curvature comb");
    sToolTipText =
        QT_TR_NOOP("Switches between showing and hiding the curvature comb for all B-splines");
    sWhatsThis = "Sketcher_BSplineComb";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_BSplineComb";
    sAccel = "";
    eType = ForEdit;
}

void CmdSketcherBSplineComb::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    ShowRestoreInformationLayer("BSplineCombVisible");
}

bool CmdSketcherBSplineComb::isActive()
{
    return isSketcherBSplineActive(getActiveGuiDocument(), false);
}

//
DEF_STD_CMD_A(CmdSketcherBSplineKnotMultiplicity)

CmdSketcherBSplineKnotMultiplicity::CmdSketcherBSplineKnotMultiplicity()
    : Command("Sketcher_BSplineKnotMultiplicity")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Show/hide B-spline knot multiplicity");
    sToolTipText =
        QT_TR_NOOP("Switches between showing and hiding the knot multiplicity for all B-splines");
    sWhatsThis = "Sketcher_BSplineKnotMultiplicity";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_BSplineKnotMultiplicity";
    sAccel = "";
    eType = ForEdit;
}

void CmdSketcherBSplineKnotMultiplicity::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    ShowRestoreInformationLayer("BSplineKnotMultiplicityVisible");
}

bool CmdSketcherBSplineKnotMultiplicity::isActive()
{
    return isSketcherBSplineActive(getActiveGuiDocument(), false);
}

//
DEF_STD_CMD_A(CmdSketcherBSplinePoleWeight)

CmdSketcherBSplinePoleWeight::CmdSketcherBSplinePoleWeight()
    : Command("Sketcher_BSplinePoleWeight")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Show/hide B-spline control point weight");
    sToolTipText = QT_TR_NOOP(
        "Switches between showing and hiding the control point weight for all B-splines");
    sWhatsThis = "Sketcher_BSplinePoleWeight";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_BSplinePoleWeight";
    sAccel = "";
    eType = ForEdit;
}

void CmdSketcherBSplinePoleWeight::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    ShowRestoreInformationLayer("BSplinePoleWeightVisible");
}

bool CmdSketcherBSplinePoleWeight::isActive()
{
    return isSketcherBSplineActive(getActiveGuiDocument(), false);
}

// Composite drop down menu for show/hide BSpline information layer
DEF_STD_CMD_ACLU(CmdSketcherCompBSplineShowHideGeometryInformation)

CmdSketcherCompBSplineShowHideGeometryInformation::
    CmdSketcherCompBSplineShowHideGeometryInformation()
    : Command("Sketcher_CompBSplineShowHideGeometryInformation")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Show/hide B-spline information layer");
    sToolTipText = sMenuText;
    sWhatsThis = "Sketcher_CompBSplineShowHideGeometryInformation";
    sStatusTip = sToolTipText;
    eType = ForEdit;
}

void CmdSketcherCompBSplineShowHideGeometryInformation::activated(int iMsg)
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();
    Gui::Command* cmd;

    if (iMsg == 0) {
        cmd = rcCmdMgr.getCommandByName("Sketcher_BSplineDegree");
    }
    else if (iMsg == 1) {
        cmd = rcCmdMgr.getCommandByName("Sketcher_BSplinePolygon");
    }
    else if (iMsg == 2) {
        cmd = rcCmdMgr.getCommandByName("Sketcher_BSplineComb");
    }
    else if (iMsg == 3) {
        cmd = rcCmdMgr.getCommandByName("Sketcher_BSplineKnotMultiplicity");
    }
    else if (iMsg == 4) {
        cmd = rcCmdMgr.getCommandByName("Sketcher_BSplinePoleWeight");
    }
    else {
        return;
    }

    cmd->invoke(0);

    // Since the default icon is reset when enabling/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
    // we must also set the tooltip of the used command
    pcAction->setToolTip(a[iMsg]->toolTip());
}

Gui::Action* CmdSketcherCompBSplineShowHideGeometryInformation::createAction()
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* c1 = pcAction->addAction(QString());
    c1->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_BSplineDegree"));
    QAction* c2 = pcAction->addAction(QString());
    c2->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_BSplinePolygon"));
    QAction* c3 = pcAction->addAction(QString());
    c3->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_BSplineComb"));
    QAction* c4 = pcAction->addAction(QString());
    c4->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_BSplineKnotMultiplicity"));
    QAction* c5 = pcAction->addAction(QString());
    c5->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_BSplinePoleWeight"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(c2->icon());
    int defaultId = 1;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdSketcherCompBSplineShowHideGeometryInformation::languageChange()
{
    Command::languageChange();

    if (!_pcAction) {
        return;
    }
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* c1 = a[0];
    c1->setText(QApplication::translate("CmdSketcherCompBSplineShowHideGeometryInformation",
                                        "Show/hide B-spline degree"));
    c1->setToolTip(QApplication::translate(
        "Sketcher_BSplineDegree",
        "Switches between showing and hiding the degree for all B-splines"));
    c1->setStatusTip(QApplication::translate(
        "Sketcher_BSplineDegree",
        "Switches between showing and hiding the degree for all B-splines"));
    QAction* c2 = a[1];
    c2->setText(QApplication::translate("CmdSketcherCompBSplineShowHideGeometryInformation",
                                        "Show/hide B-spline control polygon"));
    c2->setToolTip(QApplication::translate(
        "Sketcher_BSplinePolygon",
        "Switches between showing and hiding the control polygons for all B-splines"));
    c2->setStatusTip(QApplication::translate(
        "Sketcher_BSplinePolygon",
        "Switches between showing and hiding the control polygons for all B-splines"));
    QAction* c3 = a[2];
    c3->setText(QApplication::translate("CmdSketcherCompBSplineShowHideGeometryInformation",
                                        "Show/hide B-spline curvature comb"));
    c3->setToolTip(QApplication::translate(
        "Sketcher_BSplineComb",
        "Switches between showing and hiding the curvature comb for all B-splines"));
    c3->setStatusTip(QApplication::translate(
        "Sketcher_BSplineComb",
        "Switches between showing and hiding the curvature comb for all B-splines"));
    QAction* c4 = a[3];
    c4->setText(QApplication::translate("CmdSketcherCompBSplineShowHideGeometryInformation",
                                        "Show/hide B-spline knot multiplicity"));
    c4->setToolTip(QApplication::translate(
        "Sketcher_BSplineKnotMultiplicity",
        "Switches between showing and hiding the knot multiplicity for all B-splines"));
    c4->setStatusTip(QApplication::translate(
        "Sketcher_BSplineKnotMultiplicity",
        "Switches between showing and hiding the knot multiplicity for all B-splines"));

    QAction* c5 = a[4];
    c5->setText(QApplication::translate("CmdSketcherCompBSplineShowHideGeometryInformation",
                                        "Show/hide B-spline control point weight"));
    c5->setToolTip(QApplication::translate(
        "Sketcher_BSplinePoleWeight",
        "Switches between showing and hiding the control point weight for all B-splines"));
    c5->setStatusTip(QApplication::translate(
        "Sketcher_BSplinePoleWeight",
        "Switches between showing and hiding the control point weight for all B-splines"));
}

void CmdSketcherCompBSplineShowHideGeometryInformation::updateAction(int /*mode*/)
{}

bool CmdSketcherCompBSplineShowHideGeometryInformation::isActive()
{
    return isSketcherBSplineActive(getActiveGuiDocument(), false);
}

//
DEF_STD_CMD_A(CmdSketcherArcOverlay)

CmdSketcherArcOverlay::CmdSketcherArcOverlay()
    : Command("Sketcher_ArcOverlay")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Show/hide circular helper for arcs");
    sToolTipText =
        QT_TR_NOOP("Switches between showing and hiding the circular helper for all arcs");
    sWhatsThis = "Sketcher_ArcOverlay";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_ArcOverlay";
    sAccel = "";
    eType = ForEdit;
}

void CmdSketcherArcOverlay::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    ShowRestoreInformationLayer("ArcCircleHelperVisible");
}

bool CmdSketcherArcOverlay::isActive()
{
    return isSketchInEdit(getActiveGuiDocument());
}

void CreateSketcherCommandsOverlay()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdSketcherBSplineDegree());
    rcCmdMgr.addCommand(new CmdSketcherBSplinePolygon());
    rcCmdMgr.addCommand(new CmdSketcherBSplineComb());
    rcCmdMgr.addCommand(new CmdSketcherBSplineKnotMultiplicity());
    rcCmdMgr.addCommand(new CmdSketcherBSplinePoleWeight());
    rcCmdMgr.addCommand(new CmdSketcherCompBSplineShowHideGeometryInformation());
    rcCmdMgr.addCommand(new CmdSketcherArcOverlay());
}
