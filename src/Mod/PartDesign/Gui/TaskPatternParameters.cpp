// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/

#include <QMessageBox>
#include <QTimer>
#include <QVBoxLayout>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Origin.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Command.h>
#include <Gui/ViewProviderCoordinateSystem.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/DatumLine.h>
#include <Mod/PartDesign/App/DatumPlane.h>
#include <Mod/PartDesign/App/FeatureLinearPattern.h>
#include <Mod/PartDesign/App/FeaturePolarPattern.h>
#include <Mod/Part/Gui/PatternParametersWidget.h>

#include "ui_TaskPatternParameters.h"
#include "TaskPatternParameters.h"
#include "ReferenceSelection.h"
#include "TaskMultiTransformParameters.h"


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskPatternParameters */

TaskPatternParameters::TaskPatternParameters(ViewProviderTransformed* TransformedView, QWidget* parent)
    : TaskTransformedParameters(TransformedView, parent)
    , ui(new Ui_TaskPatternParameters)
{
    setupUI();
}

TaskPatternParameters::TaskPatternParameters(
    TaskMultiTransformParameters* parentTask,
    QWidget* parameterWidget
)
    : TaskTransformedParameters(parentTask)
    , ui(new Ui::TaskPatternParameters)
{
    setupParameterUI(parameterWidget);
}

void TaskPatternParameters::setupParameterUI(QWidget* widget)
{
    ui->setupUi(widget);  // Setup the Task's own minimal UI (placeholder)
    QMetaObject::connectSlotsByName(this);

    // --- Create and Embed the Parameter Widget ---
    auto pattern = getObject();
    if (!pattern) {
        return;
    }
    PartGui::PatternType type = pattern->isDerivedFrom<PartDesign::LinearPattern>()
        ? PartGui::PatternType::Linear
        : PartGui::PatternType::Polar;

    // Set first direction widget
    parametersWidget = new PartGui::PatternParametersWidget(type, widget);

    auto* placeholderLayout = new QVBoxLayout(ui->parametersWidgetPlaceholder);
    placeholderLayout->setContentsMargins(0, 0, 0, 0);
    placeholderLayout->addWidget(parametersWidget);
    ui->parametersWidgetPlaceholder->setLayout(placeholderLayout);

    auto* sketch = dynamic_cast<Part::Part2DObject*>(getSketchObject());
    this->fillAxisCombo(parametersWidget->dirLinks, sketch);
    connect(
        parametersWidget,
        &PartGui::PatternParametersWidget::requestReferenceSelection,
        this,
        &TaskPatternParameters::onParameterWidgetRequestReferenceSelection
    );
    connect(
        parametersWidget,
        &PartGui::PatternParametersWidget::parametersChanged,
        this,
        &TaskPatternParameters::onParameterWidgetParametersChanged
    );

    // Add second direction widget if necessary
    if (type == PartGui::PatternType::Linear) {
        parametersWidget2 = new PartGui::PatternParametersWidget(type, widget);
        auto* placeholderLayout2 = new QVBoxLayout(ui->parametersWidgetPlaceholder2);
        placeholderLayout2->setContentsMargins(0, 0, 0, 0);
        placeholderLayout2->addWidget(parametersWidget2);
        ui->parametersWidgetPlaceholder2->setLayout(placeholderLayout2);

        this->fillAxisCombo(parametersWidget2->dirLinks, sketch);
        connect(
            parametersWidget2,
            &PartGui::PatternParametersWidget::requestReferenceSelection,
            this,
            &TaskPatternParameters::onParameterWidgetRequestReferenceSelection2
        );
        connect(
            parametersWidget2,
            &PartGui::PatternParametersWidget::parametersChanged,
            this,
            &TaskPatternParameters::onParameterWidgetParametersChanged
        );
        parametersWidget2->setTitle(tr("Direction 2"));
        parametersWidget2->setCheckable(true);
    }

    bindProperties();

    // --- Task Specific Setup ---
    showOriginAxes(true);  // Show origin helper axes

    updateViewTimer = new QTimer(this);
    updateViewTimer->setSingleShot(true);
    updateViewTimer->setInterval(getUpdateViewTimeout());
    connect(updateViewTimer, &QTimer::timeout, this, &TaskPatternParameters::onUpdateViewTimer);
}

void TaskPatternParameters::bindProperties()
{
    auto pattern = getObject();
    if (!pattern) {
        return;
    }

    if (pattern->isDerivedFrom<PartDesign::LinearPattern>()) {
        auto* linear = static_cast<PartDesign::LinearPattern*>(pattern);
        parametersWidget->bindProperties(
            &linear->Direction,
            &linear->Reversed,
            &linear->Mode,
            &linear->Length,
            &linear->Offset,
            &linear->SpacingPattern,
            &linear->Occurrences,
            linear
        );
        parametersWidget2->bindProperties(
            &linear->Direction2,
            &linear->Reversed2,
            &linear->Mode2,
            &linear->Length2,
            &linear->Offset2,
            &linear->SpacingPattern2,
            &linear->Occurrences2,
            linear
        );
    }
    else if (pattern->isDerivedFrom<PartDesign::PolarPattern>()) {
        auto* polar = static_cast<PartDesign::PolarPattern*>(pattern);
        parametersWidget->bindProperties(
            &polar->Axis,
            &polar->Reversed,
            &polar->Mode,
            &polar->Angle,
            &polar->Offset,
            &polar->SpacingPattern,
            &polar->Occurrences,
            polar
        );
    }
    else {
        Base::Console().warning(
            "PatternParametersWidget property binding failed. Something is wrong please report.\n"
        );
    }
}

void TaskPatternParameters::retranslateParameterUI(QWidget* widget)
{
    ui->retranslateUi(widget);
}

void TaskPatternParameters::updateUI()
{
    if (parametersWidget) {
        parametersWidget->updateUI();
    }
    if (parametersWidget2) {
        parametersWidget2->updateUI();
    }
}

// --- Task-Specific Logic ---

void TaskPatternParameters::showOriginAxes(bool show)
{
    PartDesign::Body* body = PartDesign::Body::findBodyOf(getObject());
    if (body) {
        try {
            App::Origin* origin = body->getOrigin();
            auto vpOrigin = static_cast<ViewProviderCoordinateSystem*>(
                Gui::Application::Instance->getViewProvider(origin)
            );
            if (show) {
                vpOrigin->setTemporaryVisibility(Gui::DatumElement::Axes);
            }
            else {
                vpOrigin->resetTemporaryVisibility();
            }
        }
        catch (const Base::Exception& ex) {
            Base::Console().error("TaskPatternParameters: Error accessing origin axes: %s\n", ex.what());
        }
    }
}

void TaskPatternParameters::enterReferenceSelectionMode()
{
    if (selectionMode == SelectionMode::Reference) {
        return;
    }

    hideObject();  // Hide the pattern feature itself
    showBase();    // Show the base features/body
    Gui::Selection().clearSelection();
    // Add selection gate (allow edges, faces, potentially datums)
    addReferenceSelectionGate(AllowSelection::EDGE | AllowSelection::FACE | AllowSelection::PLANAR);
    Gui::getMainWindow()->showMessage(
        tr("Select a direction reference (edge, face, datum line)")
    );  // User feedback
}

void TaskPatternParameters::exitReferenceSelectionMode()
{
    exitSelectionMode();

    hideBase();
    Gui::getMainWindow()->showMessage(QString());
    activeDirectionWidget = nullptr;
}


// --- SLOTS ---

void TaskPatternParameters::onUpdateViewTimer()
{
    // Recompute is triggered when parameters change and this timer fires
    setupTransaction();  // Group potential property changes
    recomputeFeature();

    updateUI();
}

void TaskPatternParameters::onParameterWidgetRequestReferenceSelection()
{
    // The embedded widget wants to enter reference selection mode
    activeDirectionWidget = parametersWidget;
    enterReferenceSelectionMode();
    selectionMode = SelectionMode::Reference;
}

void TaskPatternParameters::onParameterWidgetRequestReferenceSelection2()
{
    // The embedded widget wants to enter reference selection mode
    activeDirectionWidget = parametersWidget2;
    enterReferenceSelectionMode();
    selectionMode = SelectionMode::Reference;
}

void TaskPatternParameters::onParameterWidgetParametersChanged()
{
    // A parameter in the embedded widget changed, trigger a recompute
    if (blockUpdate) {
        return;  // Avoid loops if change originated from Task update
    }
    kickUpdateViewTimer();  // Debounce recompute
}

void TaskPatternParameters::onUpdateView(bool on)
{
    // This might be less relevant now if recomputes are triggered by parametersChanged
    blockUpdate = !on;
    if (on) {
        kickUpdateViewTimer();
    }
}

void TaskPatternParameters::kickUpdateViewTimer() const
{
    updateViewTimer->start();
}

void TaskPatternParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    // Handle selection ONLY when in reference selection mode
    if (selectionMode == SelectionMode::None || msg.Type != Gui::SelectionChanges::AddSelection) {
        return;
    }

    if (originalSelected(msg)) {
        exitSelectionMode();
        return;
    }

    auto patternObj = getObject();
    if (!patternObj) {
        return;
    }

    std::vector<std::string> directions;
    App::DocumentObject* selObj = nullptr;
    getReferencedSelection(patternObj, msg, selObj, directions);
    if (!selObj) {
        Base::Console().warning(
            tr("Invalid selection. Select an edge, planar face, or datum line.").toStdString().c_str()
        );
        return;
    }

    // Note: ReferenceSelection has already checked the selection for validity
    if (selectionMode == SelectionMode::Reference || selObj->isDerivedFrom<App::Line>()) {
        setupTransaction();

        if (patternObj->isDerivedFrom<PartDesign::LinearPattern>()) {
            auto* linearPattern = static_cast<PartDesign::LinearPattern*>(patternObj);
            if (activeDirectionWidget == parametersWidget) {
                linearPattern->Direction.setValue(selObj, directions);
            }
            else {
                linearPattern->Direction2.setValue(selObj, directions);
            }
        }
        else if (patternObj->isDerivedFrom<PartDesign::PolarPattern>()) {
            auto* polarPattern = static_cast<PartDesign::PolarPattern*>(patternObj);
            polarPattern->Axis.setValue(selObj, directions);
        }
        recomputeFeature();
        updateUI();
    }
    exitReferenceSelectionMode();
}

TaskPatternParameters::~TaskPatternParameters()
{
    showOriginAxes(false);         // Clean up temporary visibility
    exitReferenceSelectionMode();  // Ensure gates are removed etc.
    // ui unique_ptr handles deletion
    // parametersWidget is deleted by Qt parent mechanism if added to layout correctly
}

void TaskPatternParameters::apply()
{
    auto pattern = getObject();
    if (!pattern || !parametersWidget) {
        return;
    }

    std::vector<std::string> dirs;
    App::DocumentObject* obj = nullptr;
    parametersWidget->getAxis(obj, dirs);
    std::string direction = buildLinkSingleSubPythonStr(obj, dirs);

    bool isLinear = pattern->isDerivedFrom<PartDesign::LinearPattern>();
    const char* propName = isLinear ? "Direction = " : "Axis = ";

    FCMD_OBJ_CMD(pattern, propName << direction.c_str());
    FCMD_OBJ_CMD(pattern, "Reversed = " << parametersWidget->getReverse());
    FCMD_OBJ_CMD(pattern, "Mode = " << parametersWidget->getMode());
    parametersWidget->applyQuantitySpinboxes();

    FCMD_OBJ_CMD(pattern, "SpacingPattern = " << parametersWidget->getSpacingPatternsAsString());

    if (parametersWidget2) {
        parametersWidget2->getAxis(obj, dirs);
        direction = buildLinkSingleSubPythonStr(obj, dirs);

        FCMD_OBJ_CMD(pattern, "Direction2 = " << direction.c_str());
        FCMD_OBJ_CMD(pattern, "Reversed2 = " << parametersWidget2->getReverse());
        FCMD_OBJ_CMD(pattern, "Mode2 = " << parametersWidget2->getMode());
        parametersWidget2->applyQuantitySpinboxes();

        FCMD_OBJ_CMD(pattern, "SpacingPattern2 = " << parametersWidget2->getSpacingPatternsAsString());
    }

    // The user may have changed a value and immediately hit 'OK' or Enter.
    // This triggers accept() before the update timer for the 3D view has a
    // chance to fire. If the timer is active, it means a recompute is
    // pending.
    if (updateViewTimer && updateViewTimer->isActive()) {
        updateViewTimer->stop();
        recomputeFeature();
    }
}

//**************************************************************************
// TaskDialog Implementation (Remains largely the same)
//**************************************************************************

TaskDlgLinearPatternParameters::TaskDlgLinearPatternParameters(
    ViewProviderTransformed* LinearPatternView
)
    : TaskDlgTransformedParameters(LinearPatternView)  // Use base class constructor
{
    // Create the specific parameter task panel
    parameter = new TaskPatternParameters(LinearPatternView);
    // Add it to the dialog's content list
    Content.push_back(parameter);
    Content.push_back(preview);
}

#include "moc_TaskPatternParameters.cpp"
