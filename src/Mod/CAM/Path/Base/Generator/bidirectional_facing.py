# -*- coding: utf-8 -*-
# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 sliptonic sliptonic@freecad.org                    *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************


"""
Bidirectional facing toolpath generator.

This module implements a bidirectional clearing pattern where passes alternate between
starting from the bottom edge and the top edge of the polygon, meeting in the middle.
Bottom passes step inward from min_t toward the center, while top passes step inward
from max_t toward the center. Passes are interleaved (bottom, top, bottom, top, etc.)
to minimize rapid move distances.

Feed moves (cutting) are aligned with the angle_degrees argument direction. Rapid moves
are perpendicular to the feed moves and always travel outside the clearing area along
the polygon edges.

This strategy always maintains either climb or conventional milling direction, but
alternates which side of the polygon is cut to maintain consistent milling direction
throughout.
"""

import FreeCAD
import Path
from . import facing_common
import math

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


def bidirectional(
    polygon,
    tool_diameter,
    stepover_percent,
    pass_extension=None,
    milling_direction="climb",
    reverse=False,
    angle_degrees=None,
):
    if pass_extension is None:
        pass_extension = tool_diameter * 0.5

    # Establish frame from angle (default 0° = +X primary)
    theta = float(angle_degrees) if angle_degrees is not None else 0.0
    primary_vec, step_vec = facing_common.unit_vectors_from_angle(theta)
    primary_vec = FreeCAD.Vector(primary_vec).normalize()
    step_vec = FreeCAD.Vector(step_vec).normalize()

    origin = polygon.BoundBox.Center
    bb = polygon.BoundBox
    z = bb.ZMin

    # Compute projection bounds
    min_s, max_s = facing_common.project_bounds(polygon, primary_vec, origin)
    min_t, max_t = facing_common.project_bounds(polygon, step_vec, origin)

    # ------------------------------------------------------------------
    # Use the proven generate_t_values (with coverage fix) for full coverage
    # ------------------------------------------------------------------
    step_positions = facing_common.generate_t_values(
        polygon, step_vec, tool_diameter, stepover_percent, origin
    )

    tool_radius = tool_diameter / 2.0
    stepover_distance = tool_diameter * stepover_percent / 100.0

    # Coverage guarantee at ≥100% stepover (exact same fix as zigzag/directional)
    if stepover_percent >= 99.9 and step_positions:
        min_covered = min(step_positions) - tool_radius
        max_covered = max(step_positions) + tool_radius

        added = False
        if max_covered < max_t - 1e-4:
            step_positions.append(step_positions[-1] + stepover_distance)
            added = True
        if min_covered > min_t + 1e-4:
            step_positions.insert(0, step_positions[0] - stepover_distance)
            added = True
        if added:
            Path.Log.info(
                "Bidirectional facing: Added extra pass(es) for full coverage at ≥100% stepover"
            )

    center = (min_t + max_t) / 2.0

    # Split into bottom (≤ center) and top (> center)
    bottom_positions = [t for t in step_positions if t <= center]  # ascending = outer → inner
    top_positions = [t for t in step_positions if t > center][::-1]  # descending = outer → inner

    # Interleave, starting with top if reverse=True
    all_passes = []
    max_passes = max(len(bottom_positions), len(top_positions))
    for i in range(max_passes):
        if reverse:
            if i < len(top_positions):
                all_passes.append(("top", top_positions[i]))
            if i < len(bottom_positions):
                all_passes.append(("bottom", bottom_positions[i]))
        else:
            if i < len(bottom_positions):
                all_passes.append(("bottom", bottom_positions[i]))
            if i < len(top_positions):
                all_passes.append(("top", top_positions[i]))

    Path.Log.debug(
        f"Bidirectional: {len(all_passes)} passes ({len(bottom_positions)} bottom, {len(top_positions)} top)"
    )

    commands = []
    tool_radius = tool_diameter / 2.0
    engagement_offset = facing_common.calculate_engagement_offset(tool_diameter, stepover_percent)
    total_extension = pass_extension + tool_radius + engagement_offset

    start_s = min_s - total_extension
    end_s = max_s + total_extension

    for side, t in all_passes:
        # Same direction for all passes on the same side → short outside rapids
        if side == "bottom":
            if milling_direction == "climb":
                p_start, p_end = end_s, start_s  # right → left
            else:
                p_start, p_end = start_s, end_s  # left → right
        else:  # top
            if milling_direction == "climb":
                p_start, p_end = start_s, end_s  # left → right
            else:
                p_start, p_end = end_s, start_s  # right → left

        start_point = origin + primary_vec * p_start + step_vec * t
        end_point = origin + primary_vec * p_end + step_vec * t
        start_point.z = z
        end_point.z = z

        if not all(
            math.isfinite(c) for c in [start_point.x, start_point.y, end_point.x, end_point.y]
        ):
            continue

        if commands:
            # Short perpendicular rapid at cutting height (outside the material)
            commands.append(Path.Command("G0", {"X": start_point.x, "Y": start_point.y}))
        else:
            # First pass – include Z for preamble replacement
            commands.append(Path.Command("G0", {"X": start_point.x, "Y": start_point.y, "Z": z}))

        commands.append(Path.Command("G1", {"X": end_point.x, "Y": end_point.y, "Z": z}))

    return commands
