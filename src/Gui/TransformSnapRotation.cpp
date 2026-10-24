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

#include "TransformSnapRotation.h"

#include <Base/Precision.h>

#include <algorithm>
#include <cmath>

namespace Gui::TransformSnap
{

RotationSolver::RotationSolver(const Base::Rotation& preferred, const std::vector<Constraint>& constraints)
    : preferred(preferred)
    , constraints(constraints)
{}

std::vector<RotationSolver::Direction> RotationSolver::collectDirections() const
{
    std::vector<Direction> directions;
    for (std::size_t i = 0; i < constraints.size(); ++i) {
        const auto& constraint = constraints[i];
        if (constraint.targetType == GeometryType::Axis
            || constraint.targetType == GeometryType::Plane) {
            directions.push_back({
                zAxis(constraint.localPlacement),
                zAxis(constraint.targetPlacement),
                i + 1 < constraints.size(),
            });
        }
    }

    // Two coincident point pairs also constrain the direction between them.
    for (std::size_t i = 0; i < constraints.size(); ++i) {
        const auto& first = constraints[i];
        if (first.targetType != GeometryType::Point) {
            continue;
        }
        for (std::size_t j = i + 1; j < constraints.size(); ++j) {
            const auto& second = constraints[j];
            if (second.targetType != GeometryType::Point) {
                continue;
            }
            const auto local = second.localPlacement.getPosition()
                - first.localPlacement.getPosition();
            const auto target = second.targetPlacement.getPosition()
                - first.targetPlacement.getPosition();
            if (local.Length() > positionTolerance && target.Length() > positionTolerance) {
                directions.push_back(
                    {local.Normalized(), target.Normalized(), j + 1 < constraints.size()}
                );
            }
        }
    }
    return directions;
}

std::optional<std::pair<std::size_t, std::size_t>> RotationSolver::independentPair(
    const std::vector<Direction>& directions,
    bool lockedOnly
)
{
    std::optional<std::pair<std::size_t, std::size_t>> result;
    double bestIndependence = directionIndependenceTolerance;
    for (std::size_t i = 0; i < directions.size(); ++i) {
        if (lockedOnly && !directions[i].locked) {
            continue;
        }
        for (std::size_t j = i + 1; j < directions.size(); ++j) {
            if (lockedOnly && !directions[j].locked) {
                continue;
            }
            const auto independence = std::min(
                directions[i].local.Cross(directions[j].local).Length(),
                directions[i].target.Cross(directions[j].target).Length()
            );
            if (independence > bestIndependence) {
                bestIndependence = independence;
                result = std::pair {i, j};
            }
        }
    }
    return result;
}

Base::Rotation RotationSolver::solve() const
{
    const auto directions = collectDirections();
    auto pair = independentPair(directions, true);
    if (!pair) {
        pair = independentPair(directions, false);
    }
    if (pair) {
        const auto& primary = directions[pair->first];
        const auto& secondary = directions[pair->second];
        const auto local
            = Base::Rotation::makeRotationByAxes(secondary.local, Base::Vector3d {}, primary.local);
        const auto target
            = Base::Rotation::makeRotationByAxes(secondary.target, Base::Vector3d {}, primary.target);
        return target * local.inverse();
    }

    auto rotation = preferred;
    if (!directions.empty()) {
        const auto& direction = directions.front();
        rotation = Base::Rotation(rotation.multVec(direction.local), direction.target) * rotation;
    }
    return alignOffsets(rotation);
}

Base::Rotation RotationSolver::alignOffsets(const Base::Rotation& rotation) const
{
    for (const auto& primary : constraints) {
        if (primary.targetType != GeometryType::Axis) {
            continue;
        }
        const auto localAxis = zAxis(primary.localPlacement);
        const auto targetAxis = zAxis(primary.targetPlacement);
        for (const auto& secondary : constraints) {
            if (&secondary == &primary) {
                continue;
            }
            // Only the last entry is new, so every distinct pair includes a locked snap.
            if (secondary.targetType == GeometryType::Axis) {
                if (localAxis.Cross(zAxis(secondary.localPlacement)).Length()
                        > directionIndependenceTolerance
                    || targetAxis.Cross(zAxis(secondary.targetPlacement)).Length()
                        > directionIndependenceTolerance) {
                    continue;
                }
            }
            else if (secondary.targetType != GeometryType::Point) {
                continue;
            }

            auto currentOffset = rotation.multVec(
                secondary.localPlacement.getPosition() - primary.localPlacement.getPosition()
            );
            auto targetOffset = secondary.targetPlacement.getPosition()
                - primary.targetPlacement.getPosition();
            currentOffset.ProjectToPlane(Base::Vector3d {}, targetAxis);
            targetOffset.ProjectToPlane(Base::Vector3d {}, targetAxis);
            if (currentOffset.Length() < Base::Precision::Confusion()
                || targetOffset.Length() < Base::Precision::Confusion()) {
                continue;
            }
            currentOffset.Normalize();
            targetOffset.Normalize();
            const auto angle = std::atan2(
                targetAxis * currentOffset.Cross(targetOffset),
                currentOffset * targetOffset
            );
            return Base::Rotation(targetAxis, angle) * rotation;
        }
    }
    return rotation;
}

}  // namespace Gui::TransformSnap
