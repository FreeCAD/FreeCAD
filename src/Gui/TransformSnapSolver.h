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
#include <vector>

namespace Gui::TransformSnap
{

/** Preserve earlier snaps while positioning a single object in the Transform task.
 *
 * Earlier constraints are locked geometry; the last entry is the proposed snap.
 * For each permitted target direction sign, solve rotation, project translation
 * onto the linear constraints, and verify every snap. Free degrees of freedom
 * stay near the preferred placement; conflicting snaps return no result.
 *
 * This task-local calculation creates neither attachment properties nor assembly
 * joints. Part's attacher derives a placement from a selected attachment mode;
 * cumulative snapping instead preserves constraints gathered during interaction.
 * Keeping this in Gui avoids a Core dependency on Part or Assembly. The linear
 * equations themselves are handled by TranslationConstraintSolver using Eigen.
 */
class PlacementSolver
{
public:
    PlacementSolver(const Base::Placement& preferred, std::vector<Constraint> constraints);
    std::optional<Base::Placement> solve() const;

private:
    static bool isUsableFrame(const Base::Placement& placement);
    bool hasValidInput() const;
    std::vector<Constraint> orientTargets(
        const std::vector<std::size_t>& unorientedIndices,
        std::size_t variant
    ) const;
    std::optional<Base::Placement> solveDirected(const std::vector<Constraint>& directed) const;
    std::optional<Base::Vector3d> solveTranslation(
        const Base::Rotation& rotation,
        const std::vector<Constraint>& directed
    ) const;
    static bool satisfiesConstraint(const Base::Placement& placement, const Constraint& constraint);
    double placementScore(const Base::Placement& placement) const;

    Base::Placement preferred;
    std::vector<Constraint> constraints;
};

}  // namespace Gui::TransformSnap
