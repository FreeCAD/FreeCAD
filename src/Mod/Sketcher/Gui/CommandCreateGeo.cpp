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
#include <cstdlib>
#include <memory>

#include <QApplication>
#include <QString>
#endif

#include <App/Datums.h>
#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/SelectionFilter.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/Window.h>
#include <Mod/Part/App/DatumFeature.h>
#include <Mod/Part/App/Geometry2d.h>
#include <Mod/Sketcher/App/Constraint.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "CircleEllipseConstructionMethod.h"
#include "GeometryCreationMode.h"
#include "Utils.h"
#include "ViewProviderSketch.h"

// DrawSketchHandler* must be last includes
#include "DrawSketchHandler.h"
#include "DrawSketchHandlerArc.h"
#include "DrawSketchHandlerArcOfEllipse.h"
#include "DrawSketchHandlerArcOfHyperbola.h"
#include "DrawSketchHandlerArcOfParabola.h"
#include "DrawSketchHandlerArcSlot.h"
#include "DrawSketchHandlerBSpline.h"
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

#define CONSTRUCTION_UPDATE_ACTION(CLASS, ICON)                                                    \
    /* This macro creates an updateAction() function that will toggle between normal &             \
     * construction icon */                                                                        \
    void CLASS::updateAction(int mode)                                                             \
    {                                                                                              \
        auto act = getAction();                                                                    \
        if (act) {                                                                                 \
            switch (static_cast<GeometryCreationMode>(mode)) {                                     \
                case GeometryCreationMode::Normal:                                                 \
                    act->setIcon(Gui::BitmapFactory().iconFromTheme(ICON));                        \
                    break;                                                                         \
                case GeometryCreationMode::Construction:                                           \
                    act->setIcon(Gui::BitmapFactory().iconFromTheme(ICON "_Constr"));              \
                    break;                                                                         \
            }                                                                                      \
        }                                                                                          \
    }

namespace SketcherGui
{
GeometryCreationMode geometryCreationMode = GeometryCreationMode::Normal;
}

/* Sketch commands =======================================================*/

// Point ================================================================

DEF_STD_CMD_A(CmdSketcherCreatePoint)

CmdSketcherCreatePoint::CmdSketcherCreatePoint()
    : Command("Sketcher_CreatePoint")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Point");
    sToolTipText = QT_TR_NOOP("Creates a point");
    sWhatsThis = "Sketcher_CreatePoint";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreatePoint";
    sAccel = "G, Y";
    eType = ForEdit;
}

void CmdSketcherCreatePoint::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), std::make_unique<DrawSketchHandlerPoint>());
}

bool CmdSketcherCreatePoint::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// ======================================================================================
// Comp for line tools =============================================

class CmdSketcherCompLine: public Gui::GroupCommand
{
public:
    CmdSketcherCompLine()
        : GroupCommand("Sketcher_CompLine")
    {
        sAppModule = "Sketcher";
        sGroup = "Sketcher";
        sMenuText = QT_TR_NOOP("Polyline");
        sToolTipText = QT_TR_NOOP("Creates a continuous polyline");
        sWhatsThis = "Sketcher_CompLine";
        sStatusTip = sToolTipText;
        sAccel = "G, M";
        eType = ForEdit;

        setCheckable(false);

        addCommand("Sketcher_CreatePolyline");
        addCommand("Sketcher_CreateLine");
    }

    void updateAction(int mode) override
    {
        Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(getAction());
        if (!pcAction) {
            return;
        }

        QList<QAction*> al = pcAction->actions();
        int index = pcAction->property("defaultAction").toInt();
        switch (static_cast<GeometryCreationMode>(mode)) {
            case GeometryCreationMode::Normal:
                al[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreatePolyline"));
                al[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateLine"));
                getAction()->setIcon(al[index]->icon());
                break;
            case GeometryCreationMode::Construction:
                al[0]->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Sketcher_CreatePolyline_Constr"));
                al[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateLine_Constr"));
                getAction()->setIcon(al[index]->icon());
                break;
        }
    }

    const char* className() const override
    {
        return "CmdSketcherCompLine";
    }

    bool isActive() override
    {
        return isCommandActive(getActiveGuiDocument());
    }
};

// Line ================================================================

DEF_STD_CMD_AU(CmdSketcherCreateLine)

CmdSketcherCreateLine::CmdSketcherCreateLine()
    : Command("Sketcher_CreateLine")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Line");
    sToolTipText = QT_TR_NOOP("Creates a line");
    sWhatsThis = "Sketcher_CreateLine";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateLine";
    sAccel = "G, L";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateLine, "Sketcher_CreateLine")

void CmdSketcherCreateLine::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), std::make_unique<DrawSketchHandlerLine>());
}

bool CmdSketcherCreateLine::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// Polyline ================================================================

DEF_STD_CMD_AU(CmdSketcherCreatePolyline)

CmdSketcherCreatePolyline::CmdSketcherCreatePolyline()
    : Command("Sketcher_CreatePolyline")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Polyline");
    sToolTipText =
        QT_TR_NOOP("Creates a continuous polyline. Press the 'M' key to switch segment modes");
    sWhatsThis = "Sketcher_CreatePolyline";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreatePolyline";
    sAccel = "G, M";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreatePolyline, "Sketcher_CreatePolyline")

void CmdSketcherCreatePolyline::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), std::make_unique<DrawSketchHandlerLineSet>());
}

bool CmdSketcherCreatePolyline::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// ======================================================================================
// Comp for arc tools =============================================

class CmdSketcherCompCreateArc: public Gui::GroupCommand
{
public:
    CmdSketcherCompCreateArc()
        : GroupCommand("Sketcher_CompCreateArc")
    {
        sAppModule = "Sketcher";
        sGroup = "Sketcher";
        sMenuText = QT_TR_NOOP("Arc");
        sToolTipText = QT_TR_NOOP("Creates an arc");
        sWhatsThis = "Sketcher_CompCreateArc";
        sStatusTip = sToolTipText;
        sAccel = "G, A";
        eType = ForEdit;

        setCheckable(false);
        // setRememberLast(true);

        addCommand("Sketcher_CreateArc");
        addCommand("Sketcher_Create3PointArc");
        addCommand("Sketcher_CreateArcOfEllipse");
        addCommand("Sketcher_CreateArcOfHyperbola");
        addCommand("Sketcher_CreateArcOfParabola");
    }

    void updateAction(int mode) override
    {
        Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(getAction());
        if (!pcAction) {
            return;
        }

        QList<QAction*> al = pcAction->actions();
        int index = pcAction->property("defaultAction").toInt();
        switch (static_cast<GeometryCreationMode>(mode)) {
            case GeometryCreationMode::Normal:
                al[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateArc"));
                al[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Create3PointArc"));
                al[2]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateElliptical_Arc"));
                al[3]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateHyperbolic_Arc"));
                al[4]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateParabolic_Arc"));
                getAction()->setIcon(al[index]->icon());
                break;
            case GeometryCreationMode::Construction:
                al[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateArc_Constr"));
                al[1]->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Sketcher_Create3PointArc_Constr"));
                al[2]->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Sketcher_CreateElliptical_Arc_Constr"));
                al[3]->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Sketcher_CreateHyperbolic_Arc_Constr"));
                al[4]->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Sketcher_CreateParabolic_Arc_Constr"));
                getAction()->setIcon(al[index]->icon());
                break;
        }
    }
    const char* className() const override
    {
        return "CmdSketcherCompCreateArc";
    }
    bool isActive() override
    {
        return isCommandActive(getActiveGuiDocument());
    }
};

// Arc by center ================================================================

DEF_STD_CMD_AU(CmdSketcherCreateArc)

CmdSketcherCreateArc::CmdSketcherCreateArc()
    : Command("Sketcher_CreateArc")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Arc From Center");
    sToolTipText = QT_TR_NOOP("Creates an arc defined by a center point and an end point");
    sWhatsThis = "Sketcher_CreateArc";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateArc";
    sAccel = "G, A";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateArc, "Sketcher_CreateArc")

void CmdSketcherCreateArc::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), std::make_unique<DrawSketchHandlerArc>());
}

bool CmdSketcherCreateArc::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}


// Arc by 3 points ================================================================

DEF_STD_CMD_AU(CmdSketcherCreate3PointArc)

CmdSketcherCreate3PointArc::CmdSketcherCreate3PointArc()
    : Command("Sketcher_Create3PointArc")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Arc From 3 Points");
    sToolTipText = QT_TR_NOOP("Creates an arc defined by 2 end points and 1 point on the arc");
    sWhatsThis = "Sketcher_Create3PointArc";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_Create3PointArc";
    sAccel = "G, 3, A";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreate3PointArc, "Sketcher_Create3PointArc")

void CmdSketcherCreate3PointArc::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),
                    std::make_unique<DrawSketchHandlerArc>(
                        ConstructionMethods::CircleEllipseConstructionMethod::ThreeRim));
}

bool CmdSketcherCreate3PointArc::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// Arc of ellipse ================================================================

DEF_STD_CMD_AU(CmdSketcherCreateArcOfEllipse)

CmdSketcherCreateArcOfEllipse::CmdSketcherCreateArcOfEllipse()
    : Command("Sketcher_CreateArcOfEllipse")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Elliptical Arc");
    sToolTipText = QT_TR_NOOP("Creates an elliptical arc");
    sWhatsThis = "Sketcher_CreateArcOfEllipse";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateElliptical_Arc";
    sAccel = "G, E, A";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateArcOfEllipse, "Sketcher_CreateElliptical_Arc")

void CmdSketcherCreateArcOfEllipse::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), std::make_unique<DrawSketchHandlerArcOfEllipse>());
}

bool CmdSketcherCreateArcOfEllipse::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// Arc of hyperbola ================================================================

DEF_STD_CMD_AU(CmdSketcherCreateArcOfHyperbola)

CmdSketcherCreateArcOfHyperbola::CmdSketcherCreateArcOfHyperbola()
    : Command("Sketcher_CreateArcOfHyperbola")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Hyperbolic Arc");
    sToolTipText = QT_TR_NOOP("Creates a hyperbolic arc");
    sWhatsThis = "Sketcher_CreateArcOfHyperbola";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateHyperbolic_Arc";
    sAccel = "G, H";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateArcOfHyperbola, "Sketcher_CreateHyperbolic_Arc")

void CmdSketcherCreateArcOfHyperbola::activated(int /*iMsg*/)
{
    ActivateHandler(getActiveGuiDocument(), std::make_unique<DrawSketchHandlerArcOfHyperbola>());
}

bool CmdSketcherCreateArcOfHyperbola::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// Arc of parabola ================================================================

DEF_STD_CMD_AU(CmdSketcherCreateArcOfParabola)

CmdSketcherCreateArcOfParabola::CmdSketcherCreateArcOfParabola()
    : Command("Sketcher_CreateArcOfParabola")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Parabolic Arc");
    sToolTipText = QT_TR_NOOP("Creates a parabolic arc");
    sWhatsThis = "Sketcher_CreateArcOfParabola";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateParabolic_Arc";
    sAccel = "G, J";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateArcOfParabola, "Sketcher_CreateParabolic_Arc")

void CmdSketcherCreateArcOfParabola::activated(int /*iMsg*/)
{
    ActivateHandler(getActiveGuiDocument(), std::make_unique<DrawSketchHandlerArcOfParabola>());
}

bool CmdSketcherCreateArcOfParabola::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// ======================================================================================
// Comp for conic tools =============================================

class CmdSketcherCompCreateConic: public Gui::GroupCommand
{
public:
    CmdSketcherCompCreateConic()
        : GroupCommand("Sketcher_CompCreateConic")
    {
        sAppModule = "Sketcher";
        sGroup = "Sketcher";
        sMenuText = QT_TR_NOOP("Conic");
        sToolTipText = QT_TR_NOOP("Creates a conic");
        sWhatsThis = "Sketcher_CompCreateConic";
        sStatusTip = sToolTipText;
        sAccel = "G, C";
        eType = ForEdit;

        setCheckable(false);
        setRememberLast(true);

        addCommand("Sketcher_CreateCircle");
        addCommand("Sketcher_Create3PointCircle");
        addCommand("Sketcher_CreateEllipseByCenter");
        addCommand("Sketcher_CreateEllipseBy3Points");
    }

    void updateAction(int mode) override
    {
        Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(getAction());
        if (!pcAction) {
            return;
        }

        QList<QAction*> al = pcAction->actions();
        int index = pcAction->property("defaultAction").toInt();
        switch (static_cast<GeometryCreationMode>(mode)) {
            case GeometryCreationMode::Normal:
                al[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateCircle"));
                al[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Create3PointCircle"));
                al[2]->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Sketcher_CreateEllipseByCenter"));
                al[3]->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Sketcher_CreateEllipse_3points"));
                getAction()->setIcon(al[index]->icon());
                break;
            case GeometryCreationMode::Construction:
                al[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateCircle_Constr"));
                al[1]->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Sketcher_Create3PointCircle_Constr"));
                al[2]->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Sketcher_CreateEllipseByCenter_Constr"));
                al[3]->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Sketcher_CreateEllipse_3points_Constr"));
                getAction()->setIcon(al[index]->icon());
                break;
        }
    }
    const char* className() const override
    {
        return "CmdSketcherCompCreateConic";
    }
    bool isActive() override
    {
        return isCommandActive(getActiveGuiDocument());
    }
};

// Circle by center ================================================================

DEF_STD_CMD_AU(CmdSketcherCreateCircle)

CmdSketcherCreateCircle::CmdSketcherCreateCircle()
    : Command("Sketcher_CreateCircle")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Circle From Center");
    sToolTipText = QT_TR_NOOP("Creates a circle from a center and rim point");
    sWhatsThis = "Sketcher_CreateCircle";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateCircle";
    sAccel = "G, C";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateCircle, "Sketcher_CreateCircle")

void CmdSketcherCreateCircle::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), std::make_unique<DrawSketchHandlerCircle>());
}

bool CmdSketcherCreateCircle::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// Circle by 3 points ================================================================

DEF_STD_CMD_AU(CmdSketcherCreate3PointCircle)

CmdSketcherCreate3PointCircle::CmdSketcherCreate3PointCircle()
    : Command("Sketcher_Create3PointCircle")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Circle From 3 Points");
    sToolTipText = QT_TR_NOOP("Creates a circle from 3 perimeter points");
    sWhatsThis = "Sketcher_Create3PointCircle";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_Create3PointCircle";
    sAccel = "G, 3, C";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreate3PointCircle, "Sketcher_Create3PointCircle")

void CmdSketcherCreate3PointCircle::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),
                    std::make_unique<DrawSketchHandlerCircle>(
                        ConstructionMethods::CircleEllipseConstructionMethod::ThreeRim));
}

bool CmdSketcherCreate3PointCircle::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// Ellipse by center ================================================================

/// @brief Macro that declares a new sketcher command class 'CmdSketcherCreateEllipseByCenter'
DEF_STD_CMD_AU(CmdSketcherCreateEllipseByCenter)

/**
 * @brief ctor
 */
CmdSketcherCreateEllipseByCenter::CmdSketcherCreateEllipseByCenter()
    : Command("Sketcher_CreateEllipseByCenter")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Ellipse From Center");
    sToolTipText = QT_TR_NOOP("Creates an ellipse from a center and rim point");
    sWhatsThis = "Sketcher_CreateEllipseByCenter";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateEllipseByCenter";
    sAccel = "G, E, E";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateEllipseByCenter, "Sketcher_CreateEllipseByCenter")

void CmdSketcherCreateEllipseByCenter::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), std::make_unique<DrawSketchHandlerEllipse>());
}

bool CmdSketcherCreateEllipseByCenter::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// Ellipse by 3 points ================================================================

/// @brief Macro that declares a new sketcher command class 'CmdSketcherCreateEllipseBy3Points'
DEF_STD_CMD_AU(CmdSketcherCreateEllipseBy3Points)

/**
 * @brief ctor
 */
CmdSketcherCreateEllipseBy3Points::CmdSketcherCreateEllipseBy3Points()
    : Command("Sketcher_CreateEllipseBy3Points")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Ellipse From 3 Points");
    sToolTipText = QT_TR_NOOP("Creates an ellipse from 3 points on its perimeter");
    sWhatsThis = "Sketcher_CreateEllipseBy3Points";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateEllipse_3points";
    sAccel = "G, 3, E";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateEllipseBy3Points, "Sketcher_CreateEllipse_3points")

void CmdSketcherCreateEllipseBy3Points::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),
                    std::make_unique<DrawSketchHandlerEllipse>(
                        ConstructionMethods::CircleEllipseConstructionMethod::ThreeRim));
}

bool CmdSketcherCreateEllipseBy3Points::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// ======================================================================================
// Comp for rectangle tools =============================================

class CmdSketcherCompCreateRectangles: public Gui::GroupCommand
{
public:
    CmdSketcherCompCreateRectangles()
        : GroupCommand("Sketcher_CompCreateRectangles")
    {
        sAppModule = "Sketcher";
        sGroup = "Sketcher";
        sMenuText = QT_TR_NOOP("Rectangle");
        sToolTipText = QT_TR_NOOP("Creates a rectangle");
        sWhatsThis = "Sketcher_CompCreateRectangles";
        sStatusTip = sToolTipText;
        sAccel = "G, R";
        eType = ForEdit;

        setCheckable(false);
        // setRememberLast(true);

        addCommand("Sketcher_CreateRectangle");
        addCommand("Sketcher_CreateRectangle_Center");
        addCommand("Sketcher_CreateOblong");
    }

    void updateAction(int mode) override
    {
        Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(getAction());
        if (!pcAction) {
            return;
        }

        QList<QAction*> al = pcAction->actions();
        int index = pcAction->property("defaultAction").toInt();
        switch (static_cast<GeometryCreationMode>(mode)) {
            case GeometryCreationMode::Normal:
                al[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangle"));
                al[1]->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangle_Center"));
                al[2]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateOblong"));
                getAction()->setIcon(al[index]->icon());
                break;
            case GeometryCreationMode::Construction:
                al[0]->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangle_Constr"));
                al[1]->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangle_Center_Constr"));
                al[2]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateOblong_Constr"));
                getAction()->setIcon(al[index]->icon());
                break;
        }
    }
    const char* className() const override
    {
        return "CmdSketcherCompCreateRectangles";
    }
    bool isActive() override
    {
        return isCommandActive(getActiveGuiDocument());
    }
};

// Rectangle ================================================================

DEF_STD_CMD_AU(CmdSketcherCreateRectangle)

CmdSketcherCreateRectangle::CmdSketcherCreateRectangle()
    : Command("Sketcher_CreateRectangle")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Rectangle");
    sToolTipText = QT_TR_NOOP("Creates a rectangle from 2 corner points");
    sWhatsThis = "Sketcher_CreateRectangle";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateRectangle";
    sAccel = "G, R";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateRectangle, "Sketcher_CreateRectangle")

void CmdSketcherCreateRectangle::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),
                    std::make_unique<DrawSketchHandlerRectangle>(
                        ConstructionMethods::RectangleConstructionMethod::Diagonal));
}

bool CmdSketcherCreateRectangle::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// Rectangle by center ================================================================

DEF_STD_CMD_AU(CmdSketcherCreateRectangleCenter)

CmdSketcherCreateRectangleCenter::CmdSketcherCreateRectangleCenter()
    : Command("Sketcher_CreateRectangle_Center")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Centered Rectangle");
    sToolTipText = QT_TR_NOOP("Creates a centered rectangle from a center and a corner point");
    sWhatsThis = "Sketcher_CreateRectangle_Center";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateRectangle_Center";
    sAccel = "G, V";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateRectangleCenter, "Sketcher_CreateRectangle_Center")

void CmdSketcherCreateRectangleCenter::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),
                    std::make_unique<DrawSketchHandlerRectangle>(
                        ConstructionMethods::RectangleConstructionMethod::CenterAndCorner));
}

bool CmdSketcherCreateRectangleCenter::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}


// Rounded rectangle ===============================================================

DEF_STD_CMD_AU(CmdSketcherCreateOblong)

CmdSketcherCreateOblong::CmdSketcherCreateOblong()
    : Command("Sketcher_CreateOblong")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Rounded Rectangle");
    sToolTipText = QT_TR_NOOP("Creates a rounded rectangle from 2 corner points");
    sWhatsThis = "Sketcher_CreateOblong";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateOblong";
    sAccel = "G, O";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateOblong, "Sketcher_CreateOblong")

void CmdSketcherCreateOblong::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),
                    std::make_unique<DrawSketchHandlerRectangle>(
                        ConstructionMethods::RectangleConstructionMethod::Diagonal,
                        true));
}

bool CmdSketcherCreateOblong::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// ======================================================================================
// Comp for polygon tools =============================================

class CmdSketcherCompCreateRegularPolygon: public Gui::GroupCommand
{
public:
    CmdSketcherCompCreateRegularPolygon()
        : GroupCommand("Sketcher_CompCreateRegularPolygon")
    {
        sAppModule = "Sketcher";
        sGroup = "Sketcher";
        sMenuText = QT_TR_NOOP("Polygon");
        sToolTipText = QT_TR_NOOP("Creates a regular polygon from a center and corner point");
        sWhatsThis = "Sketcher_CompCreateRegularPolygon";
        sStatusTip = sToolTipText;
        sAccel = "G, P, 3";
        eType = ForEdit;

        setCheckable(false);
        // setRememberLast(true);

        addCommand("Sketcher_CreateTriangle");
        addCommand("Sketcher_CreateSquare");
        addCommand("Sketcher_CreatePentagon");
        addCommand("Sketcher_CreateHexagon");
        addCommand("Sketcher_CreateHeptagon");
        addCommand("Sketcher_CreateOctagon");
        addCommand("Sketcher_CreateRegularPolygon");
    }
    void updateAction(int mode) override
    {
        Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(getAction());
        if (!pcAction) {
            return;
        }

        QList<QAction*> al = pcAction->actions();
        int index = pcAction->property("defaultAction").toInt();
        switch (static_cast<GeometryCreationMode>(mode)) {
            case GeometryCreationMode::Normal:
                al[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateTriangle"));
                al[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateSquare"));
                al[2]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreatePentagon"));
                al[3]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateHexagon"));
                al[4]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateHeptagon"));
                al[5]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateOctagon"));
                al[6]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRegularPolygon"));

                getAction()->setIcon(al[index]->icon());
                break;
            case GeometryCreationMode::Construction:
                al[0]->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Sketcher_CreateTriangle_Constr"));
                al[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateSquare_Constr"));
                al[2]->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Sketcher_CreatePentagon_Constr"));
                al[3]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateHexagon_Constr"));
                al[4]->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Sketcher_CreateHeptagon_Constr"));
                al[5]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateOctagon_Constr"));
                al[6]->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRegularPolygon_Constr"));
                getAction()->setIcon(al[index]->icon());
                break;
        }
    }
    const char* className() const override
    {
        return "CmdSketcherCompCreateRegularPolygon";
    }
    bool isActive() override
    {
        return isCommandActive(getActiveGuiDocument());
    }
};

// Triangle ================================================================

DEF_STD_CMD_AU(CmdSketcherCreateTriangle)

CmdSketcherCreateTriangle::CmdSketcherCreateTriangle()
    : Command("Sketcher_CreateTriangle")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Triangle");
    sToolTipText = QT_TR_NOOP("Creates an equilateral triangle from a center and corner point");
    sWhatsThis = "Sketcher_CreateTriangle";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateTriangle";
    sAccel = "G, P, 3";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateTriangle, "Sketcher_CreateTriangle")

void CmdSketcherCreateTriangle::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), std::make_unique<DrawSketchHandlerPolygon>(3));
}

bool CmdSketcherCreateTriangle::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// Square ================================================================

DEF_STD_CMD_AU(CmdSketcherCreateSquare)

CmdSketcherCreateSquare::CmdSketcherCreateSquare()
    : Command("Sketcher_CreateSquare")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Square");
    sToolTipText = QT_TR_NOOP("Creates a square from a center and corner point");
    sWhatsThis = "Sketcher_CreateSquare";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateSquare";
    sAccel = "G, P, 4";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateSquare, "Sketcher_CreateSquare")

void CmdSketcherCreateSquare::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), std::make_unique<DrawSketchHandlerPolygon>(4));
}

bool CmdSketcherCreateSquare::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// Pentagon ================================================================

DEF_STD_CMD_AU(CmdSketcherCreatePentagon)

CmdSketcherCreatePentagon::CmdSketcherCreatePentagon()
    : Command("Sketcher_CreatePentagon")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Pentagon");
    sToolTipText = QT_TR_NOOP("Creates a pentagon from a center and corner point");
    sWhatsThis = "Sketcher_CreatePentagon";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreatePentagon";
    sAccel = "G, P, 5";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreatePentagon, "Sketcher_CreatePentagon")

void CmdSketcherCreatePentagon::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), std::make_unique<DrawSketchHandlerPolygon>(5));
}

bool CmdSketcherCreatePentagon::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// Hexagon ================================================================

DEF_STD_CMD_AU(CmdSketcherCreateHexagon)

CmdSketcherCreateHexagon::CmdSketcherCreateHexagon()
    : Command("Sketcher_CreateHexagon")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Hexagon");
    sToolTipText = QT_TR_NOOP("Creates a hexagon from a center and corner point");
    sWhatsThis = "Sketcher_CreateHexagon";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateHexagon";
    sAccel = "G, P, 6";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateHexagon, "Sketcher_CreateHexagon")

void CmdSketcherCreateHexagon::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), std::make_unique<DrawSketchHandlerPolygon>(6));
}

bool CmdSketcherCreateHexagon::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// Heptagon ================================================================

DEF_STD_CMD_AU(CmdSketcherCreateHeptagon)

CmdSketcherCreateHeptagon::CmdSketcherCreateHeptagon()
    : Command("Sketcher_CreateHeptagon")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Heptagon");
    sToolTipText = QT_TR_NOOP("Creates a heptagon from a center and corner point");
    sWhatsThis = "Sketcher_CreateHeptagon";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateHeptagon";
    sAccel = "G, P, 7";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateHeptagon, "Sketcher_CreateHeptagon")

void CmdSketcherCreateHeptagon::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), std::make_unique<DrawSketchHandlerPolygon>(7));
}

bool CmdSketcherCreateHeptagon::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// Octagon ================================================================

DEF_STD_CMD_AU(CmdSketcherCreateOctagon)

CmdSketcherCreateOctagon::CmdSketcherCreateOctagon()
    : Command("Sketcher_CreateOctagon")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Octagon");
    sToolTipText = QT_TR_NOOP("Creates an octagon from a center and corner point");
    sWhatsThis = "Sketcher_CreateOctagon";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateOctagon";
    sAccel = "G, P, 8";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateOctagon, "Sketcher_CreateOctagon")

void CmdSketcherCreateOctagon::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), std::make_unique<DrawSketchHandlerPolygon>(8));
}

bool CmdSketcherCreateOctagon::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// Regular polygon ================================================================

DEF_STD_CMD_AU(CmdSketcherCreateRegularPolygon)

CmdSketcherCreateRegularPolygon::CmdSketcherCreateRegularPolygon()
    : Command("Sketcher_CreateRegularPolygon")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Polygon");
    sToolTipText = QT_TR_NOOP("Creates a regular polygon from a center and corner point");
    sWhatsThis = "Sketcher_CreateRegularPolygon";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateRegularPolygon";
    sAccel = "G, P, R";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateRegularPolygon, "Sketcher_CreateRegularPolygon")

void CmdSketcherCreateRegularPolygon::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // Pop-up asking for values
    SketcherRegularPolygonDialog srpd;
    if (srpd.exec() == QDialog::Accepted) {
        ActivateHandler(getActiveGuiDocument(),
                        std::make_unique<DrawSketchHandlerPolygon>(srpd.sides));
    }
}

bool CmdSketcherCreateRegularPolygon::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// ======================================================================================
// Comp for slot tools =============================================

class CmdSketcherCompSlot: public Gui::GroupCommand
{
public:
    CmdSketcherCompSlot()
        : GroupCommand("Sketcher_CompSlot")
    {
        sAppModule = "Sketcher";
        sGroup = "Sketcher";
        sMenuText = QT_TR_NOOP("Slot");
        sToolTipText = QT_TR_NOOP("Slot tools");
        sWhatsThis = "Sketcher_CompSlot";
        sStatusTip = sToolTipText;
        sAccel = "G, S";
        eType = ForEdit;

        setCheckable(false);

        addCommand("Sketcher_CreateSlot");
        addCommand("Sketcher_CreateArcSlot");
    }

    void updateAction(int mode) override
    {
        Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(getAction());
        if (!pcAction) {
            return;
        }

        QList<QAction*> al = pcAction->actions();
        int index = pcAction->property("defaultAction").toInt();
        switch (static_cast<GeometryCreationMode>(mode)) {
            case GeometryCreationMode::Normal:
                al[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateSlot"));
                al[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateArcSlot"));
                getAction()->setIcon(al[index]->icon());
                break;
            case GeometryCreationMode::Construction:
                al[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateSlot_Constr"));
                al[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateArcSlot_Constr"));
                getAction()->setIcon(al[index]->icon());
                break;
        }
    }

    const char* className() const override
    {
        return "CmdSketcherCompSlot";
    }

    bool isActive() override
    {
        return isCommandActive(getActiveGuiDocument());
    }
};

// Slot ================================================================

DEF_STD_CMD_AU(CmdSketcherCreateSlot)

CmdSketcherCreateSlot::CmdSketcherCreateSlot()
    : Command("Sketcher_CreateSlot")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Slot");
    sToolTipText = QT_TR_NOOP("Creates a slot");
    sWhatsThis = "Sketcher_CreateSlot";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateSlot";
    sAccel = "G, S";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateSlot, "Sketcher_CreateSlot")

void CmdSketcherCreateSlot::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), std::make_unique<DrawSketchHandlerSlot>());
}

bool CmdSketcherCreateSlot::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// Arc slot ================================================================

DEF_STD_CMD_AU(CmdSketcherCreateArcSlot)

CmdSketcherCreateArcSlot::CmdSketcherCreateArcSlot()
    : Command("Sketcher_CreateArcSlot")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Arc Slot");
    sToolTipText = QT_TR_NOOP("Creates an arc slot");
    sWhatsThis = "Sketcher_CreateArcSlot";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateArcSlot";
    sAccel = "G, S, 2";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateArcSlot, "Sketcher_CreateArcSlot")

void CmdSketcherCreateArcSlot::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), std::make_unique<DrawSketchHandlerArcSlot>());
}

bool CmdSketcherCreateArcSlot::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}

// ======================================================================================
// Comp for spline tools =============================================

class CmdSketcherCompCreateBSpline: public Gui::GroupCommand
{
public:
    CmdSketcherCompCreateBSpline()
        : GroupCommand("Sketcher_CompCreateBSpline")
    {
        sAppModule = "Sketcher";
        sGroup = "Sketcher";
        sMenuText = QT_TR_NOOP("B-Spline");
        sToolTipText = QT_TR_NOOP("Creates a B-spline curve defined by control points");
        sWhatsThis = "Sketcher_CompCreateBSpline";
        sStatusTip = sToolTipText;
        sAccel = "G, B, B";
        eType = ForEdit;

        setCheckable(false);
        // setRememberLast(true);

        addCommand("Sketcher_CreateBSpline");
        addCommand("Sketcher_CreatePeriodicBSpline");
        addCommand("Sketcher_CreateBSplineByInterpolation");
        addCommand("Sketcher_CreatePeriodicBSplineByInterpolation");
    }
    void updateAction(int mode) override
    {
        Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(getAction());
        if (!pcAction) {
            return;
        }

        QList<QAction*> al = pcAction->actions();
        int index = pcAction->property("defaultAction").toInt();
        switch (static_cast<GeometryCreationMode>(mode)) {
            case GeometryCreationMode::Normal:
                al[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateBSpline"));
                al[1]->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Sketcher_Create_Periodic_BSpline"));
                al[2]->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Sketcher_CreateBSplineByInterpolation"));
                al[3]->setIcon(Gui::BitmapFactory().iconFromTheme(
                    "Sketcher_CreatePeriodicBSplineByInterpolation"));
                getAction()->setIcon(al[index]->icon());
                break;
            case GeometryCreationMode::Construction:
                al[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateBSpline_Constr"));
                al[1]->setIcon(
                    Gui::BitmapFactory().iconFromTheme("Sketcher_Create_Periodic_BSpline_Constr"));
                al[2]->setIcon(Gui::BitmapFactory().iconFromTheme(
                    "Sketcher_CreateBSplineByInterpolation_Constr"));
                al[3]->setIcon(Gui::BitmapFactory().iconFromTheme(
                    "Sketcher_CreatePeriodicBSplineByInterpolation_Constr"));
                getAction()->setIcon(al[index]->icon());
                break;
        }
    }
    const char* className() const override
    {
        return "CmdSketcherCompCreateBSpline";
    }
    bool isActive() override
    {
        return isCommandActive(getActiveGuiDocument());
    }
};

// B-spline ================================================================

DEF_STD_CMD_AU(CmdSketcherCreateBSpline)

CmdSketcherCreateBSpline::CmdSketcherCreateBSpline()
    : Command("Sketcher_CreateBSpline")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("B-Spline");
    sToolTipText = QT_TR_NOOP("Creates a B-spline curve defined by control points");
    sWhatsThis = "Sketcher_CreateBSpline";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateBSpline";
    sAccel = "G, B, B";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateBSpline, "Sketcher_CreateBSpline")

void CmdSketcherCreateBSpline::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),
                    std::make_unique<DrawSketchHandlerBSpline>(
                        ConstructionMethods::BSplineConstructionMethod::ControlPoints));
}

bool CmdSketcherCreateBSpline::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// Periodic B-spline ================================================================

/// @brief Macro that declares a new sketcher command class 'CmdSketcherCreateBSpline'
DEF_STD_CMD_AU(CmdSketcherCreatePeriodicBSpline)

/**
 * @brief ctor
 */
CmdSketcherCreatePeriodicBSpline::CmdSketcherCreatePeriodicBSpline()
    : Command("Sketcher_CreatePeriodicBSpline")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Periodic B-Spline");
    sToolTipText = QT_TR_NOOP("Creates a periodic B-spline curve defined by control points");
    sWhatsThis = "Sketcher_CreatePeriodicBSpline";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_Create_Periodic_BSpline";
    sAccel = "G, B, P";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreatePeriodicBSpline, "Sketcher_Create_Periodic_BSpline")

void CmdSketcherCreatePeriodicBSpline::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),
                    std::make_unique<DrawSketchHandlerBSpline>(
                        ConstructionMethods::BSplineConstructionMethod::ControlPoints,
                        true));
}

bool CmdSketcherCreatePeriodicBSpline::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// B-spline by interpolation ================================================================

/// @brief Macro that declares a new sketcher command class
/// 'CmdSketcherCreateBSplineByInterpolation'
DEF_STD_CMD_AU(CmdSketcherCreateBSplineByInterpolation)

CmdSketcherCreateBSplineByInterpolation::CmdSketcherCreateBSplineByInterpolation()
    : Command("Sketcher_CreateBSplineByInterpolation")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("B-Spline From Knots");
    sToolTipText = QT_TR_NOOP("Creates a B-spline from knots, i.e. from interpolation");
    sWhatsThis = "Sketcher_CreateBSplineByInterpolation";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateBSplineByInterpolation";
    sAccel = "G, B, I";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreateBSplineByInterpolation,
                           "Sketcher_CreateBSplineByInterpolation")

void CmdSketcherCreateBSplineByInterpolation::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),
                    std::make_unique<DrawSketchHandlerBSpline>(
                        ConstructionMethods::BSplineConstructionMethod::Knots));
}

bool CmdSketcherCreateBSplineByInterpolation::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// Periodic B-spline by interpolation
// ================================================================

/// @brief Macro that declares a new sketcher command class
/// 'CmdSketcherCreatePeriodicBSplineByInterpolation'
DEF_STD_CMD_AU(CmdSketcherCreatePeriodicBSplineByInterpolation)

CmdSketcherCreatePeriodicBSplineByInterpolation::CmdSketcherCreatePeriodicBSplineByInterpolation()
    : Command("Sketcher_CreatePeriodicBSplineByInterpolation")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Periodic B-Spline From Knots");
    sToolTipText = QT_TR_NOOP("Creates a periodic B-spline defined by knots using interpolation");
    sWhatsThis = "Sketcher_CreatePeriodicBSplineByInterpolation";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreatePeriodicBSplineByInterpolation";
    sAccel = "G, B, O";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCreatePeriodicBSplineByInterpolation,
                           "Sketcher_CreatePeriodicBSplineByInterpolation")

void CmdSketcherCreatePeriodicBSplineByInterpolation::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    ActivateHandler(getActiveGuiDocument(),
                    std::make_unique<DrawSketchHandlerBSpline>(
                        ConstructionMethods::BSplineConstructionMethod::Knots,
                        true));
}

bool CmdSketcherCreatePeriodicBSplineByInterpolation::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// ======================================================================================
// Comp for fillet tools =============================================

class CmdSketcherCompCreateFillets: public Gui::GroupCommand
{
public:
    CmdSketcherCompCreateFillets()
        : GroupCommand("Sketcher_CompCreateFillets")
    {
        sAppModule = "Sketcher";
        sGroup = "Sketcher";
        sMenuText = QT_TR_NOOP("Fillet/Chamfer");
        sToolTipText = QT_TR_NOOP("Creates a fillet or chamfer between 2 lines");
        sWhatsThis = "Sketcher_CompCreateFillets";
        sStatusTip = sToolTipText;
        sAccel = "G, F, F";
        eType = ForEdit;

        setCheckable(false);

        addCommand("Sketcher_CreateFillet");
        addCommand("Sketcher_CreateChamfer");
    }

    const char* className() const override
    {
        return "CmdSketcherCompCreateFillets";
    }

    bool isActive() override
    {
        return isCommandActive(getActiveGuiDocument());
    }
};

// Fillet ================================================================


DEF_STD_CMD_A(CmdSketcherCreateFillet)

CmdSketcherCreateFillet::CmdSketcherCreateFillet()
    : Command("Sketcher_CreateFillet")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Fillet");
    sToolTipText = QT_TR_NOOP("Creates a fillet between 2 selected lines or 1 coincident point");
    sWhatsThis = "Sketcher_CreateFillet";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateFillet";
    sAccel = "G, F, F";
    eType = ForEdit;
}

void CmdSketcherCreateFillet::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),
                    std::make_unique<DrawSketchHandlerFillet>(
                        ConstructionMethods::FilletConstructionMethod::Fillet));
}

bool CmdSketcherCreateFillet::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// Chamfer ================================================================

DEF_STD_CMD_A(CmdSketcherCreateChamfer)

CmdSketcherCreateChamfer::CmdSketcherCreateChamfer()
    : Command("Sketcher_CreateChamfer")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Chamfer");
    sToolTipText = QT_TR_NOOP("Creates a chamfer between 2 selected lines or 1 coincident point");
    sWhatsThis = "Sketcher_CreateChamfer";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateChamfer";
    sAccel = "G, F, C";
    eType = ForEdit;
}

void CmdSketcherCreateChamfer::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),
                    std::make_unique<DrawSketchHandlerFillet>(
                        ConstructionMethods::FilletConstructionMethod::Chamfer));
}

bool CmdSketcherCreateChamfer::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// ======================================================================================
// Comp for curve edition tools =============================================

class CmdSketcherCompCurveEdition: public Gui::GroupCommand
{
public:
    CmdSketcherCompCurveEdition()
        : GroupCommand("Sketcher_CompCurveEdition")
    {
        sAppModule = "Sketcher";
        sGroup = "Sketcher";
        sMenuText = QT_TR_NOOP("Edit Edges");
        sToolTipText = QT_TR_NOOP("Edge editing tools");
        sWhatsThis = "Sketcher_CompCurveEdition";
        sStatusTip = sToolTipText;
        sAccel = "G, T";
        eType = ForEdit;

        setCheckable(false);

        addCommand("Sketcher_Trimming");
        addCommand("Sketcher_Split");
        addCommand("Sketcher_Extend");
    }

    const char* className() const override
    {
        return "CmdSketcherCompCurveEdition";
    }

    bool isActive() override
    {
        return isCommandActive(getActiveGuiDocument());
    }
};

// Trim edge ================================================================

DEF_STD_CMD_A(CmdSketcherTrimming)

CmdSketcherTrimming::CmdSketcherTrimming()
    : Command("Sketcher_Trimming")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Trim Edge");
    sToolTipText = QT_TR_NOOP("Trims an edge with respect to the selected position");
    sWhatsThis = "Sketcher_Trimming";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_Trimming";
    sAccel = "G, T";
    eType = ForEdit;
}

void CmdSketcherTrimming::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), std::make_unique<DrawSketchHandlerTrimming>());
}

bool CmdSketcherTrimming::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// Extend edge ================================================================

DEF_STD_CMD_A(CmdSketcherExtend)

// TODO: fix the translations for this
CmdSketcherExtend::CmdSketcherExtend()
    : Command("Sketcher_Extend")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Extend Edge");
    sToolTipText = QT_TR_NOOP("Extends an edge with respect to the selected position");
    sWhatsThis = "Sketcher_Extend";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_Extend";
    sAccel = "G, Q";
    eType = ForEdit;
}

void CmdSketcherExtend::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), std::make_unique<DrawSketchHandlerExtend>());
}

bool CmdSketcherExtend::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// Split edge ================================================================

DEF_STD_CMD_A(CmdSketcherSplit)

// TODO: fix the translations for this
CmdSketcherSplit::CmdSketcherSplit()
    : Command("Sketcher_Split")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Split Edge");
    sToolTipText = QT_TR_NOOP("Splits an edge into 2 segments while preserving constraints");
    sWhatsThis = "Sketcher_Split";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_Split";
    sAccel = "G, Z";
    eType = ForEdit;
}

void CmdSketcherSplit::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), std::make_unique<DrawSketchHandlerSplitting>());
}

bool CmdSketcherSplit::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// ======================================================================================
// Comp for curve external tools =============================================

class CmdSketcherCompExternal: public Gui::GroupCommand
{
public:
    CmdSketcherCompExternal()
        : GroupCommand("Sketcher_CompExternal")
    {
        sAppModule = "Sketcher";
        sGroup = "Sketcher";
        sMenuText = QT_TR_NOOP("External Geometry");
        sToolTipText =
            QT_TR_NOOP("Creates sketch elements linked to geometry defined outside the sketch");
        sWhatsThis = "Sketcher_CompExternal";
        sStatusTip = sToolTipText;
        sAccel = "G, X";
        eType = ForEdit;

        setCheckable(false);

        addCommand("Sketcher_Projection");
        addCommand("Sketcher_Intersection");
    }

    void updateAction(int mode) override
    {
        Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(getAction());
        if (!pcAction) {
            return;
        }

        QList<QAction*> al = pcAction->actions();
        int index = pcAction->property("defaultAction").toInt();
        switch (static_cast<GeometryCreationMode>(mode)) {
            case GeometryCreationMode::Normal:
                al[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Projection"));
                al[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Intersection"));
                getAction()->setIcon(al[index]->icon());
                break;
            case GeometryCreationMode::Construction:
                al[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Projection_Constr"));
                al[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Intersection_Constr"));
                getAction()->setIcon(al[index]->icon());
                break;
        }
    }

    const char* className() const override
    {
        return "CmdSketcherCompExternal";
    }

    bool isActive() override
    {
        return isCommandActive(getActiveGuiDocument());
    }
};

// Externals - Projection ==================================================================

DEF_STD_CMD_AU(CmdSketcherProjection)

CmdSketcherProjection::CmdSketcherProjection()
    : Command("Sketcher_Projection")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("External Projection");
    sToolTipText = QT_TR_NOOP("Creates the projection of external geometry in the sketch plane");
    sWhatsThis = "Sketcher_Projection";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_Projection";
    sAccel = "G, X";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherProjection, "Sketcher_Projection")

void CmdSketcherProjection::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    bool extGeoRef = Gui::WindowParameter::getDefaultParameter()
                         ->GetGroup("Mod/Sketcher/General")
                         ->GetBool("AlwaysExtGeoReference", false);
    ActivateHandler(getActiveGuiDocument(),
                    std::make_unique<DrawSketchHandlerExternal>(extGeoRef, false));
}

bool CmdSketcherProjection::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// Externals - Intersection ==================================================================

DEF_STD_CMD_AU(CmdSketcherIntersection)

CmdSketcherIntersection::CmdSketcherIntersection()
    : Command("Sketcher_Intersection")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("External Intersection");
    sToolTipText =
        QT_TR_NOOP("Creates the intersection of external geometry with the sketch plane");
    sWhatsThis = "Sketcher_Intersection";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_Intersection";
    sAccel = "G, I";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherIntersection, "Sketcher_Intersection")

void CmdSketcherIntersection::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    bool extGeoRef = Gui::WindowParameter::getDefaultParameter()
                         ->GetGroup("Mod/Sketcher/General")
                         ->GetBool("AlwaysExtGeoReference", false);
    ActivateHandler(getActiveGuiDocument(),
                    std::make_unique<DrawSketchHandlerExternal>(extGeoRef, true));
}

bool CmdSketcherIntersection::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

// ======================================================================================
// Carbon copy =============================================

DEF_STD_CMD_AU(CmdSketcherCarbonCopy)

CmdSketcherCarbonCopy::CmdSketcherCarbonCopy()
    : Command("Sketcher_CarbonCopy")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Carbon Copy");
    sToolTipText = QT_TR_NOOP("Copies the geometry of another sketch");
    sWhatsThis = "Sketcher_CarbonCopy";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CarbonCopy";
    sAccel = "G, W";
    eType = ForEdit;
}

CONSTRUCTION_UPDATE_ACTION(CmdSketcherCarbonCopy, "Sketcher_CarbonCopy")

void CmdSketcherCarbonCopy::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), std::make_unique<DrawSketchHandlerCarbonCopy>());
}

bool CmdSketcherCarbonCopy::isActive()
{
    return isCommandActive(getActiveGuiDocument());
}

void CreateSketcherCommandsCreateGeo()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdSketcherCreatePoint());
    rcCmdMgr.addCommand(new CmdSketcherCreateArc());
    rcCmdMgr.addCommand(new CmdSketcherCreate3PointArc());
    rcCmdMgr.addCommand(new CmdSketcherCreateCircle());
    rcCmdMgr.addCommand(new CmdSketcherCreate3PointCircle());
    rcCmdMgr.addCommand(new CmdSketcherCreateEllipseByCenter());
    rcCmdMgr.addCommand(new CmdSketcherCreateEllipseBy3Points());
    rcCmdMgr.addCommand(new CmdSketcherCreateArcOfEllipse());
    rcCmdMgr.addCommand(new CmdSketcherCreateArcOfHyperbola());
    rcCmdMgr.addCommand(new CmdSketcherCreateArcOfParabola());
    rcCmdMgr.addCommand(new CmdSketcherCreateBSpline());
    rcCmdMgr.addCommand(new CmdSketcherCreatePeriodicBSpline());
    rcCmdMgr.addCommand(new CmdSketcherCreateBSplineByInterpolation());
    rcCmdMgr.addCommand(new CmdSketcherCreatePeriodicBSplineByInterpolation());
    rcCmdMgr.addCommand(new CmdSketcherCreateLine());
    rcCmdMgr.addCommand(new CmdSketcherCreatePolyline());
    rcCmdMgr.addCommand(new CmdSketcherCreateRectangle());
    rcCmdMgr.addCommand(new CmdSketcherCreateRectangleCenter());
    rcCmdMgr.addCommand(new CmdSketcherCreateOblong());
    rcCmdMgr.addCommand(new CmdSketcherCreateTriangle());
    rcCmdMgr.addCommand(new CmdSketcherCreateSquare());
    rcCmdMgr.addCommand(new CmdSketcherCreatePentagon());
    rcCmdMgr.addCommand(new CmdSketcherCreateHexagon());
    rcCmdMgr.addCommand(new CmdSketcherCreateHeptagon());
    rcCmdMgr.addCommand(new CmdSketcherCreateOctagon());
    rcCmdMgr.addCommand(new CmdSketcherCreateRegularPolygon());
    rcCmdMgr.addCommand(new CmdSketcherCreateSlot());
    rcCmdMgr.addCommand(new CmdSketcherCreateArcSlot());
    rcCmdMgr.addCommand(new CmdSketcherCreateFillet());
    rcCmdMgr.addCommand(new CmdSketcherCreateChamfer());
    // rcCmdMgr.addCommand(new CmdSketcherCreateText());
    // rcCmdMgr.addCommand(new CmdSketcherCreateDraftLine());
    rcCmdMgr.addCommand(new CmdSketcherTrimming());
    rcCmdMgr.addCommand(new CmdSketcherExtend());
    rcCmdMgr.addCommand(new CmdSketcherSplit());
    rcCmdMgr.addCommand(new CmdSketcherProjection());
    rcCmdMgr.addCommand(new CmdSketcherIntersection());
    rcCmdMgr.addCommand(new CmdSketcherCarbonCopy());

    // Group command must be added after its subcommands.
    rcCmdMgr.addCommand(new CmdSketcherCompLine());
    rcCmdMgr.addCommand(new CmdSketcherCompCreateArc());
    rcCmdMgr.addCommand(new CmdSketcherCompCreateConic());
    rcCmdMgr.addCommand(new CmdSketcherCompCreateRectangles());
    rcCmdMgr.addCommand(new CmdSketcherCompCreateRegularPolygon());
    rcCmdMgr.addCommand(new CmdSketcherCompSlot());
    rcCmdMgr.addCommand(new CmdSketcherCompCreateBSpline());
    rcCmdMgr.addCommand(new CmdSketcherCompCreateFillets());
    rcCmdMgr.addCommand(new CmdSketcherCompCurveEdition());
    rcCmdMgr.addCommand(new CmdSketcherCompExternal());
}
