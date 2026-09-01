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
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <Base/Tools2D.h>
#include <Mod/Sketcher/App/GeoEnum.h>
#include <Mod/Sketcher/SketcherGlobal.h>

#include <memory>
#include <optional>
#include <vector>

namespace Sketcher
{
class SketchObject;
class Constraint;
enum ConstraintType : int;
}  // namespace Sketcher

namespace SketcherGui
{

/// Legacy datum label-position sentinel used to request automatic placement.
inline constexpr double kAutoDatumLabelPosition = 10.0;

[[nodiscard]] inline bool isAutoDatumLabelPosition(double labelPosition)
{
    return labelPosition == kAutoDatumLabelPosition;
}

/// Existing Sketcher identifier for a geometry and one of its optional points.
using DimensionReference = Sketcher::GeoElementId;

/// Complete, UI-independent description of one dimension candidate.
struct DimensionOption
{
    struct DatumPlacement
    {
        double labelDistance {0.0};
        double labelPosition {0.0};
    };

    Sketcher::ConstraintType constraintType {};
    std::vector<DimensionReference> refs;
    std::optional<Base::Vector2d> customLabelPosition;
    std::optional<DatumPlacement> preparedDatumPlacement;
};

enum class CircleDistanceMode
{
    Minimal,
    Signed,
};

/**
 * @brief Build the Sketcher constraint represented by a dimension candidate.
 * @param sketch Sketch containing the referenced geometry.
 * @param option Candidate to convert.
 * @param circleDistanceMode Circle-circle value convention required by the caller.
 * @return A constraint, or `nullptr` when the candidate is invalid.
 */
[[nodiscard]] SketcherGuiExport std::unique_ptr<Sketcher::Constraint> buildDimensionConstraint(
    const Sketcher::SketchObject& sketch,
    const DimensionOption& option,
    CircleDistanceMode circleDistanceMode = CircleDistanceMode::Minimal
);

/**
 * @brief Return the valid, non-duplicate dimension candidates for an ordered selection.
 * @param sketch Sketch containing the selection.
 * @param selectionRefs Geometry references in selection order.
 * @return Candidate dimensions not already present in the sketch.
 */
[[nodiscard]] SketcherGuiExport std::vector<DimensionOption> buildDimensionOptions(
    Sketcher::SketchObject* sketch,
    const std::vector<DimensionReference>& selectionRefs
);

/**
 * @brief Add a dimension candidate to the sketch in one undoable command.
 * @param sketch Sketch to modify.
 * @param option Candidate to commit.
 * @return `true` when the constraint was created successfully.
 */
SketcherGuiExport bool commitDimensionOption(
    Sketcher::SketchObject& sketch,
    const DimensionOption& option
);

}  // namespace SketcherGui
