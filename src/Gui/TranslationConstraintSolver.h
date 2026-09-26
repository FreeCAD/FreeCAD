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

#include <Base/Vector3D.h>

#include <optional>
#include <vector>

namespace Gui
{

/** Find the nearest translation satisfying a set of linear position constraints.
 *
 * Each equation is normal.dot(position) = offset. A plane supplies one equation,
 * an axis two, and a point three. Eigen's Jacobi SVD solves A * correction =
 * offsets - A * preferred with the minimum correction norm, retaining the
 * preferred position in unconstrained directions. Dependent equations are
 * allowed; inconsistent equations cause solve() to return no result.
 */
class TranslationConstraintSolver
{
public:
    void addEquation(const Base::Vector3d& normal, double offset);
    std::optional<Base::Vector3d> solve(const Base::Vector3d& preferred, double positionTolerance) const;

private:
    struct Equation
    {
        Base::Vector3d normal;
        double offset;
    };
    std::vector<Equation> equations;
};

}  // namespace Gui
