// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 The FreeCAD project associatioon AISBL
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include "TourStops.h"

#include <algorithm>
#include <utility>

#include <QCoreApplication>
#include <QDockWidget>
#include <QMainWindow>
#include <QToolBar>

#include <Gui/WorkbenchSelector.h>

namespace StartGui
{
// See TourStops.h -- looked up by value from Tour.cpp.
const QString kNewSketchId = QStringLiteral("New Sketch");
const QString kReadMoreId = QStringLiteral("Read More");

TourStop::TourStop(
    QWidget* widgetToHighlight,
    QStringList commandsToHighlight,
    QString id,
    QString chapterLabel,
    QString headline,
    QString description,
    bool isSubchapter,
    QList<StageRequirement> stages
)
    : widgetToHighlight(widgetToHighlight)
    , commandsToHighlight(std::move(commandsToHighlight))
    , id(std::move(id))
    , chapterLabel(std::move(chapterLabel))
    , headline(std::move(headline))
    , description(std::move(description))
    , isSubchapter(isSubchapter)
    , stages(std::move(stages))
{}

namespace
{

QWidget* workbenchSelector(const QMainWindow* mainWindow)
{
    const auto selectors = mainWindow->findChildren<Gui::WorkbenchComboBox*>();
    if (!selectors.empty()) {
        return selectors.front();
    }
    return mainWindow->findChild<QWidget*>(QStringLiteral("WbTabBar"));
}

QToolBar* firstToolBar(const QMainWindow* mainWindow)
{
    const auto toolBars = mainWindow->findChildren<QToolBar*>();
    return toolBars.empty() ? nullptr : toolBars.front();
}

QToolBar* partDesignToolBar(const QMainWindow* mainWindow)
{
    for (auto toolbar : mainWindow->findChildren<QToolBar*>()) {
        for (auto action : toolbar->actions()) {
            if (action->objectName() == QStringLiteral("PartDesign_Pad")
                || action->objectName() == QStringLiteral("PartDesign_Pocket")) {
                return toolbar;
            }
        }
    }
    return firstToolBar(mainWindow);
}

QDockWidget* findDock(const QMainWindow* mainWindow, const QStringList& objectNames)
{
    for (auto dock : mainWindow->findChildren<QDockWidget*>()) {
        if (objectNames.contains(dock->objectName())) {
            return dock;
        }
    }
    return nullptr;
}

QString tr(const char* text)
{
    return QCoreApplication::translate("TourStops", text);
}

QString icon(const char* path)
{
    return QStringLiteral("<img src=\":/icons/%1\" width=\"16\" height=\"16\">").arg(path);
}

}  // namespace

QList<TourStop> buildStops(const QMainWindow* mainWindow)
{
    const QStringList additiveSubtractiveCommands {
        QStringLiteral("PartDesign_Pad"),
        QStringLiteral("PartDesign_Pocket"),
        QStringLiteral("PartDesign_Revolution"),
        QStringLiteral("PartDesign_Groove"),
        QStringLiteral("PartDesign_AdditiveLoft"),
        QStringLiteral("PartDesign_SubtractiveLoft"),
        QStringLiteral("PartDesign_AdditivePipe"),
        QStringLiteral("PartDesign_SubtractivePipe"),
        QStringLiteral("PartDesign_AdditiveHelix"),
        QStringLiteral("PartDesign_SubtractiveHelix")
    };

    const QList<StageRequirement> sketcherWorkbenchStages {
        {TourStage::Workbench, QStringLiteral("SketcherWorkbench")}
    };
    const QList<StageRequirement> sketchEditStages {
        {TourStage::Workbench, QStringLiteral("SketcherWorkbench")},
        {TourStage::SketchEdit, {}}
    };
    const QList<StageRequirement> partDesignWorkbenchStages {
        {TourStage::Workbench, QStringLiteral("PartDesignWorkbench")}
    };
    const QList<StageRequirement> partWorkbenchStages {
        {TourStage::Workbench, QStringLiteral("PartWorkbench")}
    };

    QList<TourStop> candidates {
        {nullptr, {}, QStringLiteral("Welcome"), tr("Welcome"), tr("Welcome to the project"), {}, false},

        {findDock(
             mainWindow,
             {QStringLiteral("Model"),
              QStringLiteral("Tree view"),
              QStringLiteral("Std_TreeView"),
              QStringLiteral("Std_ComboView")}
         ),
         {},
         QStringLiteral("Tree"),
         tr("Tree"),
         tr("Read the tree"),
         tr("The tree shows what is currently open in a project: documents, bodies, and the other "
            "containers and objects they contain. A %1 document contains your design, while a %2 "
            "Part container groups objects together for organization, and a %3 Body groups the "
            "ordered feature history that Part Design builds up. Either way, a shape object stores "
            "actual geometry, while a Part container or Body only organizes what's inside it. The "
            "%4 Tip marks a Body's current, up-to-date result, and the %5 eye icon shows whether "
            "an object is visible right now -- that's independent of where it sits in the tree.")
             .arg(
                 icon("Document.svg"),
                 icon("Part_3D_object.svg"),
                 icon("PartDesign_Body.svg"),
                 icon("PartDesign_Overlay_Tip.svg"),
                 icon("Std_ToggleVisibility.svg")
             ),
         false},

        {findDock(
             mainWindow,
             {QStringLiteral("Model"),
              QStringLiteral("Tree view"),
              QStringLiteral("Std_TreeView"),
              QStringLiteral("Std_ComboView")}
         ),
         {},
         QStringLiteral("Origin Folder"),
         tr("Origin Folder"),
         tr("Hidden by default"),
         tr("Every Body (and Part) quietly comes with an Origin folder, marked by the %1 icon, "
            "holding the standard %2 XY, XZ, and YZ planes and the %3 X, Y, and Z axes -- handy "
            "references for attaching sketches or aligning features, hidden from the tree by "
            "default because most day-to-day work doesn't need them. Right-click in the tree and "
            "enable \"Show hidden items\" (or select the Origin and press the spacebar) to reveal "
            "it, then toggle any single plane or axis the same way.")
             .arg(icon("Std_CoordinateSystem.svg"), icon("Std_Plane.svg"), icon("Std_Axis.svg")),
         true},

        {findDock(
             mainWindow,
             {QStringLiteral("Model"),
              QStringLiteral("Tree view"),
              QStringLiteral("Std_TreeView"),
              QStringLiteral("Std_ComboView")}
         ),
         {},
         QStringLiteral("Rollback"),
         tr("Rollback"),
         tr("Editing an earlier feature"),
         tr("Double-clicking a feature partway down the tree -- instead of the last one -- edits "
            "it in place, and temporarily hides every feature that comes after it, since they "
            "depend on results that are about to change. That's not data loss: finish or cancel "
            "the edit and everything reappears, recomputed. If a later feature ever seems stuck "
            "hidden, right-click the Body and choose \"Move tip to end\" to bring the tree back "
            "to showing the full, final result."),
         true},

        {workbenchSelector(mainWindow),
         {},
         QStringLiteral("Sketcher Workbench"),
         tr("Sketcher Workbench"),
         tr("Sketcher Workbench"),
         tr("%1 Sketcher uses lines, arcs, and dimensional constraints; stores a flat 2D profile; "
            "is not itself a solid. %2 Part Design uses a sketch as the profile for features such "
            "as %3 Pad or %4 Pocket, and can turn it around an axis with %5 Revolution. %6 Part "
            "can use one as a profile for equivalent tools such as %7 Extrude or %8 Revolve.")
             .arg(
                 icon("SketcherWorkbench.svg"),
                 icon("PartDesignWorkbench.svg"),
                 icon("PartDesign_Pad.svg"),
                 icon("PartDesign_Pocket.svg"),
                 icon("PartDesign_Revolution.svg"),
                 icon("PartWorkbench.svg"),
                 icon("tools/Part_Extrude.svg"),
                 icon("tools/Part_Revolve.svg")
             ),
         false,
         sketcherWorkbenchStages},

        {nullptr,
         {},
         QStringLiteral("Constraint Colors"),
         tr("Constraint Colors"),
         tr("Read the colors"),
         tr("Sketch geometry changes color to tell you its status. White (or black, depending on "
            "your theme) means a line or curve isn't fully constrained yet -- it can still move. "
            "Green means fully constrained -- its position is locked down completely. Red or "
            "orange means a constraint conflicts with another or is redundant; the Report view "
            "will tell you which ones so you can remove one. Aim for a fully constrained, green "
            "sketch before using it in a feature."),
         true,
         sketcherWorkbenchStages},

        {nullptr,
         {QStringLiteral("Sketcher_NewSketch")},
         kNewSketchId,
         tr("New Sketch"),
         tr("Start a sketch"),
         tr("%1 Every Part Design feature starts from a sketch. The New Sketch button is "
            "highlighted. Click Next and this tour will create a sketch on the XY plane for you.")
             .arg(icon("general/Sketcher_NewSketch.svg")),
         true,
         sketcherWorkbenchStages},

        {nullptr,
         {},
         QStringLiteral("Select Plane"),
         tr("Select a Plane"),
         tr("Pick a plane"),
         tr("A sketch needs a flat plane to sit on. The tour attaches this one to the %1 XY plane, "
            "whose normal points along Z, and starts editing it.")
             .arg(icon("Std_Plane.svg")),
         true,
         sketchEditStages},

        {nullptr,
         {QStringLiteral("Sketcher_CompExternal")},
         QStringLiteral("External Projection"),
         tr("External Projection"),
         tr("External geometry"),
         tr("External geometry copies the outline of selected edges into the active sketch as a "
            "read-only reference, so you can constrain new geometry to it. Look for its %1 icon, "
            "highlighted here in the Sketcher toolbar. It helps line up a sketch with existing "
            "geometry, but it is not a solid and does not create a second document.")
             .arg(icon("geometry/Sketcher_Projection.svg")),
         true,
         sketchEditStages},

        {workbenchSelector(mainWindow),
         {},
         QStringLiteral("Part Design Workbench"),
         tr("Part Design Workbench"),
         tr("Part Design Workbench"),
         tr("%1 Part Design: uses a Body; stores an ordered, editable history of features, for "
            "example a sketch, then a %2 Pad, then a %3 Chamfer; is processed as a single solid "
            "built up step by step, which can be edited or rolled back at any point.")
             .arg(icon("PartDesign_Body.svg"), icon("PartDesign_Pad.svg"), icon("PartDesign_Chamfer.svg")),
         false,
         partDesignWorkbenchStages},

        {partDesignToolBar(mainWindow),
         additiveSubtractiveCommands,
         QStringLiteral("Additive / Subtractive"),
         tr("Additive / Subtractive"),
         tr("Additive and subtractive"),
         tr("Additive and subtractive features belong to the Part Design workbench -- highlighted "
            "here is the strip of feature commands where each one either adds material (like %1 "
            "Pad) or removes it (like %2 Pocket). The strip also includes operations such as %3 "
            "Revolution. Don't rely on color alone: read the command name or its tooltip. Pick a "
            "sketch, choose the operation, then set its length or depth in the Tasks panel.")
             .arg(
                 icon("PartDesign_Pad.svg"),
                 icon("PartDesign_Pocket.svg"),
                 icon("PartDesign_Revolution.svg")
             ),
         true,
         partDesignWorkbenchStages},

        {nullptr,
         {QStringLiteral("PartDesign_SubShapeBinder")},
         QStringLiteral("SubShape Binder"),
         tr("SubShape Binder"),
         tr("Referencing without duplicating"),
         tr("The green %1 icon is the Sub-Shape Binder: a live, updating reference to selected "
            "geometry from another object, such as a solid, face, edge, or vertex. The Sub-Shape "
            "Binder appears inside the current Body, so you can build against that geometry "
            "without duplicating it -- a reference, not a copy, and not a separate document. The "
            "old, blue %2 Shape Binder references an object's whole shape.")
             .arg(icon("PartDesign_SubShapeBinder.svg"), icon("PartDesign_ShapeBinder.svg")),
         true,
         partDesignWorkbenchStages},

        {workbenchSelector(mainWindow),
         {},
         QStringLiteral("Part Workbench"),
         tr("Part Workbench"),
         tr("Part Workbench"),
         tr("%1 Part: works with solids you've already created -- primitives like a cube or "
            "cylinder, "
            "or shapes brought in from other workbenches -- optionally grouped in a Part container "
            "for organization; stores shape objects directly, not a feature history; is processed "
            "immediately, so a boolean tool fuses, cuts, or intersects the geometry you select, "
            "such as a cube and a sphere. A %2 boolean operation combines selected shapes. %3 "
            "Extrude and %4 Revolve turn profiles into solids.")
             .arg(
                 icon("Part_3D_object.svg"),
                 icon("booleans/Part_Booleans.svg"),
                 icon("tools/Part_Extrude.svg"),
                 icon("tools/Part_Revolve.svg")
             ),
         false,
         partWorkbenchStages},

        {nullptr,
         {},
         QStringLiteral("Layout"),
         tr("Layout"),
         tr("Make it yours"),
         tr("Toolbars, dock panels, and tabs can all be rearranged. Drag a panel by its title bar "
            "to dock it against an edge or beside another panel. Each panel's title bar also has "
            "a small float button that pops it into its own window, and a close button that hides "
            "it -- bring a hidden panel back later from the View menu. Keep the Model and Tasks "
            "panels wherever your eyes naturally rest."),
         false},

        {firstToolBar(mainWindow),
         {QStringLiteral("Std_Refresh")},
         QStringLiteral("Recompute"),
         tr("Recompute"),
         tr("Stay up to date"),
         tr("FreeCAD doesn't always update automatically. After certain edits, the document needs "
            "recomputing before its geometry catches up -- look for the recompute icon (a small "
            "refresh arrow) lighting up in the toolbar. Click it, or press the shortcut, to bring "
            "everything current before you keep building."),
         false},

        {findDock(mainWindow, {QStringLiteral("Report view"), QStringLiteral("Std_ReportView")}),
         {},
         QStringLiteral("Report View"),
         tr("Report View"),
         tr("Check the Report view"),
         tr("Warnings and errors often show up only in the Report view, a panel that's hidden by "
            "default. Open it from the View menu, under Panels, then Report view. It's the first "
            "place to look when a recompute fails or a command doesn't do what you expected."),
         false},

        {nullptr,
         {},
         kReadMoreId,
         tr("Read More"),
         tr("Keep learning"),
         tr("Continue with FreeCAD's Getting Started guide for hands-on examples and deeper "
            "explanations. Use the Read more button below to open it in your browser."),
         false}
    };

    return candidates;
}

}  // namespace StartGui
