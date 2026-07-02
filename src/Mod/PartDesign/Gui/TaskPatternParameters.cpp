// SPDX-License-Identifier: LGPL-2.1-or-later

/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
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
#include <QVBoxLayout>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Vec.hxx>
#include <gp_Pnt.hxx>
#include <Standard_Failure.hxx>

#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>

#include <algorithm>
#include <list>
#include <optional>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Origin.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Command.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewProviderCoordinateSystem.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/DatumLine.h>
#include <Mod/PartDesign/App/DatumPlane.h>
#include <Mod/PartDesign/App/FeatureCircularPattern.h>
#include <Mod/PartDesign/App/FeatureLinearPattern.h>
#include <Mod/PartDesign/App/FeaturePathPattern.h>
#include <Mod/PartDesign/App/FeaturePointPattern.h>
#include <Mod/PartDesign/App/FeaturePolarPattern.h>
#include <Mod/PartDesign/App/FeatureAddSub.h>
#include <Mod/Part/Gui/PatternInstanceControls.h>
#include <Mod/Part/Gui/PatternParametersWidget.h>
#include <Mod/Part/App/Tools.h>

#include "ui_TaskPatternParameters.h"
#include "TaskPatternParameters.h"
#include "ReferenceSelection.h"
#include "TaskMultiTransformParameters.h"


using namespace PartDesignGui;
using namespace Gui;

namespace
{

std::optional<Base::Vector3d> shapeCenter(const TopoDS_Shape& shape)
{
    if (shape.IsNull()) {
        return std::nullopt;
    }

    Bnd_Box bndBox;
    BRepBndLib::Add(shape, bndBox);
    if (bndBox.IsVoid()) {
        return std::nullopt;
    }

    double xmin = 0.0;
    double ymin = 0.0;
    double zmin = 0.0;
    double xmax = 0.0;
    double ymax = 0.0;
    double zmax = 0.0;
    bndBox.Get(xmin, ymin, zmin, xmax, ymax, zmax);

    return Base::Vector3d((xmin + xmax) / 2.0, (ymin + ymax) / 2.0, (zmin + zmax) / 2.0);
}

Base::Vector3d transformedPoint(const Base::Vector3d& point, const gp_Trsf& transform)
{
    gp_Pnt pnt(point.x, point.y, point.z);
    pnt.Transform(transform);
    return Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z());
}

Base::Vector3d transformedVector(const Base::Vector3d& vector, const gp_Trsf& transform)
{
    gp_Vec vec(vector.x, vector.y, vector.z);
    vec.Transform(transform);
    return Base::Vector3d(vec.X(), vec.Y(), vec.Z());
}

}  // namespace

/* TRANSLATOR PartDesignGui::TaskPatternParameters */

TaskPatternParameters::TaskPatternParameters(ViewProviderTransformed* TransformedView, QWidget* parent)
    : TaskTransformedParameters(TransformedView, parent)
    , ui(new Ui_TaskPatternParameters)
{
    setupUI();
    updatePatternSpacingLabels();
    setupInstanceControls();
}

TaskPatternParameters::TaskPatternParameters(
    TaskMultiTransformParameters* parentTask,
    QWidget* parameterWidget
)
    : TaskTransformedParameters(parentTask)
    , ui(new Ui::TaskPatternParameters)
{
    setupParameterUI(parameterWidget);
    updatePatternSpacingLabels();
    setupInstanceControls();
}

void TaskPatternParameters::setupParameterUI(QWidget* widget)
{
    ui->setupUi(widget);  // Setup the Task's own minimal UI (placeholder)
    QMetaObject::connectSlotsByName(this);

    if (auto* point = dynamic_cast<PartDesign::PointPattern*>(getObject())) {
        ui->parametersWidgetPlaceholder2->hide();
        setupPointPatternParameterUI(widget, ui->parametersWidgetPlaceholder, this, &point->PointObject);
    }
    else if (auto* path = dynamic_cast<PartDesign::PathPattern*>(getObject())) {
        ui->parametersWidgetPlaceholder2->hide();
        setupPathPatternParameterUI(
            widget,
            ui->parametersWidgetPlaceholder,
            this,
            getUpdateViewTimeout(),
            &path->Path,
            &path->Count,
            &path->SpacingMode,
            &path->Spacing,
            &path->StartOffset,
            &path->EndOffset,
            &path->ReversePath,
            &path->Align
        );
    }
    else if (auto* circular = dynamic_cast<PartDesign::CircularPattern*>(getObject())) {
        ui->parametersWidgetPlaceholder2->hide();
        setupCircularPatternParameterUI(
            widget,
            ui->parametersWidgetPlaceholder,
            this,
            getUpdateViewTimeout(),
            &circular->Axis,
            &circular->RadialDistance,
            &circular->TangentialDistance,
            &circular->NumberCircles,
            &circular->Symmetry
        );
    }
    else {
        setupPatternParameterUI(
            widget,
            ui->parametersWidgetPlaceholder,
            ui->parametersWidgetPlaceholder2,
            getTopTransformedView()->getViewer(),
            this,
            getUpdateViewTimeout()
        );
    }

    // --- Task Specific Setup ---
    showOriginAxes(true);  // Show origin helper axes
}

void TaskPatternParameters::retranslateParameterUI(QWidget* widget)
{
    ui->retranslateUi(widget);
}

App::DocumentObject* TaskPatternParameters::getPatternObject() const
{
    return getObject();
}

void TaskPatternParameters::fillDirectionCombo(Gui::ComboLinks& combo, Part::LinearPatternDirection /*direction*/)
{
    auto* sketch = dynamic_cast<Part::Part2DObject*>(getSketchObject());
    this->fillAxisCombo(combo, sketch);
}

void TaskPatternParameters::setupPatternTransaction()
{
    setupTransaction();
}

void TaskPatternParameters::recomputePatternFeature()
{
    recomputeFeature();
    updateInstanceControls();
}

Base::Vector3d TaskPatternParameters::getPatternStartPoint() const
{
    return getStartPoint();
}

Base::Vector3d TaskPatternParameters::getLinearPatternFallbackDirection(
    Part::LinearPatternDirection direction
) const
{
    if (direction == Part::LinearPatternDirection::Second) {
        return Base::Vector3d(0.0, 1.0, 0.0);
    }

    return Base::Vector3d(0.0, 0.0, 1.0);
}

Base::Vector3d TaskPatternParameters::transformLinearPatternDirection(
    const Base::Vector3d& direction
) const
{
    auto* transformed = dynamic_cast<PartDesign::Transformed*>(getObject());
    if (!transformed) {
        return direction;
    }

    return transformedVector(direction, transformed->getLocation().Transformation());
}

Base::Vector3d TaskPatternParameters::getLinearPatternLabelPlaneNormal(Part::LinearPatternDirection) const
{
    return transformLinearPatternDirection(Base::Vector3d(0.0, 0.0, 1.0));
}

void TaskPatternParameters::transformPolarPatternAxis(gp_Ax2& axis) const
{
    auto* transformed = dynamic_cast<PartDesign::Transformed*>(getObject());
    if (transformed) {
        axis.Transform(transformed->getLocation().Transformation());
    }
}

std::string TaskPatternParameters::buildDirectionReferencePythonString(
    const App::DocumentObject* obj,
    const std::vector<std::string>& subs
) const
{
    return buildLinkSingleSubPythonStr(obj, subs);
}

void TaskPatternParameters::setupInstanceControls()
{
    auto* pattern = dynamic_cast<PartDesign::Transformed*>(getObject());
    auto* topPattern = getTopTransformedObject();
    auto* view = getTopTransformedView();
    if (!pattern || !topPattern || pattern != topPattern || !view) {
        instanceControls.reset();
        return;
    }

    auto* viewer = view->getViewer();
    if (!viewer) {
        instanceControls.reset();
        return;
    }

    instanceControls = std::make_unique<PartGui::PatternInstanceControls>(viewer, this);
    connect(
        instanceControls.get(),
        &PartGui::PatternInstanceControls::toggleRequested,
        this,
        [this](int index, bool suppress) { setInstanceSuppressed(index, suppress); }
    );
    updateInstanceControls();
}

void TaskPatternParameters::updateInstanceControls()
{
    if (!instanceControls) {
        return;
    }

    auto* pattern = dynamic_cast<PartDesign::Transformed*>(getObject());
    if (!pattern) {
        instanceControls->clear();
        return;
    }

    auto sourceCenter = shapeCenter(pattern->PreviewShape.getShape().getShape());
    if (!sourceCenter) {
        instanceControls->clear();
        return;
    }

    std::list<gp_Trsf> transformations;
    try {
        transformations = pattern->getTransformations(pattern->getOriginals());
    }
    catch (const Base::Exception&) {
        instanceControls->clear();
        return;
    }
    catch (const Standard_Failure&) {
        instanceControls->clear();
        return;
    }

    std::vector<PartGui::PatternInstanceControls::Instance> instances;
    int index = 0;
    const gp_Trsf patternLocation = pattern->getLocation().Transformation();
    for (const auto& transformation : transformations) {
        if (index > 0) {
            Base::Vector3d center = transformedPoint(*sourceCenter, transformation);
            center = transformedPoint(center, patternLocation);
            instances.push_back({index, center, pattern->isTransformationSuppressed(index)});
        }
        ++index;
    }

    instanceControls->setInstances(instances);
}

void TaskPatternParameters::setInstanceSuppressed(int index, bool suppress)
{
    if (index <= 0) {
        return;
    }

    auto* pattern = dynamic_cast<PartDesign::Transformed*>(getObject());
    if (!pattern) {
        return;
    }

    const long suppressedIndex = static_cast<long>(index);
    std::vector<long> suppressed = pattern->SuppressedIndices.getValues();
    auto it = std::find(suppressed.begin(), suppressed.end(), suppressedIndex);
    const bool alreadySuppressed = it != suppressed.end();
    if (suppress == alreadySuppressed) {
        return;
    }

    setupTransaction();
    if (suppress) {
        suppressed.push_back(suppressedIndex);
    }
    else {
        suppressed.erase(
            std::remove(suppressed.begin(), suppressed.end(), suppressedIndex),
            suppressed.end()
        );
    }

    std::sort(suppressed.begin(), suppressed.end());
    suppressed.erase(std::unique(suppressed.begin(), suppressed.end()), suppressed.end());
    pattern->SuppressedIndices.setValues(suppressed);
    recomputeFeature();
    updateInstanceControls();
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
    if (getObject()->isDerivedFrom<PartDesign::PointPattern>()) {
        // Clicking visible sketch/shape geometry produces a subelement selection, even though
        // PointObject stores the whole object. Accept every shape subelement here and discard the
        // subelement below when assigning the property.
        addReferenceSelectionGate(
            AllowSelection::POINT | AllowSelection::EDGE | AllowSelection::FACE | AllowSelection::WHOLE
        );
        Gui::getMainWindow()->showMessage(tr("Select a sketch or shape containing the pattern points"));
    }
    else if (getObject()->isDerivedFrom<PartDesign::PathPattern>()) {
        // Whole sketches and SubShapeBinders supply all their edges. A single selected edge is
        // also supported until a proper multi-reference selection widget is available.
        addReferenceSelectionGate(AllowSelection::EDGE | AllowSelection::FACE | AllowSelection::WHOLE);
        Gui::getMainWindow()->showMessage(tr("Select a sketch, SubShapeBinder, or path edge"));
    }
    else {
        addReferenceSelectionGate(AllowSelection::EDGE | AllowSelection::FACE | AllowSelection::PLANAR);
        Gui::getMainWindow()->showMessage(tr("Select a direction reference (edge, face, datum line)"));
    }
}

void TaskPatternParameters::exitReferenceSelectionMode()
{
    exitSelectionMode();

    hideBase();
    Gui::getMainWindow()->showMessage(QString());
    clearActiveDirectionWidget();
}


// --- SLOTS ---

void TaskPatternParameters::onReferenceSelectionRequested()
{
    // The embedded widget wants to enter reference selection mode
    enterReferenceSelectionMode();
    selectionMode = SelectionMode::Reference;
}

void TaskPatternParameters::onPatternParametersChanged()
{
    // A parameter in the embedded widget changed, trigger a recompute
    if (blockUpdate) {
        return;  // Avoid loops if change originated from Task update
    }
    PartGui::TaskPatternParameters::kickUpdateViewTimer();  // Debounce recompute
}

void TaskPatternParameters::onUpdateView(bool on)
{
    // This might be less relevant now if recomputes are triggered by parametersChanged
    blockUpdate = !on;
    if (on) {
        PartGui::TaskPatternParameters::kickUpdateViewTimer();
        updateInstanceControls();
    }
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
        const QString warning = patternObj->isDerivedFrom<PartDesign::PointPattern>()
            ? tr("Invalid selection. Select a sketch or shape containing points.")
            : (patternObj->isDerivedFrom<PartDesign::PathPattern>()
                   ? tr("Invalid selection. Select a sketch, SubShapeBinder, or path edge.")
                   : tr("Invalid selection. Select an edge, planar face, or datum line."));
        Base::Console().warning("%s\n", warning.toUtf8().constData());
        return;
    }

    // Note: ReferenceSelection has already checked the selection for validity
    if (selectionMode == SelectionMode::Reference || selObj->isDerivedFrom<App::Line>()) {
        setupTransaction();

        if (patternObj->isDerivedFrom<PartDesign::LinearPattern>()) {
            auto* linearPattern = static_cast<PartDesign::LinearPattern*>(patternObj);
            if (getActiveDirectionWidget() == getPrimaryParametersWidget()) {
                linearPattern->Direction.setValue(selObj, directions);
            }
            else {
                linearPattern->Direction2.setValue(selObj, directions);
            }
        }
        else if (patternObj->isDerivedFrom<PartDesign::CircularPattern>()) {
            auto* circularPattern = static_cast<PartDesign::CircularPattern*>(patternObj);
            circularPattern->Axis.setValue(selObj, directions);
        }
        else if (patternObj->isDerivedFrom<PartDesign::PathPattern>()) {
            auto* pathPattern = static_cast<PartDesign::PathPattern*>(patternObj);
            pathPattern->Path.setValue(selObj, directions);
        }
        else if (patternObj->isDerivedFrom<PartDesign::PointPattern>()) {
            auto* pointPattern = static_cast<PartDesign::PointPattern*>(patternObj);
            pointPattern->PointObject.setValue(selObj);
        }
        else if (patternObj->isDerivedFrom<PartDesign::PolarPattern>()) {
            auto* polarPattern = static_cast<PartDesign::PolarPattern*>(patternObj);
            polarPattern->Axis.setValue(selObj, directions);
        }
        recomputePatternFeature();
        updatePatternParameterUI();
    }
    exitReferenceSelectionMode();
}

TaskPatternParameters::~TaskPatternParameters()
{
    instanceControls.reset();
    showOriginAxes(false);         // Clean up temporary visibility
    exitReferenceSelectionMode();  // Ensure gates are removed etc.
    // ui unique_ptr handles deletion
    // parametersWidget is deleted by Qt parent mechanism if added to layout correctly
}

void TaskPatternParameters::apply()
{
    auto pattern = getObject();
    if (!pattern
        || (!getPrimaryParametersWidget() && !getCircularParametersWidget()
            && !getPathParametersWidget() && !getPointParametersWidget())) {
        return;
    }

    applyPatternParameters(pattern);

    // The user may have changed a value and immediately hit 'OK' or Enter.
    // This triggers accept() before the update timer for the 3D view has a
    // chance to fire. If the timer is active, it means a recompute is
    // pending.
    consumePendingUpdate();
    updateInstanceControls();
}

Base::Vector3d TaskPatternParameters::getStartPoint() const
{
    Base::Vector3d startPoint(0, 0, 0);

    auto* pattern = dynamic_cast<PartDesign::Transformed*>(getObject());
    if (!pattern) {
        return startPoint;
    }

    std::vector<App::DocumentObject*> originals = pattern->getOriginals();
    if (!originals.empty()) {
        BRep_Builder builder;
        TopoDS_Compound compoundShape;
        builder.MakeCompound(compoundShape);

        // 2. Collect the "delta" shapes from each original feature.
        for (App::DocumentObject* obj : originals) {
            // We are only interested in additive/subtractive features.
            if (auto* addSubFeature = dynamic_cast<PartDesign::FeatureAddSub*>(obj)) {
                const Part::TopoShape& deltaShape = addSubFeature->AddSubShape.getShape();
                if (!deltaShape.getShape().IsNull()) {
                    TopoDS_Shape shape = deltaShape.getShape();
                    shape.Move(addSubFeature->getLocation());
                    builder.Add(compoundShape, shape);
                }
            }
        }

        // 3. If we collected any shapes, calculate the center of their combined bounding box.
        if (!compoundShape.IsNull()) {
            try {
                Bnd_Box bndBox;
                BRepBndLib::Add(compoundShape, bndBox);
                if (!bndBox.IsVoid()) {
                    double xmin, ymin, zmin, xmax, ymax, zmax;
                    bndBox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
                    startPoint.x = (xmin + xmax) / 2.0;
                    startPoint.y = (ymin + ymax) / 2.0;
                    startPoint.z = (zmin + zmax) / 2.0;
                }
            }
            catch (const Base::Exception& e) {
                Base::Console().warning(
                    "Could not calculate center of patterned features: %s\n",
                    e.what()
                );
                // startPoint remains (0,0,0) as a fallback.
            }
        }
    }
    return startPoint;
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
