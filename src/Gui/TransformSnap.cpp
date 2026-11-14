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

#include <numbers>

namespace Gui::TransformSnap
{

Base::Vector3d zAxis(const Base::Placement& placement)
{
    return placement.getRotation().multVec(Base::Vector3d::UnitZ).Normalized();
}

Base::Placement invertedPlacementAroundLocalAxis(Base::Placement placement, const Base::Vector3d& localAxis)
{
    placement.setRotation(placement.getRotation() * Base::Rotation(localAxis, std::numbers::pi));
    return placement;
}

namespace
{

Base::Rotation rotationAligningDirectionNear(
    const Base::Rotation& current,
    const Base::Vector3d& localDirection,
    const Base::Vector3d& targetDirection,
    bool unorientedTarget
)
{
    const auto currentDirection = current.multVec(localDirection.Normalized());
    auto target = targetDirection.Normalized();
    if (unorientedTarget && currentDirection * target < 0.0) {
        target *= -1.0;
    }
    return Base::Rotation(currentDirection, target) * current;
}

}  // namespace

Base::Placement objectPlacementMatchingSnapFrame(
    Base::Placement objectPlacement,
    const Base::Placement& referenceLocalPlacement,
    const Base::Placement& targetPlacement
)
{
    objectPlacement.setRotation(
        targetPlacement.getRotation() * referenceLocalPlacement.getRotation().inverse()
    );
    return objectPlacement;
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
                referencePosition.Perpendicular(targetPlacement.getPosition(), zAxis(targetPlacement))
            );
            break;
        }
        case GeometryType::Plane: {
            objectPlacement.setRotation(rotationAligningDirectionNear(
                objectPlacement.getRotation(),
                zAxis(referenceLocalPlacement),
                zAxis(targetPlacement),
                false
            ));
            referencePlacement = objectPlacement * referenceLocalPlacement;
            auto position = referencePlacement.getPosition();
            position.ProjectToPlane(targetPlacement.getPosition(), zAxis(targetPlacement));
            referencePlacement.setPosition(position);
            break;
        }
        case GeometryType::Point:
            referencePlacement.setPosition(targetPlacement.getPosition());
            break;
        default:
            return objectPlacement;
    }
    return referencePlacement * referenceLocalPlacement.inverse();
}

}  // namespace Gui::TransformSnap
