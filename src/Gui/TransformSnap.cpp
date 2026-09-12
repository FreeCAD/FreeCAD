// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Max Wilfinger
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include "TransformSnap.h"

#include <Base/Precision.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <limits>
#include <utility>

namespace Gui::TransformSnap
{

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

bool isCompatible(
    App::SubObjectPlacementProvider::SnapGeometryType referenceType,
    App::SubObjectPlacementProvider::SnapGeometryType targetType
)
{
    using SnapGeometryType = App::SubObjectPlacementProvider::SnapGeometryType;

    if (referenceType == SnapGeometryType::Unknown || targetType == SnapGeometryType::Unknown) {
        return false;
    }

    if (referenceType == targetType) {
        return referenceType != SnapGeometryType::AxisSystem;
    }

    return referenceType == SnapGeometryType::AxisSystem
        && (targetType == SnapGeometryType::Axis || targetType == SnapGeometryType::Plane);
}

Base::Placement preferredPlacement(
    Base::Placement objectPlacement,
    const Base::Placement& referenceLocalPlacement,
    const Base::Placement& targetPlacement,
    GeometryType targetType
)
{
    auto referencePlacement = objectPlacement * referenceLocalPlacement;
    switch (targetType) {
        case GeometryType::Axis: {
            const auto referencePosition = referencePlacement.getPosition();
            objectPlacement.setRotation(rotationAligningDirectionNear(
                objectPlacement.getRotation(),
                zAxis(referenceLocalPlacement),
                zAxis(targetPlacement),
                true
            ));
            referencePlacement = objectPlacement * referenceLocalPlacement;
            referencePlacement.setPosition(
                projectedToLine(referencePosition, targetPlacement.getPosition(), zAxis(targetPlacement))
            );
            break;
        }
        case GeometryType::Plane:
            objectPlacement.setRotation(rotationAligningDirectionNear(
                objectPlacement.getRotation(),
                zAxis(referenceLocalPlacement),
                zAxis(targetPlacement)
            ));
            referencePlacement = objectPlacement * referenceLocalPlacement;
            referencePlacement.setPosition(projectedToPlane(
                referencePlacement.getPosition(),
                targetPlacement.getPosition(),
                zAxis(targetPlacement)
            ));
            break;
        case GeometryType::Point:
            referencePlacement.setPosition(targetPlacement.getPosition());
            break;
        default:
            return objectPlacement;
    }
    return referencePlacement * referenceLocalPlacement.inverse();
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

    void addEquation(const Base::Vector3d& normal, double value)
    {
        equations.push_back({normalized(normal), value});
    }

    std::optional<Base::Vector3d> solve() const
    {
        return solveConstrained(preferred, independentEquations());
    }

private:
    struct Equation
    {
        Base::Vector3d normal;
        double value;
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

    std::vector<Equation> independentEquations() const
    {
        std::vector<Equation> result;
        result.reserve(3);

        for (const auto& equation : equations) {
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

    Base::Vector3d preferred;
    std::vector<Equation> equations;
};

std::optional<Base::Placement> solve(
    const Base::Placement& candidate,
    const std::vector<Constraint>& constraints
)
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
        bool targetDirectionSignFixed;
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

    if (!isFinitePlacement(candidate)) {
        return std::nullopt;
    }
    if (constraints.empty()) {
        return candidate;
    }

    std::vector<PlacementConstraint> baseConstraints;
    baseConstraints.reserve(constraints.size());
    for (std::size_t i = 0; i < constraints.size(); ++i) {
        const auto& constraint = constraints[i];
        baseConstraints.push_back({
            constraint.localPlacement,
            constraint.targetPlacement,
            constraintKind(constraint.referenceType, constraint.targetType),
            i + 1 < constraints.size(),
            constraint.targetDirectionSignFixed,
        });
    }

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

    auto solveDirectedConstraints = [&](
                                        const std::vector<PlacementConstraint>& directedConstraints
                                    ) -> std::optional<Base::Placement> {
        std::vector<DirectionConstraint> directionConstraints;
        directionConstraints.reserve(directedConstraints.size());
        for (const auto& constraint : directedConstraints) {
            if (!hasDirection(constraint.kind)) {
                continue;
            }
            directionConstraints.push_back({
                zAxis(constraint.local),
                zAxis(constraint.target),
                constraint.locked,
            });
        }

        for (std::size_t i = 0; i < directedConstraints.size(); ++i) {
            const auto& first = directedConstraints[i];
            if (first.kind != ConstraintKind::PointCoincident) {
                continue;
            }
            for (std::size_t j = i + 1; j < directedConstraints.size(); ++j) {
                const auto& second = directedConstraints[j];
                if (second.kind != ConstraintKind::PointCoincident) {
                    continue;
                }
                const auto local = second.local.getPosition() - first.local.getPosition();
                const auto target = second.target.getPosition() - first.target.getPosition();
                if (local.Length() > snapPositionTolerance
                    && target.Length() > snapPositionTolerance) {
                    directionConstraints.push_back({
                        normalized(local),
                        normalized(target),
                        first.locked && second.locked,
                    });
                }
            }
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
            for (const auto& primary : directedConstraints) {
                if (primary.kind != ConstraintKind::AxisCoincident) {
                    continue;
                }

                const auto localPrimaryDirection = zAxis(primary.local);
                const auto targetPrimaryDirection = zAxis(primary.target);
                for (const auto& secondary : directedConstraints) {
                    if (&secondary == &primary || (!primary.locked && !secondary.locked)) {
                        continue;
                    }
                    if (secondary.kind == ConstraintKind::AxisCoincident) {
                        if (localPrimaryDirection.Cross(zAxis(secondary.local)).Length()
                                > directionIndependenceTolerance
                            || targetPrimaryDirection.Cross(zAxis(secondary.target)).Length()
                                > directionIndependenceTolerance) {
                            continue;
                        }
                    }
                    else if (secondary.kind != ConstraintKind::PointCoincident) {
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
        for (const auto& constraint : directedConstraints) {
            const auto rotatedLocalPosition = rotation.multVec(constraint.local.getPosition());
            const auto targetPosition = constraint.target.getPosition();

            switch (constraint.kind) {
                case ConstraintKind::PointCoincident:
                    translationSolver.addEquation(
                        Base::Vector3d::UnitX,
                        targetPosition.x - rotatedLocalPosition.x
                    );
                    translationSolver.addEquation(
                        Base::Vector3d::UnitY,
                        targetPosition.y - rotatedLocalPosition.y
                    );
                    translationSolver.addEquation(
                        Base::Vector3d::UnitZ,
                        targetPosition.z - rotatedLocalPosition.z
                    );
                    break;
                case ConstraintKind::AxisCoincident: {
                    const auto [first, second] = perpendicularDirections(zAxis(constraint.target));
                    const auto delta = targetPosition - rotatedLocalPosition;
                    translationSolver.addEquation(first, first * delta);
                    translationSolver.addEquation(second, second * delta);
                    break;
                }
                case ConstraintKind::PlaneCoincident: {
                    const auto normal = zAxis(constraint.target);
                    translationSolver.addEquation(
                        normal,
                        normal * (targetPosition - rotatedLocalPosition)
                    );
                    break;
                }
                case ConstraintKind::Unknown:
                    break;
            }
        }

        const auto translation = translationSolver.solve();
        if (!translation) {
            return std::nullopt;
        }
        const Base::Placement result {*translation, rotation};
        auto constraintSatisfied = [](const Base::Placement& objectPlacement,
                                      const PlacementConstraint& constraint,
                                      double positionTolerance,
                                      double directionTolerance) {
            const auto referencePlacement = objectPlacement * constraint.local;
            const auto referenceDirection = zAxis(referencePlacement);
            const auto targetDirection = zAxis(constraint.target);
            const auto positionDelta = referencePlacement.getPosition()
                - constraint.target.getPosition();

            switch (constraint.kind) {
                case ConstraintKind::PointCoincident:
                    return positionDelta.Length() <= positionTolerance;
                case ConstraintKind::AxisCoincident:
                    if (referenceDirection * targetDirection < 1.0 - directionTolerance) {
                        return false;
                    }
                    return (positionDelta.Cross(targetDirection)).Length() <= positionTolerance;
                case ConstraintKind::PlaneCoincident:
                    if (referenceDirection * targetDirection < 1.0 - directionTolerance) {
                        return false;
                    }
                    return std::fabs(positionDelta * targetDirection) <= positionTolerance;
                case ConstraintKind::Unknown:
                    return true;
            }
            return true;
        };

        if (!std::ranges::all_of(directedConstraints, [&](const PlacementConstraint& constraint) {
                return constraintSatisfied(
                    result,
                    constraint,
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
        if (hasDirection(baseConstraints[i].kind) && !baseConstraints[i].targetDirectionSignFixed) {
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
        auto directedConstraints = baseConstraints;
        for (std::size_t signIndex = 0; signIndex < unorientedConstraintIndices.size(); ++signIndex) {
            auto& constraint = directedConstraints[unorientedConstraintIndices[signIndex]];
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
            constraint.targetDirectionSignFixed = true;
        }

        const auto placement = solveDirectedConstraints(directedConstraints);
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

}  // namespace Gui::TransformSnap
