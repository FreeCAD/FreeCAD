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
Spiral facing toolpath generator.

This module implements the spiral clearing pattern for rectangular polygons,
including support for angled rectangles and proper tool engagement.
"""

import FreeCAD
import Path
from . import facing_common

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


def generate_spiral_corners(
    start_corner, primary_vec, step_vec, primary_length, step_length, inward_offset
):
    """
    Generate the four corners of a spiral layer offset inward from the original polygon.

    The start_corner is assumed to be the corner with minimum combined projection
    (bottom-left in the primary/step coordinate system). The offset moves inward
    by adding positive offsets in both primary and step directions.
    """
    # Calculate the four corners of this layer (reduced by inward offset)
    adjusted_primary_length = max(0, primary_length - 2 * inward_offset)
    adjusted_step_length = max(0, step_length - 2 * inward_offset)

    # Move the starting corner inward by the offset amount
    # Since start_corner is the minimum projection corner, we move inward by adding offsets
    inward_primary = FreeCAD.Vector(primary_vec).multiply(inward_offset)
    inward_step = FreeCAD.Vector(step_vec).multiply(inward_offset)

    # The actual starting corner for this layer is offset inward
    layer_start_corner = FreeCAD.Vector(start_corner).add(inward_primary).add(inward_step)

    # Build rectangle from the offset starting corner with reduced dimensions
    corner1 = FreeCAD.Vector(layer_start_corner)
    corner2 = FreeCAD.Vector(corner1).add(
        FreeCAD.Vector(primary_vec).multiply(adjusted_primary_length)
    )
    corner3 = FreeCAD.Vector(corner2).add(FreeCAD.Vector(step_vec).multiply(adjusted_step_length))
    corner4 = FreeCAD.Vector(corner3).add(
        FreeCAD.Vector(primary_vec).multiply(-adjusted_primary_length)
    )

    return [corner1, corner2, corner3, corner4]


def generate_layer_path(
    layer_corners,
    next_layer_start,
    layer_num,
    z,
    clockwise,
    start_corner_index=0,
    is_last_layer=False,
):
    """
    Generate the toolpath commands for a single spiral layer.

    For a true spiral, we do all 4 sides of the rectangle, but the 4th side only goes
    partway - it stops at the starting position of the next layer. This creates the
    continuous spiral effect.
    """
    commands = []

    # Set Z coordinate for all corners
    for corner in layer_corners:
        corner.z = z

    # For the first layer, start with a rapid move to the starting corner
    if layer_num == 0:
        commands.append(
            Path.Command(
                "G0",
                {
                    "X": layer_corners[start_corner_index].x,
                    "Y": layer_corners[start_corner_index].y,
                    "Z": z,
                },
            )
        )

    # Generate the path: go around all 4 sides
    if clockwise:
        # Clockwise: start_corner -> corner 1 -> corner 2 -> corner 3 -> back toward start
        for i in range(1, 4):
            corner_idx = (start_corner_index + i) % 4
            commands.append(
                Path.Command(
                    "G1",
                    {"X": layer_corners[corner_idx].x, "Y": layer_corners[corner_idx].y, "Z": z},
                )
            )

        # 4th side: go back toward start, but stop at next layer's starting position
        if not is_last_layer and next_layer_start:
            next_layer_start.z = z
            commands.append(
                Path.Command("G1", {"X": next_layer_start.x, "Y": next_layer_start.y, "Z": z})
            )
        else:
            # Last layer: complete the rectangle
            commands.append(
                Path.Command(
                    "G1",
                    {
                        "X": layer_corners[start_corner_index].x,
                        "Y": layer_corners[start_corner_index].y,
                        "Z": z,
                    },
                )
            )
    else:
        # Counter-clockwise: start_corner -> corner 3 -> corner 2 -> corner 1 -> back toward start
        for i in range(1, 4):
            corner_idx = (start_corner_index - i) % 4
            commands.append(
                Path.Command(
                    "G1",
                    {"X": layer_corners[corner_idx].x, "Y": layer_corners[corner_idx].y, "Z": z},
                )
            )

        # 4th side: go back toward start, but stop at next layer's starting position
        if not is_last_layer and next_layer_start:
            next_layer_start.z = z
            commands.append(
                Path.Command("G1", {"X": next_layer_start.x, "Y": next_layer_start.y, "Z": z})
            )
        else:
            # Last layer: complete the rectangle
            commands.append(
                Path.Command(
                    "G1",
                    {
                        "X": layer_corners[start_corner_index].x,
                        "Y": layer_corners[start_corner_index].y,
                        "Z": z,
                    },
                )
            )

    return commands


def spiral(
    polygon,
    tool_diameter,
    stepover_percent,
    milling_direction="climb",
    reverse=False,
    angle_degrees=None,
):
    """
    Generate a spiral clearing pattern for rectangular polygons with guaranteed full coverage.

    The radial stepover is automatically adjusted (slightly if required to ensure the tool edge reaches exactly the center
    in the limiting direction. This eliminates any uncleared areas in the center regardless of stepover% value.
    First engagement is preserved exactly at the requested percentage.
    """
    import math
    import Path
    import FreeCAD
    from . import facing_common

    theta = float(angle_degrees) if angle_degrees is not None else 0.0
    primary_vec, step_vec = facing_common.unit_vectors_from_angle(theta)
    primary_vec = FreeCAD.Vector(primary_vec).normalize()
    step_vec = FreeCAD.Vector(step_vec).normalize()

    polygon_info = facing_common.extract_polygon_geometry(polygon)
    corners = polygon_info["corners"]

    origin = facing_common.select_starting_corner(corners, primary_vec, step_vec, "climb")
    min_s, max_s = facing_common.project_bounds(polygon, primary_vec, origin)
    min_t, max_t = facing_common.project_bounds(polygon, step_vec, origin)

    primary_length = max_s - min_s
    step_length = max_t - min_t

    tool_radius = tool_diameter / 2.0
    stepover_dist = tool_diameter * (stepover_percent / 100.0)
    if stepover_dist > tool_diameter * 1.000001:
        stepover_dist = tool_diameter

    # Calculate adjusted stepover to guarantee center coverage
    starting_inset = tool_radius - stepover_dist
    limiting_half = min(primary_length, step_length) / 2.0
    total_radial_distance = limiting_half - tool_radius - starting_inset

    if total_radial_distance <= 0:
        actual_stepover = stepover_dist
    else:
        number_of_intervals = math.ceil(total_radial_distance / stepover_dist)
        actual_stepover = total_radial_distance / number_of_intervals

    Path.Log.debug(
        f"Spiral: adjusted stepover {stepover_dist:.4f} → {actual_stepover:.4f} mm, intervals={number_of_intervals if total_radial_distance > 0 else 0}"
    )

    # Standard initial_offset (preserves first engagement exactly)
    initial_offset = -tool_radius + stepover_dist

    z = polygon.BoundBox.ZMin

    clockwise = milling_direction == "conventional"

    start_corner_index = 0 if clockwise else 2
    if reverse:
        start_corner_index = (start_corner_index + 2) % 4

    commands = []
    k = 0
    first_move_done = False

    while True:
        current_offset = initial_offset + k * actual_stepover

        s0 = min_s + current_offset
        s1 = max_s - current_offset
        t0 = min_t + current_offset
        t1 = max_t - current_offset

        if s0 >= s1 or t0 >= t1:
            break

        corners_st = [(s0, t0), (s1, t0), (s1, t1), (s0, t1)]

        if clockwise:
            order = [(start_corner_index + i) % 4 for i in range(4)]
        else:
            order = [(start_corner_index - i) % 4 for i in range(4)]

        def st_to_xy(s, t):
            return origin + primary_vec * s + step_vec * t

        start_idx = order[0]
        start_xy = st_to_xy(*corners_st[start_idx])
        start_xy.z = z

        if not first_move_done:
            commands.append(Path.Command("G0", {"X": start_xy.x, "Y": start_xy.y, "Z": z}))
            first_move_done = True

        # Sides 1-3: full
        for i in range(1, 4):
            c_xy = st_to_xy(*corners_st[order[i]])
            c_xy.z = z
            commands.append(Path.Command("G1", {"X": c_xy.x, "Y": c_xy.y, "Z": z}))

        # Prepare transition to next layer (partial 4th side)
        next_offset = current_offset + actual_stepover
        s0n = min_s + next_offset
        s1n = max_s - next_offset
        t0n = min_t + next_offset
        t1n = max_t - next_offset

        if s0n < s1n and t0n < t1n:
            # Determine which edge we are on for the 4th side and compute intersection
            # the transition point on that edge
            if clockwise:
                if start_corner_index == 0:
                    transition_xy = st_to_xy(s0, t0n)
                elif start_corner_index == 1:
                    transition_xy = st_to_xy(s1n, t0)
                elif start_corner_index == 2:
                    transition_xy = st_to_xy(s1, t1n)
                else:
                    transition_xy = st_to_xy(s0n, t1)
            else:  # counter-clockwise
                if start_corner_index == 0:
                    transition_xy = st_to_xy(s0n, t0)
                elif start_corner_index == 1:
                    transition_xy = st_to_xy(s1, t0n)
                elif start_corner_index == 2:
                    transition_xy = st_to_xy(s1n, t1)
                else:
                    transition_xy = st_to_xy(s0, t1n)

            transition_xy.z = z
            commands.append(
                Path.Command("G1", {"X": transition_xy.x, "Y": transition_xy.y, "Z": z})
            )
            k += 1
        else:
            # Final layer - close back to start
            commands.append(Path.Command("G1", {"X": start_xy.x, "Y": start_xy.y, "Z": z}))
            break

    return commands
