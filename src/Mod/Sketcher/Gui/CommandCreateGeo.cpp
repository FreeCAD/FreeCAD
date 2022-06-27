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
# include <Inventor/events/SoKeyboardEvent.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <QApplication>
# include <QInputDialog>
# include <QMessageBox>
# include <QString>
# include <cstdlib>
# include <qdebug.h>
# include <GC_MakeEllipse.hxx>
# include <boost/math/special_functions/fpclassify.hpp>
# include <memory>
#endif

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Tools.h>

#include <App/OriginFeature.h>
#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Document.h>
#include <Gui/CommandT.h>
#include <Gui/MainWindow.h>
#include <Gui/DlgEditFileIncludePropertyExternal.h>
#include <Gui/Selection.h>
#include <Gui/SelectionFilter.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/Part/App/DatumFeature.h>
#include <Mod/Part/App/BodyBase.h>
#include <Mod/Part/App/Geometry2d.h>
#include <Mod/Sketcher/App/Constraint.h>

#include "ViewProviderSketch.h"
#include "DrawSketchHandler.h"
#include "Utils.h"

#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/SoFCUnifiedSelection.h>

#include <Gui/ToolBarManager.h>

#include "GeometryCreationMode.h"

#include "DrawSketchHandlerLine.h"
#include "DrawSketchHandlerRectangle.h"
#include "DrawSketchHandlerPolygon.h"
#include "DrawSketchHandlerLineSet.h"
#include "DrawSketchHandlerCircle.h"
#include "DrawSketchHandlerEllipse.h"
#include "DrawSketchHandlerArc.h"
#include "DrawSketchHandlerArcOfEllipse.h"
#include "DrawSketchHandlerArcOfHyperbola.h"
#include "DrawSketchHandlerArcOfParabola.h"
#include "DrawSketchHandlerBSpline.h"
#include "DrawSketchHandlerPoint.h"
#include "DrawSketchHandlerFillet.h"
#include "DrawSketchHandlerTrimming.h"
#include "DrawSketchHandlerExtend.h"
#include "DrawSketchHandlerSplitting.h"
#include "DrawSketchHandlerExternal.h"
#include "DrawSketchHandlerCarbonCopy.h"
#include "DrawSketchHandlerSlot.h"

using namespace std;
using namespace SketcherGui;

namespace bp = boost::placeholders;

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

void CmdSketcherCreateLine::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerLine() );
}

void CmdSketcherCreateLine::updateAction(int mode)
{
    switch (mode) {
    case Normal:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateLine"));
        break;
    case Construction:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateLine_Constr"));
        break;
    }
}

bool CmdSketcherCreateLine::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}


/* Create Rectangle =======================================================*/

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

void CmdSketcherCreateRectangle::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerRectangle(ConstructionMethods::RectangleConstructionMethod::Diagonal) );
}

void CmdSketcherCreateRectangle::updateAction(int mode)
{
    switch (mode) {
    case Normal:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangle"));
        break;
    case Construction:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangle_Constr"));
        break;
    }
}

bool CmdSketcherCreateRectangle::isActive(void)
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

void CmdSketcherCreateRectangleCenter::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerRectangle(ConstructionMethods::RectangleConstructionMethod::CenterAndCorner) );
}

void CmdSketcherCreateRectangleCenter::updateAction(int mode)
{
    switch (mode) {
    case Normal:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangle_Center"));
        break;
    case Construction:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangle_Center_Constr"));
        break;
    }
}

bool CmdSketcherCreateRectangleCenter::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}

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

void CmdSketcherCreateOblong::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerRectangle(ConstructionMethods::RectangleConstructionMethod::Diagonal, true, false));
}

void CmdSketcherCreateOblong::updateAction(int mode)
{
    switch (mode) {
    case Normal:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateOblong"));
        break;
    case Construction:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateOblong_Constr"));
        break;
    }
}

bool CmdSketcherCreateOblong::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}


/* Polygon ================================================================================*/


DEF_STD_CMD_A(CmdSketcherCreatePolygon)

CmdSketcherCreatePolygon::CmdSketcherCreatePolygon()
    : Command("Sketcher_CreatePolygon")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Create polygon");
    sToolTipText = QT_TR_NOOP("Create a polygon in the sketch");
    sWhatsThis = "Sketcher_CreatePolygon";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreatePolygon";
    sAccel = "G, P, R";
    eType = ForEdit;
}

void CmdSketcherCreatePolygon::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerPolygon());
}

bool CmdSketcherCreatePolygon::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}

DEF_STD_CMD_A(CmdSketcherCreateTriangle)

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

void CmdSketcherCreateTriangle::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerPolygon(3));
}

bool CmdSketcherCreateTriangle::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}

DEF_STD_CMD_A(CmdSketcherCreateSquare)

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

void CmdSketcherCreateSquare::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerPolygon(4));
}

bool CmdSketcherCreateSquare::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}

DEF_STD_CMD_A(CmdSketcherCreatePentagon)

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

void CmdSketcherCreatePentagon::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerPolygon(5));
}

bool CmdSketcherCreatePentagon::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}


DEF_STD_CMD_A(CmdSketcherCreateHexagon)

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

void CmdSketcherCreateHexagon::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerPolygon(6));
}

bool CmdSketcherCreateHexagon::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}

DEF_STD_CMD_A(CmdSketcherCreateHeptagon)

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

void CmdSketcherCreateHeptagon::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerPolygon(7));
}

bool CmdSketcherCreateHeptagon::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}

DEF_STD_CMD_A(CmdSketcherCreateOctagon)

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

void CmdSketcherCreateOctagon::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerPolygon(8));
}

bool CmdSketcherCreateOctagon::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}

/* Rectangles Comp command ==============================================================*/

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
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerRectangle(ConstructionMethods::RectangleConstructionMethod::Diagonal));
    else if (iMsg == 1)
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerPolygon());
    else
        return;

    // Since the default icon is reset when enabling/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action* CmdSketcherCompCreateRectangles::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* arc1 = pcAction->addAction(QString());
    arc1->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangle"));
    QAction* arc2 = pcAction->addAction(QString());
    arc2->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreatePolygon"));

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
        a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreatePolygon"));
        getAction()->setIcon(a[index]->icon());
        break;
    case Construction:
        a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangle_Constr"));
        a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreatePolygon_Constr"));
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
    rectangle2->setText(QApplication::translate("CmdSketcherCompCreateRectangles", "Polygon"));
    rectangle2->setToolTip(QApplication::translate("Sketcher_CreatePolygon", "Create a regular polygon"));
    rectangle2->setStatusTip(rectangle2->toolTip());
}

bool CmdSketcherCompCreateRectangles::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}

/* Polyline ================================================================================*/



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

void CmdSketcherCreatePolyline::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerLineSet() );
}

void CmdSketcherCreatePolyline::updateAction(int mode)
{
    switch (mode) {
    case Normal:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreatePolyline"));
        break;
    case Construction:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreatePolyline_Constr"));
        break;
    }
}

bool CmdSketcherCreatePolyline::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}


/* Circle ================================================================================*/


DEF_STD_CMD_A(CmdSketcherCreateCircle)

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

void CmdSketcherCreateCircle::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerCircle(ConstructionMethods::CircleEllipseConstructionMethod::Center));
}

bool CmdSketcherCreateCircle::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}

DEF_STD_CMD_A(CmdSketcherCreate3PointCircle)

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

void CmdSketcherCreate3PointCircle::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerCircle(ConstructionMethods::CircleEllipseConstructionMethod::ThreeRim));
}

bool CmdSketcherCreate3PointCircle::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}

DEF_STD_CMD_A(CmdSketcherCreateEllipseByCenter)

CmdSketcherCreateEllipseByCenter::CmdSketcherCreateEllipseByCenter()
  : Command("Sketcher_CreateEllipseByCenter")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create ellipse by center");
    sToolTipText    = QT_TR_NOOP("Create an ellipse by center in the sketch");
    sWhatsThis      = "Sketcher_CreateEllipseByCenter";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_Conics_Ellipse_Center";
    sAccel          = "G, E, E";
    eType           = ForEdit;
}

void CmdSketcherCreateEllipseByCenter::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerEllipse(ConstructionMethods::CircleEllipseConstructionMethod::Center));
}

bool CmdSketcherCreateEllipseByCenter::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}

/// @brief Macro that declares a new sketcher command class 'CmdSketcherCreateEllipseBy3Points'
DEF_STD_CMD_A(CmdSketcherCreateEllipseBy3Points)

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

void CmdSketcherCreateEllipseBy3Points::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerEllipse(ConstructionMethods::CircleEllipseConstructionMethod::ThreeRim));
}

bool CmdSketcherCreateEllipseBy3Points::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}

// Comp for circle and ellipse ============================================================

DEF_STD_CMD_ACLU(CmdSketcherCompCreateCircle)

CmdSketcherCompCreateCircle::CmdSketcherCompCreateCircle()
    : Command("Sketcher_CompCreateCircle")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Create circle");
    sToolTipText = QT_TR_NOOP("Create a circle in the sketcher");
    sWhatsThis = "Sketcher_CompCreateCircle";
    sStatusTip = sToolTipText;
    eType = ForEdit;
}

void CmdSketcherCompCreateCircle::activated(int iMsg)
{
    if (iMsg == 0)
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerCircle(ConstructionMethods::CircleEllipseConstructionMethod::Center));
    else if (iMsg == 1)
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerEllipse(ConstructionMethods::CircleEllipseConstructionMethod::Center));
    else
        return;

    // Since the default icon is reset when enabling/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action* CmdSketcherCompCreateCircle::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* circle = pcAction->addAction(QString());
    circle->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateCircle"));
    QAction* ellipse = pcAction->addAction(QString());
    ellipse->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateEllipse"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(circle->icon());
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
        a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateEllipse"));
        getAction()->setIcon(a[index]->icon());
        break;
    case Construction:
        a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateCircle_Constr"));
        a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateEllipse_Constr"));
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

    QAction* circle = a[0];
    circle->setText(QApplication::translate("CmdSketcherCompCreateCircle", "Circle"));
    circle->setToolTip(QApplication::translate("Sketcher_CreateCircle", "Create a circle by its center or by rim points"));
    circle->setStatusTip(QApplication::translate("Sketcher_CreateCircle", "Create a circle by its center or by rim points"));
    QAction* ellipse = a[1];
    ellipse->setText(QApplication::translate("CmdSketcherCompCreateCircle", "Ellipse"));
    ellipse->setToolTip(QApplication::translate("Sketcher_CreateEllipse", "Create an ellipse by center or by rim points"));
    ellipse->setStatusTip(QApplication::translate("Sketcher_CreateEllipse", "Create an ellipse by center or by rim points"));
}

bool CmdSketcherCompCreateCircle::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}


/* Arc of Circle tool  =================================================================*/


DEF_STD_CMD_A(CmdSketcherCreateArc)

CmdSketcherCreateArc::CmdSketcherCreateArc()
    : Command("Sketcher_CreateArc")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Create arc of circle");
    sToolTipText = QT_TR_NOOP("Create an arc by its center or by its end points");
    sWhatsThis = "Sketcher_CreateArc";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateArc";
    sAccel = "G, A";
    eType = ForEdit;
}

void CmdSketcherCreateArc::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerArc());
}

bool CmdSketcherCreateArc::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}

DEF_STD_CMD_A(CmdSketcherCreate3PointArc)

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

void CmdSketcherCreate3PointArc::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerArc(ConstructionMethods::CircleEllipseConstructionMethod::ThreeRim));
}

bool CmdSketcherCreate3PointArc::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}


/* Arc of Ellipse tool  ===============================================================*/

DEF_STD_CMD_A(CmdSketcherCreateArcOfEllipse)

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

void CmdSketcherCreateArcOfEllipse::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerArcOfEllipse() );
}

bool CmdSketcherCreateArcOfEllipse::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}


DEF_STD_CMD_A(CmdSketcherCreateArcOfHyperbola)

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

void CmdSketcherCreateArcOfHyperbola::activated(int /*iMsg*/)
{
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerArcOfHyperbola() );
}

bool CmdSketcherCreateArcOfHyperbola::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}


DEF_STD_CMD_A(CmdSketcherCreateArcOfParabola)

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

void CmdSketcherCreateArcOfParabola::activated(int /*iMsg*/)
{
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerArcOfParabola() );
}

bool CmdSketcherCreateArcOfParabola::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}

// Comp for arcs (circle, ellipse, hyperbola, parabola)===========================================

DEF_STD_CMD_ACLU(CmdSketcherCompCreateArc)

CmdSketcherCompCreateArc::CmdSketcherCompCreateArc()
    : Command("Sketcher_CompCreateArc")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Create arc");
    sToolTipText = QT_TR_NOOP("Create an arc in the sketcher");
    sWhatsThis = "Sketcher_CompCreateArc";
    sStatusTip = sToolTipText;
    eType = ForEdit;
}

void CmdSketcherCompCreateArc::activated(int iMsg)
{
    if (iMsg == 0)
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerArc());
    else if (iMsg == 1)
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerArcOfEllipse());
    else if (iMsg == 2)
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerArcOfHyperbola());
    else if (iMsg == 3)
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerArcOfParabola());
    else
        return;

    // Since the default icon is reset when enabling/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action* CmdSketcherCompCreateArc::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* arc = pcAction->addAction(QString());
    arc->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateArc"));

    QAction* arcofellipse = pcAction->addAction(QString());
    arcofellipse->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateElliptical_Arc"));

    QAction* arcofhyperbola = pcAction->addAction(QString());
    arcofhyperbola->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateHyperbolic_Arc"));

    QAction* arcofparabola = pcAction->addAction(QString());
    arcofparabola->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateParabolic_Arc"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(arc->icon());
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
        a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateElliptical_Arc"));
        a[2]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateHyperbolic_Arc"));
        a[3]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateParabolic_Arc"));
        getAction()->setIcon(a[index]->icon());
        break;
    case Construction:
        a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateArc_Constr"));
        a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateElliptical_Arc_Constr"));
        a[2]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateHyperbolic_Arc_Constr"));
        a[3]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateParabolic_Arc_Constr"));
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
    arc1->setText(QApplication::translate("CmdSketcherCompCreateArc", "Arc of Circle"));
    arc1->setToolTip(QApplication::translate("Sketcher_CreateArc", "Create an arc by its center or by its end points"));
    arc1->setStatusTip(QApplication::translate("Sketcher_CreateArc", "Create an arc by its center or by its end points"));
    QAction* arcofellipse = a[1];
    arcofellipse->setText(QApplication::translate("CmdSketcherCompCreateArc", "Arc of ellipse"));
    arcofellipse->setToolTip(QApplication::translate("Sketcher_CreateArcOfEllipse", "Create an arc of ellipse by its center, major radius, and endpoints"));
    arcofellipse->setStatusTip(QApplication::translate("Sketcher_CreateArcOfEllipse", "Create an arc of ellipse by its center, major radius, and endpoints"));
    QAction* arcofhyperbola = a[2];
    arcofhyperbola->setText(QApplication::translate("CmdSketcherCompCreateArc", "Arc of hyperbola"));
    arcofhyperbola->setToolTip(QApplication::translate("Sketcher_CreateArcOfHyperbola", "Create an arc of hyperbola by its center, major radius, and endpoints"));
    arcofhyperbola->setStatusTip(QApplication::translate("Sketcher_CreateArcOfHyperbola", "Create an arc of hyperbola by its center, major radius, and endpoints"));
    QAction* arcofparabola = a[3];
    arcofparabola->setText(QApplication::translate("CmdSketcherCompCreateArc", "Arc of parabola"));
    arcofparabola->setToolTip(QApplication::translate("Sketcher_CreateArcOfParabola", "Create an arc of parabola by its focus, vertex, and endpoints"));
    arcofparabola->setStatusTip(QApplication::translate("Sketcher_CreateArcOfParabola", "Create an arc of parabola by its focus, vertex, and endpoints"));
}

bool CmdSketcherCompCreateArc::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}

// ======================================================================================

DEF_STD_CMD_A(CmdSketcherCreateBSpline)

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

void CmdSketcherCreateBSpline::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerBSpline(0) );
}

/*void CmdSketcherCreateBSpline::updateAction(int mode)
{
    switch (mode) {
    case Normal:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateBSpline"));
        break;
    case Construction:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateBSpline_Constr"));
        break;
    }
}*/

bool CmdSketcherCreateBSpline::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}

/// @brief Macro that declares a new sketcher command class 'CmdSketcherCreateBSpline'
DEF_STD_CMD_A(CmdSketcherCreatePeriodicBSpline)

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

void CmdSketcherCreatePeriodicBSpline::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerBSpline(1) );
}

bool CmdSketcherCreatePeriodicBSpline::isActive(void)
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
    } else if (iMsg == 1) {
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerBSpline(iMsg));
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

Gui::Action * CmdSketcherCompCreateBSpline::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* bspline = pcAction->addAction(QString());
    bspline->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateBSpline"));

    QAction* periodicbspline = pcAction->addAction(QString());
    periodicbspline->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Create_Periodic_BSpline"));

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

bool CmdSketcherCompCreateBSpline::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}

/* Create Point =======================================================*/

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

bool CmdSketcherCreatePoint::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}

// Fillet and Chamfer ===================================================================



DEF_STD_CMD_AU(CmdSketcherCreateFillet)

CmdSketcherCreateFillet::CmdSketcherCreateFillet()
  : Command("Sketcher_CreateFillet")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Fillet - Chamfer");
    sToolTipText    = QT_TR_NOOP("Create a fillet or chamfer between two lines or at a coincident point");
    sWhatsThis      = "Sketcher_CreateFillet";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreateFillet";
    sAccel          = "G, F";
    eType           = ForEdit;
}

void CmdSketcherCreateFillet::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerFillet());
}

void CmdSketcherCreateFillet::updateAction(int mode)
{
    switch (mode) {
    case Normal:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateFillet"));
        break;
    case Construction:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateFillet_Constr"));
        break;
    }
}

bool CmdSketcherCreateFillet::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}

// Trim edge =========================================================================

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

bool CmdSketcherTrimming::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}


// Extend edge ========================================================================

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

bool CmdSketcherExtend::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}


// Split edge ==========================================================================

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

bool CmdSketcherSplit::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}

DEF_STD_CMD_A(CmdSketcherInsert)

CmdSketcherInsert::CmdSketcherInsert()
    : Command("Sketcher_Insert")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Insert geometry on line");
    sToolTipText = QT_TR_NOOP("Insert a geometry in a line while preserving constraints.");
    sWhatsThis = "Sketcher_Insert";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_Insert";
    sAccel = "G, I";
    eType = ForEdit;
}

void CmdSketcherInsert::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    //The following code let us catch if user selected an edge before launching the tool.
    int geoId = Sketcher::GeoEnum::GeoUndef;
    std::vector<Gui::SelectionObject> selection;
    selection = Gui::Command::getSelection().getSelectionEx(0, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() == 1) {
        // get the needed lists and objects
        const std::vector<std::string>& SubNames = selection[0].getSubNames();
        if (SubNames.size() == 1) {
            if (SubNames[0].size() > 4 && SubNames[0].substr(0, 4) == "Edge") {
                geoId = std::atoi(SubNames[0].substr(4, 4000).c_str()) - 1;
            }
        }
    }
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerInsert(geoId));
}

bool CmdSketcherInsert::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}

/* Modify edge comp ====================================================================*/

DEF_STD_CMD_ACL(CmdSketcherCompModifyEdge)

CmdSketcherCompModifyEdge::CmdSketcherCompModifyEdge()
    : Command("Sketcher_CompModifyEdge")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Modify an edge");
    sToolTipText = QT_TR_NOOP("Trim, extend, split... and edge");
    sWhatsThis = "Sketcher_CompModifyEdge";
    sStatusTip = sToolTipText;
    sAccel = "G, T";
    eType = ForEdit;
}

void CmdSketcherCompModifyEdge::activated(int iMsg)
{
    switch (iMsg) {
    case 0:
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerTrimming()); break;
    case 1:
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerExtend()); break;
    case 2:
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerSplitting()); break;
    case 3:
    {
        //The following code let us catch if user selected an edge before launching the tool.
        int geoId = Sketcher::GeoEnum::GeoUndef;
        std::vector<Gui::SelectionObject> selection;
        selection = Gui::Command::getSelection().getSelectionEx(0, Sketcher::SketchObject::getClassTypeId());

        // only one sketch with its subelements are allowed to be selected
        if (selection.size() == 1) {
            // get the needed lists and objects
            const std::vector<std::string>& SubNames = selection[0].getSubNames();
            if (SubNames.size() == 1) {
                if (SubNames[0].size() > 4 && SubNames[0].substr(0, 4) == "Edge") {
                    geoId = std::atoi(SubNames[0].substr(4, 4000).c_str()) - 1;
                }
            }
        }
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerInsert(geoId)); break;
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

Gui::Action* CmdSketcherCompModifyEdge::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* trim = pcAction->addAction(QString());
    trim->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Trimming"));
    QAction* extend = pcAction->addAction(QString());
    extend->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Extend"));
    QAction* split = pcAction->addAction(QString());
    split->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Split"));
    QAction* insert = pcAction->addAction(QString());
    insert->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Insert"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(trim->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdSketcherCompModifyEdge::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* trim = a[0];
    trim->setText(QApplication::translate("CmdSketcherCompModifyEdge", "Trim edge"));
    trim->setToolTip(QApplication::translate("Sketcher_Trimming", "Trim an edge with respect to the picked position"));
    trim->setStatusTip(QApplication::translate("Sketcher_Trimming", "Trim an edge with respect to the picked position"));
    QAction* extend = a[1];
    extend->setText(QApplication::translate("CmdSketcherCompModifyEdge", "Extend edge"));
    extend->setToolTip(QApplication::translate("Sketcher_Extend", "Extend an edge with respect to the picked position"));
    extend->setStatusTip(QApplication::translate("Sketcher_Extend", "Extend an edge with respect to the picked position"));
    QAction* split = a[2];
    split->setText(QApplication::translate("CmdSketcherCompModifyEdge", "Split edge"));
    split->setToolTip(QApplication::translate("Sketcher_Split", "Splits an edge into two while preserving constraintst"));
    split->setStatusTip(QApplication::translate("Sketcher_Split", "Splits an edge into two while preserving constraints"));
    QAction* insert = a[3];
    insert->setText(QApplication::translate("CmdSketcherCompModifyEdge", "Insert geometry on line"));
    split->setToolTip(QApplication::translate("Sketcher_Insert", "Insert a geometry in a line while preserving constraints"));
    split->setStatusTip(QApplication::translate("Sketcher_Insert", "Insert a geometry in a line while preserving constraints"));
}

bool CmdSketcherCompModifyEdge::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}

/* External Geometries ==================================================================*/


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

bool CmdSketcherExternal::isActive(void)
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

void CmdSketcherCarbonCopy::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerCarbonCopy());
}

bool CmdSketcherCarbonCopy::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}

void CmdSketcherCarbonCopy::updateAction(int mode)
{
    switch (mode) {
        case Normal:
            if (getAction())
                getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CarbonCopy"));
            break;
        case Construction:
            if (getAction())
                getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CarbonCopy_Constr"));
            break;
    }
}


/* Create Slot =========================================================*/

DEF_STD_CMD_A(CmdSketcherCreateSlot)

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
    sAccel = "G, S";
    eType = ForEdit;
}

void CmdSketcherCreateSlot::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerSlot());
}

bool CmdSketcherCreateSlot::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}

/* Create Arc Slot =========================================================*/


DEF_STD_CMD_A(CmdSketcherCreateArcSlot)

CmdSketcherCreateArcSlot::CmdSketcherCreateArcSlot()
    : Command("Sketcher_CreateArcSlot")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Create arc slot");
    sToolTipText = QT_TR_NOOP("Create an arc slot in the sketch");
    sWhatsThis = "Sketcher_CreateArcSlot";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateArcSlot";
    sAccel = "G, S, 2";
    eType = ForEdit;
}

void CmdSketcherCreateArcSlot::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerArcSlot());
}

bool CmdSketcherCreateArcSlot::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}

/* Slot comp ============================================================*/

DEF_STD_CMD_ACLU(CmdSketcherCompCreateSlot)

CmdSketcherCompCreateSlot::CmdSketcherCompCreateSlot()
    : Command("Sketcher_CompCreateSlot")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Create slot");
    sToolTipText = QT_TR_NOOP("Create a slot in the sketcher");
    sWhatsThis = "Sketcher_CompCreateSlot";
    sStatusTip = sToolTipText;
    sAccel = "G, S, S";
    eType = ForEdit;
}

void CmdSketcherCompCreateSlot::activated(int iMsg)
{
    switch (iMsg) {
    case 0:
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerSlot()); break;
    case 1:
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerArcSlot()); break;
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

Gui::Action* CmdSketcherCompCreateSlot::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* slot = pcAction->addAction(QString());
    slot->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateSlot"));
    QAction* arcSlot = pcAction->addAction(QString());
    arcSlot->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateArcSlot"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(slot->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdSketcherCompCreateSlot::updateAction(int mode)
{
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(getAction());
    if (!pcAction)
        return;

    QList<QAction*> a = pcAction->actions();
    int index = pcAction->property("defaultAction").toInt();
    switch (mode) {
    case Normal:
        a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateSlot"));
        a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateArcSlot"));
        getAction()->setIcon(a[index]->icon());
        break;
    case Construction:
        a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateSlot_Constr"));
        a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateArcSlot_Constr"));
        getAction()->setIcon(a[index]->icon());
        break;
    }
}

void CmdSketcherCompCreateSlot::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* twoPointsSlot = a[0];
    twoPointsSlot->setText(QApplication::translate("CmdSketcherCompCreateSlot", "Slot"));
    twoPointsSlot->setToolTip(QApplication::translate("Sketcher_CreateTriangle", "Create a slot by its two center points and radius point"));
    twoPointsSlot->setStatusTip(QApplication::translate("Sketcher_CreateTriangle", "Create a slot by its two center points and radius point"));
    QAction* arcSlot = a[1];
    arcSlot->setText(QApplication::translate("CmdSketcherCompCreateSlot", "Arc slot"));
    arcSlot->setToolTip(QApplication::translate("Sketcher_CreateSquare", "Create a slot by its arc center first"));
    arcSlot->setStatusTip(QApplication::translate("Sketcher_CreateSquare", "Create a slot by its arc center first"));
}

bool CmdSketcherCompCreateSlot::isActive(void)
{
    return isCommandActive(getActiveGuiDocument());
}

/*=========================================================================*/

void CreateSketcherCommandsCreateGeo(void)
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
    rcCmdMgr.addCommand(new CmdSketcherCreatePolygon());
    rcCmdMgr.addCommand(new CmdSketcherCreateTriangle());
    rcCmdMgr.addCommand(new CmdSketcherCreateSquare());
    rcCmdMgr.addCommand(new CmdSketcherCreatePentagon());
    rcCmdMgr.addCommand(new CmdSketcherCreateHexagon());
    rcCmdMgr.addCommand(new CmdSketcherCreateHeptagon());
    rcCmdMgr.addCommand(new CmdSketcherCreateOctagon());
    rcCmdMgr.addCommand(new CmdSketcherCompCreateRectangles());
    rcCmdMgr.addCommand(new CmdSketcherCompCreateSlot());
    rcCmdMgr.addCommand(new CmdSketcherCreateSlot());
    rcCmdMgr.addCommand(new CmdSketcherCreateArcSlot());
    rcCmdMgr.addCommand(new CmdSketcherCreateFillet());
    rcCmdMgr.addCommand(new CmdSketcherCompModifyEdge());
    rcCmdMgr.addCommand(new CmdSketcherTrimming());
    rcCmdMgr.addCommand(new CmdSketcherExtend());
    rcCmdMgr.addCommand(new CmdSketcherSplit());
    rcCmdMgr.addCommand(new CmdSketcherInsert());
    rcCmdMgr.addCommand(new CmdSketcherExternal());
    rcCmdMgr.addCommand(new CmdSketcherCarbonCopy());
}
