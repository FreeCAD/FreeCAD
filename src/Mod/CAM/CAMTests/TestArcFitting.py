# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 David kaufman <davidgilkaufman@gmail.com>
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
Unit tests for clipper arc fitting/unfitting operations.
"""

import unittest
import area
import math
from CAMTests.PathTestUtils import PathTestBase


def make_curve(vertices):
    """Helper: Create a curve from a list of vertex specs.

    Each vertex can be:
    - (x, y) for a line vertex (type 0)
    - (x, y, type, cx, cy) for an arc vertex (type 1=CCW, -1=CW)
    """
    c = area.Curve()
    for spec in vertices:
        if len(spec) == 2:
            # Line vertex
            x, y = spec
            c.append(area.Vertex(area.Point(x, y)))
        elif len(spec) == 5:
            # Arc vertex
            x, y, vtype, cx, cy = spec
            if vtype not in (1, -1):
                raise ValueError(f"Arc vertex type must be 1 (CCW) or -1 (CW), got {vtype}")
            c.append(area.Vertex(vtype, area.Point(x, y), area.Point(cx, cy)))
        else:
            raise ValueError(f"Invalid vertex spec: {spec}")
    return c


def make_regular_polygon(num_sides, radius, subdivisions=1):
    """Create a regular polygon with line segments.

    Args:
        num_sides: Number of sides
        radius: Radius of circumscribed circle
        subdivisions: Number of segments to split each side into (1 = no subdivision)
    """
    vertices = []
    angle_step = 2 * math.pi / num_sides

    # Generate vertices around a circle
    for i in range(num_sides):
        angle_start = i * angle_step
        angle_end = (i + 1) * angle_step

        # Interpolate along this edge
        for j in range(subdivisions):
            t = j / subdivisions
            angle = angle_start + t * (angle_end - angle_start)
            x = radius * math.cos(angle)
            y = radius * math.sin(angle)
            vertices.append((x, y))

    # Close the curve - add first point again
    start_x = radius * math.cos(0)
    start_y = radius * math.sin(0)
    vertices.append((start_x, start_y))

    return make_curve(vertices)


def make_area(curves):
    """Helper: Create an Area from a list of curves.

    Args:
        curves: Single curve or list of curves to add to the area
    """
    a = area.Area()
    if isinstance(curves, list):
        for c in curves:
            a.append(c)
    else:
        a.append(curves)
    return a


def make_mirrored_arcs(x, y):
    """Upper arc and lower arc centered on y-axis at +-y, hitting the x-axis at x"""
    return make_area(
        [
            make_curve(
                [
                    (x, 0),  # Start at right intersection point
                    (-x, 0, 1, 0, y),  # Arc to left intersection (through top)
                    (x, 0, 1, 0, -y),  # Arc back to start (through bottom)
                ]
            )
        ]
    )


def rotate_curve(curve, start_i):
    """Rotate a (closed) curve to start at a different vertex index.

    Args:
        curve: Curve object to rotate
        start_i: Index of the vertex to become the new start (must be >= 1)

    Returns:
        New rotated Curve object
    """
    vertices = list(curve.getVertices())

    if start_i == 0 or len(vertices) <= 1:
        return curve

    # Create new curve starting at start_i
    new_curve = area.Curve()
    new_curve.append(area.Vertex(area.Point(vertices[start_i].p.x, vertices[start_i].p.y)))

    # Add remaining vertices in rotated order
    indices = list(range(start_i + 1, len(vertices))) + list(range(1, start_i + 1))
    for i in indices:
        v = vertices[i % len(vertices)]
        if v.type == 0:
            new_curve.append(area.Vertex(area.Point(v.p.x, v.p.y)))
        else:
            new_curve.append(
                area.Vertex(v.type, area.Point(v.p.x, v.p.y), area.Point(v.c.x, v.c.y))
            )

    return new_curve


def rotate_curve_in_area(a, curve_i, start_i):
    """Create a new area with one curve rotated to start at a different vertex.

    Args:
        a: Area object
        curve_i: Index of the curve to rotate
        start_i: Index of the vertex to become the new start (must be >= 1)

    Returns:
        New Area object with the specified curve rotated
    """
    result = area.Area()

    for i, c in enumerate(a.getCurves()):
        if i == curve_i:
            result.append(rotate_curve(c, start_i))
        else:
            result.append(c)

    return result


def canonicalize_area(a):
    """Sort area contents canonically.

    For each curve, rotate to start at the vertex with lowest y (tie break lowest x).
    Sort curves by their (sorted) first point using the same criteria.

    Args:
        a: Area object to canonicalize

    Returns:
        New canonicalized Area object
    """
    canonicalized_curves = []

    for curve in a.getCurves():
        vertices = list(curve.getVertices())
        if len(vertices) <= 1:
            # Empty or single-vertex curve, nothing to rotate
            canonicalized_curves.append(curve)
            continue

        # Find the canonical start index (lowest y, then lowest x)
        # Start from index 0 (the starting point) since all positions are valid starts
        start_i = 0
        min_y = vertices[0].p.y
        min_x = vertices[0].p.x

        for i, v in enumerate(vertices):
            if v.p.y < min_y or (v.p.y == min_y and v.p.x < min_x):
                start_i = i
                min_y = v.p.y
                min_x = v.p.x

        # Rotate the curve to start at start_i
        canonicalized_curves.append(rotate_curve(curve, start_i))

    # Sort curves by their first point (lowest y, then lowest x)
    canonicalized_curves.sort(key=lambda c: (c.getVertices()[0].p.y, c.getVertices()[0].p.x))

    # Create new area with sorted curves
    result = area.Area()
    for c in canonicalized_curves:
        result.append(c)

    return result


def areas_equal(a1, a2):
    """Compare if two areas are exactly equal, including curve order.

    Note: You may want to canonicalize areas before using this function.

    Args:
        a1: First area to compare
        a2: Second area to compare

    Returns:
        True if areas are equal, False otherwise
    """
    # Compare number of curves
    curves1 = list(a1.getCurves())
    curves2 = list(a2.getCurves())

    if len(curves1) != len(curves2):
        return False

    # Compare each curve
    for curve1, curve2 in zip(curves1, curves2):
        vertices1 = list(curve1.getVertices())
        vertices2 = list(curve2.getVertices())

        if len(vertices1) != len(vertices2):
            return False

        # Compare each vertex
        for v1, v2 in zip(vertices1, vertices2):
            # Compare type
            if v1.type != v2.type:
                return False

            # Compare position (exact)
            if v1.p.x != v2.p.x or v1.p.y != v2.p.y:
                return False

            # Compare center for arcs (type != 0)
            if v1.type != 0:
                if v1.c.x != v2.c.x or v1.c.y != v2.c.y:
                    return False

    return True


class TestArcFittingRoundTrip(PathTestBase):
    """Tests for round-trip conversions to clipper and back (ClipperNoop)."""

    def assert_area_unchanged_by_roundtrip(self, a):
        """Helper: Assert that ClipperNoop (arc to lines and restore) doesn't change the Area."""
        # Store original vertices from all curves
        orig = []
        for curve in a.getCurves():
            orig.extend(list(curve.getVertices()))

        a.ClipperNoop()

        # Extract result vertices from all curves
        result = []
        for curve in a.getCurves():
            result.extend(list(curve.getVertices()))

        # Helper to format vertex for display
        def fmt_vertex(v):
            return (
                f"type={v.type:2d}, p=({v.p.x:6.2f}, {v.p.y:6.2f}), c=({v.c.x:6.2f}, {v.c.y:6.2f})"
            )

        # Compare exhaustively
        if len(orig) != len(result):
            orig_str = "\n  ".join([fmt_vertex(v) for v in orig])
            result_str = "\n  ".join([fmt_vertex(v) for v in result])
            self.fail(
                f"Vertex count mismatch: {len(orig)} -> {len(result)}\n"
                f"Original vertices:\n  {orig_str}\n"
                f"Result vertices:\n  {result_str}"
            )

        for vert_idx, (orig_v, result_v) in enumerate(zip(orig, result)):
            mismatch = None

            # Check type exactly
            if orig_v.type != result_v.type:
                mismatch = "type mismatch"

            # Check points and centers with tolerance
            if not mismatch:
                try:
                    self.assertAlmostEqual(orig_v.p.x, result_v.p.x, places=3)
                    self.assertAlmostEqual(orig_v.p.y, result_v.p.y, places=3)
                    self.assertAlmostEqual(orig_v.c.x, result_v.c.x, places=3)
                    self.assertAlmostEqual(orig_v.c.y, result_v.c.y, places=3)
                except AssertionError:
                    mismatch = "coordinate mismatch"

            if mismatch:
                orig_str = "\n  ".join([fmt_vertex(v) for v in orig])
                result_str = "\n  ".join([fmt_vertex(v) for v in result])
                self.fail(
                    f"Vertex {vert_idx} {mismatch}:\n"
                    f"  Original: {fmt_vertex(orig_v)}\n"
                    f"  Result:   {fmt_vertex(result_v)}\n"
                    f"Full original curve:\n  {orig_str}\n"
                    f"Full result curve:\n  {result_str}"
                )

    def test_empty_roundtrip(self):
        """Test that round-trip preserves an empty curve."""
        c = area.Curve()
        self.assert_area_unchanged_by_roundtrip(make_area(c))

    def test_single_vertex_roundtrip(self):
        """Test that round-trip preserves a single-vertex curve."""
        c = make_curve([(1, 1)])
        self.assert_area_unchanged_by_roundtrip(make_area(c))

    def test_line_roundtrip(self):
        """Test that round-trip preserves a simple line."""
        c = make_curve([(0, 0), (10, 10)])
        self.assert_area_unchanged_by_roundtrip(make_area(c))

    def test_small_arc_ccw_roundtrip(self):
        """Test that round-trip preserves a 90-degree CCW arc."""
        c = make_curve([(10, 0), (0, 10, 1, 0, 0)])
        self.assert_area_unchanged_by_roundtrip(make_area(c))

    def test_small_arc_cw_roundtrip(self):
        """Test that round-trip preserves a 90-degree CW arc."""
        c = make_curve([(10, 0), (0, -10, -1, 0, 0)])
        self.assert_area_unchanged_by_roundtrip(make_area(c))

    def test_180_arc_ccw_roundtrip(self):
        """Test that round-trip preserves a 180-degree CCW arc."""
        c = make_curve([(10, 0), (-10, 0, 1, 0, 0)])
        self.assert_area_unchanged_by_roundtrip(make_area(c))

    def test_180_arc_cw_roundtrip(self):
        """Test that round-trip preserves a 180-degree CW arc."""
        c = make_curve([(-10, 0), (10, 0, -1, 0, 0)])
        self.assert_area_unchanged_by_roundtrip(make_area(c))

    def test_closed_line_before_arc_ccw_roundtrip(self):
        """Test closed curve: line then CCW arc back to start."""
        c = make_curve([(0, 0), (10, 0), (0, 0, 1, 5, 0)])
        self.assert_area_unchanged_by_roundtrip(make_area(c))

    def test_closed_line_before_arc_cw_roundtrip(self):
        """Test closed curve: line then CW arc back to start."""
        c = make_curve([(0, 0), (10, 0), (0, 0, -1, 5, 0)])
        self.assert_area_unchanged_by_roundtrip(make_area(c))

    def test_closed_line_after_arc_ccw_roundtrip(self):
        """Test closed curve: CCW arc then line back to start."""
        c = make_curve([(0, 0), (10, 0, 1, 5, 0), (0, 0)])
        self.assert_area_unchanged_by_roundtrip(make_area(c))

    def test_closed_line_after_arc_cw_roundtrip(self):
        """Test closed curve: CW arc then line back to start."""
        c = make_curve([(0, 0), (10, 0, -1, 5, 0), (0, 0)])
        self.assert_area_unchanged_by_roundtrip(make_area(c))

    def test_subdivided_polygons_roundtrip(self):
        """Exploratory test: sweep parameters on regular polygon side count and side subdivision count."""
        failures = []
        results = {}  # (num_sides, subdivisions) -> pass/fail
        radius = 1

        sides_list = [3, 4, 5, 6, 8, 12, 16]
        subdiv_list = [1, 2, 3, 4, 5]

        # Test different polygon configurations
        for num_sides in sides_list:
            for subdivisions in subdiv_list:
                test_name = f"{num_sides}-gon, subdiv={subdivisions}"

                try:
                    c = make_regular_polygon(num_sides, radius, subdivisions)
                    self.assert_area_unchanged_by_roundtrip(make_area(c))
                    results[(num_sides, subdivisions)] = "PASS"
                except AssertionError as e:
                    results[(num_sides, subdivisions)] = "FAIL"
                    failures.append((test_name, str(e)))

        # Create summary table
        header = "Summary (- = pass, X = fail):\n"
        header += "Sides \\ Subdiv | " + " | ".join(f"{s}" for s in subdiv_list) + " |\n"
        header += "-" * (17 + 4 * len(subdiv_list) - 1) + "\n"

        table_rows = []
        for num_sides in sides_list:
            row = f"{num_sides:5d}          | "
            symbols = []
            for s in subdiv_list:
                symbol = "-" if results[(num_sides, s)] == "PASS" else "X"
                symbols.append(symbol)
            row += " | ".join(symbols)
            row += " |"
            table_rows.append(row)

        summary_table = header + "\n".join(table_rows)

        # Report all failures
        if failures:
            failure_report = "\n\n".join([f"{name}:\n{error}" for name, error in failures])
            self.fail(
                f"Found {len(failures)} failing configurations:\n\n"
                f"{failure_report}\n\n{summary_table}"
            )


class TestArcFittingOffsets(PathTestBase):
    """Tests for arc fitting with offset operations."""

    def assert_offset_line_and_arc_count(
        self,
        area_orig,
        offset_distance,
        expected_curves,
        expected_lines,
        expected_ccw_arcs,
        expected_cw_arcs=0,
    ):
        """Helper: Assert that offsetting an area produces expected line and arc counts.

        Args:
            area_orig: Area object to offset
            offset_distance: Distance to offset (positive = outward for CCW curves)
            expected_curves: Expected number of curves after offset
            expected_lines: Expected total number of line segments across all curves
            expected_ccw_arcs: Expected total number of CCW arc segments (type=1) across all curves
            expected_cw_arcs: Expected total number of CW arc segments (type=-1) across all curves (default: 0)
        """
        # Make a copy to preserve the original
        a = area.copy_area(area_orig)

        # Store original curves for debug output
        orig_curves = a.getCurves()
        orig_summary = []
        for i, curve in enumerate(orig_curves):
            vertices = list(curve.getVertices())
            orig_summary.append(f"  Curve {i}: {len(vertices)} vertices")
            for v in vertices:
                orig_summary.append(
                    f"    type={v.type:2d}, p=({v.p.x:6.2f}, {v.p.y:6.2f}), c=({v.c.x:6.2f}, {v.c.y:6.2f})"
                )

        a.Offset(offset_distance)

        # Get the resulting curves
        curves = a.getCurves()

        # Build detailed output for debugging
        result_summary = []
        total_lines = 0
        total_ccw_arcs = 0
        total_cw_arcs = 0

        for i, curve in enumerate(curves):
            vertices = list(curve.getVertices())
            lines = [v for v in vertices[1:] if v.type == 0]
            ccw_arcs = [v for v in vertices[1:] if v.type == 1]
            cw_arcs = [v for v in vertices[1:] if v.type == -1]
            total_lines += len(lines)
            total_ccw_arcs += len(ccw_arcs)
            total_cw_arcs += len(cw_arcs)

            result_summary.append(
                f"  Curve {i}: {len(vertices)} vertices ({len(lines)} lines, {len(ccw_arcs)} CCW arcs, {len(cw_arcs)} CW arcs)"
            )
            for v in vertices:
                result_summary.append(
                    f"    type={v.type:2d}, p=({v.p.x:6.2f}, {v.p.y:6.2f}), c=({v.c.x:6.2f}, {v.c.y:6.2f})"
                )

        # Check curve count
        if len(curves) != expected_curves:
            self.fail(
                f"Expected {expected_curves} curves after offset, got {len(curves)}\n"
                f"Offset distance: {offset_distance}\n"
                f"Original curves:\n" + "\n".join(orig_summary) + "\n"
                f"Result curves:\n" + "\n".join(result_summary)
            )

        # Check line and arc counts
        if (
            total_lines != expected_lines
            or total_ccw_arcs != expected_ccw_arcs
            or total_cw_arcs != expected_cw_arcs
        ):
            self.fail(
                f"Expected {expected_lines} lines, {expected_ccw_arcs} CCW arcs, and {expected_cw_arcs} CW arcs, "
                f"got {total_lines} lines, {total_ccw_arcs} CCW arcs, and {total_cw_arcs} CW arcs\n"
                f"Offset distance: {offset_distance}\n"
                f"Original curves:\n" + "\n".join(orig_summary) + "\n"
                f"Result curves:\n" + "\n".join(result_summary)
            )

        # Test rotation invariance: result should be the same regardless of input curve starting position
        result_orig_canon = canonicalize_area(a)
        for curve_i, orig_curve in enumerate(area_orig.getCurves()):
            for start_i in range(1, len(orig_curve.getVertices())):
                rotated_area = rotate_curve_in_area(area_orig, curve_i, start_i)
                rotated_area.Offset(offset_distance)
                rotated_result_canon = canonicalize_area(rotated_area)

                self.assertTrue(
                    areas_equal(rotated_result_canon, result_orig_canon),
                    f"Offset result differs when curve {curve_i} starts at index {start_i} instead of 0",
                )

    def test_square_offset_outward(self):
        """Test that offsetting a square outward produces 4 lines and 4 arcs."""
        a = make_area(make_curve([(0, 0), (10, 0), (10, 10), (0, 10), (0, 0)]))
        self.assert_offset_line_and_arc_count(a, 1.0, 1, 4, 4)  # 1 curve, 4 lines, 4 CCW arcs

    def test_square_offset_inward(self):
        """Test that offsetting a square inward produces 4 lines and no arcs."""
        a = make_area(make_curve([(0, 0), (10, 0), (10, 10), (0, 10), (0, 0)]))
        self.assert_offset_line_and_arc_count(a, -1.0, 1, 4, 0)  # 1 curve, 4 lines, 0 CCW arcs

    def test_square_offset_inward_collapse(self):
        """Test that offsetting a square inward past its half-width produces no curves."""
        a = make_area(make_curve([(0, 0), (10, 0), (10, 10), (0, 10), (0, 0)]))
        self.assert_offset_line_and_arc_count(a, -6.0, 0, 0, 0)  # 0 curves

    def test_square_with_semicircle_top_offset_outward(self):
        """Test offsetting a square with semicircular top outward."""
        a = make_area(make_curve([(0, 0), (10, 0), (10, 10), (0, 10, 1, 5, 10), (0, 0)]))
        self.assert_offset_line_and_arc_count(a, 1.0, 1, 3, 3)  # 1 curve, 3 lines, 3 CCW arcs

    def test_square_with_semicircle_top_offset_inward(self):
        """Test offsetting a square with semicircular top inward."""
        a = make_area(make_curve([(0, 0), (10, 0), (10, 10), (0, 10, 1, 5, 10), (0, 0)]))
        self.assert_offset_line_and_arc_count(a, -1.0, 1, 3, 1)  # 1 curve, 3 lines, 1 CCW arc

    def test_square_with_quarter_circle_top_offset_outward(self):
        """Test offsetting a square with quarter-circular top outward."""
        # The points at the end of the circle generate little ccw arcs centered on them
        a = make_area(make_curve([(0, 0), (10, 0), (10, 10), (0, 10, 1, 5, 5), (0, 0)]))
        self.assert_offset_line_and_arc_count(a, 1.0, 1, 3, 5)  # 1 curve, 3 lines, 5 CCW arcs

    def test_square_with_quarter_circle_top_offset_inward(self):
        """Test offsetting a square with quarter-circular top inward."""
        a = make_area(make_curve([(0, 0), (10, 0), (10, 10), (0, 10, 1, 5, 5), (0, 0)]))
        self.assert_offset_line_and_arc_count(a, -1.0, 1, 3, 1)  # 1 curve, 3 lines, 1 CCW arc

    def test_square_with_three_quarter_circle_top_offset_outward(self):
        """Test offsetting a square with 3/4-circular top outward."""
        a = make_area(make_curve([(0, 0), (10, 0), (10, 10), (0, 10, 1, 5, 15), (0, 0)]))
        self.assert_offset_line_and_arc_count(a, 1.0, 1, 3, 3)  # 1 curve, 3 lines, 3 CCW arcs

    def test_square_with_three_quarter_circle_top_offset_inward(self):
        """Test offsetting a square with 3/4-circular top inward."""
        # The points at the end of the circle generate little cw arcs centered on them
        a = make_area(make_curve([(0, 0), (10, 0), (10, 10), (0, 10, 1, 5, 15), (0, 0)]))
        self.assert_offset_line_and_arc_count(a, -1.0, 1, 3, 1, 2)  # 1 curve, 3 lines, 1 CCW, 2 CW

    def test_two_circles_offset_outward(self):
        """Test offsetting two overlapping circles outward."""
        # 8 shape: top circle (0,4) r=5 from (3,0) to (-3,0), bottom circle (0,-4) r=5 back to (3,0)
        a = make_area(make_curve([(3, 0), (-3, 0, 1, 0, 4), (3, 0, 1, 0, -4)]))
        self.assert_offset_line_and_arc_count(a, 1.0, 1, 0, 2)  # 1 curve, 2 CCW arcs

    def test_two_circles_offset_inward(self):
        """Test offsetting two overlapping circles inward."""
        # 8 shape: top circle (0,4) r=5 from (3,0) to (-3,0), bottom circle (0,-4) r=5 back to (3,0)
        a = make_area(make_curve([(3, 0), (-3, 0, 1, 0, 4), (3, 0, 1, 0, -4)]))
        self.assert_offset_line_and_arc_count(a, -1.0, 1, 0, 2, 2)  # 1 curve, 2 CCW arcs, 2 CW arcs

    def test_two_shapes_offset_merge(self):
        """Test offsetting two shapes that merge together."""
        # Bottom: square with semicircular top (top of arc at y=15)
        # Top: 10x10 square with 3-unit gap from arc top
        a = make_area(
            [
                make_curve([(0, 0), (10, 0), (10, 10), (0, 10, 1, 5, 10), (0, 0)]),
                make_curve([(0, 18), (10, 18), (10, 28), (0, 28), (0, 18)]),
            ]
        )
        a2 = make_area(a.getCurves())

        # Offset by 1.0 (not enough to merge)
        self.assert_offset_line_and_arc_count(a, 1, 2, 7, 7)  # 2 curves, 7 lines, 7 CCW arcs

        # Offset by 2.0 (merges --> +1 line, +1 arc)
        self.assert_offset_line_and_arc_count(a2, 2, 1, 8, 8)  # 1 curve, 8 lines, 8 CCW arcs

    def test_sharp_triangle(self):
        a = make_area(make_curve([(-1, 0), (1, 0), (0, 10), (-1, 0)]))
        self.assert_offset_line_and_arc_count(a, 150, 1, 3, 3)  # 1 curve, 3 lines, 3 CCW arcs

    def test_mirrored_semicircles(self):
        a = make_mirrored_arcs(5, 0)  # arcs about (0, 0) that hit x-axis at 5
        self.assert_offset_line_and_arc_count(a, 150, 1, 0, 2)  # 1 curve, 0 lines, 2 CCW arcs

    def test_mirrored_less_than_semicircles(self):
        # choose y big enough to not round the point expansion to nothing, but just barely
        x = 5
        y = area.get_accuracy() * x * 1.5
        a = make_mirrored_arcs(x, -y)  # arcs about (0, -+epsilon) that hit x-axis at 5
        self.assert_offset_line_and_arc_count(a, 1, 1, 0, 4)  # 1 curve, 0 lines, 4 CCW arcs

    def test_canonicalize(self):
        """Test canonicalization of area."""
        # Create a simple square starting at different positions
        square1 = make_curve([(0, 0), (10, 0), (10, 10), (0, 10), (0, 0)])
        square2 = make_curve([(10, 0), (10, 10), (0, 10), (0, 0), (10, 0)])  # rotated start

        a1 = make_area(square1)
        a2 = make_area(square2)

        # Canonicalize both
        a1_canon = canonicalize_area(a1)
        a2_canon = canonicalize_area(a2)
        self.assertTrue(areas_equal(a1_canon, a2_canon))

    def test_rotate_canonicalize(self):
        """Test canonicalization of area."""
        # Create a simple square starting at different positions
        square1 = make_curve([(0, 0), (10, 0), (10, 10), (0, 10), (0, 0)])
        a1 = make_area(square1)
        a2 = rotate_curve_in_area(a1, 0, 1)

        # Canonicalize both
        a1_canon = canonicalize_area(a1)
        a2_canon = canonicalize_area(a2)
        self.assertTrue(areas_equal(a1_canon, a2_canon))


class TestArcFittingBooleans(PathTestBase):
    """Tests for arc fitting with boolean operations."""

    def setUp(self):
        """Set up test geometry."""
        super().setUp()

        # Semicircle (D-shape): straight left edge + curved right side
        semicircle_curve = make_curve(
            [
                (0, 0),
                (0, 10, 1, 0, 5),  # CCW arc from (0,10) to (0,0), center at (0,5), radius 5
                (0, 0),
            ]
        )
        self.semicircle = make_area(semicircle_curve)

        # Square overlapping the middle of the curved part
        square_curve = make_curve(
            [
                (3, 0),
                (13, 0),
                (13, 10),
                (3, 10),
                (3, 0),
            ]
        )
        self.square = make_area(square_curve)

    def assert_boolean_line_and_arc_count(
        self,
        a1_orig,
        a2_orig,
        operation,
        expected_curves,
        expected_lines,
        expected_ccw_arcs,
        expected_cw_arcs=0,
    ):
        """Helper: Perform boolean operation and assert line and arc counts.

        Args:
            a1_orig: First area object
            a2_orig: Second area object
            operation: Boolean operation name ("Union", "Subtract", "Intersect", "Xor")
            expected_curves: Expected number of curves after operation
            expected_lines: Expected total number of line segments across all curves
            expected_ccw_arcs: Expected total number of CCW arc segments (type=1) across all curves
            expected_cw_arcs: Expected total number of CW arc segments (type=-1) across all curves (default 0)
        """
        # Make copies to preserve the originals
        a1 = area.copy_area(a1_orig)
        a2 = area.copy_area(a2_orig)

        # Store input curves for debug output
        input1_summary = []
        for i, curve in enumerate(a1.getCurves()):
            vertices = list(curve.getVertices())
            input1_summary.append(f"  Curve {i}: {len(vertices)} vertices")
            for v in vertices:
                input1_summary.append(
                    f"    type={v.type:2d}, p=({v.p.x:6.2f}, {v.p.y:6.2f}), c=({v.c.x:6.2f}, {v.c.y:6.2f})"
                )

        input2_summary = []
        for i, curve in enumerate(a2.getCurves()):
            vertices = list(curve.getVertices())
            input2_summary.append(f"  Curve {i}: {len(vertices)} vertices")
            for v in vertices:
                input2_summary.append(
                    f"    type={v.type:2d}, p=({v.p.x:6.2f}, {v.p.y:6.2f}), c=({v.c.x:6.2f}, {v.c.y:6.2f})"
                )

        # Perform the boolean operation
        op_method = getattr(a1, operation)
        op_method(a2)

        curves = a1.getCurves()

        # Build detailed output for debugging
        result_summary = []
        total_lines = 0
        total_ccw_arcs = 0
        total_cw_arcs = 0

        for i, curve in enumerate(curves):
            vertices = list(curve.getVertices())
            lines = [v for v in vertices[1:] if v.type == 0]
            ccw_arcs = [v for v in vertices[1:] if v.type == 1]
            cw_arcs = [v for v in vertices[1:] if v.type == -1]
            total_lines += len(lines)
            total_ccw_arcs += len(ccw_arcs)
            total_cw_arcs += len(cw_arcs)

            result_summary.append(
                f"  Curve {i}: {len(vertices)} vertices ({len(lines)} lines, {len(ccw_arcs)} CCW arcs, {len(cw_arcs)} CW arcs)"
            )
            for v in vertices:
                result_summary.append(
                    f"    type={v.type:2d}, p=({v.p.x:6.2f}, {v.p.y:6.2f}), c=({v.c.x:6.2f}, {v.c.y:6.2f})"
                )

        # Check curve count
        if len(curves) != expected_curves:
            self.fail(
                f"Expected {expected_curves} curves after {operation}, got {len(curves)}\n"
                f"Input area 1:\n" + "\n".join(input1_summary) + "\n"
                f"Input area 2:\n" + "\n".join(input2_summary) + "\n"
                f"Result curves:\n" + "\n".join(result_summary)
            )

        # Check line and arc counts
        if (
            total_lines != expected_lines
            or total_ccw_arcs != expected_ccw_arcs
            or total_cw_arcs != expected_cw_arcs
        ):
            self.fail(
                f"Expected {expected_lines} lines, {expected_ccw_arcs} CCW arcs, and {expected_cw_arcs} CW arcs after {operation}, "
                f"got {total_lines} lines, {total_ccw_arcs} CCW arcs, and {total_cw_arcs} CW arcs\n"
                f"Input area 1:\n" + "\n".join(input1_summary) + "\n"
                f"Input area 2:\n" + "\n".join(input2_summary) + "\n"
                f"Result curves:\n" + "\n".join(result_summary)
            )

        # Test rotation invariance: result should be the same regardless of input curve starting positions
        result_orig_canon = canonicalize_area(a1)
        for curve1_i, curve1 in enumerate(a1_orig.getCurves()):
            for start1_i in range(1, len(curve1.getVertices())):
                for curve2_i, curve2 in enumerate(a2_orig.getCurves()):
                    for start2_i in range(1, len(curve2.getVertices())):
                        # Create rotated versions of both areas
                        rotated_a1 = rotate_curve_in_area(a1_orig, curve1_i, start1_i)
                        rotated_a2 = rotate_curve_in_area(a2_orig, curve2_i, start2_i)

                        # Perform same operation
                        op_method = getattr(rotated_a1, operation)
                        op_method(rotated_a2)

                        # Check result is the same
                        rotated_result_canon = canonicalize_area(rotated_a1)
                        self.assertTrue(
                            areas_equal(rotated_result_canon, result_orig_canon),
                            f"{operation} result differs when area1 curve {curve1_i} starts at {start1_i} "
                            f"and area2 curve {curve2_i} starts at {start2_i}",
                        )

    def test_union_square_semicircle(self):
        """Test union of square overlapping a semicircular shape."""
        self.assert_boolean_line_and_arc_count(self.semicircle, self.square, "Union", 1, 6, 2)

    def test_intersect_square_semicircle(self):
        """Test intersection of square overlapping a semicircular shape."""
        self.assert_boolean_line_and_arc_count(self.semicircle, self.square, "Intersect", 1, 1, 1)

    def test_subtract_square_from_semicircle(self):
        """Test subtracting square from semicircular shape (semicircle - square)."""
        self.assert_boolean_line_and_arc_count(self.semicircle, self.square, "Subtract", 1, 2, 2)

    def test_subtract_semicircle_from_square(self):
        """Test subtracting semicircle from square (square - semicircle)."""
        self.assert_boolean_line_and_arc_count(self.square, self.semicircle, "Subtract", 1, 5, 0, 1)


class TestArcFittingOpenPathReversal(PathTestBase):
    """Tests for open path reversal handling in clipper operations."""

    def assert_open_path_reversal_identical(self, closed_curve):
        """Helper: Assert that TestIntersectOpenPathReversal produces identical results for all reversal permutations.

        Args:
            closed_area: Area containing closed path(s) to intersect with
        """
        # Use the same open curve for all tests
        open_curve = make_curve([(1, 1), (3, 3), (5, 3), (7, 1)])

        a = [make_area(open_curve) for i in range(4)]
        closed_area = make_area(closed_curve)

        # Test all 4 permutations of the two types of reversal
        a[0].TestIntersectOpenPathReversal(closed_area, False, False)
        a[1].TestIntersectOpenPathReversal(closed_area, False, True)
        a[2].TestIntersectOpenPathReversal(closed_area, True, False)
        a[3].TestIntersectOpenPathReversal(closed_area, True, True)

        def format_area(area_obj, label):
            """Format area for debug output."""
            lines = [f"\n{label}:"]
            for i, curve in enumerate(area_obj.getCurves()):
                vertices = list(curve.getVertices())
                lines.append(f"  Curve {i} ({len(vertices)} vertices):")
                for j, v in enumerate(vertices):
                    lines.append(
                        f"    [{j}] type={v.type}, p=({v.p.x:.2f}, {v.p.y:.2f}), c=({v.c.x:.2f}, {v.c.y:.2f})"
                    )
            return "\n".join(lines)

        # Assert that output points have increasing x values across all curves
        lastX = None
        for curve_idx, curve in enumerate(a[0].getCurves()):
            vertices = list(curve.getVertices())
            for vert_idx, v in enumerate(vertices):
                if lastX is not None and v.p.x < lastX:
                    msg = f"Output (no reversal) vertices do not have increasing x values: "
                    msg += f"curve[{curve_idx}] vertex[{vert_idx}].x={v.p.x} < lastX={lastX}"
                    msg += format_area(make_area(open_curve), "Input open curve")
                    msg += format_area(closed_area, "Input closed curve")
                    msg += format_area(a[0], "Output (no reversal)")
                    self.fail(msg)
                lastX = v.p.x

        # All results should be identical
        if not areas_equal(a[0], a[1]):
            msg = "Results differ: 'no reversal' vs 'path order reversed'"
            msg += format_area(make_area(open_curve), "Input open curve")
            msg += format_area(closed_area, "Input closed curve")
            msg += format_area(a[0], "Output (no reversal)")
            msg += format_area(a[1], "Output (path order reversed)")
            self.fail(msg)

        if not areas_equal(a[0], a[2]):
            msg = "Results differ: 'no reversal' vs 'path contents reversed'"
            msg += format_area(make_area(open_curve), "Input open curve")
            msg += format_area(closed_area, "Input closed curve")
            msg += format_area(a[0], "Output (no reversal)")
            msg += format_area(a[2], "Output (path contents reversed)")
            self.fail(msg)

        if not areas_equal(a[0], a[3]):
            msg = "Results differ: 'no reversal' vs both 'path contents and order reversed'"
            msg += format_area(make_area(open_curve), "Input open curve")
            msg += format_area(closed_area, "Input closed curve")
            msg += format_area(a[0], "Output (no reversal)")
            msg += format_area(a[3], "Output (both path contents and order reversed)")
            self.fail(msg)

    def test_open_path_reversal_no_clip(self):
        """Test open path reversal when path is fully contained in box (no clipping)."""
        closed_curve = make_curve([(0, 0), (8, 0), (8, 8), (0, 8), (0, 0)])
        self.assert_open_path_reversal_identical(closed_curve)

    def test_open_path_reversal_clip_first_vertex(self):
        """Test open path reversal when box removes the first vertex."""
        closed_curve = make_curve([(2, 0), (8, 0), (8, 8), (2, 8), (2, 0)])
        self.assert_open_path_reversal_identical(closed_curve)

    def test_open_path_reversal_clip_first_segment(self):
        """Test open path reversal when box removes the first segment."""
        closed_curve = make_curve([(4, 0), (8, 0), (8, 8), (4, 8), (4, 0)])
        self.assert_open_path_reversal_identical(closed_curve)

    def test_open_path_reversal_clip_last_segment(self):
        """Test open path reversal when box removes the last segment."""
        closed_curve = make_curve([(0, 0), (4, 0), (4, 8), (0, 8), (0, 0)])
        self.assert_open_path_reversal_identical(closed_curve)

    def test_open_path_reversal_clip_last_vertex(self):
        """Test open path reversal when box removes the last vertex."""
        closed_curve = make_curve([(0, 0), (6, 0), (6, 8), (0, 8), (0, 0)])
        self.assert_open_path_reversal_identical(closed_curve)

    def test_open_path_reversal_keep_segment_middle(self):
        """Test open path reversal when box keeps only the middle of one segment."""
        closed_curve = make_curve([(1.5, 0), (2.5, 0), (2.5, 8), (1.5, 8), (1.5, 0)])
        self.assert_open_path_reversal_identical(closed_curve)

    def test_open_path_reversal_split_path(self):
        """Test open path reversal when box splits the path in two."""
        closed_curve = make_curve([(0, 0), (8, 0), (8, 2.5), (0, 2.5), (0, 0)])
        self.assert_open_path_reversal_identical(closed_curve)

    def test_open_path_reversal_split_single_segment(self):
        """Test open path reversal when a single segment is split in two by an L-shaped area."""
        # Create an L-shaped closed curve that captures two separate parts of the first segment
        closed_curve = make_curve(
            [
                (1.25, 0),
                (1.75, 0),
                (1.75, 2.25),
                (3, 2.25),
                (3, 2.75),
                (1.25, 2.75),
                (1.25, 0),
            ]
        )
        self.assert_open_path_reversal_identical(closed_curve)


if __name__ == "__main__":
    unittest.main()
