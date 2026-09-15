// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2026 Boyer Pierre-Louis <pierrelouis.boyer@gmail.com>    *
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

#include <QCoreApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QString>

#include <algorithm>
#include <optional>
#include <string>

#include <gp_Ax2.hxx>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/GeoFeature.h>
#include <App/Link.h>
#include <App/Part.h>
#include <App/SuppressibleExtension.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/ComboLinks.h>
#include <Gui/Control.h>
#include <Gui/MainWindow.h>
#include <Gui/MDIView.h>
#include <Gui/Selection/Selection.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewProvider.h>
#include <Mod/Part/App/LinkArray.h>
#include <Mod/Part/App/LinkArrayCircular.h>
#include <Mod/Part/App/LinkArrayLinear.h>
#include <Mod/Part/App/LinkArrayPath.h>
#include <Mod/Part/App/LinkArrayPoint.h>
#include <Mod/Part/App/LinkArrayPolar.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/PolarPatternExtension.h>
#include <Mod/Part/App/TopoShape.h>

#include "PatternInstanceControls.h"
#include "PatternParametersWidget.h"
#include "TaskLinkArrayParameters.h"
#include "ui_TaskLinkArrayParameters.h"

using namespace PartGui;

namespace
{
std::string stripToSelectableSubName(std::string subName)
{
    if (!subName.empty() && subName.back() == '.') {
        subName.pop_back();
    }

    const auto dot = subName.rfind('.');
    if (dot == std::string::npos) {
        return subName;
    }

    const std::string tail = subName.substr(dot + 1);
    if (tail.rfind("Face", 0) == 0 || tail.rfind("Edge", 0) == 0 || tail.rfind("Vertex", 0) == 0
        || tail == "X_Axis" || tail == "Y_Axis" || tail == "Z_Axis" || tail == "XY_Plane"
        || tail == "XZ_Plane" || tail == "YZ_Plane") {
        return tail;
    }

    return subName;
}

std::vector<std::string> cleanSubNames(std::vector<std::string> subNames)
{
    if (subNames.size() == 1) {
        subNames.front() = stripToSelectableSubName(subNames.front());
    }

    return subNames;
}

Gui::View3DInventorViewer* active3DViewer()
{
    if (auto* view = freecad_cast<Gui::View3DInventor*>(Gui::getMainWindow()->activeWindow())) {
        return view->getViewer();
    }

    return nullptr;
}

bool isSuppressed(App::DocumentObject* obj)
{
    auto* suppressible = obj ? obj->getExtension<App::SuppressibleExtension>() : nullptr;
    return suppressible && suppressible->Suppressed.getValue();
}

std::optional<Base::Vector3d> viewProviderCenter(
    App::DocumentObject* obj,
    Gui::View3DInventorViewer* viewer,
    bool transform
)
{
    auto* viewProvider = obj ? Gui::Application::Instance->getViewProvider(obj) : nullptr;
    if (!viewProvider) {
        return std::nullopt;
    }

    Base::BoundBox3d bbox = viewProvider->getBoundingBox(nullptr, nullptr, transform, viewer);
    if (!bbox.IsValid()) {
        return std::nullopt;
    }

    return bbox.GetCenter();
}

}  // namespace

namespace PartGui
{

void showLinkArrayTask(App::DocumentObject* object, const App::SubObjectT& reference)
{
    auto* array = freecad_cast<Part::LinkArray*>(object);
    if (!array) {
        return;
    }
    if (Gui::Control().activeDialog(array->getDocument())) {
        return;
    }

    Gui::Control().showDialog(new PartGui::TaskDlgLinkArrayParameters(array, reference));
}

}  // namespace PartGui

/* TRANSLATOR PartGui::TaskLinkArrayParameters */

QString TaskLinkArrayParameters::taskTitle(Part::LinkArray* array)
{
    if (array->isDerivedFrom<Part::LinkArrayCircular>()) {
        return tr("Circular Link Array");
    }
    if (array->isDerivedFrom<Part::LinkArrayPath>()) {
        return tr("Path Link Array");
    }
    if (array->isDerivedFrom<Part::LinkArrayPoint>()) {
        return tr("Point Link Array");
    }

    if (array->isDerivedFrom<Part::LinkArrayPolar>()) {
        return tr("Polar Link Array");
    }

    return tr("Linear Link Array");
}

const char* TaskLinkArrayParameters::taskIcon(Part::LinkArray* array)
{
    if (array->isDerivedFrom<Part::LinkArrayCircular>()) {
        return "Part_CircularLinkArray";
    }
    if (array->isDerivedFrom<Part::LinkArrayPath>()) {
        return "Part_PathLinkArray";
    }
    if (array->isDerivedFrom<Part::LinkArrayPoint>()) {
        return "Part_PointLinkArray";
    }
    if (array->isDerivedFrom<Part::LinkArrayPolar>()) {
        return "Part_PolarLinkArray";
    }
    return "LinkArray";
}

TaskLinkArrayParameters::TaskLinkArrayParameters(
    Part::LinkArray* array,
    const App::SubObjectT& reference,
    QWidget* parent
)
    : Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap(taskIcon(array)), taskTitle(array), true, parent)
    , Gui::SelectionObserver(false, Gui::ResolveMode::OldStyleElement)
    , array(array)
    , arrayReference(reference)
{
    proxy = new QWidget(this);
    ui = std::make_unique<Ui_TaskLinkArrayParameters>();
    ui->setupUi(proxy);
    groupLayout()->addWidget(proxy);
    setupLinkedObjectButton();
    applyInitialSelection();
    Gui::View3DInventorViewer* viewer = active3DViewer();

    if (auto* circular = freecad_cast<Part::LinkArrayCircular*>(array)) {
        ui->parametersWidgetPlaceholder2->hide();
        setupCircularPatternParameterUI(
            proxy,
            ui->parametersWidgetPlaceholder,
            this,
            &circular->Axis,
            &circular->RadialDistance,
            &circular->TangentialDistance,
            &circular->NumberCircles,
            &circular->Symmetry
        );
        setupInstanceControls(viewer);
        return;
    }
    if (auto* path = freecad_cast<Part::LinkArrayPath*>(array)) {
        ui->parametersWidgetPlaceholder2->hide();
        setupPathPatternParameterUI(
            proxy,
            ui->parametersWidgetPlaceholder,
            this,
            &path->Path,
            &path->Count,
            &path->SpacingMode,
            &path->Spacing,
            &path->StartOffset,
            &path->EndOffset,
            &path->ReversePath,
            &path->Align
        );
        setupInstanceControls(viewer);
        return;
    }
    if (auto* point = freecad_cast<Part::LinkArrayPoint*>(array)) {
        ui->parametersWidgetPlaceholder2->hide();
        setupPointPatternParameterUI(proxy, ui->parametersWidgetPlaceholder, this, &point->PointObject);
        setupInstanceControls(viewer);
        return;
    }

    setupPatternParameterUI(
        proxy,
        ui->parametersWidgetPlaceholder,
        ui->parametersWidgetPlaceholder2,
        viewer,
        this
    );
    if (!array->isDerivedFrom<Part::LinkArrayLinear>()) {
        ui->parametersWidgetPlaceholder2->hide();
    }
    updatePatternSpacingLabels();
    setupInstanceControls(viewer);
}

TaskLinkArrayParameters::~TaskLinkArrayParameters()
{
    cancelPendingUpdate();
    instanceControls.reset();
    array = nullptr;
    exitLinkedObjectSelectionMode();
    exitReferenceSelectionMode();
}

App::DocumentObject* TaskLinkArrayParameters::getPatternObject() const
{
    return array;
}

void TaskLinkArrayParameters::setupLinkedObjectButton()
{
    updateLinkedObjectButton();
    connect(ui->linkedObjectButton, &QPushButton::toggled, this, [this](bool checked) {
        if (blockUpdate) {
            return;
        }

        if (checked) {
            enterLinkedObjectSelectionMode();
        }
        else {
            exitLinkedObjectSelectionMode();
        }
    });
}

void TaskLinkArrayParameters::updateLinkedObjectButton()
{
    if (linkedObjectSelectionMode) {
        ui->linkedObjectButton->setText(tr("Selecting…"));
        return;
    }

    App::DocumentObject* linked = getSelectedLinkedObject();
    ui->linkedObjectButton->setText(
        linked ? QString::fromUtf8(linked->getLabelOrName()) : tr("Select Object")
    );
}

void TaskLinkArrayParameters::enterLinkedObjectSelectionMode()
{
    if (!array) {
        return;
    }

    if (referenceSelectionMode) {
        exitReferenceSelectionMode();
    }

    linkedObjectSelectionMode = true;
    attachSelection();
    Gui::Selection().clearSelection();
    updateLinkedObjectButton();
    Gui::getMainWindow()->showMessage(tr("Select an object to link"));
}

void TaskLinkArrayParameters::exitLinkedObjectSelectionMode()
{
    if (!linkedObjectSelectionMode) {
        updateLinkedObjectButton();
        return;
    }

    linkedObjectSelectionMode = false;
    if (!referenceSelectionMode) {
        detachSelection();
    }

    blockUpdate = true;
    ui->linkedObjectButton->setChecked(false);
    blockUpdate = false;
    ui->linkedObjectButton->clearFocus();
    updateLinkedObjectButton();
    Gui::getMainWindow()->showMessage(QString());
}

void TaskLinkArrayParameters::applyLinkedObjectSelection(App::DocumentObject* linked)
{
    if (!isUsefulLinkedObject(linked)) {
        return;
    }

    setupPatternTransaction();
    array->LinkedObject.setValue(linked);
    linked->Visibility.setValue(false);
    recomputePatternFeature();
    updatePatternSpacingLabels();
    updateLinkedObjectButton();
}

void TaskLinkArrayParameters::applyInitialSelection()
{
    if (!array || array->LinkedObject.getValue()) {
        return;
    }

    auto selection = Gui::Selection().getSelectionEx(
        nullptr,
        App::DocumentObject::getClassTypeId(),
        Gui::ResolveMode::OldStyleElement,
        true
    );
    if (selection.empty()) {
        return;
    }

    applyLinkedObjectSelection(selection.front().getObject());
}

bool TaskLinkArrayParameters::isUsefulLinkedObject(App::DocumentObject* obj) const
{
    if (!obj || !array || obj == array || obj->getDocument() != array->getDocument()
        || obj->isInOutListRecursive(array)) {
        return false;
    }

    if (obj->isDerivedFrom(Part::Feature::getClassTypeId())
        || obj->isDerivedFrom(App::Part::getClassTypeId())) {
        return true;
    }

    auto* link = freecad_cast<App::Link*>(obj);
    return link && !link->isLinkGroup();
}

App::DocumentObject* TaskLinkArrayParameters::getSelectedLinkedObject() const
{
    return array ? array->LinkedObject.getValue() : nullptr;
}

void TaskLinkArrayParameters::setupInstanceControls(Gui::View3DInventorViewer* viewer)
{
    instanceControlsViewer = viewer;
    if (!viewer) {
        instanceControls.reset();
        return;
    }

    instanceControls = std::make_unique<PatternInstanceControls>(viewer, this);
    connect(
        instanceControls.get(),
        &PatternInstanceControls::toggleRequested,
        this,
        [this](int index, bool suppress) { setInstanceSuppressed(index, suppress); }
    );
    updateInstanceControls();
}

std::optional<Base::Vector3d> TaskLinkArrayParameters::getInstanceCenter(int index) const
{
    auto* root = arrayReference.getObject();
    auto* viewProvider = root ? Gui::Application::Instance->getViewProvider(root) : nullptr;
    if (!viewProvider || index < 0) {
        return std::nullopt;
    }

    std::string sub = arrayReference.getSubNameNoElement();
    if (!sub.empty() && sub.back() != '.') {
        sub += '.';
    }
    sub += std::to_string(index) + '.';
    const auto bbox = viewProvider->getBoundingBox(sub.c_str(), nullptr, true, instanceControlsViewer);
    return bbox.IsValid() ? std::optional<Base::Vector3d>(bbox.GetCenter()) : std::nullopt;
}

std::optional<Base::Vector3d> TaskLinkArrayParameters::estimateInstanceCenter(int index) const
{
    auto localCenter
        = viewProviderCenter(array->getTrueLinkedObject(false), instanceControlsViewer, false);
    if (!localCenter) {
        return std::nullopt;
    }

    // getPlacementOf includes the array's own placement; replace it with the edited occurrence.
    const auto localPlacement = App::GeoFeature::getGlobalPlacement(array, array, "");
    const auto placement = getArrayPlacement() * localPlacement.inverse()
        * array->getPlacementOf(std::to_string(index), nullptr);
    Base::Vector3d center;
    placement.multVec(*localCenter, center);
    return center;
}

void TaskLinkArrayParameters::updateInstanceControls()
{
    if (!instanceControls || !instanceControlsViewer) {
        return;
    }

    if (!array) {
        instanceControls->clear();
        return;
    }

    const auto elements = array->ElementList.getValues();
    if (instanceControlCenters.size() != elements.size()) {
        instanceControlCenters.resize(elements.size());
        instanceControlCentersValid.assign(elements.size(), false);
    }

    std::vector<PatternInstanceControls::Instance> instances;
    instances.reserve(elements.size());
    for (size_t i = 0; i < elements.size(); ++i) {
        App::DocumentObject* element = elements[i];
        if (!element) {
            continue;
        }

        const bool suppressed = isSuppressed(element);
        std::optional<Base::Vector3d> center;
        if (!suppressed) {
            center = getInstanceCenter(static_cast<int>(i));
        }
        if (center) {
            instanceControlCenters[i] = *center;
            instanceControlCentersValid[i] = true;
        }
        else if (instanceControlCentersValid[i]) {
            center = instanceControlCenters[i];
        }
        else {
            center = estimateInstanceCenter(static_cast<int>(i));
        }

        if (!center) {
            continue;
        }

        instances.push_back({static_cast<int>(i), *center, suppressed});
    }

    instanceControls->setInstances(instances);
}

void TaskLinkArrayParameters::setInstanceSuppressed(int index, bool suppress)
{
    if (!array || index < 0) {
        return;
    }

    const auto elements = array->ElementList.getValues();
    const auto idx = static_cast<size_t>(index);
    if (idx >= elements.size() || !elements[idx]) {
        return;
    }

    auto* suppressible = elements[idx]->getExtension<App::SuppressibleExtension>();
    if (!suppressible || suppressible->Suppressed.getValue() == suppress) {
        return;
    }

    if (suppress) {
        auto center = getInstanceCenter(index);
        if (center && idx < instanceControlCenters.size()) {
            instanceControlCenters[idx] = *center;
            instanceControlCentersValid[idx] = true;
        }
    }

    setupPatternTransaction();
    suppressible->Suppressed.setValue(suppress);
    updateInstanceControls();
}

void TaskLinkArrayParameters::fillDirectionCombo(
    Gui::ComboLinks& combo,
    Part::LinearPatternDirection direction
)
{
    combo.clear();

    App::PropertyLinkSub defaultAxis;
    const bool isLinear = array->isDerivedFrom<Part::LinkArrayLinear>();
    const bool isSecondDirection = direction == Part::LinearPatternDirection::Second;

    if (isLinear && !isSecondDirection) {
        combo.addLink(defaultAxis, tr("Object X-axis"), PatternParametersWidget::DefaultDirectionUserData);
        combo.addLink(
            nullptr,
            "Y_Axis",
            tr("Object Y-axis"),
            PatternParametersWidget::ObjectDirectionUserData
        );
        combo.addLink(
            nullptr,
            "Z_Axis",
            tr("Object Z-axis"),
            PatternParametersWidget::ObjectDirectionUserData
        );
    }
    else if (isLinear && isSecondDirection) {
        combo.addLink(
            nullptr,
            "X_Axis",
            tr("Object X-axis"),
            PatternParametersWidget::ObjectDirectionUserData
        );
        combo.addLink(defaultAxis, tr("Object Y-axis"), PatternParametersWidget::DefaultDirectionUserData);
        combo.addLink(
            nullptr,
            "Z_Axis",
            tr("Object Z-axis"),
            PatternParametersWidget::ObjectDirectionUserData
        );
    }
    else {
        combo.addLink(
            nullptr,
            "X_Axis",
            tr("Object X-axis"),
            PatternParametersWidget::ObjectDirectionUserData
        );
        combo.addLink(
            nullptr,
            "Y_Axis",
            tr("Object Y-axis"),
            PatternParametersWidget::ObjectDirectionUserData
        );
        combo.addLink(defaultAxis, tr("Object Z-axis"), PatternParametersWidget::DefaultDirectionUserData);
    }

    combo.addLink(
        nullptr,
        std::string(),
        tr("Select reference…"),
        PatternParametersWidget::SelectReferenceUserData
    );
}

void TaskLinkArrayParameters::setupPatternTransaction()
{
    if (!array) {
        return;
    }

    App::Document* doc = array->getDocument();
    if (!doc || doc->getBookedTransactionID() != App::NullTransaction) {
        return;
    }

    std::string name("Edit ");
    name += array->Label.getValue();
    doc->openTransaction(name.c_str());
}

void TaskLinkArrayParameters::recomputePatternFeature()
{
    if (array && array->getDocument() && array->getDocument()->recomputeFeature(array)) {
        array->purgeTouched();
    }
    // vector<bool> does not satisfy the C++20 output_range requirements of ranges::fill.
    std::fill(instanceControlCentersValid.begin(), instanceControlCentersValid.end(), false);
    updateInstanceControls();
}

Base::Placement TaskLinkArrayParameters::getArrayPlacement() const
{
    return App::GeoFeature::getGlobalPlacement(
        array,
        arrayReference.getObject(),
        arrayReference.getSubName()
    );
}

Base::Vector3d TaskLinkArrayParameters::getPatternStartPoint() const
{
    return array ? getArrayPlacement().getPosition() : Base::Vector3d();
}

Base::Vector3d TaskLinkArrayParameters::getLinearPatternFallbackDirection(
    Part::LinearPatternDirection direction
) const
{
    Base::Vector3d fallback = TaskPatternParameters::getLinearPatternFallbackDirection(direction);
    auto* linear = freecad_cast<Part::LinkArrayLinear*>(array);
    if (!linear) {
        return fallback;
    }

    const bool second = direction == Part::LinearPatternDirection::Second;
    const auto& directionProp = second ? linear->Direction2 : linear->Direction;
    if (!directionProp.getValue()) {
        const auto& subValues = directionProp.getSubValues();
        if (!subValues.empty()) {
            std::string role = subValues.front();
            const auto dot = role.rfind('.');
            if (dot != std::string::npos) {
                role = role.substr(dot + 1);
            }
            if (role == "X_Axis") {
                fallback = Base::Vector3d::UnitX;
            }
            else if (role == "Y_Axis") {
                fallback = Base::Vector3d::UnitY;
            }
            else if (role == "Z_Axis") {
                fallback = Base::Vector3d::UnitZ;
            }
        }
    }

    const auto& reversed = second ? linear->Reversed2 : linear->Reversed;
    return reversed.getValue() ? -fallback : fallback;
}

Base::Vector3d TaskLinkArrayParameters::transformLinearPatternDirection(
    const Base::Vector3d& direction
) const
{
    if (!array) {
        return direction;
    }
    Base::Vector3d transformed;
    getArrayPlacement().getRotation().multVec(direction, transformed);
    return transformed;
}

void TaskLinkArrayParameters::transformPolarPatternAxis(gp_Ax2& axis) const
{
    if (array) {
        axis.Transform(Part::TopoShape::convert(getArrayPlacement().toMatrix()));
    }
}

void TaskLinkArrayParameters::onReferenceSelectionRequested()
{
    enterReferenceSelectionMode();
}

void TaskLinkArrayParameters::enterReferenceSelectionMode()
{
    if (linkedObjectSelectionMode) {
        exitLinkedObjectSelectionMode();
    }

    referenceSelectionMode = true;
    attachSelection();
    Gui::Selection().clearSelection();
    const QString message = [this]() {
        if (array->isDerivedFrom<Part::LinkArrayPath>()) {
            return tr("Select connected path edges");
        }
        if (array->isDerivedFrom<Part::LinkArrayPoint>()) {
            return tr("Select a sketch or shape containing points");
        }
        if (array->isDerivedFrom<Part::LinkArrayPolar>()
            || array->isDerivedFrom<Part::LinkArrayCircular>()) {
            return tr("Select a rotation axis");
        }
        return tr("Select a direction reference");
    }();
    Gui::getMainWindow()->showMessage(message);
}

void TaskLinkArrayParameters::onPatternParametersChanged()
{
    if (blockUpdate) {
        return;
    }

    kickUpdateViewTimer();
}

void TaskLinkArrayParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type != Gui::SelectionChanges::AddSelection || !array) {
        return;
    }

    if (linkedObjectSelectionMode) {
        App::Document* doc = App::GetApplication().getDocument(msg.pDocName);
        App::DocumentObject* obj = doc ? doc->getObject(msg.pObjectName) : nullptr;
        if (isUsefulLinkedObject(obj)) {
            applyLinkedObjectSelection(obj);
            exitLinkedObjectSelectionMode();
        }
        return;
    }

    if (!referenceSelectionMode) {
        return;
    }

    App::DocumentObject* obj = nullptr;
    std::vector<std::string> subNames;

    auto selection = Gui::Selection().getSelectionEx(
        "*",
        App::DocumentObject::getClassTypeId(),
        Gui::ResolveMode::FollowLink,
        true
    );
    if (!selection.empty()) {
        obj = selection.front().getObject();
        subNames = selection.front().getSubNames();
    }
    else {
        App::Document* doc = App::GetApplication().getDocument(msg.pDocName);
        obj = doc ? doc->getObject(msg.pObjectName) : nullptr;
        if (!Base::Tools::isNullOrEmpty(msg.pSubName)) {
            subNames.emplace_back(msg.pSubName);
        }
    }

    if (!obj || obj == array || obj->isInOutListRecursive(array)) {
        return;
    }

    subNames = cleanSubNames(std::move(subNames));

    setupPatternTransaction();
    App::PropertyLinkSub* reference = nullptr;
    const char* referenceKind = "pattern reference";
    if (auto* linear = freecad_cast<Part::LinkArrayLinear*>(array)) {
        reference = getActiveDirectionWidget() == getSecondaryParametersWidget()
            ? &linear->Direction2
            : &linear->Direction;
        referenceKind = "linear pattern direction";
    }
    else if (auto* polar = freecad_cast<Part::LinkArrayPolar*>(array)) {
        reference = &polar->Axis;
        referenceKind = "polar pattern axis";
    }
    else if (auto* circular = freecad_cast<Part::LinkArrayCircular*>(array)) {
        reference = &circular->Axis;
        referenceKind = "circular pattern axis";
    }
    else if (auto* path = freecad_cast<Part::LinkArrayPath*>(array)) {
        reference = &path->Path;
        referenceKind = "path pattern edges";
    }
    else if (auto* point = freecad_cast<Part::LinkArrayPoint*>(array)) {
        reference = &point->PointObject;
        referenceKind = "point pattern object";
        subNames.clear();
    }

    if (!reference) {
        return;
    }

    App::DocumentObject* oldObj = reference->getValue();
    std::vector<std::string> oldSubNames = reference->getSubValues();

    try {
        reference->setValue(obj, subNames);
        recomputePatternFeature();
        updatePatternSpacingLabels();
        updatePatternParameterUI();
    }
    catch (const Base::Exception& e) {
        reference->setValue(oldObj, oldSubNames);
        Base::Console().warning("Could not set %s: %s\n", referenceKind, e.what());
    }

    exitReferenceSelectionMode();
}

void TaskLinkArrayParameters::exitReferenceSelectionMode()
{
    referenceSelectionMode = false;
    clearActiveDirectionWidget();
    if (!linkedObjectSelectionMode) {
        detachSelection();
    }
    Gui::Selection().clearSelection();
    Gui::getMainWindow()->showMessage(QString());
}

bool TaskLinkArrayParameters::accept()
{
    if (!array) {
        return true;
    }

    try {
        App::DocumentObject* linked = getSelectedLinkedObject();
        if (!linked) {
            QMessageBox::warning(this, tr("Input Error"), tr("Select an object to link."));
            return false;
        }

        setupPatternTransaction();
        array->LinkedObject.setValue(linked);
        linked->Visibility.setValue(false);
        applyPatternParameters(array);
        if (!consumePendingUpdate()) {
            recomputePatternFeature();
        }
        array->getDocument()->commitTransaction();
    }
    catch (const Base::Exception& e) {
        // Keep the creation transaction and its array alive so the user can correct the input.
        QMessageBox::warning(
            this,
            tr("Input Error"),
            QCoreApplication::translate("Exception", e.what())
        );
        return false;
    }

    return true;
}

bool TaskLinkArrayParameters::reject()
{
    cancelPendingUpdate();
    if (array && array->getDocument()) {
        array->getDocument()->abortTransaction();
        Gui::Command::updateActive();
    }

    return true;
}

/* TRANSLATOR PartGui::TaskDlgLinkArrayParameters */

TaskDlgLinkArrayParameters::TaskDlgLinkArrayParameters(
    Part::LinkArray* array,
    const App::SubObjectT& reference
)
{
    associateToObject3dView(reference.getObject());
    setAutoCloseOnDeletedDocument(true);
    setAutoCloseOnTransactionChange(true);
    parameter = new TaskLinkArrayParameters(array, reference);
    Content.push_back(parameter);
}

bool TaskDlgLinkArrayParameters::accept()
{
    parameter->exitLinkedObjectSelectionMode();
    parameter->exitReferenceSelectionMode();
    return parameter->accept();
}

bool TaskDlgLinkArrayParameters::reject()
{
    parameter->exitLinkedObjectSelectionMode();
    parameter->exitReferenceSelectionMode();
    return parameter->reject();
}

#include "moc_TaskLinkArrayParameters.cpp"
