# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *   Copyright (c) 2025 sliptonic <shopinthewoods@gmail.com>               *
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

"""Scan pattern generation for 3D surface operations.

Acts as a high-performance bridge to the C++ surface_generator module.
Passes bounding boxes, centers, and high-resolution boundary polygons
to C++ for instant pattern generation and exact binary-search clipping.
"""

import math
import Path

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


def generate_offset_scan_lines(boundary_face, stepover, sample_interval, reversed_pattern=False, climb=False):
    """
    Generates concentric toolpath rings that progressively shrink inwards from a boundary.

    Unlike standard geometric patterns (which use C++), Offset patterns natively rely on
    the shape of the boundary itself. This function uses the OpenCASCADE/ClipperLib engine
    via PathUtils to repeatedly collapse the boundary geometry inward by the stepover amount.

    Args:
        boundary_face (Part.Face): The outermost boundary mask to shrink.
        stepover (float): The radial distance to shrink the geometry for each subsequent pass.
        sample_interval (float): The distance between points along the resulting rings.
        reversed_pattern (bool): If True, cuts from the inside out (reverses the ring order).

    Returns:
        list: A nested list of scan lines, where each line is a list of (x, y, z) tuples
              forming an offset ring.
    """
    import PathScripts.PathUtils as PathUtils

    if boundary_face is None or boundary_face.isNull():
        return []

    offset_lines = []
    current_offset = 0.0

    while True:
        # Using a negative offset mathematically shrinks the geometry inwards
        offset_shape = PathUtils.getOffsetArea(
            boundary_face, current_offset, removeHoles=False, tolerance=0.01, plane=boundary_face
        )

        # If the shape collapses entirely or errors out, we've reached the absolute center
        if not offset_shape or offset_shape.isNull() or len(offset_shape.Wires) == 0:
            break

        layer_lines = []
        for wire in offset_shape.Wires:
            # Discretize the wire into a smooth array of coordinates
            pts = wire.discretize(Distance=sample_interval)
            if len(pts) < 2:
                continue
            if not climb:
                pts.reverse()

            # Ensure perfectly closed loops by connecting the final point back to the start
            if wire.isClosed() and (pts[0] - pts[-1]).Length > 1e-5:
                pts.append(pts[0])

            line_points = [(p.x, p.y, 0.0) for p in pts]
            layer_lines.append(line_points)

        if not layer_lines:
            break

        offset_lines.extend(layer_lines)
        current_offset -= stepover

    if reversed_pattern:
        offset_lines.reverse()

    return offset_lines


def extract_polygons_from_face(boundary_face, tolerance=0.005):
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
    if not boundary_face or boundary_face.isNull():
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

    polys = extract_polygons_from_face(boundary_face, tolerance)

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
