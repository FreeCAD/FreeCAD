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

#pragma once

#include <App/Services.h>
#include <Base/Placement.h>

#include <optional>
#include <vector>

namespace Gui::TransformSnap
{

using GeometryType = App::SubObjectPlacementProvider::SnapGeometryType;

Base::Vector3d zAxis(const Base::Placement& placement);
Base::Placement invertedPlacementAroundLocalAxis(
    Base::Placement placement,
    const Base::Vector3d& localAxis
);
Base::Placement objectPlacementMatchingSnapFrame(
    Base::Placement objectPlacement,
    const Base::Placement& referenceLocalPlacement,
    const Base::Placement& targetPlacement,
    const Base::Vector3d& localAxis
);
bool isFinitePlacement(const Base::Placement& placement);
bool isCompatible(GeometryType referenceType, GeometryType targetType);
Base::Placement preferredPlacement(
    Base::Placement objectPlacement,
    const Base::Placement& referenceLocalPlacement,
    const Base::Placement& targetPlacement,
    GeometryType targetType
);

struct Constraint
{
    Base::Placement localPlacement;
    Base::Placement targetPlacement;
    GeometryType referenceType;
    GeometryType targetType;
    // Otherwise, the solver may reverse the target axis or normal to choose an orientation.
    bool targetDirectionSignFixed {false};
};

// Earlier entries are locked snaps; the final entry is the proposed snap.
std::optional<Base::Placement> solve(
    const Base::Placement& candidate,
    const std::vector<Constraint>& constraints
);

}  // namespace Gui::TransformSnap
