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

#include "Tour.h"

#include <algorithm>

#include <QDockWidget>
#include <QDesktopServices>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QMenuBar>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QResizeEvent>
#include <QStatusBar>
#include <QToolBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QUrl>

#include <Gui/BitmapFactory.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MDIView.h>
#include <Gui/WorkbenchSelector.h>

#include <Base/Bitmask.h>
#include <Base/Interpreter.h>

namespace StartGui
{
enum class TourStopExitAction : int
{
    None = 0,
    CreateSketchOnXYPlane = 1 << 0,
    LeaveSketchEditMode = 1 << 1
};
}  // namespace StartGui

ENABLE_BITMASK_OPERATORS(StartGui::TourStopExitAction)

namespace StartGui
{
namespace
{

// Chapter ids used internally (advancing logic, button visibility). These are never shown to the
// user directly -- see TourStop::chapterLabel and TourStop::headline for the text that is shown.
const auto kWelcomeId = QStringLiteral("Welcome");
const auto kSketcherWorkbenchId = QStringLiteral("Sketcher Workbench");
const auto kNewSketchId = QStringLiteral("New Sketch");
const auto kSelectPlaneId = QStringLiteral("Select Plane");
const auto kExternalProjectionId = QStringLiteral("External Projection");
const auto kSubShapeBinderId = QStringLiteral("SubShape Binder");
const auto kReadMoreId = QStringLiteral("Read More");

class TourOverlay: public QWidget
{
public:
    explicit TourOverlay(QMainWindow* mainWindow)
        : QWidget(mainWindow)
        , _mainWindow(mainWindow)
    {
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TranslucentBackground);

        _bubble = new QFrame(this);
        _bubble->setStyleSheet(
            "QFrame { background: #20252b; border: none; border-radius: 8px; }"
            "QLabel { color: #f5f7f8; }"
            "QPushButton { background: #3d8f9b; color: white; border: 0; border-radius: 4px; "
            "padding: 6px 12px; }"
            "QPushButton:hover { background: #50a9b5; }"
        );
        auto layout = new QVBoxLayout(_bubble);
        auto topRow = new QHBoxLayout();
        auto mascot = new QLabel();
        const auto cloneIcon = QIcon(Gui::BitmapFactory().pixmap("PartDesign_Clone"));
        mascot->setPixmap(cloneIcon.pixmap(QSize(26, 26)));
        _headline = new QLabel();
        _headline->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 13px;"));
        topRow->addWidget(mascot);
        topRow->addWidget(_headline);
        topRow->addStretch();
        layout->addLayout(topRow);

        _body = new QLabel();
        _body->setWordWrap(true);
        _body->setFixedWidth(340);
        _body->setTextFormat(Qt::RichText);
        layout->addWidget(_body);

        auto buttonRow = new QHBoxLayout();
        auto skipButton = new QPushButton(QStringLiteral("Skip tour"));
        connect(skipButton, &QPushButton::clicked, this, &TourOverlay::closeTour);
        auto readMoreButton = new QPushButton(QStringLiteral("Read more"));
        _readMoreButton = readMoreButton;
        _readMoreButton->hide();
        connect(readMoreButton, &QPushButton::clicked, this, []() {
            QDesktopServices::openUrl(QUrl(QStringLiteral("https://wiki.freecad.org/Getting_started")));
        });
        _nextButton = new QPushButton(QStringLiteral("Next"));
        connect(_nextButton, &QPushButton::clicked, this, &TourOverlay::advance);
        buttonRow->addWidget(skipButton);
        buttonRow->addWidget(readMoreButton);
        buttonRow->addStretch();
        buttonRow->addWidget(_nextButton);
        layout->addLayout(buttonRow);

        _chapters = new QListWidget(this);
        _chapters->setStyleSheet(
            "QListWidget { background: #20252b; color: #d7dde2; border: 1px solid #59636e; "
            "border-radius: 8px; padding: 4px; }"
            "QListWidget::item { padding: 7px 10px; }"
            "QListWidget::item:selected { background: #3d8f9b; color: white; border-radius: 4px; }"
        );
        _chapters->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        _chapters->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        connect(_chapters, &QListWidget::currentRowChanged, this, &TourOverlay::showStop);

        _stops = buildStops();
        _stops.front().description = QStringLiteral(
            "Welcome to your project. This short tour will take just a few minutes to familiarize "
            "you with FreeCAD's document tree, workbenches, and the sketch and modeling commands "
            "you'll use most."
        );
        for (const auto& stop : _stops) {
            // Subsections are indented under the chapter they belong to, with an em dash so the
            // hierarchy is obvious at a glance rather than relying on faint leading spaces.
            auto chapterItem = new QListWidgetItem(
                stop.isSubchapter ? QStringLiteral("   \u2014 %1").arg(stop.chapterLabel)
                                  : stop.chapterLabel
            );
            _chapters->addItem(chapterItem);
        }
        setGeometry(_mainWindow->rect());
        showStop(0);
    }

protected:
    void resizeEvent(QResizeEvent* event) override
    {
        QWidget::resizeEvent(event);
        setGeometry(_mainWindow->rect());
        showStop(_index);
    }

    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        QPainterPath path;
        path.addRect(QRectF(rect()));
        if (!_targetRect.isNull()) {
            QPainterPath hole;
            hole.addRoundedRect(QRectF(_targetRect).adjusted(-6, -6, 6, 6), 8, 8);
            path = path.subtracted(hole);
        }
        painter.fillPath(path, QColor(0, 0, 0, 150));
        if (!_targetRect.isNull()) {
            painter.setPen(QPen(QColor(91, 190, 198), 2));
            painter.drawRoundedRect(QRectF(_targetRect).adjusted(-6, -6, 6, 6), 8, 8);
        }
    }

private:
    struct TourStop
    {
        // Fallback widget to highlight. Used directly when commandNames is empty, and used as a
        // fallback if none of commandNames can be found (e.g. a customized toolbar layout).
        QWidget* widget = nullptr;
        // Stable internal identifier used by advance()/showStop() logic. Never shown to the user.
        QString id;
        // Short label shown in the chapter outline on the left.
        QString chapterLabel;
        // Bold title shown at the top of the tour bubble.
        QString headline;
        // Rich text body of the tour bubble.
        QString description;
        // Toolbar command(s) to highlight. When there is more than one, the highlighted rectangle
        // spans all of them, so a whole strip of related buttons can be called out at once.
        QStringList commandNames;
        // Whether this stop draws a spotlight at all. Some stops (Welcome, Read More, Layout,
        // Commands) are purely explanatory and highlight nothing.
        bool highlight = true;
        // Indents this entry under the previous top-level chapter in the outline.
        bool isSubchapter = false;
        // Workbench to activate when this stop is selected. Keep empty when no switch is required.
        QString workbenchName;
        // Optional action to perform when the user moves to the next stop. This keeps the stop's
        // behavior co-located with the stop data instead of hidden in id-based branching.
        Base::Flags<TourStopExitAction> onExit = TourStopExitAction::None;

        TourStop(
            QWidget* widget = nullptr,
            QString id = {},
            QString chapterLabel = {},
            QString headline = {},
            QString description = {},
            QStringList commandNames = {},
            bool highlight = true,
            bool isSubchapter = false,
            QString workbenchName = {},
            Base::Flags<TourStopExitAction> onExit = TourStopExitAction::None
        )
            : widget(widget)
            , id(std::move(id))
            , chapterLabel(std::move(chapterLabel))
            , headline(std::move(headline))
            , description(std::move(description))
            , commandNames(std::move(commandNames))
            , highlight(highlight)
            , isSubchapter(isSubchapter)
            , workbenchName(std::move(workbenchName))
            , onExit(onExit)
        {}
    };

    QList<TourStop> buildStops() const
    {
        // Additive and subtractive PartDesign features that normally sit together as one strip of
        // buttons on the Part Design toolbar. The highlight below spans whichever of these are
        // actually present, so it still works if the toolbar has been customized.
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

        QList<TourStop> candidates {
            {nullptr,
             kWelcomeId,
             QStringLiteral("Welcome"),
             QStringLiteral("Welcome to the project"),
             QString(),
             {},
             false,
             false},

            {findDock({QStringLiteral("Tree View"), QStringLiteral("Model")}),
             QStringLiteral("Tree"),
             QStringLiteral("Tree"),
             QStringLiteral("Read the tree"),
             QStringLiteral(
                 "The tree shows what is currently open in a project: documents, bodies, and the "
                 "other containers and objects they contain. A <img src=\":/icons/Document.svg\" "
                 "width=\"16\" "
                 "height=\"16\"> document contains your design, while a "
                 "<img src=\":/icons/Part_3D_object.svg\" width=\"16\" height=\"16\"> Part "
                 "container "
                 "groups objects together for organization, and a <img "
                 "src=\":/icons/PartDesign_Body.svg\" "
                 "width=\"16\" height=\"16\"> Body groups the ordered feature history that Part "
                 "Design "
                 "builds up. Either way, a shape object stores actual geometry, while a Part "
                 "container or "
                 "Body only organizes what's inside it. The <img "
                 "src=\":/icons/PartDesign_Overlay_Tip.svg\" "
                 "width=\"16\" height=\"16\"> Tip marks a Body's current, up-to-date result, and "
                 "the "
                 "<img src=\":/icons/Std_ToggleVisibility.svg\" width=\"16\" height=\"16\"> eye "
                 "icon shows "
                 "whether an object is visible right now -- that's independent of where it sits in "
                 "the "
                 "tree."
             ),
             {}},


            {findDock({QStringLiteral("Tree View"), QStringLiteral("Model")}),
             QStringLiteral("Origin Folder"),
             QStringLiteral("Origin Folder"),
             QStringLiteral("Hidden by default"),
             QStringLiteral(
                 "Every Body (and Part) quietly comes with an Origin folder, marked by the <img "
                 "src=\":/icons/Std_CoordinateSystem.svg\" width=\"16\" height=\"16\"> icon, "
                 "holding the standard "
                 "<img src=\":/icons/Std_Plane.svg\" width=\"16\" height=\"16\"> XY, XZ, and YZ "
                 "planes and the <img src=\":/icons/Std_Axis.svg\" width=\"16\" height=\"16\"> X, "
                 "Y, "
                 "and Z axes -- handy references for attaching "
                 "sketches or aligning features, hidden from the tree by default because most "
                 "day-to-day work doesn't need them. Right-click in the tree and enable \"Show "
                 "hidden "
                 "items\" (or select the Origin and press the spacebar) to reveal it, then toggle "
                 "any "
                 "single plane or axis the same way."
             ),
             {},
             true,
             true},

            {findDock({QStringLiteral("Tree View"), QStringLiteral("Model")}),
             QStringLiteral("Rollback"),
             QStringLiteral("Rollback"),
             QStringLiteral("Editing an earlier feature"),
             QStringLiteral(
                 "Double-clicking a feature partway down the tree -- instead of the last one -- "
                 "edits "
                 "it in place, and temporarily hides every feature that comes after it, since they "
                 "depend on results that are about to change. That's not data loss: finish or "
                 "cancel "
                 "the edit and everything reappears, recomputed. If a later feature ever seems "
                 "stuck "
                 "hidden, right-click the Body and choose \"Move tip to end\" to bring the tree "
                 "back "
                 "to showing the full, final result."
             ),
             {},
             true,
             true},

            {workbenchSelector(),
             kSketcherWorkbenchId,
             QStringLiteral("Sketcher Workbench"),
             QStringLiteral("Sketcher Workbench"),
             QStringLiteral(
                 "<img src=\":/icons/SketcherWorkbench.svg\" width=\"16\" height=\"16\"> Sketcher "
                 "uses lines, arcs, and dimensional constraints; stores a flat 2D profile; is "
                 "not itself a solid. <img src=\":/icons/PartDesignWorkbench.svg\" width=\"16\" "
                 "height=\"16\"> Part Design uses a sketch as the profile for features such as "
                 "<img src=\":/icons/PartDesign_Pad.svg\" width=\"16\" height=\"16\"> Pad or "
                 "<img src=\":/icons/PartDesign_Pocket.svg\" width=\"16\" height=\"16\"> Pocket, "
                 "and can turn it around an axis with "
                 "<img src=\":/icons/PartDesign_Revolution.svg\" width=\"16\" height=\"16\"> "
                 "Revolution. "
                 "<img src=\":/icons/PartWorkbench.svg\" width=\"16\" height=\"16\"> Part can use "
                 "one as a profile for equivalent tools such as "
                 "<img src=\":/icons/tools/Part_Extrude.svg\" width=\"16\" height=\"16\"> Extrude "
                 "or "
                 "<img src=\":/icons/tools/Part_Revolve.svg\" width=\"16\" height=\"16\"> Revolve."
             ),
             {},
             false,
             false,
             QStringLiteral("SketcherWorkbench")},

            {nullptr,
             QStringLiteral("Constraint Colors"),
             QStringLiteral("Constraint Colors"),
             QStringLiteral("Read the colors"),
             QStringLiteral(
                 "Sketch geometry changes color to tell you its status. White (or black, depending "
                 "on "
                 "your theme) means a line or curve isn't fully constrained yet -- it can still "
                 "move. "
                 "Green means fully constrained -- its position is locked down completely. Red or "
                 "orange means a constraint conflicts with another or is redundant; the Report "
                 "view "
                 "will tell you which ones so you can remove one. Aim for a fully constrained, "
                 "green "
                 "sketch before using it in a feature."
             ),
             {},
             false,
             true,
             QStringLiteral("SketcherWorkbench")},

            {nullptr,
             kNewSketchId,
             QStringLiteral("New Sketch"),
             QStringLiteral("Start a sketch"),
             QStringLiteral(
                 "<img src=\":/icons/general/Sketcher_NewSketch.svg\" width=\"16\" height=\"16\"> "
                 "Every Part Design feature starts from a sketch. The New Sketch button is "
                 "highlighted. "
                 "Click Next and this tour will create a sketch on the XY plane for you."
             ),
             {QStringLiteral("Sketcher_NewSketch")},
             true,
             true,
             QStringLiteral("SketcherWorkbench"),
             TourStopExitAction::CreateSketchOnXYPlane},

            {nullptr,
             kSelectPlaneId,
             QStringLiteral("Select a Plane"),
             QStringLiteral("Pick a plane"),
             QStringLiteral(
                 "A sketch needs a flat plane to sit on. The tour attaches this one to the "
                 "<img src=\":/icons/Std_Plane.svg\" width=\"16\" height=\"16\"> XY plane, whose "
                 "normal points along Z, and starts editing it."
             ),
             {},
             false,
             true,
             QStringLiteral("SketcherWorkbench")},

            {nullptr,
             kExternalProjectionId,
             QStringLiteral("External Projection"),
             QStringLiteral("External geometry"),
             QStringLiteral(
                 "External geometry copies the outline of selected edges into the active sketch as "
                 "a "
                 "read-only reference, so you can constrain new geometry to it. Look for its "
                 "<img src=\":/icons/geometry/Sketcher_Projection.svg\" width=\"16\" "
                 "height=\"16\"> icon, "
                 "highlighted here in the Sketcher toolbar. It helps line up a sketch with "
                 "existing "
                 "geometry, but it is not a solid and does not create a second document."
             ),
             {QStringLiteral("Sketcher_CompExternal")},
             true,
             true,
             QStringLiteral("SketcherWorkbench"),
             TourStopExitAction::LeaveSketchEditMode},

            {workbenchSelector(),
             QStringLiteral("Part Design Workbench"),
             QStringLiteral("Part Design Workbench"),
             QStringLiteral("Part Design Workbench"),
             QStringLiteral(
                 "<img src=\":/icons/PartDesign_Body.svg\" width=\"16\" height=\"16\"> Part "
                 "Design: uses a Body; stores an ordered, editable history of features, for "
                 "example a sketch, then a <img src=\":/icons/PartDesign_Pad.svg\" width=\"16\" "
                 "height=\"16\"> Pad, then a <img src=\":/icons/PartDesign_Chamfer.svg\" "
                 "width=\"16\" height=\"16\"> Chamfer; is processed as a single solid built up "
                 "step by step, which can be edited or rolled back at any point."
             ),
             {},
             false,
             false,
             QStringLiteral("PartDesignWorkbench")},

            {partDesignToolBar(),
             QStringLiteral("Additive / Subtractive"),
             QStringLiteral("Additive / Subtractive"),
             QStringLiteral("Additive and subtractive"),
             QStringLiteral(
                 "Additive and subtractive features belong to the Part Design workbench -- "
                 "highlighted "
                 "here is the strip of feature commands where each one either adds material (like "
                 "<img src=\":/icons/PartDesign_Pad.svg\" width=\"16\" height=\"16\"> Pad) or "
                 "removes it (like "
                 "<img src=\":/icons/PartDesign_Pocket.svg\" width=\"16\" height=\"16\"> Pocket). "
                 "The strip also includes "
                 "operations such as <img src=\":/icons/PartDesign_Revolution.svg\" width=\"16\" "
                 "height=\"16\"> Revolution. Don't rely on color alone: read the command name or "
                 "its "
                 "tooltip. Pick a sketch, choose the operation, "
                 "then set its length or depth in the Tasks panel."
             ),
             additiveSubtractiveCommands,
             true,
             true,
             QStringLiteral("PartDesignWorkbench")},

            {nullptr,
             kSubShapeBinderId,
             QStringLiteral("SubShape Binder"),
             QStringLiteral("Referencing without duplicating"),
             QStringLiteral(
                 "The green <img src=\":/icons/PartDesign_SubShapeBinder.svg\" width=\"16\" "
                 "height=\"16\"> icon is the Sub-Shape Binder: a live, updating reference to "
                 "selected geometry from another object, such as a solid, face, edge, or vertex. "
                 "The Sub-Shape Binder appears inside the current Body, so you can "
                 "build against that geometry without duplicating it -- a reference, not a copy, "
                 "and not a separate document. The old, blue <img "
                 "src=\":/icons/PartDesign_ShapeBinder.svg\" width=\"16\" height=\"16\"> Shape "
                 "Binder "
                 "references an object's whole shape."
             ),
             {QStringLiteral("PartDesign_SubShapeBinder")},
             true,
             true,
             QStringLiteral("PartDesignWorkbench")},

            {workbenchSelector(),
             QStringLiteral("Part Workbench"),
             QStringLiteral("Part Workbench"),
             QStringLiteral("Part Workbench"),
             QStringLiteral(
                 "<img src=\":/icons/Part_3D_object.svg\" width=\"16\" height=\"16\"> Part: works "
                 "with solids you've already created -- primitives like a cube or cylinder, "
                 "or shapes brought in from other workbenches -- optionally grouped in a Part "
                 "container "
                 "for organization; stores shape objects directly, not a feature history; is "
                 "processed "
                 "immediately, so a boolean tool fuses, cuts, or intersects the geometry you "
                 "select, "
                 "such as a cube and a sphere. A <img src=\":/icons/booleans/Part_Booleans.svg\" "
                 "width=\"16\" height=\"16\"> boolean operation combines selected shapes. "
                 "<img src=\":/icons/tools/Part_Extrude.svg\" width=\"16\" height=\"16\"> Extrude "
                 "and "
                 "<img src=\":/icons/tools/Part_Revolve.svg\" width=\"16\" height=\"16\"> Revolve "
                 "turn profiles into solids."
             ),
             {},
             false,
             false,
             QStringLiteral("PartWorkbench")},

            {findDock({QStringLiteral("Tree View"), QStringLiteral("Model")}),
             QStringLiteral("Layout"),
             QStringLiteral("Layout"),
             QStringLiteral("Make it yours"),
             QStringLiteral(
                 "Toolbars, dock panels, and tabs can all be rearranged. Drag a panel by its title "
                 "bar "
                 "to dock it against an edge or beside another panel. Each panel's title bar also "
                 "has a "
                 "small float button that pops it into its own window, and a close button that "
                 "hides it "
                 "-- bring a hidden panel back later from the View menu. Keep the Model and Tasks "
                 "panels "
                 "wherever your eyes naturally rest."
             ),
             {},
             false},

            {firstToolBar(),
             QStringLiteral("Recompute"),
             QStringLiteral("Recompute"),
             QStringLiteral("Stay up to date"),
             QStringLiteral(
                 "FreeCAD doesn't always update automatically. After certain edits, the document "
                 "needs "
                 "recomputing before its geometry catches up -- look for the recompute icon (a "
                 "small "
                 "refresh arrow) lighting up in the toolbar. Click it, or press the shortcut, to "
                 "bring "
                 "everything current before you keep building."
             ),
             {QStringLiteral("Std_Refresh")},
             true,
             false},

            {_mainWindow->findChild<QDockWidget*>(QStringLiteral("Std_ReportView")),
             QStringLiteral("Report View"),
             QStringLiteral("Report View"),
             QStringLiteral("Check the Report view"),
             QStringLiteral(
                 "Warnings and errors often show up only in the Report view, a panel that's hidden "
                 "by "
                 "default. Open it from the View menu, under Panels, then Report view. It's the "
                 "first "
                 "place to look when a recompute fails or a command doesn't do what you expected."
             ),
             {},
             true,
             false},

            {nullptr,
             kReadMoreId,
             QStringLiteral("Read More"),
             QStringLiteral("Keep learning"),
             QStringLiteral(
                 "Continue with FreeCAD's Getting Started guide for hands-on examples and deeper "
                 "explanations. Use the Read more button below to open it in your browser."
             ),
             {},
             false}
        };

        // Drop only stops that need a spotlight but have nothing to point it at (for example a dock
        // or toolbar that couldn't be found in this window layout). Purely explanatory stops, which
        // set highlight to false on purpose, are always kept.
        candidates.erase(
            std::remove_if(
                candidates.begin(),
                candidates.end(),
                [](const TourStop& stop) {
                    return stop.highlight && stop.widget == nullptr && stop.commandNames.isEmpty();
                }
            ),
            candidates.end()
        );
        return candidates;
    }

    QToolBar* firstToolBar() const
    {
        const auto toolBars = _mainWindow->findChildren<QToolBar*>();
        return toolBars.empty() ? nullptr : toolBars.front();
    }

    QToolBar* partDesignToolBar() const
    {
        for (auto toolbar : _mainWindow->findChildren<QToolBar*>()) {
            for (auto action : toolbar->actions()) {
                if (action->objectName() == QStringLiteral("PartDesign_Pad")
                    || action->objectName() == QStringLiteral("PartDesign_Pocket")) {
                    return toolbar;
                }
            }
        }
        return firstToolBar();
    }

    QWidget* workbenchSelector() const
    {
        const auto selectors = _mainWindow->findChildren<Gui::WorkbenchComboBox*>();
        if (!selectors.empty()) {
            return selectors.front();
        }
        return _mainWindow->findChild<QWidget*>(QStringLiteral("WbTabBar"));
    }

    // Select the active Body's XY plane before running New Sketch so the tour opens a real sketch
    // in edit mode and the following External Geometry stop has a toolbar command to highlight.
    //
    // This goes through the Python interpreter rather than PartDesign::Body's C++ API on purpose:
    // the object model can be driven dynamically from Gui, without StartGui taking on a build
    // dependency on PartDesignApp (and, transitively, on OpenCASCADE) for this one tour step.
    void createSketchOnXYPlane() const
    {
        try {
            Base::Interpreter().runStringObject(
                "exec(\"import FreeCAD as App, FreeCADGui as Gui\\n"
                "_body = next((obj for obj in App.ActiveDocument.Objects if obj.TypeId == "
                "'PartDesign::Body'), None) if App.ActiveDocument else None\\n"
                "if _body is not None and _body.Origin is not None:\\n"
                "    Gui.Selection.clearSelection()\\n"
                "    Gui.Selection.addSelection(_body.Origin.OriginFeatures[3])\\n"
                "    Gui.runCommand('PartDesign_NewSketch')\")"
            );
        }
        catch (Base::PyException& error) {
            error.reportException();
        }
    }

    QWidget* commandWidget(const QString& commandName) const
    {
        for (auto toolbar : _mainWindow->findChildren<QToolBar*>()) {
            for (auto action : toolbar->actions()) {
                if (action->objectName() == commandName) {
                    return toolbar->widgetForAction(action);
                }
            }
        }
        return nullptr;
    }

    // Union of the on-screen rectangles of every command in commandNames that could be found. Lets
    // a single tour stop highlight a whole strip of related toolbar buttons instead of just one.
    QRect commandStripRect(const QStringList& commandNames) const
    {
        QRect strip;
        for (const auto& commandName : commandNames) {
            auto widget = commandWidget(commandName);
            if (widget == nullptr) {
                continue;
            }
            const auto topLeft = widget->mapTo(_mainWindow, QPoint(0, 0));
            const QRect widgetRect(topLeft, widget->size());
            strip = strip.isNull() ? widgetRect : strip.united(widgetRect);
        }
        return strip;
    }

    QDockWidget* findDock(const QStringList& titles) const
    {
        for (auto dock : _mainWindow->findChildren<QDockWidget*>()) {
            if (titles.contains(dock->windowTitle())) {
                return dock;
            }
        }
        return nullptr;
    }

    void showStop(int index)
    {
        if (_stops.isEmpty() || index >= _stops.size()) {
            closeTour();
            return;
        }
        _index = index;
        const auto& stop = _stops.at(index);
        bool workbenchChanged = false;
        if (!stop.workbenchName.isEmpty()) {
            Gui::Application::Instance->activateWorkbench(stop.workbenchName.toUtf8().constData());
            workbenchChanged = true;
        }
        if (!stop.highlight) {
            _targetRect = QRect();
        }
        else {
            _targetRect = stop.commandNames.isEmpty() ? QRect() : commandStripRect(stop.commandNames);
            if (_targetRect.isNull() && stop.widget != nullptr) {
                const auto topLeft = stop.widget->mapTo(_mainWindow, QPoint(0, 0));
                _targetRect = QRect(topLeft, stop.widget->size());
            }
        }
        _headline->setText(stop.headline);
        _body->setText(stop.description);
        _readMoreButton->setVisible(stop.id == kReadMoreId);
        _chapters->blockSignals(true);
        _chapters->setCurrentRow(index);
        _chapters->blockSignals(false);
        _nextButton->setText(
            index == _stops.size() - 1 ? QStringLiteral("Done") : QStringLiteral("Next")
        );
        _bubble->adjustSize();
        const auto margin = 24;
        const auto chapterHeight = _chapters->sizeHintForRow(0) * _chapters->count() + 10;
        _chapters->setGeometry(margin, std::max((height() - chapterHeight) / 2, margin), 170, chapterHeight);
        auto bubbleX = _targetRect.isNull() ? margin + 194 : _targetRect.right() + 16;
        auto bubbleY = _targetRect.isNull() ? (height() - _bubble->height()) / 2 : _targetRect.top();
        if (bubbleX + _bubble->width() > width() - margin) {
            bubbleX = _targetRect.left() - _bubble->width() - 16;
        }
        if (bubbleX < margin + 194) {
            bubbleX = margin + 194;
            bubbleY = _targetRect.bottom() + 16;
        }
        if (bubbleX + _bubble->width() > width() - margin) {
            bubbleX = std::max(width() - _bubble->width() - margin, margin);
        }
        if (bubbleY + _bubble->height() > height() - margin) {
            bubbleY = std::max(height() - _bubble->height() - margin, margin);
        }
        _bubble->move(bubbleX, std::max(bubbleY, margin));
        raise();
        update();

        if (workbenchChanged) {
            QTimer::singleShot(0, this, [this, index]() {
                if (_index != index) {
                    return;
                }
                const auto& currentStop = _stops.at(index);
                _targetRect = currentStop.commandNames.isEmpty()
                    ? QRect()
                    : commandStripRect(currentStop.commandNames);
                if (_targetRect.isNull() && currentStop.widget != nullptr) {
                    const auto topLeft = currentStop.widget->mapTo(_mainWindow, QPoint(0, 0));
                    _targetRect = QRect(topLeft, currentStop.widget->size());
                }
                update();
            });
        }
    }

    void advance()
    {
        const auto& stop = _stops.at(_index);
        if (stop.onExit.testFlag(TourStopExitAction::CreateSketchOnXYPlane)) {
            createSketchOnXYPlane();
        }
        if (stop.onExit.testFlag(TourStopExitAction::LeaveSketchEditMode)) {
            // Leave sketch edit mode (opened at the previous stop) before switching workbenches.
            if (auto doc = Gui::Application::Instance->activeDocument()) {
                doc->resetEdit();
            }
        }

        const auto nextIndex = _index + 1;
        if (nextIndex < _stops.size()) {
            const auto& nextStop = _stops.at(nextIndex);
            if (!nextStop.workbenchName.isEmpty()) {
                Gui::Application::Instance->activateWorkbench(
                    nextStop.workbenchName.toUtf8().constData()
                );
            }
        }

        showStop(nextIndex);
    }

    void closeTour()
    {
        deleteLater();
    }

    QMainWindow* _mainWindow;
    QFrame* _bubble = nullptr;
    QLabel* _headline = nullptr;
    QLabel* _body = nullptr;
    QPushButton* _nextButton = nullptr;
    QPushButton* _readMoreButton = nullptr;
    QListWidget* _chapters = nullptr;
    QList<TourStop> _stops;
    QRect _targetRect;
    int _index = 0;
};

}  // namespace

void Tour::start(QMainWindow* mainWindow)
{
    if (mainWindow != nullptr) {
        Gui::Application::Instance->commandManager().runCommandByName("Std_New");
        Gui::Application::Instance->activateWorkbench("PartDesignWorkbench");
        Gui::Application::Instance->commandManager().runCommandByName("PartDesign_Body");
        auto tourOverlay = new TourOverlay(mainWindow);
        tourOverlay->show();
    }
}

}  // namespace StartGui
