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
    """Generate the four corners of a spiral layer offset inward from the original polygon."""
    # Calculate the four corners of this layer (reduced by inward offset)
    adjusted_primary_length = max(0, primary_length - 2 * inward_offset)
    adjusted_step_length = max(0, step_length - 2 * inward_offset)
    
    # Move the starting corner inward by the offset amount
    # The inward direction is the sum of the normalized inward normals of both edges
    inward_primary = primary_vec * inward_offset  # Move inward along primary direction
    inward_step = step_vec * inward_offset        # Move inward along step direction
    
    # The actual starting corner for this layer is offset inward
    layer_start_corner = start_corner + inward_primary + inward_step
    
    # Build rectangle from the offset starting corner with reduced dimensions
    corner1 = FreeCAD.Vector(layer_start_corner)
    corner2 = corner1 + primary_vec * adjusted_primary_length
    corner3 = corner2 + step_vec * adjusted_step_length
    corner4 = corner3 - primary_vec * adjusted_primary_length
    
    return [corner1, corner2, corner3, corner4]


def generate_layer_path(layer_corners, layer_num, z, clockwise):
    """Generate the toolpath commands for a single spiral layer."""
    commands = []
    
    # Set Z coordinate for all corners
    for corner in layer_corners:
        corner.z = z
    
    # For the first layer, start with a rapid move to the first corner
    if layer_num == 0:
        commands.append(Path.Command("G0", {"X": layer_corners[0].x, "Y": layer_corners[0].y, "Z": z}))
    
    # Generate the rectangular path - always go around the full rectangle
    if clockwise:
        # Clockwise: 0 -> 1 -> 2 -> 3 -> back to 0
        for i in range(1, 4):
            commands.append(Path.Command("G1", {"X": layer_corners[i].x, "Y": layer_corners[i].y, "Z": z}))
        # Complete the rectangle
        commands.append(Path.Command("G1", {"X": layer_corners[0].x, "Y": layer_corners[0].y, "Z": z}))
    else:
        # Counter-clockwise: 0 -> 3 -> 2 -> 1 -> back to 0
        for i in [3, 2, 1]:
            commands.append(Path.Command("G1", {"X": layer_corners[i].x, "Y": layer_corners[i].y, "Z": z}))
        # Complete the rectangle
        commands.append(Path.Command("G1", {"X": layer_corners[0].x, "Y": layer_corners[0].y, "Z": z}))
    
    return commands


def spiral(polygon, tool_diameter, stepover_percent, axis_preference="long", milling_direction="climb"):
    """
    Generate a spiral clearing pattern for rectangular polygons.
    
    This algorithm works directly with the polygon's actual geometry, including
    angled rectangles, rather than using axis-aligned bounding boxes.
    
    Args:
        polygon: The rectangular polygon boundary to clear
        tool_diameter: Diameter of the cutting tool
        stepover_percent: Stepover as percentage of tool diameter
        axis_preference: "long" or "short" - which polygon edge to use as primary direction
        milling_direction: "climb" or "conventional" - spiral rotation direction
        
    Returns:
        List of Path.Command objects representing the toolpath
    """
    Path.Log.debug(f"tool_diameter: {tool_diameter}, stepover_percent: {stepover_percent}, axis_preference: {axis_preference}, milling_direction: {milling_direction}")

    # Extract polygon edges and corners directly
    polygon_info = facing_common.extract_polygon_geometry(polygon)
    edges = polygon_info['edges']
    corners = polygon_info['corners']
    
    # Determine primary and step edges based on axis preference
    edge_info = facing_common.select_primary_step_edges(edges, axis_preference)
    primary_edge = edge_info['primary_edge']
    step_edge = edge_info['step_edge']
    primary_vec = edge_info['primary_vec']
    step_vec = edge_info['step_vec']
    primary_length = edge_info['primary_length']
    step_length = edge_info['step_length']
    
    # Determine starting corner based on milling direction
    start_corner = facing_common.select_starting_corner(corners, primary_vec, step_vec, milling_direction)

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
    
    # Determine spiral direction based on milling direction
    clockwise = (milling_direction == "climb")
    
    for layer in range(num_layers + 1):
        # Calculate inward offset for this layer
        # Start outside the polygon by engagement_offset, then move inward by stepover each layer
        inward_offset = -engagement_offset + layer * stepover
        
        # Generate the corners for this layer by offsetting inward
        layer_corners = generate_spiral_corners(start_corner, primary_vec, step_vec, 
                                              primary_length, step_length, inward_offset)
        
        # Skip if layer becomes too small
        layer_primary_length = primary_length - 2 * inward_offset
        layer_step_length = step_length - 2 * inward_offset
        
        if (layer_primary_length <= tool_diameter or layer_step_length <= tool_diameter) and layer >= num_layers:
            break
        
        # Generate the spiral path for this layer
        layer_commands = generate_layer_path(layer_corners, layer, z, clockwise)
        commands.extend(layer_commands)
        
        # Add transition to next layer if not the last layer
        if layer < num_layers and layer_primary_length > tool_diameter and layer_step_length > tool_diameter:
            # Calculate the starting corner of the next layer
            next_inward_offset = -engagement_offset + (layer + 1) * stepover
            next_layer_corners = generate_spiral_corners(start_corner, primary_vec, step_vec, 
                                                       primary_length, step_length, next_inward_offset)
            # Move to the starting corner of the next layer
            commands.append(Path.Command("G1", {"X": next_layer_corners[0].x, "Y": next_layer_corners[0].y, "Z": z}))
    
    return commands
