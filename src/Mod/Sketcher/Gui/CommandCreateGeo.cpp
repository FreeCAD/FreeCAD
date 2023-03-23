/***************************************************************************
 *   Copyright (c) 2010 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <cstdlib>
# include <memory>

# include <QApplication>
# include <QString>
#endif

#include <App/OriginFeature.h>
#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/SelectionFilter.h>
#include <Gui/SoFCUnifiedSelection.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Mod/Part/App/DatumFeature.h>
#include <Mod/Part/App/Geometry2d.h>
#include <Mod/Sketcher/App/Constraint.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "GeometryCreationMode.h"
#include "Utils.h"
#include "ViewProviderSketch.h"

// DrawSketchHandler* must be last includes
#include "DrawSketchHandler.h"
#include "DrawSketchHandlerArc.h"
#include "DrawSketchHandlerArcOfEllipse.h"
#include "DrawSketchHandlerArcOfHyperbola.h"
#include "DrawSketchHandlerArcOfParabola.h"
#include "DrawSketchHandlerBSpline.h"
#include "DrawSketchHandlerBSplineByInterpolation.h"
#include "DrawSketchHandlerCarbonCopy.h"
#include "DrawSketchHandlerCircle.h"
#include "DrawSketchHandlerEllipse.h"
#include "DrawSketchHandlerExtend.h"
#include "DrawSketchHandlerExternal.h"
#include "DrawSketchHandlerFillet.h"
#include "DrawSketchHandlerLine.h"
#include "DrawSketchHandlerLineSet.h"
#include "DrawSketchHandlerPoint.h"
#include "DrawSketchHandlerPolygon.h"
#include "DrawSketchHandlerRectangle.h"
#include "DrawSketchHandlerSlot.h"
#include "DrawSketchHandlerSplitting.h"
#include "DrawSketchHandlerTrimming.h"


using namespace std;
using namespace SketcherGui;

#define CONSTRUCTION_UPDATE_ACTION(CLASS, ICON)                                                             \
    /* This macro creates an updateAction() function that will toggle between normal & construction icon */ \
    void CLASS::updateAction(int mode)                                                                      \
    {                                                                                                       \
        auto act = getAction();                                                                             \
        if (act) {                                                                                          \
            switch (mode) {                                                                                 \
                case Normal:                                                                                \
                    act->setIcon(Gui::BitmapFactory().iconFromTheme(ICON));                                 \
                    break;                                                                                  \
                case Construction:                                                                          \
                    act->setIcon(Gui::BitmapFactory().iconFromTheme(ICON "_Constr"));                       \
                    break;                                                                                  \
            }                                                                                               \
        }                                                                                                   \
    }

namespace SketcherGui {
GeometryCreationMode geometryCreationMode=Normal;
}

/* Sketch commands =======================================================*/

DEF_STD_CMD_AU(CmdSketcherCreateLine)

CmdSketcherCreateLine::CmdSketcherCreateLine()
  : Command("Sketcher_CreateLine")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create line");
    sToolTipText    = QT_TR_NOOP("Create a line in the sketch");
    sWhatsThis      = "Sketcher_CreateLine";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreateLine";
    sAccel          = "G, L";
    eType           = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateLine,"Sketcher_CreateLine")

void CmdSketcherCreateLine::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerLine() );
}

bool CmdSketcherCreateLine::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}


/* Create Box =======================================================*/

DEF_STD_CMD_AU(CmdSketcherCreateRectangle)

CmdSketcherCreateRectangle::CmdSketcherCreateRectangle()
  : Command("Sketcher_CreateRectangle")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create rectangle");
    sToolTipText    = QT_TR_NOOP("Create a rectangle in the sketch");
    sWhatsThis      = "Sketcher_CreateRectangle";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreateRectangle";
    sAccel          = "G, R";
    eType           = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateRectangle,"Sketcher_CreateRectangle")

void CmdSketcherCreateRectangle::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerBox(DrawSketchHandlerBox::Diagonal) );
}

bool CmdSketcherCreateRectangle::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

DEF_STD_CMD_AU(CmdSketcherCreateRectangleCenter)

CmdSketcherCreateRectangleCenter::CmdSketcherCreateRectangleCenter()
  : Command("Sketcher_CreateRectangle_Center")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create centered rectangle");
    sToolTipText    = QT_TR_NOOP("Create a centered rectangle in the sketch");
    sWhatsThis      = "Sketcher_CreateRectangle_Center";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreateRectangle_Center";
    sAccel          = "G, V";
    eType           = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateRectangleCenter,"Sketcher_CreateRectangle_Center")

void CmdSketcherCreateRectangleCenter::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerBox(DrawSketchHandlerBox::CenterAndCorner) );
}

bool CmdSketcherCreateRectangleCenter::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}


/* Create rounded oblong =======================================================*/

DEF_STD_CMD_AU(CmdSketcherCreateOblong)

CmdSketcherCreateOblong::CmdSketcherCreateOblong()
    : Command("Sketcher_CreateOblong")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Create rounded rectangle");
    sToolTipText = QT_TR_NOOP("Create a rounded rectangle in the sketch");
    sWhatsThis = "Sketcher_CreateOblong";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateOblong";
    sAccel = "G, O";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateOblong,"Sketcher_CreateOblong")

void CmdSketcherCreateOblong::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerOblong());
}

bool CmdSketcherCreateOblong::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

/* Rectangles Comp command =========================================*/

DEF_STD_CMD_ACLU(CmdSketcherCompCreateRectangles)

CmdSketcherCompCreateRectangles::CmdSketcherCompCreateRectangles()
    : Command("Sketcher_CompCreateRectangles")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Create rectangles");
    sToolTipText = QT_TR_NOOP("Creates a rectangle in the sketch");
    sWhatsThis = "Sketcher_CompCreateRectangles";
    sStatusTip = sToolTipText;
    eType = ForEdit;
}

void CmdSketcherCompCreateRectangles::activated(int iMsg)
{
    if (iMsg == 0)
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerBox(DrawSketchHandlerBox::Diagonal));
    else if (iMsg == 1)
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerBox(DrawSketchHandlerBox::CenterAndCorner));
    else if (iMsg == 2)
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerOblong());
    else
        return;

    // Since the default icon is reset when enabling/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action* CmdSketcherCompCreateRectangles::createAction()
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* arc1 = pcAction->addAction(QString());
    arc1->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangle"));
    QAction* arc2 = pcAction->addAction(QString());
    arc2->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangle_Center"));
    QAction* arc3 = pcAction->addAction(QString());
    arc3->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateOblong"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(arc1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdSketcherCompCreateRectangles::updateAction(int mode)
{
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(getAction());
    if (!pcAction)
        return;

    QList<QAction*> a = pcAction->actions();
    int index = pcAction->property("defaultAction").toInt();
    switch (mode) {
    case Normal:
        a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangle"));
        a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangle_Center"));
        a[2]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateOblong"));
        getAction()->setIcon(a[index]->icon());
        break;
    case Construction:
        a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangle_Constr"));
        a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangle_Center_Constr"));
        a[2]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateOblong_Constr"));
        getAction()->setIcon(a[index]->icon());
        break;
    }
}

void CmdSketcherCompCreateRectangles::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* rectangle1 = a[0];
    rectangle1->setText(QApplication::translate("CmdSketcherCompCreateRectangles", "Rectangle"));
    rectangle1->setToolTip(QApplication::translate("Sketcher_CreateRectangle", "Create a rectangle"));
    rectangle1->setStatusTip(rectangle1->toolTip());
    QAction* rectangle2 = a[1];
    rectangle2->setText(QApplication::translate("CmdSketcherCompCreateRectangles", "Centered rectangle"));
    rectangle2->setToolTip(QApplication::translate("Sketcher_CreateRectangle_Center", "Create a centered rectangle"));
    rectangle2->setStatusTip(rectangle2->toolTip());
    QAction* rectangle3 = a[2];
    rectangle3->setText(QApplication::translate("CmdSketcherCompCreateRectangles", "Rounded rectangle"));
    rectangle3->setToolTip(QApplication::translate("Sketcher_CreateOblong", "Create a rounded rectangle"));
    rectangle3->setStatusTip(rectangle3->toolTip());
}

bool CmdSketcherCompCreateRectangles::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// ======================================================================================

DEF_STD_CMD_AU(CmdSketcherCreatePolyline)

CmdSketcherCreatePolyline::CmdSketcherCreatePolyline()
  : Command("Sketcher_CreatePolyline")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create polyline");
    sToolTipText    = QT_TR_NOOP("Create a polyline in the sketch. 'M' Key cycles behaviour");
    sWhatsThis      = "Sketcher_CreatePolyline";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreatePolyline";
    sAccel          = "G, M";
    eType           = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreatePolyline,"Sketcher_CreatePolyline")

void CmdSketcherCreatePolyline::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerLineSet() );
}

bool CmdSketcherCreatePolyline::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}


// ======================================================================================

DEF_STD_CMD_AU(CmdSketcherCreateArc)

CmdSketcherCreateArc::CmdSketcherCreateArc()
  : Command("Sketcher_CreateArc")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create arc by center");
    sToolTipText    = QT_TR_NOOP("Create an arc by its center and by its end points");
    sWhatsThis      = "Sketcher_CreateArc";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreateArc";
    sAccel          = "G, A";
    eType           = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateArc,"Sketcher_CreateArc")

void CmdSketcherCreateArc::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerArc() );
}

bool CmdSketcherCreateArc::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}


// ======================================================================================

DEF_STD_CMD_AU(CmdSketcherCreate3PointArc)

CmdSketcherCreate3PointArc::CmdSketcherCreate3PointArc()
  : Command("Sketcher_Create3PointArc")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create arc by three points");
    sToolTipText    = QT_TR_NOOP("Create an arc by its end points and a point along the arc");
    sWhatsThis      = "Sketcher_Create3PointArc";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_Create3PointArc";
    sAccel          = "G, 3, A";
    eType           = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreate3PointArc,"Sketcher_Create3PointArc")

void CmdSketcherCreate3PointArc::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandler3PointArc() );
}

bool CmdSketcherCreate3PointArc::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}


DEF_STD_CMD_ACLU(CmdSketcherCompCreateArc)

CmdSketcherCompCreateArc::CmdSketcherCompCreateArc()
  : Command("Sketcher_CompCreateArc")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create arc");
    sToolTipText    = QT_TR_NOOP("Create an arc in the sketcher");
    sWhatsThis      = "Sketcher_CompCreateArc";
    sStatusTip      = sToolTipText;
    eType           = ForEdit;
}

void CmdSketcherCompCreateArc::activated(int iMsg)
{
    if (iMsg==0)
        ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerArc());
    else if (iMsg==1)
        ActivateHandler(getActiveGuiDocument(),new DrawSketchHandler3PointArc());
    else
        return;

    // Since the default icon is reset when enabling/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action * CmdSketcherCompCreateArc::createAction()
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* arc1 = pcAction->addAction(QString());
    arc1->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateArc"));
    QAction* arc2 = pcAction->addAction(QString());
    arc2->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Create3PointArc"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(arc1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdSketcherCompCreateArc::updateAction(int mode)
{
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(getAction());
    if (!pcAction)
        return;

    QList<QAction*> a = pcAction->actions();
    int index = pcAction->property("defaultAction").toInt();
    switch (mode) {
    case Normal:
        a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateArc"));
        a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Create3PointArc"));
        getAction()->setIcon(a[index]->icon());
        break;
    case Construction:
        a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateArc_Constr"));
        a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Create3PointArc_Constr"));
        getAction()->setIcon(a[index]->icon());
        break;
    }
}

void CmdSketcherCompCreateArc::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* arc1 = a[0];
    arc1->setText(QApplication::translate("CmdSketcherCompCreateArc","Center and end points"));
    arc1->setToolTip(QApplication::translate("Sketcher_CreateArc","Create an arc by its center and by its end points"));
    arc1->setStatusTip(QApplication::translate("Sketcher_CreateArc","Create an arc by its center and by its end points"));
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("CmdSketcherCompCreateArc","End points and rim point"));
    arc2->setToolTip(QApplication::translate("Sketcher_Create3PointArc","Create an arc by its end points and a point along the arc"));
    arc2->setStatusTip(QApplication::translate("Sketcher_Create3PointArc","Create an arc by its end points and a point along the arc"));
}

bool CmdSketcherCompCreateArc::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}


// ======================================================================================


DEF_STD_CMD_AU(CmdSketcherCreateCircle)

CmdSketcherCreateCircle::CmdSketcherCreateCircle()
  : Command("Sketcher_CreateCircle")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create circle");
    sToolTipText    = QT_TR_NOOP("Create a circle in the sketch");
    sWhatsThis      = "Sketcher_CreateCircle";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreateCircle";
    sAccel          = "G, C";
    eType           = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateCircle,"Sketcher_CreateCircle")

void CmdSketcherCreateCircle::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerCircle() );
}

bool CmdSketcherCreateCircle::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}
// ======================================================================================

/// @brief Macro that declares a new sketcher command class 'CmdSketcherCreateEllipseByCenter'
DEF_STD_CMD_AU(CmdSketcherCreateEllipseByCenter)

/**
 * @brief ctor
 */
CmdSketcherCreateEllipseByCenter::CmdSketcherCreateEllipseByCenter()
  : Command("Sketcher_CreateEllipseByCenter")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create ellipse by center");
    sToolTipText    = QT_TR_NOOP("Create an ellipse by center in the sketch");
    sWhatsThis      = "Sketcher_CreateEllipseByCenter";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreateEllipseByCenter";
    sAccel          = "G, E, E";
    eType           = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateEllipseByCenter, "Sketcher_CreateEllipseByCenter")

void CmdSketcherCreateEllipseByCenter::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerEllipse(0) );
}

bool CmdSketcherCreateEllipseByCenter::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

/// @brief Macro that declares a new sketcher command class 'CmdSketcherCreateEllipseBy3Points'
DEF_STD_CMD_AU(CmdSketcherCreateEllipseBy3Points)

/**
 * @brief ctor
 */
CmdSketcherCreateEllipseBy3Points::CmdSketcherCreateEllipseBy3Points()
  : Command("Sketcher_CreateEllipseBy3Points")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create ellipse by 3 points");
    sToolTipText    = QT_TR_NOOP("Create an ellipse by 3 points in the sketch");
    sWhatsThis      = "Sketcher_CreateEllipseBy3Points";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreateEllipse_3points";
    sAccel          = "G, 3, E";
    eType           = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateEllipseBy3Points,"Sketcher_CreateEllipse_3points")

void CmdSketcherCreateEllipseBy3Points::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerEllipse(1) );
}

bool CmdSketcherCreateEllipseBy3Points::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

DEF_STD_CMD_AU(CmdSketcherCreateArcOfEllipse)

CmdSketcherCreateArcOfEllipse::CmdSketcherCreateArcOfEllipse()
  : Command("Sketcher_CreateArcOfEllipse")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create an arc of ellipse");
    sToolTipText    = QT_TR_NOOP("Create an arc of ellipse in the sketch");
    sWhatsThis      = "Sketcher_CreateArcOfEllipse";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreateElliptical_Arc";
    sAccel          = "G, E, A";
    eType           = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateArcOfEllipse,"Sketcher_CreateElliptical_Arc")

void CmdSketcherCreateArcOfEllipse::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerArcOfEllipse() );
}

bool CmdSketcherCreateArcOfEllipse::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

DEF_STD_CMD_AU(CmdSketcherCreateArcOfHyperbola)

CmdSketcherCreateArcOfHyperbola::CmdSketcherCreateArcOfHyperbola()
  : Command("Sketcher_CreateArcOfHyperbola")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create an arc of hyperbola");
    sToolTipText    = QT_TR_NOOP("Create an arc of hyperbola in the sketch");
    sWhatsThis      = "Sketcher_CreateArcOfHyperbola";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreateHyperbolic_Arc";
    sAccel          = "G, H";
    eType           = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateArcOfHyperbola,"Sketcher_CreateHyperbolic_Arc")

void CmdSketcherCreateArcOfHyperbola::activated(int /*iMsg*/)
{
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerArcOfHyperbola() );
}

bool CmdSketcherCreateArcOfHyperbola::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

DEF_STD_CMD_AU(CmdSketcherCreateArcOfParabola)

CmdSketcherCreateArcOfParabola::CmdSketcherCreateArcOfParabola()
  : Command("Sketcher_CreateArcOfParabola")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create an arc of parabola");
    sToolTipText    = QT_TR_NOOP("Create an arc of parabola in the sketch");
    sWhatsThis      = "Sketcher_CreateArcOfParabola";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreateParabolic_Arc";
    sAccel          = "G, J";
    eType           = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateArcOfParabola,"Sketcher_CreateParabolic_Arc")

void CmdSketcherCreateArcOfParabola::activated(int /*iMsg*/)
{
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerArcOfParabola() );
}

bool CmdSketcherCreateArcOfParabola::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}




/// @brief Macro that declares a new sketcher command class 'CmdSketcherCompCreateEllipse'
DEF_STD_CMD_ACLU(CmdSketcherCompCreateConic)

/**
 * @brief ctor
 */
CmdSketcherCompCreateConic::CmdSketcherCompCreateConic()
  : Command("Sketcher_CompCreateConic")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create a conic");
    sToolTipText    = QT_TR_NOOP("Create a conic in the sketch");
    sWhatsThis      = "Sketcher_CompCreateConic";
    sStatusTip      = sToolTipText;
    eType           = ForEdit;
}

/**
 * @brief Instantiates the conic handler when the conic command activated
 * @param int iMsg
 */
void CmdSketcherCompCreateConic::activated(int iMsg)
{
    if (iMsg == 0) {
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerEllipse(iMsg));
    } else if (iMsg == 1) {
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerEllipse(iMsg));
    } else if (iMsg == 2) {
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerArcOfEllipse());
    } else if (iMsg == 3) {
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerArcOfHyperbola());
    } else if (iMsg == 4) {
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerArcOfParabola());
    } else {
        return;
    }

    // Since the default icon is reset when enabling/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action * CmdSketcherCompCreateConic::createAction()
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* ellipseByCenter = pcAction->addAction(QString());
    ellipseByCenter->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateEllipseByCenter"));
    QAction* ellipseBy3Points = pcAction->addAction(QString());
    ellipseBy3Points->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateEllipse_3points"));

    QAction* arcofellipse = pcAction->addAction(QString());
    arcofellipse->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateElliptical_Arc"));

    QAction* arcofhyperbola = pcAction->addAction(QString());
    arcofhyperbola->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateHyperbolic_Arc"));

    QAction* arcofparabola = pcAction->addAction(QString());
    arcofparabola->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateParabolic_Arc"));

    _pcAction = pcAction;
    languageChange();

    // set ellipse by center, a, b as default method
    pcAction->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Conics"));
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdSketcherCompCreateConic::updateAction(int mode)
{
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(getAction());
    if (!pcAction)
        return;

    QList<QAction*> a = pcAction->actions();
    int index = pcAction->property("defaultAction").toInt();
    switch (mode) {
    case Normal:
        a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateEllipseByCenter"));
        a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateEllipse_3points"));
        a[2]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateElliptical_Arc"));
        a[3]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateHyperbolic_Arc"));
        a[4]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateParabolic_Arc"));
        getAction()->setIcon(a[index]->icon());
        break;
    case Construction:
        a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateEllipseByCenter_Constr"));
        a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateEllipse_3points_Constr"));
        a[2]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateElliptical_Arc_Constr"));
        a[3]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateHyperbolic_Arc_Constr"));
        a[4]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateParabolic_Arc_Constr"));
        getAction()->setIcon(a[index]->icon());
        break;
    }
}

void CmdSketcherCompCreateConic::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* ellipseByCenter = a[0];
    ellipseByCenter->setText(QApplication::translate("CmdSketcherCompCreateConic","Ellipse by center, major radius, point"));
    ellipseByCenter->setToolTip(QApplication::translate("Sketcher_CreateEllipseByCenter","Create an ellipse by center, major radius and point"));
    ellipseByCenter->setStatusTip(QApplication::translate("Sketcher_CreateEllipseByCenter","Create an ellipse by center, major radius and point"));
    QAction* ellipseBy3Points = a[1];
    ellipseBy3Points->setText(QApplication::translate("CmdSketcherCompCreateConic","Ellipse by periapsis, apoapsis, minor radius"));
    ellipseBy3Points->setToolTip(QApplication::translate("Sketcher_CreateEllipseBy3Points","Create a ellipse by periapsis, apoapsis, and minor radius"));
    ellipseBy3Points->setStatusTip(QApplication::translate("Sketcher_CreateEllipseBy3Points","Create a ellipse by periapsis, apoapsis, and minor radius"));
    QAction* arcofellipse = a[2];
    arcofellipse->setText(QApplication::translate("CmdSketcherCompCreateConic","Arc of ellipse by center, major radius, endpoints"));
    arcofellipse->setToolTip(QApplication::translate("Sketcher_CreateArcOfEllipse","Create an arc of ellipse by its center, major radius, and endpoints"));
    arcofellipse->setStatusTip(QApplication::translate("Sketcher_CreateArcOfEllipse","Create an arc of ellipse by its center, major radius, and endpoints"));
    QAction* arcofhyperbola = a[3];
    arcofhyperbola->setText(QApplication::translate("CmdSketcherCompCreateConic","Arc of hyperbola by center, major radius, endpoints"));
    arcofhyperbola->setToolTip(QApplication::translate("Sketcher_CreateArcOfHyperbola","Create an arc of hyperbola by its center, major radius, and endpoints"));
    arcofhyperbola->setStatusTip(QApplication::translate("Sketcher_CreateArcOfHyperbola","Create an arc of hyperbola by its center, major radius, and endpoints"));
    QAction* arcofparabola = a[4];
    arcofparabola->setText(QApplication::translate("CmdSketcherCompCreateConic","Arc of parabola by focus, vertex, endpoints"));
    arcofparabola->setToolTip(QApplication::translate("Sketcher_CreateArcOfParabola","Create an arc of parabola by its focus, vertex, and endpoints"));
    arcofparabola->setStatusTip(QApplication::translate("Sketcher_CreateArcOfParabola","Create an arc of parabola by its focus, vertex, and endpoints"));
}

bool CmdSketcherCompCreateConic::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// ======================================================================================

DEF_STD_CMD_AU(CmdSketcherCreateBSpline)

CmdSketcherCreateBSpline::CmdSketcherCreateBSpline()
  : Command("Sketcher_CreateBSpline")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create B-spline");
    sToolTipText    = QT_TR_NOOP("Create a B-spline via control points in the sketch.");
    sWhatsThis      = "Sketcher_CreateBSpline";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreateBSpline";
    sAccel          = "G, B, B";
    eType           = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateBSpline,"Sketcher_CreateBSpline")

void CmdSketcherCreateBSpline::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerBSpline(0) );
}

bool CmdSketcherCreateBSpline::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

/// @brief Macro that declares a new sketcher command class 'CmdSketcherCreateBSpline'
DEF_STD_CMD_AU(CmdSketcherCreatePeriodicBSpline)

/**
 * @brief ctor
 */
CmdSketcherCreatePeriodicBSpline::CmdSketcherCreatePeriodicBSpline()
: Command("Sketcher_CreatePeriodicBSpline")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create periodic B-spline");
    sToolTipText    = QT_TR_NOOP("Create a periodic B-spline via control points in the sketch.");
    sWhatsThis      = "Sketcher_CreatePeriodicBSpline";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_Create_Periodic_BSpline";
    sAccel          = "G, B, P";
    eType           = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreatePeriodicBSpline,"Sketcher_Create_Periodic_BSpline")

void CmdSketcherCreatePeriodicBSpline::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerBSpline(1) );
}

bool CmdSketcherCreatePeriodicBSpline::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}


/// @brief Macro that declares a new sketcher command class 'CmdSketcherCompCreateBSpline'
DEF_STD_CMD_ACLU(CmdSketcherCompCreateBSpline)

/**
 * @brief ctor
 */
CmdSketcherCompCreateBSpline::CmdSketcherCompCreateBSpline()
: Command("Sketcher_CompCreateBSpline")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create a B-spline");
    sToolTipText    = QT_TR_NOOP("Create a B-spline in the sketch");
    sWhatsThis      = "Sketcher_CompCreateBSpline";
    sStatusTip      = sToolTipText;
    eType           = ForEdit;
}

/**
 * @brief Instantiates the B-spline handler when the B-spline command activated
 * @param int iMsg
 */
void CmdSketcherCompCreateBSpline::activated(int iMsg)
{
    if (iMsg == 0) {
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerBSpline(iMsg));
    }
    else if (iMsg == 1) {
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerBSpline(iMsg));
    }
    else if (iMsg == 2) {
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerBSplineByInterpolation(0));
    }
    else if (iMsg == 3) {
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerBSplineByInterpolation(1));
    }
    else {
        return;
    }

    // Since the default icon is reset when enabling/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action * CmdSketcherCompCreateBSpline::createAction()
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* bspline = pcAction->addAction(QString());
    bspline->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateBSpline"));

    QAction* periodicbspline = pcAction->addAction(QString());
    periodicbspline->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Create_Periodic_BSpline"));

    QAction* bsplinebyknot = pcAction->addAction(QString());
    bsplinebyknot->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateBSplineByInterpolation"));

    QAction* periodicbsplinebyknot = pcAction->addAction(QString());
    periodicbsplinebyknot->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Create_Periodic_BSplineByInterpolation"));

    _pcAction = pcAction;
    languageChange();

    // default
    pcAction->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateBSpline"));
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdSketcherCompCreateBSpline::updateAction(int mode)
{
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(getAction());
    if (!pcAction)
        return;

    QList<QAction*> a = pcAction->actions();
    int index = pcAction->property("defaultAction").toInt();
    switch (mode) {
        case Normal:
            a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateBSpline"));
            a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Create_Periodic_BSpline"));
            getAction()->setIcon(a[index]->icon());
            break;
        case Construction:
            a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateBSpline_Constr"));
            a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Create_Periodic_BSpline_Constr"));
            getAction()->setIcon(a[index]->icon());
            break;
    }
}

void CmdSketcherCompCreateBSpline::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* bspline = a[0];
    bspline->setText(QApplication::translate("Sketcher_CreateBSpline","B-spline by control points"));
    bspline->setToolTip(QApplication::translate("Sketcher_CreateBSpline","Create a B-spline by control points"));
    bspline->setStatusTip(QApplication::translate("Sketcher_CreateBSpline","Create a B-spline by control points"));
    QAction* periodicbspline = a[1];
    periodicbspline->setText(QApplication::translate("Sketcher_Create_Periodic_BSpline","Periodic B-spline by control points"));
    periodicbspline->setToolTip(QApplication::translate("Sketcher_Create_Periodic_BSpline","Create a periodic B-spline by control points"));
    periodicbspline->setStatusTip(QApplication::translate("Sketcher_Create_Periodic_BSpline","Create a periodic B-spline by control points"));
}

bool CmdSketcherCompCreateBSpline::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}


// ======================================================================================

DEF_STD_CMD_AU(CmdSketcherCreate3PointCircle)

CmdSketcherCreate3PointCircle::CmdSketcherCreate3PointCircle()
  : Command("Sketcher_Create3PointCircle")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create circle by three points");
    sToolTipText    = QT_TR_NOOP("Create a circle by 3 perimeter points");
    sWhatsThis      = "Sketcher_Create3PointCircle";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_Create3PointCircle";
    sAccel          = "G, 3, C";
    eType           = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreate3PointCircle,"Sketcher_Create3PointCircle")

void CmdSketcherCreate3PointCircle::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandler3PointCircle() );
}

bool CmdSketcherCreate3PointCircle::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}


DEF_STD_CMD_ACLU(CmdSketcherCompCreateCircle)

CmdSketcherCompCreateCircle::CmdSketcherCompCreateCircle()
  : Command("Sketcher_CompCreateCircle")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create circle");
    sToolTipText    = QT_TR_NOOP("Create a circle in the sketcher");
    sWhatsThis      = "Sketcher_CompCreateCircle";
    sStatusTip      = sToolTipText;
    eType           = ForEdit;
}

void CmdSketcherCompCreateCircle::activated(int iMsg)
{
    if (iMsg==0)
        ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerCircle());
    else if (iMsg==1)
        ActivateHandler(getActiveGuiDocument(),new DrawSketchHandler3PointCircle());
    else
        return;

    // Since the default icon is reset when enabling/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action * CmdSketcherCompCreateCircle::createAction()
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* arc1 = pcAction->addAction(QString());
    arc1->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateCircle"));
    QAction* arc2 = pcAction->addAction(QString());
    arc2->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Create3PointCircle"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(arc1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdSketcherCompCreateCircle::updateAction(int mode)
{
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(getAction());
    if (!pcAction)
        return;

    QList<QAction*> a = pcAction->actions();
    int index = pcAction->property("defaultAction").toInt();
    switch (mode) {
    case Normal:
        a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateCircle"));
        a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Create3PointCircle"));
        getAction()->setIcon(a[index]->icon());
        break;
    case Construction:
        a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateCircle_Constr"));
        a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Create3PointCircle_Constr"));
        getAction()->setIcon(a[index]->icon());
        break;
    }
}

void CmdSketcherCompCreateCircle::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* arc1 = a[0];
    arc1->setText(QApplication::translate("CmdSketcherCompCreateCircle", "Center and rim point"));
    arc1->setToolTip(QApplication::translate("Sketcher_CreateCircle", "Create a circle by its center and by a rim point"));
    arc1->setStatusTip(QApplication::translate("Sketcher_CreateCircle", "Create a circle by its center and by a rim point"));
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("CmdSketcherCompCreateCircle", "3 rim points"));
    arc2->setToolTip(QApplication::translate("Sketcher_Create3PointCircle", "Create a circle by 3 rim points"));
    arc2->setStatusTip(QApplication::translate("Sketcher_Create3PointCircle", "Create a circle by 3 rim points"));
}

bool CmdSketcherCompCreateCircle::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}


// ======================================================================================


DEF_STD_CMD_A(CmdSketcherCreatePoint)

CmdSketcherCreatePoint::CmdSketcherCreatePoint()
  : Command("Sketcher_CreatePoint")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create point");
    sToolTipText    = QT_TR_NOOP("Create a point in the sketch");
    sWhatsThis      = "Sketcher_CreatePoint";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreatePoint";
    sAccel          = "G, Y";
    eType           = ForEdit;
}

void CmdSketcherCreatePoint::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerPoint());
}

bool CmdSketcherCreatePoint::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// ======================================================================================

DEF_STD_CMD_A(CmdSketcherCreateFillet)

CmdSketcherCreateFillet::CmdSketcherCreateFillet()
  : Command("Sketcher_CreateFillet")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create fillet");
    sToolTipText    = QT_TR_NOOP("Create a fillet between two lines or at a coincident point");
    sWhatsThis      = "Sketcher_CreateFillet";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreateFillet";
    sAccel          = "G, F, F";
    eType           = ForEdit;
}

void CmdSketcherCreateFillet::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerFillet(DrawSketchHandlerFillet::SimpleFillet));
}

bool CmdSketcherCreateFillet::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// ======================================================================================

DEF_STD_CMD_A(CmdSketcherCreatePointFillet)

CmdSketcherCreatePointFillet::CmdSketcherCreatePointFillet()
  : Command("Sketcher_CreatePointFillet")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create corner-preserving fillet");
    sToolTipText    = QT_TR_NOOP("Fillet that preserves intersection point and most constraints");
    sWhatsThis      = "Sketcher_CreatePointFillet";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreatePointFillet";
    sAccel          = "G, F, P";
    eType           = ForEdit;
}

void CmdSketcherCreatePointFillet::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerFillet(DrawSketchHandlerFillet::ConstraintPreservingFillet));
}

bool CmdSketcherCreatePointFillet::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

/// @brief Macro that declares a new sketcher command class 'CmdSketcherCompCreateFillets'
DEF_STD_CMD_ACLU(CmdSketcherCompCreateFillets)

/**
 * @brief ctor
 */
CmdSketcherCompCreateFillets::CmdSketcherCompCreateFillets()
  : Command("Sketcher_CompCreateFillets")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Fillets");
    sToolTipText    = QT_TR_NOOP("Create a fillet between two lines");
    sWhatsThis      = "Sketcher_CompCreateFillets";
    sStatusTip      = sToolTipText;
    eType           = ForEdit;
}

/**
 * @brief Instantiates the fillet handler when the fillet command activated
 * @param int iMsg
 */
void CmdSketcherCompCreateFillets::activated(int iMsg)
{
    if (iMsg == 0) {
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerFillet(DrawSketchHandlerFillet::SimpleFillet));
    } else if (iMsg == 1) {
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerFillet(DrawSketchHandlerFillet::ConstraintPreservingFillet));
    } else {
        return;
    }

    // Since the default icon is reset when enabling/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action * CmdSketcherCompCreateFillets::createAction()
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* oldFillet = pcAction->addAction(QString());
    oldFillet->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateFillet"));

    QAction* pointFillet = pcAction->addAction(QString());
    pointFillet->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreatePointFillet"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateFillet"));
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdSketcherCompCreateFillets::updateAction(int mode)
{
    Q_UNUSED(mode);
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(getAction());
    if (!pcAction)
        return;

    QList<QAction*> a = pcAction->actions();
    int index = pcAction->property("defaultAction").toInt();
    a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateFillet"));
    a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreatePointFillet"));
    getAction()->setIcon(a[index]->icon());
}

void CmdSketcherCompCreateFillets::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* oldFillet = a[0];
    oldFillet->setText(QApplication::translate("CmdSketcherCompCreateFillets","Sketch fillet"));
    oldFillet->setToolTip(QApplication::translate("Sketcher_CreateFillet","Creates a radius between two lines"));
    oldFillet->setStatusTip(QApplication::translate("Sketcher_CreateFillet","Creates a radius between two lines"));
    QAction* pointFillet = a[1];
    pointFillet->setText(QApplication::translate("CmdSketcherCompCreateFillets","Constraint-preserving sketch fillet"));
    pointFillet->setToolTip(QApplication::translate("Sketcher_CreatePointFillet","Fillet that preserves constraints and intersection point"));
    pointFillet->setStatusTip(QApplication::translate("Sketcher_CreatePointFillet","Fillet that preserves constraints and intersection point"));
}

bool CmdSketcherCompCreateFillets::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// ======================================================================================

DEF_STD_CMD_A(CmdSketcherTrimming)

CmdSketcherTrimming::CmdSketcherTrimming()
  : Command("Sketcher_Trimming")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Trim edge");
    sToolTipText    = QT_TR_NOOP("Trim an edge with respect to the picked position");
    sWhatsThis      = "Sketcher_Trimming";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_Trimming";
    sAccel          = "G, T";
    eType           = ForEdit;
}

void CmdSketcherTrimming::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerTrimming());
}

bool CmdSketcherTrimming::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}


// ======================================================================================

DEF_STD_CMD_A(CmdSketcherExtend)

//TODO: fix the translations for this
CmdSketcherExtend::CmdSketcherExtend()
  : Command("Sketcher_Extend")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Extend edge");
    sToolTipText    = QT_TR_NOOP("Extend an edge with respect to the picked position");
    sWhatsThis      = "Sketcher_Extend";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_Extend";
    sAccel          = "G, Q";
    eType           = ForEdit;
}

void CmdSketcherExtend::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerExtend());
}

bool CmdSketcherExtend::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}


// ======================================================================================

DEF_STD_CMD_A(CmdSketcherSplit)

//TODO: fix the translations for this
CmdSketcherSplit::CmdSketcherSplit()
  : Command("Sketcher_Split")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Split edge");
    sToolTipText    = QT_TR_NOOP("Splits an edge into two while preserving constraints");
    sWhatsThis      = "Sketcher_Split";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_Split";
    sAccel          = "G, Z";
    eType           = ForEdit;
}

void CmdSketcherSplit::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerSplitting());
}

bool CmdSketcherSplit::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

DEF_STD_CMD_A(CmdSketcherExternal)

CmdSketcherExternal::CmdSketcherExternal()
  : Command("Sketcher_External")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("External geometry");
    sToolTipText    = QT_TR_NOOP("Create an edge linked to an external geometry");
    sWhatsThis      = "Sketcher_External";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_External";
    sAccel          = "G, X";
    eType           = ForEdit;
}

void CmdSketcherExternal::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerExternal());
}

bool CmdSketcherExternal::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// ======================================================================================

DEF_STD_CMD_AU(CmdSketcherCarbonCopy)

CmdSketcherCarbonCopy::CmdSketcherCarbonCopy()
: Command("Sketcher_CarbonCopy")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Carbon copy");
    sToolTipText    = QT_TR_NOOP("Copies the geometry of another sketch");
    sWhatsThis      = "Sketcher_CarbonCopy";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CarbonCopy";
    sAccel          = "G, W";
    eType           = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCarbonCopy,"Sketcher_CarbonCopy")

void CmdSketcherCarbonCopy::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerCarbonCopy());
}

bool CmdSketcherCarbonCopy::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}


/**
 * Create Slot
 */

DEF_STD_CMD_AU(CmdSketcherCreateSlot)

CmdSketcherCreateSlot::CmdSketcherCreateSlot()
    : Command("Sketcher_CreateSlot")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Create slot");
    sToolTipText = QT_TR_NOOP("Create a slot in the sketch");
    sWhatsThis = "Sketcher_CreateSlot";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateSlot";
    sAccel  = "G, S";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateSlot,"Sketcher_CreateSlot")

void CmdSketcherCreateSlot::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerSlot());
}

bool CmdSketcherCreateSlot::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

/* Create Regular Polygon ==============================================*/

DEF_STD_CMD_AU(CmdSketcherCreateTriangle)

CmdSketcherCreateTriangle::CmdSketcherCreateTriangle()
  : Command("Sketcher_CreateTriangle")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create equilateral triangle");
    sToolTipText    = QT_TR_NOOP("Create an equilateral triangle in the sketch");
    sWhatsThis      = "Sketcher_CreateTriangle";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreateTriangle";
    sAccel          = "G, P, 3";
    eType           = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateTriangle,"Sketcher_CreateTriangle")

void CmdSketcherCreateTriangle::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerRegularPolygon(3) );
}

bool CmdSketcherCreateTriangle::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

DEF_STD_CMD_AU(CmdSketcherCreateSquare)

CmdSketcherCreateSquare::CmdSketcherCreateSquare()
  : Command("Sketcher_CreateSquare")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create square");
    sToolTipText    = QT_TR_NOOP("Create a square in the sketch");
    sWhatsThis      = "Sketcher_CreateSquare";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreateSquare";
    sAccel          = "G, P, 4";
    eType           = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateSquare,"Sketcher_CreateSquare")

void CmdSketcherCreateSquare::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerRegularPolygon(4) );
}

bool CmdSketcherCreateSquare::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

DEF_STD_CMD_AU(CmdSketcherCreatePentagon)

CmdSketcherCreatePentagon::CmdSketcherCreatePentagon()
  : Command("Sketcher_CreatePentagon")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create pentagon");
    sToolTipText    = QT_TR_NOOP("Create a pentagon in the sketch");
    sWhatsThis      = "Sketcher_CreatePentagon";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreatePentagon";
    sAccel          = "G, P, 5";
    eType           = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreatePentagon,"Sketcher_CreatePentagon")

void CmdSketcherCreatePentagon::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerRegularPolygon(5) );
}

bool CmdSketcherCreatePentagon::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}


DEF_STD_CMD_AU(CmdSketcherCreateHexagon)

CmdSketcherCreateHexagon::CmdSketcherCreateHexagon()
  : Command("Sketcher_CreateHexagon")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create hexagon");
    sToolTipText    = QT_TR_NOOP("Create a hexagon in the sketch");
    sWhatsThis      = "Sketcher_CreateHexagon";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreateHexagon";
    sAccel          = "G, P, 6";
    eType           = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateHexagon,"Sketcher_CreateHexagon")

void CmdSketcherCreateHexagon::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerRegularPolygon(6) );
}

bool CmdSketcherCreateHexagon::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

DEF_STD_CMD_AU(CmdSketcherCreateHeptagon)

CmdSketcherCreateHeptagon::CmdSketcherCreateHeptagon()
  : Command("Sketcher_CreateHeptagon")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create heptagon");
    sToolTipText    = QT_TR_NOOP("Create a heptagon in the sketch");
    sWhatsThis      = "Sketcher_CreateHeptagon";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreateHeptagon";
    sAccel          = "G, P, 7";
    eType           = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateHeptagon,"Sketcher_CreateHeptagon")

void CmdSketcherCreateHeptagon::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerRegularPolygon(7) );
}

bool CmdSketcherCreateHeptagon::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

DEF_STD_CMD_AU(CmdSketcherCreateOctagon)

CmdSketcherCreateOctagon::CmdSketcherCreateOctagon()
  : Command("Sketcher_CreateOctagon")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create octagon");
    sToolTipText    = QT_TR_NOOP("Create an octagon in the sketch");
    sWhatsThis      = "Sketcher_CreateOctagon";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreateOctagon";
    sAccel          = "G, P, 8";
    eType           = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateOctagon,"Sketcher_CreateOctagon")

void CmdSketcherCreateOctagon::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerRegularPolygon(8) );
}

bool CmdSketcherCreateOctagon::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

DEF_STD_CMD_AU(CmdSketcherCreateRegularPolygon)

CmdSketcherCreateRegularPolygon::CmdSketcherCreateRegularPolygon()
: Command("Sketcher_CreateRegularPolygon")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create regular polygon");
    sToolTipText    = QT_TR_NOOP("Create a regular polygon in the sketch");
    sWhatsThis      = "Sketcher_CreateRegularPolygon";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreateRegularPolygon";
    sAccel          = "G, P, R";
    eType           = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateRegularPolygon,"Sketcher_CreateRegularPolygon")

void CmdSketcherCreateRegularPolygon::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // Pop-up asking for values
    SketcherRegularPolygonDialog srpd;
    if (srpd.exec() == QDialog::Accepted)
        ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerRegularPolygon(srpd.sides));
}

bool CmdSketcherCreateRegularPolygon::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

DEF_STD_CMD_ACLU(CmdSketcherCompCreateRegularPolygon)

CmdSketcherCompCreateRegularPolygon::CmdSketcherCompCreateRegularPolygon()
  : Command("Sketcher_CompCreateRegularPolygon")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create regular polygon");
    sToolTipText    = QT_TR_NOOP("Create a regular polygon in the sketcher");
    sWhatsThis      = "Sketcher_CompCreateRegularPolygon";
    sStatusTip      = sToolTipText;
    sAccel          = "G, P, P";
    eType           = ForEdit;
}

void CmdSketcherCompCreateRegularPolygon::activated(int iMsg)
{
    switch( iMsg ){
    case 0:
        ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerRegularPolygon(3)); break;
    case 1:
        ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerRegularPolygon(4)); break;
    case 2:
        ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerRegularPolygon(5)); break;
    case 3:
        ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerRegularPolygon(6)); break;
    case 4:
        ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerRegularPolygon(7)); break;
    case 5:
        ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerRegularPolygon(8)); break;
    case 6:
    {
        // Pop-up asking for values
        SketcherRegularPolygonDialog srpd;
        if (srpd.exec() == QDialog::Accepted)
            ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerRegularPolygon(srpd.sides));
    }
    break;
    default:
        return;
    }

    // Since the default icon is reset when enabling/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action * CmdSketcherCompCreateRegularPolygon::createAction()
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* triangle = pcAction->addAction(QString());
    triangle->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateTriangle"));
    QAction* square = pcAction->addAction(QString());
    square->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateSquare"));
    QAction* pentagon = pcAction->addAction(QString());
    pentagon->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreatePentagon"));
    QAction* hexagon = pcAction->addAction(QString());
    hexagon->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateHexagon"));
    QAction* heptagon = pcAction->addAction(QString());
    heptagon->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateHeptagon"));
    QAction* octagon = pcAction->addAction(QString());
    octagon->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateOctagon"));
    QAction* regular = pcAction->addAction(QString());
    regular->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRegularPolygon"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(hexagon->icon());
    int defaultId = 3;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdSketcherCompCreateRegularPolygon::updateAction(int mode)
{
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(getAction());
    if (!pcAction)
        return;

    QList<QAction*> a = pcAction->actions();
    int index = pcAction->property("defaultAction").toInt();
    switch (mode) {
    case Normal:
        a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateTriangle"));
        a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateSquare"));
        a[2]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreatePentagon"));
        a[3]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateHexagon"));
        a[4]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateHeptagon"));
        a[5]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateOctagon"));
        a[6]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRegularPolygon"));
        getAction()->setIcon(a[index]->icon());
        break;
    case Construction:
        a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateTriangle_Constr"));
        a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateSquare_Constr"));
        a[2]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreatePentagon_Constr"));
        a[3]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateHexagon_Constr"));
        a[4]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateHeptagon_Constr"));
        a[5]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateOctagon_Constr"));
        a[6]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRegularPolygon_Constr"));
        getAction()->setIcon(a[index]->icon());
        break;
    }
}

void CmdSketcherCompCreateRegularPolygon::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* triangle = a[0];
    triangle->setText(QApplication::translate("CmdSketcherCompCreateRegularPolygon","Triangle"));
    triangle->setToolTip(QApplication::translate("Sketcher_CreateTriangle","Create an equilateral triangle by its center and by one corner"));
    triangle->setStatusTip(QApplication::translate("Sketcher_CreateTriangle","Create an equilateral triangle by its center and by one corner"));
    QAction* square = a[1];
    square->setText(QApplication::translate("CmdSketcherCompCreateRegularPolygon","Square"));
    square->setToolTip(QApplication::translate("Sketcher_CreateSquare","Create a square by its center and by one corner"));
    square->setStatusTip(QApplication::translate("Sketcher_CreateSquare","Create a square by its center and by one corner"));
    QAction* pentagon = a[2];
    pentagon->setText(QApplication::translate("CmdSketcherCompCreateRegularPolygon","Pentagon"));
    pentagon->setToolTip(QApplication::translate("Sketcher_CreatePentagon","Create a pentagon by its center and by one corner"));
    pentagon->setStatusTip(QApplication::translate("Sketcher_CreatePentagon","Create a pentagon by its center and by one corner"));
    QAction* hexagon = a[3];
    hexagon->setText(QApplication::translate("CmdSketcherCompCreateRegularPolygon","Hexagon"));
    hexagon->setToolTip(QApplication::translate("Sketcher_CreateHexagon","Create a hexagon by its center and by one corner"));
    hexagon->setStatusTip(QApplication::translate("Sketcher_CreateHexagon","Create a hexagon by its center and by one corner"));
    QAction* heptagon = a[4];
    heptagon->setText(QApplication::translate("CmdSketcherCompCreateRegularPolygon","Heptagon"));
    heptagon->setToolTip(QApplication::translate("Sketcher_CreateHeptagon","Create a heptagon by its center and by one corner"));
    heptagon->setStatusTip(QApplication::translate("Sketcher_CreateHeptagon","Create a heptagon by its center and by one corner"));
    QAction* octagon = a[5];
    octagon->setText(QApplication::translate("CmdSketcherCompCreateRegularPolygon","Octagon"));
    octagon->setToolTip(QApplication::translate("Sketcher_CreateOctagon","Create an octagon by its center and by one corner"));
    octagon->setStatusTip(QApplication::translate("Sketcher_CreateOctagon","Create an octagon by its center and by one corner"));
    QAction* regular = a[6];
    regular->setText(QApplication::translate("CmdSketcherCompCreateRegularPolygon","Regular polygon"));
    regular->setToolTip(QApplication::translate("Sketcher_CreateOctagon","Create a regular polygon by its center and by one corner"));
    regular->setStatusTip(QApplication::translate("Sketcher_CreateOctagon","Create a regular polygon by its center and by one corner"));
}

bool CmdSketcherCompCreateRegularPolygon::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

void CreateSketcherCommandsCreateGeo()
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdSketcherCreatePoint());
    rcCmdMgr.addCommand(new CmdSketcherCreateArc());
    rcCmdMgr.addCommand(new CmdSketcherCreate3PointArc());
    rcCmdMgr.addCommand(new CmdSketcherCompCreateArc());
    rcCmdMgr.addCommand(new CmdSketcherCreateCircle());
    rcCmdMgr.addCommand(new CmdSketcherCreate3PointCircle());
    rcCmdMgr.addCommand(new CmdSketcherCompCreateCircle());
    rcCmdMgr.addCommand(new CmdSketcherCreateEllipseByCenter());
    rcCmdMgr.addCommand(new CmdSketcherCreateEllipseBy3Points());
    rcCmdMgr.addCommand(new CmdSketcherCompCreateConic());
    rcCmdMgr.addCommand(new CmdSketcherCreateArcOfEllipse());
    rcCmdMgr.addCommand(new CmdSketcherCreateArcOfHyperbola());
    rcCmdMgr.addCommand(new CmdSketcherCreateArcOfParabola());
    rcCmdMgr.addCommand(new CmdSketcherCreateBSpline());
    rcCmdMgr.addCommand(new CmdSketcherCreatePeriodicBSpline());
    rcCmdMgr.addCommand(new CmdSketcherCompCreateBSpline());
    rcCmdMgr.addCommand(new CmdSketcherCreateLine());
    rcCmdMgr.addCommand(new CmdSketcherCreatePolyline());
    rcCmdMgr.addCommand(new CmdSketcherCreateRectangle());
    rcCmdMgr.addCommand(new CmdSketcherCreateRectangleCenter());
    rcCmdMgr.addCommand(new CmdSketcherCreateOblong());
    rcCmdMgr.addCommand(new CmdSketcherCompCreateRegularPolygon());
    rcCmdMgr.addCommand(new CmdSketcherCreateTriangle());
    rcCmdMgr.addCommand(new CmdSketcherCreateSquare());
    rcCmdMgr.addCommand(new CmdSketcherCreatePentagon());
    rcCmdMgr.addCommand(new CmdSketcherCreateHexagon());
    rcCmdMgr.addCommand(new CmdSketcherCreateHeptagon());
    rcCmdMgr.addCommand(new CmdSketcherCreateOctagon());
    rcCmdMgr.addCommand(new CmdSketcherCreateRegularPolygon());
    rcCmdMgr.addCommand(new CmdSketcherCompCreateRectangles());
    rcCmdMgr.addCommand(new CmdSketcherCreateSlot());
    rcCmdMgr.addCommand(new CmdSketcherCompCreateFillets());
    rcCmdMgr.addCommand(new CmdSketcherCreateFillet());
    rcCmdMgr.addCommand(new CmdSketcherCreatePointFillet());
    //rcCmdMgr.addCommand(new CmdSketcherCreateText());
    //rcCmdMgr.addCommand(new CmdSketcherCreateDraftLine());
    rcCmdMgr.addCommand(new CmdSketcherTrimming());
    rcCmdMgr.addCommand(new CmdSketcherExtend());
    rcCmdMgr.addCommand(new CmdSketcherSplit());
    rcCmdMgr.addCommand(new CmdSketcherExternal());
    rcCmdMgr.addCommand(new CmdSketcherCarbonCopy());
}
