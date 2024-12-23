/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/MDIView.h>
#include <Mod/Sketcher/Gui/Workbench.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureMultiTransform.h>

#include "Utils.h"
#include "Workbench.h"
#include "WorkflowManager.h"

using namespace PartDesignGui;
namespace sp = std::placeholders;

#if 0 // needed for Qt's lupdate utility
    qApp->translate("Workbench", "&Sketch");
    //
    qApp->translate("Workbench", "&Part Design");
    qApp->translate("Workbench", "Create a datum");
    qApp->translate("Workbench", "Create an additive feature");
    qApp->translate("Workbench", "Create a subtractive feature");
    qApp->translate("Workbench", "Apply a pattern");
    qApp->translate("Workbench", "Apply a dress-up feature");
    qApp->translate("Workbench", "Sprocket...");
    qApp->translate("Workbench", "Involute gear...");
    qApp->translate("Workbench", "Shaft design wizard");
    qApp->translate("Gui::TaskView::TaskWatcherCommands", "Face tools");
    qApp->translate("Gui::TaskView::TaskWatcherCommands", "Edge tools");
    qApp->translate("Gui::TaskView::TaskWatcherCommands", "Boolean tools");
    qApp->translate("Gui::TaskView::TaskWatcherCommands", "Helper tools");
    qApp->translate("Gui::TaskView::TaskWatcherCommands", "Modeling tools");
    qApp->translate("Gui::TaskView::TaskWatcherCommands", "Create Geometry");
    //
    qApp->translate("Workbench", "Measure");
    qApp->translate("Workbench", "Refresh");
    qApp->translate("Workbench", "Toggle 3D");
    qApp->translate("Workbench", "Part Design Helper");
    qApp->translate("Workbench", "Part Design Modeling");
#endif

/// @namespace PartDesignGui @class Workbench
TYPESYSTEM_SOURCE(PartDesignGui::Workbench, Gui::StdWorkbench)

Workbench::Workbench() = default;

Workbench::~Workbench()
{
    WorkflowManager::destruct();
}

void Workbench::setupContextMenu(const char* recipient, Gui::MenuItem* item) const
{
    auto selection = Gui::Selection().getSelection();
    // Add move Tip Command
    if (!selection.empty()) {
        App::DocumentObject *feature = selection.front().pObject;
        PartDesign::Body *body = nullptr;

        body = PartDesignGui::getBodyFor (feature, false, false, true);
        // lote of assertion so feature should be marked as a tip
        if ( selection.size() == 1 && feature && body && (
            feature->isDerivedFrom<PartDesign::Feature>() ||
            ( feature->isDerivedFrom<Part::Feature>() &&
              body->BaseFeature.getValue() == feature )
        ) ) {
            *item << "PartDesign_MoveTip";
        }

        if (strcmp(recipient, "Tree") == 0) {
            Gui::MDIView *activeView = Gui::Application::Instance->activeView();

            if (activeView ) {
                if (feature && feature->isDerivedFrom<PartDesign::Body>()){
                    *item   << "Std_ToggleFreeze";
                }

                bool docHaveBodies = activeView->getAppDocument()->countObjectsOfType (
                                        PartDesign::Body::getClassTypeId () ) > 0;

                if ( docHaveBodies ) {
                    bool addMoveFeature = true;
                    bool addMoveFeatureInTree = (body != nullptr);
                    for (auto sel: selection) {
                        // if at least one selected feature cannot be moved to a body
                        // disable the entry
                        if ( addMoveFeature && !PartDesign::Body::isAllowed ( sel.pObject ) ) {
                            addMoveFeature = false;
                        }
                        // if all at least one selected feature doesn't belong to the same body
                        // disable the menu entry
                        if ( addMoveFeatureInTree && !body->hasObject ( sel.pObject ) ) {
                            addMoveFeatureInTree = false;
                        }

                        if ( !addMoveFeatureInTree && !addMoveFeature ) {
                            break;
                        }
                    }
                    if (addMoveFeature) {
                        *item   << "PartDesign_MoveFeature";
                    }
                    if (addMoveFeatureInTree) {
                        *item   << "PartDesign_MoveFeatureInTree";
                    }
                }
            }

            if (Gui::Selection().countObjectsOfType(PartDesign::Transformed::getClassTypeId()) -
                Gui::Selection().countObjectsOfType(PartDesign::MultiTransform::getClassTypeId()) == 1 )
                *item << "PartDesign_MultiTransform";

            if (Gui::Selection().countObjectsOfType(App::DocumentObject::getClassTypeId()) > 0) {
                *item << "Std_Placement"
                      << "Std_ToggleVisibility"
                      << "Std_ShowSelection"
                      << "Std_HideSelection"
                      << "Std_ToggleSelectability"
                      << "Std_TreeSelectAllInstances"
                      << "Separator"
                      << "Std_RandomColor"
                      << "Std_ToggleTransparency"
                      << "Std_Cut"
                      << "Std_Copy"
                      << "Std_Paste"
                      << "Std_Delete"
                      << "Std_SendToPythonConsole"
                      << "Separator";
            }
        }
    }

    if (strcmp(recipient, "View") == 0) {
        if (item->hasItems())
            *item << "Separator";
        Gui::StdWorkbench::setupContextMenu(recipient, item);
    }
}

void Workbench::activated()
{
    Gui::Workbench::activated();

    WorkflowManager::init();

    std::vector<Gui::TaskView::TaskWatcher*> Watcher;

    const char* Vertex[] = {
        "PartDesign_Point",
        "PartDesign_Line",
        "PartDesign_Plane",
        "PartDesign_CoordinateSystem",
        nullptr};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT Part::Feature SUBELEMENT Vertex COUNT 1..",
        Vertex,
        "Vertex tools",
        "PartDesign_Body"
    ));

    const char* Edge[] = {
        "PartDesign_Fillet",
        "PartDesign_Chamfer",
        "PartDesign_Point",
        "PartDesign_Line",
        "PartDesign_Plane",
        "PartDesign_CoordinateSystem",
        nullptr};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT Part::Feature SUBELEMENT Edge COUNT 1..",
        Edge,
        "Edge tools",
        "PartDesign_Body"
    ));

    const char* Face[] = {
        "PartDesign_NewSketch",
        "PartDesign_Fillet",
        "PartDesign_Chamfer",
        "PartDesign_Draft",
        "PartDesign_Thickness",
        "PartDesign_Point",
        "PartDesign_Line",
        "PartDesign_Plane",
        "PartDesign_CoordinateSystem",
        nullptr};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT Part::Feature SUBELEMENT Face COUNT 1",
        Face,
        "Face tools",
        "PartDesign_Body"
    ));

    const char* Body[] = {
        "PartDesign_NewSketch",
        nullptr};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT PartDesign::Body COUNT 1",
        Body,
        "Helper tools",
        "PartDesign_Body"
    ));

    const char* Body2[] = {
        "PartDesign_Boolean",
        nullptr};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT PartDesign::Body COUNT 1..",
        Body2,
        "Boolean tools",
        "PartDesign_Body"
    ));

    const char* Plane1[] = {
        "PartDesign_NewSketch",
        "PartDesign_Plane",
        "PartDesign_Line",
        "PartDesign_Point",
        "PartDesign_CoordinateSystem",
        nullptr};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT App::Plane COUNT 1",
        Plane1,
        "Helper tools",
        "PartDesign_Body"
    ));
    const char* Plane2[] = {
        "PartDesign_NewSketch",
        "PartDesign_Point",
        "PartDesign_Line",
        "PartDesign_Plane",
        "PartDesign_CoordinateSystem",
        nullptr};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT PartDesign::Plane COUNT 1",
        Plane2,
        "Helper tools",
        "PartDesign_Body"
    ));

    const char* Line[] = {
        "PartDesign_Point",
        "PartDesign_Line",
        "PartDesign_Plane",
        nullptr};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT PartDesign::Line COUNT 1",
        Line,
        "Helper tools",
        "PartDesign_Body"
    ));

    const char* Point[] = {
        "PartDesign_Point",
        "PartDesign_Line",
        "PartDesign_Plane",
        "PartDesign_CoordinateSystem",
        nullptr};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT PartDesign::Point COUNT 1",
        Point,
        "Helper tools",
        "PartDesign_Body"
    ));

    const char* NoSel[] = {
        "PartDesign_Body",
        nullptr};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommandsEmptySelection(
        NoSel,
        "Start Part",
        "Part_Box_Parametric"
    ));

    const char* Faces[] = {
        "PartDesign_Fillet",
        "PartDesign_Chamfer",
        "PartDesign_Draft",
        "PartDesign_Thickness",
        nullptr};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT Part::Feature SUBELEMENT Face COUNT 2..",
        Faces,
        "Face tools",
        "PartDesign_Body"
    ));

    const char* Sketch[] = {
        "PartDesign_NewSketch",
        "PartDesign_Pad",
        "PartDesign_Pocket",
        "PartDesign_Hole",
        "PartDesign_Revolution",
        "PartDesign_Groove",
        "PartDesign_AdditivePipe",
        "PartDesign_SubtractivePipe",
        "PartDesign_AdditiveLoft",
        "PartDesign_SubtractiveLoft",
        "PartDesign_AdditiveHelix",
        "PartDesign_SubtractiveHelix",
        nullptr};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT Sketcher::SketchObject COUNT 1",
        Sketch,
        "Modeling tools",
        "PartDesign_Body"
    ));

    const char* Transformed[] = {
        "PartDesign_Mirrored",
        "PartDesign_LinearPattern",
        "PartDesign_PolarPattern",
        "PartDesign_MultiTransform",
        nullptr};
    Watcher.push_back(new Gui::TaskView::TaskWatcherCommands(
        "SELECT PartDesign::SketchBased",
        Transformed,
        "Transformation tools",
        "PartDesign_MultiTransform"
    ));

    addTaskWatcher(Watcher);
    if(App::GetApplication().GetUserParameter().GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/PartDesign")->GetBool("SwitchToTask", true))
        Gui::Control().showTaskView();
}

void Workbench::deactivated()
{
    removeTaskWatcher();
    // reset the active Body
    Gui::Command::doCommand(Gui::Command::Doc,"import PartDesignGui");

    Gui::Workbench::deactivated();
}

Gui::MenuItem* Workbench::setupMenuBar() const
{
    Gui::MenuItem* root = StdWorkbench::setupMenuBar();
    Gui::MenuItem* item = root->findItem("&Windows");

    // add another top level menu left besides the Part Design menu for the Sketcher commands
    Gui::MenuItem* sketch = new Gui::MenuItem;
    root->insertItem(item, sketch);
    sketch->setCommand("&Sketch");

    *sketch << "PartDesign_NewSketch"
            << "Sketcher_EditSketch"
            << "Sketcher_MapSketch"
            << "Sketcher_ReorientSketch"
            << "Sketcher_ValidateSketch"
            << "Sketcher_MergeSketches"
            << "Sketcher_MirrorSketch";

    Gui::MenuItem* part = new Gui::MenuItem;
    root->insertItem(item, part);
    part->setCommand("&Part Design");

    // additives
    Gui::MenuItem* additives = new Gui::MenuItem;
    additives->setCommand("Create an additive feature");

    *additives << "PartDesign_Pad"
               << "PartDesign_Revolution"
               << "PartDesign_AdditiveLoft"
               << "PartDesign_AdditivePipe"
               << "PartDesign_AdditiveHelix";

    // subtractives
    Gui::MenuItem* subtractives = new Gui::MenuItem;
    subtractives->setCommand("Create a subtractive feature");

    *subtractives << "PartDesign_Pocket"
                  << "PartDesign_Hole"
                  << "PartDesign_Groove"
                  << "PartDesign_SubtractiveLoft"
                  << "PartDesign_SubtractivePipe"
                  << "PartDesign_SubtractiveHelix";

    // transformations
    Gui::MenuItem* transformations = new Gui::MenuItem;
    transformations->setCommand("Apply a pattern");

    *transformations << "PartDesign_Mirrored"
                     << "PartDesign_LinearPattern"
                     << "PartDesign_PolarPattern"
                     << "PartDesign_MultiTransform";

    // dressups
    Gui::MenuItem* dressups = new Gui::MenuItem;
    dressups->setCommand("Apply a dress-up feature");

    *dressups << "PartDesign_Fillet"
              << "PartDesign_Chamfer"
              << "PartDesign_Draft"
              << "PartDesign_Thickness";

    *part << "PartDesign_Body"
          << "Separator"
          << "PartDesign_ShapeBinder"
          << "PartDesign_SubShapeBinder"
          << "PartDesign_Clone"
          << "Separator"
          << additives
          << "PartDesign_CompPrimitiveAdditive"
          << "Separator"
          << subtractives
          << "PartDesign_CompPrimitiveSubtractive"
          << "Separator"
          << transformations
          << "Separator"
          << dressups
          << "Separator"
          << "Materials_InspectAppearance"
          << "Materials_InspectMaterial"
          << "Separator"
          << "PartDesign_Boolean"
          << "Separator"
          << "Part_CheckGeometry"
          << "Separator"
          << "PartDesign_InvoluteGear"
          << "PartDesign_Sprocket";

    // For 0.13 a couple of python packages like numpy, matplotlib and others
    // are not deployed with the installer on Windows. Thus, the WizardShaft is
    // not deployed either hence the check for the existence of the command.
    if (Gui::Application::Instance->commandManager().getCommandByName("PartDesign_WizardShaft")) {
        *part << "Separator" << "PartDesign_WizardShaft";
    }

    Gui::MenuItem* view = root->findItem("&View");
    if (view) {
        Gui::MenuItem* appr = view->findItem("Std_RandomColor");
        appr = view->afterItem(appr);
        Gui::MenuItem* face = new Gui::MenuItem();
        face->setCommand("Part_ColorPerFace");
        view->insertItem(appr, face);
    }

    // Replace the "Duplicate selection" menu item with a replacement that is compatible with Body
    item = root->findItem("&Edit");
    Gui::MenuItem* dup = item->findItem("Std_DuplicateSelection");
    dup->setCommand("PartDesign_DuplicateSelection");

    return root;
}

Gui::ToolBarItem* Workbench::setupToolBars() const
{
    Gui::ToolBarItem* root = StdWorkbench::setupToolBars();
    Gui::ToolBarItem* part = new Gui::ToolBarItem(root);
    part->setCommand("Part Design Helper");

    *part << "PartDesign_Body"
          << "PartDesign_CompSketches"
          << "Sketcher_ValidateSketch"
          << "Part_CheckGeometry"
          << "PartDesign_SubShapeBinder"
          << "PartDesign_Clone";

    part = new Gui::ToolBarItem(root);
    part->setCommand("Part Design Modeling");

    *part << "PartDesign_Pad"
          << "PartDesign_Revolution"
          << "PartDesign_AdditiveLoft"
          << "PartDesign_AdditivePipe"
          << "PartDesign_AdditiveHelix"
          << "PartDesign_CompPrimitiveAdditive"
          << "Separator"
          << "PartDesign_Pocket"
          << "PartDesign_Hole"
          << "PartDesign_Groove"
          << "PartDesign_SubtractiveLoft"
          << "PartDesign_SubtractivePipe"
          << "PartDesign_SubtractiveHelix"
          << "PartDesign_CompPrimitiveSubtractive"
          << "Separator"
          << "PartDesign_Boolean";

    part = new Gui::ToolBarItem(root);

    part->setCommand("Part Design Dressup");
    *part << "PartDesign_Fillet"
          << "PartDesign_Chamfer"
          << "PartDesign_Draft"
          << "PartDesign_Thickness";

    part = new Gui::ToolBarItem(root);
    part->setCommand("Part Design Patterns");

    *part << "PartDesign_Mirrored"
          << "PartDesign_LinearPattern"
          << "PartDesign_PolarPattern"
          << "PartDesign_MultiTransform";

    return root;
}
