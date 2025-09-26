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
Zigzag facing toolpath generator.

This module implements the zigzag clearing pattern that cuts back and forth
across the polygon in alternating directions, creating a continuous zigzag pattern.
"""

import FreeCAD
import Path
from . import facing_common


def zigzag(polygon, tool_diameter, stepover_percent, axis_preference="long", pass_extension=None, retract_height=None, milling_direction="climb"):
    """
    Generate a zigzag clearing pattern.
    
    This strategy cuts back and forth across the polygon in alternating directions.
    Each pass reverses direction from the previous pass, creating a zigzag pattern.
    The tool cuts continuously without lifting between passes, minimizing air time.
    Cutting direction alternates to maintain consistent chip load and surface finish.
    
    Args:
        polygon: The polygon boundary to clear
        tool_diameter: Diameter of the cutting tool
        stepover_percent: Stepover as percentage of tool diameter
        axis_preference: "long" or "short" - which axis to align primary cutting direction with
        pass_extension: Distance to extend cuts beyond polygon boundary for tool disengagement
        retract_height: Z height for rapid moves between passes (None = cutting height, no retracts)
        milling_direction: "climb" or "conventional" - affects direction of first pass
        
    Returns:
        List of Path.Command objects representing the toolpath
    """
    if pass_extension is None:
        pass_extension = tool_diameter * 0.5  # Default to half tool diameter
    
    # Analyze the rectangle to get orientation and dimensions
    rect_info = analyze_rectangle(polygon, axis_preference)
    primary_vec = rect_info['primary_vec']
    step_vec = rect_info['step_vec']
    primary_length = rect_info['primary_length']
    step_length = rect_info['step_length']
    reference_corner = rect_info['reference_corner']

    # Ensure vectors are properly normalized
    primary_vec = primary_vec.multiply(1.0 / primary_vec.Length)
    step_vec = step_vec.multiply(1.0 / step_vec.Length)

    # Calculate stepover distance and tool radius
    stepover = (stepover_percent / 100.0) * tool_diameter
    tool_radius = tool_diameter / 2.0
    
    Path.Log.debug("Zigzag: Tool diameter: {}, stepover_percent: {}, stepover: {}, tool_radius: {}".format(
        tool_diameter, stepover_percent, stepover, tool_radius))
    
    # Generate step positions along the stepping direction
    engagement_offset = tool_radius * (1.0 - stepover_percent / 100.0)
    step_positions = []
    current_step = -engagement_offset
    
    # Stop when tool edge reaches polygon edge (tool center at step_length - tool_radius)
    while current_step <= step_length - tool_radius:
        step_positions.append(current_step)
        current_step += stepover
    
    # Ensure we cover the end - tool edge should reach polygon edge
    if step_positions and step_positions[-1] < step_length - tool_radius - 0.001:
        step_positions.append(step_length - tool_radius)
    
    Path.Log.debug("Zigzag: Generated {} steps with stepover: {}, tool_radius: {}".format(len(step_positions), stepover, tool_radius))
    
    # Generate toolpath commands
    commands = []
    z = polygon.BoundBox.ZMin
    
    for i, step_distance in enumerate(step_positions):
        # Calculate the start and end points for this pass
        step_offset = FreeCAD.Vector(step_vec).multiply(step_distance)
        pass_start_base = FreeCAD.Vector(reference_corner).add(step_offset)
        
        # Extend the pass beyond the polygon boundaries
        primary_extension = FreeCAD.Vector(primary_vec).multiply(pass_extension)
        primary_full_length = FreeCAD.Vector(primary_vec).multiply(primary_length)
        
        # Alternate cutting direction for zigzag pattern based on milling direction preference
        # For climb milling, start with primary direction; for conventional, start opposite
        if milling_direction == "climb":
            if i % 2 == 0:
                # Even passes: cut in primary vector direction
                start_point = FreeCAD.Vector(pass_start_base).sub(primary_extension)
                end_point = FreeCAD.Vector(pass_start_base).add(primary_full_length).add(primary_extension)
            else:
                # Odd passes: cut opposite to primary vector direction
                start_point = FreeCAD.Vector(pass_start_base).add(primary_full_length).add(primary_extension)
                end_point = FreeCAD.Vector(pass_start_base).sub(primary_extension)
        else:  # conventional milling
            if i % 2 == 0:
                # Even passes: cut opposite to primary vector direction
                start_point = FreeCAD.Vector(pass_start_base).add(primary_full_length).add(primary_extension)
                end_point = FreeCAD.Vector(pass_start_base).sub(primary_extension)
            else:
                # Odd passes: cut in primary vector direction
                start_point = FreeCAD.Vector(pass_start_base).sub(primary_extension)
                end_point = FreeCAD.Vector(pass_start_base).add(primary_full_length).add(primary_extension)
        
        Path.Log.debug("Zigzag Step {}: pass={}, step_offset={}, start_point={}, end_point={}".format(
            step_distance, i, step_offset, start_point, end_point))
        
        # Set Z coordinate
        start_point.z = z
        end_point.z = z
        
        # Handle connection between passes based on retract_height setting
        if i > 0:
            if retract_height is not None:
                # Lift to retract height, rapid to start, then plunge
                commands.append(Path.Command("G0", {"Z": retract_height}))
                commands.append(Path.Command("G0", {
                    "X": start_point.x,
                    "Y": start_point.y
                }))
                commands.append(Path.Command("G0", {"Z": z}))
            else:
                # Traditional zigzag - cutting move to connect passes
                commands.append(Path.Command("G1", {
                    "X": start_point.x,
                    "Y": start_point.y,
                    "Z": z
                }))
        else:
            # First pass - position to start
            commands.append(Path.Command("G1", {
                "X": start_point.x,
                "Y": start_point.y,
                "Z": z
            }))
        
        # Add cutting move across the pass
        commands.append(Path.Command("G1", {
            "X": end_point.x,
            "Y": end_point.y,
            "Z": z
        }))
    
    return commands


def analyze_rectangle(polygon, axis_preference):
    """Analyze rectangle to determine orientation and dimensions."""
    # Extract polygon geometry
    polygon_info = facing_common.extract_polygon_geometry(polygon)
    edges = polygon_info['edges']
    corners = polygon_info['corners']
    
    # Get primary and step edges
    edge_info = facing_common.select_primary_step_edges(edges, axis_preference)
    
    # Find reference corner using dot product projections
    primary_vec = edge_info['primary_vec']
    step_vec = edge_info['step_vec']
    
    # Find the corner with minimum combined projection (the "origin" corner)
    min_projection = float('inf')
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
        'primary_vec': primary_vec,
        'step_vec': step_vec,
        'primary_length': edge_info['primary_length'],
        'step_length': edge_info['step_length'],
        'reference_corner': reference_corner
    }

