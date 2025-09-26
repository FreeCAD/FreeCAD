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
    
    # Get vertices from edges
    vertices = []
    for edge in edges:
        vertices.append(edge.Vertexes[0].Point)
    
    # Find the two edge vectors to determine rectangle orientation
    edge1_vec = vertices[1].sub(vertices[0])
    edge2_vec = vertices[2].sub(vertices[1])
    
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
    # We want the corner that is at the "origin" of our coordinate system
    # This should be the corner where both primary_vec and step_vec point "outward"
    reference_corner = None
    min_projection = float('inf')
    
    for vertex in vertices:
        # Project this vertex onto both direction vectors
        # The reference corner should have the minimum projections in both directions
        primary_proj = vertex.dot(primary_vec)
        step_proj = vertex.dot(step_vec)
        combined_proj = primary_proj + step_proj
        
        if combined_proj < min_projection:
            min_projection = combined_proj
            reference_corner = vertex
    
    if reference_corner is None:
        reference_corner = vertices[0]  # Fallback

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

def _spiral(polygon, tool_diameter, stepover_percent, axis_preference="long"):
    """
    Generate a spiral clearing pattern.
    
    This strategy cuts in a continuous spiral from the outside toward the center
    (or center outward). The tool follows a rectangular spiral path that gradually
    works inward, maintaining a continuous cutting motion. This minimizes tool
    lifting and provides smooth material removal, but may require careful
    consideration of chip evacuation in deep pockets.
    
    Args:
        polygon: The polygon boundary to clear
        tool_diameter: Diameter of the cutting tool
        stepover_percent: Stepover as percentage of tool diameter
        axis_preference: "long" or "short" - which axis to align primary cutting direction with
    """
    pass

def _bidirectional(polygon, tool_diameter, stepover_percent, axis_preference="long", pass_extension=None):
    """
    Generate a bidirectional clearing pattern.
    
    This strategy cuts back and forth across the polygon like zigzag, but only
    makes cutting moves on the long sides (similar to spiral pattern). The end
    connections between passes are rapid moves that stay outside the stock area,
    so no tool lifting is required. This provides efficient material removal
    while avoiding cutting on the short ends of the rectangle, which can be
    beneficial for surface finish and tool life.
    
    Args:
        polygon: The polygon boundary to clear
        tool_diameter: Diameter of the cutting tool
        stepover_percent: Stepover as percentage of tool diameter
        axis_preference: "long" or "short" - which axis to align primary cutting direction with
        pass_extension: Distance to extend cuts beyond polygon boundary for tool disengagement
    """
    pass

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
