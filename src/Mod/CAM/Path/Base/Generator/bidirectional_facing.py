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
    """
    Generate a bidirectional clearing pattern using scanline intersection.

    This strategy cuts back and forth across the polygon, alternating which side
    is cut to maintain consistent milling direction. Rapids between passes stay
    at cutting height for efficiency.

    Args:
        polygon: The polygon boundary to clear
        tool_diameter: Diameter of the cutting tool
        stepover_percent: Stepover as percentage of tool diameter
        pass_extension: Distance to extend cuts beyond polygon boundary
        milling_direction: "climb" or "conventional"
        reverse: Reverse the alternation pattern
        angle_degrees: Angle in degrees to rotate the cutting direction

    Returns:
        List of Path.Command objects representing the toolpath. First move is G0 to start position.
    """
    if pass_extension is None:
        pass_extension = tool_diameter * 0.5

    import math

    # Establish frame from angle (default 0° = +X primary)
    theta = float(angle_degrees) if angle_degrees is not None else 0.0
    primary_vec, step_vec = facing_common.unit_vectors_from_angle(theta)
    # Make copies to avoid mutation
    primary_vec = FreeCAD.Vector(primary_vec).multiply(1.0 / (primary_vec.Length or 1.0))
    step_vec = FreeCAD.Vector(step_vec).multiply(1.0 / (step_vec.Length or 1.0))

    origin = polygon.BoundBox.Center
    bb = polygon.BoundBox
    z = bb.ZMin

    Path.Log.debug(
        f"Bidirectional: dia={tool_diameter}, stepover%={stepover_percent}, "
        f"dir={milling_direction}, angle={theta}, reverse={reverse}"
    )

    # Compute projection bounds
    min_s, max_s = facing_common.project_bounds(polygon, primary_vec, origin)
    min_t, max_t = facing_common.project_bounds(polygon, step_vec, origin)
    if not (
        math.isfinite(min_s)
        and math.isfinite(max_s)
        and math.isfinite(min_t)
        and math.isfinite(max_t)
    ):
        Path.Log.error("Bidirectional: non-finite projection bounds; aborting")
        return []

    # Generate step positions for BOTH sides with proper engagement offsets
    tool_radius = tool_diameter / 2.0
    engagement_amount = tool_diameter * (stepover_percent / 100.0)
    stepover = tool_diameter * (stepover_percent / 100.0)

    # Calculate the midpoint - passes should stop here
    t_mid = (min_t + max_t) / 2.0

    # Bottom positions: start at min_t with engagement offset, stop at midpoint
    bottom_positions = []
    t = min_t - tool_radius + engagement_amount
    while t <= t_mid:
        bottom_positions.append(t)
        t += stepover

    # Top positions: start at max_t with engagement offset, stop at midpoint
    top_positions = []
    t = max_t + tool_radius - engagement_amount
    while t >= t_mid:
        top_positions.append(t)
        t -= stepover

    # Interleave bottom and top positions
    # reverse controls which side starts first
    all_passes = []
    max_passes = max(len(bottom_positions), len(top_positions))
    for i in range(max_passes):
        if not reverse:
            # Start with bottom
            if i < len(bottom_positions):
                all_passes.append(("bottom", bottom_positions[i]))
            if i < len(top_positions):
                all_passes.append(("top", top_positions[i]))
        else:
            # Start with top
            if i < len(top_positions):
                all_passes.append(("top", top_positions[i]))
            if i < len(bottom_positions):
                all_passes.append(("bottom", bottom_positions[i]))

    Path.Log.debug(
        f"Bidirectional: {len(all_passes)} passes generated ({len(bottom_positions)} bottom, {len(top_positions)} top)"
    )

    commands = []
    s_margin = pass_extension + tool_diameter
    kept_segments = 0
    engagement_offset = facing_common.calculate_engagement_offset(tool_diameter, stepover_percent)

    for side, t in all_passes:
        # Cut full-length passes from min_s to max_s (with extensions)
        # This ensures all passes have the same length and endpoints align
        total_extension = pass_extension + tool_radius + engagement_offset
        start_s = min_s - total_extension
        end_s = max_s + total_extension

        # Determine cutting direction based on side and milling_direction
        # Bottom and top must cut in OPPOSITE directions to maintain perpendicular rapids
        # This keeps rapids outside the clearing area
        if side == "bottom":
            if milling_direction == "climb":
                p_start, p_end = end_s, start_s  # right-to-left from bottom
            else:  # conventional
                p_start, p_end = start_s, end_s  # left-to-right from bottom
        else:  # top
            if milling_direction == "climb":
                p_start, p_end = start_s, end_s  # left-to-right from top (opposite)
            else:  # conventional
                p_start, p_end = end_s, start_s  # right-to-left from top (opposite)

        # Map to XY - use copies to avoid vector mutation
        start_point = (
            FreeCAD.Vector(origin)
            .add(FreeCAD.Vector(primary_vec).multiply(p_start))
            .add(FreeCAD.Vector(step_vec).multiply(t))
        )
        end_point = (
            FreeCAD.Vector(origin)
            .add(FreeCAD.Vector(primary_vec).multiply(p_end))
            .add(FreeCAD.Vector(step_vec).multiply(t))
        )
        start_point.z = z
        end_point.z = z

        # Finite check
        if not (
            math.isfinite(start_point.x)
            and math.isfinite(start_point.y)
            and math.isfinite(end_point.x)
            and math.isfinite(end_point.y)
        ):
            continue

        if commands:
            # Rapid perpendicular to feed direction (along step_vec)
            # Since all passes have same s-coordinates, rapids only move in t direction
            # Just rapid from end of last pass to start of this pass
            commands.append(Path.Command("G0", {"X": start_point.x, "Y": start_point.y}))
        else:
            # First segment: emit G0 to start for op preamble replacement
            commands.append(Path.Command("G0", {"X": start_point.x, "Y": start_point.y, "Z": z}))

        commands.append(Path.Command("G1", {"X": end_point.x, "Y": end_point.y, "Z": z}))
        kept_segments += 1

    Path.Log.debug(f"Bidirectional: generated {kept_segments} segments")
    return commands
