# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2024 sliptonic <shopinthewoods@gmail.com>               *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import FreeCAD
import Path
import Part
import math
from Path.Op import Base as PathOpBase

__title__ = "Facing toolpath Generator"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Generates facing toolpaths for large cutters with side entry"

if True:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


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


def _analyze_rectangle(polygon, axis_preference="long"):
    """
    Analyze a rectangular polygon to determine its orientation and dimensions.
    
    Args:
        polygon: The rectangular polygon to analyze
        axis_preference: "long" or "short" - which axis to use as primary cutting direction
    
    Returns:
        dict: Contains primary_vec, step_vec, primary_length, step_length, reference_corner
    """
    # Get the four corners of the rectangular polygon
    edges = polygon.Edges
    if len(edges) != 4:
        raise ValueError("Polygon must be rectangular (4 edges)")
    
    # Get vertices from edges - use bounding box to ensure consistent ordering
    bbox = polygon.BoundBox
    
    # Define the four corners based on bounding box (consistent regardless of polygon vertex order)
    vertices = [
        FreeCAD.Vector(bbox.XMin, bbox.YMin, bbox.ZMin),  # bottom-left
        FreeCAD.Vector(bbox.XMax, bbox.YMin, bbox.ZMin),  # bottom-right  
        FreeCAD.Vector(bbox.XMax, bbox.YMax, bbox.ZMin),  # top-right
        FreeCAD.Vector(bbox.XMin, bbox.YMax, bbox.ZMin)   # top-left
    ]
    
    # Find the two edge vectors to determine rectangle orientation
    edge1_vec = vertices[1].sub(vertices[0])  # horizontal edge (X direction)
    edge2_vec = vertices[3].sub(vertices[0])  # vertical edge (Y direction)
    
    # Calculate edge lengths
    edge1_length = edge1_vec.Length
    edge2_length = edge2_vec.Length
    
    # Determine which edge represents the long/short axis based on preference
    if axis_preference == "long":
        if edge1_length >= edge2_length:
            primary_vec = edge1_vec.multiply(1.0 / edge1_length)
            step_vec = edge2_vec.multiply(1.0 / edge2_length)
            primary_length = edge1_length
            step_length = edge2_length
        else:
            primary_vec = edge2_vec.multiply(1.0 / edge2_length)
            step_vec = edge1_vec.multiply(1.0 / edge1_length)
            primary_length = edge2_length
            step_length = edge1_length
    else:  # axis_preference == "short"
        if edge1_length <= edge2_length:
            primary_vec = edge1_vec.multiply(1.0 / edge1_length)
            step_vec = edge2_vec.multiply(1.0 / edge2_length)
            primary_length = edge1_length
            step_length = edge2_length
        else:
            primary_vec = edge2_vec.multiply(1.0 / edge2_length)
            step_vec = edge1_vec.multiply(1.0 / edge1_length)
            primary_length = edge2_length
            step_length = edge1_length
    
    # Find the corner that will serve as our reference point
    # Use the polygon's bounding box minimum corner as the reference
    # This ensures consistent behavior regardless of world coordinate origin
    reference_corner = FreeCAD.Vector(polygon.BoundBox.XMin, polygon.BoundBox.YMin, polygon.BoundBox.ZMin)

    Path.Log.debug("Primary vector: {} (length: {})".format(primary_vec, primary_vec.Length))
    Path.Log.debug("Step vector: {} (length: {})".format(step_vec, step_vec.Length))
    Path.Log.debug("Primary length: {}".format(primary_length))
    Path.Log.debug("Step length: {}".format(step_length))
    Path.Log.debug("Reference corner: {}".format(reference_corner)) 

    return {
        'primary_vec': primary_vec,
        'step_vec': step_vec,
        'primary_length': primary_length,
        'step_length': step_length,
        'reference_corner': reference_corner
    }


def _zigzag(polygon, tool_diameter, stepover_percent, axis_preference="long", pass_extension=None, retract_height=None, milling_direction="climb"):
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
    rect_info = _analyze_rectangle(polygon, axis_preference)
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

def _directional(polygon, tool_diameter, stepover_percent, axis_preference="long", pass_extension=None, retract_height=None, milling_direction="climb"):
    """
    Generate a unidirectional clearing pattern.
    
    This strategy cuts in the same direction for every pass across the polygon.
    After each cutting pass, the tool lifts up (rapid move) and repositions to 
    the start of the next pass. All cutting moves go in the same direction,
    which can provide more consistent surface finish but requires more air time
    due to the rapid repositioning moves between passes.
    
    Args:
        polygon: The polygon boundary to clear (rectangular, possibly rotated)
        tool_diameter: Diameter of the cutting tool
        stepover_percent: Stepover as percentage of tool diameter
        axis_preference: "long" or "short" - which axis to align primary cutting direction with
        pass_extension: Distance to extend cuts beyond polygon boundary for tool disengagement
        retract_height: Z height for rapid moves (None = cutting height)
        milling_direction: "climb" or "conventional" - milling direction preference
    
    Returns:
        List of Path.Command objects representing the toolpath
    """
    if pass_extension is None:
        pass_extension = tool_diameter * 0.5  # Default to half tool diameter
    
    # Analyze the rectangle to get orientation and dimensions
    rect_info = _analyze_rectangle(polygon, axis_preference)
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
    
    Path.Log.debug("Tool diameter: {}, stepover_percent: {}, stepover: {}, tool_radius: {}".format(
        tool_diameter, stepover_percent, stepover, tool_radius))
    
    # Generate step positions along the stepping direction
    # For 30% engagement: tool center should be at (tool_radius - 30% of tool_radius) outside polygon
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
    
    Path.Log.debug("Engagement offset: {}, step positions: {}".format(engagement_offset, step_positions))
    Path.Log.debug("Generated {} steps with stepover: {}, tool_radius: {}".format(len(step_positions), stepover, tool_radius))
    
    # Generate toolpath commands
    commands = []
    z = polygon.BoundBox.ZMin
    
    for step_distance in step_positions:
        # Calculate the start and end points for this pass
        step_offset = FreeCAD.Vector(step_vec).multiply(step_distance)
        pass_start_base = FreeCAD.Vector(reference_corner).add(step_offset)
        
        # Extend the pass beyond the polygon boundaries
        primary_extension = FreeCAD.Vector(primary_vec).multiply(pass_extension)
        primary_full_length = FreeCAD.Vector(primary_vec).multiply(primary_length)
        
        # Determine cutting direction based on milling preference
        if milling_direction == "climb":
            # Climb milling: cut in primary vector direction
            start_point = FreeCAD.Vector(pass_start_base).sub(primary_extension)
            end_point = FreeCAD.Vector(pass_start_base).add(primary_full_length).add(primary_extension)
        else:
            # Conventional milling: cut opposite to primary vector direction
            start_point = FreeCAD.Vector(pass_start_base).add(primary_full_length).add(primary_extension)
            end_point = FreeCAD.Vector(pass_start_base).sub(primary_extension)
        
        Path.Log.debug("Step {}: step_offset={}, pass_start_base={}, start_point={}, end_point={}".format(
            step_distance, step_offset, pass_start_base, start_point, end_point))
        
        # Set Z coordinate
        start_point.z = z
        end_point.z = z
        
        # Add rapid move to start of pass (except for first pass)
        if commands:
            # Retract to safe height if specified
            if retract_height is not None:
                commands.append(Path.Command("G0", {"Z": retract_height}))
            
            # Rapid to XY position
            commands.append(Path.Command("G0", {
                "X": start_point.x,
                "Y": start_point.y
            }))
            
            # Rapid down to cutting height if we retracted
            if retract_height is not None:
                commands.append(Path.Command("G0", {"Z": z}))
        
        # Add cutting moves
        commands.append(Path.Command("G1", {
            "X": start_point.x,
            "Y": start_point.y,
            "Z": z
        }))
        commands.append(Path.Command("G1", {
            "X": end_point.x,
            "Y": end_point.y,
            "Z": z
        }))
    
    return commands

def _spiral(polygon, tool_diameter, stepover_percent, axis_preference="long", milling_direction="climb"):
    """
    Generate a spiral clearing pattern for rectangular polygons.
    
    NEW ALGORITHM DESIGN:
    This algorithm works directly with the polygon's actual geometry, including
    angled rectangles, rather than using axis-aligned bounding boxes.
    
    GEOMETRIC APPROACH:
    1. Extract the 4 edges of the rectangular polygon directly
    2. Determine primary/step edges based on axis_preference and edge lengths
    3. Find the starting corner based on milling direction and edge orientation
    4. Generate concentric rectangular layers that follow the polygon's orientation
    5. Each layer is offset inward by stepover distance along polygon normals
    
    MILLING DIRECTION & STARTING CORNER:
    - Climb milling: Start from corner that allows clockwise traversal
    - Conventional milling: Start from corner that allows counter-clockwise traversal
    - Starting corner selection is independent of coordinate system origin
    
    POLYGON ORIENTATION HANDLING:
    - Works with rectangles at any angle, not just axis-aligned
    - Spiral follows the actual polygon shape, maintaining its orientation
    - Edge vectors and normals are derived from actual polygon geometry
    
    Args:
        polygon: The rectangular polygon boundary to clear
        tool_diameter: Diameter of the cutting tool
        stepover_percent: Stepover as percentage of tool diameter
        axis_preference: "long" or "short" - which polygon edge to use as primary direction
        milling_direction: "climb" or "conventional" - spiral rotation direction
        
    Returns:
        List of Path.Command objects representing the toolpath
    """
    # Extract polygon edges and corners directly
    polygon_info = _extract_polygon_geometry(polygon)
    edges = polygon_info['edges']
    corners = polygon_info['corners']
    
    # Determine primary and step edges based on axis preference
    edge_info = _select_primary_step_edges(edges, axis_preference)
    primary_edge = edge_info['primary_edge']
    step_edge = edge_info['step_edge']
    primary_vec = edge_info['primary_vec']
    step_vec = edge_info['step_vec']
    primary_length = edge_info['primary_length']
    step_length = edge_info['step_length']
    
    # Determine starting corner based on milling direction
    start_corner = _select_starting_corner(corners, primary_edge, step_edge, milling_direction)
    
    Path.Log.debug("Spiral: primary_vec={}, step_vec={}, primary_length={}, step_length={}".format(
        primary_vec, step_vec, primary_length, step_length))
    Path.Log.debug("Spiral: start_corner={}".format(start_corner))

    # Calculate stepover distance
    stepover = tool_diameter * stepover_percent / 100.0

    # Calculate the number of layers (concentric rectangles) we need
    max_layers_primary = int((primary_length / 2.0) / stepover)
    max_layers_step = int((step_length / 2.0) / stepover)
    num_layers = min(max_layers_primary, max_layers_step)
    
    Path.Log.debug("Spiral: max_layers_primary={}, max_layers_step={}, num_layers={}".format(
        max_layers_primary, max_layers_step, num_layers))
    
    # Generate toolpath commands
    commands = []
    z = polygon.BoundBox.ZMin
    
    # Determine spiral direction based on milling direction
    clockwise = (milling_direction == "climb")
    
    for layer in range(num_layers + 1):
        # Calculate inward offset for this layer
        inward_offset = layer * stepover
        
        # Generate the corners for this layer by offsetting inward
        layer_corners = _generate_layer_corners(start_corner, primary_vec, step_vec, 
                                               primary_length, step_length, inward_offset)
        
        # Skip if layer becomes too small
        layer_primary_length = primary_length - 2 * inward_offset
        layer_step_length = step_length - 2 * inward_offset
        
        if (layer_primary_length <= tool_diameter or layer_step_length <= tool_diameter) and layer >= num_layers:
            Path.Log.debug("Spiral layer {}: Rectangle too small, breaking.".format(layer))
            break
        
        # Generate the spiral path for this layer
        layer_commands = _generate_layer_path(layer_corners, layer, z, clockwise, stepover, 
                                            layer < num_layers)
        commands.extend(layer_commands)
        
        # Add transition to next layer if not the last layer
        if layer < num_layers and layer_primary_length > tool_diameter and layer_step_length > tool_diameter:
            # Calculate the starting corner of the next layer
            next_inward_offset = (layer + 1) * stepover
            next_layer_corners = _generate_layer_corners(start_corner, primary_vec, step_vec, 
                                                       primary_length, step_length, next_inward_offset)
            # Move to the starting corner of the next layer
            commands.append(Path.Command("G1", {"X": next_layer_corners[0].x, "Y": next_layer_corners[0].y, "Z": z}))
    
    return commands

def _extract_polygon_geometry(polygon):
    """Extract edges and corners from a rectangular polygon."""
    # Get the polygon edges
    edges = []
    corners = []
    
    # Assuming polygon is a closed wire with 4 edges (rectangle)
    for edge in polygon.Edges:
        edge_vector = edge.Vertexes[1].Point.sub(edge.Vertexes[0].Point)
        edges.append({
            'start': edge.Vertexes[0].Point,
            'end': edge.Vertexes[1].Point,
            'vector': edge_vector,
            'length': edge.Length
        })
        corners.append(edge.Vertexes[0].Point)
        Path.Log.debug("Edge: start={}, end={}, vector={}, length={}".format(
            edge.Vertexes[0].Point, edge.Vertexes[1].Point, edge_vector, edge.Length))
    
    return {
        'edges': edges,
        'corners': corners
    }

def _select_primary_step_edges(edges, axis_preference):
    """Select primary and step edges based on axis preference."""
    # Find the unique edge lengths (assuming rectangle)
    edge_lengths = [edge['length'] for edge in edges]
    unique_lengths = list(set(edge_lengths))
    
    if len(unique_lengths) == 1:
        # Square case - all edges are the same length
        # For squares, we need to pick two perpendicular edges
        # Find two edges that are perpendicular (dot product near 0)
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
    
    Path.Log.debug("Edge selection: primary_length={}, step_length={}, primary_vec={}, step_vec={}".format(
        primary_edge['length'], step_edge['length'], primary_vec, step_vec))
    
    return {
        'primary_edge': primary_edge,
        'step_edge': step_edge,
        'primary_vec': primary_vec,
        'step_vec': step_vec,
        'primary_length': primary_edge['length'],
        'step_length': step_edge['length']
    }

def _select_starting_corner(corners, primary_edge, step_edge, milling_direction):
    """Select starting corner based on milling direction and edge orientation."""
    # For now, use the first corner as starting point
    # TODO: Implement proper corner selection logic based on milling direction
    return corners[0]

def _generate_layer_corners(start_corner, primary_vec, step_vec, primary_length, step_length, inward_offset):
    """Generate the four corners of a spiral layer offset inward from the original polygon."""
    # Calculate the four corners of this layer (reduced by inward offset)
    adjusted_primary_length = max(0, primary_length - 2 * inward_offset)
    adjusted_step_length = max(0, step_length - 2 * inward_offset)
    
    Path.Log.debug("Layer corners: inward_offset={}, primary_length={}, step_length={}, adjusted_primary={}, adjusted_step={}".format(
        inward_offset, primary_length, step_length, adjusted_primary_length, adjusted_step_length))
    
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
    
    Path.Log.debug("Generated corners: c1={}, c2={}, c3={}, c4={}".format(corner1, corner2, corner3, corner4))
    
    return [corner1, corner2, corner3, corner4]

def _generate_layer_path(layer_corners, layer_num, z, clockwise, stepover, has_next_layer):
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

def _bidirectional(polygon, tool_diameter, stepover_percent, axis_preference="long", pass_extension=None, retract_height=None, milling_direction="climb"):
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
    if pass_extension is None:
        pass_extension = tool_diameter * 0.5  # Default to half tool diameter
    
    # Analyze the rectangle to get orientation and dimensions
    rect_info = _analyze_rectangle(polygon, axis_preference)
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

def generate(
    wire,
    tool_diameter,
    stepover_percent,
    start_depth,
    final_depth,
    spindle_direction,
    max_depth_per_pass=None,
    pattern="zigzag",
    milling_direction="climb",
    cutting_feedrate=None,
    approach_distance=None,
    retract_height=None
):
    """
    Generate a facing toolpath for a closed polygon with proper side entry for large cutters.
    
    Args:
        wire: Part.Wire representing the closed polygon boundary
        tool_diameter: Diameter of the facing cutter
        stepover_percent: Stepover as percentage of tool diameter (e.g., 75 for 75%)
        start_depth: Z height to start cutting (typically stock top)
        final_depth: Z height to finish cutting (typically part surface)
        spindle_direction: Spindle direction ("Forward", "Reverse", or "None")
        start_point: Optional FreeCAD.Vector for toolpath start (Z component ignored)
        max_depth_per_pass: Maximum depth to cut in single pass (None = single pass)
        pattern: "zigzag", "unidirectional", "spiral"
        milling_direction: "climb" or "conventional"
        cutting_feedrate: Feedrate for cutting moves (mm/min)
        approach_distance: Distance to approach from outside the polygon (None = 1.5 * tool_diameter)
        retract_height: Height to retract for rapids (None = start_depth + tool_diameter)
    
    Returns:
        List of Path.Command objects representing the toolpath
        
    Raises:
        ValueError: If input validation fails
    """
    
    Path.Log.debug(f"Generating facing toolpath: tool_diameter={tool_diameter}, stepover_percent={stepover_percent}")
    
    # Validate inputs
    _validate_inputs(wire, tool_diameter, stepover_percent, start_depth, final_depth, 
                    start_point, pattern, milling_direction)
    
    # Convert stepover percentage to absolute distance
    stepover = (stepover_percent / 100.0) * tool_diameter
    Path.Log.debug(f"Calculated stepover distance: {stepover}")
    
    # Get wire bounding box
    bb = wire.BoundBox
    tool_radius = tool_diameter / 2.0
    
    # Calculate default values
    if approach_distance is None:
        approach_distance = max(tool_diameter * 1.5, 10.0)
    
    if retract_height is None:
        retract_height = start_depth + tool_diameter
    
    if max_depth_per_pass is None:
        max_depth_per_pass = abs(start_depth - final_depth)
    
    # Determine approach direction from start point or auto-select
    actual_approach = _determine_approach_from_start_point(bb, start_point)
    Path.Log.debug(f"Using approach direction: {actual_approach}")
    Path.Log.debug(f"Wire bounding box: XMin={bb.XMin}, XMax={bb.XMax}, YMin={bb.YMin}, YMax={bb.YMax}, XLength={bb.XLength}, YLength={bb.YLength}")
    
    # Calculate depth passes
    depth_passes = _calculate_depth_passes(start_depth, final_depth, max_depth_per_pass)
    Path.Log.debug(f"Depth passes: {depth_passes}")
    
    # Generate clearing boundary (expanded by tool radius only)
    clearing_boundary = _generate_clearing_boundary(bb, tool_diameter)
    
    commands = []
    
    # Start with simple retract to safe height
    commands.append(Path.Command("G0", {"Z": retract_height}))
    
    # Process each depth pass
    for pass_num, target_depth in enumerate(depth_passes):
        Path.Log.debug(f"Processing depth pass {pass_num + 1}/{len(depth_passes)} to depth {target_depth}")
        
        # Generate cutting pattern for this depth
        pass_commands = _generate_cutting_pattern(
            clearing_boundary,
            tool_diameter,
            stepover,
            target_depth,
            pattern,
            milling_direction,
            cutting_feedrate,
            pass_num,
            spindle_direction
        )
        
        commands.extend(pass_commands)
    
    # Final retract
    commands.append(Path.Command("G0", {"Z": retract_height}))
    
    Path.Log.debug(f"Generated {len(commands)} commands")
    return commands


def _determine_approach_from_start_point(bb, start_point):
    """Determine approach direction from start point or auto-select."""
    if start_point is None:
        # Auto-select based on geometry aspect ratio
        if bb.XLength >= bb.YLength:
            return "X-"  # Approach from negative X (longer or equal dimension)
        else:
            return "Y-"  # Approach from negative Y
    else:
        # Determine approach direction based on start point location relative to bounding box
        center_x = (bb.XMin + bb.XMax) / 2.0
        center_y = (bb.YMin + bb.YMax) / 2.0
        
        dx = start_point.x - center_x
        dy = start_point.y - center_y
        
        # Choose approach direction based on which side the start point is closest to
        if abs(dx) > abs(dy):
            return "X-" if dx < 0 else "X+"
        else:
            return "Y-" if dy < 0 else "Y+"


def _calculate_depth_passes(start_depth, final_depth, max_depth_per_pass):
    """Calculate the Z depths for each cutting pass."""
    total_depth = abs(start_depth - final_depth)
    num_passes = max(1, math.ceil(total_depth / max_depth_per_pass))
    
    passes = []
    for i in range(num_passes):
        depth = start_depth - (i + 1) * (total_depth / num_passes)
        passes.append(depth)
    
    # Ensure final pass hits exact final depth
    passes[-1] = final_depth
    
    return passes


def _generate_clearing_boundary(bb, tool_diameter):
    """Generate clearing boundary box expanded by tool radius for clearance."""
    tool_radius = tool_diameter / 2.0
    
    # Expand bounding box by tool radius in all directions for clearance
    clearing_bb = FreeCAD.BoundBox(bb)
    clearing_bb.XMin -= tool_radius
    clearing_bb.XMax += tool_radius
    clearing_bb.YMin -= tool_radius
    clearing_bb.YMax += tool_radius
    
    return clearing_bb


def _get_approach_position(clearing_bb, approach_direction, z_height, start_point=None, approach_distance=None):
    """Get the initial approach position outside the material."""
    
    # Default approach distance if not provided
    if approach_distance is None:
        approach_distance = 10.0
    
    Path.Log.debug(f"_get_approach_position: approach_direction={approach_direction}, start_point={start_point}, approach_distance={approach_distance}")
    
    # Calculate approach position based on direction, extended by approach distance
    if start_point is not None:
        # Use start point to determine which side to approach from, but still apply approach distance
        if approach_direction == "X-":
            return FreeCAD.Vector(clearing_bb.XMin - approach_distance, start_point.y, z_height)
        elif approach_direction == "X+":
            return FreeCAD.Vector(clearing_bb.XMax + approach_distance, start_point.y, z_height)
        elif approach_direction == "Y-":
            return FreeCAD.Vector(start_point.x, clearing_bb.YMin - approach_distance, z_height)
        elif approach_direction == "Y+":
            return FreeCAD.Vector(start_point.x, clearing_bb.YMax + approach_distance, z_height)
        else:
            # Default to X- approach with start point Y
            return FreeCAD.Vector(clearing_bb.XMin - approach_distance, start_point.y, z_height)
    else:
        # Default positions based on approach direction, extended by approach distance
        if approach_direction == "X-":
            return FreeCAD.Vector(clearing_bb.XMin - approach_distance, clearing_bb.YMin, z_height)
        elif approach_direction == "X+":
            return FreeCAD.Vector(clearing_bb.XMax + approach_distance, clearing_bb.YMin, z_height)
        elif approach_direction == "Y-":
            return FreeCAD.Vector(clearing_bb.XMin, clearing_bb.YMin - approach_distance, z_height)
        elif approach_direction == "Y+":
            return FreeCAD.Vector(clearing_bb.XMin, clearing_bb.YMax + approach_distance, z_height)
        else:
            # Default to X- approach
            return FreeCAD.Vector(clearing_bb.XMin - approach_distance, clearing_bb.YMin, z_height)


def _generate_cutting_pattern(
    clearing_bb,
    tool_diameter,
    stepover,
    target_depth,
    pattern,
    milling_direction,
    cutting_feedrate,
    pass_num,
    spindle_direction
):
    """Generate the cutting pattern for one depth pass."""
    commands = []
    tool_radius = tool_diameter / 2.0
    
    # Simple X-axis cutting (left to right), stepping in Y
    primary_axis = "X"
    step_axis = "Y"
    primary_start = clearing_bb.XMin
    primary_end = clearing_bb.XMax
    step_start = clearing_bb.YMin + tool_radius
    step_end = clearing_bb.YMax - tool_radius
    
    # Calculate step positions
    step_positions = []
    current_step = step_start
    while current_step <= step_end:
        step_positions.append(current_step)
        current_step += stepover
    
    # Ensure we cover the end
    if step_positions and step_positions[-1] < step_end - 0.001:
        step_positions.append(step_end)
    
    # Generate simple cutting moves - always left to right for now
    for i, step_pos in enumerate(step_positions):
        # Simple pattern: zigzag (alternate direction each pass)
        if pattern == "zigzag" and i % 2 == 1:
            start_pos = primary_end  # Start from right
            end_pos = primary_start  # Cut to left
        else:
            start_pos = primary_start  # Start from left
            end_pos = primary_end     # Cut to right
        
        # Move to start of cut if needed
        if primary_axis == "X":
            start_point = FreeCAD.Vector(start_pos, step_pos, target_depth)
            end_point = FreeCAD.Vector(end_pos, step_pos, target_depth)
        else:
            start_point = FreeCAD.Vector(step_pos, start_pos, target_depth)
            end_point = FreeCAD.Vector(step_pos, end_pos, target_depth)
        
        # Rapid to start position (if first move or significant position change)
        if not commands or _needs_rapid_move(commands[-1], start_point):
            commands.append(Path.Command("G0", {
                "X": start_point.x,
                "Y": start_point.y,
                "Z": target_depth
            }))
        
        # Cutting move
        cut_params = {
            "X": end_point.x,
            "Y": end_point.y,
            "Z": target_depth
        }
        if cutting_feedrate is not None:
            cut_params["F"] = cutting_feedrate
        
        commands.append(Path.Command("G1", cut_params))
    
    return commands


def _needs_rapid_move(last_command, target_point):
    """Check if a rapid move is needed to reach the target point."""
    if last_command.Name != "G1":
        return True
    
    last_x = last_command.Parameters.get("X", 0)
    last_y = last_command.Parameters.get("Y", 0)
    
    distance = math.sqrt((target_point.x - last_x)**2 + (target_point.y - last_y)**2)
    return distance > 0.1  # Threshold for requiring rapid move


def _determine_climb_step_order(approach_direction, milling_direction, spindle_direction=None):
    """Determine if step positions should be in climb order.
    
    Returns True for climb order, False for conventional order.
    """
    # Use legacy logic for step direction - Compass class has incorrect logic for facing operations
    # TODO: Fix Compass class logic for facing operations
    # For climb milling, the cutter rotates in the direction of feed
    # For conventional milling, the cutter rotates opposite to feed
    
    if approach_direction in ["X-", "X+"]:
        # Stepping in Y direction
        if milling_direction == "climb":
            return approach_direction == "X-"  # Y+ steps for X- approach
        else:  # conventional
            return approach_direction == "X+"  # Y+ steps for X+ approach
    else:  # Y approach
        # Stepping in X direction  
        if milling_direction == "climb":
            return approach_direction == "Y-"  # X+ steps for Y- approach
        else:  # conventional
            return approach_direction == "Y+"  # X+ steps for Y+ approach


def _determine_cutting_direction(pass_index, pattern, approach_direction, milling_direction, 
                               primary_start, primary_end, spindle_direction=None):
    """Determine cutting direction for a specific pass.
    
    Returns (start_pos, end_pos) tuple.
    """
    # Use legacy logic for cutting direction - Compass class has incorrect logic for facing operations
    # TODO: Fix Compass class logic for facing operations where X- approach cuts along X axis, not Y axis
    base_forward = _legacy_cutting_direction(approach_direction, milling_direction, pass_index, pattern)
    Path.Log.debug(f"Cutting direction: approach={approach_direction}, pass={pass_index}, milling={milling_direction}, base_forward={base_forward}")
    
    # Return start and end positions
    if base_forward:
        result = (primary_start, primary_end)
    else:
        result = (primary_end, primary_start)
    
    Path.Log.debug(f"Cutting direction result: primary_start={primary_start}, primary_end={primary_end}, base_forward={base_forward}, result={result}")
    return result


def _legacy_cutting_direction(approach_direction, milling_direction, pass_index, pattern):
    """Legacy logic for determining cutting direction."""
    # Base direction depends on approach and milling type
    if approach_direction in ["X-", "X+"]:
        # Cutting along X axis
        if milling_direction == "climb":
            base_forward = approach_direction == "X-"  # X- approach: start from left (XMin)
        else:  # conventional
            base_forward = approach_direction == "X+"  # X- approach: start from right (XMax)
        Path.Log.debug(f"X-axis cutting: approach={approach_direction}, milling={milling_direction}, base_forward={base_forward}")
    else:
        # Cutting along Y axis
        if milling_direction == "climb":
            base_forward = approach_direction == "Y-"
        else:  # conventional
            base_forward = approach_direction == "Y+"
    
    # Apply pattern modifications
    if pattern == "zigzag" and pass_index % 2 == 1:
        base_forward = not base_forward
    elif pattern == "unidirectional":
        # Always same direction
        pass
        
    return base_forward


def _validate_inputs(wire, tool_diameter, stepover_percent, start_depth, final_depth, 
                    start_point, pattern, milling_direction):
    """Validate all input parameters."""
    
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
