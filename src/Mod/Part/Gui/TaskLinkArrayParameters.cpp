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

#include <gp_Ax2.hxx>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/GeoFeature.h>
#include <App/Link.h>
#include <App/Part.h>
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
#include <Mod/Part/App/LinkArray.h>
#include <Mod/Part/App/LinkArrayCircular.h>
#include <Mod/Part/App/LinkArrayLinear.h>
#include <Mod/Part/App/LinkArrayPath.h>
#include <Mod/Part/App/LinkArrayPoint.h>
#include <Mod/Part/App/LinkArrayPolar.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/PolarPatternExtension.h>
#include <Mod/Part/App/TopoShape.h>

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
    if (tail.rfind("Face", 0) == 0 || tail.rfind("Edge", 0) == 0
        || tail.rfind("Vertex", 0) == 0 || tail == "X_Axis" || tail == "Y_Axis"
        || tail == "Z_Axis" || tail == "XY_Plane" || tail == "XZ_Plane"
        || tail == "YZ_Plane") {
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
}  // namespace

namespace PartGui
{

void showLinkArrayTask(App::DocumentObject* object)
{
    auto* array = dynamic_cast<Part::LinkArray*>(object);
    if (!array) {
        return;
    }
    if (Gui::Control().activeDialog(array->getDocument())) {
        return;
    }

    Gui::Control().showDialog(new PartGui::TaskDlgLinkArrayParameters(array));
}

void showLinkArrayLinearTask(App::DocumentObject* object)
{
    showLinkArrayTask(object);
}

void showLinkArrayPathTask(App::DocumentObject* object)
{
    showLinkArrayTask(object);
}

void showLinkArrayPointTask(App::DocumentObject* object)
{
    showLinkArrayTask(object);
}

void showLinkArrayCircularTask(App::DocumentObject* object)
{
    showLinkArrayTask(object);
}

void showLinkArrayPolarTask(App::DocumentObject* object)
{
    showLinkArrayTask(object);
}

}  // namespace PartGui

/* TRANSLATOR PartGui::TaskLinkArrayParameters */

TaskLinkArrayParameters::TaskLinkArrayParameters(Part::LinkArray* array, QWidget* parent)
    : Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap(taskIcon(array)),
                             taskTitle(array),
                             true,
                             parent)
    , Gui::SelectionObserver(false, Gui::ResolveMode::OldStyleElement)
    , array(array)
{
    proxy = new QWidget(this);
    ui = std::make_unique<Ui_TaskLinkArrayParameters>();
    ui->setupUi(proxy);
    groupLayout()->addWidget(proxy);
    setupLinkedObjectButton();
    applyInitialSelection();

    if (dynamic_cast<Part::LinkArrayCircular*>(array)) {
        ui->parametersWidgetPlaceholder2->hide();
        auto* circular = static_cast<Part::LinkArrayCircular*>(array);
        setupCircularPatternParameterUI(proxy,
                                        ui->parametersWidgetPlaceholder,
                                        this,
                                        500,
                                        &circular->Axis,
                                        &circular->RadialDistance,
                                        &circular->TangentialDistance,
                                        &circular->NumberCircles,
                                        &circular->Symmetry);
        return;
    }
    if (dynamic_cast<Part::LinkArrayPath*>(array)) {
        ui->parametersWidgetPlaceholder2->hide();
        auto* path = static_cast<Part::LinkArrayPath*>(array);
        setupPathPatternParameterUI(proxy,
                                    ui->parametersWidgetPlaceholder,
                                    this,
                                    500,
                                    &path->Path,
                                    &path->Count,
                                    &path->SpacingMode,
                                    &path->Spacing,
                                    &path->StartOffset,
                                    &path->EndOffset,
                                    &path->ReversePath,
                                    &path->Align);
        return;
    }
    if (dynamic_cast<Part::LinkArrayPoint*>(array)) {
        ui->parametersWidgetPlaceholder2->hide();
        auto* point = static_cast<Part::LinkArrayPoint*>(array);
        setupPointPatternParameterUI(proxy,
                                     ui->parametersWidgetPlaceholder,
                                     this,
                                     &point->PointObject);
        return;
    }

    Gui::View3DInventorViewer* viewer = nullptr;
    if (auto* view = Gui::getMainWindow()->activeWindow()) {
        if (view->isDerivedFrom<Gui::View3DInventor>()) {
            viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        }
    }

    setupPatternParameterUI(proxy,
                            ui->parametersWidgetPlaceholder,
                            ui->parametersWidgetPlaceholder2,
                            viewer,
                            this,
                            500);
    if (!dynamic_cast<Part::LinkArrayLinear*>(array)) {
        ui->parametersWidgetPlaceholder2->hide();
    }
    updatePatternSpacingLabels();
}

TaskLinkArrayParameters::~TaskLinkArrayParameters()
{
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
        ui->linkedObjectButton->setText(translate("Selecting"));
        return;
    }

    App::DocumentObject* linked = getSelectedLinkedObject();
    ui->linkedObjectButton->setText(linked ? objectLabel(linked) : translate("None"));
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

void TaskLinkArrayParameters::fillDirectionCombo(Gui::ComboLinks& combo,
                                                 Part::LinearPatternDirection direction)
{
    combo.clear();

    App::PropertyLinkSub defaultAxis;
    const bool isLinear = dynamic_cast<Part::LinkArrayLinear*>(array);
    const bool isSecondDirection = direction == Part::LinearPatternDirection::Second;

    if (isLinear && !isSecondDirection) {
        combo.addLink(defaultAxis,
                      translate("Object X-axis"),
                      PatternParametersWidget::DefaultDirectionUserData);
        combo.addLink(nullptr,
                      "Y_Axis",
                      translate("Object Y-axis"),
                      PatternParametersWidget::ObjectDirectionUserData);
        combo.addLink(nullptr,
                      "Z_Axis",
                      translate("Object Z-axis"),
                      PatternParametersWidget::ObjectDirectionUserData);
    }
    else if (isLinear && isSecondDirection) {
        combo.addLink(nullptr,
                      "X_Axis",
                      translate("Object X-axis"),
                      PatternParametersWidget::ObjectDirectionUserData);
        combo.addLink(defaultAxis,
                      translate("Object Y-axis"),
                      PatternParametersWidget::DefaultDirectionUserData);
        combo.addLink(nullptr,
                      "Z_Axis",
                      translate("Object Z-axis"),
                      PatternParametersWidget::ObjectDirectionUserData);
    }
    else {
        combo.addLink(nullptr,
                      "X_Axis",
                      translate("Object X-axis"),
                      PatternParametersWidget::ObjectDirectionUserData);
        combo.addLink(nullptr,
                      "Y_Axis",
                      translate("Object Y-axis"),
                      PatternParametersWidget::ObjectDirectionUserData);
        combo.addLink(defaultAxis,
                      translate("Object Z-axis"),
                      PatternParametersWidget::DefaultDirectionUserData);
    }

    combo.addLink(nullptr,
                  std::string(),
                  translate("Select reference..."),
                  PatternParametersWidget::SelectReferenceUserData);
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
                return Base::Vector3d(1.0, 0.0, 0.0);
            }
            if (role == "Y_Axis") {
                return Base::Vector3d(0.0, 1.0, 0.0);
            }
            if (role == "Z_Axis") {
                return Base::Vector3d(0.0, 0.0, 1.0);
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

    auto selection = Gui::Selection().getSelectionEx("*",
                                                     App::DocumentObject::getClassTypeId(),
                                                     Gui::ResolveMode::FollowLink,
                                                     true);
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

bool TaskLinkArrayParameters::accept()
{
    if (!array) {
        return true;
    }

    try {
        App::DocumentObject* linked = getSelectedLinkedObject();
        if (!linked) {
            QMessageBox::warning(this,
                                 translate("Input error"),
                                 translate("Select an object to link."));
            return false;
        }

        array->LinkedObject.setValue(linked);
        applyPatternParameters(array);
        consumePendingUpdate();
        recomputePatternFeature();
        array->getDocument()->commitTransaction();
    }
    catch (const Base::Exception& e) {
        array->getDocument()->abortTransaction();
        QMessageBox::warning(this,
                             translate("Input error"),
                             QCoreApplication::translate("Exception", e.what()));
        return false;
    }

    return true;
}

bool TaskLinkArrayParameters::reject()
{
    if (array && array->getDocument()) {
        array->getDocument()->abortTransaction();
        Gui::Command::updateActive();
    }

    return true;
}

/* TRANSLATOR PartGui::TaskDlgLinkArrayParameters */

TaskDlgLinkArrayParameters::TaskDlgLinkArrayParameters(Part::LinkArray* array)
{
    parameter = new TaskLinkArrayParameters(array);
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
