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

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())



def zigzag(polygon, tool_diameter, stepover_percent, pass_extension=None, retract_height=None, milling_direction="climb", reverse=False, angle_degrees=None):
    """
    Generate a zigzag clearing pattern.
    
    This strategy cuts back and forth across the polygon in alternating directions.
    Each pass reverses direction from the previous pass, creating a zigzag pattern.
    The tool cuts continuously without lifting between passes, minimizing air time.
    
    Args:
        polygon: The polygon boundary to clear
        tool_diameter: Diameter of the cutting tool
        stepover_percent: Stepover as percentage of tool diameter
        pass_extension: Distance to extend cuts beyond polygon boundary
        retract_height: Z height for rapid moves (None = cutting height)
        milling_direction: "climb" or "conventional" - affects direction of first pass
        reverse: Reverse the alternation pattern
        angle_degrees: Angle in degrees to rotate the cutting direction
        
    Returns:
        List of Path.Command objects representing the toolpath
    """

    if pass_extension is None:
        pass_extension = tool_diameter * 0.5
    
    import math
    # Establish frame from angle (default 0° = +X primary)
    theta = float(angle_degrees) if angle_degrees is not None else 0.0
    primary_vec, step_vec = facing_common.unit_vectors_from_angle(theta)
    # Make copies to avoid mutation
    primary_vec = FreeCAD.Vector(primary_vec).multiply(1.0 / (primary_vec.Length or 1.0))
    step_vec = FreeCAD.Vector(step_vec).multiply(1.0 / (step_vec.Length or 1.0))

    Path.Log.debug(
        f"Zigzag: dia={tool_diameter}, stepover%={stepover_percent}, "
        f"dir={milling_direction}, angle={theta}, reverse={reverse}"
    )

    origin = polygon.BoundBox.Center
    bb = polygon.BoundBox
    z = bb.ZMin

    # Compute projection bounds
    min_s, max_s = facing_common.project_bounds(polygon, primary_vec, origin)
    min_t, max_t = facing_common.project_bounds(polygon, step_vec, origin)
    if not (math.isfinite(min_s) and math.isfinite(max_s) and math.isfinite(min_t) and math.isfinite(max_t)):
        Path.Log.error("Zigzag: non-finite projection bounds; aborting")
        return []

    # Generate step positions
    step_positions = facing_common.generate_t_values(polygon, step_vec, tool_diameter, stepover_percent, origin)
    
    # Reverse controls direction: False=bottom-to-top, True=top-to-bottom
    Path.Log.debug(f"Zigzag: Before reverse - min_t={min_t}, max_t={max_t}, first_t={step_positions[0] if step_positions else None}, last_t={step_positions[-1] if step_positions else None}")
    if reverse:
        # Mirror positions around center to maintain proper engagement offset at max_t
        # This both reverses the order AND maintains proper engagement
        center = (min_t + max_t) / 2.0
        step_positions = [(2 * center - t) for t in step_positions]
        Path.Log.debug(f"Zigzag: After reverse - center={center}, first_t={step_positions[0] if step_positions else None}, last_t={step_positions[-1] if step_positions else None}")
    
    Path.Log.debug(f"Zigzag: {len(step_positions)} passes generated, reverse={reverse}, step_vec={step_vec}")
    
    commands = []
    s_margin = pass_extension + tool_diameter
    kept_segments = 0
    tool_radius = tool_diameter / 2.0
    engagement_offset = facing_common.calculate_engagement_offset(tool_diameter, stepover_percent)

    for idx, t in enumerate(step_positions):
        if idx == 0:
            # Debug first pass
            world_y = origin.y + t
            Path.Log.debug(f"Zigzag: first pass t={t}, origin.y={origin.y}, world_y={world_y}")
        
        intervals = facing_common.slice_wire_segments(polygon, primary_vec, step_vec, t, origin)
        # If no intervals found (pass is outside polygon), skip this pass
        if not intervals:
            continue
        
        for (s0, s1) in intervals:
            # Extend in primary direction
            total_extension = pass_extension + tool_radius + engagement_offset
            start_s = s0 - total_extension
            end_s = s1 + total_extension
            # Clip to safe bounds
            start_s = max(start_s, min_s - s_margin)
            end_s = min(end_s, max_s + s_margin)
            if end_s <= start_s:
                continue

            # Determine direction based on segment parity, milling mode, and reverse
            # reverse=False, climb: start right (end_s), alternate
            # reverse=False, conventional: start left (start_s), alternate
            # reverse=True, climb: start left (start_s), alternate
            # reverse=True, conventional: start right (end_s), alternate
            parity = kept_segments % 2
            
            if not reverse:
                # Bottom-to-top
                if milling_direction == "climb":
                    # Start right, alternate
                    if parity == 0:
                        p_start, p_end = end_s, start_s
                    else:
                        p_start, p_end = start_s, end_s
                else:  # conventional
                    # Start left, alternate
                    if parity == 0:
                        p_start, p_end = start_s, end_s
                    else:
                        p_start, p_end = end_s, start_s
            else:
                # Top-to-bottom
                if milling_direction == "climb":
                    # Start left, alternate
                    if parity == 0:
                        p_start, p_end = start_s, end_s
                    else:
                        p_start, p_end = end_s, start_s
                else:  # conventional
                    # Start right, alternate
                    if parity == 0:
                        p_start, p_end = end_s, start_s
                    else:
                        p_start, p_end = start_s, end_s

            # Map to XY - use copies to avoid vector mutation
            start_point = FreeCAD.Vector(origin).add(FreeCAD.Vector(primary_vec).multiply(p_start)).add(FreeCAD.Vector(step_vec).multiply(t))
            end_point = FreeCAD.Vector(origin).add(FreeCAD.Vector(primary_vec).multiply(p_end)).add(FreeCAD.Vector(step_vec).multiply(t))
            start_point.z = z
            end_point.z = z

            # Sanity check for absurdly large coordinates
            if not (math.isfinite(start_point.x) and math.isfinite(start_point.y) and 
                    math.isfinite(end_point.x) and math.isfinite(end_point.y)):
                continue

            if commands:
                # Rapid move at cutting height (no need to retract since tool is outside stock)
                commands.append(Path.Command("G0", {"X": start_point.x, "Y": start_point.y}))
            else:
                # First segment: emit G0 to start for op preamble replacement
                commands.append(Path.Command("G0", {"X": start_point.x, "Y": start_point.y, "Z": z}))

            commands.append(Path.Command("G1", {"X": end_point.x, "Y": end_point.y, "Z": z}))
            kept_segments += 1
    
    Path.Log.debug(f"Zigzag: generated {kept_segments} segments")
    return commands

