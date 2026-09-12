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

#include <cassert>
#include <algorithm>
#include <cmath>
#include <string_view>
#include <QAbstractItemView>
#include <QApplication>

#include <View3DInventorViewer.h>
#include <Utilities.h>

#include <App/Document.h>
#include <App/GeoFeature.h>
#include <App/Services.h>
#include <Base/Exception.h>
#include <Base/Precision.h>
#include <Base/ServiceProvider.h>
#include <Base/Tools.h>

#include "Document.h"  // must be before TaskTransform.h
#include "Application.h"
#include "BitmapFactory.h"
#include "Command.h"
#include "Inventor/Draggers/SoTransformDragger.h"
#include "MainWindow.h"
#include "QuantitySpinBox.h"
#include "ViewProviderDragger.h"
#include "TaskView/TaskView.h"

#include "TaskTransform.h"
#include "TransformSnap.h"
#include "ui_TaskTransform.h"

#include "Inventor/SoFCPlacementIndicatorKit.h"
#include "Inventor/So3DAnnotation.h"
#include "View3DInventor.h"

#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoGroup.h>

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

constexpr std::array<const char*, 3> customCoordinateSystemLabels {"X′", "Y′", "Z′"};

QString linkedSelectionLabel(const SelectionChanges& msg)
{
    if (!msg.pOriginalMsg) {
        return QStringLiteral("%1.%2").arg(QLatin1String(msg.pObjectName), QLatin1String(msg.pSubName));
    }

    return QStringLiteral("%1#%2.%3")
        .arg(
            QLatin1String(msg.pOriginalMsg->pObjectName),
            QLatin1String(msg.pObjectName),
            QLatin1String(msg.pSubName)
        );
}

QString snapTypeLabel(App::SubObjectPlacementProvider::SnapGeometryType type)
{
    using SnapGeometryType = App::SubObjectPlacementProvider::SnapGeometryType;

    switch (type) {
        case SnapGeometryType::Point:
            return QCoreApplication::translate("Gui::TaskTransform", "Point");
        case SnapGeometryType::Axis:
            return QCoreApplication::translate("Gui::TaskTransform", "Axis");
        case SnapGeometryType::Plane:
            return QCoreApplication::translate("Gui::TaskTransform", "Plane");
        case SnapGeometryType::AxisSystem:
            return QCoreApplication::translate("Gui::TaskTransform", "Axis System");
        case SnapGeometryType::Unknown:
            return QCoreApplication::translate("Gui::TaskTransform", "Reference");
    }

    return QCoreApplication::translate("Gui::TaskTransform", "Reference");
}

bool isPlaneSnap(
    App::SubObjectPlacementProvider::SnapGeometryType referenceType,
    App::SubObjectPlacementProvider::SnapGeometryType targetType
)
{
    using SnapGeometryType = App::SubObjectPlacementProvider::SnapGeometryType;

    return (referenceType == SnapGeometryType::Plane && targetType == SnapGeometryType::Plane)
        || (referenceType == SnapGeometryType::AxisSystem && targetType == SnapGeometryType::Plane);
}

}  // namespace

TaskTransform::TaskTransform(
    Gui::ViewProviderDragger* vp,
    Gui::SoTransformDragger* dragger,
    QWidget* parent,
    App::SubObjectPlacementProvider* subObjectPlacementProvider,
    App::CenterOfMassProvider* centerOfMassProvider
)
    : QWidget(parent)
    , vp(vp)
    , subObjectPlacementProvider(subObjectPlacementProvider)
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

    globalOrigin = vp->getObjectPlacement()
        * App::GeoFeature::getGlobalPlacement(vp->getObject()).inverse();

    setupGui();
}

TaskTransform::~TaskTransform()
{
    hideCoordinateSystemIndicator();

    Gui::Application::Instance->commandManager()
        .getCommandByName("Std_OrthographicCamera")
        ->setEnabled(true);

    Gui::Application::Instance->commandManager()
        .getCommandByName("Std_PerspectiveCamera")
        ->setEnabled(true);

    savePreferences();
    dragger->removeStartCallback(dragStartCallback, this);
    dragger->removeMotionCallback(dragMotionCallback, this);
    delete ui;
}

void TaskTransform::dragStartCallback([[maybe_unused]] void* data, [[maybe_unused]] SoDragger* dragger)
{
    // This is called when a manipulator is about to manipulating
    if (firstDrag) {
        Gui::Application::Instance->activeDocument()->openCommand(
            QT_TRANSLATE_NOOP("Command", "Transform")
        );
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

    ui->placementComboBox->addItem(
        tr("Object origin"),
        QVariant::fromValue(PlacementMode::ObjectOrigin)
    );

    if (centerOfMassProvider->supports(vp->getObject())) {
        ui->placementComboBox->addItem(
            tr("Center of mass / centroid"),
            QVariant::fromValue(PlacementMode::Centroid)
        );
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
    if (subObjectPlacementProvider) {
        ui->positionModeComboBox->addItem(tr("Custom"), QVariant::fromValue(PositionMode::Custom));
    }
}

void TaskTransform::setupGui()
{
    ui->setupUi(this);

    coordinatesWidget = new QWidget(this);
    coordinatesWidget->setWindowTitle(tr("Transforms"));
    auto* coordinatesLayout = new QVBoxLayout(coordinatesWidget);
    coordinatesLayout->setContentsMargins(0, 0, 0, 0);
    coordinatesLayout->setSpacing(6);
    coordinatesLayout->addWidget(ui->coordinateSystemWidget);
    coordinatesLayout->addWidget(ui->alignRotationCheckBox);
    coordinatesLayout->addWidget(ui->positionGroupBox);
    coordinatesLayout->addWidget(ui->rotationGroupBox);

    loadPlacementModeItems();
    loadPositionModeItems();

    ui->referenceLabel->hide();
    ui->referencePickerWidget->hide();
    ui->customCSReferenceLabel->hide();
    ui->customCSPickerWidget->hide();
    ui->alignRotationCheckBox->hide();
    ui->cumulativeSnapHistoryList->setSelectionMode(QAbstractItemView::NoSelection);
    ui->cumulativeSnapHistoryList->setFocusPolicy(Qt::NoFocus);

    for (auto positionSpinBox :
         {ui->translationIncrementSpinBox,
          ui->xPositionSpinBox,
          ui->yPositionSpinBox,
          ui->zPositionSpinBox}) {
        positionSpinBox->setUnit(Base::Unit::Length);
    }

    for (auto rotationSpinBox :
         {ui->rotationIncrementSpinBox,
          ui->xRotationSpinBox,
          ui->yRotationSpinBox,
          ui->zRotationSpinBox}) {
        rotationSpinBox->setUnit(Base::Unit::Angle);
    }

    connect(
        ui->translationIncrementSpinBox,
        qOverload<double>(&QuantitySpinBox::valueChanged),
        this,
        [this](double) { updateIncrements(); }
    );
    connect(
        ui->rotationIncrementSpinBox,
        qOverload<double>(&QuantitySpinBox::valueChanged),
        this,
        [this](double) { updateIncrements(); }
    );
    connect(
        ui->positionModeComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged),
        this,
        &TaskTransform::onCoordinateSystemChange
    );
    connect(
        ui->placementComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged),
        this,
        &TaskTransform::onPlacementModeChange
    );
    connect(
        ui->pickTransformOriginButton,
        &QPushButton::clicked,
        this,
        &TaskTransform::onPickTransformOrigin
    );
    connect(
        ui->pickCoordinateSystemReferenceButton,
        &QPushButton::clicked,
        this,
        &TaskTransform::onPickCoordinateSystemReference
    );
    connect(ui->alignToOtherObjectButton, &QPushButton::clicked, this, &TaskTransform::onAlignToOtherObject);
    connect(ui->cumulativeSnapButton, &QPushButton::clicked, this, &TaskTransform::onCumulativeSnap);
    connect(ui->undoCumulativeSnapButton, &QPushButton::clicked, this, &TaskTransform::onUndoCumulativeSnap);
    connect(
        ui->clearCumulativeSnapButton,
        &QPushButton::clicked,
        this,
        &TaskTransform::onClearCumulativeSnap
    );
    connect(
        ui->invertCumulativeSnapUButton,
        &QPushButton::clicked,
        this,
        &TaskTransform::onInvertCumulativeSnapU
    );
    connect(
        ui->invertCumulativeSnapVButton,
        &QPushButton::clicked,
        this,
        &TaskTransform::onInvertCumulativeSnapV
    );
    connect(ui->moveOptionsButton, &QPushButton::toggled, ui->frameMoveOptions, &QWidget::setVisible);
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

    connect(ui->alignRotationCheckBox, &QCheckBox::clicked, this, &TaskTransform::onAlignRotationChanged);

    for (auto positionSpinBox : {ui->xPositionSpinBox, ui->yPositionSpinBox, ui->zPositionSpinBox}) {
        connect(positionSpinBox, qOverload<double>(&QuantitySpinBox::valueChanged), this, [this](double) {
            onPositionChange();
        });
    }

    for (auto rotationSpinBox : {ui->xRotationSpinBox, ui->yRotationSpinBox, ui->zRotationSpinBox}) {
        connect(
            rotationSpinBox,
            qOverload<double>(&QuantitySpinBox::valueChanged),
            this,
            [this, rotationSpinBox](double) { onRotationChange(rotationSpinBox); }
        );
    }

    alignGridLayoutColumns(
        {ui->absolutePositionLayout,
         ui->absoluteRotationLayout,
         ui->transformOriginLayout,
         ui->coordinateSystemLayout}
    );

    loadPreferences();

    updateInputLabels();
    updateDraggerLabels();
    updateIncrements();
    updatePositionAndRotationUi();
    updateCumulativeSnapUi();
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

    setPositionValues(
        uvwPlacement.getPosition(),
        ui->xPositionSpinBox,
        ui->yPositionSpinBox,
        ui->zPositionSpinBox
    );

    setRotationValues(
        positionMode == PositionMode::Local ? referenceRotation.inverse() * xyzPlacement.getRotation()
                                            : uvwPlacement.getRotation(),
        ui->xRotationSpinBox,
        ui->yRotationSpinBox,
        ui->zRotationSpinBox
    );
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
    auto coordinateSystem = isDraggerAlignedToCoordinateSystem() ? currentCoordinateSystem()
                                                                 : localCoordinateSystem();

    auto [xLabel, yLabel, zLabel] = coordinateSystem.labels;

    dragger->xAxisLabel.setValue(xLabel.c_str());
    dragger->yAxisLabel.setValue(yLabel.c_str());
    dragger->zAxisLabel.setValue(zLabel.c_str());
}

void TaskTransform::updateIncrements() const
{
    dragger->translationIncrement.setValue(
        std::max(ui->translationIncrementSpinBox->rawValue(), 0.001)
    );
    dragger->rotationIncrement.setValue(
        Base::toRadians(std::max(ui->rotationIncrementSpinBox->rawValue(), 0.01))
    );
}

void TaskTransform::setSelectionMode(SelectionMode mode)
{
    Gui::Selection().clearSelection();

    SoPickStyle* draggerPickStyle = SO_GET_PART(dragger, "pickStyle", SoPickStyle);

    ui->pickTransformOriginButton->setText(tr("Pick Reference"));
    ui->alignToOtherObjectButton->setText(tr("Move to Other Object"));
    ui->pickCoordinateSystemReferenceButton->setText(tr("Pick Reference"));

    switch (mode) {
        case SelectionMode::SelectTransformOrigin:
            draggerPickStyle->style = SoPickStyle::UNPICKABLE;
            draggerPickStyle->setOverride(true);
            blockSelection(false);
            ui->referenceLineEdit->setText(tr("Select object, face, edge…"));
            ui->pickTransformOriginButton->setText(tr("Cancel"));
            break;

        case SelectionMode::SelectAlignTarget:
            draggerPickStyle->style = SoPickStyle::UNPICKABLE;
            draggerPickStyle->setOverride(true);
            ui->alignToOtherObjectButton->setText(tr("Cancel"));
            blockSelection(false);
            break;

        case SelectionMode::SelectCustomCS:
            draggerPickStyle->style = SoPickStyle::UNPICKABLE;
            draggerPickStyle->setOverride(true);
            blockSelection(false);
            ui->customCSLineEdit->setText(tr("Select object, face, edge…"));
            ui->pickCoordinateSystemReferenceButton->setText(tr("Cancel"));
            break;

        case SelectionMode::SelectCumulativeSnapReference:
            draggerPickStyle->style = SoPickStyle::UNPICKABLE;
            draggerPickStyle->setOverride(true);
            getMainWindow()->showMessage(tr("Select reference geometry on the transformed object"));
            blockSelection(false);
            break;

        case SelectionMode::SelectCumulativeSnapTarget:
            draggerPickStyle->style = SoPickStyle::UNPICKABLE;
            draggerPickStyle->setOverride(true);
            getMainWindow()->showMessage(tr("Select target geometry on another object"));
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

TaskTransform::CoordinateSystem TaskTransform::customCoordinateSystem() const
{
    if (!customCoordinateSystemPlacement.has_value()) {
        return globalCoordinateSystem();
    }
    auto [xLabel, yLabel, zLabel] = customCoordinateSystemLabels;
    return {{xLabel, yLabel, zLabel}, globalOrigin * (*customCoordinateSystemPlacement)};
}

TaskTransform::CoordinateSystem TaskTransform::currentCoordinateSystem() const
{
    switch (positionMode) {
        case PositionMode::Local:
            return localCoordinateSystem();
        case PositionMode::Global:
            return globalCoordinateSystem();
        case PositionMode::Custom:
            return customCoordinateSystem();
    }
    return localCoordinateSystem();
}

Base::Rotation::EulerSequence TaskTransform::eulerSequence() const
{
    return positionMode == PositionMode::Local ? Base::Rotation::Intrinsic_XYZ
                                               : Base::Rotation::Extrinsic_XYZ;
}

void TaskTransform::onSelectionChanged(const SelectionChanges& msg)
{
    const auto isSupportedMessage = msg.Type == SelectionChanges::AddSelection
        || msg.Type == SelectionChanges::SetPreselect;

    if (!isSupportedMessage) {
        return;
    }

    if (selectionMode == SelectionMode::SelectCustomCS && msg.Type == SelectionChanges::AddSelection) {
        setCustomCoordinateSystemFromSelection(msg);
        return;
    }

    const bool isCumulativeSnapSelection = selectionMode == SelectionMode::SelectCumulativeSnapReference
        || selectionMode == SelectionMode::SelectCumulativeSnapTarget;

    if (selectionMode != SelectionMode::SelectTransformOrigin
        && selectionMode != SelectionMode::SelectAlignTarget && !isCumulativeSnapSelection) {
        return;
    }

    if (isCumulativeSnapSelection && !subObjectPlacementProvider) {
        return;
    }

    auto reference = referencePlacementFromSelection(
        msg,
        ReferencePlacementOption::UseSubObjectPlacement
            | (selectionMode == SelectionMode::SelectTransformOrigin || isCumulativeSnapSelection
                   ? ReferencePlacementOption::UseSnapPosition
                   : ReferencePlacementOption::None)
    );
    if (!reference) {
        return;
    }

    auto geometryType = App::SubObjectPlacementProvider::SnapGeometryType::Unknown;
    bool isMovingObjectSelection = false;
    auto snapObjectPlacement = reference->objectPlacement;
    if (isCumulativeSnapSelection) {
        auto doc = Application::Instance->getDocument(msg.pDocName);
        auto obj = doc->getDocument()->getObject(msg.pObjectName);
        auto orgObj = obj;
        std::string orgSubName;
        if (msg.pOriginalMsg) {
            auto orgDoc = Application::Instance->getDocument(msg.pOriginalMsg->pDocName);
            orgObj = orgDoc->getDocument()->getObject(msg.pOriginalMsg->pObjectName);
            orgSubName = msg.pOriginalMsg->pSubName;
        }

        geometryType = snapGeometryType(msg);
        isMovingObjectSelection = isCumulativeSnapMovingObjectSelection(msg, obj, orgObj);
        auto localPlacement = App::GeoFeature::getPlacementFromProp(obj, "Placement");
        if (auto placement = subObjectPlacementProvider->snapPlacement(msg.Object, localPlacement)) {
            auto globalPlacement = msg.pOriginalMsg
                ? App::GeoFeature::getGlobalPlacement(obj, orgObj, orgSubName)
                : App::GeoFeature::getGlobalPlacement(obj);
            auto rootPlacement = App::GeoFeature::getGlobalPlacement(vp->getObject());
            snapObjectPlacement = rootPlacement.inverse() * globalPlacement * *placement;
        }
    }

    switch (selectionMode) {
        case SelectionMode::SelectTransformOrigin: {
            if (msg.Type == SelectionChanges::AddSelection) {
                ui->referenceLineEdit->setText(reference->label);
                customTransformOrigin = reference->objectPlacement;
                updateTransformOrigin();
                setSelectionMode(SelectionMode::None);
            }
            else {
                vp->setTransformOrigin(reference->objectPlacement);
            }

            break;
        }

        case SelectionMode::SelectAlignTarget: {
            vp->setDraggerPlacement(vp->getObjectPlacement() * reference->objectPlacement);

            if (msg.Type == SelectionChanges::AddSelection) {
                moveObjectToDragger(getRelevantComponents());

                setSelectionMode(SelectionMode::None);
            }

            break;
        }

        case SelectionMode::SelectCumulativeSnapReference: {
            if (geometryType == App::SubObjectPlacementProvider::SnapGeometryType::Unknown) {
                if (msg.Type == SelectionChanges::AddSelection) {
                    Gui::Selection().clearSelection();
                    getMainWindow()->showMessage(
                        tr("Select point, axis, plane, or axis system reference geometry")
                    );
                }
                break;
            }

            if (!isMovingObjectSelection) {
                if (msg.Type == SelectionChanges::AddSelection) {
                    Gui::Selection().clearSelection();
                    getMainWindow()->showMessage(tr("Select a reference on the transformed object"));
                }
                break;
            }

            vp->setTransformOrigin(snapObjectPlacement);
            if (msg.Type == SelectionChanges::AddSelection) {
                currentCumulativeSnapReference = CumulativeSnapReference {
                    reference->label.toStdString(),
                    snapObjectPlacement,
                    geometryType,
                };
                setSelectionMode(SelectionMode::SelectCumulativeSnapTarget);
                updateCumulativeSnapUi();
            }

            break;
        }

        case SelectionMode::SelectCumulativeSnapTarget: {
            if (!currentCumulativeSnapReference) {
                setSelectionMode(SelectionMode::SelectCumulativeSnapReference);
                break;
            }

            if (isMovingObjectSelection) {
                if (msg.Type == SelectionChanges::AddSelection) {
                    Gui::Selection().clearSelection();
                    getMainWindow()->showMessage(tr("Select a target on a different object"));
                }
                break;
            }

            if (!TransformSnap::isCompatible(currentCumulativeSnapReference->type, geometryType)) {
                if (msg.Type == SelectionChanges::AddSelection) {
                    Gui::Selection().clearSelection();
                    getMainWindow()->showMessage(
                        tr("Select target geometry with a compatible snap type")
                    );
                }
                break;
            }

            const auto referenceType = currentCumulativeSnapReference->type;
            const auto targetType = geometryType;
            const auto targetReferencePlacement = vp->getObjectPlacement() * snapObjectPlacement;
            const auto candidateObjectPlacement = TransformSnap::preferredPlacement(
                vp->getObjectPlacement(),
                currentCumulativeSnapReference->localPlacement,
                targetReferencePlacement,
                targetType
            );

            const TransformSnap::Constraint constraint {
                currentCumulativeSnapReference->localPlacement,
                targetReferencePlacement,
                referenceType,
                targetType,
            };
            const auto constrainedObjectPlacement
                = solveCumulativeSnapObjectPlacement(candidateObjectPlacement, constraint);
            if (!constrainedObjectPlacement) {
                if (msg.Type == SelectionChanges::AddSelection) {
                    Gui::Selection().clearSelection();
                    getMainWindow()->showMessage(tr("Unable to compute a valid snap placement"));
                }
                vp->setDraggerPlacement(
                    vp->getObjectPlacement() * currentCumulativeSnapReference->localPlacement
                );
                vp->updateTransformFromDragger();
                break;
            }

            vp->setTransformOrigin(currentCumulativeSnapReference->localPlacement);
            vp->setDraggerPlacement(
                *constrainedObjectPlacement * currentCumulativeSnapReference->localPlacement
            );

            if (msg.Type == SelectionChanges::AddSelection) {
                restoreCumulativeSnapPlacement(*constrainedObjectPlacement);
                appendCumulativeSnapStep(
                    QString::fromStdString(currentCumulativeSnapReference->label),
                    reference->label,
                    constraint
                );
                currentCumulativeSnapReference.reset();
                setSelectionMode(SelectionMode::SelectCumulativeSnapReference);
            }

            break;
        }

        default:
            // no-op
            break;
    }
}

std::optional<TaskTransform::ReferencePlacement> TaskTransform::referencePlacementFromSelection(
    const SelectionChanges& msg,
    ReferencePlacementOptions options
) const
{
    const auto useSubObjectPlacement = options.testFlag(
        ReferencePlacementOption::UseSubObjectPlacement
    );
    const auto useSnapPosition = options.testFlag(ReferencePlacementOption::UseSnapPosition);

    auto doc = Application::Instance->getDocument(msg.pDocName);
    if (!doc) {
        return std::nullopt;
    }

    auto obj = doc->getDocument()->getObject(msg.pObjectName);
    if (!obj) {
        return std::nullopt;
    }

    auto rootPlacement = App::GeoFeature::getGlobalPlacement(vp->getObject());
    Base::Placement documentPlacement;
    QString label;

    if (msg.pOriginalMsg) {
        if (useSubObjectPlacement && !subObjectPlacementProvider) {
            return std::nullopt;
        }

        auto orgDoc = Application::Instance->getDocument(msg.pOriginalMsg->pDocName);
        if (!orgDoc) {
            return std::nullopt;
        }

        auto orgObj = orgDoc->getDocument()->getObject(msg.pOriginalMsg->pObjectName);
        if (!orgObj) {
            return std::nullopt;
        }

        auto globalPlacement
            = App::GeoFeature::getGlobalPlacement(obj, orgObj, msg.pOriginalMsg->pSubName);
        auto localPlacement = App::GeoFeature::getPlacementFromProp(obj, "Placement");

        documentPlacement = globalPlacement;
        if (useSubObjectPlacement) {
            try {
                documentPlacement = globalPlacement
                    * subObjectPlacementProvider->calculate(msg.Object, localPlacement);
            }
            catch (const Base::Exception&) {
                // Shape type unsupported for attacher (e.g. Solid): fall back to the
                // link-aware placement already resolved above, preserving link-instance transforms.
                documentPlacement = globalPlacement;
            }
        }

        auto objectPlacement = rootPlacement.inverse() * documentPlacement;
        if (useSnapPosition) {
            std::optional<Base::Vector3d> worldCursor;
            if (msg.hasPickedPoint) {
                worldCursor = Base::Vector3d(msg.x, msg.y, msg.z);
            }
            if (auto snapPos = subObjectPlacementProvider->snapPosition(
                    msg.Object,
                    worldCursor,
                    globalPlacement.toMatrix()
                )) {
                Base::Vector3d rootLocalSnapPos;
                rootPlacement.inverse().toMatrix().multVec(*snapPos, rootLocalSnapPos);
                objectPlacement.setPosition(rootLocalSnapPos);
                documentPlacement = rootPlacement * objectPlacement;
            }
        }

        label = linkedSelectionLabel(msg);
        return ReferencePlacement {documentPlacement, objectPlacement, label};
    }

    if (msg.Type == SelectionChanges::AddSelection
        && obj->getDocument() == vp->getObject()->getDocument()) {
        // Tree-view pick without link resolution: only accept same-document objects
        // to avoid treating cross-document placements as if they were in this document's space.
        documentPlacement = App::GeoFeature::getGlobalPlacement(obj);
        label = QString::fromUtf8(obj->Label.getValue());
        return ReferencePlacement {documentPlacement, rootPlacement.inverse() * documentPlacement, label};
    }

    return std::nullopt;
}

void TaskTransform::setCustomCoordinateSystemFromSelection(const SelectionChanges& msg)
{
    auto reference
        = referencePlacementFromSelection(msg, ReferencePlacementOption::UseSubObjectPlacement);
    if (!reference) {
        return;
    }

    customCoordinateSystemPlacement = reference->documentPlacement;
    ui->customCSLineEdit->setText(reference->label);
    setSelectionMode(SelectionMode::None);
    showCoordinateSystemIndicator();
    updateInputLabels();
    updatePositionAndRotationUi();
    updateTransformOrigin();
    updateDraggerLabels();
}

void TaskTransform::onAlignRotationChanged()
{
    updateDraggerLabels();
    updateTransformOrigin();
}

void TaskTransform::onAlignToOtherObject()
{
    if (cumulativeSnapActive) {
        return;
    }

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

App::SubObjectPlacementProvider::SnapGeometryType TaskTransform::snapGeometryType(
    const SelectionChanges& msg
) const
{
    if (subObjectPlacementProvider) {
        return subObjectPlacementProvider->snapGeometryType(msg.Object);
    }

    std::string elementName = msg.Object.getOldElementName();
    if (elementName.starts_with("Vertex")) {
        return App::SubObjectPlacementProvider::SnapGeometryType::Point;
    }
    if (elementName.starts_with("Edge")) {
        return App::SubObjectPlacementProvider::SnapGeometryType::Axis;
    }
    if (elementName.starts_with("Face")) {
        return App::SubObjectPlacementProvider::SnapGeometryType::Plane;
    }

    return App::SubObjectPlacementProvider::SnapGeometryType::Unknown;
}

bool TaskTransform::isCumulativeSnapMovingObjectSelection(
    const SelectionChanges& msg,
    const App::DocumentObject* object,
    const App::DocumentObject* originalObject
) const
{
    const auto* transformedObject = vp->getObject();
    if (!transformedObject) {
        return false;
    }

    auto isSameSubObjectPathOrChild = [](std::string_view selection, std::string_view parent) {
        if (parent.empty()) {
            return false;
        }
        if (selection == parent) {
            return true;
        }
        if (!selection.starts_with(parent) || selection.size() <= parent.size()) {
            return false;
        }
        if (parent.back() == '.') {
            return true;
        }
        return selection[parent.size()] == '.';
    };

    ViewProviderDocumentObject* editParentViewProvider = nullptr;
    std::string editSubname;
    if (vp->getDocument()) {
        vp->getDocument()->getInEdit(&editParentViewProvider, &editSubname);
    }
    if (editParentViewProvider && !editSubname.empty() && msg.pOriginalMsg) {
        const auto* editParentObject = editParentViewProvider->getObject();
        const auto* selectedParentObject = msg.pOriginalMsg->Object.getObject();
        return selectedParentObject == editParentObject
            && isSameSubObjectPathOrChild(msg.pOriginalMsg->pSubName, editSubname);
    }

    auto containsTransformedObject =
        [transformedObject](const std::vector<App::DocumentObject*>& subObjects) {
            return std::ranges::find(subObjects, transformedObject) != subObjects.end();
        };

    if (msg.Object.getObject() == transformedObject || object == transformedObject
        || originalObject == transformedObject) {
        return true;
    }

    if (containsTransformedObject(msg.Object.getSubObjectList())) {
        return true;
    }

    if (msg.pOriginalMsg && containsTransformedObject(msg.pOriginalMsg->Object.getSubObjectList())) {
        return true;
    }

    return false;
}

std::optional<Base::Placement> TaskTransform::solveCumulativeSnapObjectPlacement(
    const Base::Placement& candidate,
    const TransformSnap::Constraint& constraint,
    std::size_t historySize
) const
{
    std::vector<TransformSnap::Constraint> constraints;
    const auto activeHistorySize = std::min(historySize, cumulativeSnapHistory.size());
    constraints.reserve(activeHistorySize + 1);
    for (std::size_t i = 0; i < activeHistorySize; ++i) {
        constraints.push_back(cumulativeSnapHistory[i].constraint);
    }
    constraints.push_back(constraint);
    return TransformSnap::solve(candidate, constraints);
}

void TaskTransform::startCumulativeSnap()
{
    cumulativeSnapActive = true;
    cumulativeSnapStartPlacement = vp->getObjectPlacement();
    currentCumulativeSnapReference.reset();
    cumulativeSnapHistory.clear();

    setSelectionMode(SelectionMode::SelectCumulativeSnapReference);
    updateCumulativeSnapUi();
}

void TaskTransform::stopCumulativeSnap()
{
    cumulativeSnapActive = false;
    currentCumulativeSnapReference.reset();
    cumulativeSnapStartPlacement.reset();
    cumulativeSnapHistory.clear();
    setSelectionMode(SelectionMode::None);
    updateTransformOrigin();
    vp->updateTransformFromDragger();
    updateCumulativeSnapUi();
}

void TaskTransform::onDocumentRestored()
{
    const bool wasCumulativeSnapActive = cumulativeSnapActive;
    stopCumulativeSnap();
    if (wasCumulativeSnapActive) {
        getMainWindow()->showMessage(tr("Cumulative snap stopped after undo or redo"));
    }
}

std::array<QWidget*, 3> TaskTransform::taskWidgets() const
{
    return {ui->draggerWidget, coordinatesWidget, ui->utilitiesWidget};
}

void TaskTransform::appendCumulativeSnapStep(
    const QString& referenceLabel,
    const QString& targetLabel,
    TransformSnap::Constraint constraint
)
{
    const auto constraintLabel = snapTypeLabel(constraint.referenceType);
    const auto objectPlacement = vp->getObjectPlacement();
    constraint.targetDirectionSignFixed = isPlaneSnap(constraint.referenceType, constraint.targetType);
    if (constraint.targetDirectionSignFixed) {
        const auto acceptedReferencePlacement = objectPlacement * constraint.localPlacement;
        if (TransformSnap::zAxis(acceptedReferencePlacement)
                * TransformSnap::zAxis(constraint.targetPlacement)
            < 0.0) {
            constraint.targetPlacement = TransformSnap::invertedPlacementAroundLocalAxis(
                constraint.targetPlacement,
                Base::Vector3d::UnitX
            );
        }
    }

    cumulativeSnapHistory.push_back({
        QStringLiteral("%1: %2 -> %3").arg(constraintLabel, referenceLabel, targetLabel).toStdString(),
        objectPlacement,
        constraint,
    });
    updateCumulativeSnapUi();
}

void TaskTransform::restoreCumulativeSnapPlacement(const Base::Placement& placement)
{
    if (auto* property = vp->getObject()->getPlacementProperty()) {
        property->setValue(placement);
    }

    vp->setDraggerPlacement(vp->getObjectPlacement() * vp->getTransformOrigin());
    vp->updateTransformFromDragger();

    resetReferencePlacement();
    resetReferenceRotation();
    updatePositionAndRotationUi();
}

bool TaskTransform::isCumulativeSnapStepInvertible(const CumulativeSnapStep& step) const
{
    using SnapGeometryType = App::SubObjectPlacementProvider::SnapGeometryType;

    const auto& constraint = step.constraint;
    if (constraint.referenceType == constraint.targetType) {
        return constraint.referenceType == SnapGeometryType::Axis
            || constraint.referenceType == SnapGeometryType::Plane;
    }

    return constraint.referenceType == SnapGeometryType::AxisSystem
        && (constraint.targetType == SnapGeometryType::Axis
            || constraint.targetType == SnapGeometryType::Plane);
}

std::optional<std::size_t> TaskTransform::cumulativeSnapInvertTargetIndex() const
{
    if (cumulativeSnapHistory.empty()) {
        return std::nullopt;
    }

    const auto latestIndex = cumulativeSnapHistory.size() - 1;
    if (!isCumulativeSnapStepInvertible(cumulativeSnapHistory[latestIndex])) {
        return std::nullopt;
    }

    return latestIndex;
}

bool TaskTransform::canInvertCumulativeSnapDirection() const
{
    if (!cumulativeSnapActive || cumulativeSnapHistory.empty()) {
        return false;
    }

    return cumulativeSnapInvertTargetIndex().has_value();
}

void TaskTransform::invertCumulativeSnapDirection(const Base::Vector3d& localAxis)
{
    const auto targetIndex = cumulativeSnapInvertTargetIndex();
    if (!targetIndex) {
        return;
    }

    const auto previousHistory = cumulativeSnapHistory;
    const auto previousTransformOrigin = vp->getTransformOrigin();

    auto& step = cumulativeSnapHistory[*targetIndex];
    auto& constraint = step.constraint;
    constraint.targetPlacement
        = TransformSnap::invertedPlacementAroundLocalAxis(constraint.targetPlacement, localAxis);
    constraint.targetDirectionSignFixed = true;
    step.objectPlacement = TransformSnap::objectPlacementMatchingSnapFrame(
        step.objectPlacement,
        constraint.localPlacement,
        constraint.targetPlacement,
        localAxis
    );

    vp->setTransformOrigin(constraint.localPlacement);
    if (!updateCumulativeSnapHistoryPlacements()) {
        cumulativeSnapHistory = previousHistory;
        vp->setTransformOrigin(previousTransformOrigin);
        vp->setDraggerPlacement(vp->getObjectPlacement() * previousTransformOrigin);
        vp->updateTransformFromDragger();
        getMainWindow()->showMessage(tr("Unable to compute a valid snap placement"));
        return;
    }

    updateCumulativeSnapUi();

    if (selectionMode == SelectionMode::SelectCumulativeSnapTarget && currentCumulativeSnapReference) {
        vp->setTransformOrigin(currentCumulativeSnapReference->localPlacement);
        vp->setDraggerPlacement(
            vp->getObjectPlacement() * currentCumulativeSnapReference->localPlacement
        );
        vp->updateTransformFromDragger();
    }
    else {
        currentCumulativeSnapReference.reset();
        setSelectionMode(SelectionMode::SelectCumulativeSnapReference);
    }
}

bool TaskTransform::updateCumulativeSnapHistoryPlacements()
{
    auto candidate = cumulativeSnapStartPlacement.value_or(vp->getObjectPlacement());
    for (std::size_t i = 0; i < cumulativeSnapHistory.size(); ++i) {
        const auto& step = cumulativeSnapHistory[i];
        const auto preferredPlacement = TransformSnap::isFinitePlacement(step.objectPlacement)
            ? step.objectPlacement
            : candidate;
        const auto placement
            = solveCumulativeSnapObjectPlacement(preferredPlacement, step.constraint, i);
        if (!placement) {
            return false;
        }
        cumulativeSnapHistory[i].objectPlacement = *placement;
        candidate = *placement;
    }

    restoreCumulativeSnapPlacement(candidate);
    return true;
}

void TaskTransform::updateCumulativeSnapUi() const
{
    QSignalBlocker blocker(ui->cumulativeSnapButton);

    ui->cumulativeSnapButton->setChecked(cumulativeSnapActive);
    ui->cumulativeSnapButton->setText(
        cumulativeSnapActive ? tr("Stop Cumulative Snap") : tr("Cumulative Snap")
    );
    ui->alignToOtherObjectButton->setEnabled(!cumulativeSnapActive);
    ui->moveOptionsButton->setEnabled(!cumulativeSnapActive);
    ui->flipPartButton->setEnabled(!cumulativeSnapActive);
    ui->placementComboBox->setEnabled(!cumulativeSnapActive);
    ui->referencePickerWidget->setEnabled(!cumulativeSnapActive);
    coordinatesWidget->setEnabled(!cumulativeSnapActive);
    ui->cumulativeSnapButton->setEnabled(subObjectPlacementProvider != nullptr);

    ui->cumulativeSnapHistoryList->clear();
    for (std::size_t i = 0; i < cumulativeSnapHistory.size(); ++i) {
        ui->cumulativeSnapHistoryList->addItem(QStringLiteral("%1) %2").arg(i + 1).arg(
            QString::fromStdString(cumulativeSnapHistory[i].label)
        ));
    }

    ui->cumulativeSnapHistoryList->setVisible(cumulativeSnapActive);
    ui->undoCumulativeSnapButton->setVisible(cumulativeSnapActive);
    ui->clearCumulativeSnapButton->setVisible(cumulativeSnapActive);
    ui->invertCumulativeSnapUButton->setVisible(cumulativeSnapActive);
    ui->invertCumulativeSnapVButton->setVisible(cumulativeSnapActive);

    ui->undoCumulativeSnapButton->setEnabled(cumulativeSnapActive && !cumulativeSnapHistory.empty());
    ui->clearCumulativeSnapButton->setEnabled(cumulativeSnapActive && !cumulativeSnapHistory.empty());
    ui->invertCumulativeSnapUButton->setEnabled(
        cumulativeSnapActive && canInvertCumulativeSnapDirection()
    );
    ui->invertCumulativeSnapVButton->setEnabled(
        cumulativeSnapActive && canInvertCumulativeSnapDirection()
    );
}

void TaskTransform::onCumulativeSnap()
{
    if (cumulativeSnapActive) {
        stopCumulativeSnap();
    }
    else {
        startCumulativeSnap();
    }
}

void TaskTransform::onUndoCumulativeSnap()
{
    if (cumulativeSnapHistory.empty()) {
        return;
    }

    cumulativeSnapHistory.pop_back();
    const auto placement = cumulativeSnapHistory.empty()
        ? cumulativeSnapStartPlacement.value_or(vp->getObjectPlacement())
        : cumulativeSnapHistory.back().objectPlacement;

    restoreCumulativeSnapPlacement(placement);
    updateCumulativeSnapUi();

    if (cumulativeSnapActive) {
        currentCumulativeSnapReference.reset();
        setSelectionMode(SelectionMode::SelectCumulativeSnapReference);
    }
}

void TaskTransform::onClearCumulativeSnap()
{
    if (!cumulativeSnapStartPlacement.has_value()) {
        return;
    }

    cumulativeSnapHistory.clear();
    currentCumulativeSnapReference.reset();
    restoreCumulativeSnapPlacement(*cumulativeSnapStartPlacement);
    updateCumulativeSnapUi();

    if (cumulativeSnapActive) {
        setSelectionMode(SelectionMode::SelectCumulativeSnapReference);
    }
}

void TaskTransform::onInvertCumulativeSnapU()
{
    invertCumulativeSnapDirection(Base::Vector3d::UnitX);
}

void TaskTransform::onInvertCumulativeSnapV()
{
    invertCumulativeSnapDirection(Base::Vector3d::UnitY);
}

void TaskTransform::onFlip()
{
    auto placement = vp->getDraggerPlacement();

    placement.setRotation(
        placement.getRotation() * Base::Rotation::fromNormalVector(Base::Vector3d(0, 0, -1))
    );

    vp->setDraggerPlacement(placement);

    moveObjectToDragger();
}

void TaskTransform::onPickTransformOrigin()
{
    setSelectionMode(
        selectionMode == SelectionMode::None ? SelectionMode::SelectTransformOrigin
                                             : SelectionMode::None
    );
}

void TaskTransform::onPickCoordinateSystemReference()
{
    setSelectionMode(
        selectionMode == SelectionMode::SelectCustomCS ? SelectionMode::None
                                                       : SelectionMode::SelectCustomCS
    );
}

void TaskTransform::onPlacementModeChange([[maybe_unused]] int index)
{
    placementMode = ui->placementComboBox->currentData().value<PlacementMode>();
    if (placementMode != PlacementMode::Custom
        && selectionMode == SelectionMode::SelectTransformOrigin) {
        setSelectionMode(SelectionMode::None);
    }

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

    const bool showReference = (placementMode == PlacementMode::Custom);
    ui->referenceLabel->setVisible(showReference);
    ui->referencePickerWidget->setVisible(showReference);

    if (placementMode == PlacementMode::Custom && !customTransformOrigin.has_value()) {
        setSelectionMode(SelectionMode::SelectTransformOrigin);
        return;
    }

    auto transformOrigin = getTransformOrigin(placementMode);
    if (isDraggerAlignedToCoordinateSystem()) {
        transformOrigin.setRotation(
            (vp->getObjectPlacement().inverse() * currentCoordinateSystem().origin).getRotation()
        );
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
    return positionMode != PositionMode::Local && ui->alignRotationCheckBox->isChecked();
}

static SoGroup* findActiveEditingRoot(Gui::Document* doc)
{
    const auto views = doc->getMDIViewsOfType(View3DInventor::getClassTypeId());
    for (auto* mdi : views) {
        auto* view3d = static_cast<View3DInventor*>(mdi);
        View3DInventorViewer* viewer = view3d->getViewer();
        if (viewer->isEditingViewProvider()) {
            return dynamic_cast<SoGroup*>(viewer->getEditingRoot());
        }
    }
    return nullptr;
}

void TaskTransform::showCoordinateSystemIndicator()
{
    auto* editingRoot = findActiveEditingRoot(vp->getDocument());
    if (!editingRoot) {
        return;
    }

    hideCoordinateSystemIndicator();

    csIndicatorTransform = new SoTransform();

    auto* indicator = new SoFCPlacementIndicatorKit();
    if (positionMode == PositionMode::Custom) {
        auto [xLabel, yLabel, zLabel] = customCoordinateSystemLabels;
        indicator->axisLabels.set1Value(0, xLabel);
        indicator->axisLabels.set1Value(1, yLabel);
        indicator->axisLabels.set1Value(2, zLabel);
    }

    auto* annotation = new So3DAnnotation();
    annotation->addChild(csIndicatorTransform);
    annotation->addChild(indicator);

    auto* root = new SoSeparator();
    root->addChild(annotation);
    csIndicatorRoot = root;

    editingRoot->addChild(csIndicatorRoot);

    updateCoordinateSystemIndicator();
}

void TaskTransform::hideCoordinateSystemIndicator()
{
    if (!csIndicatorRoot) {
        return;
    }

    auto* editingRoot = findActiveEditingRoot(vp->getDocument());
    if (editingRoot) {
        editingRoot->removeChild(csIndicatorRoot);
    }

    csIndicatorRoot = nullptr;
    csIndicatorTransform = nullptr;
}

void TaskTransform::updateCoordinateSystemIndicator()
{
    if (!csIndicatorTransform) {
        return;
    }
    ViewProviderDragger::updateTransform(currentCoordinateSystem().origin, csIndicatorTransform);
}

void TaskTransform::onTransformOriginReset()
{
    vp->resetTransformOrigin();
}

void TaskTransform::onCoordinateSystemChange([[maybe_unused]] int mode)
{
    if (selectionMode == SelectionMode::SelectCustomCS) {
        setSelectionMode(SelectionMode::None);
    }

    positionMode = ui->positionModeComboBox->currentData().value<PositionMode>();

    ui->alignRotationCheckBox->setVisible(positionMode != PositionMode::Local);
    ui->customCSReferenceLabel->setVisible(positionMode == PositionMode::Custom);
    ui->customCSPickerWidget->setVisible(positionMode == PositionMode::Custom);

    if (positionMode == PositionMode::Local) {
        hideCoordinateSystemIndicator();
    }
    else if (
        positionMode == PositionMode::Global
        || (positionMode == PositionMode::Custom && customCoordinateSystemPlacement.has_value())
    ) {
        showCoordinateSystemIndicator();
    }

    updateInputLabels();
    updatePositionAndRotationUi();
    updateTransformOrigin();

    if (positionMode == PositionMode::Custom && !customCoordinateSystemPlacement.has_value()) {
        setSelectionMode(SelectionMode::SelectCustomCS);
    }
}

void TaskTransform::onPositionChange()
{
    const auto uvwPosition = Base::Vector3d(
        ui->xPositionSpinBox->rawValue(),
        ui->yPositionSpinBox->rawValue(),
        ui->zPositionSpinBox->rawValue()
    );

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
        for (auto rotationSpinBox :
             {ui->xRotationSpinBox, ui->yRotationSpinBox, ui->zRotationSpinBox}) {
            QSignalBlocker blocker(rotationSpinBox);

            // if any other spinbox contains non-zero value we need to reset rotation reference first
            if (std::fabs(rotationSpinBox->rawValue()) > tolerance && rotationSpinBox != changed) {
                resetReferenceRotation();
                rotationSpinBox->setValue(0.0);
            }
        }
    }

    const auto uvwRotation = Base::Rotation::fromEulerAngles(
        eulerSequence(),
        ui->xRotationSpinBox->rawValue(),
        ui->yRotationSpinBox->rawValue(),
        ui->zRotationSpinBox->rawValue()
    );

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
    , transform(std::make_unique<TaskTransform>(vp, dragger))
{
    const auto [draggerWidget, transformsWidget, utilitiesWidget] = transform->taskWidgets();
    addTaskBox(Gui::BitmapFactory().pixmap("Std_TransformManip"), draggerWidget);
    addTaskBox(Gui::BitmapFactory().pixmap("Std_CoordinateSystem"), transformsWidget);
    addTaskBox(Gui::BitmapFactory().pixmap("Std_Alignment"), utilitiesWidget);
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

void TaskTransformDialog::onUndo()
{
    transform->onDocumentRestored();
    openCommand();
}

void TaskTransformDialog::onRedo()
{
    transform->onDocumentRestored();
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
