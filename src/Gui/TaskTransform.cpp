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
#include <iterator>
#include <limits>
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

Base::Vector3d normalized(Base::Vector3d vector)
{
    if (vector.Length() < Base::Precision::Confusion()) {
        return Base::Vector3d::UnitZ;
    }

    vector.Normalize();
    return vector;
}

Base::Vector3d zAxis(const Base::Placement& placement)
{
    return normalized(placement.getRotation().multVec(Base::Vector3d::UnitZ));
}

Base::Vector3d xAxis(const Base::Placement& placement)
{
    return normalized(placement.getRotation().multVec(Base::Vector3d::UnitX));
}

Base::Vector3d yAxis(const Base::Placement& placement)
{
    return normalized(placement.getRotation().multVec(Base::Vector3d::UnitY));
}

Base::Placement invertedPlacementAroundLocalAxis(Base::Placement placement, const Base::Vector3d& localAxis)
{
    auto x = xAxis(placement);
    auto y = yAxis(placement);
    auto z = zAxis(placement);

    if (std::fabs(localAxis * Base::Vector3d::UnitX) >= std::fabs(localAxis * Base::Vector3d::UnitY)) {
        y *= -1.0;
        z *= -1.0;
    }
    else {
        x *= -1.0;
        z *= -1.0;
    }

    placement.setRotation(Base::Rotation::makeRotationByAxes(x, y, z, "ZXY"));
    return placement;
}

Base::Vector3d placementLocalAxis(const Base::Placement& placement, const Base::Vector3d& localAxis)
{
    if (std::fabs(localAxis * Base::Vector3d::UnitX) >= std::fabs(localAxis * Base::Vector3d::UnitY)) {
        return xAxis(placement);
    }

    return yAxis(placement);
}

Base::Vector3d projectedToLine(
    const Base::Vector3d& point,
    const Base::Vector3d& linePoint,
    const Base::Vector3d& lineDirection
)
{
    const auto axis = normalized(lineDirection);
    return linePoint + axis * ((point - linePoint) * axis);
}

Base::Vector3d projectedToPlane(
    const Base::Vector3d& point,
    const Base::Vector3d& planePoint,
    const Base::Vector3d& planeNormal
)
{
    const auto normal = normalized(planeNormal);
    return point - normal * ((point - planePoint) * normal);
}

Base::Rotation rotationAligningDirectionNear(
    const Base::Rotation& current,
    const Base::Vector3d& localDirection,
    const Base::Vector3d& targetDirection,
    bool unorientedTarget = false
)
{
    const auto currentDirection = current.multVec(normalized(localDirection));
    auto target = normalized(targetDirection);
    if (unorientedTarget && currentDirection * target < 0.0) {
        target *= -1.0;
    }
    return Base::Rotation(currentDirection, target) * current;
}

Base::Rotation rotationFromPrimaryAndSecondaryDirections(
    const Base::Vector3d& localPrimary,
    const Base::Vector3d& localSecondary,
    const Base::Vector3d& worldPrimary,
    const Base::Vector3d& worldSecondary
)
{
    const auto localZ = normalized(localPrimary);
    auto localX = localSecondary - localZ * (localSecondary * localZ);
    if (localX.Length() < Base::Precision::Confusion()) {
        localX = localZ.Cross(Base::Vector3d::UnitX);
    }
    if (localX.Length() < Base::Precision::Confusion()) {
        localX = localZ.Cross(Base::Vector3d::UnitY);
    }
    localX.Normalize();
    auto localY = localZ.Cross(localX);
    localY.Normalize();

    const auto worldZ = normalized(worldPrimary);
    auto worldX = worldSecondary - worldZ * (worldSecondary * worldZ);
    if (worldX.Length() < Base::Precision::Confusion()) {
        worldX = worldZ.Cross(Base::Vector3d::UnitX);
    }
    if (worldX.Length() < Base::Precision::Confusion()) {
        worldX = worldZ.Cross(Base::Vector3d::UnitY);
    }
    worldX.Normalize();
    auto worldY = worldZ.Cross(worldX);
    worldY.Normalize();

    const auto localBasis = Base::Rotation::makeRotationByAxes(localX, localY, localZ, "ZXY");
    const auto worldBasis = Base::Rotation::makeRotationByAxes(worldX, worldY, worldZ, "ZXY");
    return worldBasis * localBasis.inverse();
}

Base::Placement objectPlacementMatchingSnapFrame(
    Base::Placement objectPlacement,
    const Base::Placement& referenceLocalPlacement,
    const Base::Placement& targetPlacement,
    const Base::Vector3d& localAxis
)
{
    objectPlacement.setRotation(rotationFromPrimaryAndSecondaryDirections(
        zAxis(referenceLocalPlacement),
        placementLocalAxis(referenceLocalPlacement, localAxis),
        zAxis(targetPlacement),
        placementLocalAxis(targetPlacement, localAxis)
    ));
    return objectPlacement;
}

std::pair<Base::Vector3d, Base::Vector3d> perpendicularDirections(const Base::Vector3d& direction)
{
    const auto axis = normalized(direction);
    auto first = axis.Cross(Base::Vector3d::UnitX);
    if (first.Length() < Base::Precision::Confusion()) {
        first = axis.Cross(Base::Vector3d::UnitY);
    }
    first.Normalize();
    auto second = axis.Cross(first);
    second.Normalize();
    return {first, second};
}

bool isFiniteValue(double value)
{
    return std::isfinite(value) && !Base::Precision::IsInfinite(value);
}

bool isFiniteVector(const Base::Vector3d& vector)
{
    return isFiniteValue(vector.x) && isFiniteValue(vector.y) && isFiniteValue(vector.z);
}

bool isFiniteRotation(const Base::Rotation& rotation)
{
    double q0 {};
    double q1 {};
    double q2 {};
    double q3 {};
    rotation.getValue(q0, q1, q2, q3);

    return !rotation.isNull() && isFiniteValue(q0) && isFiniteValue(q1) && isFiniteValue(q2)
        && isFiniteValue(q3);
}

bool isFinitePlacement(const Base::Placement& placement)
{
    return isFiniteVector(placement.getPosition()) && isFiniteRotation(placement.getRotation());
}

int rankOfDirections(const std::vector<Base::Vector3d>& directions)
{
    double rows[3][3] {};
    const auto size = std::min<std::size_t>(directions.size(), 3);
    for (std::size_t row = 0; row < size; ++row) {
        const auto direction = normalized(directions[row]);
        rows[row][0] = direction.x;
        rows[row][1] = direction.y;
        rows[row][2] = direction.z;
    }

    int rank = 0;
    for (int col = 0; col < 3 && rank < static_cast<int>(size); ++col) {
        int best = rank;
        for (int row = rank + 1; row < static_cast<int>(size); ++row) {
            if (std::fabs(rows[row][col]) > std::fabs(rows[best][col])) {
                best = row;
            }
        }
        if (std::fabs(rows[best][col]) < Base::Precision::Confusion()) {
            continue;
        }
        if (best != rank) {
            for (int swapCol = col; swapCol < 3; ++swapCol) {
                std::swap(rows[rank][swapCol], rows[best][swapCol]);
            }
        }
        const auto divisor = rows[rank][col];
        for (int normalizeCol = col; normalizeCol < 3; ++normalizeCol) {
            rows[rank][normalizeCol] /= divisor;
        }
        for (int row = 0; row < static_cast<int>(size); ++row) {
            if (row == rank) {
                continue;
            }
            const auto factor = rows[row][col];
            for (int eliminateCol = col; eliminateCol < 3; ++eliminateCol) {
                rows[row][eliminateCol] -= factor * rows[rank][eliminateCol];
            }
        }
        ++rank;
    }
    return rank;
}

class TranslationSolver
{
public:
    explicit TranslationSolver(const Base::Vector3d& preferred)
        : preferred(preferred)
    {}

    void addEquation(const Base::Vector3d& normal, double value, bool locked = false)
    {
        equations.push_back({normalized(normal), value, locked});
    }

    Base::Vector3d solve(const Base::Vector3d& fallback) const
    {
        const auto lockedEquations = independentEquations(true);
        auto translated = solveConstrained(preferred, lockedEquations).value_or(fallback);
        const auto basis = nullspaceBasis(lockedEquations);

        if (basis.empty()) {
            return translated;
        }

        double matrix[3][3] {};
        double rhs[3] {};

        constexpr double regularization = 1e-10;
        for (std::size_t i = 0; i < basis.size(); ++i) {
            matrix[i][i] = regularization;
        }

        for (const auto& equation : equations) {
            if (equation.locked) {
                continue;
            }

            double components[3] {};
            double length = 0.0;
            for (std::size_t i = 0; i < basis.size(); ++i) {
                components[i] = equation.normal * basis[i];
                length += components[i] * components[i];
            }
            if (length < Base::Precision::Confusion()) {
                continue;
            }

            const auto residual = equation.value - equation.normal * translated;
            for (std::size_t row = 0; row < basis.size(); ++row) {
                rhs[row] += components[row] * residual;
                for (std::size_t col = 0; col < basis.size(); ++col) {
                    matrix[row][col] += components[row] * components[col];
                }
            }
        }

        const auto alpha = solveLinearSystem(matrix, rhs, basis.size());
        if (!alpha) {
            return translated;
        }

        for (std::size_t i = 0; i < basis.size(); ++i) {
            translated += basis[i] * (*alpha)[i];
        }
        return translated;
    }

private:
    struct Equation
    {
        Base::Vector3d normal;
        double value;
        bool locked;
    };

    static std::optional<std::array<double, 3>> solveLinearSystem(
        const double matrix[3][3],
        const double rhs[3],
        std::size_t size
    )
    {
        double augmented[3][4] {};
        for (std::size_t row = 0; row < size; ++row) {
            for (std::size_t col = 0; col < size; ++col) {
                augmented[row][col] = matrix[row][col];
            }
            augmented[row][size] = rhs[row];
        }

        for (std::size_t pivot = 0; pivot < size; ++pivot) {
            auto best = pivot;
            for (std::size_t row = pivot + 1; row < size; ++row) {
                if (std::fabs(augmented[row][pivot]) > std::fabs(augmented[best][pivot])) {
                    best = row;
                }
            }
            if (std::fabs(augmented[best][pivot]) < Base::Precision::Confusion()) {
                return std::nullopt;
            }
            if (best != pivot) {
                for (std::size_t col = pivot; col <= size; ++col) {
                    std::swap(augmented[pivot][col], augmented[best][col]);
                }
            }

            const auto divisor = augmented[pivot][pivot];
            for (std::size_t col = pivot; col <= size; ++col) {
                augmented[pivot][col] /= divisor;
            }
            for (std::size_t row = 0; row < size; ++row) {
                if (row == pivot) {
                    continue;
                }
                const auto factor = augmented[row][pivot];
                for (std::size_t col = pivot; col <= size; ++col) {
                    augmented[row][col] -= factor * augmented[pivot][col];
                }
            }
        }

        std::array<double, 3> result {};
        for (std::size_t row = 0; row < size; ++row) {
            result[row] = augmented[row][size];
        }
        return result;
    }

    static int rankOf(const std::vector<Equation>& equations)
    {
        std::vector<Base::Vector3d> directions;
        directions.reserve(equations.size());
        std::ranges::transform(equations, std::back_inserter(directions), [](const auto& equation) {
            return equation.normal;
        });
        return rankOfDirections(directions);
    }

    std::vector<Equation> independentEquations(bool locked) const
    {
        std::vector<Equation> result;
        result.reserve(3);

        for (const auto& equation : equations) {
            if (equation.locked != locked) {
                continue;
            }
            auto candidate = result;
            candidate.push_back(equation);
            if (rankOf(candidate) > rankOf(result)) {
                result.push_back(equation);
                if (result.size() == 3) {
                    break;
                }
            }
        }
        return result;
    }

    static std::optional<Base::Vector3d> solveConstrained(
        const Base::Vector3d& base,
        const std::vector<Equation>& constraints
    )
    {
        if (constraints.empty()) {
            return base;
        }

        double matrix[3][3] {};
        double rhs[3] {};
        for (std::size_t row = 0; row < constraints.size(); ++row) {
            rhs[row] = constraints[row].value - constraints[row].normal * base;
            for (std::size_t col = 0; col < constraints.size(); ++col) {
                matrix[row][col] = constraints[row].normal * constraints[col].normal;
            }
        }

        const auto lambda = solveLinearSystem(matrix, rhs, constraints.size());
        if (!lambda) {
            return std::nullopt;
        }

        auto result = base;
        for (std::size_t i = 0; i < constraints.size(); ++i) {
            result += constraints[i].normal * (*lambda)[i];
        }
        return result;
    }

    static std::vector<Base::Vector3d> nullspaceBasis(const std::vector<Equation>& constraints)
    {
        if (constraints.empty()) {
            return {Base::Vector3d::UnitX, Base::Vector3d::UnitY, Base::Vector3d::UnitZ};
        }
        if (constraints.size() == 1) {
            const auto [first, second] = perpendicularDirections(constraints.front().normal);
            return {first, second};
        }
        if (constraints.size() == 2) {
            auto direction = constraints[0].normal.Cross(constraints[1].normal);
            if (direction.Length() < Base::Precision::Confusion()) {
                return {};
            }
            direction.Normalize();
            return {direction};
        }
        return {};
    }

    Base::Vector3d preferred;
    std::vector<Equation> equations;
};

QString snapTypeLabel(App::SubObjectPlacementProvider::SnapGeometryType type)
{
    using SnapGeometryType = App::SubObjectPlacementProvider::SnapGeometryType;

    switch (type) {
        case SnapGeometryType::Point:
            return QStringLiteral("Point");
        case SnapGeometryType::Axis:
            return QStringLiteral("Axis");
        case SnapGeometryType::Plane:
            return QStringLiteral("Plane");
        case SnapGeometryType::AxisSystem:
            return QStringLiteral("Axis System");
        case SnapGeometryType::Unknown:
            return QStringLiteral("Reference");
    }

    return QStringLiteral("Reference");
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
    : TaskBox(Gui::BitmapFactory().pixmap("Std_TransformManip.svg"), tr("Transform"), false, parent)
    , vp(vp)
    , subObjectPlacementProvider(subObjectPlacementProvider)
    , centerOfMassProvider(centerOfMassProvider)
    , dragger(dragger)
    , ui(new Ui_TaskTransformDialog)
{
    blockSelection(true);
    clearDocumentScope();  // allow cross-document selection for links

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
    auto proxy = new QWidget(this);
    ui->setupUi(proxy);
    this->groupLayout()->addWidget(proxy);

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
            ui->referenceLineEdit->setText(tr("Select face, edge, or vertex…"));
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
            ui->customCSLineEdit->setText(tr("Select object…"));
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

    if (!subObjectPlacementProvider) {
        return;
    }

    auto doc = Application::Instance->getDocument(msg.pDocName);
    if (!doc) {
        return;
    }
    auto obj = doc->getDocument()->getObject(msg.pObjectName);

    if (!obj) {
        return;
    }

    App::DocumentObject* orgObj = obj;
    std::string orgSubName;
    if (msg.pOriginalMsg) {
        auto orgDoc = Application::Instance->getDocument(msg.pOriginalMsg->pDocName);
        if (!orgDoc) {
            return;
        }
        orgObj = orgDoc->getDocument()->getObject(msg.pOriginalMsg->pObjectName);
        if (!orgObj) {
            return;
        }
        orgSubName = msg.pOriginalMsg->pSubName;
    }

    const bool isCumulativeSnapSelection = selectionMode == SelectionMode::SelectCumulativeSnapReference
        || selectionMode == SelectionMode::SelectCumulativeSnapTarget;

    auto globalPlacement = msg.pOriginalMsg
        ? App::GeoFeature::getGlobalPlacement(obj, orgObj, orgSubName)
        : App::GeoFeature::getGlobalPlacement(obj);
    auto localPlacement = App::GeoFeature::getPlacementFromProp(obj, "Placement");
    auto rootPlacement = App::GeoFeature::getGlobalPlacement(vp->getObject());

    auto selectedObjectPlacement = rootPlacement.inverse() * globalPlacement;
    try {
        auto attachedPlacement = subObjectPlacementProvider->calculate(msg.Object, localPlacement);
        selectedObjectPlacement = rootPlacement.inverse() * globalPlacement * attachedPlacement;
    }
    catch (const Base::Exception&) {
        if (!isCumulativeSnapSelection) {
            return;
        }
    }

    std::optional<Base::Vector3d> worldCursor;
    if (msg.hasPickedPoint) {
        worldCursor = Base::Vector3d(msg.x, msg.y, msg.z);
    }
    if (auto snapPos = subObjectPlacementProvider
                           ->snapPosition(msg.Object, worldCursor, globalPlacement.toMatrix())) {
        Base::Vector3d rootLocalSnapPos;
        rootPlacement.inverse().toMatrix().multVec(*snapPos, rootLocalSnapPos);
        selectedObjectPlacement.setPosition(rootLocalSnapPos);
    }

    auto label = linkedSelectionLabel(msg);
    auto geometryType = snapGeometryType(msg);
    auto isMovingObjectSelection = isCumulativeSnapMovingObjectSelection(msg, obj, orgObj);
    auto snapObjectPlacement = selectedObjectPlacement;
    if (subObjectPlacementProvider) {
        if (auto placement = subObjectPlacementProvider->snapPlacement(msg.Object, localPlacement)) {
            snapObjectPlacement = rootPlacement.inverse() * globalPlacement * *placement;
        }
    }

    switch (selectionMode) {
        case SelectionMode::SelectTransformOrigin: {
            if (msg.Type == SelectionChanges::AddSelection) {
                ui->referenceLineEdit->setText(label);
                customTransformOrigin = selectedObjectPlacement;
                updateTransformOrigin();
                setSelectionMode(SelectionMode::None);
            }
            else {
                vp->setTransformOrigin(selectedObjectPlacement);
            }

            break;
        }

        case SelectionMode::SelectAlignTarget: {
            auto targetDraggerPlacement = vp->getObjectPlacement() * selectedObjectPlacement;
            vp->setDraggerPlacement(targetDraggerPlacement);

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
                    label.toStdString(),
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

            if (!isCompatibleCumulativeSnapTarget(currentCumulativeSnapReference->type, geometryType)) {
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
            auto candidateObjectPlacement = vp->getObjectPlacement();
            auto candidateReferencePlacement = candidateObjectPlacement
                * currentCumulativeSnapReference->localPlacement;
            const auto currentReferencePosition = candidateReferencePlacement.getPosition();

            if (referenceType == App::SubObjectPlacementProvider::SnapGeometryType::Axis
                && targetType == App::SubObjectPlacementProvider::SnapGeometryType::Axis) {
                const auto rotation = rotationAligningDirectionNear(
                    candidateObjectPlacement.getRotation(),
                    zAxis(currentCumulativeSnapReference->localPlacement),
                    zAxis(targetReferencePlacement),
                    true
                );
                candidateObjectPlacement.setRotation(rotation);
                candidateReferencePlacement = candidateObjectPlacement
                    * currentCumulativeSnapReference->localPlacement;
                candidateReferencePlacement.setPosition(projectedToLine(
                    currentReferencePosition,
                    targetReferencePlacement.getPosition(),
                    zAxis(targetReferencePlacement)
                ));
                candidateObjectPlacement = candidateReferencePlacement
                    * currentCumulativeSnapReference->localPlacement.inverse();
            }
            else if (
                referenceType == App::SubObjectPlacementProvider::SnapGeometryType::Plane
                && targetType == App::SubObjectPlacementProvider::SnapGeometryType::Plane
            ) {
                const auto rotation = rotationAligningDirectionNear(
                    candidateObjectPlacement.getRotation(),
                    zAxis(currentCumulativeSnapReference->localPlacement),
                    zAxis(targetReferencePlacement)
                );
                candidateObjectPlacement.setRotation(rotation);
                candidateReferencePlacement = candidateObjectPlacement
                    * currentCumulativeSnapReference->localPlacement;
                candidateReferencePlacement.setPosition(projectedToPlane(
                    candidateReferencePlacement.getPosition(),
                    targetReferencePlacement.getPosition(),
                    zAxis(targetReferencePlacement)
                ));
                candidateObjectPlacement = candidateReferencePlacement
                    * currentCumulativeSnapReference->localPlacement.inverse();
            }
            else if (
                referenceType == App::SubObjectPlacementProvider::SnapGeometryType::AxisSystem
                && targetType == App::SubObjectPlacementProvider::SnapGeometryType::Axis
            ) {
                const auto rotation = rotationAligningDirectionNear(
                    candidateObjectPlacement.getRotation(),
                    zAxis(currentCumulativeSnapReference->localPlacement),
                    zAxis(targetReferencePlacement),
                    true
                );
                candidateObjectPlacement.setRotation(rotation);
                candidateReferencePlacement = candidateObjectPlacement
                    * currentCumulativeSnapReference->localPlacement;
                candidateReferencePlacement.setPosition(projectedToLine(
                    currentReferencePosition,
                    targetReferencePlacement.getPosition(),
                    zAxis(targetReferencePlacement)
                ));
                candidateObjectPlacement = candidateReferencePlacement
                    * currentCumulativeSnapReference->localPlacement.inverse();
            }
            else if (
                referenceType == App::SubObjectPlacementProvider::SnapGeometryType::AxisSystem
                && targetType == App::SubObjectPlacementProvider::SnapGeometryType::Plane
            ) {
                const auto rotation = rotationAligningDirectionNear(
                    candidateObjectPlacement.getRotation(),
                    zAxis(currentCumulativeSnapReference->localPlacement),
                    zAxis(targetReferencePlacement)
                );
                candidateObjectPlacement.setRotation(rotation);
                candidateReferencePlacement = candidateObjectPlacement
                    * currentCumulativeSnapReference->localPlacement;
                candidateReferencePlacement.setPosition(projectedToPlane(
                    candidateReferencePlacement.getPosition(),
                    targetReferencePlacement.getPosition(),
                    zAxis(targetReferencePlacement)
                ));
                candidateObjectPlacement = candidateReferencePlacement
                    * currentCumulativeSnapReference->localPlacement.inverse();
            }
            else {
                candidateReferencePlacement.setPosition(targetReferencePlacement.getPosition());
                candidateObjectPlacement = candidateReferencePlacement
                    * currentCumulativeSnapReference->localPlacement.inverse();
            }

            const auto constrainedObjectPlacement = solveCumulativeSnapObjectPlacement(
                candidateObjectPlacement,
                currentCumulativeSnapReference->localPlacement,
                targetReferencePlacement,
                referenceType,
                targetType
            );
            if (!constrainedObjectPlacement || !isFinitePlacement(*constrainedObjectPlacement)) {
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
                    label,
                    currentCumulativeSnapReference->localPlacement,
                    targetReferencePlacement,
                    referenceType,
                    targetType
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

void TaskTransform::setCustomCoordinateSystemFromSelection(const SelectionChanges& msg)
{
    auto doc = Application::Instance->getDocument(msg.pDocName);
    auto obj = doc->getDocument()->getObject(msg.pObjectName);
    if (!obj) {
        return;
    }

    Base::Placement refPlacement;
    QString label;

    if (msg.pOriginalMsg && subObjectPlacementProvider) {
        auto orgDoc = Application::Instance->getDocument(msg.pOriginalMsg->pDocName);
        auto orgObj = orgDoc->getDocument()->getObject(msg.pOriginalMsg->pObjectName);
        auto gp = App::GeoFeature::getGlobalPlacement(obj, orgObj, msg.pOriginalMsg->pSubName);
        auto localPlacement = App::GeoFeature::getPlacementFromProp(obj, "Placement");
        try {
            refPlacement = gp * subObjectPlacementProvider->calculate(msg.Object, localPlacement);
        }
        catch (const Base::Exception&) {
            // Shape type unsupported for attacher (e.g. Solid): fall back to the
            // link-aware placement already resolved above, preserving link-instance transforms.
            refPlacement = gp;
        }
        label = linkedSelectionLabel(msg);
    }
    else if (obj->getDocument() == vp->getObject()->getDocument()) {
        // Tree-view pick without link resolution: only accept same-document objects
        // to avoid treating cross-document placements as if they were in this document's space.
        refPlacement = App::GeoFeature::getGlobalPlacement(obj);
        label = QString::fromUtf8(obj->Label.getValue());
    }
    else {
        return;
    }

    customCoordinateSystemPlacement = refPlacement;
    ui->customCSLineEdit->setText(label);
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

bool TaskTransform::isCompatibleCumulativeSnapTarget(
    App::SubObjectPlacementProvider::SnapGeometryType referenceType,
    App::SubObjectPlacementProvider::SnapGeometryType targetType
) const
{
    using SnapGeometryType = App::SubObjectPlacementProvider::SnapGeometryType;

    if (referenceType == SnapGeometryType::Unknown || targetType == SnapGeometryType::Unknown) {
        return false;
    }

    if (referenceType == targetType) {
        return true;
    }

    return referenceType == SnapGeometryType::AxisSystem
        && (targetType == SnapGeometryType::Axis || targetType == SnapGeometryType::Plane);
}

std::optional<Base::Placement> TaskTransform::solveCumulativeSnapObjectPlacement(
    const Base::Placement& candidate,
    const Base::Placement& currentReferenceLocalPlacement,
    const Base::Placement& currentReferenceTargetPlacement,
    App::SubObjectPlacementProvider::SnapGeometryType currentReferenceType,
    App::SubObjectPlacementProvider::SnapGeometryType currentTargetType,
    bool currentTargetDirectionFixed,
    std::size_t historySize
) const
{
    using SnapGeometryType = App::SubObjectPlacementProvider::SnapGeometryType;

    enum class ConstraintKind
    {
        Unknown,
        PointCoincident,
        AxisCoincident,
        PlaneCoincident,
    };

    struct PlacementConstraint
    {
        Base::Placement local;
        Base::Placement target;
        ConstraintKind kind;
        bool locked;
        bool targetDirectionFixed;
    };

    struct DirectionConstraint
    {
        Base::Vector3d local;
        Base::Vector3d target;
        bool locked;
    };

    auto constraintKind = [](SnapGeometryType referenceType, SnapGeometryType targetType) {
        if (referenceType == SnapGeometryType::Point && targetType == SnapGeometryType::Point) {
            return ConstraintKind::PointCoincident;
        }
        if (referenceType == SnapGeometryType::Axis && targetType == SnapGeometryType::Axis) {
            return ConstraintKind::AxisCoincident;
        }
        if (referenceType == SnapGeometryType::AxisSystem && targetType == SnapGeometryType::Axis) {
            return ConstraintKind::AxisCoincident;
        }
        if (referenceType == SnapGeometryType::Plane && targetType == SnapGeometryType::Plane) {
            return ConstraintKind::PlaneCoincident;
        }
        if (referenceType == SnapGeometryType::AxisSystem && targetType == SnapGeometryType::Plane) {
            return ConstraintKind::PlaneCoincident;
        }
        return ConstraintKind::Unknown;
    };

    auto hasDirection = [](ConstraintKind kind) {
        return kind == ConstraintKind::AxisCoincident || kind == ConstraintKind::PlaneCoincident;
    };

    std::vector<PlacementConstraint> baseConstraints;
    const auto activeHistorySize = std::min(historySize, cumulativeSnapHistory.size());
    baseConstraints.reserve(activeHistorySize + 1);

    for (std::size_t i = 0; i < activeHistorySize; ++i) {
        const auto& step = cumulativeSnapHistory[i];
        baseConstraints.push_back({
            step.referenceLocalPlacement,
            step.targetPlacement,
            constraintKind(step.referenceType, step.targetType),
            true,
            step.targetDirectionFixed,
        });
    }
    baseConstraints.push_back({
        currentReferenceLocalPlacement,
        currentReferenceTargetPlacement,
        constraintKind(currentReferenceType, currentTargetType),
        false,
        currentTargetDirectionFixed,
    });

    if (baseConstraints.back().kind == ConstraintKind::Unknown) {
        return std::nullopt;
    }

    constexpr double directionIndependenceTolerance = 1e-4;
    constexpr double snapPositionTolerance = 1e-5;
    constexpr double snapDirectionTolerance = 1e-5;

    auto findIndependentDirectionPair = [](const std::vector<DirectionConstraint>& directions,
                                           bool lockedOnly,
                                           std::size_t& firstIndex,
                                           std::size_t& secondIndex) {
        std::size_t bestFirst = 0;
        std::size_t bestSecond = 0;
        double bestIndependence = 0.0;

        for (std::size_t i = 0; i < directions.size(); ++i) {
            if (lockedOnly && !directions[i].locked) {
                continue;
            }
            for (std::size_t j = i + 1; j < directions.size(); ++j) {
                if (lockedOnly && !directions[j].locked) {
                    continue;
                }
                const auto localIndependence = directions[i].local.Cross(directions[j].local).Length();
                const auto targetIndependence
                    = directions[i].target.Cross(directions[j].target).Length();
                const auto independence = std::min(localIndependence, targetIndependence);
                if (independence > bestIndependence) {
                    bestIndependence = independence;
                    bestFirst = i;
                    bestSecond = j;
                }
            }
        }

        firstIndex = bestFirst;
        secondIndex = bestSecond;
        return bestIndependence;
    };

    auto solveDirectedConstraints =
        [&](const std::vector<PlacementConstraint>& constraints) -> std::optional<Base::Placement> {
        std::vector<DirectionConstraint> directionConstraints;
        directionConstraints.reserve(constraints.size());
        for (const auto& constraint : constraints) {
            if (!hasDirection(constraint.kind)) {
                continue;
            }
            directionConstraints.push_back({
                zAxis(constraint.local),
                zAxis(constraint.target),
                constraint.locked,
            });
        }

        auto rotation = candidate.getRotation();
        bool rotationDeterminedByDirections = false;
        if (directionConstraints.size() == 1) {
            rotation = rotationAligningDirectionNear(
                rotation,
                directionConstraints.front().local,
                directionConstraints.front().target
            );
        }
        else if (directionConstraints.size() > 1) {
            std::size_t first = 0;
            std::size_t second = 0;
            if (findIndependentDirectionPair(directionConstraints, true, first, second)
                    <= directionIndependenceTolerance
                && findIndependentDirectionPair(directionConstraints, false, first, second)
                    <= directionIndependenceTolerance) {
                rotation = rotationAligningDirectionNear(
                    rotation,
                    directionConstraints.front().local,
                    directionConstraints.front().target
                );
            }
            else {
                rotation = rotationFromPrimaryAndSecondaryDirections(
                    directionConstraints[first].local,
                    directionConstraints[second].local,
                    directionConstraints[first].target,
                    directionConstraints[second].target
                );
                rotationDeterminedByDirections = true;
            }
        }

        if (!rotationDeterminedByDirections) {
            bool twistApplied = false;
            for (const auto& primary : constraints) {
                if (!primary.locked || primary.kind != ConstraintKind::AxisCoincident) {
                    continue;
                }

                const auto localPrimaryDirection = zAxis(primary.local);
                const auto targetPrimaryDirection = zAxis(primary.target);
                for (const auto& secondary : constraints) {
                    if (&secondary == &primary || secondary.kind != ConstraintKind::AxisCoincident) {
                        continue;
                    }
                    if (localPrimaryDirection.Cross(zAxis(secondary.local)).Length()
                            > directionIndependenceTolerance
                        || targetPrimaryDirection.Cross(zAxis(secondary.target)).Length()
                            > directionIndependenceTolerance) {
                        continue;
                    }

                    const auto rotatedOffset = rotation.multVec(
                        secondary.local.getPosition() - primary.local.getPosition()
                    );
                    auto currentOffset = rotatedOffset
                        - targetPrimaryDirection * (rotatedOffset * targetPrimaryDirection);
                    auto targetOffset = secondary.target.getPosition() - primary.target.getPosition();
                    targetOffset = targetOffset
                        - targetPrimaryDirection * (targetOffset * targetPrimaryDirection);
                    if (currentOffset.Length() < Base::Precision::Confusion()
                        || targetOffset.Length() < Base::Precision::Confusion()) {
                        continue;
                    }

                    currentOffset.Normalize();
                    targetOffset.Normalize();
                    const auto angle = std::atan2(
                        targetPrimaryDirection * currentOffset.Cross(targetOffset),
                        currentOffset * targetOffset
                    );
                    rotation = Base::Rotation(targetPrimaryDirection, angle) * rotation;
                    twistApplied = true;
                    break;
                }
                if (twistApplied) {
                    break;
                }
            }
        }

        TranslationSolver translationSolver(candidate.getPosition());
        for (const auto& constraint : constraints) {
            const auto rotatedLocalPosition = rotation.multVec(constraint.local.getPosition());
            const auto targetPosition = constraint.target.getPosition();

            switch (constraint.kind) {
                case ConstraintKind::PointCoincident:
                    translationSolver.addEquation(
                        Base::Vector3d::UnitX,
                        targetPosition.x - rotatedLocalPosition.x,
                        constraint.locked
                    );
                    translationSolver.addEquation(
                        Base::Vector3d::UnitY,
                        targetPosition.y - rotatedLocalPosition.y,
                        constraint.locked
                    );
                    translationSolver.addEquation(
                        Base::Vector3d::UnitZ,
                        targetPosition.z - rotatedLocalPosition.z,
                        constraint.locked
                    );
                    break;
                case ConstraintKind::AxisCoincident: {
                    const auto [first, second] = perpendicularDirections(zAxis(constraint.target));
                    const auto delta = targetPosition - rotatedLocalPosition;
                    translationSolver.addEquation(first, first * delta, constraint.locked);
                    translationSolver.addEquation(second, second * delta, constraint.locked);
                    break;
                }
                case ConstraintKind::PlaneCoincident: {
                    const auto normal = zAxis(constraint.target);
                    translationSolver.addEquation(
                        normal,
                        normal * (targetPosition - rotatedLocalPosition),
                        constraint.locked
                    );
                    break;
                }
                case ConstraintKind::Unknown:
                    break;
            }
        }

        const Base::Placement result {translationSolver.solve(candidate.getPosition()), rotation};
        std::vector<Base::Vector3d> lockedTranslationNormals;
        for (const auto& constraint : constraints) {
            if (!constraint.locked) {
                continue;
            }
            switch (constraint.kind) {
                case ConstraintKind::PointCoincident:
                    lockedTranslationNormals.push_back(Base::Vector3d::UnitX);
                    lockedTranslationNormals.push_back(Base::Vector3d::UnitY);
                    lockedTranslationNormals.push_back(Base::Vector3d::UnitZ);
                    break;
                case ConstraintKind::AxisCoincident: {
                    const auto [first, second] = perpendicularDirections(zAxis(constraint.target));
                    lockedTranslationNormals.push_back(first);
                    lockedTranslationNormals.push_back(second);
                    break;
                }
                case ConstraintKind::PlaneCoincident:
                    lockedTranslationNormals.push_back(zAxis(constraint.target));
                    break;
                case ConstraintKind::Unknown:
                    break;
            }
        }

        auto hasRemainingTranslationFreedom =
            [&lockedTranslationNormals](const std::vector<Base::Vector3d>& normals) {
                const auto lockedRank = rankOfDirections(lockedTranslationNormals);
                for (const auto& normal : normals) {
                    auto candidateNormals = lockedTranslationNormals;
                    candidateNormals.push_back(normal);
                    if (rankOfDirections(candidateNormals) > lockedRank) {
                        return true;
                    }
                }
                return false;
            };
        auto shouldValidateTranslation = [&](const PlacementConstraint& constraint) {
            if (constraint.locked) {
                return true;
            }
            switch (constraint.kind) {
                case ConstraintKind::PointCoincident:
                    return hasRemainingTranslationFreedom({
                        Base::Vector3d::UnitX,
                        Base::Vector3d::UnitY,
                        Base::Vector3d::UnitZ,
                    });
                case ConstraintKind::AxisCoincident: {
                    const auto [first, second] = perpendicularDirections(zAxis(constraint.target));
                    return hasRemainingTranslationFreedom({first, second});
                }
                case ConstraintKind::PlaneCoincident:
                    return hasRemainingTranslationFreedom({zAxis(constraint.target)});
                case ConstraintKind::Unknown:
                    return false;
            }
            return false;
        };
        auto constraintSatisfied = [](const Base::Placement& objectPlacement,
                                      const PlacementConstraint& constraint,
                                      bool validateTranslation,
                                      double positionTolerance,
                                      double directionTolerance) {
            const auto referencePlacement = objectPlacement * constraint.local;
            const auto referenceDirection = zAxis(referencePlacement);
            const auto targetDirection = zAxis(constraint.target);
            const auto positionDelta = referencePlacement.getPosition()
                - constraint.target.getPosition();

            switch (constraint.kind) {
                case ConstraintKind::PointCoincident:
                    if (!validateTranslation) {
                        return true;
                    }
                    return positionDelta.Length() <= positionTolerance;
                case ConstraintKind::AxisCoincident:
                    if (referenceDirection * targetDirection < 1.0 - directionTolerance) {
                        return false;
                    }
                    if (!validateTranslation) {
                        return true;
                    }
                    return (positionDelta.Cross(targetDirection)).Length() <= positionTolerance;
                case ConstraintKind::PlaneCoincident:
                    if (referenceDirection * targetDirection < 1.0 - directionTolerance) {
                        return false;
                    }
                    if (!validateTranslation) {
                        return true;
                    }
                    return std::fabs(positionDelta * targetDirection) <= positionTolerance;
                case ConstraintKind::Unknown:
                    return true;
            }
            return true;
        };

        if (!std::ranges::all_of(constraints, [&](const PlacementConstraint& constraint) {
                return constraintSatisfied(
                    result,
                    constraint,
                    shouldValidateTranslation(constraint),
                    snapPositionTolerance,
                    snapDirectionTolerance
                );
            })) {
            return std::nullopt;
        }

        return result;
    };

    std::vector<std::size_t> unorientedConstraintIndices;
    for (std::size_t i = 0; i < baseConstraints.size(); ++i) {
        if (hasDirection(baseConstraints[i].kind) && !baseConstraints[i].targetDirectionFixed) {
            unorientedConstraintIndices.push_back(i);
        }
    }

    constexpr std::size_t maxEnumeratedUnorientedDirections = 12;
    const auto enumeratedUnorientedDirections
        = std::min(maxEnumeratedUnorientedDirections, unorientedConstraintIndices.size());
    const std::size_t variantCount = std::size_t {1} << enumeratedUnorientedDirections;

    auto rotationScore = [](const Base::Rotation& rotation, const Base::Rotation& preferred) {
        return 3.0
            - rotation.multVec(Base::Vector3d::UnitX) * preferred.multVec(Base::Vector3d::UnitX)
            - rotation.multVec(Base::Vector3d::UnitY) * preferred.multVec(Base::Vector3d::UnitY)
            - rotation.multVec(Base::Vector3d::UnitZ) * preferred.multVec(Base::Vector3d::UnitZ);
    };

    std::optional<Base::Placement> bestPlacement;
    auto bestScore = std::numeric_limits<double>::max();
    for (std::size_t variant = 0; variant < variantCount; ++variant) {
        auto constraints = baseConstraints;
        for (std::size_t signIndex = 0; signIndex < unorientedConstraintIndices.size(); ++signIndex) {
            auto& constraint = constraints[unorientedConstraintIndices[signIndex]];
            const auto preferredPositive = candidate.getRotation().multVec(zAxis(constraint.local))
                    * zAxis(constraint.target)
                >= 0.0;
            const auto flipFromPreferred = signIndex < enumeratedUnorientedDirections
                && (variant & (std::size_t {1} << signIndex)) != 0;
            const auto usePositive = flipFromPreferred ? !preferredPositive : preferredPositive;
            if (!usePositive) {
                constraint.target
                    = invertedPlacementAroundLocalAxis(constraint.target, Base::Vector3d::UnitX);
            }
            constraint.targetDirectionFixed = true;
        }

        const auto placement = solveDirectedConstraints(constraints);
        if (!placement || !isFinitePlacement(*placement)) {
            continue;
        }

        const auto score = rotationScore(placement->getRotation(), candidate.getRotation())
            + (placement->getPosition() - candidate.getPosition()).Length() * 1e-6;
        if (score < bestScore) {
            bestPlacement = placement;
            bestScore = score;
        }
    }

    return bestPlacement;
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
    updateCumulativeSnapUi();
}

void TaskTransform::appendCumulativeSnapStep(
    const QString& referenceLabel,
    const QString& targetLabel,
    const Base::Placement& referenceLocalPlacement,
    const Base::Placement& targetPlacement,
    App::SubObjectPlacementProvider::SnapGeometryType referenceType,
    App::SubObjectPlacementProvider::SnapGeometryType targetType
)
{
    const auto constraintLabel = snapTypeLabel(referenceType);
    const auto objectPlacement = vp->getObjectPlacement();
    auto storedTargetPlacement = targetPlacement;
    const auto targetDirectionFixed = isPlaneSnap(referenceType, targetType);
    if (targetDirectionFixed) {
        const auto acceptedReferencePlacement = objectPlacement * referenceLocalPlacement;
        if (zAxis(acceptedReferencePlacement) * zAxis(storedTargetPlacement) < 0.0) {
            storedTargetPlacement
                = invertedPlacementAroundLocalAxis(storedTargetPlacement, Base::Vector3d::UnitX);
        }
    }

    cumulativeSnapHistory.push_back({
        QStringLiteral("%1: %2 -> %3").arg(constraintLabel).arg(referenceLabel).arg(targetLabel).toStdString(),
        objectPlacement,
        referenceLocalPlacement,
        storedTargetPlacement,
        referenceType,
        targetType,
        targetDirectionFixed,
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

    if (step.referenceType == step.targetType) {
        return step.referenceType == SnapGeometryType::Axis
            || step.referenceType == SnapGeometryType::Plane;
    }

    return step.referenceType == SnapGeometryType::AxisSystem
        && (step.targetType == SnapGeometryType::Axis || step.targetType == SnapGeometryType::Plane);
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
    step.targetPlacement = invertedPlacementAroundLocalAxis(step.targetPlacement, localAxis);
    step.targetDirectionFixed = true;
    step.objectPlacement = objectPlacementMatchingSnapFrame(
        step.objectPlacement,
        step.referenceLocalPlacement,
        step.targetPlacement,
        localAxis
    );

    vp->setTransformOrigin(step.referenceLocalPlacement);
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
        const auto preferredPlacement = isFinitePlacement(step.objectPlacement) ? step.objectPlacement
                                                                                : candidate;
        const auto placement = solveCumulativeSnapObjectPlacement(
            preferredPlacement,
            step.referenceLocalPlacement,
            step.targetPlacement,
            step.referenceType,
            step.targetType,
            step.targetDirectionFixed,
            i
        );
        if (!placement || !isFinitePlacement(*placement)) {
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
