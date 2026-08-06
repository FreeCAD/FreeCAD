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

#include <cmath>
#include <limits>
#include <string>
#include <vector>

#include <QApplication>
#include <QMessageBox>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPoint>
#include <QPixmap>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionObject.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Mod/TechDraw/App/DrawDimHelper.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawViewDimension.h>
#include <Mod/TechDraw/App/DrawViewBalloon.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/Geometry.h>
#include <Mod/TechDraw/App/LandmarkDimension.h>
#include <Mod/TechDraw/App/Preferences.h>

#include "CommandExtensionDims.h"
#include "DimensionValidators.h"
#include "DrawGuiUtil.h"
#include "QGIDatumLabel.h"
#include "QGIViewDimension.h"
#include "QGVPage.h"
#include "MDIViewPage.h"
#include "TaskDimRepair.h"
#include "TaskLinkDim.h"
#include "TaskSelectLineAttributes.h"
#include "TechDrawHandler.h"
#include "TechDrawDimensionHandler.h"
#include "ViewProviderDimension.h"
#include "ViewProviderDrawingView.h"


using namespace TechDrawGui;
using namespace TechDraw;
using namespace std;
using DimensionType = TechDraw::DrawViewDimension::DimensionType;
using DimensionGeometry = TechDraw::DimensionGeometry;

//===========================================================================
// utility routines
//===========================================================================

//internal functions
bool _checkSelection(Gui::Command* cmd, unsigned maxObjs = 2);
bool _checkDrawViewPart(Gui::Command* cmd);
bool _checkSelectionFeatures();

bool isDimCmdActive(Gui::Command* cmd)
{
    bool havePage = DrawGuiUtil::needPage(cmd);
    bool haveView = DrawGuiUtil::needView(cmd);
    return (havePage && haveView);
}


void execDistance(Gui::Command* cmd);
void execDistanceX(Gui::Command* cmd);
void execDistanceY(Gui::Command* cmd);
void execAngle(Gui::Command* cmd);
void execAngle3Pt(Gui::Command* cmd);
void execRadius(Gui::Command* cmd);
void execDiameter(Gui::Command* cmd);
void execArea(Gui::Command* cmd);
void execDim(Gui::Command* cmd, std::string type, StringVector acceptableGeometry, std::vector<int> minimumCounts, std::vector<DimensionGeometry> acceptableDimensionGeometrys);

void execExtent(Gui::Command* cmd, const std::string& dimType);

DrawViewDimension* dimensionMaker(TechDraw::DrawViewPart* dvp, std::string dimType,
                                  ReferenceVector references2d, ReferenceVector references3d);
DrawViewDimension* dimMaker(TechDraw::DrawViewPart* dvp, std::string dimType,
                                  ReferenceVector references2d, ReferenceVector references3d);

void positionDimText(DrawViewDimension* dim, int indexOffset = 0);

void activateHandler(TechDrawHandler* newHandler)
{
    std::unique_ptr<TechDrawHandler> ptr(newHandler);
    auto* mdi = qobject_cast<MDIViewPage*>(Gui::getMainWindow()->activeWindow());
    if (!mdi) {
        return;
    }

    ViewProviderPage* vp = mdi->getViewProviderPage();
    if (!vp) {
        return;
    }

    QGVPage* viewPage = vp->getQGVPage();
    if (!viewPage) {
        return;
    }
    viewPage->activateHandler(ptr.release());
}

//===========================================================================
// TechDraw_Dimension
//===========================================================================


DEF_STD_CMD_A(CmdTechDrawDimension)

CmdTechDrawDimension::CmdTechDrawDimension()
    : Command("TechDraw_Dimension")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Dimension");
    sToolTipText = QT_TR_NOOP("Inserts new contextual dimensions to the selection.\n"
        "Depending on your selection you might have several dimensions available. You can cycle through them using the M key.\n"
        "Left clicking on empty space will validate the current dimension. Right clicking or pressing Esc will cancel.");
    sWhatsThis = "TechDraw_Dimension";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_Dimension";
    sAccel = "D";
}

void CmdTechDrawDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    ReferenceVector references2d;
    ReferenceVector references3d;
    TechDraw::DrawViewPart* partFeat =
        TechDraw::getReferencesFromSelection(references2d, references3d);

    activateHandler(new TDHandlerDimension(references2d, partFeat));
}

bool CmdTechDrawDimension::isActive()
{
    return isDimCmdActive(this);
}


// Comp for dimension tools =============================================

class CmdTechDrawCompDimensionTools : public Gui::GroupCommand
{
public:
    CmdTechDrawCompDimensionTools()
        : GroupCommand("TechDraw_CompDimensionTools")
    {
        sAppModule = "TechDraw";
        sGroup = "TechDraw";
        sMenuText = QT_TR_NOOP("Dimension");
        sToolTipText = QT_TR_NOOP("Dimension tools");
        sWhatsThis = "TechDraw_CompDimensionTools";
        sStatusTip = sToolTipText;

        setCheckable(false);
        setRememberLast(false);

        addCommand("TechDraw_Dimension");
        addCommand(); //separator
        addCommand("TechDraw_LengthDimension");
        addCommand("TechDraw_HorizontalDimension");
        addCommand("TechDraw_VerticalDimension");
        addCommand("TechDraw_RadiusDimension");
        addCommand("TechDraw_DiameterDimension");
        addCommand("TechDraw_AngleDimension");
        addCommand("TechDraw_3PtAngleDimension");
        addCommand("TechDraw_AreaDimension");
        addCommand("TechDraw_ExtensionCreateLengthArc");
        addCommand(); //separator
        addCommand("TechDraw_HorizontalExtentDimension");
        addCommand("TechDraw_VerticalExtentDimension");
        addCommand(); //separator
        addCommand("TechDraw_ExtensionCreateHorizChainDimension");
        addCommand("TechDraw_ExtensionCreateVertChainDimension");
        addCommand("TechDraw_ExtensionCreateObliqueChainDimension");
        addCommand(); //separator
        addCommand("TechDraw_ExtensionCreateHorizCoordDimension");
        addCommand("TechDraw_ExtensionCreateVertCoordDimension");
        addCommand("TechDraw_ExtensionCreateObliqueCoordDimension");
        addCommand(); //separator
        addCommand("TechDraw_ExtensionCreateHorizChamferDimension");
        addCommand("TechDraw_ExtensionCreateVertChamferDimension");
    }

    const char* className() const override { return "CmdTechDrawCompDimensionTools"; }

    bool isActive() override
    {
        return isDimCmdActive(this);
    }
};

//===========================================================================
// TechDraw_RadiusDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawRadiusDimension)

CmdTechDrawRadiusDimension::CmdTechDrawRadiusDimension()
    : Command("TechDraw_RadiusDimension")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Radius Dimension");
    sToolTipText = QT_TR_NOOP("Inserts a radius dimension of a circular edge or arc");
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
                             QObject::tr("Task in progress"),
                             QObject::tr("Close the active task dialog and try again"));
        return;
    }

    if (_checkSelectionFeatures()) {
        execRadius(this);
    }
    else {
        ReferenceVector references2d;
        ReferenceVector references3d;
        TechDraw::DrawViewPart* partFeat =
            TechDraw::getReferencesFromSelection(references2d, references3d);
    
        activateHandler(new TDHandlerDimension(references2d, partFeat, "Radius"));
    }
}

bool CmdTechDrawRadiusDimension::isActive()
{
    return isDimCmdActive(this);
}

void execRadius(Gui::Command* cmd)
{
    //Define the geometric configuration required for a radius dimension
    StringVector acceptableGeometry({"Edge"});
    std::vector<int> minimumCounts({1});
    std::vector<DimensionGeometry> acceptableDimensionGeometrys(
        {DimensionGeometry::isCircle, DimensionGeometry::isEllipse, DimensionGeometry::isBSplineCircle, DimensionGeometry::isBSpline});

    execDim(cmd, "Radius", acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);
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
    sMenuText = QT_TR_NOOP("Diameter Dimension");
    sToolTipText = QT_TR_NOOP("Inserts a diameter dimension of a circular edge or arc");
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
                             QObject::tr("Task in progress"),
                             QObject::tr("Close the active task dialog and try again"));
        return;
    }

    if (_checkSelectionFeatures()) {
        execDiameter(this);
    }
    else {
        ReferenceVector references2d;
        ReferenceVector references3d;
        TechDraw::DrawViewPart* partFeat =
            TechDraw::getReferencesFromSelection(references2d, references3d);
    
        activateHandler(new TDHandlerDimension(references2d, partFeat, "Diameter"));
    }
}

bool CmdTechDrawDiameterDimension::isActive()
{
    return isDimCmdActive(this);
}

void execDiameter(Gui::Command* cmd)
{
    //Define the geometric configuration required for a diameter dimension
    StringVector acceptableGeometry({"Edge"});
    std::vector<int> minimumCounts({1});
    std::vector<DimensionGeometry> acceptableDimensionGeometrys(
        {DimensionGeometry::isCircle, DimensionGeometry::isEllipse, DimensionGeometry::isBSplineCircle, DimensionGeometry::isBSpline});

    execDim(cmd, "Diameter", acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);
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
    sMenuText = QT_TR_NOOP("Length Dimension");
    sToolTipText = QT_TR_NOOP("Inserts a length dimension of an edge or distance between two points");
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
                             QObject::tr("Task in progress"),
                             QObject::tr("Close the active task dialog and try again"));
        return;
    }

    if (_checkSelectionFeatures()) {
        execDistance(this);
    }
    else {
        ReferenceVector references2d;
        ReferenceVector references3d;
        TechDraw::DrawViewPart* partFeat =
            TechDraw::getReferencesFromSelection(references2d, references3d);
    
        activateHandler(new TDHandlerDimension(references2d, partFeat, "LengthFree"));
    }
}

bool CmdTechDrawLengthDimension::isActive()
{
    return isDimCmdActive(this);
}

void execDistance(Gui::Command* cmd)
{
    StringVector acceptableGeometry({"Edge", "Vertex"});
    std::vector<int> minimumCounts({1, 2});
    std::vector<DimensionGeometry> acceptableDimensionGeometrys(
        {DimensionGeometry::isVertical, DimensionGeometry::isHorizontal, DimensionGeometry::isDiagonal, DimensionGeometry::isHybrid});

    execDim(cmd, "Distance", acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);
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
    sMenuText = QT_TR_NOOP("Horizontal Length Dimension");
    sToolTipText = QT_TR_NOOP("Inserts a horizontal length dimension of an edge or distance between two points");
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
                             QObject::tr("Task in progress"),
                             QObject::tr("Close the active task dialog and try again"));
        return;
    }

    if (_checkSelectionFeatures()) {
        execDistanceX(this);
    }
    else {
        ReferenceVector references2d;
        ReferenceVector references3d;
        TechDraw::DrawViewPart* partFeat =
            TechDraw::getReferencesFromSelection(references2d, references3d);
    
        activateHandler(new TDHandlerDimension(references2d, partFeat, "HorizontalLength"));
    }
}

bool CmdTechDrawHorizontalDimension::isActive()
{
    return isDimCmdActive(this);
}

void execDistanceX(Gui::Command* cmd)
{
    //Define the geometric configuration required for a length dimension
    StringVector acceptableGeometry({"Edge", "Vertex"});
    std::vector<int> minimumCounts({1, 2});
    std::vector<DimensionGeometry> acceptableDimensionGeometrys({DimensionGeometry::isHorizontal,
                                                                 DimensionGeometry::isDiagonal,
                                                                 DimensionGeometry::isHybrid,
                                                                 DimensionGeometry::isMultiEdge});

    execDim(cmd, "DistanceX", acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);
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
    sMenuText = QT_TR_NOOP("Vertical Length Dimension");
    sToolTipText = QT_TR_NOOP("Inserts a vertical length dimension of an edge or distance between two points");
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
                             QObject::tr("Task in progress"),
                             QObject::tr("Close the active task dialog and try again"));
        return;
    }

    if (_checkSelectionFeatures()) {
        execDistanceY(this);
    }
    else {
        ReferenceVector references2d;
        ReferenceVector references3d;
        TechDraw::DrawViewPart* partFeat =
            TechDraw::getReferencesFromSelection(references2d, references3d);
    
        activateHandler(new TDHandlerDimension(references2d, partFeat, "VerticalLength"));
    }
}

bool CmdTechDrawVerticalDimension::isActive()
{
    return isDimCmdActive(this);
}

void execDistanceY(Gui::Command* cmd)
{
    //Define the geometric configuration required for a length dimension
    StringVector acceptableGeometry({"Edge", "Vertex"});
    std::vector<int> minimumCounts({1, 2});
    std::vector<DimensionGeometry> acceptableDimensionGeometrys({DimensionGeometry::isVertical,
                                                                 DimensionGeometry::isDiagonal,
                                                                 DimensionGeometry::isHybrid,
                                                                 DimensionGeometry::isMultiEdge});

    execDim(cmd, "DistanceY", acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);
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
    sMenuText = QT_TR_NOOP("Angle Dimension");
    sToolTipText = QT_TR_NOOP("Inserts an angle dimension between two edges");
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
                             QObject::tr("Task in progress"),
                             QObject::tr("Close the active task dialog and try again"));
        return;
    }

    if (_checkSelectionFeatures()) {
        execAngle(this);
    }
    else {
        ReferenceVector references2d;
        ReferenceVector references3d;
        TechDraw::DrawViewPart* partFeat =
            TechDraw::getReferencesFromSelection(references2d, references3d);
    
        activateHandler(new TDHandlerDimension(references2d, partFeat, "Angle"));
    }
}

bool CmdTechDrawAngleDimension::isActive()
{
    return isDimCmdActive(this);
}

void execAngle(Gui::Command* cmd)
{
    //Define the geometric configuration required for a length dimension
    StringVector acceptableGeometry({"Edge"});
    std::vector<int> minimumCounts({2});
    std::vector<DimensionGeometry> acceptableDimensionGeometrys({DimensionGeometry::isAngle});

    execDim(cmd, "Angle", acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);
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
    sMenuText = QT_TR_NOOP("Angle Dimension From 3 Points");
    sToolTipText = QT_TR_NOOP("Inserts an angle dimension between 3 selected points");
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
                             QObject::tr("Task in progress"),
                             QObject::tr("Close the active task dialog and try again"));
        return;
    }

    if (_checkSelectionFeatures()) {
        execAngle3Pt(this);
    }
    else {
        ReferenceVector references2d;
        ReferenceVector references3d;
        TechDraw::DrawViewPart* partFeat =
            TechDraw::getReferencesFromSelection(references2d, references3d);
    
        activateHandler(new TDHandlerDimension(references2d, partFeat, "3ptAngle_1"));
    }
}

bool CmdTechDraw3PtAngleDimension::isActive()
{
    return isDimCmdActive(this);
}

void execAngle3Pt(Gui::Command* cmd)
{
    //Define the geometric configuration required for a length dimension
    StringVector acceptableGeometry({"Vertex"});
    std::vector<int> minimumCounts({3});
    std::vector<DimensionGeometry> acceptableDimensionGeometrys({DimensionGeometry::isAngle3Pt});

    execDim(cmd, "Angle3Pt", acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);
}

//===========================================================================
// TechDraw_AreaDimension
//===========================================================================

DEF_STD_CMD_A(CmdTechDrawAreaDimension)

CmdTechDrawAreaDimension::CmdTechDrawAreaDimension()
    : Command("TechDraw_AreaDimension")
{
    sAppModule = "TechDraw";
    sGroup = QT_TR_NOOP("TechDraw");
    sMenuText = QT_TR_NOOP("Area Annotation");
    sToolTipText = QT_TR_NOOP("Inserts an annotation showing the area of a selected face");
    sWhatsThis = "TechDraw_AreaDimension";
    sStatusTip = sToolTipText;
    sPixmap = "TechDraw_AreaDimension";
}

void CmdTechDrawAreaDimension::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Task in progress"),
                             QObject::tr("Close the active task dialog and try again"));
        return;
    }

    if (_checkSelectionFeatures()) {
        execArea(this);
    }
    else {
        ReferenceVector references2d;
        ReferenceVector references3d;
        TechDraw::DrawViewPart* partFeat =
            TechDraw::getReferencesFromSelection(references2d, references3d);
    
        activateHandler(new TDHandlerDimension(references2d, partFeat, "Area"));
    }
}

bool CmdTechDrawAreaDimension::isActive()
{
    return isDimCmdActive(this);
}

void execArea(Gui::Command* cmd)
{
    //Define the geometric configuration required for a area dimension
    StringVector acceptableGeometry({"Face"});
    std::vector<int> minimumCounts({1});
    std::vector<DimensionGeometry> acceptableDimensionGeometrys({DimensionGeometry::isFace});

    execDim(cmd, "Area", acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);
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
    sMenuText = QT_TR_NOOP("Extent Dimension");
    sToolTipText = QT_TR_NOOP("Inserts a dimension showing the extent (overall length) of an object or feature");
    sWhatsThis = "TechDraw_ExtentGroup";
    sStatusTip = sToolTipText;
    //    eType           = ForEdit;
}

void CmdTechDrawExtentGroup::activated(int iMsg)
{
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Task in progress"),
                             QObject::tr("Close the active task dialog and try again"));
        return;
    }

    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    pcAction->setIcon(pcAction->actions().at(iMsg)->icon());
    switch (iMsg) {
        case 0:
            execExtent(this, "DistanceX");
            break;
        case 1:
            execExtent(this, "DistanceY");
            break;
        default:
            Base::Console().message("CMD::ExtGrp - invalid iMsg: %d\n", iMsg);
    };
}

Gui::Action* CmdTechDrawExtentGroup::createAction()
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* p1 = pcAction->addAction(QString());
    p1->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_HorizontalExtentDimension"));
    p1->setObjectName(QStringLiteral("TechDraw_HorizontalExtentDimension"));
    p1->setWhatsThis(QStringLiteral("TechDraw_HorizontalExtentDimension"));
    QAction* p2 = pcAction->addAction(QString());
    p2->setIcon(Gui::BitmapFactory().iconFromTheme("TechDraw_VerticalExtentDimension"));
    p2->setObjectName(QStringLiteral("TechDraw_VerticalExtentDimension"));
    p2->setWhatsThis(QStringLiteral("TechDraw_VerticalExtentDimension"));

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
    arc1->setText(QApplication::translate("CmdTechDrawExtentGroup", "Horizontal extent"));
    arc1->setToolTip(
        QApplication::translate("TechDraw_HorizontalExtent", "Insert horizontal extent dimension"));
    arc1->setStatusTip(arc1->toolTip());
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("CmdTechDrawExtentGroup", "Vertical extent"));
    arc2->setToolTip(QApplication::translate("TechDraw_VerticalExtentDimension",
                                             "Insert vertical extent dimension"));
    arc2->setStatusTip(arc2->toolTip());
}

bool CmdTechDrawExtentGroup::isActive()
{
    return isDimCmdActive(this);
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
    sMenuText = QT_TR_NOOP("Horizontal Extent Dimension");
    sToolTipText = QT_TR_NOOP("Inserts a dimension showing the horizontal extent (overall length) of an object or feature");
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
                             QObject::tr("Task in progress"),
                             QObject::tr("Close the active task dialog and try again"));
        return;
    }

    if (_checkSelectionFeatures()) {
        execExtent(this, "DistanceX");
    }
    else {
        ReferenceVector references2d;
        ReferenceVector references3d;
        TechDraw::DrawViewPart* partFeat =
            TechDraw::getReferencesFromSelection(references2d, references3d);
    
        activateHandler(new TDHandlerDimension(references2d, partFeat, "HorizontalExtent"));
    }
}

bool CmdTechDrawHorizontalExtentDimension::isActive()
{
    return isDimCmdActive(this);
}

void execExtent(Gui::Command* cmd, const std::string& dimType)
{
    const char* commandString = nullptr;
    if (dimType == "DistanceX") {
        commandString = QT_TRANSLATE_NOOP("Command", "Create Dimension DistanceX");
    } else if (dimType == "DistanceY") {
        commandString = QT_TRANSLATE_NOOP("Command", "Create Dimension DistanceY");
    }
    cmd->openCommand(QT_TRANSLATE_NOOP("Command", commandString));

    bool result = _checkDrawViewPart(cmd);
    if (!result) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect Selection"),
                             QObject::tr("No view of a part in selection."));
        cmd->abortCommand();
        return;
    }
    ReferenceVector references2d;
    ReferenceVector references3d;
    TechDraw::DrawViewPart* partFeat =
        TechDraw::getReferencesFromSelection(references2d, references3d);

    // if sticky selection is in use we may get confusing selections that appear to
    // include both 2d and 3d geometry for the extent dim.
    if (!references3d.empty())  {
        for (auto& ref : references2d) {
            if (!ref.getSubName().empty()) {
                QMessageBox::warning(Gui::getMainWindow(),
                    QObject::tr("Incorrect Selection"),
                    QObject::tr("Selection contains both 2D and 3D geometry"));
                cmd->abortCommand();
                return;
            }
        }
    }

    //Define the geometric configuration required for a extent dimension
    StringVector acceptableGeometry({"Edge"});
    std::vector<int> minimumCounts({1});
    std::vector<DimensionGeometry> acceptableDimensionGeometrys({DimensionGeometry::isMultiEdge,
                                                                     DimensionGeometry::isHorizontal,
                                                                     DimensionGeometry::isVertical,
                                                                     DimensionGeometry::isDiagonal,
                                                                     DimensionGeometry::isCircle,
                                                                     DimensionGeometry::isEllipse,
                                                                     DimensionGeometry::isBSplineCircle,
                                                                     DimensionGeometry::isBSpline,
                                                                     DimensionGeometry::isZLimited});

    //what 2d geometry configuration did we receive?
    DimensionGeometry geometryRefs2d = validateDimSelection(
        references2d, acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);
    if (geometryRefs2d == DimensionGeometry::isInvalid) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect Selection"),
                             QObject::tr("Cannot make 2D extent dimension from selection"));
        cmd->abortCommand();
        return;
    }

    //what 3d geometry configuration did we receive?
    DimensionGeometry geometryRefs3d;
    if (geometryRefs2d == DimensionGeometry::isViewReference && !references3d.empty()) {
        geometryRefs3d = validateDimSelection3d(partFeat,
                                                references3d,
                                                acceptableGeometry,
                                                minimumCounts,
                                                acceptableDimensionGeometrys);
        if (geometryRefs3d == DimensionGeometry::isInvalid) {
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr("Incorrect Selection"),
                                 QObject::tr("Cannot make 3D extent dimension from selection"));
            cmd->abortCommand();
            return;
        }
    }

    if (references3d.empty()) {
        DrawDimHelper::makeExtentDim(partFeat, dimType, references2d);
    }
    else {
        DrawDimHelper::makeExtentDim3d(partFeat, dimType, references3d);
    }
    cmd->commitCommand();
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
    sMenuText = QT_TR_NOOP("Vertical Extent Dimension");
    sToolTipText = QT_TR_NOOP("Inserts a dimension showing the vertical extent (overall length) of an object or feature");
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
                             QObject::tr("Task in progress"),
                             QObject::tr("Close the active task dialog and try again"));
        return;
    }

    if (_checkSelectionFeatures()) {
        execExtent(this, "DistanceY");
    }
    else {
        ReferenceVector references2d;
        ReferenceVector references3d;
        TechDraw::DrawViewPart* partFeat =
            TechDraw::getReferencesFromSelection(references2d, references3d);
    
        activateHandler(new TDHandlerDimension(references2d, partFeat, "VerticalExtent"));
    }
}

bool CmdTechDrawVerticalExtentDimension::isActive()
{
    return isDimCmdActive(this);
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
    sToolTipText = QT_TR_NOOP("Repairs broken or incorrect dimension references");
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
                             QObject::tr("There is no dimension in your selection"));
        return;
    } else {
        dim = static_cast<TechDraw::DrawViewDimension*>(dimObjs.at(0));
    }

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
    rcCmdMgr.addCommand(new CmdTechDrawAreaDimension());
    rcCmdMgr.addCommand(new CmdTechDrawExtentGroup());
    rcCmdMgr.addCommand(new CmdTechDrawVerticalExtentDimension());
    rcCmdMgr.addCommand(new CmdTechDrawHorizontalExtentDimension());
    rcCmdMgr.addCommand(new CmdTechDrawDimensionRepair());
    rcCmdMgr.addCommand(new CmdTechDrawCompDimensionTools());
}

//------------------------------------------------------------------------------

//Common code to build a dimension feature

void execDim(Gui::Command* cmd, std::string type, StringVector acceptableGeometry, std::vector<int> minimumCounts, std::vector<DimensionGeometry> acceptableDimensionGeometrys)
{
    bool result = _checkDrawViewPart(cmd);
    if (!result) {
        QMessageBox::warning(Gui::getMainWindow(),
            QObject::tr("Incorrect Selection"),
            QObject::tr("No view of a part in selection."));
        return;
    }

    ReferenceVector references2d;
    ReferenceVector references3d;
    TechDraw::DrawViewPart* partFeat =
        TechDraw::getReferencesFromSelection(references2d, references3d);

    //what 2d geometry configuration did we receive?
    DimensionGeometry geometryRefs2d = validateDimSelection(
        references2d, acceptableGeometry, minimumCounts, acceptableDimensionGeometrys);
    if (geometryRefs2d == DimensionGeometry::isInvalid) {
        QMessageBox::warning(Gui::getMainWindow(),
            QObject::tr("Incorrect Selection"),
            QObject::tr("Cannot make 2D dimension from selection"));
        return;
    }

    if (geometryRefs2d == DimensionGeometry::isViewReference && references3d.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
            QObject::tr("Incorrect Selection"),
            QObject::tr("Cannot make 3D dimension without 3d references"));
        return;
    }

    //what 3d geometry configuration did we receive?
    DimensionGeometry geometryRefs3d{DimensionGeometry::isInvalid};
    if (geometryRefs2d == DimensionGeometry::isViewReference && !references3d.empty()) {
        geometryRefs3d = validateDimSelection3d(partFeat,
            references3d,
            acceptableGeometry,
            minimumCounts,
            acceptableDimensionGeometrys);
        if (geometryRefs3d == DimensionGeometry::isInvalid) {
            QMessageBox::warning(Gui::getMainWindow(),
                QObject::tr("Incorrect Selection"),
                QObject::tr("Cannot make 3D dimension from selection"));
            return;
        }
    }
    else {
        references3d.clear();
    }

    //errors and warnings
    if (type == "Radius" || type == "Diameter") {
        if (geometryRefs2d == DimensionGeometry::isEllipse || geometryRefs3d == DimensionGeometry::isEllipse) {
            QMessageBox::StandardButton result = QMessageBox::warning(
                Gui::getMainWindow(),
                QObject::tr("Ellipse curve warning"),
                QObject::tr("Selected edge is an Ellipse. Value will be approximate. Continue?"),
                QMessageBox::Ok | QMessageBox::Cancel,
                QMessageBox::Cancel);
            if (result != QMessageBox::Ok) {
                return;
            }
        }
        if (geometryRefs2d == DimensionGeometry::isBSplineCircle || geometryRefs3d == DimensionGeometry::isBSplineCircle) {
            QMessageBox::StandardButton result = QMessageBox::warning(
                Gui::getMainWindow(),
                QObject::tr("B-spline curve warning"),
                QObject::tr("Selected edge is a B-spline. Value will be approximate. Continue?"),
                QMessageBox::Ok | QMessageBox::Cancel,
                QMessageBox::Cancel);
            if (result != QMessageBox::Ok) {
                return;
            }
        }
        if (geometryRefs2d == DimensionGeometry::isBSpline || geometryRefs3d == DimensionGeometry::isBSpline) {
            QMessageBox::critical(
                Gui::getMainWindow(),
                QObject::tr("B-spline curve error"),
                QObject::tr("Selected edge is a B-spline and a radius/diameter cannot be calculated."));
            return;
        }
    }
    if (geometryRefs2d == DimensionGeometry::isFace &&
        references2d.size() > 1) {
        Base::Console().warning("Multiple faces are selected. Using first.\n");
        references2d.resize(1);
    }

    //build the dimension
    DrawViewDimension* dim = dimensionMaker(partFeat, type, references2d, references3d);

    if (type == "Distance" || type == "DistanceX" || type == "DistanceY" || type == "Angle" || type == "Angle3Pt") {
        //position the Dimension text on the view
        positionDimText(dim);
    }
}

DrawViewDimension* dimensionMaker(TechDraw::DrawViewPart* dvp, std::string dimType,
                                  ReferenceVector references2d, ReferenceVector references3d)
{
    int tid = Gui::Command::openActiveDocumentCommand(QT_TRANSLATE_NOOP("Command", "Create dimension"));

    TechDraw::DrawViewDimension* dim = dimMaker(dvp, dimType, references2d, references3d);

    Gui::Command::commitCommand(tid);

    // Touch the parent feature so the dimension in tree view appears as a child
    dvp->touch(true);

    // Select only the newly created dimension
    Gui::Selection().clearSelection();
    Gui::Selection().addSelection(dvp->getDocument()->getName(), dim->getNameInDocument());

    return dim;
}

DrawViewDimension* dimMaker(TechDraw::DrawViewPart* dvp, std::string dimType,
                            ReferenceVector references2d, ReferenceVector references3d)
{
    TechDraw::DrawPage* page = dvp->findParentPage();
    std::string PageName = page->getNameInDocument();

    std::string dimName = dvp->getDocument()->getUniqueObjectName("Dimension");

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

    auto* dim = dynamic_cast<TechDraw::DrawViewDimension*>(dvp->getDocument()->getObject(dimName.c_str()));
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

    dim->recomputeFeature();

    return dim;
}

//position the Dimension text on the view
void positionDimText(DrawViewDimension* dim, int offsetIndex)
{
    TechDraw::pointPair pp = dim->getLinearPoints();
    Base::Vector3d mid = (pp.first() + pp.second()) / 2.0;
    dim->X.setValue(mid.x);
    double fontSize = Preferences::dimFontSizeMM();
    dim->Y.setValue(-mid.y + (offsetIndex * 1.5 + 0.5) * fontSize);
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
                             QObject::tr("Incorrect Selection"),
                             QObject::tr("Select an object first"));
        return false;
    }

    const std::vector<std::string> SubNames = selection[0].getSubNames();
    if (SubNames.size() > maxObjs) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect Selection"),
                             QObject::tr("Too many objects selected"));
        return false;
    }

    std::vector<App::DocumentObject*> pages =
        cmd->getDocument()->getObjectsOfType(TechDraw::DrawPage::getClassTypeId());
    if (pages.empty()) {
        QMessageBox::warning(Gui::getMainWindow(),
                             QObject::tr("Incorrect Selection"),
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

bool _checkSelectionFeatures() {
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();
    if (selection.empty()) {
        return false;
    }
    std::vector<std::string> SubNames = selection[0].getSubNames();
    for (auto& subName : SubNames) {
        std::string GeoType = TechDraw::DrawUtil::getGeomTypeFromName(subName);
        if (GeoType == "Edge" || GeoType == "Vertex" || GeoType == "Face") {
            return true;
        }
    }
    return false;
}
