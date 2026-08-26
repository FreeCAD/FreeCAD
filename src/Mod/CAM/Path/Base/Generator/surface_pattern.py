# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2025 Dimitrios Pana <dimitriospana75@gmail.com>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""
2D Pattern Generation Engine for the 3D Surface Operation.

This module is the single source of truth for creating 2D toolpath coordinates
for all pattern-based strategies (Line, ZigZag, Circular, Spiral, Offset).

It serves two primary roles:
1.  **C++ Bridge:** It acts as the high-performance bridge to the compiled C++
    `surface_generator` module, which instantly generates and clips the raw
    coordinates for linear and radial patterns.
2.  **Offset Generator:** It contains the pure Python implementation for the
    'Offset' pattern, which relies on OpenCASCADE's robust offsetting engine
    via `PathUtils`.

The functions in this module return lists of 2D (x,y,z=0) coordinates that
are ready to be projected onto the 3D model by the OCL drop-cutter engine.
"""

import math
import Path
import Part
import FreeCAD

try:
    import surface_generator as _pattern_cpp

    Path.Log.info("Successfully loaded C++ surface generator module.")
except ImportError as e:
    Path.Log.critical("Failed to load the critical C++ surface generator module!")
    Path.Log.critical("The 3D Surface operation will be non-functional.")
    Path.Log.critical(f"Error details: {e}")
    # Re-raise the error to halt module loading.
    raise e

__title__ = "Surface Scan Pattern Generator"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


# ---------------------------------------------------------------------------
# Bounding-box helper
# ---------------------------------------------------------------------------


class BBox:
    """Lightweight axis-aligned bounding box for scan generation.

    Accepts either keyword arguments or a FreeCAD BoundBox-like object
    via the ``from_bbox`` class method.
    """

    __slots__ = ("xmin", "xmax", "ymin", "ymax")

    def __init__(self, xmin, xmax, ymin, ymax):
        self.xmin = float(xmin)
        self.xmax = float(xmax)
        self.ymin = float(ymin)
        self.ymax = float(ymax)

    @classmethod
    def from_bbox(cls, bb):
        """Create from a FreeCAD ``BoundBox`` (or any object with
        XMin/XMax/YMin/YMax attributes)."""
        return cls(bb.XMin, bb.XMax, bb.YMin, bb.YMax)

    @property
    def x_length(self):
        return self.xmax - self.xmin

    @property
    def y_length(self):
        return self.ymax - self.ymin

    @property
    def center(self):
        return (
            (self.xmin + self.xmax) / 2.0,
            (self.ymin + self.ymax) / 2.0,
        )

    @property
    def diagonal(self):
        return math.hypot(self.x_length, self.y_length)


# ---------------------------------------------------------------------------
# Operation Data Extraction
# ---------------------------------------------------------------------------


def split_selected_features(base_property, avoid_count):
    """
    Extracts and splits face geometry from an operation's Base property.

    This pure function takes the raw Base property and an integer, and it
    separates the Part.Face objects it finds into two lists: those to be
    machined and those to be avoided.

    Args:
        base_property (list): The operation's `obj.Base` list of (object, subnames).
        avoid_count (int): The number of faces from the end of the list to treat as 'avoid'.

    Returns:
        tuple: (cutting_faces, avoid_faces)
    """

    if not base_property:
        Path.Log.debug(
            "surface_pattern.split_selected_features: no Base geometry, using whole model"
        )
        return [], []

    all_selected = []
    cutting_faces, avoid_faces = [], []
    total_subs = 0
    for base, subs in base_property:
        Path.Log.debug(
            "surface_pattern.split_selected_features: base={}, subs={}".format(base.Label, subs)
        )
        for sub in subs:
            if not sub:
                Path.Log.debug(
                    "surface_pattern.split_selected_features: skipping empty sub-element for whole object"
                )
                continue
            total_subs += 1
            try:
                shape = base.Shape.getElement(sub)
                if shape and isinstance(shape, Part.Face):
                    all_selected.append(shape)
            except Exception:
                continue

    Path.Log.debug(
        "surface_pattern.split_selected_features: extraction completed for {} subs, {} faces".format(
            total_subs, len(all_selected)
        )
    )

    if not all_selected:
        return cutting_faces, avoid_faces

    if avoid_count > 0 and avoid_count < len(all_selected):
        cutting_faces = all_selected[:-avoid_count]
        avoid_faces = all_selected[-avoid_count:]
    elif avoid_count >= len(all_selected):
        avoid_faces = all_selected
    else:
        cutting_faces = all_selected

    Path.Log.debug(
        "surface_pattern.split_selected_features: AvoidLastX_Faces process completed for {} cut, {} avoid faces".format(
            len(cutting_faces), len(avoid_faces)
        )
    )

    return cutting_faces, avoid_faces


def group_features(faces_to_group, handle_mode):
    """
    Groups a list of faces based on the requested strategy.

    This is a pure utility function that takes a list of faces and returns a
    list of face-groups to be processed by the main operation loop.

    Args:
        faces_to_group (list): The definitive list of Part.Face objects to process.
        handle_mode (str): The user's selection ("Individually" or "Collectively").

    Returns:
        list: A list of lists, e.g., `[[f1, f2]]` for collective or `[[f1], [f2]]` for individual.
    """
    if handle_mode == "Individually" and faces_to_group:
        Path.Log.debug(f"Preparing to process {len(faces_to_group)} features individually.")
        return [[face] for face in faces_to_group]
    else:
        # Default to collective mode for safety and for the "whole model" case
        if len(faces_to_group) > 1:
            Path.Log.debug("Preparing to process all selected features collectively.")
        return [faces_to_group]


# ---------------------------------------------------------------------------
# Scan lines reconstruction
# ---------------------------------------------------------------------------


def reconstruct_scan_lines(flat_points, gap_threshold):
    """
    Reconstructs a flat list of 3D points back into continuous toolpath segments.

    OCL's PathDropCutter returns a single, continuous stream of points. This helper
    function intelligently groups those points back into discrete scan lines by detecting
    large "jumps" (rapids) where the tool lifted and moved to a new cutting area.

    Args:
        flat_points (list): A flat list of (x, y, z) tuples from the OCL engine.
        gap_threshold (float): The minimum distance between two points to be considered a "jump".
                               Typically set to a value slightly larger than the sample_interval.

    Returns:
        list: A nested list of scan lines, where each line is a list of continuous
              (x, y, z) tuples.
    """
    if not flat_points:
        return []

    lines = []
    current_line = [flat_points[0]]

    for i in range(1, len(flat_points)):
        # Calculate the 2D distance between the current and previous point
        dist = math.hypot(
            flat_points[i][0] - flat_points[i - 1][0], flat_points[i][1] - flat_points[i - 1][1]
        )

        # If the distance is greater than our threshold, it signifies a rapid move (a break in the path)
        if dist > gap_threshold:
            if len(current_line) >= 2:
                lines.append(current_line)
            current_line = []
        current_line.append(flat_points[i])

    if len(current_line) >= 2:
        lines.append(current_line)

    return lines


# ---------------------------------------------------------------------------
# Pattern Generators & C++ Bridge
# ---------------------------------------------------------------------------


def _extract_polygons_from_face(boundary_face, tolerance=0.005):
    """
    Converts the wires of a Part.Face into raw 2D point arrays for the C++ Ray-Caster.

    This function takes the mathematical boundaries computed by OpenCASCADE and discretizes
    them into a dense array of[x, y] coordinates. This prepares the boundary data in a format
    that can be instantly passed across the SWIG/PyBind boundary into C++ without heavy objects.

    Args:
        boundary_face (Part.Face): The 2D boundary mask generated by surface_common.
        tolerance (float): The LinearDeflection accuracy. A tighter tolerance creates
                           smoother polygons, guaranteeing precision when C++ snaps to the edge.

    Returns:
        list: A nested list of polygons in the format [[[x1, y1], [x2, y2], ...], ...]
    """
    polygons = []
    if not boundary_face:
        return polygons

    for wire in boundary_face.Wires:
        # Use high precision to ensure the C++ ray-caster sees a perfectly smooth curve
        pts = wire.discretize(Deflection=tolerance)
        poly = [[p.x, p.y] for p in pts]
        if len(poly) > 2:
            polygons.append(poly)

    return polygons


def fast_generate_pattern(
    pattern_type,
    bbox,
    center,
    stepover,
    sample_interval,
    angle,
    is_zigzag,
    reversed_pattern,
    climb,
    boundary_face,
    tolerance=0.005,
):
    """
    Bridges Python to the ultrafast C++ generation and clipping module.

    This acts as the master router for Line, ZigZag, Circular, and Spiral patterns. It
    extracts the boundaries, forwards all mathematical parameters to the compiled C++ engine,
    and returns perfectly clipped, high-resolution scan lines.

    Args:
        pattern_type (str): The requested pattern ("Line", "ZigZag", "Circular", "Spiral", etc.)
        bbox (BBox): The axis-aligned boundary limits of the operation.
        center (tuple): The (X, Y) origin point for radial patterns (Circular/Spiral).
        stepover (float): The distance between adjacent toolpaths in mm.
        sample_interval (float): The requested distance between points along a continuous path segment.
        angle (float): The rotation angle in degrees (used by Line/ZigZag).
        is_zigzag (bool): True if the tool should continuously alternate direction.
        reversed_pattern (bool): True if the toolpath order should be flipped (e.g., Outside-In).
        boundary_face (Part.Face): The 2D mask used to clip the toolpaths.
        tolerance (float): The mesh accuracy tolerance for polygon extraction.

    Returns:
        list: A nested list of successfully clipped and ordered scan lines, where each line
              is a list of (x, y, z) tuples.
    """
    if stepover <= 0.0:
        Path.Log.error(
            f"fast_generate_pattern: stepover must be positive, got {stepover}. "
            "Check the StepOver percentage and tool diameter."
        )
        return []

    polys = _extract_polygons_from_face(boundary_face, tolerance)

    if pattern_type in ("Line", "ZigZag"):
        # C++ now returns just the clipped endpoints for maximum OCL performance
        return _pattern_cpp.generate_linear_pattern_cpp(
            bbox.xmin,
            bbox.xmax,
            bbox.ymin,
            bbox.ymax,
            stepover,
            angle,
            is_zigzag,
            reversed_pattern,
            polys,
        )

    elif pattern_type in ("Circular", "CircularZigZag"):
        # C++ calculates the exact distance to the furthest corner dynamically
        return _pattern_cpp.generate_circular_pattern_cpp(
            bbox.xmin,
            bbox.xmax,
            bbox.ymin,
            bbox.ymax,
            center[0],
            center[1],
            stepover,
            sample_interval,
            is_zigzag,
            reversed_pattern,
            polys,
        )

    elif pattern_type == "Spiral":
        # C++ calculates the exact distance to the furthest corner dynamically
        return _pattern_cpp.generate_spiral_pattern_cpp(
            bbox.xmin,
            bbox.xmax,
            bbox.ymin,
            bbox.ymax,
            center[0],
            center[1],
            stepover,
            sample_interval,
            reversed_pattern,
            polys,
        )

    return []


# ---------------------------------------------------------------------------
# Offset Pattern Generator
# ---------------------------------------------------------------------------


def _reorient_wire_start(wire, start_point):
    """
    Rebuilds a closed wire so that its first edge begins at the vertex
    closest to the provided start_point.

    This is a travel-distance heuristic only (minimize rapid movement
    between successive offset rings) — unlike Waterline's loop-start
    search, there's no retract to avoid here, so the *exact* nearest
    point doesn't matter, only a reasonable approximation of it. The
    boundary face this wire comes from is always a flat 2D projection
    onto the XY plane (see create_boundary_face), so Z is constant and
    can be dropped from the comparison. We also compare squared distance
    directly instead of true distance, since we only need the arg-min,
    not the distance value itself — this skips a sqrt() per edge.
    """
    if not wire.isClosed():
        return wire

    edges = wire.Edges
    if not edges:
        return wire

    closest_idx = 0
    min_dist_sq = float("inf")
    sx, sy = start_point.x, start_point.y

    for i, edge in enumerate(edges):
        if not edge.Vertexes:
            continue

        v_start = edge.Vertexes[0].Point
        dx = v_start.x - sx
        dy = v_start.y - sy
        dist_sq = dx * dx + dy * dy

        if dist_sq < min_dist_sq:
            min_dist_sq = dist_sq
            closest_idx = i

    if closest_idx == 0:
        return wire

    reordered_edges = edges[closest_idx:] + edges[:closest_idx]

    try:
        new_wire = Part.Wire(reordered_edges)
        return new_wire
    except Exception as e:
        Path.Log.debug(f"surface_pattern._reorient_wire_start: Failed to reorient wire: {e}")
        return wire


def _collect_offset_levels(face, stepover, tool_diam):
    """
    Steps a Path.Area offset inward over `face` until it collapses, returning
    one list of (wire, xy_center) per step. Raw collection only — no
    discretizing, no ordering.
    """
    offset_engine = Path.Area()
    offset_engine.setParams(Tolerance=0.01)
    offset_engine.add(face)

    min_path_length = tool_diam
    current_offset = -0.005
    levels = []

    while True:
        offset_engine.setParams(Offset=current_offset)
        try:
            offset_shape = offset_engine.getShape()
        except Exception as e:
            Path.Log.debug(f"generate_offset_scan_lines: Offset layer failed: {e}.")
            break

        if not offset_shape or offset_shape.isNull() or len(offset_shape.Wires) == 0:
            break

        level_wires = []
        for wire in offset_shape.Wires:
            if wire.Length < min_path_length:
                continue
            bb = wire.BoundBox
            level_wires.append((wire, FreeCAD.Vector(bb.Center.x, bb.Center.y, 0.0)))

        if not level_wires:
            break

        levels.append(level_wires)
        current_offset -= stepover

    return levels


def _chain_wires_into_zones(levels, sample_interval, stepover):
    """
    Groups wires across offset levels into zones (e.g. the outer boundary's
    shrinking sequence, or one hole's growing sequence) by nearest XY-center
    proximity. Heuristic, not exact topology tracking — Path.Area doesn't
    expose wire lineage through splits/merges.
    """
    max_chain_gap = max(sample_interval, stepover) * 4.0

    chains = [[w] for w in levels[0]]
    chain_last_level = [0] * len(chains)

    for level_idx in range(1, len(levels)):
        unmatched = list(levels[level_idx])
        for chain_pos, chain in enumerate(chains):
            if chain_last_level[chain_pos] != level_idx - 1 or not unmatched:
                continue
            _, last_center = chain[-1]
            nearest = min(unmatched, key=lambda w: (w[1] - last_center).Length)
            if (nearest[1] - last_center).Length <= max_chain_gap:
                chain.append(nearest)
                chain_last_level[chain_pos] = level_idx
                unmatched.remove(nearest)
        for w in unmatched:
            chains.append([w])
            chain_last_level.append(level_idx)

    return chains


def _emit_zones_nearest_neighbor(chains, sample_interval, climb, current_start_pt):
    """
    Visits zones nearest-neighbor from the tool's current position, emitting
    each zone's rings fully (outer-most to inner-most) before moving to the
    next. Returns (line_points list, new current_start_pt).
    """
    region_lines = []
    remaining = list(chains)

    while remaining:
        if current_start_pt is None:
            # No established position yet — default to the outer boundary's
            # zone (largest bounding box at level 0).
            chain = max(remaining, key=lambda c: c[0][0].BoundBox.DiagonalLength)
        else:
            chain = min(remaining, key=lambda c: (c[0][1] - current_start_pt).Length)
        remaining.remove(chain)

        for wire, _center in chain:
            if current_start_pt and wire.isClosed() and wire.BoundBox.DiagonalLength > 2.0:
                wire = _reorient_wire_start(wire, current_start_pt)

            pts = wire.discretize(Distance=sample_interval)
            if len(pts) < 2:
                continue
            if not climb:
                pts.reverse()
            if wire.isClosed() and (pts[0] - pts[-1]).Length > 1e-5:
                pts.append(pts[0])

            current_start_pt = FreeCAD.Vector(pts[-1].x, pts[-1].y, 0.0)
            region_lines.append([(p.x, p.y, 0.0) for p in pts])

    return region_lines, current_start_pt


def _offset_rings_for_region(
    face, stepover, tool_diam, sample_interval, climb, current_start_pt=None
):
    """
    Generates concentric offset rings for one connected region (outer
    boundary + any holes), keeping each hole's rings and the outer
    boundary's rings grouped into their own zones instead of interleaving
    them at every offset step. See _collect_offset_levels,
    _chain_wires_into_zones, and _emit_zones_nearest_neighbor for the three
    stages.

    Returns:
        tuple: (list of line_points lists for this region, the new current_start_pt)
    """
    levels = _collect_offset_levels(face, stepover, tool_diam)
    if not levels:
        return [], current_start_pt

    chains = _chain_wires_into_zones(levels, sample_interval, stepover)
    return _emit_zones_nearest_neighbor(chains, sample_interval, climb, current_start_pt)


def generate_offset_scan_lines(
    boundary_face, stepover, tool_diam, sample_interval, reversed_pattern=False, climb=False
):
    """
    Generates concentric toolpath rings that progressively shrink inwards from a boundary,
    using Path.Area() to repeatedly collapse the boundary geometry by the stepover amount.

    Pipeline: split boundary_face into disjoint regions -> visit regions nearest-neighbor,
    each fully cleared via _offset_rings_for_region (collect levels -> chain into zones ->
    emit zones nearest-neighbor) -> optionally reverse the whole sequence.

    Args:
        boundary_face (Part.Face): The outermost boundary mask to shrink.
        stepover (float): The radial distance to shrink the geometry for each subsequent pass.
        tool_diam (float): The diameter of the tool.
        sample_interval (float): The distance between points along the resulting rings.
        reversed_pattern (bool): If True, cuts from the inside out (reverses the ring order).

    Returns:
        list: A nested list of scan lines, where each line is a list of (x, y, z) tuples
              forming an offset ring.
    """
    if boundary_face is None or boundary_face.isNull():
        return []

    if stepover <= 0.0:
        Path.Log.error(
            f"generate_offset_scan_lines: stepover must be positive, got {stepover}. "
            "Check the StepOver percentage and tool diameter."
        )
        return []

    if hasattr(boundary_face, "removeSplitter"):
        try:
            cleaned_face = boundary_face.removeSplitter()
            if cleaned_face and not cleaned_face.isNull():
                boundary_face = cleaned_face
        except Exception as e:
            Path.Log.debug(f"generate_offset_scan_lines: removeSplitter ignored: {e}")

    # .Faces splits any disjoint regions apart; for the common single-region
    # case it just returns [boundary_face] unchanged.
    regions = list(boundary_face.Faces)

    if len(regions) > 1:
        Path.Log.debug(
            f"generate_offset_scan_lines: boundary is {len(regions)} disjoint region(s); "
            "clearing each fully before moving to the next."
        )
        regions.sort(key=lambda f: f.BoundBox.DiagonalLength, reverse=True)

    offset_lines = []
    current_start_pt = None

    while regions:
        if current_start_pt is None:
            region = regions.pop(0)
        else:
            # Nearest remaining region to the tool's last position; the exact
            # entry point is refined afterwards by _reorient_wire_start.
            region = min(regions, key=lambda f: (f.BoundBox.Center - current_start_pt).Length)
            regions.remove(region)

        region_lines, current_start_pt = _offset_rings_for_region(
            region, stepover, tool_diam, sample_interval, climb, current_start_pt
        )
        offset_lines.extend(region_lines)

    if reversed_pattern:
        offset_lines.reverse()

    return offset_lines
