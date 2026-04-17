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

Generates 2D scan paths (line, zigzag, circular, spiral) as sequences of
(x, y) points or arc definitions.  These are pure-geometry functions that
work with bounding-box parameters — no FreeCAD document access.

The scan paths are later fed to OCL drop-cutter algorithms which project
them onto the 3D model to produce cutter-location points.
"""

import math
import Path

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
        return math.sqrt(self.x_length**2 + self.y_length**2)


# ---------------------------------------------------------------------------
# Line scan
# ---------------------------------------------------------------------------


def generate_line_scan(bbox, step_over, tool_diameter, angle=0.0, reversed=False):
    """Generate parallel line scan paths over a bounding box.

    Lines are spaced by *step_over* and oriented along the X axis by
    default.  An optional *angle* (degrees) rotates the entire pattern
    around the bounding-box center.

    Args:
        bbox: :class:`BBox` or FreeCAD BoundBox-like object.
        step_over: Distance between adjacent scan lines (mm).
        tool_diameter: Tool diameter — used to extend lines beyond the
                       bounding box so the full cutter sweeps the edges.
        angle: Rotation angle in degrees (0 = lines along X).
        reversed: If True, reverse the order of scan lines.

    Returns:
        List of scan lines.  Each scan line is a list of two ``(x, y)``
        tuples representing the line endpoints.
    """
    if not isinstance(bbox, BBox):
        bbox = BBox.from_bbox(bbox)

    if step_over <= 0:
        raise ValueError("step_over must be positive, got {}".format(step_over))

    diag = bbox.diagonal
    line_len = diag + 2.0 * tool_diameter
    half_diag = math.ceil(line_len / 2.0)
    num_passes = int(math.ceil(line_len / step_over)) + 1
    half_passes = int(math.ceil(num_passes / 2.0))

    cx, cy = bbox.center
    cos_a = math.cos(math.radians(angle))
    sin_a = math.sin(math.radians(angle))

    lines = []
    for i in range(-half_passes + 1, half_passes + 1):
        # Unrotated line at y-offset
        y_off = i * step_over
        x1, y1 = -half_diag, y_off
        x2, y2 = half_diag, y_off

        # Rotate around origin then translate to bbox center
        p1 = (
            cx + x1 * cos_a - y1 * sin_a,
            cy + x1 * sin_a + y1 * cos_a,
        )
        p2 = (
            cx + x2 * cos_a - y2 * sin_a,
            cy + x2 * sin_a + y2 * cos_a,
        )
        lines.append([p1, p2])

    if reversed:
        lines.reverse()

    return lines


# ---------------------------------------------------------------------------
# Zigzag scan
# ---------------------------------------------------------------------------


def generate_zigzag_scan(bbox, step_over, tool_diameter, angle=0.0, reversed=False):
    """Generate zigzag scan paths (alternating direction).

    Same geometry as :func:`generate_line_scan` but alternating lines
    have their endpoints swapped so the cutter travels in a continuous
    back-and-forth pattern.

    Returns:
        List of scan lines, each a list of two ``(x, y)`` tuples.
    """
    lines = generate_line_scan(bbox, step_over, tool_diameter, angle=angle, reversed=reversed)

    for i, line in enumerate(lines):
        if i % 2 == 1:
            line.reverse()

    return lines


# ---------------------------------------------------------------------------
# Circular scan
# ---------------------------------------------------------------------------


def generate_circular_scan(
    center,
    max_radius,
    step_over,
    tool_diameter,
    min_radius=None,
    reversed=False,
):
    """Generate concentric circular scan paths.

    Each circle is returned as a dict with ``center``, ``radius``, and
    ``direction`` (+1 = CCW, -1 = CW).

    Args:
        center: ``(x, y)`` center of the circular pattern.
        max_radius: Maximum radius to cover (typically half the bounding
                    box diagonal plus tool radius).
        step_over: Radial distance between concentric circles.
        tool_diameter: Tool diameter for minimum-radius calculation.
        min_radius: Override minimum radius.  If *None*, derived from
                    tool diameter.
        reversed: If True, scan from outside in.

    Returns:
        List of dicts, each with keys ``center``, ``radius``,
        ``direction``.
    """
    if step_over <= 0:
        raise ValueError("step_over must be positive")

    if min_radius is None:
        min_radius = tool_diameter * 0.45

    circles = []
    num_passes = int(math.ceil(max_radius / step_over)) + 1

    for i in range(1, num_passes + 1):
        r = i * step_over
        if r >= min_radius:
            circles.append(
                {
                    "center": center,
                    "radius": r,
                    "direction": 1,  # CCW
                }
            )

    if reversed:
        circles.reverse()

    return circles


def generate_circular_zigzag_scan(
    center,
    max_radius,
    step_over,
    tool_diameter,
    min_radius=None,
    reversed=False,
):
    """Generate concentric circular scan paths with alternating direction.

    Same as :func:`generate_circular_scan` but alternating circles have
    opposite winding direction (CCW / CW).
    """
    circles = generate_circular_scan(
        center,
        max_radius,
        step_over,
        tool_diameter,
        min_radius=min_radius,
        reversed=reversed,
    )
    for i, c in enumerate(circles):
        c["direction"] = 1 if i % 2 == 0 else -1

    return circles


# ---------------------------------------------------------------------------
# Spiral scan
# ---------------------------------------------------------------------------


def generate_spiral_scan(
    center,
    max_radius,
    step_over,
    sample_interval,
    reversed=False,
):
    """Generate an Archimedean spiral scan path from center outward.

    The spiral is returned as a list of ``(x, y)`` points sampled at
    approximately *sample_interval* arc-length spacing.

    Args:
        center: ``(x, y)`` center of the spiral.
        max_radius: Maximum radius to cover.
        step_over: Radial distance per full revolution.
        sample_interval: Approximate arc-length between sample points.
        reversed: If True, spiral from outside in.

    Returns:
        List of ``(x, y)`` tuples forming the spiral path.
    """
    if step_over <= 0:
        raise ValueError("step_over must be positive")
    if sample_interval <= 0:
        raise ValueError("sample_interval must be positive")

    two_pi = 2.0 * math.pi
    # Archimedean spiral: r = b * theta, where b = step_over / (2*pi)
    b = step_over / two_pi
    stop_radians = max_radius / b if b > 0 else 0

    cx, cy = center
    points = []
    theta = 0.0
    loop_count = 0
    loop_radians = 0.0

    while theta <= stop_radians:
        r = b * theta
        x = cx + r * math.cos(theta)
        y = cy + r * math.sin(theta)
        points.append((x, y))

        # Adaptive step angle: smaller steps at larger radii
        current_radius = max((loop_count + 1) * step_over, step_over)
        step_angle = sample_interval / current_radius
        theta += step_angle
        loop_radians += step_angle

        if loop_radians > two_pi:
            loop_count += 1
            loop_radians -= two_pi

    if reversed:
        points.reverse()

    return points


# ---------------------------------------------------------------------------
# Grid point generation for BatchDropCutter
# ---------------------------------------------------------------------------


def generate_grid_points(bbox, step_over_x, step_over_y, min_z=0.0):
    """Generate a regular grid of (x, y, z) points for BatchDropCutter.

    Unlike the line-based scans, this produces individual points suitable
    for OCL's ``BatchDropCutter`` which processes them all in one
    optimized batch with KDTree acceleration.

    Args:
        bbox: :class:`BBox` or FreeCAD BoundBox-like object.
        step_over_x: Grid spacing in X direction (mm).
        step_over_y: Grid spacing in Y direction (mm).
        min_z: Initial Z value for all points (will be updated by
               drop-cutter).

    Returns:
        List of ``(x, y, z)`` tuples and the grid dimensions as
        ``(nx, ny)`` tuple.
    """
    if not isinstance(bbox, BBox):
        bbox = BBox.from_bbox(bbox)

    nx = int(math.ceil(bbox.x_length / step_over_x)) + 1
    ny = int(math.ceil(bbox.y_length / step_over_y)) + 1

    points = []
    for j in range(ny):
        y = bbox.ymin + j * step_over_y
        if y > bbox.ymax:
            y = bbox.ymax
        for i in range(nx):
            x = bbox.xmin + i * step_over_x
            if x > bbox.xmax:
                x = bbox.xmax
            points.append((x, y, min_z))

    return points, (nx, ny)


# ---------------------------------------------------------------------------
# OCL Path construction helpers
# ---------------------------------------------------------------------------


def lines_to_ocl_path(scan_lines, min_z):
    """Convert scan lines to an ``ocl.Path`` of ``ocl.Line`` segments.

    Each scan line is a list of ``(x, y)`` endpoint pairs.  This creates
    one ``ocl.Line`` per scan line.

    Args:
        scan_lines: Output from :func:`generate_line_scan` or similar.
        min_z: Z value for all path points.

    Returns:
        An ``ocl.Path`` object.
    """
    from Path.Base.Generator.surface_common import _get_ocl

    ocl = _get_ocl()
    path = ocl.Path()

    for line in scan_lines:
        p1 = ocl.Point(line[0][0], line[0][1], min_z)
        p2 = ocl.Point(line[1][0], line[1][1], min_z)
        path.append(ocl.Line(p1, p2))

    return path


def circles_to_ocl_path(circles, min_z):
    """Convert circular scan data to an ``ocl.Path`` of ``ocl.Arc`` segments.

    Each circle dict must have ``center``, ``radius``, and ``direction``.

    Args:
        circles: Output from :func:`generate_circular_scan`.
        min_z: Z value for all path points.

    Returns:
        An ``ocl.Path`` object.
    """
    from Path.Base.Generator.surface_common import _get_ocl

    ocl = _get_ocl()
    path = ocl.Path()

    for c in circles:
        cx, cy = c["center"]
        r = c["radius"]
        d = c["direction"]  # +1 CCW, -1 CW

        # Create a near-full-circle arc (small gap to avoid degenerate arc)
        gap_angle = 0.005  # radians, ~0.3 degrees
        sp = ocl.Point(cx + r, cy, min_z)
        ep = ocl.Point(
            cx + r * math.cos(gap_angle),
            cy - r * math.sin(gap_angle),
            min_z,
        )
        cp = ocl.Point(cx, cy, min_z)

        if d > 0:
            arc = ocl.Arc(sp, ep, cp, True)  # CCW
        else:
            arc = ocl.Arc(ep, sp, cp, False)  # CW
        path.append(arc)

    return path
