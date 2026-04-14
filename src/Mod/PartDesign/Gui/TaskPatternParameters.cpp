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
#include <QTimer>
#include <QVBoxLayout>
#include <cmath>
#include <cstdlib>

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <GeomAbs_CurveType.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Dir.hxx>
#include <Standard_Failure.hxx>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Datums.h>
#include <App/Origin.h>
#include <Base/Axis.h>
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
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/Part2DObject.h>
#include <Mod/Part/App/TopoShape.h>

#include "ui_TaskPatternParameters.h"
#include "TaskPatternParameters.h"
#include "ReferenceSelection.h"
#include "TaskMultiTransformParameters.h"


using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskPatternParameters */

namespace
{
bool isEmptySub(const std::vector<std::string>& sub)
{
    return sub.empty() || (sub.size() == 1 && sub.front().empty());
}

bool sameLink(
    App::DocumentObject* obj1,
    const std::vector<std::string>& sub1,
    App::DocumentObject* obj2,
    const std::vector<std::string>& sub2
)
{
    if (obj1 != obj2) {
        return false;
    }
    if (sub1 == sub2) {
        return true;
    }
    return isEmptySub(sub1) && isEmptySub(sub2);
}

bool isSketchDefaultHVAxisLink(App::DocumentObject* obj, const std::vector<std::string>& sub)
{
    if (!obj || !obj->isDerivedFrom<Part::Part2DObject>() || sub.size() != 1) {
        return false;
    }
    return sub.front() == "H_Axis" || sub.front() == "V_Axis";
}

bool isBodyBaseAxisLink(
    PartDesign::LinearPattern* pattern,
    App::DocumentObject* obj,
    const std::vector<std::string>& sub
)
{
    if (!pattern || !obj || !isEmptySub(sub)) {
        return false;
    }

    auto* body = PartDesign::Body::findBodyOf(pattern);
    if (!body) {
        return false;
    }

    try {
        App::Origin* origin = body->getOrigin();
        return obj == origin->getX() || obj == origin->getY() || obj == origin->getZ();
    }
    catch (const Base::Exception&) {
        return false;
    }
}

bool tryGetDirectionFromLink(const App::PropertyLinkSub& dirLink, gp_Dir& outDirection)
{
    try {
        App::DocumentObject* refObject = dirLink.getValue();
        if (!refObject) {
            return false;
        }

        std::vector<std::string> subStrings = dirLink.getSubValues();

        if (auto* refSketch = dynamic_cast<Part::Part2DObject*>(refObject)) {
            if (subStrings.size() != 1) {
                return false;
            }

            Base::Axis axis;
            if (subStrings.front() == "H_Axis") {
                axis = refSketch->getAxis(Part::Part2DObject::H_Axis);
                axis *= refSketch->Placement.getValue();
            }
            else if (subStrings.front() == "V_Axis") {
                axis = refSketch->getAxis(Part::Part2DObject::V_Axis);
                axis *= refSketch->Placement.getValue();
            }
            else if (subStrings.front() == "N_Axis") {
                axis = refSketch->getAxis(Part::Part2DObject::N_Axis);
                axis *= refSketch->Placement.getValue();
            }
            else if (subStrings.front().compare(0, 4, "Axis") == 0) {
                int axId = std::atoi(subStrings.front().substr(4, 4000).c_str());
                if (axId < 0 || axId >= refSketch->getAxisCount()) {
                    return false;
                }
                axis = refSketch->getAxis(axId);
                axis *= refSketch->Placement.getValue();
            }
            else if (subStrings.front().compare(0, 4, "Edge") == 0) {
                Part::TopoShape refShape = refSketch->Shape.getShape();
                TopoDS_Shape ref = refShape.getSubShape(subStrings.front().c_str());
                TopoDS_Edge refEdge = TopoDS::Edge(ref);
                if (refEdge.IsNull()) {
                    return false;
                }

                BRepAdaptor_Curve adapt(refEdge);
                if (adapt.GetType() != GeomAbs_Line) {
                    return false;
                }

                gp_Dir d = adapt.Line().Direction();
                axis.setDirection(Base::Vector3d(d.X(), d.Y(), d.Z()));
            }
            else {
                return false;
            }

            auto d = axis.getDirection();
            outDirection = gp_Dir(d.x, d.y, d.z);
            return true;
        }

        if (auto* plane = dynamic_cast<PartDesign::Plane*>(refObject)) {
            Base::Vector3d d = plane->getNormal();
            outDirection = gp_Dir(d.x, d.y, d.z);
            return true;
        }
        if (auto* line = dynamic_cast<PartDesign::Line*>(refObject)) {
            Base::Vector3d d = line->getDirection();
            outDirection = gp_Dir(d.x, d.y, d.z);
            return true;
        }
        if (auto* plane = dynamic_cast<App::Plane*>(refObject)) {
            Base::Vector3d d = plane->getDirection();
            outDirection = gp_Dir(d.x, d.y, d.z);
            return true;
        }
        if (auto* line = dynamic_cast<App::Line*>(refObject)) {
            Base::Vector3d d = line->getDirection();
            outDirection = gp_Dir(d.x, d.y, d.z);
            return true;
        }

        auto* refFeature = dynamic_cast<Part::Feature*>(refObject);
        if (!refFeature || subStrings.empty() || subStrings.front().empty()) {
            return false;
        }

        Part::TopoShape refShape = refFeature->Shape.getShape();
        TopoDS_Shape ref = refShape.getSubShape(subStrings.front().c_str());

        if (ref.ShapeType() == TopAbs_FACE) {
            TopoDS_Face refFace = TopoDS::Face(ref);
            if (refFace.IsNull()) {
                return false;
            }
            BRepAdaptor_Surface adapt(refFace);
            if (adapt.GetType() != GeomAbs_Plane) {
                return false;
            }
            outDirection = adapt.Plane().Axis().Direction();
            return true;
        }

        if (ref.ShapeType() == TopAbs_EDGE) {
            TopoDS_Edge refEdge = TopoDS::Edge(ref);
            if (refEdge.IsNull()) {
                return false;
            }
            BRepAdaptor_Curve adapt(refEdge);
            if (adapt.GetType() != GeomAbs_Line) {
                return false;
            }
            outDirection = adapt.Line().Direction();
            return true;
        }

        return false;
    }
    catch (const Base::Exception&) {
        return false;
    }
    catch (const Standard_Failure&) {
        return false;
    }
    catch (...) {
        return false;
    }
}
}  // namespace

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
    if (parametersWidget2) {
        connect(
            parametersWidget2,
            &PartGui::PatternParametersWidget::enabledChanged,
            this,
            &TaskPatternParameters::onDirection2EnabledChanged
        );
    }

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

void TaskPatternParameters::onDirection2EnabledChanged(bool enabled)
{
    if (!enabled) {
        return;
    }

    setDefaultPerpendicularDirection2();
}

void TaskPatternParameters::setDefaultPerpendicularDirection2()
{
    auto* patternObj = getObject();
    auto* linearPattern = patternObj ? dynamic_cast<PartDesign::LinearPattern*>(patternObj) : nullptr;
    if (!linearPattern || !parametersWidget || !parametersWidget2) {
        return;
    }

    App::DocumentObject* dir1Obj = nullptr;
    std::vector<std::string> dir1Sub;
    parametersWidget->getAxis(dir1Obj, dir1Sub);
    if (!dir1Obj) {
        return;
    }

    App::DocumentObject* dir2Obj = nullptr;
    std::vector<std::string> dir2Sub;
    parametersWidget2->getAxis(dir2Obj, dir2Sub);

    App::DocumentObject* candidateObj = nullptr;
    std::vector<std::string> candidateSub;
    gp_Dir direction1;
    bool hasDirection1 = tryGetDirectionFromLink(linearPattern->Direction, direction1);

    // Sketch axis defaults: H <-> V.
    if (dir1Sub.size() == 1) {
        if (dir1Sub.front() == "H_Axis") {
            candidateObj = dir1Obj;
            candidateSub = {"V_Axis"};
        }
        else if (dir1Sub.front() == "V_Axis") {
            candidateObj = dir1Obj;
            candidateSub = {"H_Axis"};
        }
    }

    // Origin axis defaults: X <-> Y (for common 2D grid usage).
    if (!candidateObj) {
        auto* body = PartDesign::Body::findBodyOf(linearPattern);
        if (body) {
            try {
                App::Origin* origin = body->getOrigin();
                if (dir1Obj == origin->getX()) {
                    candidateObj = origin->getY();
                    candidateSub = {""};
                }
                else if (dir1Obj == origin->getY()) {
                    candidateObj = origin->getX();
                    candidateSub = {""};
                }
            }
            catch (const Base::Exception&) {
            }
        }
    }

    // Generic fallback for custom references:
    // pick the available direction option that is most orthogonal to Direction 1.
    if (!candidateObj && hasDirection1) {
        double bestAbsDot = 2.0;
        App::DocumentObject* bestObj = nullptr;
        std::vector<std::string> bestSub;

        for (int i = 0; i < parametersWidget2->dirLinks.count(); ++i) {
            auto& link = parametersWidget2->dirLinks.getLink(i);
            App::DocumentObject* obj = link.getValue();
            std::vector<std::string> sub = link.getSubValues();

            if (!obj || sameLink(obj, sub, dir1Obj, dir1Sub)) {
                continue;
            }

            gp_Dir testDirection;
            if (!tryGetDirectionFromLink(link, testDirection)) {
                continue;
            }

            double absDot = std::abs(direction1.Dot(testDirection));
            if (absDot < bestAbsDot) {
                bestAbsDot = absDot;
                bestObj = obj;
                bestSub = sub;
            }
        }

        if (bestObj) {
            candidateObj = bestObj;
            candidateSub = bestSub;
        }
    }

    if (!candidateObj) {
        return;
    }

    // Keep an existing explicit second direction chosen by the user,
    // except when Direction 1 uses body base axes and Direction 2 still
    // points to a sketch axis fallback.
    bool direction2Unset = (dir2Obj == nullptr);
    bool direction2SameAsDirection1 = sameLink(dir1Obj, dir1Sub, dir2Obj, dir2Sub);
    bool direction1IsBodyBaseAxis = isBodyBaseAxisLink(linearPattern, dir1Obj, dir1Sub);
    bool direction1IsSketchHV = (dir1Sub.size() == 1)
        && (dir1Sub.front() == "H_Axis" || dir1Sub.front() == "V_Axis");
    bool direction1IsBuiltinAxis = direction1IsSketchHV || direction1IsBodyBaseAxis;
    bool direction2LooksLikeSketchFallback = isSketchDefaultHVAxisLink(dir2Obj, dir2Sub);
    bool direction2IsBodyAxis = isBodyBaseAxisLink(linearPattern, dir2Obj, dir2Sub);
    bool direction2LooksLikeBuiltinFallback = direction2LooksLikeSketchFallback || direction2IsBodyAxis;
    bool shouldOverride = direction2Unset || direction2SameAsDirection1
        || (direction1IsBodyBaseAxis && direction2LooksLikeSketchFallback)
        || (!direction1IsBuiltinAxis && direction2LooksLikeBuiltinFallback);
    if (!shouldOverride) {
        return;
    }

    setupTransaction();
    linearPattern->Direction2.setValue(candidateObj, candidateSub);
    parametersWidget2->updateUI();
    kickUpdateViewTimer();
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
