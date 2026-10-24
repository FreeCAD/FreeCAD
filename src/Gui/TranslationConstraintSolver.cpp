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

#include "TranslationConstraintSolver.h"

#include <Base/Precision.h>
#include <Eigen/SVD>

#include <cmath>

namespace Gui
{

void TranslationConstraintSolver::addEquation(const Base::Vector3d& normal, double offset)
{
    equations.push_back({normal, offset});
}

std::optional<Base::Vector3d> TranslationConstraintSolver::solve(
    const Base::Vector3d& preferred,
    double positionTolerance
) const
{
    if (!preferred.isFinite()) {
        return std::nullopt;
    }
    if (equations.empty()) {
        return preferred;
    }

    const auto count = static_cast<Eigen::Index>(equations.size());
    Eigen::MatrixXd normals(count, 3);
    Eigen::VectorXd offsets(count);
    for (Eigen::Index row = 0; row < count; ++row) {
        const auto& equation = equations[row];
        const auto length = equation.normal.Length();
        if (!std::isfinite(length) || length < Base::Precision::Confusion()
            || !std::isfinite(equation.offset)) {
            return std::nullopt;
        }
        const auto normal = equation.normal / length;
        normals.row(row) << normal.x, normal.y, normal.z;
        offsets[row] = equation.offset / length;
    }
    if (!normals.allFinite() || !offsets.allFinite()) {
        return std::nullopt;
    }

    const Eigen::Vector3d preferredPosition(preferred.x, preferred.y, preferred.z);
    const Eigen::VectorXd residual = offsets - normals * preferredPosition;
    if (!residual.allFinite()) {
        return std::nullopt;
    }

    Eigen::JacobiSVD<Eigen::MatrixXd> decomposition(normals, Eigen::ComputeThinU | Eigen::ComputeThinV);
    decomposition.setThreshold(Base::Precision::Confusion());
    const Eigen::Vector3d correction = decomposition.solve(residual);
    const Eigen::Vector3d position = preferredPosition + correction;
    // SVD also produces least-squares fits for conflicting snaps. Accept only
    // positions that satisfy every equation within the snap distance tolerance.
    if (!position.allFinite()
        || (normals * position - offsets).lpNorm<Eigen::Infinity>() > positionTolerance) {
        return std::nullopt;
    }
    return Base::Vector3d(position.x(), position.y(), position.z());
}

}  // namespace Gui
