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

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


def extract_polygon_geometry(polygon):
    """Extract edges and corners from a rectangular polygon."""
    edges = []
    corners = []

    # Validate that polygon has exactly 4 edges (rectangle)
    if len(polygon.Edges) != 4:
        raise ValueError("Polygon must be rectangular (4 edges)")

    for edge in polygon.Edges:
        edge_vector = edge.Vertexes[1].Point.sub(edge.Vertexes[0].Point)
        edges.append(
            {
                "start": edge.Vertexes[0].Point,
                "end": edge.Vertexes[1].Point,
                "vector": edge_vector,
                "length": edge.Length,
            }
        )
        corners.append(edge.Vertexes[0].Point)

    return {"edges": edges, "corners": corners}


def select_primary_step_edges(edges, axis_preference):
    """Select primary and step edges based on axis preference."""
    edge_lengths = [edge["length"] for edge in edges]
    unique_lengths = list(set(edge_lengths))

    if len(unique_lengths) == 1:
        # Square case - all edges are the same length
        # For squares, we need to pick two perpendicular edges
        primary_edge = edges[0]
        step_edge = None

        for edge in edges[1:]:
            # Check if this edge is perpendicular to the primary edge
            dot_product = abs(primary_edge["vector"].normalize().dot(edge["vector"].normalize()))
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
        long_edges = [edge for edge in edges if abs(edge["length"] - long_length) < 1e-6]
        short_edges = [edge for edge in edges if abs(edge["length"] - short_length) < 1e-6]

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
    primary_vec = primary_edge["vector"]
    step_vec = step_edge["vector"]

    # Manual normalization to ensure it works correctly
    primary_length_calc = primary_vec.Length
    step_length_calc = step_vec.Length

    if primary_length_calc > 0:
        primary_vec = primary_vec.multiply(1.0 / primary_length_calc)
    if step_length_calc > 0:
        step_vec = step_vec.multiply(1.0 / step_length_calc)

    return {
        "primary_edge": primary_edge,
        "step_edge": step_edge,
        "primary_vec": primary_vec,
        "step_vec": step_vec,
        "primary_length": primary_edge["length"],
        "step_length": step_edge["length"],
    }


def select_starting_corner(corners, primary_vec, step_vec, milling_direction):
    """
    Select starting corner based on milling direction and edge orientation.

    For climb milling (clockwise spiral), start from the corner with minimum
    combined projection (bottom-left in the primary/step coordinate system).
    For conventional milling (counter-clockwise spiral), start from the opposite corner.

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

    # Find the corner with minimum combined projection in the primary/step coordinate system
    # This is the "origin" corner in the rotated coordinate frame
    min_projection = float("inf")
    selected_corner = corners[0]

    for corner in corners:
        # Project corner onto the primary and step vectors
        primary_proj = corner.dot(primary_vec)
        step_proj = corner.dot(step_vec)

        # Combined projection gives distance from origin in direction space
        combined_proj = primary_proj + step_proj

        if combined_proj < min_projection:
            min_projection = combined_proj
            selected_corner = corner

    # For conventional milling, start from the diagonally opposite corner
    if milling_direction == "conventional":
        # Find the corner that's furthest from the selected corner (diagonal opposite)
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
        FreeCAD.Vector(bounding_box.XMin, bounding_box.YMax, bounding_box.ZMin),
    ]

    # Close the polygon by adding the first corner again
    corners.append(corners[0])
    bounding_wire = Part.makePolygon(corners)

    # Step 3: Rotate the bounding box to the desired angle
    bounding_wire.rotate(center, rotation_axis, angle)

    return bounding_wire


def calculate_engagement_offset(tool_diameter, stepover_percent):
    """Calculate the engagement offset for proper tool engagement.

    For 50% stepover, engagement should be 50% of tool diameter.
    engagement_offset is how much of the tool is NOT engaged.
    """
    return tool_diameter * (1.0 - stepover_percent / 100.0)


def validate_inputs(
    wire,
    tool_diameter,
    stepover_percent,
    start_depth,
    final_depth,
    start_point,
    pattern,
    milling_direction,
):
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
        Path.Log.warning(
            f"Very small stepover percentage ({stepover_percent}%) may result in excessive cutting time"
        )

    # Validate depths
    if start_depth == final_depth:
        raise ValueError("Start depth must be different from final depth")

    # Validate start point if provided
    if start_point is not None:
        if not hasattr(start_point, "x") or not hasattr(start_point, "y"):
            raise ValueError("Start point must be a FreeCAD.Vector with x and y coordinates")

        # Check if start point is too close to polygon (within bounding box + tool radius)
        tool_radius = tool_diameter / 2.0
        expanded_bb_xmin = bb.XMin - tool_radius
        expanded_bb_xmax = bb.XMax + tool_radius
        expanded_bb_ymin = bb.YMin - tool_radius
        expanded_bb_ymax = bb.YMax + tool_radius

        if (
            expanded_bb_xmin <= start_point.x <= expanded_bb_xmax
            and expanded_bb_ymin <= start_point.y <= expanded_bb_ymax
        ):
            raise ValueError("Start point is too close to the polygon to be cleared")

    # Validate pattern
    valid_patterns = ["zigzag", "unidirectional", "spiral"]
    if pattern not in valid_patterns:
        raise ValueError(f"Invalid pattern: {pattern}. Must be one of {valid_patterns}")

    # Validate milling direction
    valid_directions = ["climb", "conventional"]
    if milling_direction not in valid_directions:
        raise ValueError(
            f"Invalid milling direction: {milling_direction}. Must be one of {valid_directions}"
        )


def align_edges_to_angle(primary_vec, step_vec, primary_length, step_length, angle_degrees):
    """Ensure primary_vec aligns with the desired angle direction.

    If the provided angle direction is closer to step_vec than primary_vec,
    swap the vectors and their associated lengths so primary_vec matches the
    intended cut direction. This prevents sudden flips around angles like 46°/90°.
    """
    if angle_degrees is None:
        return primary_vec, step_vec, primary_length, step_length

    # Build a unit vector for the angle in the XY plane (degrees)
    import math

    rad = math.radians(angle_degrees)
    dir_vec = FreeCAD.Vector(math.cos(rad), math.sin(rad), 0)

    # Compare absolute alignment; both primary_vec and step_vec should be normalized already by caller
    # If not, normalize here safely.
    def norm(v):
        L = v.Length
        return v if L == 0 else v.multiply(1.0 / L)

    p = norm(primary_vec)
    s = norm(step_vec)

    if abs(dir_vec.dot(p)) >= abs(dir_vec.dot(s)):
        return p, s, primary_length, step_length
    else:
        # Swap so primary follows requested angle direction
        return s, p, step_length, primary_length


def unit_vectors_from_angle(angle_degrees):
    """Return (primary_vec, step_vec) unit vectors from angle in degrees in XY plane.

    primary_vec points in the angle direction. step_vec is +90° rotation (left normal).
    """
    import math

    rad = math.radians(angle_degrees)
    p = FreeCAD.Vector(math.cos(rad), math.sin(rad), 0)
    # +90° rotation
    s = FreeCAD.Vector(-math.sin(rad), math.cos(rad), 0)
    return p, s


def project_bounds(wire, vec, origin):
    """Project all vertices of wire onto vec relative to origin and return (min_t, max_t)."""
    ts = []
    for v in wire.Vertexes:
        ts.append(vec.dot(v.Point.sub(origin)))
    return (min(ts), max(ts))


def generate_t_values(wire, step_vec, tool_diameter, stepover_percent, origin):
    """Generate step positions along step_vec with engagement offset and stepover.

    The first pass engages (100 - stepover_percent)% of the tool diameter.
    For 50% stepover, first pass engages 50% of tool diameter.
    Tool center is positioned so the engaged portion touches the polygon edge.
    """
    tool_radius = tool_diameter / 2.0
    stepover = tool_diameter * (stepover_percent / 100.0)
    min_t, max_t = project_bounds(wire, step_vec, origin)

    # Calculate how much of the tool should engage on first pass
    # For 20% stepover: engage 20% of diameter
    # For 50% stepover: engage 50% of diameter
    engagement_amount = tool_diameter * (stepover_percent / 100.0)

    # Start position: tool center positioned so engagement_amount reaches polygon edge
    # Tool center at: min_t - tool_radius + engagement_amount
    # This positions the engaged portion at the polygon edge
    t = min_t - tool_radius + engagement_amount
    t_end = max_t + tool_radius - engagement_amount

    values = []
    # Guard against zero/negative stepover
    if stepover <= 0:
        return [t]
    while t <= t_end + 1e-9:
        values.append(t)
        t += stepover
    return values


def slice_wire_segments(wire, primary_vec, step_vec, t, origin):
    """Intersect the polygon wire with the infinite line at step coordinate t.

    Returns a sorted list of (s_in, s_out) intervals along primary_vec within the polygon.
    """
    import math

    # For diagnostics
    bb = wire.BoundBox
    diag = math.hypot(bb.XLength, bb.YLength)
    s_debug_threshold = max(1.0, diag * 10.0)

    s_vals = []
    # Use scale-relative tolerance for near-parallel detection
    eps_abs = 1e-12
    t_scale = max(abs(t), diag, 1.0)
    eps_parallel = max(eps_abs, t_scale * 1e-9)

    # Iterate edges
    for edge in wire.Edges:
        A = FreeCAD.Vector(edge.Vertexes[0].Point)
        B = FreeCAD.Vector(edge.Vertexes[1].Point)
        a = step_vec.dot(A.sub(origin))
        b = step_vec.dot(B.sub(origin))
        da = a - t
        db = b - t
        # Check for crossing; ignore edges parallel to the step line (db == da)
        denom = b - a
        if not (math.isfinite(a) and math.isfinite(b) and math.isfinite(denom)):
            continue
        # Reject near-parallel edges using scale-relative tolerance
        if abs(denom) < eps_parallel:
            continue
        # Crossing if signs differ or one is zero
        if da == 0.0 and db == 0.0:
            # Line coincident with edge: skip to avoid degenerate doubles
            continue
        if (da <= 0 and db >= 0) or (da >= 0 and db <= 0):
            # Linear interpolation factor u from A to B
            u = (t - a) / (b - a)
            if not math.isfinite(u):
                continue
            # Strict bounds check: u must be in [0,1] with tight tolerance
            # Reject if u is way outside - indicates numerical instability
            if u < -0.01 or u > 1.01:
                continue
            # Clamp u to [0,1] for valid intersections
            u = max(0.0, min(1.0, u))

            # Interpolate using scalars to avoid any in-place vector mutation
            ux = A.x + u * (B.x - A.x)
            uy = A.y + u * (B.y - A.y)
            uz = A.z + u * (B.z - A.z)
            s = (
                primary_vec.x * (ux - origin.x)
                + primary_vec.y * (uy - origin.y)
                + primary_vec.z * (uz - origin.z)
            )

            # Final sanity check: reject if s is absurdly large
            if not math.isfinite(s) or abs(s) > diag * 100.0:
                continue
            s_vals.append(s)

    # Sort and pair successive intersections into interior segments
    s_vals.sort()
    segments = []
    for i in range(0, len(s_vals) - 1, 2):
        s0 = s_vals[i]
        s1 = s_vals[i + 1]
        if s1 > s0 + 1e-9 and math.isfinite(s0) and math.isfinite(s1):
            segments.append((s0, s1))
    return segments
