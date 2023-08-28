/***************************************************************************
 *   Copyright (c) 2014 Luke Parry <l.parry@warwick.ac.uk>                 *
 *   Copyright (c) 2022 WandererFan <wandererfan@gmail.com>                *
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
#include <string>
#include <vector>

#include <QApplication>
#include <QMessageBox>
#endif//#ifndef _PreComp_

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/SelectionObject.h>
#include <Mod/TechDraw/App/DrawDimHelper.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawViewDimension.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/Geometry.h>
#include <Mod/TechDraw/App/LandmarkDimension.h>
#include <Mod/TechDraw/App/Preferences.h>

#include "DimensionValidators.h"
#include "DrawGuiUtil.h"
#include "TaskDimRepair.h"
#include "TaskLinkDim.h"


using namespace TechDrawGui;
using namespace TechDraw;
using namespace std;


//===========================================================================
// utility routines
//===========================================================================

//internal functions
bool _checkSelection(Gui::Command* cmd, unsigned maxObjs = 2);
bool _checkDrawViewPart(Gui::Command* cmd);

void execDistance(Gui::Command* cmd);
void execDistanceX(Gui::Command* cmd);
void execDistanceY(Gui::Command* cmd);
void execAngle(Gui::Command* cmd);
void execAngle3Pt(Gui::Command* cmd);
void execRadius(Gui::Command* cmd);
void execDiameter(Gui::Command* cmd);

void execExtent(Gui::Command* cmd, int direction);

DrawViewDimension* dimensionMaker(TechDraw::DrawViewPart* dvp, std::string dimType,
                                  ReferenceVector references2d, ReferenceVector references3d);

void positionDimText(DrawViewDimension* dim);

//NOTE: this is not shown in toolbar and doesn't always work right in the menu.
//      should be removed.
//===========================================================================
// TechDraw_NewDimension
//===========================================================================

// this is deprecated. use individual add dimension commands.

DEF_STD_CMD_A(CmdTechDrawDimension)

CmdTechDrawDimension::CmdTechDrawDimension()
    : Command("TechDraw_Dimension")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Dimension");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_Dimension";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_Dimension";
}

void CmdTechDrawDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
}

bool CmdTechDrawDimension::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_RadiusDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawRadiusDimension)

CmdTechDrawRadiusDimension::CmdTechDrawRadiusDimension()
    : Command("TechDraw_RadiusDimension")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Radius Dimension");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_RadiusDimension";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_RadiusDimension";
}

void CmdTechDrawRadiusDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Task In Progress"),
                             QObject::tr("Close active task dialog and try again."));
        return;
    }

    execRadius(this);
}

bool CmdTechDrawRadiusDimension::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

void execRadius(Gui::Command* cmd)
{
    bool result = _checkDrawViewPart(cmd);
    if (!result) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect selection"),
                             QObject::tr("No View of a Part in selection."));
        return;
    }

    ReferenceVector references2d;
    ReferenceVector references3d;
    TechDraw::DrawViewPart* partFeat =
        TechDraw::getReferencesFromSelection(references2d, references3d);

    //Define the geometric configuration required for a radius dimension
    StringVector acceptableGeometry({"Edge"});
    std::vector<int> minimumCounts({1});
    std::vector<DimensionGeometryType> acceptableDimensionGeometrys(
        {isCircle, isEllipse, isBSplineCircle});

    //what 2d geometry configuration did we receive?
    DimensionGeometryType geometryRefs2d = validateDimSelection(
        references2d, acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);
    if (geometryRefs2d == TechDraw::isInvalid) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect Selection"),
                             QObject::tr("Can not make 2d radius dimension from selection"));
        return;
    }

    //what 3d geometry configuration did we receive?
    DimensionGeometryType geometryRefs3d(isInvalid);
    if (geometryRefs2d == TechDraw::isViewReference && !references3d.empty()) {
        geometryRefs3d = validateDimSelection3d(partFeat,
                                                references3d,
                                                acceptableGeometry,
                                                minimumCounts,
                                                acceptableDimensionGeometrys);
        if (geometryRefs3d == TechDraw::isInvalid) {
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr("Incorrect Selection"),
                                 QObject::tr("Can not make 3d radius dimension from selection"));
            return;
        }
    }

    //errors and warnings
    if (geometryRefs2d == isEllipse || geometryRefs3d == isEllipse) {
        QMessageBox::StandardButton result = QMessageBox::warning(
            Gui::getMainWindow(),
            QObject::tr("Ellipse Curve Warning"),
            QObject::tr("Selected edge is an Ellipse.  Radius will be approximate. Continue?"),
            QMessageBox::Ok | QMessageBox::Cancel,
            QMessageBox::Cancel);
        if (result != QMessageBox::Ok) {
            return;
        }
    }
    if (geometryRefs2d == isBSplineCircle || geometryRefs3d == isBSplineCircle) {
        QMessageBox::StandardButton result = QMessageBox::warning(
            Gui::getMainWindow(),
            QObject::tr("BSpline Curve Warning"),
            QObject::tr("Selected edge is a BSpline.  Radius will be approximate. Continue?"),
            QMessageBox::Ok | QMessageBox::Cancel,
            QMessageBox::Cancel);
        if (result != QMessageBox::Ok) {
            return;
        }
    }
    if (geometryRefs2d == isBSpline || geometryRefs3d == isBSpline) {
        QMessageBox::critical(
            Gui::getMainWindow(),
            QObject::tr("BSpline Curve Error"),
            QObject::tr("Selected edge is a BSpline and a radius can not be calculated."));
        return;
    }

    //build the dimension
    //    DrawViewDimension* dim =
    dimensionMaker(partFeat, "Radius", references2d, references3d);

    //Horrible hack to force Tree update
    double x = partFeat->X.getValue();
    partFeat->X.setValue(x);
}

//===========================================================================
// TechDraw_DiameterDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawDiameterDimension)

CmdTechDrawDiameterDimension::CmdTechDrawDiameterDimension()
    : Command("TechDraw_DiameterDimension")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Diameter Dimension");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_DiameterDimension";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_DiameterDimension";
}

void CmdTechDrawDiameterDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Task In Progress"),
                             QObject::tr("Close active task dialog and try again."));
        return;
    }

    execDiameter(this);
}

bool CmdTechDrawDiameterDimension::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

void execDiameter(Gui::Command* cmd)
{
    bool result = _checkDrawViewPart(cmd);
    if (!result) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect selection"),
                             QObject::tr("No View of a Part in selection."));
        return;
    }

    ReferenceVector references2d;
    ReferenceVector references3d;
    TechDraw::DrawViewPart* partFeat =
        TechDraw::getReferencesFromSelection(references2d, references3d);

    //Define the geometric configuration required for a diameter dimension
    StringVector acceptableGeometry({"Edge"});
    std::vector<int> minimumCounts({1});
    std::vector<DimensionGeometryType> acceptableDimensionGeometrys(
        {isCircle, isEllipse, isBSplineCircle});

    //what 2d geometry configuration did we receive?
    DimensionGeometryType geometryRefs2d = validateDimSelection(
        references2d, acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);
    if (geometryRefs2d == TechDraw::isInvalid) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect Selection"),
                             QObject::tr("Can not make 2d diameter dimension from selection"));
        return;
    }

    //what 3d geometry configuration did we receive?
    DimensionGeometryType geometryRefs3d(isInvalid);
    if (geometryRefs2d == TechDraw::isViewReference && !references3d.empty()) {
        geometryRefs3d = validateDimSelection3d(partFeat,
                                                references3d,
                                                acceptableGeometry,
                                                minimumCounts,
                                                acceptableDimensionGeometrys);
        if (geometryRefs3d == TechDraw::isInvalid) {
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr("Incorrect Selection"),
                                 QObject::tr("Can not make 3d diameter dimension from selection"));
            return;
        }
    }

    //errors and warnings
    if (geometryRefs2d == isEllipse || geometryRefs3d == isEllipse) {
        QMessageBox::StandardButton result = QMessageBox::warning(
            Gui::getMainWindow(),
            QObject::tr("Ellipse Curve Warning"),
            QObject::tr("Selected edge is an Ellipse.  Diameter will be approximate. Continue?"),
            QMessageBox::Ok | QMessageBox::Cancel,
            QMessageBox::Cancel);
        if (result != QMessageBox::Ok) {
            return;
        }
    }
    if (geometryRefs2d == isBSplineCircle || geometryRefs3d == isBSplineCircle) {
        QMessageBox::StandardButton result = QMessageBox::warning(
            Gui::getMainWindow(),
            QObject::tr("BSpline Curve Warning"),
            QObject::tr("Selected edge is a BSpline.  Diameter will be approximate. Continue?"),
            QMessageBox::Ok | QMessageBox::Cancel,
            QMessageBox::Cancel);
        if (result != QMessageBox::Ok) {
            return;
        }
    }
    if (geometryRefs2d == isBSpline || geometryRefs3d == isBSpline) {
        QMessageBox::critical(
            Gui::getMainWindow(),
            QObject::tr("BSpline Curve Error"),
            QObject::tr("Selected edge is a BSpline and a diameter can not be calculated."));
        return;
    }

    //build the dimension
    //    DrawViewDimension* dim =
    dimensionMaker(partFeat, "Diameter", references2d, references3d);

    //Horrible hack to force Tree update
    double x = partFeat->X.getValue();
    partFeat->X.setValue(x);
}

//===========================================================================
// TechDraw_LengthDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawLengthDimension)

CmdTechDrawLengthDimension::CmdTechDrawLengthDimension()
    : Command("TechDraw_LengthDimension")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Length Dimension");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_LengthDimension";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_LengthDimension";
}

void CmdTechDrawLengthDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Task In Progress"),
                             QObject::tr("Close active task dialog and try again."));
        return;
    }

    execDistance(this);
}

bool CmdTechDrawLengthDimension::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

void execDistance(Gui::Command* cmd)
{
    bool result = _checkDrawViewPart(cmd);
    if (!result) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect selection"),
                             QObject::tr("No View of a Part in selection."));
        return;
    }

    ReferenceVector references2d;
    ReferenceVector references3d;
    TechDraw::DrawViewPart* partFeat =
        TechDraw::getReferencesFromSelection(references2d, references3d);

    //Define the geometric configuration required for a length dimension
    StringVector acceptableGeometry({"Edge", "Vertex"});
    std::vector<int> minimumCounts({1, 2});
    std::vector<DimensionGeometryType> acceptableDimensionGeometrys(
        {isVertical, isHorizontal, isDiagonal});

    //what 2d geometry configuration did we receive?
    DimensionGeometryType geometryRefs2d = validateDimSelection(
        references2d, acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);

    if (geometryRefs2d == TechDraw::isInvalid) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect Selection"),
                             QObject::tr("Can not make 2d linear dimension from selection"));
        return;
    }

    //what 3d geometry configuration did we receive?
    DimensionGeometryType geometryRefs3d;
    if (geometryRefs2d == TechDraw::isViewReference && !references3d.empty()) {
        geometryRefs3d = validateDimSelection3d(partFeat,
                                                references3d,
                                                acceptableGeometry,
                                                minimumCounts,
                                                acceptableDimensionGeometrys);

        if (geometryRefs3d == TechDraw::isInvalid) {
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr("Incorrect Selection"),
                                 QObject::tr("Can not make 3d linear dimension from selection"));
            return;
        }
    }

    //build the dimension
    DrawViewDimension* dim = dimensionMaker(partFeat, "Distance", references2d, references3d);

    //position the Dimension text on the view
    positionDimText(dim);

    //Horrible hack to force Tree update (claimChildren)
    double x = partFeat->X.getValue();
    partFeat->X.setValue(x);
}

//===========================================================================
// TechDraw_HorizontalDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawHorizontalDimension)

CmdTechDrawHorizontalDimension::CmdTechDrawHorizontalDimension()
    : Command("TechDraw_HorizontalDimension")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Horizontal Dimension");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_HorizontalDimension";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_HorizontalDimension";
    sAccel = "SHIFT+H";
}

void CmdTechDrawHorizontalDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Task In Progress"),
                             QObject::tr("Close active task dialog and try again."));
        return;
    }

    execDistanceX(this);
}

bool CmdTechDrawHorizontalDimension::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

void execDistanceX(Gui::Command* cmd)
{
    bool result = _checkDrawViewPart(cmd);
    if (!result) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect selection"),
                             QObject::tr("No View of a Part in selection."));
        return;
    }

    ReferenceVector references2d;
    ReferenceVector references3d;
    TechDraw::DrawViewPart* partFeat =
        TechDraw::getReferencesFromSelection(references2d, references3d);

    //Define the geometric configuration required for a length dimension
    StringVector acceptableGeometry({"Edge", "Vertex"});
    std::vector<int> minimumCounts({1, 2});
    std::vector<DimensionGeometryType> acceptableDimensionGeometrys({isHorizontal, isDiagonal});

    //what 2d geometry configuration did we receive?
    DimensionGeometryType geometryRefs2d = validateDimSelection(
        references2d, acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);
    if (geometryRefs2d == TechDraw::isInvalid) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect Selection"),
                             QObject::tr("Can not make 2d horizontal dimension from selection"));
        return;
    }

    //what 3d geometry configuration did we receive?
    DimensionGeometryType geometryRefs3d;
    if (geometryRefs2d == TechDraw::isViewReference && !references3d.empty()) {
        geometryRefs3d = validateDimSelection3d(partFeat,
                                                references3d,
                                                acceptableGeometry,
                                                minimumCounts,
                                                acceptableDimensionGeometrys);
        if (geometryRefs3d == TechDraw::isInvalid) {
            QMessageBox::warning(
                Gui::getMainWindow(),
                QObject::tr("Incorrect Selection"),
                QObject::tr("Can not make 3d horizontal dimension from selection"));
            return;
        }
    }

    //build the dimension
    DrawViewDimension* dim = dimensionMaker(partFeat, "DistanceX", references2d, references3d);


    //position the Dimension text on the view
    positionDimText(dim);

    //Horrible hack to force Tree update (claimChildren)
    double x = partFeat->X.getValue();
    partFeat->X.setValue(x);
}

//===========================================================================
// TechDraw_VerticalDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawVerticalDimension)

CmdTechDrawVerticalDimension::CmdTechDrawVerticalDimension()
    : Command("TechDraw_VerticalDimension")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Vertical Dimension");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_VerticalDimension";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_VerticalDimension";
    sAccel = "SHIFT+V";
}

void CmdTechDrawVerticalDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Task In Progress"),
                             QObject::tr("Close active task dialog and try again."));
        return;
    }

    execDistanceY(this);
}

bool CmdTechDrawVerticalDimension::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

void execDistanceY(Gui::Command* cmd)
{
    bool result = _checkDrawViewPart(cmd);
    if (!result) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect selection"),
                             QObject::tr("No View of a Part in selection."));
        return;
    }

    ReferenceVector references2d;
    ReferenceVector references3d;
    TechDraw::DrawViewPart* partFeat =
        TechDraw::getReferencesFromSelection(references2d, references3d);

    //Define the geometric configuration required for a length dimension
    StringVector acceptableGeometry({"Edge", "Vertex"});
    std::vector<int> minimumCounts({1, 2});
    std::vector<DimensionGeometryType> acceptableDimensionGeometrys({isVertical, isDiagonal});

    //what 2d geometry configuration did we receive?
    DimensionGeometryType geometryRefs2d = validateDimSelection(
        references2d, acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);
    if (geometryRefs2d == TechDraw::isInvalid) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect Selection"),
                             QObject::tr("Can not make 2d vertical dimension from selection"));
        return;
    }

    //what 3d geometry configuration did we receive?
    DimensionGeometryType geometryRefs3d;
    if (geometryRefs2d == TechDraw::isViewReference && !references3d.empty()) {
        geometryRefs3d = validateDimSelection3d(partFeat,
                                                references3d,
                                                acceptableGeometry,
                                                minimumCounts,
                                                acceptableDimensionGeometrys);
        if (geometryRefs3d == TechDraw::isVertical) {
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr("Incorrect Selection"),
                                 QObject::tr("Can not make 3d vertical dimension from selection"));
            return;
        }
    }

    //build the dimension
    DrawViewDimension* dim = dimensionMaker(partFeat, "DistanceY", references2d, references3d);

    //position the Dimension text on the view
    positionDimText(dim);

    //Horrible hack to force Tree update (claimChildren)
    double x = partFeat->X.getValue();
    partFeat->X.setValue(x);
}

//===========================================================================
// TechDraw_AngleDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawAngleDimension)

CmdTechDrawAngleDimension::CmdTechDrawAngleDimension()
    : Command("TechDraw_AngleDimension")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Angle Dimension");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_AngleDimension";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_AngleDimension";
}

void CmdTechDrawAngleDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Task In Progress"),
                             QObject::tr("Close active task dialog and try again."));
        return;
    }

    execAngle(this);
}

bool CmdTechDrawAngleDimension::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

void execAngle(Gui::Command* cmd)
{
    bool result = _checkDrawViewPart(cmd);
    if (!result) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect selection"),
                             QObject::tr("No View of a Part in selection."));
        return;
    }

    ReferenceVector references2d;
    ReferenceVector references3d;
    TechDraw::DrawViewPart* partFeat =
        TechDraw::getReferencesFromSelection(references2d, references3d);

    //Define the geometric configuration required for a length dimension
    StringVector acceptableGeometry({"Edge"});
    std::vector<int> minimumCounts({2});
    std::vector<DimensionGeometryType> acceptableDimensionGeometrys({isAngle});

    //what 2d geometry configuration did we receive?
    DimensionGeometryType geometryRefs2d = validateDimSelection(
        references2d, acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);
    if (geometryRefs2d == TechDraw::isInvalid) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect Selection"),
                             QObject::tr("Can not make 2d angle dimension from selection"));
        return;
    }

    //what 3d geometry configuration did we receive?
    DimensionGeometryType geometryRefs3d;
    if (geometryRefs2d == TechDraw::isViewReference && !references3d.empty()) {
        geometryRefs3d = validateDimSelection3d(partFeat,
                                                references3d,
                                                acceptableGeometry,
                                                minimumCounts,
                                                acceptableDimensionGeometrys);
        if (geometryRefs3d == TechDraw::isInvalid) {
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr("Incorrect Selection"),
                                 QObject::tr("Can not make 3d angle dimension from selection"));
            return;
        }
    }

    //build the dimension
    DrawViewDimension* dim = dimensionMaker(partFeat, "Angle", references2d, references3d);

    //position the Dimension text on the view
    positionDimText(dim);

    //Horrible hack to force Tree update (claimChildren)
    double x = partFeat->X.getValue();
    partFeat->X.setValue(x);
}

//===========================================================================
// TechDraw_3PtAngleDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDraw3PtAngleDimension)

CmdTechDraw3PtAngleDimension::CmdTechDraw3PtAngleDimension()
    : Command("TechDraw_3PtAngleDimension")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert 3-Point Angle Dimension");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_3PtAngleDimension";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_3PtAngleDimension";
}

void CmdTechDraw3PtAngleDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Task In Progress"),
                             QObject::tr("Close active task dialog and try again."));
        return;
    }

    execAngle3Pt(this);
}

bool CmdTechDraw3PtAngleDimension::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

void execAngle3Pt(Gui::Command* cmd)
{
    bool result = _checkDrawViewPart(cmd);
    if (!result) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect selection"),
                             QObject::tr("No View of a Part in selection."));
        return;
    }

    ReferenceVector references2d;
    ReferenceVector references3d;
    TechDraw::DrawViewPart* partFeat =
        TechDraw::getReferencesFromSelection(references2d, references3d);

    //Define the geometric configuration required for a length dimension
    StringVector acceptableGeometry({"Vertex"});
    std::vector<int> minimumCounts({3});
    std::vector<DimensionGeometryType> acceptableDimensionGeometrys({isAngle3Pt});

    //what 2d geometry configuration did we receive?
    DimensionGeometryType geometryRefs2d = validateDimSelection(
        references2d, acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);
    if (geometryRefs2d == TechDraw::isInvalid) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect Selection"),
                             QObject::tr("Can not make 2d angle dimension from selection"));
        return;
    }

    //what 3d geometry configuration did we receive?
    DimensionGeometryType geometryRefs3d;
    if (geometryRefs2d == TechDraw::isViewReference && !references3d.empty()) {
        geometryRefs3d = validateDimSelection3d(partFeat,
                                                references3d,
                                                acceptableGeometry,
                                                minimumCounts,
                                                acceptableDimensionGeometrys);
        if (geometryRefs3d == TechDraw::isInvalid) {
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr("Incorrect Selection"),
                                 QObject::tr("Can not make 3d angle dimension from selection"));
            return;
        }
    }

    //build the dimension
    DrawViewDimension* dim = dimensionMaker(partFeat, "Angle3Pt", references2d, references3d);
    //position the Dimension text on the view
    positionDimText(dim);

    //Horrible hack to force Tree update (claimChildren)
    double x = partFeat->X.getValue();
    partFeat->X.setValue(x);
}

//! link 3D geometry to Dimension(s) on a Page
//TODO: should we present all potential Dimensions from all Pages?
//===========================================================================
// TechDraw_LinkDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawLinkDimension)

CmdTechDrawLinkDimension::CmdTechDrawLinkDimension()
    : Command("TechDraw_LinkDimension")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Link Dimension to 3D Geometry");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_LinkDimension";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_LinkDimension";
}

void CmdTechDrawLinkDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    TechDraw::DrawPage* page = DrawGuiUtil::findPage(this);
    if (!page) {
        return;
    }

    bool result = _checkSelection(this, 2);
    if (!result) {
        return;
    }

    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx(
        nullptr, App::DocumentObject::getClassTypeId(), Gui::ResolveMode::NoResolve);

    App::DocumentObject* obj3D = nullptr;
    std::vector<App::DocumentObject*> parts;
    std::vector<std::string> subs;

    std::vector<Gui::SelectionObject>::iterator itSel = selection.begin();
    for (; itSel != selection.end(); itSel++) {
        obj3D = ((*itSel).getObject());
        std::vector<std::string> subList = (*itSel).getSubNames();
        for (auto& s : subList) {
            parts.push_back(obj3D);
            subs.push_back(s);
        }
    }

    if (!obj3D) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect Selection"),
                             QObject::tr("There is no 3D object in your selection"));
        return;
    }

    if (subs.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect Selection"),
                             QObject::tr("There are no 3D Edges or Vertices in your selection"));
        return;
    }


    // dialog to select the Dimension to link
    Gui::Control().showDialog(new TaskDlgLinkDim(parts, subs, page));

    page->getDocument()->recompute();//still need to recompute in Gui. why?
}

bool CmdTechDrawLinkDimension::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    bool taskInProgress = false;
    if (havePage) {
        taskInProgress = Gui::Control().activeDialog();
    }
    return (havePage && haveView && !taskInProgress);
}

//===========================================================================
// TechDraw_ExtentGroup
//===========================================================================

DEF_STD_CMD_ACL(CmdTechDrawExtentGroup)

CmdTechDrawExtentGroup::CmdTechDrawExtentGroup()
    : Command("TechDraw_ExtentGroup")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Extent Dimension");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_ExtentGroup";
    sStatusTip = sToolTipText;
    //    eType           = ForEdit;
}

void CmdTechDrawExtentGroup::activated(int iMsg)
{
    //    Base::Console().Message("CMD::ExtentGrp - activated(%d)\n", iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Task In Progress"),
                             QObject::tr("Close active task dialog and try again."));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch (iMsg) {
        case 0:
            execExtent(this, 0);
            break;
        case 1:
            execExtent(this, 1);
            break;
        default:
            Base::Console().Message("CMD::ExtGrp - invalid iMsg: %d\n", iMsg);
    };
}

Gui::Action* CmdTechDrawExtentGroup::createAction()
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_HorizontalExtentDimension"));
    p1->setObjectName(QString::fromLatin1("TechDraw_HorizontalExtentDimension"));
    p1->setWhatsThis(QString::fromLatin1("TechDraw_HorizontalExtentDimension"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_VerticalExtentDimension"));
    p2->setObjectName(QString::fromLatin1("TechDraw_VerticalExtentDimension"));
    p2->setWhatsThis(QString::fromLatin1("TechDraw_VerticalExtentDimension"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(p1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdTechDrawExtentGroup::languageChange()
{
    Command::languageChange();

    if (!_pcAction) {
        return;
    }
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* arc1 = a[0];
    arc1->setText(QApplication::translate("CmdTechDrawExtentGroup", "Horizontal Extent"));
    arc1->setToolTip(
        QApplication::translate("TechDraw_HorizontalExtent", "Insert Horizontal Extent Dimension"));
    arc1->setStatusTip(arc1->toolTip());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("CmdTechDrawExtentGroup", "Vertical Extent"));
    arc2->setToolTip(QApplication::translate("TechDraw_VerticalExtentDimension",
                                             "Insert Vertical Extent Dimension"));
    arc2->setStatusTip(arc2->toolTip());
}

bool CmdTechDrawExtentGroup::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_HorizontalExtentDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawHorizontalExtentDimension)

CmdTechDrawHorizontalExtentDimension::CmdTechDrawHorizontalExtentDimension()
    : Command("TechDraw_HorizontalExtentDimension")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Horizontal Extent Dimension");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_HorizontalExtentDimension";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_HorizontalExtentDimension";
}

void CmdTechDrawHorizontalExtentDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Task In Progress"),
                             QObject::tr("Close active task dialog and try again."));
        return;
    }

    execExtent(this, 0);
}

bool CmdTechDrawHorizontalExtentDimension::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
}

void execExtent(Gui::Command* cmd, int direction)
{
    bool result = _checkDrawViewPart(cmd);
    if (!result) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect selection"),
                             QObject::tr("No View of a Part in selection."));
        return;
    }

    ReferenceVector references2d;
    ReferenceVector references3d;
    TechDraw::DrawViewPart* partFeat =
        TechDraw::getReferencesFromSelection(references2d, references3d);

    //Define the geometric configuration required for a extent dimension
    StringVector acceptableGeometry({"Edge"});
    std::vector<int> minimumCounts({1});
    std::vector<DimensionGeometryType> acceptableDimensionGeometrys({isMultiEdge,
                                                                     isHorizontal,
                                                                     isVertical,
                                                                     isDiagonal,
                                                                     isCircle,
                                                                     isEllipse,
                                                                     isBSplineCircle,
                                                                     isBSpline,
                                                                     isZLimited});

    //what 2d geometry configuration did we receive?
    DimensionGeometryType geometryRefs2d = validateDimSelection(
        references2d, acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);
    if (geometryRefs2d == TechDraw::isInvalid) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect Selection"),
                             QObject::tr("Can not make 2d extent dimension from selection"));
        return;
    }
    //what 3d geometry configuration did we receive?
    DimensionGeometryType geometryRefs3d;
    if (geometryRefs2d == TechDraw::isViewReference && !references3d.empty()) {
        geometryRefs3d = validateDimSelection3d(partFeat,
                                                references3d,
                                                acceptableGeometry,
                                                minimumCounts,
                                                acceptableDimensionGeometrys);
        if (geometryRefs3d == isInvalid) {
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr("Incorrect Selection"),
                                 QObject::tr("Can not make 3d extent dimension from selection"));
            return;
        }
    }

    if (references3d.empty()) {
        std::vector<std::string> edgeNames;
        for (auto& ref : references2d) {
            if (ref.getSubName().empty()) {
                continue;
            }
            std::string geomType = DrawUtil::getGeomTypeFromName(ref.getSubName());
            if (geomType == "Edge") {
                edgeNames.push_back(ref.getSubName());
            }
        }
        DrawDimHelper::makeExtentDim(partFeat, edgeNames,
                                     direction);//0 - horizontal, 1 - vertical
    } else {
        DrawDimHelper::makeExtentDim3d(partFeat,
                                       references3d,
                                       direction);//0 - horizontal, 1 - vertical
    }
}

//===========================================================================
// TechDraw_VerticalExtentDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawVerticalExtentDimension)

CmdTechDrawVerticalExtentDimension::CmdTechDrawVerticalExtentDimension()
    : Command("TechDraw_VerticalExtentDimension")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Vertical Extent Dimension");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_VerticalExtentDimension";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_VerticalExtentDimension";
}

void CmdTechDrawVerticalExtentDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Task In Progress"),
                             QObject::tr("Close active task dialog and try again."));
        return;
    }

    execExtent(this, 1);
}

bool CmdTechDrawVerticalExtentDimension::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this, false);
    return (havePage && haveView);
}

//===========================================================================
// TechDraw_DimensionRepair
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawDimensionRepair)

CmdTechDrawDimensionRepair::CmdTechDrawDimensionRepair()
    : Command("TechDraw_DimensionRepair")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Repair Dimension References");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_DimensionRepair";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_DimensionRepair";
}

void CmdTechDrawDimensionRepair::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    std::vector<App::DocumentObject*> dimObjs =
        getSelection().getObjectsOfType(TechDraw::DrawViewDimension::getClassTypeId());
    TechDraw::DrawViewDimension* dim = nullptr;
    if (dimObjs.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect Selection"),
                             QObject::tr("There is no Dimension in your selection"));
        return;
    } else {
        dim = static_cast<TechDraw::DrawViewDimension*>(dimObjs.at(0));
    }

    //    ReferenceVector references2d;
    //    ReferenceVector references3d;
    //    //TechDraw::DrawViewPart* partFeat =
    //    TechDraw::getReferencesFromSelection(references2d, references3d);

    //    Gui::Control().showDialog(new TaskDlgDimReference(dim, references2d, references3d));
    Gui::Control().showDialog(new TaskDlgDimReference(dim));
}

bool CmdTechDrawDimensionRepair::isActive(void)
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    bool taskInProgress = false;
    if (havePage) {
        taskInProgress = Gui::Control().activeDialog();
    }
    return (havePage && haveView && !taskInProgress);
}

//NOTE: to be deprecated.  revisions to the basic dimension allows it to handle
//everything that the Landmark Dimension was created to handle.
//===========================================================================
// TechDraw_LandmarkDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawLandmarkDimension)

CmdTechDrawLandmarkDimension::CmdTechDrawLandmarkDimension()
    : Command("TechDraw_LandmarkDimension")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Insert Landmark Dimension - EXPERIMENTAL");
    sToolTipText = sMenuText;
    sWhatsThis = "TechDraw_LandmarkDimension";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_LandmarkDimension";
}

void CmdTechDrawLandmarkDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    bool result = _checkSelection(this, 3);
    if (!result) {
        return;
    }

    const std::vector<App::DocumentObject*> objects =
        getSelection().getObjectsOfType(Part::Feature::getClassTypeId());//??
    if (objects.size() != 2) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Wrong selection"),
                             QObject::tr("Select 2 point objects and 1 View. (1)"));
        return;
    }

    const std::vector<App::DocumentObject*> views =
        getSelection().getObjectsOfType(TechDraw::DrawViewPart::getClassTypeId());
    if (views.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Wrong selection"),
                             QObject::tr("Select 2 point objects and 1 View. (2)"));
        return;
    }

    TechDraw::DrawViewPart* dvp = static_cast<TechDraw::DrawViewPart*>(views.front());

    std::vector<App::DocumentObject*> refs2d;

    std::vector<std::string> subs;
    subs.emplace_back("Vertex1");
    subs.emplace_back("Vertex1");
    TechDraw::DrawPage* page = dvp->findParentPage();
    std::string parentName = dvp->getNameInDocument();
    std::string PageName = page->getNameInDocument();

    TechDraw::LandmarkDimension* dim = nullptr;
    std::string FeatName = getUniqueObjectName("LandmarkDim");

    openCommand(QT_TRANSLATE_NOOP("Command", "Create Dimension"));
    doCommand(Doc,
              "App.activeDocument().addObject('TechDraw::LandmarkDimension', '%s')",
              FeatName.c_str());
    doCommand(Doc, "App.activeDocument().%s.translateLabel('LandmarkDimension', 'LandmarkDim', '%s')",
              FeatName.c_str(), FeatName.c_str());

    if (objects.size() == 2) {
        //what about distanceX and distanceY??
        doCommand(Doc, "App.activeDocument().%s.Type = '%s'", FeatName.c_str(), "Distance");
        refs2d.push_back(dvp);
        refs2d.push_back(dvp);
    }
    //    } else if (objects.size() == 3) {             //not implemented yet
    //        doCommand(Doc, "App.activeDocument().%s.Type = '%s'", FeatName.c_str(), "Angle3Pt");
    //        refs2d.push_back(dvp);
    //        refs2d.push_back(dvp);
    //        refs2d.push_back(dvp);
    //        //subs.push_back("Vertex1");
    //        //subs.push_back("Vertex1");
    //        //subs.push_back("Vertex1");
    //    }

    dim = dynamic_cast<TechDraw::LandmarkDimension*>(getDocument()->getObject(FeatName.c_str()));
    if (!dim) {
        throw Base::TypeError("CmdTechDrawLandmarkDimension - dim not found\n");
    }
    dim->References2D.setValues(refs2d, subs);
    dim->References3D.setValues(objects, subs);
    doCommand(Doc,
              "App.activeDocument().%s.addView(App.activeDocument().%s)",
              PageName.c_str(),
              FeatName.c_str());

    commitCommand();
    dim->recomputeFeature();

    //Horrible hack to force Tree update
    double x = dvp->X.getValue();
    dvp->X.setValue(x);
}

bool CmdTechDrawLandmarkDimension::isActive()
{
    bool havePage = DrawGuiUtil::needPage(this);
    bool haveView = DrawGuiUtil::needView(this);
    return (havePage && haveView);
}

//------------------------------------------------------------------------------

void CreateTechDrawCommandsDims()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdTechDrawDimension());
    rcCmdMgr.addCommand(new CmdTechDrawRadiusDimension());
    rcCmdMgr.addCommand(new CmdTechDrawDiameterDimension());
    rcCmdMgr.addCommand(new CmdTechDrawLengthDimension());
    rcCmdMgr.addCommand(new CmdTechDrawHorizontalDimension());
    rcCmdMgr.addCommand(new CmdTechDrawVerticalDimension());
    rcCmdMgr.addCommand(new CmdTechDrawAngleDimension());
    rcCmdMgr.addCommand(new CmdTechDraw3PtAngleDimension());
    rcCmdMgr.addCommand(new CmdTechDrawExtentGroup());
    rcCmdMgr.addCommand(new CmdTechDrawVerticalExtentDimension());
    rcCmdMgr.addCommand(new CmdTechDrawHorizontalExtentDimension());
    rcCmdMgr.addCommand(new CmdTechDrawLinkDimension());
    rcCmdMgr.addCommand(new CmdTechDrawLandmarkDimension());
    rcCmdMgr.addCommand(new CmdTechDrawDimensionRepair());
}

//------------------------------------------------------------------------------

//Common code to build a dimension feature
DrawViewDimension* dimensionMaker(TechDraw::DrawViewPart* dvp, std::string dimType,
                                  ReferenceVector references2d, ReferenceVector references3d)
{
    TechDraw::DrawPage* page = dvp->findParentPage();
    std::string PageName = page->getNameInDocument();

    TechDraw::DrawViewDimension* dim = nullptr;
    std::string dimName = dvp->getDocument()->getUniqueObjectName("Dimension");

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create Dimension"));
    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.activeDocument().addObject('TechDraw::DrawViewDimension', '%s')",
                            dimName.c_str());
    Gui::Command::doCommand(Gui::Command::Doc, "App.activeDocument().%s.translateLabel('DrawViewDimension', 'Dimension', '%s')",
              dimName.c_str(), dimName.c_str());

    Gui::Command::doCommand(
        Gui::Command::Doc, "App.activeDocument().%s.Type = '%s'", dimName.c_str(), dimType.c_str());

    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.activeDocument().%s.MeasureType = '%s'",
                            dimName.c_str(),
                            "Projected");

    dim =
        dynamic_cast<TechDraw::DrawViewDimension*>(dvp->getDocument()->getObject(dimName.c_str()));
    if (!dim) {
        throw Base::TypeError("CmdTechDrawNewDiameterDimension - dim not found\n");
    }

    //always have References2D, even if only for the parent DVP
    dim->setReferences2d(references2d);
    dim->setReferences3d(references3d);

    Gui::Command::doCommand(Gui::Command::Doc,
                            "App.activeDocument().%s.addView(App.activeDocument().%s)",
                            PageName.c_str(),
                            dimName.c_str());

    Gui::Command::commitCommand();
    dim->recomputeFeature();
    return dim;
}

//position the Dimension text on the view
void positionDimText(DrawViewDimension* dim)
{
    TechDraw::pointPair pp = dim->getLinearPoints();
    Base::Vector3d mid = (pp.first() + pp.second()) / 2.0;
    dim->X.setValue(mid.x);
    double fontSize = Preferences::dimFontSizeMM();
    dim->Y.setValue(-mid.y + 0.5 * fontSize);
}
//===========================================================================
// Selection Validation Helpers
//===========================================================================

//! common checks of Selection for Dimension commands
//non-empty selection, no more than maxObjs selected and at least 1 DrawingPage exists
bool _checkSelection(Gui::Command* cmd, unsigned maxObjs)
{
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    if (selection.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect selection"),
                             QObject::tr("Select an object first"));
        return false;
    }

    const std::vector<std::string> SubNames = selection[0].getSubNames();
    if (SubNames.size() > maxObjs) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect selection"),
                             QObject::tr("Too many objects selected"));
        return false;
    }

    std::vector<App::DocumentObject*> pages =
        cmd->getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    if (pages.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect selection"),
                             QObject::tr("Create a page first."));
        return false;
    }
    return true;
}

bool _checkDrawViewPart(Gui::Command* cmd)
{
    std::vector<Gui::SelectionObject> selection = cmd->getSelection().getSelectionEx();
    for (auto& sel : selection) {
        auto dvp = dynamic_cast<TechDraw::DrawViewPart*>(sel.getObject());
        if (dvp) {
            return true;
        }
    }
    return false;
}
