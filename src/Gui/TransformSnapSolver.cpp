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

#include "TransformSnapSolver.h"
#include "TransformSnapRotation.h"
#include "TranslationConstraintSolver.h"

#include <Base/Precision.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace Gui::TransformSnap
{
namespace
{
// Limit interactive work to 4096 sign combinations. Any further ambiguous
// directions retain the sign closest to the preferred rotation.
constexpr std::size_t maxEnumeratedDirections = 12;
}  // namespace

PlacementSolver::PlacementSolver(const Base::Placement& preferred, std::vector<Constraint> constraints)
    : preferred(preferred)
    , constraints(std::move(constraints))
{}

bool PlacementSolver::isUsableFrame(const Base::Placement& placement)
{
    // A finite number can still represent geometric infinity in FreeCAD.
    const auto& position = placement.getPosition();
    const auto largestCoordinate = std::max(
        {std::fabs(position.x), std::fabs(position.y), std::fabs(position.z)}
    );
    return placement.isFinite() && !placement.getRotation().isNull()
        && !Base::Precision::IsInfinite(largestCoordinate);
}

bool PlacementSolver::hasValidInput() const
{
    return isUsableFrame(preferred)
        && std::ranges::all_of(constraints, [](const Constraint& constraint) {
               return isCompatible(constraint.referenceType, constraint.targetType)
                   && isUsableFrame(constraint.localPlacement)
                   && isUsableFrame(constraint.targetPlacement);
           });
}

std::optional<Base::Placement> PlacementSolver::solve() const
{
    if (!hasValidInput()) {
        return std::nullopt;
    }
    if (constraints.empty()) {
        return preferred;
    }

    std::vector<std::size_t> unorientedIndices;
    for (std::size_t i = 0; i < constraints.size(); ++i) {
        if (constraints[i].targetType != GeometryType::Point
            && !constraints[i].targetDirectionSignFixed) {
            unorientedIndices.push_back(i);
        }
    }
    const auto directionCount = std::min(maxEnumeratedDirections, unorientedIndices.size());
    const std::size_t variantCount = std::size_t {1} << directionCount;
    std::optional<Base::Placement> best;
    double bestScore = std::numeric_limits<double>::max();
    for (std::size_t variant = 0; variant < variantCount; ++variant) {
        const auto placement = solveDirected(orientTargets(unorientedIndices, variant));
        if (!placement) {
            continue;
        }
        const auto score = placementScore(*placement);
        if (score < bestScore) {
            best = placement;
            bestScore = score;
        }
    }
    return best;
}

std::vector<Constraint> PlacementSolver::orientTargets(
    const std::vector<std::size_t>& unorientedIndices,
    std::size_t variant
) const
{
    auto directed = constraints;
    for (std::size_t signIndex = 0; signIndex < unorientedIndices.size(); ++signIndex) {
        auto& constraint = directed[unorientedIndices[signIndex]];
        const auto referenceDirection = preferred.getRotation().multVec(
            zAxis(constraint.localPlacement)
        );
        const bool preferredPositive = referenceDirection * zAxis(constraint.targetPlacement) >= 0.0;
        const bool flipFromPreferred = signIndex < maxEnumeratedDirections
            && (variant & (std::size_t {1} << signIndex)) != 0;
        const bool usePositive = flipFromPreferred ? !preferredPositive : preferredPositive;
        if (!usePositive) {
            constraint.targetPlacement
                = invertedPlacementAroundLocalAxis(constraint.targetPlacement, Base::Vector3d::UnitX);
        }
        constraint.targetDirectionSignFixed = true;
    }
    return directed;
}

std::optional<Base::Placement> PlacementSolver::solveDirected(
    const std::vector<Constraint>& directed
) const
{
    const auto rotation = RotationSolver(preferred.getRotation(), directed).solve();
    const auto translation = solveTranslation(rotation, directed);
    if (!translation) {
        return std::nullopt;
    }
    const Base::Placement result(*translation, rotation);
    if (!isUsableFrame(result) || !std::ranges::all_of(directed, [&](const Constraint& constraint) {
            return satisfiesConstraint(result, constraint);
        })) {
        return std::nullopt;
    }
    return result;
}

std::optional<Base::Vector3d> PlacementSolver::solveTranslation(
    const Base::Rotation& rotation,
    const std::vector<Constraint>& directed
) const
{
    TranslationConstraintSolver solver;
    for (const auto& constraint : directed) {
        const auto delta = constraint.targetPlacement.getPosition()
            - rotation.multVec(constraint.localPlacement.getPosition());
        switch (constraint.targetType) {
            case GeometryType::Point:
                solver.addEquation(Base::Vector3d::UnitX, delta.x);
                solver.addEquation(Base::Vector3d::UnitY, delta.y);
                solver.addEquation(Base::Vector3d::UnitZ, delta.z);
                break;
            case GeometryType::Axis: {
                // Matching an axis leaves translation along it unconstrained.
                const auto frame = Base::Rotation::fromNormalVector(zAxis(constraint.targetPlacement));
                const auto first = frame.multVec(Base::Vector3d::UnitX);
                const auto second = frame.multVec(Base::Vector3d::UnitY);
                solver.addEquation(first, first * delta);
                solver.addEquation(second, second * delta);
                break;
            }
            case GeometryType::Plane: {
                const auto normal = zAxis(constraint.targetPlacement);
                solver.addEquation(normal, normal * delta);
                break;
            }
            default:
                return std::nullopt;
        }
    }
    return solver.solve(preferred.getPosition(), positionTolerance);
}

bool PlacementSolver::satisfiesConstraint(const Base::Placement& placement, const Constraint& constraint)
{
    const auto reference = placement * constraint.localPlacement;
    const auto targetDirection = zAxis(constraint.targetPlacement);
    const auto delta = reference.getPosition() - constraint.targetPlacement.getPosition();
    if (constraint.targetType != GeometryType::Point
        && zAxis(reference) * targetDirection < 1.0 - directionTolerance) {
        return false;
    }
    switch (constraint.targetType) {
        case GeometryType::Point:
            return delta.Length() <= positionTolerance;
        case GeometryType::Axis:
            return delta.Cross(targetDirection).Length() <= positionTolerance;
        case GeometryType::Plane:
            return std::fabs(delta * targetDirection) <= positionTolerance;
        default:
            return false;
    }
}

double PlacementSolver::placementScore(const Base::Placement& placement) const
{
    // Sum the changes to the three frame axes, with a small distance weight to
    // favour nearby placements when the orientation scores are similar.
    double rotationChange = 0.0;
    for (const auto& axis : {Base::Vector3d::UnitX, Base::Vector3d::UnitY, Base::Vector3d::UnitZ}) {
        rotationChange += 1.0
            - placement.getRotation().multVec(axis) * preferred.getRotation().multVec(axis);
    }
    constexpr double translationWeight = 1e-6;
    return rotationChange
        + (placement.getPosition() - preferred.getPosition()).Length() * translationWeight;
}

}  // namespace Gui::TransformSnap
