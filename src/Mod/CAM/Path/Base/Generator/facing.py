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

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


def generate(
    wire,
    tool_diameter,
    stepover_percent,
    start_depth,
    final_depth,
    start_point=None,
    max_depth_per_pass=None,
    pattern="zigzag",
    milling_direction="climb",
    cutting_feedrate=None,
    approach_distance=None,
    retract_height=None,
    tool_controller=None
):
    """
    Generate a facing toolpath for a closed polygon with proper side entry for large cutters.
    
    Args:
        wire: Part.Wire representing the closed polygon boundary
        tool_diameter: Diameter of the facing cutter
        stepover_percent: Stepover as percentage of tool diameter (e.g., 75 for 75%)
        start_depth: Z height to start cutting (typically stock top)
        final_depth: Z height to finish cutting (typically part surface)
        start_point: Optional FreeCAD.Vector for toolpath start (Z component ignored)
        max_depth_per_pass: Maximum depth to cut in single pass (None = single pass)
        pattern: "zigzag", "unidirectional", "spiral"
        milling_direction: "climb" or "conventional"
        cutting_feedrate: Feedrate for cutting moves (mm/min)
        approach_distance: Distance to approach from outside the polygon (None = 1.5 * tool_diameter)
        retract_height: Height to retract for rapids (None = start_depth + tool_diameter)
        tool_controller: Optional tool controller for spindle direction and cut mode
            If provided, will use Compass class for climb/conventional milling decisions
    
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
    
    # Calculate depth passes
    depth_passes = _calculate_depth_passes(start_depth, final_depth, max_depth_per_pass)
    Path.Log.debug(f"Depth passes: {depth_passes}")
    
    # Generate extended boundary for side entry
    extended_boundary = _generate_extended_boundary(
        bb, tool_diameter, approach_distance, actual_approach
    )
    
    commands = []
    
    # Add initial rapid to approach position
    approach_pos = _get_approach_position(extended_boundary, actual_approach, retract_height, start_point)
    commands.append(Path.Command("G0", {
        "X": approach_pos.x,
        "Y": approach_pos.y,
        "Z": retract_height
    }))
    
    # Process each depth pass
    for pass_num, target_depth in enumerate(depth_passes):
        Path.Log.debug(f"Processing depth pass {pass_num + 1}/{len(depth_passes)} to depth {target_depth}")
        
        # Generate cutting pattern for this depth
        spindle_dir = getattr(tool_controller, "SpindleDir", "None") if tool_controller else "None"
        pass_commands = _generate_cutting_pattern(
            extended_boundary,
            bb,
            tool_diameter,
            stepover,
            target_depth,
            actual_approach,
            pattern,
            milling_direction,
            cutting_feedrate,
            pass_num,
            spindle_dir
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
        if bb.XLength > bb.YLength:
            return "X-"  # Approach from negative X (longer dimension)
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


def _generate_extended_boundary(bb, tool_diameter, approach_distance, approach_direction):
    """Generate extended boundary box for side entry approach."""
    tool_radius = tool_diameter / 2.0
    
    # Start with original bounding box
    extended_bb = FreeCAD.BoundBox(bb)
    
    # Extend based on approach direction
    if approach_direction == "X-":
        extended_bb.XMin -= (approach_distance + tool_radius)
        extended_bb.XMax += tool_radius
        extended_bb.YMin -= tool_radius
        extended_bb.YMax += tool_radius
    elif approach_direction == "X+":
        extended_bb.XMin -= tool_radius
        extended_bb.XMax += (approach_distance + tool_radius)
        extended_bb.YMin -= tool_radius
        extended_bb.YMax += tool_radius
    elif approach_direction == "Y-":
        extended_bb.XMin -= tool_radius
        extended_bb.XMax += tool_radius
        extended_bb.YMin -= (approach_distance + tool_radius)
        extended_bb.YMax += tool_radius
    elif approach_direction == "Y+":
        extended_bb.XMin -= tool_radius
        extended_bb.XMax += tool_radius
        extended_bb.YMin -= tool_radius
        extended_bb.YMax += (approach_distance + tool_radius)
    else:
        # For angled approach, extend in all directions
        extended_bb.XMin -= (approach_distance + tool_radius)
        extended_bb.XMax += (approach_distance + tool_radius)
        extended_bb.YMin -= (approach_distance + tool_radius)
        extended_bb.YMax += (approach_distance + tool_radius)
    
    return extended_bb


def _get_approach_position(extended_bb, approach_direction, z_height, start_point=None):
    """Get the initial approach position outside the material."""
    if start_point is not None:
        # Use the provided start point (ignore Z component)
        return FreeCAD.Vector(start_point.x, start_point.y, z_height)
    
    # Default positions based on approach direction
    if approach_direction == "X-":
        return FreeCAD.Vector(extended_bb.XMin, extended_bb.YMin, z_height)
    elif approach_direction == "X+":
        return FreeCAD.Vector(extended_bb.XMax, extended_bb.YMin, z_height)
    elif approach_direction == "Y-":
        return FreeCAD.Vector(extended_bb.XMin, extended_bb.YMin, z_height)
    elif approach_direction == "Y+":
        return FreeCAD.Vector(extended_bb.XMin, extended_bb.YMax, z_height)
    else:
        # Default to X- approach
        return FreeCAD.Vector(extended_bb.XMin, extended_bb.YMin, z_height)


def _generate_cutting_pattern(
    extended_bb,
    original_bb,
    tool_diameter,
    stepover,
    target_depth,
    approach_direction,
    pattern,
    milling_direction,
    cutting_feedrate,
    pass_num,
    spindle_direction
):
    """Generate the cutting pattern for one depth pass."""
    commands = []
    tool_radius = tool_diameter / 2.0
    
    # Determine cutting direction based on approach
    if approach_direction in ["X-", "X+"]:
        # Cutting along X, stepping in Y
        primary_axis = "X"
        step_axis = "Y"
        primary_start = extended_bb.XMin
        primary_end = extended_bb.XMax
        step_start = extended_bb.YMin + tool_radius
        step_end = extended_bb.YMax - tool_radius
    else:
        # Cutting along Y, stepping in X
        primary_axis = "Y"
        step_axis = "X"
        primary_start = extended_bb.YMin
        primary_end = extended_bb.YMax
        step_start = extended_bb.XMin + tool_radius
        step_end = extended_bb.XMax - tool_radius
    
    # Calculate step positions
    step_positions = []
    current_step = step_start
    while current_step <= step_end:
        step_positions.append(current_step)
        current_step += stepover
    
    # Ensure we cover the end
    if step_positions and step_positions[-1] < step_end - 0.001:
        step_positions.append(step_end)
    
    # Apply milling direction - this affects the order of step positions
    # For climb milling: tool rotates in direction of feed
    # For conventional milling: tool rotates opposite to feed direction
    climb_step_order = _determine_climb_step_order(approach_direction, milling_direction, spindle_direction)
    if not climb_step_order:
        step_positions.reverse()
    
    # Generate cutting moves
    for i, step_pos in enumerate(step_positions):
        # Determine direction for this pass based on pattern and climb/conventional
        cutting_direction = _determine_cutting_direction(
            i, pattern, approach_direction, milling_direction, 
            primary_start, primary_end, spindle_direction
        )
        start_pos, end_pos = cutting_direction
        
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
        if cutting_feedrate:
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
    # Use Compass class if spindle_direction is provided
    if spindle_direction is not None and spindle_direction != "None":
        try:
            compass = PathOpBase.Compass(spindle_direction, "Area")
            compass.cut_mode = "Climb" if milling_direction == "climb" else "Conventional"
            result = compass.get_step_direction(approach_direction)
            Path.Log.debug(f"Compass step direction: spindle={spindle_direction}, mode={milling_direction}, approach={approach_direction} -> {result}")
            return result
        except Exception as e:
            Path.Log.warning(f"Failed to use Compass for step direction, falling back to legacy logic: {e}")
    
    # Legacy logic for backward compatibility
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
    # Use Compass class if spindle_direction is provided
    if spindle_direction is not None and spindle_direction != "None":
        try:
            compass = PathOpBase.Compass(spindle_direction, "Area")
            compass.cut_mode = "Climb" if milling_direction == "climb" else "Conventional"
            base_forward = compass.get_cutting_direction(approach_direction, pass_index, pattern)
        except Exception as e:
            Path.Log.warning(f"Failed to use Compass for cutting direction, falling back to legacy logic: {e}")
            base_forward = _legacy_cutting_direction(approach_direction, milling_direction, pass_index, pattern)
    else:
        # Legacy logic for backward compatibility
        base_forward = _legacy_cutting_direction(approach_direction, milling_direction, pass_index, pattern)
    
    # Return start and end positions
    if base_forward:
        return (primary_start, primary_end)
    else:
        return (primary_end, primary_start)


def _legacy_cutting_direction(approach_direction, milling_direction, pass_index, pattern):
    """Legacy logic for determining cutting direction."""
    # Base direction depends on approach and milling type
    if approach_direction in ["X-", "X+"]:
        # Cutting along X axis
        if milling_direction == "climb":
            base_forward = approach_direction == "X-"
        else:  # conventional
            base_forward = approach_direction == "X+"
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
