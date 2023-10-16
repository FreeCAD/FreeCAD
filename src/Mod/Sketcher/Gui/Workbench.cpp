/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include "Utils.h"
#include "Workbench.h"
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/WorkbenchManager.h>
#include <Mod/Sketcher/App/Constraint.h>

using namespace SketcherGui;

#if 0  // needed for Qt's lupdate utility
    qApp->translate("CommandGroup", "Sketcher");
    qApp->translate("Workbench","P&rofiles");
    qApp->translate("Workbench","S&ketch");
    qApp->translate("Workbench", "Sketcher");
    qApp->translate("Workbench", "Sketcher edit mode");
    qApp->translate("Workbench", "Sketcher geometries");
    qApp->translate("Workbench", "Sketcher constraints");
    qApp->translate("Workbench", "Sketcher tools");
    qApp->translate("Workbench", "Sketcher B-spline tools");
    qApp->translate("Workbench", "Sketcher virtual space");
    qApp->translate("Workbench", "Sketcher edit tools");
#endif

/// @namespace SketcherGui @class Workbench
TYPESYSTEM_SOURCE(SketcherGui::Workbench, Gui::StdWorkbench)

Workbench::Workbench()
{}

Workbench::~Workbench()
{}

Gui::MenuItem* Workbench::setupMenuBar() const
{
    Gui::MenuItem* root = StdWorkbench::setupMenuBar();
    Gui::MenuItem* item = root->findItem("&Windows");

    // == Profile menu ==========================================
    /* TODO: implement profile menu with different profiles
        Gui::MenuItem* profile = new Gui::MenuItem;
        root->insertItem(item, profile);
        profile->setCommand("P&rofiles");

        *profile << "Sketcher_ProfilesHexagon1";
    */

    // == Sketcher menu ==========================================

    Gui::MenuItem* geom = new Gui::MenuItem();
    geom->setCommand("Sketcher geometries");
    addSketcherWorkbenchGeometries(*geom);

    Gui::MenuItem* cons = new Gui::MenuItem();
    cons->setCommand("Sketcher constraints");
    addSketcherWorkbenchConstraints(*cons);

    Gui::MenuItem* consaccel = new Gui::MenuItem();
    consaccel->setCommand("Sketcher tools");
    addSketcherWorkbenchTools(*consaccel);

    Gui::MenuItem* bsplines = new Gui::MenuItem();
    bsplines->setCommand("Sketcher B-spline tools");
    addSketcherWorkbenchBSplines(*bsplines);

    Gui::MenuItem* visual = new Gui::MenuItem();
    visual->setCommand("Sketcher visual");
    addSketcherWorkbenchVisual(*visual);

    Gui::MenuItem* sketch = new Gui::MenuItem;
    root->insertItem(item, sketch);
    sketch->setCommand("S&ketch");
    addSketcherWorkbenchSketchActions(*sketch);
    addSketcherWorkbenchSketchEditModeActions(*sketch);
    *sketch << geom << cons << consaccel << bsplines << visual;

    return root;
}

Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();

    Gui::ToolBarItem* sketcher = new Gui::ToolBarItem(root);
    sketcher->setCommand("Sketcher");
    addSketcherWorkbenchSketchActions(*sketcher);

    Gui::ToolBarItem* sketcherEditMode =
        new Gui::ToolBarItem(root, Gui::ToolBarItem::DefaultVisibility::Unavailable);
    sketcherEditMode->setCommand("Sketcher edit mode");
    addSketcherWorkbenchSketchEditModeActions(*sketcherEditMode);

    Gui::ToolBarItem* geom =
        new Gui::ToolBarItem(root, Gui::ToolBarItem::DefaultVisibility::Unavailable);
    geom->setCommand("Sketcher geometries");
    addSketcherWorkbenchGeometries(*geom);

    Gui::ToolBarItem* cons =
        new Gui::ToolBarItem(root, Gui::ToolBarItem::DefaultVisibility::Unavailable);
    cons->setCommand("Sketcher constraints");
    addSketcherWorkbenchConstraints(*cons);

    Gui::ToolBarItem* consaccel =
        new Gui::ToolBarItem(root, Gui::ToolBarItem::DefaultVisibility::Unavailable);
    consaccel->setCommand("Sketcher tools");
    addSketcherWorkbenchTools(*consaccel);

    Gui::ToolBarItem* bspline =
        new Gui::ToolBarItem(root, Gui::ToolBarItem::DefaultVisibility::Unavailable);
    bspline->setCommand("Sketcher B-spline tools");
    addSketcherWorkbenchBSplines(*bspline);

    Gui::ToolBarItem* visual =
        new Gui::ToolBarItem(root, Gui::ToolBarItem::DefaultVisibility::Unavailable);
    visual->setCommand("Sketcher visual");
    addSketcherWorkbenchVisual(*visual);

    Gui::ToolBarItem* edittools =
        new Gui::ToolBarItem(root, Gui::ToolBarItem::DefaultVisibility::Unavailable);
    edittools->setCommand("Sketcher edit tools");
    addSketcherWorkbenchEditTools(*edittools);

    return root;
}

Gui::ToolBarItem* Workbench::setupCommandBars() const
{
    // Sketcher tools
    Gui::ToolBarItem* root = new Gui::ToolBarItem;
    return root;
}


namespace
{
inline const QStringList editModeToolbarNames()
{
    return QStringList {QString::fromLatin1("Sketcher edit mode"),
                        QString::fromLatin1("Sketcher geometries"),
                        QString::fromLatin1("Sketcher constraints"),
                        QString::fromLatin1("Sketcher tools"),
                        QString::fromLatin1("Sketcher B-spline tools"),
                        QString::fromLatin1("Sketcher visual"),
                        QString::fromLatin1("Sketcher edit tools")};
}

inline const QStringList nonEditModeToolbarNames()
{
    return QStringList {QString::fromLatin1("Structure"), QString::fromLatin1("Sketcher")};
}
}  // namespace

void Workbench::activated()
{
    /* When the workbench is activated, it may happen that we are in edit mode or not.
     * If we are not in edit mode, the function enterEditMode (called by the ViewProvider) takes
     * care to save the state of toolbars outside of edit mode. We cannot do it here, as we are
     * coming from another WB.
     *
     * If we moved to another WB from edit mode, the new WB was activated before deactivating this.
     * Therefore we had no chance to tidy up the save state. We assume a loss of any CHANGE to
     * toolbar configuration since last entering edit mode in this case (for any change in
     * configuration to be stored, the edit mode must be left while the selected Workbench is the
     * sketcher WB).
     *
     * However, now that we are back (from another WB), we need to make the toolbars available.
     * These correspond to the last saved state.
     */
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (isSketchInEdit(doc)) {
        Gui::ToolBarManager::getInstance()->setState(editModeToolbarNames(),
                                                     Gui::ToolBarManager::State::ForceAvailable);
    }
}

void Workbench::enterEditMode()
{
    /* Ensure the state left by the non-edit mode toolbars is saved (in case of changing to edit
     * mode) without changing workbench
     */
    Gui::ToolBarManager::getInstance()->setState(nonEditModeToolbarNames(),
                                                 Gui::ToolBarManager::State::SaveState);

    Gui::ToolBarManager::getInstance()->setState(editModeToolbarNames(),
                                                 Gui::ToolBarManager::State::ForceAvailable);
    Gui::ToolBarManager::getInstance()->setState(nonEditModeToolbarNames(),
                                                 Gui::ToolBarManager::State::ForceHidden);
}

void Workbench::leaveEditMode()
{
    /* Ensure the state left by the edit mode toolbars is saved (in case of changing to edit mode)
     * without changing workbench.
     *
     * However, do not save state if the current workbench is not the Sketcher WB, because otherwise
     * we would be saving the state of the currently activated workbench, and the toolbars would
     * disappear (as the toolbars of that other WB are only visible).
     */
    auto* workbench = Gui::WorkbenchManager::instance()->active();

    if (workbench->name() == "SketcherWorkbench") {
        Gui::ToolBarManager::getInstance()->setState(editModeToolbarNames(),
                                                     Gui::ToolBarManager::State::SaveState);
    }

    Gui::ToolBarManager::getInstance()->setState(editModeToolbarNames(),
                                                 Gui::ToolBarManager::State::RestoreDefault);
    Gui::ToolBarManager::getInstance()->setState(nonEditModeToolbarNames(),
                                                 Gui::ToolBarManager::State::RestoreDefault);
}

namespace SketcherGui
{

template<typename T>
void SketcherAddWorkbenchSketchActions(T& sketch);

template<>
inline void SketcherAddWorkbenchSketchActions(Gui::MenuItem& sketch)
{
    sketch << "Sketcher_NewSketch"
           << "Sketcher_EditSketch"
           << "Sketcher_MapSketch"
           << "Sketcher_ReorientSketch"
           << "Sketcher_ValidateSketch"
           << "Sketcher_MergeSketches"
           << "Sketcher_MirrorSketch";
}
template<>
inline void SketcherAddWorkbenchSketchActions(Gui::ToolBarItem& sketch)
{
    sketch << "Sketcher_NewSketch"
           << "Sketcher_EditSketch"
           << "Sketcher_MapSketch"
           << "Sketcher_ReorientSketch"
           << "Sketcher_ValidateSketch"
           << "Sketcher_MergeSketches"
           << "Sketcher_MirrorSketch";
}

template<typename T>
void SketcherAddWorkbenchSketchEditModeActions(T& sketch);

template<>
inline void SketcherAddWorkbenchSketchEditModeActions(Gui::MenuItem& sketch)
{
    sketch << "Sketcher_LeaveSketch"
           << "Sketcher_ViewSketch"
           << "Sketcher_ViewSection"
           << "Sketcher_StopOperation";
}
template<>
inline void SketcherAddWorkbenchSketchEditModeActions(Gui::ToolBarItem& sketch)
{
    sketch << "Sketcher_LeaveSketch"
           << "Sketcher_ViewSketch"
           << "Sketcher_ViewSection";
}

template<typename T>
void SketcherAddWorkbenchGeometries(T& geom);

template<typename T>
void SketcherAddWorkspaceArcs(T& geom);

template<>
inline void SketcherAddWorkspaceArcs<Gui::MenuItem>(Gui::MenuItem& geom)
{
    geom << "Sketcher_CreateArc"
         << "Sketcher_Create3PointArc"
         << "Sketcher_CreateCircle"
         << "Sketcher_Create3PointCircle"
         << "Sketcher_CreateEllipseByCenter"
         << "Sketcher_CreateEllipseBy3Points"
         << "Sketcher_CreateArcOfEllipse"
         << "Sketcher_CreateArcOfHyperbola"
         << "Sketcher_CreateArcOfParabola"
         << "Sketcher_CreateBSpline"
         << "Sketcher_CreatePeriodicBSpline"
         << "Sketcher_CreateBSplineByInterpolation"
         << "Sketcher_CreatePeriodicBSplineByInterpolation";
}

template<>
inline void SketcherAddWorkspaceArcs<Gui::ToolBarItem>(Gui::ToolBarItem& geom)
{
    geom << "Sketcher_CompCreateArc"
         << "Sketcher_CompCreateCircle"
         << "Sketcher_CompCreateConic"
         << "Sketcher_CompCreateBSpline";
}

template<typename T>
void SketcherAddWorkspaceRegularPolygon(T& geom);

template<>
inline void SketcherAddWorkspaceRegularPolygon<Gui::MenuItem>(Gui::MenuItem& geom)
{
    geom << "Sketcher_CreateTriangle"
         << "Sketcher_CreateSquare"
         << "Sketcher_CreatePentagon"
         << "Sketcher_CreateHexagon"
         << "Sketcher_CreateHeptagon"
         << "Sketcher_CreateOctagon"
         << "Sketcher_CreateRegularPolygon";
}

template<>
inline void SketcherAddWorkspaceRegularPolygon<Gui::ToolBarItem>(Gui::ToolBarItem& geom)
{
    geom << "Sketcher_CompCreateRegularPolygon";
}

template<typename T>
void SketcherAddWorkspaceRectangles(T& geom);

template<>
inline void SketcherAddWorkspaceRectangles<Gui::MenuItem>(Gui::MenuItem& geom)
{
    geom << "Sketcher_CreateRectangle"
         << "Sketcher_CreateRectangle_Center"
         << "Sketcher_CreateOblong";
}

template<>
inline void SketcherAddWorkspaceRectangles<Gui::ToolBarItem>(Gui::ToolBarItem& geom)
{
    geom << "Sketcher_CompCreateRectangles";
}

template<typename T>
void SketcherAddWorkspaceFillets(T& geom);

template<>
inline void SketcherAddWorkspaceFillets<Gui::MenuItem>(Gui::MenuItem& geom)
{
    geom << "Sketcher_CreateFillet"
         << "Sketcher_CreatePointFillet";
}

template<>
inline void SketcherAddWorkspaceFillets<Gui::ToolBarItem>(Gui::ToolBarItem& geom)
{
    geom << "Sketcher_CompCreateFillets";
}

template<typename T>
inline void SketcherAddWorkbenchGeometries(T& geom)
{
    geom << "Sketcher_CreatePoint"
         << "Sketcher_CreateLine";
    SketcherAddWorkspaceArcs(geom);
    geom << "Separator"
         << "Sketcher_CreatePolyline";
    SketcherAddWorkspaceRectangles(geom);
    SketcherAddWorkspaceRegularPolygon(geom);
    geom << "Sketcher_CreateSlot"
         << "Separator";
    SketcherAddWorkspaceFillets(geom);
    geom << "Sketcher_Trimming"
         << "Sketcher_Extend"
         << "Sketcher_Split"
         << "Sketcher_External"
         << "Sketcher_CarbonCopy"
         << "Sketcher_ToggleConstruction"
        /*<< "Sketcher_CreateText"*/
        /*<< "Sketcher_CreateDraftLine"*/;
}

template<typename T>
inline void SketcherAddWorkbenchConstraints(T& cons);

template<>
inline void SketcherAddWorkbenchConstraints<Gui::MenuItem>(Gui::MenuItem& cons)
{
    cons << "Sketcher_ConstrainCoincident"
         << "Sketcher_ConstrainPointOnObject"
         << "Sketcher_ConstrainVertical"
         << "Sketcher_ConstrainHorizontal"
         << "Sketcher_ConstrainParallel"
         << "Sketcher_ConstrainPerpendicular"
         << "Sketcher_ConstrainTangent"
         << "Sketcher_ConstrainEqual"
         << "Sketcher_ConstrainSymmetric"
         << "Sketcher_ConstrainBlock"
         << "Separator"
         << "Sketcher_Dimension"
         << "Sketcher_ConstrainLock"
         << "Sketcher_ConstrainDistanceX"
         << "Sketcher_ConstrainDistanceY"
         << "Sketcher_ConstrainDistance"
         << "Sketcher_ConstrainRadius"
         << "Sketcher_ConstrainDiameter"
         << "Sketcher_ConstrainRadiam"
         << "Sketcher_ConstrainAngle"
         << "Sketcher_ConstrainSnellsLaw"
         << "Separator"
         << "Sketcher_ToggleDrivingConstraint"
         << "Sketcher_ToggleActiveConstraint";
}

template<>
inline void SketcherAddWorkbenchConstraints<Gui::ToolBarItem>(Gui::ToolBarItem& cons)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/dimensioning");

    cons << "Sketcher_ConstrainCoincident"
         << "Sketcher_ConstrainPointOnObject"
         << "Sketcher_ConstrainVertical"
         << "Sketcher_ConstrainHorizontal"
         << "Sketcher_ConstrainParallel"
         << "Sketcher_ConstrainPerpendicular"
         << "Sketcher_ConstrainTangent"
         << "Sketcher_ConstrainEqual"
         << "Sketcher_ConstrainSymmetric"
         << "Sketcher_ConstrainBlock"
         << "Separator";
    if (hGrp->GetBool("SingleDimensioningTool", true)) {
        if (!hGrp->GetBool("SeparatedDimensioningTools", false)) {
            cons << "Sketcher_CompDimensionTools";
        }
        else {
            cons << "Sketcher_Dimension";
        }
    }
    if (hGrp->GetBool("SeparatedDimensioningTools", false)) {
        cons << "Sketcher_ConstrainLock"
             << "Sketcher_ConstrainDistanceX"
             << "Sketcher_ConstrainDistanceY"
             << "Sketcher_ConstrainDistance"
             << "Sketcher_CompConstrainRadDia"
             << "Sketcher_ConstrainAngle";
        // << "Sketcher_ConstrainSnellsLaw" // Rarely used, show only in menu
    }
    cons << "Separator"
         << "Sketcher_ToggleDrivingConstraint"
         << "Sketcher_ToggleActiveConstraint";
}

template<typename T>
inline void SketcherAddWorkbenchTools(T& consaccel);

template<>
inline void SketcherAddWorkbenchTools<Gui::MenuItem>(Gui::MenuItem& consaccel)
{
    consaccel << "Sketcher_SelectElementsWithDoFs"
              << "Sketcher_SelectConstraints"
              << "Sketcher_SelectElementsAssociatedWithConstraints"
              << "Sketcher_SelectRedundantConstraints"
              << "Sketcher_SelectConflictingConstraints"
              << "Sketcher_RestoreInternalAlignmentGeometry"
              << "Separator"
              << "Sketcher_SelectOrigin"
              << "Sketcher_SelectHorizontalAxis"
              << "Sketcher_SelectVerticalAxis"
              << "Separator"
              << "Sketcher_Symmetry"
              << "Sketcher_Clone"
              << "Sketcher_Copy"
              << "Sketcher_Move"
              << "Sketcher_RectangularArray"
              << "Sketcher_RemoveAxesAlignment"
              << "Separator"
              << "Sketcher_DeleteAllGeometry"
              << "Sketcher_DeleteAllConstraints";
}

template<>
inline void SketcherAddWorkbenchTools<Gui::ToolBarItem>(Gui::ToolBarItem& consaccel)
{
    consaccel  //<< "Sketcher_SelectElementsWithDoFs" //rarely used, it is usually accessed by
               // solver
               // message.
        << "Sketcher_SelectConstraints"
        << "Sketcher_SelectElementsAssociatedWithConstraints"
        //<< "Sketcher_SelectRedundantConstraints" //rarely used, it is usually accessed by solver
        // message.
        //<< "Sketcher_SelectConflictingConstraints"
        << "Sketcher_RestoreInternalAlignmentGeometry"
        << "Sketcher_Symmetry"
        << "Sketcher_CompCopy"
        << "Sketcher_RectangularArray"
        << "Sketcher_RemoveAxesAlignment"
        << "Sketcher_DeleteAllConstraints";
}

template<typename T>
inline void SketcherAddWorkbenchBSplines(T& bspline);

template<>
inline void SketcherAddWorkbenchBSplines<Gui::MenuItem>(Gui::MenuItem& bspline)
{
    bspline << "Sketcher_BSplineConvertToNURBS"
            << "Sketcher_BSplineIncreaseDegree"
            << "Sketcher_BSplineDecreaseDegree"
            << "Sketcher_BSplineIncreaseKnotMultiplicity"
            << "Sketcher_BSplineDecreaseKnotMultiplicity"
            << "Sketcher_BSplineInsertKnot"
            << "Sketcher_JoinCurves";
}

template<>
inline void SketcherAddWorkbenchBSplines<Gui::ToolBarItem>(Gui::ToolBarItem& bspline)
{
    bspline << "Sketcher_BSplineConvertToNURBS"
            << "Sketcher_BSplineIncreaseDegree"
            << "Sketcher_BSplineDecreaseDegree"
            << "Sketcher_CompModifyKnotMultiplicity"
            << "Sketcher_BSplineInsertKnot"
            << "Sketcher_JoinCurves";
}

template<typename T>
inline void SketcherAddWorkbenchVisual(T& visual);

template<>
inline void SketcherAddWorkbenchVisual<Gui::MenuItem>(Gui::MenuItem& visual)
{
    visual << "Sketcher_SwitchVirtualSpace"
           << "Sketcher_CompBSplineShowHideGeometryInformation"
           << "Sketcher_ArcOverlay";
}

template<>
inline void SketcherAddWorkbenchVisual<Gui::ToolBarItem>(Gui::ToolBarItem& visual)
{
    visual << "Sketcher_SwitchVirtualSpace"
           << "Sketcher_CompBSplineShowHideGeometryInformation"
           << "Sketcher_ArcOverlay";
}

template<typename T>
inline void SketcherAddWorkbenchEditTools(T& virtualspedittoolsace);

template<>
inline void SketcherAddWorkbenchEditTools<Gui::ToolBarItem>(Gui::ToolBarItem& edittools)
{
    edittools << "Sketcher_Grid"
              << "Sketcher_Snap"
              << "Sketcher_RenderingOrder";
}

void addSketcherWorkbenchSketchActions(Gui::MenuItem& sketch)
{
    SketcherAddWorkbenchSketchActions(sketch);
}

void addSketcherWorkbenchSketchEditModeActions(Gui::MenuItem& sketch)
{
    SketcherAddWorkbenchSketchEditModeActions(sketch);
}

void addSketcherWorkbenchGeometries(Gui::MenuItem& geom)
{
    SketcherAddWorkbenchGeometries(geom);
}

void addSketcherWorkbenchConstraints(Gui::MenuItem& cons)
{
    SketcherAddWorkbenchConstraints(cons);
}

void addSketcherWorkbenchTools(Gui::MenuItem& consaccel)
{
    SketcherAddWorkbenchTools(consaccel);
}

void addSketcherWorkbenchBSplines(Gui::MenuItem& bspline)
{
    SketcherAddWorkbenchBSplines(bspline);
}

void addSketcherWorkbenchVisual(Gui::MenuItem& visual)
{
    SketcherAddWorkbenchVisual(visual);
}

void addSketcherWorkbenchSketchActions(Gui::ToolBarItem& sketch)
{
    SketcherAddWorkbenchSketchActions(sketch);
}

void addSketcherWorkbenchSketchEditModeActions(Gui::ToolBarItem& sketch)
{
    SketcherAddWorkbenchSketchEditModeActions(sketch);
}

void addSketcherWorkbenchGeometries(Gui::ToolBarItem& geom)
{
    SketcherAddWorkbenchGeometries(geom);
}

void addSketcherWorkbenchConstraints(Gui::ToolBarItem& cons)
{
    SketcherAddWorkbenchConstraints(cons);
}

void addSketcherWorkbenchTools(Gui::ToolBarItem& consaccel)
{
    SketcherAddWorkbenchTools(consaccel);
}

void addSketcherWorkbenchBSplines(Gui::ToolBarItem& bspline)
{
    SketcherAddWorkbenchBSplines(bspline);
}

void addSketcherWorkbenchVisual(Gui::ToolBarItem& visual)
{
    SketcherAddWorkbenchVisual(visual);
}

void addSketcherWorkbenchEditTools(Gui::ToolBarItem& edittools)
{
    SketcherAddWorkbenchEditTools(edittools);
}

} /* namespace SketcherGui */
