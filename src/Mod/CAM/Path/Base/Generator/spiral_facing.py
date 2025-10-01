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



def generate_spiral_corners(start_corner, primary_vec, step_vec, primary_length, step_length, inward_offset):
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
    corner2 = FreeCAD.Vector(corner1).add(FreeCAD.Vector(primary_vec).multiply(adjusted_primary_length))
    corner3 = FreeCAD.Vector(corner2).add(FreeCAD.Vector(step_vec).multiply(adjusted_step_length))
    corner4 = FreeCAD.Vector(corner3).add(FreeCAD.Vector(primary_vec).multiply(-adjusted_primary_length))
    
    return [corner1, corner2, corner3, corner4]


def generate_layer_path(layer_corners, next_layer_start, layer_num, z, clockwise, start_corner_index=0, is_last_layer=False):
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
        commands.append(Path.Command("G0", {"X": layer_corners[start_corner_index].x, "Y": layer_corners[start_corner_index].y, "Z": z}))
    
    # Generate the path: go around all 4 sides
    if clockwise:
        # Clockwise: start_corner -> corner 1 -> corner 2 -> corner 3 -> back toward start
        for i in range(1, 4):
            corner_idx = (start_corner_index + i) % 4
            commands.append(Path.Command("G1", {"X": layer_corners[corner_idx].x, "Y": layer_corners[corner_idx].y, "Z": z}))
        
        # 4th side: go back toward start, but stop at next layer's starting position
        if not is_last_layer and next_layer_start:
            next_layer_start.z = z
            commands.append(Path.Command("G1", {"X": next_layer_start.x, "Y": next_layer_start.y, "Z": z}))
        else:
            # Last layer: complete the rectangle
            commands.append(Path.Command("G1", {"X": layer_corners[start_corner_index].x, "Y": layer_corners[start_corner_index].y, "Z": z}))
    else:
        # Counter-clockwise: start_corner -> corner 3 -> corner 2 -> corner 1 -> back toward start
        for i in range(1, 4):
            corner_idx = (start_corner_index - i) % 4
            commands.append(Path.Command("G1", {"X": layer_corners[corner_idx].x, "Y": layer_corners[corner_idx].y, "Z": z}))
        
        # 4th side: go back toward start, but stop at next layer's starting position
        if not is_last_layer and next_layer_start:
            next_layer_start.z = z
            commands.append(Path.Command("G1", {"X": next_layer_start.x, "Y": next_layer_start.y, "Z": z}))
        else:
            # Last layer: complete the rectangle
            commands.append(Path.Command("G1", {"X": layer_corners[start_corner_index].x, "Y": layer_corners[start_corner_index].y, "Z": z}))
    
    return commands


def spiral(polygon, tool_diameter, stepover_percent, milling_direction="climb", reverse=False, angle_degrees=None):
    """
    Generate a spiral clearing pattern for rectangular polygons.
    
    Args:
        polygon: The rectangular polygon boundary to clear
        tool_diameter: Diameter of the cutting tool
        stepover_percent: Stepover as percentage of tool diameter
        milling_direction: "climb" or "conventional" - spiral rotation direction
        reverse: Reverse the starting corner
        angle_degrees: Angle in degrees to rotate the cutting direction
        
    Returns:
        List of Path.Command objects representing the toolpath
    """
    import math
    
    # Get primary and step vectors from angle
    theta = float(angle_degrees) if angle_degrees is not None else 0.0
    primary_vec, step_vec = facing_common.unit_vectors_from_angle(theta)
    # Normalize defensively
    primary_vec = FreeCAD.Vector(primary_vec).multiply(1.0 / (primary_vec.Length or 1.0))
    step_vec = FreeCAD.Vector(step_vec).multiply(1.0 / (step_vec.Length or 1.0))
    
    Path.Log.debug(f"tool_diameter: {tool_diameter}, stepover_percent: {stepover_percent}, angle: {theta}, milling_direction: {milling_direction}")

    # Extract polygon corners
    polygon_info = facing_common.extract_polygon_geometry(polygon)
    corners = polygon_info['corners']
    
    # Project polygon onto primary/step axes to get bounds
    origin = facing_common.select_starting_corner(corners, primary_vec, step_vec, "climb")
    min_s, max_s = facing_common.project_bounds(polygon, primary_vec, origin)
    min_t, max_t = facing_common.project_bounds(polygon, step_vec, origin)
    
    primary_length = max_s - min_s
    step_length = max_t - min_t
    
    # Spiral generation always works from the minimum projection corner
    # to ensure consistent inward offset calculations
    start_corner = facing_common.select_starting_corner(corners, primary_vec, step_vec, "climb")

    # Calculate stepover distance and engagement offset
    stepover = tool_diameter * stepover_percent / 100.0
    engagement_offset = facing_common.calculate_engagement_offset(tool_diameter, stepover_percent)

    # Calculate the number of layers (concentric rectangles) we need
    # Account for starting outside the polygon
    max_layers_primary = int((primary_length / 2.0 + engagement_offset) / stepover)
    max_layers_step = int((step_length / 2.0 + engagement_offset) / stepover)
    num_layers = min(max_layers_primary, max_layers_step)
    
    # Generate toolpath commands
    commands = []
    z = polygon.BoundBox.ZMin
    
    # Determine spiral direction and starting corner based on milling direction and reverse
    # Climb = clockwise (inward spiral), Conventional = counter-clockwise
    clockwise = (milling_direction == "conventional")

    # Find origin corner (corner with minimum combined projection)
    origin = facing_common.select_starting_corner(corners, primary_vec, step_vec, "climb")
    Path.Log.debug(f"Origin corner: {origin}")
    Path.Log.debug(f"Primary vec: {primary_vec}, Step vec: {step_vec}")

    # Project polygon bounds in s/t space to build axis-aligned rectangles per layer
    min_s, max_s = facing_common.project_bounds(polygon, primary_vec, origin)
    min_t, max_t = facing_common.project_bounds(polygon, step_vec, origin)
    Path.Log.debug(f"S bounds: [{min_s:.2f}, {max_s:.2f}], T bounds: [{min_t:.2f}, {max_t:.2f}]")

    # Start outside polygon - tool center positioned so stepover% of tool engages polygon
    # Match generate_t_values logic: t = min_t - tool_radius + engagement_amount
    # For 50% stepover: offset = 0 (on boundary), for 20%: offset = -0.6*radius (outside)
    tool_radius = tool_diameter / 2.0
    engagement_amount = tool_diameter * (stepover_percent / 100.0)
    initial_offset = -tool_radius + engagement_amount
    k = 0

    def st_to_xy(s, t):
        # origin + primary_vec*s + step_vec*t using safe vector copies
        p = FreeCAD.Vector(origin)
        p = p.add(FreeCAD.Vector(primary_vec).multiply(s))
        p = p.add(FreeCAD.Vector(step_vec).multiply(t))
        p.z = z
        return p

    # Start corner index: 0=(s0,t0)=bottom-left, 1=(s1,t0)=bottom-right, 2=(s1,t1)=top-right, 3=(s0,t1)=top-left
    # Climb (clockwise=True): start at bottom-left (0), reverse -> top-right (2) - diagonal
    # Conventional (clockwise=False): start at top-right (2), reverse -> bottom-left (0) - diagonal
    # Reverse moves to diagonally opposite corner
    start_corner_index = 0 if clockwise else 2
    if reverse:
        # Move to diagonally opposite corner
        start_corner_index = (start_corner_index + 2) % 4

    first_move_done = False
    while True:
        # Calculate current layer offset (negative for first layer = outside polygon)
        current_offset = initial_offset + k * stepover
        
        # When offset is negative (outside), expand rectangle; when positive (inside), shrink it
        s0 = min_s + current_offset
        s1 = max_s - current_offset
        t0 = min_t + current_offset
        t1 = max_t - current_offset

        if not (s0 < s1 and t0 < t1):
            break

        # Build layer corners in fixed order
        corners_st = [(s0, t0), (s1, t0), (s1, t1), (s0, t1)]

        # Build traversal order starting at start_corner_index
        if clockwise:
            order = [(start_corner_index + i) % 4 for i in range(4)]
        else:
            order = [(start_corner_index - i) % 4 for i in range(4)]

        # Map first corner to XY and rapid there once
        start_idx = order[0]
        if k == 0:
            Path.Log.debug(f"Layer {k}: start_corner_index={start_corner_index}, order={order}, start_idx={start_idx}")
            Path.Log.debug(f"Layer {k}: corners_st={corners_st}")
            Path.Log.debug(f"Layer {k}: corners_st[start_idx]={corners_st[start_idx]}")
        start_xy = st_to_xy(*corners_st[start_idx])
        if k == 0:
            Path.Log.debug(f"Layer {k}: start_xy={start_xy}")
        if not first_move_done:
            commands.append(Path.Command("G0", {"X": start_xy.x, "Y": start_xy.y, "Z": z}))
            first_move_done = True

        # Move along sides 1..3 fully
        for i in range(1, 4):
            c_xy = st_to_xy(*corners_st[order[i]])
            commands.append(Path.Command("G1", {"X": c_xy.x, "Y": c_xy.y, "Z": z}))

        # Prepare next layer's transition point
        # The 4th side should move along the edge from corner 3 toward corner 0,
        # stopping where that edge meets the next layer boundary
        next_offset = current_offset + stepover
        s0n = min_s + next_offset
        s1n = max_s - next_offset
        t0n = min_t + next_offset
        t1n = max_t - next_offset
        valid_next = (s0n < s1n and t0n < t1n and (s1n - s0n) > tool_diameter and (t1n - t0n) > tool_diameter)

        if valid_next:
            # Calculate the point along the 4th edge where it meets the next layer
            # For clockwise from corner 0: edge goes from corner 3=(s0,t1) to corner 0=(s0,t0)
            # Stop at the next layer's t coordinate
            if clockwise:
                if start_corner_index == 0:  # Edge from (s0,t1) to (s0,t0), stop at t=t0n
                    transition_xy = st_to_xy(s0, t0n)
                elif start_corner_index == 1:  # Edge from (s0,t0) to (s1,t0), stop at s=s1n
                    transition_xy = st_to_xy(s1n, t0)
                elif start_corner_index == 2:  # Edge from (s1,t0) to (s1,t1), stop at t=t1n
                    transition_xy = st_to_xy(s1, t1n)
                else:  # Edge from (s1,t1) to (s0,t1), stop at s=s0n
                    transition_xy = st_to_xy(s0n, t1)
            else:
                # Counter-clockwise - opposite direction
                if start_corner_index == 0:  # Edge from (s1,t0) to (s0,t0), stop at s=s0n
                    transition_xy = st_to_xy(s0n, t0)
                elif start_corner_index == 1:  # Edge from (s1,t1) to (s1,t0), stop at t=t0n
                    transition_xy = st_to_xy(s1, t0n)
                elif start_corner_index == 2:  # Edge from (s0,t1) to (s1,t1), stop at s=s1n
                    transition_xy = st_to_xy(s1n, t1)
                else:  # Edge from (s0,t0) to (s0,t1), stop at t=t1n
                    transition_xy = st_to_xy(s0, t1n)
            
            # Side 4 as partial: move along edge to transition point
            commands.append(Path.Command("G1", {"X": transition_xy.x, "Y": transition_xy.y, "Z": z}))
            k += 1
        else:
            # Final: close back to start corner
            commands.append(Path.Command("G1", {"X": start_xy.x, "Y": start_xy.y, "Z": z}))
            break

    return commands
