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
QString translate(const char* text)
{
    return QCoreApplication::translate("PartGui::TaskLinkArrayParameters", text);
}

QString objectLabel(App::DocumentObject* obj)
{
    QString label = QString::fromUtf8(obj->Label.getValue());
    if (label.isEmpty()) {
        label = QString::fromLatin1(obj->getNameInDocument());
    }

    return label;
}

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

QString taskTitle(Part::LinkArray* array)
{
    if (dynamic_cast<Part::LinkArrayCircular*>(array)) {
        return translate("Circular Link Array");
    }
    if (dynamic_cast<Part::LinkArrayPath*>(array)) {
        return translate("Path Link Array");
    }
    if (dynamic_cast<Part::LinkArrayPoint*>(array)) {
        return translate("Point Link Array");
    }

    if (dynamic_cast<Part::LinkArrayPolar*>(array)) {
        return translate("Polar Link Array");
    }

    return translate("Linear Link Array");
}

const char* taskIcon(Part::LinkArray* array)
{
    if (dynamic_cast<Part::LinkArrayCircular*>(array)) {
        return "Part_CircularLinkArray";
    }
    if (dynamic_cast<Part::LinkArrayPath*>(array)) {
        return "Part_PathLinkArray";
    }
    if (dynamic_cast<Part::LinkArrayPoint*>(array)) {
        return "Part_PointLinkArray";
    }
    if (dynamic_cast<Part::LinkArrayPolar*>(array)) {
        return "Part_PolarLinkArray";
    }
    return "LinkArray";
}

Base::Placement arrayGlobalPlacement(const Part::LinkArray* array)
{
    if (!array) {
        return {};
    }

    return App::GeoFeature::getGlobalPlacement(array);
}

void hideArraySource(App::DocumentObject* obj)
{
    if (obj) {
        obj->Visibility.setValue(false);
    }
}

Gui::View3DInventorViewer* active3DViewer()
{
    if (auto* view = Gui::getMainWindow()->activeWindow()) {
        if (view->isDerivedFrom<Gui::View3DInventor>()) {
            return static_cast<Gui::View3DInventor*>(view)->getViewer();
        }
    }

    return nullptr;
}

bool isSuppressed(App::DocumentObject* obj)
{
    auto* suppressible = obj ? obj->getExtensionByType<App::SuppressibleExtension>(true) : nullptr;
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

std::optional<Base::Vector3d> estimateElementCenter(
    Part::LinkArray* array,
    int index,
    Gui::View3DInventorViewer* viewer
)
{
    if (!array || index < 0) {
        return std::nullopt;
    }

    auto localCenter = viewProviderCenter(array->getTrueLinkedObject(false), viewer, false);
    if (!localCenter) {
        return std::nullopt;
    }

    Base::Vector3d center;
    array->getPlacementOf(std::to_string(index), nullptr).multVec(*localCenter, center);
    return center;
}
}  // namespace

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
        ui->linkedObjectButton->setText(translate("Selecting…"));
        return;
    }

    App::DocumentObject* linked = getSelectedLinkedObject();
    ui->linkedObjectButton->setText(linked ? objectLabel(linked) : translate("Select Object"));
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
    Gui::getMainWindow()->showMessage(translate("Select an object to link"));
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
    hideArraySource(linked);
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

void TaskLinkArrayParameters::fillDirectionCombo(
    Gui::ComboLinks& combo,
    Part::LinearPatternDirection direction
)
{
    combo.clear();

    App::PropertyLinkSub defaultAxis;
    const bool isLinear = dynamic_cast<Part::LinkArrayLinear*>(array);
    const bool isSecondDirection = direction == Part::LinearPatternDirection::Second;

    if (isLinear && !isSecondDirection) {
        combo.addLink(
            defaultAxis,
            translate("Object X-axis"),
            PatternParametersWidget::DefaultDirectionUserData
        );
        combo.addLink(
            nullptr,
            "Y_Axis",
            translate("Object Y-axis"),
            PatternParametersWidget::ObjectDirectionUserData
        );
        combo.addLink(
            nullptr,
            "Z_Axis",
            translate("Object Z-axis"),
            PatternParametersWidget::ObjectDirectionUserData
        );
    }
    else if (isLinear && isSecondDirection) {
        combo.addLink(
            nullptr,
            "X_Axis",
            translate("Object X-axis"),
            PatternParametersWidget::ObjectDirectionUserData
        );
        combo.addLink(
            defaultAxis,
            translate("Object Y-axis"),
            PatternParametersWidget::DefaultDirectionUserData
        );
        combo.addLink(
            nullptr,
            "Z_Axis",
            translate("Object Z-axis"),
            PatternParametersWidget::ObjectDirectionUserData
        );
    }
    else {
        combo.addLink(
            nullptr,
            "X_Axis",
            translate("Object X-axis"),
            PatternParametersWidget::ObjectDirectionUserData
        );
        combo.addLink(
            nullptr,
            "Y_Axis",
            translate("Object Y-axis"),
            PatternParametersWidget::ObjectDirectionUserData
        );
        combo.addLink(
            defaultAxis,
            translate("Object Z-axis"),
            PatternParametersWidget::DefaultDirectionUserData
        );
    }

    combo.addLink(
        nullptr,
        std::string(),
        translate("Select reference…"),
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
    if (array && array->getDocument()) {
        if (array->getDocument()->recomputeFeature(array)) {
            array->purgeTouched();
        }
    }
    std::fill(instanceControlCentersValid.begin(), instanceControlCentersValid.end(), false);
    updateInstanceControls();
}

Base::Vector3d TaskLinkArrayParameters::getPatternStartPoint() const
{
    return arrayGlobalPlacement(array).getPosition();
}

Base::Vector3d TaskLinkArrayParameters::getLinearPatternFallbackDirection(
    Part::LinearPatternDirection direction
) const
{
    auto* linear = dynamic_cast<Part::LinkArrayLinear*>(array);
    const auto* directionProp = linear
        ? (direction == Part::LinearPatternDirection::Second ? &linear->Direction2
                                                             : &linear->Direction)
        : nullptr;
    if (directionProp && !directionProp->getValue()) {
        const auto& subValues = directionProp->getSubValues();
        if (!subValues.empty()) {
            std::string role = subValues.front();
            const auto dot = role.rfind('.');
            if (dot != std::string::npos) {
                role = role.substr(dot + 1);
            }
            if (role == "X_Axis") {
                return Base::Vector3d::UnitX;
            }
            if (role == "Y_Axis") {
                return Base::Vector3d::UnitY;
            }
            if (role == "Z_Axis") {
                return Base::Vector3d::UnitZ;
            }
        }
    }

    return TaskPatternParameters::getLinearPatternFallbackDirection(direction);
}

Base::Vector3d TaskLinkArrayParameters::transformLinearPatternDirection(
    const Base::Vector3d& direction
) const
{
    Base::Vector3d transformed;
    arrayGlobalPlacement(array).getRotation().multVec(direction, transformed);
    return transformed;
}

void TaskLinkArrayParameters::transformPolarPatternAxis(gp_Ax2& axis) const
{
    axis.Transform(Part::TopoShape::convert(arrayGlobalPlacement(array).toMatrix()));
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
    Gui::getMainWindow()->showMessage(
        dynamic_cast<Part::LinkArrayPath*>(array)
            ? translate("Select connected path edges")
            : (dynamic_cast<Part::LinkArrayPoint*>(array)
                   ? translate("Select a sketch or shape containing points")
                   : (dynamic_cast<Part::LinkArrayPolar*>(array)
                          ? translate("Select a rotation axis")
                          : translate("Select a direction reference")))
    );
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
        if (msg.pSubName && msg.pSubName[0] != '\0') {
            subNames.emplace_back(msg.pSubName);
        }
    }

    if (!obj) {
        return;
    }

    subNames = cleanSubNames(std::move(subNames));

    setupPatternTransaction();
    App::PropertyLinkSub* reference = nullptr;
    const char* referenceKind = "pattern reference";
    if (auto* linear = dynamic_cast<Part::LinkArrayLinear*>(array)) {
        reference = getActiveDirectionWidget() == getSecondaryParametersWidget()
            ? &linear->Direction2
            : &linear->Direction;
        referenceKind = "linear pattern direction";
    }
    else if (auto* polar = dynamic_cast<Part::LinkArrayPolar*>(array)) {
        reference = &polar->Axis;
        referenceKind = "polar pattern axis";
    }
    else if (auto* circular = dynamic_cast<Part::LinkArrayCircular*>(array)) {
        reference = &circular->Axis;
        referenceKind = "circular pattern axis";
    }
    else if (auto* path = dynamic_cast<Part::LinkArrayPath*>(array)) {
        reference = &path->Path;
        referenceKind = "path pattern edges";
    }
    else if (auto* point = dynamic_cast<Part::LinkArrayPoint*>(array)) {
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
