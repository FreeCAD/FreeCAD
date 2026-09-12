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

}  // namespace Gui::TransformSnap
