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

This module implements the bidirectional clearing pattern that cuts back and forth
across the polygon but alternates which side of the polygon is cut, maintaining
consistent milling direction throughout.
"""

import FreeCAD
import Path
from . import facing_common


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


def bidirectional(polygon, tool_diameter, stepover_percent, axis_preference="long", pass_extension=None, retract_height=None, milling_direction="climb"):
    """
    Generate a bidirectional clearing pattern.
    
    This strategy cuts back and forth across the polygon but alternates which side of the polygon is cut. 
    Like spiral, this maintains consistency of milling type.  Unlike the sprial strategy,
    -bidirectional only cuts on the preferred axis and does rapid moves between them.
    The end moves on the non-preferred axis are rapid moves that stay outside the stock area,
    so no tool lifting is required. This provides efficient material removal
    while avoiding cutting on the short ends of the rectangle, which can be
    beneficial for surface finish and tool life.
    
    Args:
        polygon: The polygon boundary to clear
        tool_diameter: Diameter of the cutting tool
        stepover_percent: Stepover as percentage of tool diameter
        axis_preference: "long" or "short" - which axis to align primary cutting direction with
        pass_extension: Distance to extend cuts beyond polygon boundary for tool disengagement
        retract_height: Z height for rapid moves (None = cutting height, no retracts)
        milling_direction: "climb" or "conventional" - affects direction of first pass
        
    Returns:
        List of Path.Command objects representing the toolpath
    """
    Path.Log.debug(f"Bidirectional: Tool diameter: {tool_diameter}, stepover_percent: {stepover_percent}, axis_preference: {axis_preference}, pass_extension: {pass_extension}, retract_height: {retract_height}, milling_direction: {milling_direction}")

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
    
    Path.Log.debug("Bidirectional: Tool diameter: {}, stepover_percent: {}, stepover: {}, tool_radius: {}".format(
        tool_diameter, stepover_percent, stepover, tool_radius))
    
    # For bidirectional, we start outside the polygon and step inward
    # Calculate how many passes we need based on stepover
    max_passes = int((step_length + 2 * tool_radius) / stepover) + 1
    
    Path.Log.debug("Bidirectional: Calculated {} passes with stepover {}".format(max_passes, stepover))
    
    # Generate toolpath commands
    commands = []
    z = polygon.BoundBox.ZMin
    
    # Bidirectional alternates between bottom and top while stepping inward
    pass_count = 0
    while pass_count < max_passes:
        # Calculate step distance - start outside and step inward
        if pass_count % 2 == 0:
            # Bottom passes: start outside bottom edge, step inward (increasing)
            step_distance = -tool_radius + (pass_count // 2) * stepover
        else:
            # Top passes: start outside top edge, step inward (decreasing)
            step_distance = step_length + tool_radius - (pass_count // 2) * stepover
        
        # Stop if we've stepped too far inward
        if pass_count % 2 == 0:  # Bottom pass
            if step_distance > step_length - tool_radius:
                break
        else:  # Top pass
            if step_distance < tool_radius:
                break
        
        # Calculate the start and end points for this pass
        step_offset = FreeCAD.Vector(step_vec).multiply(step_distance)
        pass_base = FreeCAD.Vector(reference_corner).add(step_offset)
        
        # Extend the pass beyond the polygon boundaries
        primary_extension = FreeCAD.Vector(primary_vec).multiply(pass_extension)
        primary_full_length = FreeCAD.Vector(primary_vec).multiply(primary_length)
        
        # Maintain consistent milling direction but alternate cutting direction to stay climb/conventional
        if milling_direction == "climb":
            if pass_count % 2 == 0:
                # Bottom pass: cut left to right (climb)
                start_point = FreeCAD.Vector(pass_base).sub(primary_extension)
                end_point = FreeCAD.Vector(pass_base).add(primary_full_length).add(primary_extension)
            else:
                # Top pass: cut right to left (still climb)
                start_point = FreeCAD.Vector(pass_base).add(primary_full_length).add(primary_extension)
                end_point = FreeCAD.Vector(pass_base).sub(primary_extension)
        else:  # conventional milling
            if pass_count % 2 == 0:
                # Bottom pass: cut right to left (conventional)
                start_point = FreeCAD.Vector(pass_base).add(primary_full_length).add(primary_extension)
                end_point = FreeCAD.Vector(pass_base).sub(primary_extension)
            else:
                # Top pass: cut left to right (still conventional)
                start_point = FreeCAD.Vector(pass_base).sub(primary_extension)
                end_point = FreeCAD.Vector(pass_base).add(primary_full_length).add(primary_extension)
        
        Path.Log.debug("Bidirectional Pass {}: step_distance={}, start_point={}, end_point={}".format(
            pass_count, step_distance, start_point, end_point))
        
        # Set Z coordinate
        start_point.z = z
        end_point.z = z
        
        # Bidirectional uses rapid moves between passes (like directional)
        if pass_count > 0:
            if retract_height is not None:
                # Retract to safe height
                commands.append(Path.Command("G0", {"Z": retract_height}))
            
            # Rapid to XY position
            commands.append(Path.Command("G0", {
                "X": start_point.x,
                "Y": start_point.y
            }))
            
            # Rapid down to cutting height if we retracted
            if retract_height is not None:
                commands.append(Path.Command("G0", {"Z": z}))
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
        
        pass_count += 1
    
    return commands


def analyze_rectangle(polygon, axis_preference):
    """Analyze rectangle to determine orientation and dimensions."""
    # Extract polygon geometry
    polygon_info = facing_common.extract_polygon_geometry(polygon)
    edges = polygon_info['edges']
    corners = polygon_info['corners']
    
    # Get primary and step edges
    edge_info = facing_common.select_primary_step_edges(edges, axis_preference)
    
    return {
        'primary_vec': edge_info['primary_vec'],
        'step_vec': edge_info['step_vec'],
        'primary_length': edge_info['primary_length'],
        'step_length': edge_info['step_length'],
        'reference_corner': facing_common.select_starting_corner(corners, edge_info['primary_vec'], edge_info['step_vec'], "climb")
    }
