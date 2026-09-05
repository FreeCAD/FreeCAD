// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 The FreeCAD Project Association AISBL               *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#pragma once

#include <QList>
#include <QMainWindow>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QWidget>

namespace StartGui
{

// IDs for stops Tour.cpp needs to reference directly (see its uses). Add one here only when
// code outside buildStops() actually looks it up.
extern const QString kNewSketchId;
extern const QString kReadMoreId;

// An environment a stop needs active while shown (a workbench, a sketch being edited). `param`
// is whatever that stage needs to enter itself; see each enumerator.
//
// To add a new stage kind, edit 3 places in total:
//   1. The enum here, with a comment describing the `param` it expects.
//   2. TourOverlay::enterStage() / exitStage() in Tour.cpp to react to that stage.
//   3. The relevant stop list(s) in buildStops() in TourStops.cpp.
//
// Keep names normalized: stage kinds are nouns of the environment being activated, while stop
// ids and chapter labels stay descriptive of the narrative step the user is shown.
enum class TourStage
{
    Workbench,   // param: QString, workbench's internal name (e.g. "PartDesignWorkbench").
    SketchEdit,  // param: QString, sketch to open; empty reuses the sketch already being
                 // edited, else the first sketch in the document, else creates one. See
                 // openSketchForEdit().
};

// One entry in a stop's `stages` list. List outermost-to-innermost (Workbench before
// SketchEdit) -- transitions tear down and rebuild in that order, see transitionStages().
struct StageRequirement
{
    TourStage stage;
    QVariant param;

    bool operator==(const StageRequirement& other) const
    {
        return stage == other.stage && param == other.param;
    }
    bool operator!=(const StageRequirement& other) const
    {
        return !(*this == other);
    }
};

struct TourStop
{
    // A stop targets either a single widget (dock, panel, workbench selector, toolbar) or a set of
    // command actions that should be unioned into a single highlight rectangle. If both are empty,
    // the stop is informational only and no callout is drawn.
    QWidget* widgetToHighlight = nullptr;
    QStringList commandsToHighlight;

    QString id;
    QString chapterLabel;
    QString headline;
    QString description;

    bool isSubchapter = false;

    // Outermost-to-innermost stages this stop needs active. Empty means no requirement. Diffed
    // against the previous stop's stages regardless of how the user got here (Next, Back, jump).
    QList<StageRequirement> stages;

    // The highlight target is expressed in the most natural form for the UI element involved:
    //   - widgetToHighlight: a single QWidget/dock/tab/workbench selector
    //   - commandsToHighlight: a toolbar command strip identified by action names
    // The caller decides which style fits a given step in buildStops().
    TourStop(
        QWidget* widgetToHighlight = nullptr,
        QStringList commandsToHighlight = {},
        QString id = {},
        QString chapterLabel = {},
        QString headline = {},
        QString description = {},
        bool isSubchapter = false,
        QList<StageRequirement> stages = {}
    );
};

QList<TourStop> buildStops(const QMainWindow* mainWindow);

}  // namespace StartGui
