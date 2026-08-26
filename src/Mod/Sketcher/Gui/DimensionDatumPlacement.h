// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Turan Furkan Topak                                 *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the          *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA 02111-1307, USA                                 *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <Base/Tools2D.h>
#include <Base/Vector3D.h>
#include <Mod/Sketcher/App/GeoList.h>

#include <optional>

namespace Sketcher
{
class Constraint;
class SketchObject;
}  // namespace Sketcher

namespace SketcherGui
{

struct DimensionDatumEndpoints
{
    Base::Vector3d first;
    Base::Vector3d second;
};

/**
 * @brief Resolve the geometry endpoints used to display a dimensional constraint.
 * @param geometry Geometry source used to render the constraint.
 * @param constraint Dimensional constraint whose references are resolved.
 * @param labelPosition Label position used to choose a radial direction when needed.
 * @return The resolved endpoints, or no value for an unsupported or degenerate reference set.
 */
[[nodiscard]] std::optional<DimensionDatumEndpoints> resolveDimensionDatumEndpoints(
    const Sketcher::GeoListFacade& geometry,
    const Sketcher::Constraint& constraint,
    const Base::Vector2d& labelPosition
);

/// Return Sketcher's established automatic label position for a dimensional constraint.
[[nodiscard]] std::optional<Base::Vector2d> defaultDimensionDatumLabelPosition(
    const Sketcher::SketchObject& sketch,
    const Sketcher::Constraint& constraint
);

/**
 * @brief Compute datum label parameters for an explicit point in sketch coordinates.
 * @param sketch Sketch containing the referenced geometry.
 * @param constraint Constraint whose label parameters are updated.
 * @param labelPosition Requested label position in sketch coordinates.
 * @param labelOffset Additional spacing from the requested label position.
 * @return `true` when a valid placement was computed.
 */
bool prepareDimensionDatumPlacement(
    const Sketcher::SketchObject& sketch,
    Sketcher::Constraint& constraint,
    const Base::Vector2d& labelPosition,
    double labelOffset = 0.0
);

/// Prepare the default label position with Sketcher's established placement rules.
bool prepareDimensionDatumPlacement(
    const Sketcher::SketchObject& sketch,
    Sketcher::Constraint& constraint
);

}  // namespace SketcherGui
