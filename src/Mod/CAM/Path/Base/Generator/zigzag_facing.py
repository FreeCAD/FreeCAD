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


def _create_link(prev_seg, next_seg, link_mode, link_radius, stepover_distance, tool_radius, primary_vec, z):
    """
    Create linking moves between two segments.
    
    Args:
        prev_seg: Previous segment dict with 'end', 'side', 't' keys
        next_seg: Next segment dict with 'start', 'side', 't' keys
        link_mode: "arc" or "straight"
        link_radius: Radius for arc links (None = auto)
        stepover_distance: Distance between passes
        tool_radius: Tool radius
        primary_vec: Primary direction vector
        z: Z height
        
    Returns:
        List of Path.Command objects for the link
    """
    import math
    
    P = prev_seg['end']
    Q = next_seg['start']
    
    # Safety checks
    if not (math.isfinite(P.x) and math.isfinite(P.y) and 
            math.isfinite(Q.x) and math.isfinite(Q.y)):
        return [Path.Command("G0", {"X": Q.x, "Y": Q.y})]
    
    # Check if we should use arc mode
    if link_mode != "arc":
        return [Path.Command("G0", {"X": Q.x, "Y": Q.y})]
    
    # Calculate chord vector and distance
    dx = Q.x - P.x
    dy = Q.y - P.y
    chord_length = math.sqrt(dx*dx + dy*dy)
    
    # Minimum chord length check
    if chord_length < 1e-6:
        return [Path.Command("G0", {"X": Q.x, "Y": Q.y})]
    
    # Natural semicircle radius for 180° arc
    r0 = chord_length / 2.0
    
    # Use specified radius or default to natural radius
    if link_radius is not None and link_radius > r0:
        r = link_radius
    else:
        r = r0
    
    # Minimum radius check
    if r < 1e-6:
        return [Path.Command("G0", {"X": Q.x, "Y": Q.y})]
    
    # Calculate arc center
    # Midpoint of chord
    mx = 0.5 * (P.x + Q.x)
    my = 0.5 * (P.y + Q.y)
    
    # Normal to chord (rotate chord by 90°)
    # Two options: rotate left (-dy, dx) or rotate right (dy, -dx)
    # For zigzag, we need the arc to bulge in the primary direction away from center
    # Use cross product to determine which way to rotate the chord
    
    # The arc should bulge in the direction perpendicular to the chord
    # and in the same primary direction as the segment side
    # For vertical chords (dy != 0, dx ≈ 0), normal is horizontal
    # For horizontal chords (dx != 0, dy ≈ 0), normal is vertical
    
    # Calculate both possible normals (90° rotations of chord)
    n1x = -dy / chord_length  # Rotate left
    n1y = dx / chord_length
    n2x = dy / chord_length   # Rotate right  
    n2y = -dx / chord_length
    
    # Choose normal based on which side we're on
    # The arc should bulge in the primary direction indicated by prev_seg['side']
    outward_x = primary_vec.x * prev_seg['side']
    outward_y = primary_vec.y * prev_seg['side']
    
    dot1 = n1x * outward_x + n1y * outward_y
    dot2 = n2x * outward_x + n2y * outward_y
    
    if dot1 > dot2:
        nx, ny = n1x, n1y
        Path.Log.debug(f"  Chose n1: ({nx:.3f}, {ny:.3f}), dot1={dot1:.3f} > dot2={dot2:.3f}")
    else:
        nx, ny = n2x, n2y
        Path.Log.debug(f"  Chose n2: ({nx:.3f}, {ny:.3f}), dot2={dot2:.3f} > dot1={dot1:.3f}")
    
    Path.Log.debug(f"  Chord: dx={dx:.3f}, dy={dy:.3f}, side={prev_seg['side']}, outward=({outward_x:.3f},{outward_y:.3f})")
    
    # Calculate offset distance for the arc center from the chord midpoint
    # Geometry: For an arc with radius r connecting two points separated by chord length 2*r0,
    # the center must be perpendicular to the chord at distance offset from the midpoint,
    # where: r^2 = r0^2 + offset^2 (Pythagorean theorem)
    # Therefore: offset = sqrt(r^2 - r0^2)
    # 
    # For r = r0 (minimum possible radius): offset = 0 (semicircle, 180° arc)
    # For r > r0: offset > 0 (less than semicircle)
    # For nice smooth arcs, we could use r = 2*r0, giving offset = sqrt(3)*r0
    
    if r >= r0:
        offset = math.sqrt(r*r - r0*r0)
    else:
        # Radius too small to connect endpoints - shouldn't happen but handle gracefully
        offset = 0.0
    
    # Arc center
    cx = mx + nx * offset
    cy = my + ny * offset
    
    # Verify center is finite
    if not (math.isfinite(cx) and math.isfinite(cy)):
        return [Path.Command("G0", {"X": Q.x, "Y": Q.y})]
    
    # Determine arc direction (G2=CW, G3=CCW)
    # For semicircles where center is on the chord, cross product is unreliable
    # Instead, use the normal direction and chord direction to determine arc sense
    # The arc goes from P to Q, bulging in direction (nx, ny)
    # Cross product of chord direction with normal gives us the arc direction
    # chord × normal = (dx, dy, 0) × (nx, ny, 0) = (0, 0, dx*ny - dy*nx)
    z_cross = dx * ny - dy * nx
    
    Path.Log.debug(f"  z_cross = {dx:.3f}*{ny:.3f} - {dy:.3f}*{nx:.3f} = {z_cross:.3f}")
    
    # Invert the logic - positive cross product means clockwise for our convention
    if z_cross < 0:
        arc_cmd = "G3"  # Counter-clockwise
    else:
        arc_cmd = "G2"  # Clockwise
    
    # Calculate IJ (relative to start point P)
    I = cx - P.x
    J = cy - P.y
    
    # Verify IJ are finite
    if not (math.isfinite(I) and math.isfinite(J)):
        return [Path.Command("G0", {"X": Q.x, "Y": Q.y})]
    
    Path.Log.debug(f"Arc link: P=({P.x:.3f},{P.y:.3f}) Q=({Q.x:.3f},{Q.y:.3f}) "
                   f"C=({cx:.3f},{cy:.3f}) r={r:.3f} {arc_cmd} I={I:.3f} J={J:.3f}")
    
    # K=0 for XY plane arcs - use string format to ensure I, J, K are preserved
    cmd_string = f"{arc_cmd} I{I:.6f} J{J:.6f} K0.0 X{Q.x:.6f} Y{Q.y:.6f} Z{z:.6f}"
    return [Path.Command(cmd_string)]


def zigzag(polygon, tool_diameter, stepover_percent, pass_extension=None, retract_height=None, milling_direction="climb", reverse=False, angle_degrees=None, link_mode="arc", link_radius=None):
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
        link_mode: "arc" for smooth arc transitions, "straight" for G0 links (default: "arc")
        link_radius: Radius for arc links in mm (None = auto-calculate from pass spacing)
        
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
    
    # First pass: collect all segments
    segments = []
    s_margin = pass_extension + tool_diameter
    tool_radius = tool_diameter / 2.0
    engagement_offset = facing_common.calculate_engagement_offset(tool_diameter, stepover_percent)
    stepover_distance = tool_diameter * stepover_percent / 100.0

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
            parity = len(segments) % 2
            
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

            # Determine side: +1 if ending on right (larger s), -1 if on left
            s_mid = 0.5 * (s0 + s1)
            side = 1 if p_end > s_mid else -1
            
            segments.append({
                't': t,
                'side': side,
                'start': start_point,
                'end': end_point,
                's_start': p_start,
                's_end': p_end
            })
    
    Path.Log.debug(f"Zigzag: collected {len(segments)} segments")
    
    # Second pass: emit commands with arc links
    commands = []
    for i, seg in enumerate(segments):
        if i == 0:
            # First segment: emit G0 to start for op preamble replacement
            commands.append(Path.Command("G0", {"X": seg['start'].x, "Y": seg['start'].y, "Z": z}))
        else:
            # Link from previous segment end to this segment start
            prev_seg = segments[i - 1]
            link_commands = _create_link(prev_seg, seg, link_mode, link_radius, 
                                        stepover_distance, tool_radius, primary_vec, z)
            commands.extend(link_commands)
        
        # Emit cutting move
        commands.append(Path.Command("G1", {"X": seg['end'].x, "Y": seg['end'].y, "Z": z}))
    
    Path.Log.debug(f"Zigzag: generated {len(commands)} commands from {len(segments)} segments")
    return commands

