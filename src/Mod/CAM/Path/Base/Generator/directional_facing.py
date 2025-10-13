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
Directional (unidirectional) facing toolpath generator.

This module implements the unidirectional clearing pattern that cuts in the same
direction for every pass, providing consistent surface finish.
"""

import FreeCAD
import Path
from . import facing_common

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


def analyze_rectangle(polygon, axis_preference):
    """Analyze rectangle to determine orientation and dimensions."""
    # Extract polygon geometry
    polygon_info = facing_common.extract_polygon_geometry(polygon)
    edges = polygon_info["edges"]
    corners = polygon_info["corners"]

    # Get primary and step edges
    edge_info = facing_common.select_primary_step_edges(edges, axis_preference)

    # Find reference corner using dot product projections
    primary_vec = edge_info["primary_vec"]
    step_vec = edge_info["step_vec"]

    # Find the corner with minimum combined projection (the "origin" corner)
    min_projection = float("inf")
    reference_corner = corners[0]

    for corner in corners:
        # Calculate projections onto both direction vectors
        primary_proj = corner.dot(primary_vec)
        step_proj = corner.dot(step_vec)
        combined_proj = primary_proj + step_proj

        if combined_proj < min_projection:
            min_projection = combined_proj
            reference_corner = corner

    return {
        "primary_vec": primary_vec,
        "step_vec": step_vec,
        "primary_length": edge_info["primary_length"],
        "step_length": edge_info["step_length"],
        "reference_corner": reference_corner,
    }


def directional(
    polygon,
    tool_diameter,
    stepover_percent,
    pass_extension=None,
    retract_height=None,
    milling_direction="climb",
    reverse=False,
    angle_degrees=None,
):
    """
    Generate a unidirectional clearing pattern.

    This strategy cuts in the same direction for every pass across the polygon.
    After each cutting pass, the tool lifts up (rapid move) and repositions to
    the start of the next pass. All cutting moves go in the same direction,
    which can provide more consistent surface finish but requires more air time
    due to the rapid repositioning moves between passes.

    Args:
        polygon: The polygon boundary to clear (rectangular)
        tool_diameter: Diameter of the cutting tool
        stepover_percent: Stepover as percentage of tool diameter
        pass_extension: Distance to extend cuts beyond polygon boundary for tool disengagement
        retract_height: Z height for rapid moves (None = cutting height)
        milling_direction: "climb" or "conventional" - milling direction preference
        reverse: Reverse the cutting direction for the selected pattern.
        angle_degrees: Angle in degrees to rotate the cutting direction.

    Returns:
        List of Path.Command objects representing the toolpath. The list will begin with a G0 to the starting position.
        It is assumed that the calling function will replace the first move with an appropriate set of entry moves if needed
    """
    if pass_extension is None:
        pass_extension = tool_diameter * 0.5  # Default to half tool diameter

    import math

    # 1) Establish frame from angle (default 0° = +X primary)
    theta = float(angle_degrees) if angle_degrees is not None else 0.0
    primary_vec, step_vec = facing_common.unit_vectors_from_angle(theta)
    # Normalize defensively - make copies to avoid mutation
    primary_vec = FreeCAD.Vector(primary_vec).multiply(1.0 / (primary_vec.Length or 1.0))
    step_vec = FreeCAD.Vector(step_vec).multiply(1.0 / (step_vec.Length or 1.0))

    origin = polygon.BoundBox.Center
    bb = polygon.BoundBox
    diag = math.hypot(bb.XLength, bb.YLength)

    Path.Log.debug(
        f"Directional: dia={tool_diameter}, stepover%={stepover_percent}, "
        f"dir={milling_direction}, angle={theta}, reverse={reverse}"
    )

    # 2) Compute projection bounds
    min_s, max_s = facing_common.project_bounds(polygon, primary_vec, origin)
    min_t, max_t = facing_common.project_bounds(polygon, step_vec, origin)
    if not (
        math.isfinite(min_s)
        and math.isfinite(max_s)
        and math.isfinite(min_t)
        and math.isfinite(max_t)
    ):
        Path.Log.error("Directional: non-finite projection bounds; aborting")
        return []
    # 3) Steps along t
    step_positions = facing_common.generate_t_values(
        polygon, step_vec, tool_diameter, stepover_percent, origin
    )
    # Reverse order if requested (top-to-bottom instead of bottom-to-top)
    if reverse:
        # Mirror positions around center to maintain proper engagement at max_t
        # This both reverses the order AND maintains proper engagement
        center = (min_t + max_t) / 2.0
        step_positions = [(2 * center - t) for t in step_positions]
    Path.Log.debug(f"Directional: {len(step_positions)} passes generated")

    # 4) Slice and emit
    commands = []
    z = polygon.BoundBox.ZMin
    s_margin = pass_extension + tool_diameter
    kept_segments = 0
    skipped_segments = 0

    for t in step_positions:
        intervals = facing_common.slice_wire_segments(polygon, primary_vec, step_vec, t, origin)
        # If no intervals found (pass is outside polygon), skip this pass
        if not intervals:
            skipped_segments += 1
            continue
        for s0, s1 in intervals:
            # Extend in primary direction by pass_extension + tool_radius + engagement
            # This ensures the tool fully clears the polygon boundary
            tool_radius = tool_diameter / 2.0
            engagement_offset = facing_common.calculate_engagement_offset(
                tool_diameter, stepover_percent
            )
            total_extension = pass_extension + tool_radius + engagement_offset
            start_s = s0 - total_extension
            end_s = s1 + total_extension
            # Clip to safe bounds
            start_s = max(start_s, min_s - s_margin)
            end_s = min(end_s, max_s + s_margin)
            if end_s <= start_s:
                skipped_segments += 1
                continue

            # Determine cutting direction based on milling_direction and reverse
            # When direction of travel reverses, cutting direction must flip to maintain climb/conventional
            # Bottom-to-top: climb=right-to-left, conventional=left-to-right
            # Top-to-bottom: climb=left-to-right, conventional=right-to-left
            if milling_direction == "climb":
                if reverse:
                    p_start, p_end = start_s, end_s  # left-to-right for top-to-bottom
                else:
                    p_start, p_end = end_s, start_s  # right-to-left for bottom-to-top
            else:  # conventional
                if reverse:
                    p_start, p_end = end_s, start_s  # right-to-left for top-to-bottom
                else:
                    p_start, p_end = start_s, end_s  # left-to-right for bottom-to-top

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

            # Sanity check for absurdly large coordinates
            if not (
                math.isfinite(start_point.x)
                and math.isfinite(start_point.y)
                and math.isfinite(end_point.x)
                and math.isfinite(end_point.y)
            ):
                continue
            # Do NOT clamp XY after mapping; we already clipped s-range and t was selected from bbox.

            if commands:
                if retract_height is not None:
                    commands.append(Path.Command("G0", {"Z": retract_height}))
                    commands.append(Path.Command("G0", {"X": start_point.x, "Y": start_point.y}))
                    commands.append(Path.Command("G0", {"Z": z}))
                else:
                    commands.append(
                        Path.Command("G0", {"X": start_point.x, "Y": start_point.y, "Z": z})
                    )
            else:
                # First segment: emit a simple G0 to XYZ start position.
                # The operation will replace this with its own preamble sequence.
                commands.append(
                    Path.Command("G0", {"X": start_point.x, "Y": start_point.y, "Z": z})
                )

            commands.append(Path.Command("G1", {"X": end_point.x, "Y": end_point.y, "Z": z}))
            kept_segments += 1

    Path.Log.debug(f"Directional: generated {kept_segments} segments")
    # Fallback: if nothing kept due to numeric guards, emit a single mid-line pass across bbox
    if kept_segments == 0:
        t_candidates = []
        # mid, min, max t positions
        t_candidates.append(0.5 * (min_t + max_t))
        t_candidates.append(min_t)
        t_candidates.append(max_t)
        for t in t_candidates:
            intervals = facing_common.slice_wire_segments(polygon, primary_vec, step_vec, t, origin)
            if not intervals:
                continue
            s0, s1 = intervals[0]
            start_s = max(s0 - pass_extension, min_s - s_margin)
            end_s = min(s1 + pass_extension, max_s + s_margin)
            if end_s <= start_s:
                continue
            if milling_direction == "climb":
                p_start, p_end = start_s, end_s
            else:
                p_start, p_end = end_s, start_s
            if reverse:
                p_start, p_end = p_end, p_start
            sp = (
                FreeCAD.Vector(origin)
                .add(FreeCAD.Vector(primary_vec).multiply(p_start))
                .add(FreeCAD.Vector(step_vec).multiply(t))
            )
            ep = (
                FreeCAD.Vector(origin)
                .add(FreeCAD.Vector(primary_vec).multiply(p_end))
                .add(FreeCAD.Vector(step_vec).multiply(t))
            )
            sp.z = z
            ep.z = z
            # Minimal preamble
            if retract_height is not None:
                commands.append(Path.Command("G0", {"Z": retract_height}))
                commands.append(Path.Command("G0", {"X": sp.x, "Y": sp.y}))
                commands.append(Path.Command("G0", {"Z": z}))
            else:
                commands.append(Path.Command("G1", {"X": sp.x, "Y": sp.y, "Z": z}))
            commands.append(Path.Command("G1", {"X": ep.x, "Y": ep.y, "Z": z}))
            break
    return commands
