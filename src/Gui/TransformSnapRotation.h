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

#include "TransformSnap.h"

#include <optional>
#include <utility>
#include <vector>

namespace Gui::TransformSnap
{

/** Choose a rotation for one set of resolved target axis/normal signs.
 * Earlier constraints take priority when selecting independent directions.
 * One direction leaves twist free; two independent directions fix rotation.
 * Offsets between parallel axes or points can resolve the remaining twist.
 * The placement solver must verify the resulting rotation against all snaps.
 * The constraints must outlive this solver.
 */
class RotationSolver
{
public:
    RotationSolver(const Base::Rotation& preferred, const std::vector<Constraint>& constraints);
    Base::Rotation solve() const;

private:
    struct Direction
    {
        Base::Vector3d local;
        Base::Vector3d target;
        bool locked;
    };

    std::vector<Direction> collectDirections() const;
    static std::optional<std::pair<std::size_t, std::size_t>> independentPair(
        const std::vector<Direction>& directions,
        bool lockedOnly
    );
    Base::Rotation alignOffsets(const Base::Rotation& rotation) const;

    Base::Rotation preferred;
    const std::vector<Constraint>& constraints;
};

}  // namespace Gui::TransformSnap
