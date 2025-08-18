/***************************************************************************
 *   Copyright (c) 2015 Thomas Anderson <blobfish[at]gmx.com>              *
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
#ifndef _PreComp_
#include <cassert>
#include <limits>
#include <QApplication>
#endif

#include <View3DInventorViewer.h>
#include <Utilities.h>

#include <App/Document.h>
#include <App/GeoFeature.h>
#include <App/Services.h>
#include <Base/Precision.h>
#include <Base/ServiceProvider.h>
#include <Base/Tools.h>

#include "Document.h"  // must be before TaskTransform.h
#include "Application.h"
#include "BitmapFactory.h"
#include "Command.h"
#include "Inventor/Draggers/SoTransformDragger.h"
#include "QuantitySpinBox.h"
#include "ViewProviderDragger.h"
#include "TaskView/TaskView.h"

#include "TaskTransform.h"
#include "ui_TaskTransform.h"

#include <Inventor/nodes/SoPickStyle.h>

using namespace Gui;

namespace
{

void alignGridLayoutColumns(const std::list<QGridLayout*>& layouts, unsigned column = 0)
{
    std::vector<int> widths;

    auto getActualWidth = [&](const QGridLayout* layout) -> int {
        if (auto const item = layout->itemAtPosition(0, column)) {
            return item->geometry().width();
        }

        return 0;
    };

    for (const auto layout : layouts) {
        widths.push_back(getActualWidth(layout));
    }

    const auto maxWidth = *std::max_element(widths.begin(), widths.end());
    for (const auto layout : layouts) {
        layout->setColumnMinimumWidth(column, maxWidth);
    }
}

}  // namespace

TaskTransform::TaskTransform(Gui::ViewProviderDragger* vp,
                             Gui::SoTransformDragger* dragger,
                             QWidget* parent,
                             App::SubObjectPlacementProvider* subObjectPlacemenProvider,
                             App::CenterOfMassProvider* centerOfMassProvider)
    : TaskBox(Gui::BitmapFactory().pixmap("Std_TransformManip.svg"), tr("Transform"), false, parent)
    , vp(vp)
    , subObjectPlacementProvider(subObjectPlacemenProvider)
    , centerOfMassProvider(centerOfMassProvider)
    , dragger(dragger)
    , ui(new Ui_TaskTransformDialog)
{
    blockSelection(true);

    dragger->addStartCallback(dragStartCallback, this);
    dragger->addMotionCallback(dragMotionCallback, this);

    vp->resetTransformOrigin();

    referencePlacement = vp->getObjectPlacement();
    referenceRotation = referencePlacement.getRotation();

    globalOrigin = vp->getObjectPlacement() * App::GeoFeature::getGlobalPlacement(vp->getObject()).inverse();

    setupGui();
}

TaskTransform::~TaskTransform()
{
    Gui::Application::Instance->commandManager()
        .getCommandByName("Std_OrthographicCamera")
        ->setEnabled(true);

    Gui::Application::Instance->commandManager()
        .getCommandByName("Std_PerspectiveCamera")
        ->setEnabled(true);

    savePreferences();
}

void TaskTransform::dragStartCallback([[maybe_unused]] void* data,
                                      [[maybe_unused]] SoDragger* dragger)
{
    // This is called when a manipulator is about to manipulating
    if (firstDrag) {
        Gui::Application::Instance->activeDocument()->openCommand(
            QT_TRANSLATE_NOOP("Command", "Transform"));
        firstDrag = false;
    }
}

void TaskTransform::dragMotionCallback(void* data, [[maybe_unused]] SoDragger* dragger)
{
    auto task = static_cast<TaskTransform*>(data);

    const auto currentRotation = task->vp->getOriginalDraggerPlacement().getRotation();
    const auto updatedRotation = task->vp->getDraggerPlacement().getRotation();

    const auto rotationAxisHasChanged = [task](auto first, auto second) {
        double alpha, beta, gamma;

        (first.inverse() * second).getEulerAngles(task->eulerSequence(), alpha, beta, gamma);

        auto angles = {alpha, beta, gamma};
        const int changed = std::count_if(angles.begin(), angles.end(), [](double angle) {
            return std::fabs(angle) > tolerance;
        });

        // if representation of both differs by more than one axis the axis of rotation must be
        // different
        return changed > 1;
    };

    if (!updatedRotation.isSame(currentRotation, tolerance)) {
        task->resetReferencePlacement();

        if (rotationAxisHasChanged(task->referenceRotation, updatedRotation)) {
            task->referenceRotation = currentRotation;
        }
    }

    task->updatePositionAndRotationUi();
}

void TaskTransform::loadPlacementModeItems() const
{
    ui->placementComboBox->clear();

    ui->placementComboBox->addItem(tr("Object origin"),
                                   QVariant::fromValue(PlacementMode::ObjectOrigin));

    if (centerOfMassProvider->ofDocumentObject(vp->getObject()).has_value()) {
        ui->placementComboBox->addItem(tr("Center of mass / centroid"),
                                       QVariant::fromValue(PlacementMode::Centroid));
    }

    if (subObjectPlacementProvider) {
        ui->placementComboBox->addItem(tr("Custom"), QVariant::fromValue(PlacementMode::Custom));
    }
}

void TaskTransform::loadPositionModeItems() const
{
    ui->positionModeComboBox->clear();
    ui->positionModeComboBox->addItem(tr("Local"), QVariant::fromValue(PositionMode::Local));
    ui->positionModeComboBox->addItem(tr("Global"), QVariant::fromValue(PositionMode::Global));
}

void TaskTransform::setupGui()
{
    auto proxy = new QWidget(this);
    ui->setupUi(proxy);
    this->groupLayout()->addWidget(proxy);

    loadPlacementModeItems();
    loadPositionModeItems();

    ui->referencePickerWidget->hide();
    ui->alignRotationCheckBox->hide();

    for (auto positionSpinBox : {ui->translationIncrementSpinBox,
                                 ui->xPositionSpinBox,
                                 ui->yPositionSpinBox,
                                 ui->zPositionSpinBox}) {
        positionSpinBox->setUnit(Base::Unit::Length);
    }

    for (auto rotationSpinBox : {ui->rotationIncrementSpinBox,
                                 ui->xRotationSpinBox,
                                 ui->yRotationSpinBox,
                                 ui->zRotationSpinBox}) {
        rotationSpinBox->setUnit(Base::Unit::Angle);
    }

    connect(ui->translationIncrementSpinBox,
            qOverload<double>(&QuantitySpinBox::valueChanged),
            this,
            [this](double) {
                updateIncrements();
            });
    connect(ui->rotationIncrementSpinBox,
            qOverload<double>(&QuantitySpinBox::valueChanged),
            this,
            [this](double) {
                updateIncrements();
            });
    connect(ui->positionModeComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &TaskTransform::onCoordinateSystemChange);
    connect(ui->placementComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &TaskTransform::onPlacementModeChange);
    connect(ui->pickTransformOriginButton,
            &QPushButton::clicked,
            this,
            &TaskTransform::onPickTransformOrigin);
    connect(ui->alignToOtherObjectButton,
            &QPushButton::clicked,
            this,
            &TaskTransform::onAlignToOtherObject);
    connect(ui->moveOptionsButton,
            &QPushButton::toggled,
            ui->frameMoveOptions,
            &QWidget::setVisible);
    connect(ui->translateCheckbox, &QCheckBox::toggled, this, [this](bool translateChecked) {
        ui->matchXcheckbox->setEnabled(translateChecked);
        ui->matchYcheckbox->setEnabled(translateChecked);
        ui->matchZcheckbox->setEnabled(translateChecked);
    });
    connect(ui->rotateCheckbox, &QCheckBox::toggled, this, [this](bool rotateChecked) {
        ui->alignXcheckbox->setEnabled(rotateChecked);
        ui->alignYcheckbox->setEnabled(rotateChecked);
        ui->alignZcheckbox->setEnabled(rotateChecked);
    });

    connect(ui->flipPartButton, &QPushButton::clicked, this, &TaskTransform::onFlip);

    connect(ui->alignRotationCheckBox,
            &QCheckBox::clicked,
            this,
            &TaskTransform::onAlignRotationChanged);

    for (auto positionSpinBox :
         {ui->xPositionSpinBox, ui->yPositionSpinBox, ui->zPositionSpinBox}) {
        connect(positionSpinBox,
                qOverload<double>(&QuantitySpinBox::valueChanged),
                this,
                [this](double) {
                    onPositionChange();
                });
    }

    for (auto rotationSpinBox :
         {ui->xRotationSpinBox, ui->yRotationSpinBox, ui->zRotationSpinBox}) {
        connect(rotationSpinBox,
                qOverload<double>(&QuantitySpinBox::valueChanged),
                this,
                [this,rotationSpinBox](double) {
                    onRotationChange(rotationSpinBox);
                });
    }

    alignGridLayoutColumns({ui->absolutePositionLayout,
                            ui->absoluteRotationLayout,
                            ui->transformOriginLayout,
                            ui->referencePickerLayout});

    loadPreferences();

    updateInputLabels();
    updateDraggerLabels();
    updateIncrements();
    updatePositionAndRotationUi();
}

void TaskTransform::loadPreferences()
{
    double lastTranslationIncrement = hGrp->GetFloat("LastTranslationIncrement", 1.0);
    double lastRotationIncrement = hGrp->GetFloat("LastRotationIncrement", 5.0);

    ui->translationIncrementSpinBox->setValue(lastTranslationIncrement);
    ui->rotationIncrementSpinBox->setValue(lastRotationIncrement);
    ui->moveOptionsButton->setIcon(Gui::BitmapFactory().pixmap("Std_DlgParameter"));
}

void TaskTransform::savePreferences()
{
    hGrp->SetFloat("LastTranslationIncrement", ui->translationIncrementSpinBox->rawValue());
    hGrp->SetFloat("LastRotationIncrement", ui->rotationIncrementSpinBox->rawValue());
}

void TaskTransform::updatePositionAndRotationUi() const
{
    const auto referencePlacement = currentCoordinateSystem().origin;

    const auto xyzPlacement = vp->getDraggerPlacement();
    const auto uvwPlacement = referencePlacement.inverse() * xyzPlacement;

    auto fixNegativeZero = [](const double value) {
        return std::fabs(value) < Base::Precision::Confusion() ? 0.0 : value;
    };

    auto setPositionValues = [&](const Base::Vector3d& vec, auto* x, auto* y, auto* z) {
        [[maybe_unused]]
        auto blockers = {QSignalBlocker(x), QSignalBlocker(y), QSignalBlocker(z)};

        x->setValue(fixNegativeZero(vec.x));
        y->setValue(fixNegativeZero(vec.y));
        z->setValue(fixNegativeZero(vec.z));
    };

    auto setRotationValues = [&](const Base::Rotation& rot, auto* x, auto* y, auto* z) {
        [[maybe_unused]]
        auto blockers = {QSignalBlocker(x), QSignalBlocker(y), QSignalBlocker(z)};

        double alpha, beta, gamma;
        rot.getEulerAngles(eulerSequence(), alpha, beta, gamma);

        x->setValue(fixNegativeZero(alpha));
        y->setValue(fixNegativeZero(beta));
        z->setValue(fixNegativeZero(gamma));
    };

    setPositionValues(uvwPlacement.getPosition(),
                      ui->xPositionSpinBox,
                      ui->yPositionSpinBox,
                      ui->zPositionSpinBox);

    setRotationValues(positionMode == PositionMode::Local
                          ? referenceRotation.inverse() * xyzPlacement.getRotation()
                          : uvwPlacement.getRotation(),
                      ui->xRotationSpinBox,
                      ui->yRotationSpinBox,
                      ui->zRotationSpinBox);
}

void TaskTransform::updateInputLabels() const
{
    auto [xLabel, yLabel, zLabel] = currentCoordinateSystem().labels;

    ui->xPositionLabel->setText(QString::fromStdString(xLabel));
    ui->yPositionLabel->setText(QString::fromStdString(yLabel));
    ui->zPositionLabel->setText(QString::fromStdString(zLabel));

    ui->xRotationLabel->setText(QString::fromStdString(xLabel));
    ui->yRotationLabel->setText(QString::fromStdString(yLabel));
    ui->zRotationLabel->setText(QString::fromStdString(zLabel));
}

void TaskTransform::updateDraggerLabels() const
{
    auto coordinateSystem =
        isDraggerAlignedToCoordinateSystem() ? globalCoordinateSystem() : localCoordinateSystem();

    auto [xLabel, yLabel, zLabel] = coordinateSystem.labels;

    dragger->xAxisLabel.setValue(xLabel.c_str());
    dragger->yAxisLabel.setValue(yLabel.c_str());
    dragger->zAxisLabel.setValue(zLabel.c_str());
}

void TaskTransform::updateIncrements() const
{
    dragger->translationIncrement.setValue(
        std::max(ui->translationIncrementSpinBox->rawValue(), 0.001));
    dragger->rotationIncrement.setValue(
        Base::toRadians(std::max(ui->rotationIncrementSpinBox->rawValue(), 0.01)));
}

void TaskTransform::setSelectionMode(SelectionMode mode)
{
    Gui::Selection().clearSelection();

    SoPickStyle* draggerPickStyle = SO_GET_PART(dragger, "pickStyle", SoPickStyle);

    ui->pickTransformOriginButton->setText(tr("Pick Reference"));
    ui->alignToOtherObjectButton->setText(tr("Move to Other Object"));

    switch (mode) {
        case SelectionMode::SelectTransformOrigin:
            draggerPickStyle->style = SoPickStyle::UNPICKABLE;
            draggerPickStyle->setOverride(true);
            blockSelection(false);
            ui->referenceLineEdit->setText(tr("Select face, edge, or vertex…"));
            ui->pickTransformOriginButton->setText(tr("Cancel"));
            break;

        case SelectionMode::SelectAlignTarget:
            draggerPickStyle->style = SoPickStyle::UNPICKABLE;
            draggerPickStyle->setOverride(true);
            ui->alignToOtherObjectButton->setText(tr("Cancel"));
            blockSelection(false);
            break;

        case SelectionMode::None:
            draggerPickStyle->style = SoPickStyle::SHAPE_ON_TOP;
            draggerPickStyle->setOverride(false);
            blockSelection(true);

            vp->setTransformOrigin(vp->getTransformOrigin());

            break;
    }

    selectionMode = mode;

    updateSpinBoxesReadOnlyStatus();
}

TaskTransform::SelectionMode TaskTransform::getSelectionMode() const
{
    return selectionMode;
}

TaskTransform::CoordinateSystem TaskTransform::localCoordinateSystem() const
{
    auto origin = referencePlacement;
    origin.setRotation(vp->getDraggerPlacement().getRotation());

    return {{"U", "V", "W"}, origin};
}

TaskTransform::CoordinateSystem TaskTransform::globalCoordinateSystem() const
{
    return {{"X", "Y", "Z"}, globalOrigin};
}

TaskTransform::CoordinateSystem TaskTransform::currentCoordinateSystem() const
{
    return ui->positionModeComboBox->currentIndex() == 0 ? localCoordinateSystem()
                                                         : globalCoordinateSystem();
}

Base::Rotation::EulerSequence TaskTransform::eulerSequence() const
{
    return positionMode == PositionMode::Local ? Base::Rotation::Intrinsic_XYZ
                                               : Base::Rotation::Extrinsic_XYZ;
}

void TaskTransform::onSelectionChanged(const SelectionChanges& msg)
{
    const auto isSupportedMessage =
        msg.Type == SelectionChanges::AddSelection || msg.Type == SelectionChanges::SetPreselect;

    if (!isSupportedMessage) {
        return;
    }

    if (!subObjectPlacementProvider) {
        return;
    }

    if (!msg.pOriginalMsg) {
        // this should not happen! Original should contain unresolved message.
        return;
    }

    auto doc = Application::Instance->getDocument(msg.pDocName);
    auto obj = doc->getDocument()->getObject(msg.pObjectName);

    auto orgDoc = Application::Instance->getDocument(msg.pOriginalMsg->pDocName);
    auto orgObj = orgDoc->getDocument()->getObject(msg.pOriginalMsg->pObjectName);

    auto globalPlacement = App::GeoFeature::getGlobalPlacement(obj, orgObj, msg.pOriginalMsg->pSubName);
    auto localPlacement = App::GeoFeature::getPlacementFromProp(obj, "Placement");
    auto rootPlacement = App::GeoFeature::getGlobalPlacement(vp->getObject());
    auto attachedPlacement = subObjectPlacementProvider->calculate(msg.Object, localPlacement);

    auto selectedObjectPlacement = rootPlacement.inverse() * globalPlacement * attachedPlacement;

    auto label = QStringLiteral("%1#%2.%3")
                     .arg(QLatin1String(msg.pOriginalMsg->pObjectName),
                          QLatin1String(msg.pObjectName),
                          QLatin1String(msg.pSubName));

    switch (selectionMode) {
        case SelectionMode::SelectTransformOrigin: {
            if (msg.Type == SelectionChanges::AddSelection) {
                ui->referenceLineEdit->setText(label);
                customTransformOrigin = selectedObjectPlacement;
                updateTransformOrigin();
                setSelectionMode(SelectionMode::None);
            } else {
                vp->setTransformOrigin(selectedObjectPlacement);
            }

            break;
        }

        case SelectionMode::SelectAlignTarget: {
            vp->setDraggerPlacement(vp->getObjectPlacement() * selectedObjectPlacement);

            if (msg.Type == SelectionChanges::AddSelection) {
                moveObjectToDragger(getRelevantComponents());

                setSelectionMode(SelectionMode::None);
            }

            break;
        }

        default:
            // no-op
            break;
    }
}

void TaskTransform::onAlignRotationChanged()
{
    updateDraggerLabels();
    updateTransformOrigin();
}

void TaskTransform::onAlignToOtherObject()
{
    if (selectionMode == SelectionMode::SelectAlignTarget) {
        setSelectionMode(SelectionMode::None);
        return;
    }

    setSelectionMode(SelectionMode::SelectAlignTarget);
}

ViewProviderDragger::DraggerComponents TaskTransform::getRelevantComponents()
{
    // Check which dragger components should be considered
    ViewProviderDragger::DraggerComponents components;

    if (ui->matchXcheckbox->isChecked()) {
        components |= ViewProviderDragger::DraggerComponent::XPos;
    }
    if (ui->matchYcheckbox->isChecked()) {
        components |= ViewProviderDragger::DraggerComponent::YPos;
    }
    if (ui->matchZcheckbox->isChecked()) {
        components |= ViewProviderDragger::DraggerComponent::ZPos;
    }
    if (ui->alignXcheckbox->isChecked()) {
        components |= ViewProviderDragger::DraggerComponent::XRot;
    }
    if (ui->alignYcheckbox->isChecked()) {
        components |= ViewProviderDragger::DraggerComponent::YRot;
    }
    if (ui->alignZcheckbox->isChecked()) {
        components |= ViewProviderDragger::DraggerComponent::ZRot;
    }
    if (!ui->translateCheckbox->isChecked()) {
        components &= ~ViewProviderDragger::DraggerComponent::XPos;
        components &= ~ViewProviderDragger::DraggerComponent::YPos;
        components &= ~ViewProviderDragger::DraggerComponent::ZPos;
    }
    if (!ui->rotateCheckbox->isChecked()) {
        components &= ~ViewProviderDragger::DraggerComponent::XRot;
        components &= ~ViewProviderDragger::DraggerComponent::YRot;
        components &= ~ViewProviderDragger::DraggerComponent::ZRot;
    }

    return components;
}

void TaskTransform::moveObjectToDragger(ViewProviderDragger::DraggerComponents components)
{
    vp->updateTransformFromDragger();
    vp->updatePlacementFromDragger(components);

    resetReferenceRotation();
    resetReferencePlacement();

    updatePositionAndRotationUi();
}

void TaskTransform::onFlip()
{
    auto placement = vp->getDraggerPlacement();

    placement.setRotation(placement.getRotation()
                          * Base::Rotation::fromNormalVector(Base::Vector3d(0, 0, -1)));

    vp->setDraggerPlacement(placement);

    moveObjectToDragger();
}

void TaskTransform::onPickTransformOrigin()
{
    setSelectionMode(selectionMode == SelectionMode::None ? SelectionMode::SelectTransformOrigin
                                                          : SelectionMode::None);
}

void TaskTransform::onPlacementModeChange([[maybe_unused]] int index)
{
    placementMode = ui->placementComboBox->currentData().value<PlacementMode>();

    updateTransformOrigin();
}

void TaskTransform::updateTransformOrigin()
{
    auto getTransformOrigin = [this](const PlacementMode& mode) -> Base::Placement {
        switch (mode) {
            case PlacementMode::ObjectOrigin:
                return {};
            case PlacementMode::Centroid:
                if (const auto com = centerOfMassProvider->ofDocumentObject(vp->getObject())) {
                    return {*com, {}};
                }
                return {};
            case PlacementMode::Custom:
                return customTransformOrigin.value_or(Base::Placement {});
            default:
                return {};
        }
    };

    ui->referencePickerWidget->setVisible(placementMode == PlacementMode::Custom);

    if (placementMode == PlacementMode::Custom && !customTransformOrigin.has_value()) {
        setSelectionMode(SelectionMode::SelectTransformOrigin);
        return;
    }

    auto transformOrigin = getTransformOrigin(placementMode);
    if (isDraggerAlignedToCoordinateSystem()) {
        transformOrigin.setRotation(
            (vp->getObjectPlacement().inverse() * globalCoordinateSystem().origin).getRotation());
    }

    vp->setTransformOrigin(transformOrigin);

    resetReferencePlacement();
    resetReferenceRotation();

    updatePositionAndRotationUi();
    updateDraggerLabels();
}

void TaskTransform::updateSpinBoxesReadOnlyStatus() const
{
    const bool isReadOnly = selectionMode != SelectionMode::None;

    const auto controls = {
        ui->xPositionSpinBox,
        ui->yPositionSpinBox,
        ui->zPositionSpinBox,
        ui->xRotationSpinBox,
        ui->yRotationSpinBox,
        ui->zRotationSpinBox,
    };

    for (const auto& control : controls) {
        control->setReadOnly(isReadOnly);
    }
}

void TaskTransform::resetReferencePlacement()
{
    referencePlacement = vp->getDraggerPlacement();
}

void TaskTransform::resetReferenceRotation()
{
    referenceRotation = vp->getDraggerPlacement().getRotation();
}

bool TaskTransform::isDraggerAlignedToCoordinateSystem() const
{
    return positionMode == PositionMode::Global && ui->alignRotationCheckBox->isChecked();
}

void TaskTransform::onTransformOriginReset()
{
    vp->resetTransformOrigin();
}

void TaskTransform::onCoordinateSystemChange([[maybe_unused]] int mode)
{
    positionMode = ui->positionModeComboBox->currentData().value<PositionMode>();

    ui->alignRotationCheckBox->setVisible(positionMode != PositionMode::Local);

    updateInputLabels();
    updatePositionAndRotationUi();
    updateTransformOrigin();
}

void TaskTransform::onPositionChange()
{
    const auto uvwPosition = Base::Vector3d(ui->xPositionSpinBox->rawValue(),
                                            ui->yPositionSpinBox->rawValue(),
                                            ui->zPositionSpinBox->rawValue());

    const auto xyzPosition = currentCoordinateSystem().origin.getPosition()
        + currentCoordinateSystem().origin.getRotation().multVec(uvwPosition);

    const auto placement = vp->getDraggerPlacement();

    vp->setDraggerPlacement({xyzPosition, placement.getRotation()});

    vp->updateTransformFromDragger();
    vp->updatePlacementFromDragger();
}

void TaskTransform::onRotationChange(QuantitySpinBox* changed)
{
    if (positionMode == PositionMode::Local) {
        for (auto rotationSpinBox : {ui->xRotationSpinBox,
                                     ui->yRotationSpinBox,
                                     ui->zRotationSpinBox}) {
            QSignalBlocker blocker(rotationSpinBox);

            // if any other spinbox contains non-zero value we need to reset rotation reference first
            if (std::fabs(rotationSpinBox->rawValue()) > tolerance && rotationSpinBox != changed) {
                resetReferenceRotation();
                rotationSpinBox->setValue(0.0);
            }
        }
    }

    const auto uvwRotation = Base::Rotation::fromEulerAngles(eulerSequence(),
                                                             ui->xRotationSpinBox->rawValue(),
                                                             ui->yRotationSpinBox->rawValue(),
                                                             ui->zRotationSpinBox->rawValue());

    auto referenceRotation = positionMode == PositionMode::Local
        ? this->referenceRotation
        : currentCoordinateSystem().origin.getRotation();

    const auto xyzRotation = referenceRotation * uvwRotation;

    const auto placement = vp->getDraggerPlacement();

    vp->setDraggerPlacement({placement.getPosition(), xyzRotation});

    vp->updateTransformFromDragger();
    vp->updatePlacementFromDragger();

    resetReferencePlacement();
}

TaskTransformDialog::TaskTransformDialog(ViewProviderDragger* vp, SoTransformDragger* dragger)
    : vp(vp)
{
    transform = new TaskTransform(vp, dragger);
    Content.push_back(transform);
}

void TaskTransformDialog::open()
{
    // we can't have user switching camera types while dragger is shown.
    Gui::Application::Instance->commandManager()
        .getCommandByName("Std_OrthographicCamera")
        ->setEnabled(false);

    Gui::Application::Instance->commandManager()
        .getCommandByName("Std_PerspectiveCamera")
        ->setEnabled(false);

    Gui::TaskView::TaskDialog::open();

    openCommand();
}

void TaskTransformDialog::openCommand()
{
    if (auto document = vp->getDocument()) {
        if (!document->hasPendingCommand()) {
            document->openCommand(QT_TRANSLATE_NOOP("Command", "Transform"));
        }
    }
}

void TaskTransformDialog::updateDraggerPlacement()
{
    const auto placement = vp->getObjectPlacement();
    vp->setDraggerPlacement(placement);
}

void TaskTransformDialog::onUndo()
{
    updateDraggerPlacement();
    openCommand();
}

void TaskTransformDialog::onRedo()
{
    updateDraggerPlacement();
    openCommand();
}

bool TaskTransformDialog::accept()
{
    if (auto document = vp->getDocument()) {
        document->commitCommand();
        document->resetEdit();
        document->getDocument()->recompute();
    }

    return Gui::TaskView::TaskDialog::accept();
}

bool TaskTransformDialog::reject()
{
    if (auto document = vp->getDocument()) {
        document->abortCommand();
        document->resetEdit();
        document->getDocument()->recompute();
    }

    return Gui::TaskView::TaskDialog::reject();
}

#include "moc_TaskTransform.cpp"
