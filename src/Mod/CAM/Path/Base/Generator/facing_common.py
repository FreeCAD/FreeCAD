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
Common helper functions for facing toolpath generators.

This module contains shared geometric and utility functions used by
different facing strategies (spiral, zigzag, etc.).
"""

import FreeCAD
import Path
import Part


def extract_polygon_geometry(polygon):
    """Extract edges and corners from a rectangular polygon."""
    edges = []
    corners = []
    
    # Validate that polygon has exactly 4 edges (rectangle)
    if len(polygon.Edges) != 4:
        raise ValueError("Polygon must be rectangular (4 edges)")
    
    for edge in polygon.Edges:
        edge_vector = edge.Vertexes[1].Point.sub(edge.Vertexes[0].Point)
        edges.append({
            'start': edge.Vertexes[0].Point,
            'end': edge.Vertexes[1].Point,
            'vector': edge_vector,
            'length': edge.Length
        })
        corners.append(edge.Vertexes[0].Point)
    
    return {
        'edges': edges,
        'corners': corners
    }


def select_primary_step_edges(edges, axis_preference):
    """Select primary and step edges based on axis preference."""
    edge_lengths = [edge['length'] for edge in edges]
    unique_lengths = list(set(edge_lengths))
    
    if len(unique_lengths) == 1:
        # Square case - all edges are the same length
        # For squares, we need to pick two perpendicular edges
        primary_edge = edges[0]
        step_edge = None
        
        for edge in edges[1:]:
            # Check if this edge is perpendicular to the primary edge
            dot_product = abs(primary_edge['vector'].normalize().dot(edge['vector'].normalize()))
            if dot_product < 0.1:  # Nearly perpendicular
                step_edge = edge
                break
        
        if step_edge is None:
            # Fallback - just use adjacent edges (should be perpendicular for rectangles)
            step_edge = edges[1]
    elif len(unique_lengths) == 2:
        # Rectangle case - two different edge lengths
        long_length = max(unique_lengths)
        short_length = min(unique_lengths)
        
        # Find edges with long and short lengths
        long_edges = [edge for edge in edges if abs(edge['length'] - long_length) < 1e-6]
        short_edges = [edge for edge in edges if abs(edge['length'] - short_length) < 1e-6]
        
        # Select primary edge based on preference
        if axis_preference == "long":
            primary_edge = long_edges[0]
            step_edge = short_edges[0]
        else:  # "short"
            primary_edge = short_edges[0]
            step_edge = long_edges[0]
    else:
        raise ValueError("Polygon must be rectangular with 1 or 2 unique edge lengths")
    
    # Normalize vectors properly
    primary_vec = primary_edge['vector']
    step_vec = step_edge['vector']
    
    # Manual normalization to ensure it works correctly
    primary_length_calc = primary_vec.Length
    step_length_calc = step_vec.Length
    
    if primary_length_calc > 0:
        primary_vec = primary_vec.multiply(1.0 / primary_length_calc)
    if step_length_calc > 0:
        step_vec = step_vec.multiply(1.0 / step_length_calc)
    
    return {
        'primary_edge': primary_edge,
        'step_edge': step_edge,
        'primary_vec': primary_vec,
        'step_vec': step_vec,
        'primary_length': primary_edge['length'],
        'step_length': step_edge['length']
    }


def select_starting_corner(corners, primary_vec, step_vec, milling_direction):
    """
    Select starting corner based on milling direction and edge orientation.
    
    For climb milling, we want to start from the corner that allows the tool to
    move in the direction that creates the optimal cutting conditions.
    For conventional milling, we start from the opposite corner.
    
    Args:
        corners (list): List of corner points from the polygon
        primary_vec (FreeCAD.Vector): Primary direction vector (normalized)
        step_vec (FreeCAD.Vector): Step direction vector (normalized)
        milling_direction (str): "climb" or "conventional"
    
    Returns:
        FreeCAD.Vector: The selected starting corner point
    """
    if len(corners) < 4:
        return corners[0]
    
    # Find the corner with minimum combined projection onto both direction vectors
    # This gives us the "origin" corner regardless of polygon rotation
    min_projection = float('inf')
    selected_corner = corners[0]
    
    for corner in corners:
        # Calculate projections onto both direction vectors
        primary_proj = corner.dot(primary_vec)
        step_proj = corner.dot(step_vec)
        
        # Combined projection (distance from origin in direction space)
        combined_proj = primary_proj + step_proj
        
        if combined_proj < min_projection:
            min_projection = combined_proj
            selected_corner = corner
    
    # For conventional milling, we want to start from the opposite corner
    # to ensure proper cutting direction
    if milling_direction == "conventional":
        # Find the corner that's furthest from the selected corner
        max_distance = 0
        opposite_corner = selected_corner
        
        for corner in corners:
            distance = selected_corner.distanceToPoint(corner)
            if distance > max_distance:
                max_distance = distance
                opposite_corner = corner
        
        selected_corner = opposite_corner
    
    return selected_corner


def get_angled_polygon(wire, angle):
    """
    Create a rotated bounding box that fully contains the input wire.
    
    This function generates a rectangular wire representing a bounding box rotated by the 
    specified angle that completely encompasses the original wire. The algorithm works by:
    1. Rotating the original wire in the opposite direction to align it optimally
    2. Computing the axis-aligned bounding box of the rotated wire
    3. Rotating the bounding box back to the desired angle
    
    Args:
        wire (Part.Wire): A closed wire to create the rotated bounding box for
        angle (float): Rotation angle in degrees (positive = counterclockwise)
    
    Returns:
        Part.Wire: A closed rectangular wire representing the rotated bounding box
        
    Raises:
        ValueError: If the input wire is not closed
    """
    if not wire.isClosed():
        raise ValueError("Wire must be closed")

    # Get the center point of the original wire for all rotations
    center = wire.BoundBox.Center
    rotation_axis = FreeCAD.Vector(0, 0, 1)  # Z-axis
    
    Path.Log.debug(f"Original wire center: {center}")

    # Step 1: Rotate the wire in the opposite direction to align optimally with axes
    temp_wire = wire.copy()
    temp_wire.rotate(center, rotation_axis, -angle)
    
    # Step 2: Get the axis-aligned bounding box of the rotated wire
    bounding_box = temp_wire.BoundBox
    Path.Log.debug(f"Rotated bounding box center: {bounding_box.Center}")

    # Create the four corners of the bounding box rectangle
    corners = [
        FreeCAD.Vector(bounding_box.XMin, bounding_box.YMin, bounding_box.ZMin),
        FreeCAD.Vector(bounding_box.XMax, bounding_box.YMin, bounding_box.ZMin),
        FreeCAD.Vector(bounding_box.XMax, bounding_box.YMax, bounding_box.ZMin),
        FreeCAD.Vector(bounding_box.XMin, bounding_box.YMax, bounding_box.ZMin)
    ]
    
    # Close the polygon by adding the first corner again
    corners.append(corners[0])
    bounding_wire = Part.makePolygon(corners)

    # Step 3: Rotate the bounding box to the desired angle
    bounding_wire.rotate(center, rotation_axis, angle)
    
    return bounding_wire


def calculate_engagement_offset(tool_diameter, stepover_percent):
    """Calculate the engagement offset for proper tool engagement."""
    tool_radius = tool_diameter / 2.0
    return tool_radius * (1.0 - stepover_percent / 100.0)


def validate_inputs(wire, tool_diameter, stepover_percent, start_depth, final_depth, 
                   start_point, pattern, milling_direction):
    """Validate all input parameters for facing operations."""
    
    # Validate wire is closed
    if not wire.isClosed():
        raise ValueError("Wire must be a closed polygon for facing operation")
    
    # Validate wire is co-planar with XY plane
    bb = wire.BoundBox
    z_tolerance = 0.001  # 1 micron tolerance
    if abs(bb.ZMax - bb.ZMin) > z_tolerance:
        raise ValueError("Wire must be co-planar with XY plane for facing operation")
    
    # Validate tool diameter
    if tool_diameter <= 0:
        raise ValueError("Tool diameter must be positive")
    if tool_diameter > 100:  # Reasonable upper limit
        raise ValueError("Tool diameter too large (>100mm)")
    
    # Validate stepover percentage
    if stepover_percent <= 0:
        raise ValueError("Stepover percentage must be positive")
    if stepover_percent > 100:
        Path.Log.warning(f"Stepover percentage ({stepover_percent}%) is greater than 100%")
    if stepover_percent > 200:
        raise ValueError("Stepover percentage too large (>200%)")
    if stepover_percent < 1:
        Path.Log.warning(f"Very small stepover percentage ({stepover_percent}%) may result in excessive cutting time")
    
    # Validate depths
    if start_depth == final_depth:
        raise ValueError("Start depth must be different from final depth")
    
    # Validate start point if provided
    if start_point is not None:
        if not hasattr(start_point, 'x') or not hasattr(start_point, 'y'):
            raise ValueError("Start point must be a FreeCAD.Vector with x and y coordinates")
        
        # Check if start point is too close to polygon (within bounding box + tool radius)
        tool_radius = tool_diameter / 2.0
        expanded_bb_xmin = bb.XMin - tool_radius
        expanded_bb_xmax = bb.XMax + tool_radius
        expanded_bb_ymin = bb.YMin - tool_radius
        expanded_bb_ymax = bb.YMax + tool_radius
        
        if (expanded_bb_xmin <= start_point.x <= expanded_bb_xmax and 
            expanded_bb_ymin <= start_point.y <= expanded_bb_ymax):
            raise ValueError("Start point is too close to the polygon to be cleared")
    
    # Validate pattern
    valid_patterns = ["zigzag", "unidirectional", "spiral"]
    if pattern not in valid_patterns:
        raise ValueError(f"Invalid pattern: {pattern}. Must be one of {valid_patterns}")
    
    # Validate milling direction
    valid_directions = ["climb", "conventional"]
    if milling_direction not in valid_directions:
        raise ValueError(f"Invalid milling direction: {milling_direction}. Must be one of {valid_directions}")
