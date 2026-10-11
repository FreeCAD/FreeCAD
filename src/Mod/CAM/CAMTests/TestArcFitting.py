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

import itertools
import unittest
import area
import math
from CAMTests.PathTestUtils import PathTestBase


def format_vertex(v, prev=None, p_decimals=3):
    """One-line description of a vertex. For arcs, includes the center and, if prev is given, the
    signed angular span from prev (positive CCW, negative CW)."""
    w = p_decimals + 4
    text = f"type={v.type:2d}, p=({v.p.x:{w}.{p_decimals}f}, {v.p.y:{w}.{p_decimals}f})"
    if v.type != 0:
        text += f", c=({v.c.x:7.3f}, {v.c.y:7.3f})"
        if prev is not None:
            a0 = math.atan2(prev.p.y - v.c.y, prev.p.x - v.c.x)
            a1 = math.atan2(v.p.y - v.c.y, v.p.x - v.c.x)
            span = a1 - a0
            if v.type == 1 and span < 0:
                span += 2 * math.pi  # CCW arcs
            elif v.type == -1 and span > 0:
                span -= 2 * math.pi  # CW arcs
            text += f", {math.degrees(span):7.2f}°"
    return text


def count_segments(vertices):
    """(lines, ccw_arcs, cw_arcs) among the segments of a curve's vertex list."""
    types = [v.type for v in vertices[1:]]
    return types.count(0), types.count(1), types.count(-1)


def format_curves(area_obj, counts=False):
    """Multi-line description of an area's curves and vertices. With counts=True, each curve header
    also lists its line/arc segment counts."""
    lines = []
    for i, curve in enumerate(area_obj.getCurves()):
        vertices = list(curve.getVertices())
        header = f"  Curve {i}: {len(vertices)} vertices"
        if counts:
            n_lines, n_ccw, n_cw = count_segments(vertices)
            header += f" ({n_lines} lines, {n_ccw} CCW arcs, {n_cw} CW arcs)"
        lines.append(header)
        for j, v in enumerate(vertices):
            lines.append("    " + format_vertex(v, vertices[j - 1] if j > 0 else None))
    return "\n".join(lines)


def format_area(area_obj, label):
    return f"\n{label}:\n" + format_curves(area_obj)


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


def curves_equal(c1, c2, tol=1e-6, ctol=0):
    vertices1 = list(c1.getVertices())
    vertices2 = list(c2.getVertices())

    if len(vertices1) != len(vertices2):
        return False

    for v1, v2 in zip(vertices1, vertices2):
        if v1.type != v2.type:
            return False
        if abs(v1.p.x - v2.p.x) > tol or abs(v1.p.y - v2.p.y) > tol:
            return False
        if v1.type != 0:
            if abs(v1.c.x - v2.c.x) > ctol or abs(v1.c.y - v2.c.y) > ctol:
                return False

    return True


def areas_equal(a1, a2, tol=1e-6, ctol=1e-2):
    """Compare if two areas are equal within tolerance, including curve order.

    Note: You may want to canonicalize areas before using this function.
    """
    curves1 = list(a1.getCurves())
    curves2 = list(a2.getCurves())

    if len(curves1) != len(curves2):
        return False

    return all(curves_equal(c1, c2, tol, ctol) for c1, c2 in zip(curves1, curves2))


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

        # Compare exhaustively
        if len(orig) != len(result):
            orig_str = "\n  ".join([format_vertex(v) for v in orig])
            result_str = "\n  ".join([format_vertex(v) for v in result])
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
                orig_str = "\n  ".join([format_vertex(v) for v in orig])
                result_str = "\n  ".join([format_vertex(v) for v in result])
                self.fail(
                    f"Vertex {vert_idx} {mismatch}:\n"
                    f"  Original: {format_vertex(orig_v)}\n"
                    f"  Result:   {format_vertex(result_v)}\n"
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

    def test_no_fit_arcs_lines_unchanged(self):
        """Test that round-trip with m_fit_arcs=False leaves line-only curves unchanged."""
        c = make_curve([(0, 0), (10, 0), (10, 10), (0, 10), (0, 0)])
        prev = area.get_fit_arcs()
        try:
            area.set_fit_arcs(False)
            self.assert_area_unchanged_by_roundtrip(make_area(c))
        finally:
            area.set_fit_arcs(prev)

    def test_no_fit_arcs_arcs_become_lines(self):
        """Test that round-trip with m_fit_arcs=False converts arcs to line segments."""
        c = make_curve([(10, 0), (0, 10, 1, 0, 0)])
        a = make_area(c)
        prev = area.get_fit_arcs()
        try:
            area.set_fit_arcs(False)
            a.ClipperNoop()
        finally:
            area.set_fit_arcs(prev)

        for curve in a.getCurves():
            for v in list(curve.getVertices())[1:]:
                self.assertEqual(v.type, 0, f"Expected line (type=0) but got type={v.type}")

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
        orig_summary = format_curves(a)

        a.Offset(offset_distance)

        # Get the resulting curves
        curves = a.getCurves()

        # Count segments, and build detailed output for debugging
        counts = [count_segments(list(curve.getVertices())) for curve in curves]
        total_lines = sum(c[0] for c in counts)
        total_ccw_arcs = sum(c[1] for c in counts)
        total_cw_arcs = sum(c[2] for c in counts)
        result_summary = format_curves(a, counts=True)

        # Check curve count
        if len(curves) != expected_curves:
            self.fail(
                f"Expected {expected_curves} curves after offset, got {len(curves)}\n"
                f"Offset distance: {offset_distance}\n"
                f"Original curves:\n" + orig_summary + "\n"
                f"Result curves:\n" + result_summary
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
                f"Original curves:\n" + orig_summary + "\n"
                f"Result curves:\n" + result_summary
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

    def test_regression_32993_no_full_loops(self):
        """Regression test from issue #32993: offsetting this curve must not produce spurious full-circle loops."""
        a = make_area(
            make_curve(
                [
                    (-5.75750550, 5.72138770),
                    (-6.26980670, -0.26675220, -1, -85.35090510, 9.51482961),
                    (-7.24018490, -6.19758460, -1, -82.53221598, 9.16618609),
                    (5.96435000, -6.19758450),
                    (6.78940620, -0.42110040, 1, -82.09355169, 9.32690359),
                    (7.23967320, 5.39673010, 1, -83.87094484, 9.52183613),
                    (6.32998440, 5.70523940, 1, 6.74018482, 5.41934480),
                    (-4.83506040, 5.96415700, -1, 0.83330052, 9.53622689),
                    (-5.75750550, 5.72138770, 1, -5.25807239, 5.69758455),
                ]
            )
        )

        original_scale = area.get_clipper_scale()
        area.set_clipper_scale_and_point_tolerance(1e7)
        try:
            self.assert_offset_line_and_arc_count(a, 2.5, 1, 1, 6, 3)
        finally:
            area.set_clipper_scale_and_point_tolerance(original_scale)

    def _make_32677_test_geometry(self):
        """Reproduce test geometry from cam_mill_face_issue.FCStd in issue #32677"""
        return make_area(
            make_curve(
                [
                    (30.7105992000, -23.6063009000),
                    (31.7566086000, -24.4752671000, 1, 65.6674931178, 17.4086687027),
                    (32.8111863000, -25.3072878000, 1, 65.2530561250, 16.8967902112),
                    (33.8866673000, -26.1120977000, 1, 64.4750831937, 15.8847144420),
                    (34.9695836000, -26.8796130000, 1, 64.1111977152, 15.3851125609),
                    (36.0722028000, -27.6185340000, 1, 63.4350721510, 14.4045127437),
                    (38.2963008000, -28.9842817000, 1, 62.9700386254, 13.6903286664),
                    (40.5909579000, -30.2276617000, 1, 61.9205959018, 11.8752587487),
                    (42.9037648000, -31.3200521000, 1, 61.4905811646, 11.0264509400),
                    (45.2743594000, -32.2804118000, 1, 60.8173205874, 9.4925453535),
                    (47.6569200000, -33.0898676000, 1, 60.5624846162, 8.8076583118),
                    (50.0824895000, -33.7592883000, 1, 60.2037386794, 7.6430004552),
                    (52.5151510000, -34.2783799000, 1, 60.0862154590, 7.1622564049),
                    (54.9743675000, -34.6516864000, 1, 59.9533133706, 6.4348095398),
                    (57.4368415000, -34.8752415000, 1, 59.9232771251, 6.1869490285),
                    (59.9082985000, -34.9499999000, 1, 59.9082967493, 5.9395846679),
                    (67.9082985000, -34.9500000000),
                    (70.4365979000, -34.8717598000, 1, 67.9082984873, 5.9396129066),
                    (72.9552825000, -34.6378281000, 1, 67.8922644092, 6.1984322312),
                    (75.4704689000, -34.2471609000, 1, 67.8601062163, 6.4577894833),
                    (77.9572514000, -33.7040966000, 1, 67.7179358957, 7.2182168426),
                    (80.4363941000, -33.0036755000, 1, 67.5922516466, 7.7205248581),
                    (82.8695917000, -32.1571340000, 1, 67.2089970480, 8.9356886669),
                    (85.2898534000, -31.1526410000, 1, 66.9369506128, 9.6495284755),
                    (87.6484894000, -30.0107375000, 1, 66.2191696835, 11.2452988878),
                    (88.8205068000, -29.3808435000, 1, 65.8797780772, 11.8987005222),
                    (89.9745202000, -28.7185299000, 1, 65.3738145878, 12.8091403907),
                    (91.1226911000, -28.0168045000, 1, 65.0946279954, 13.2804175130),
                    (92.2513203000, -27.2840517000, 1, 64.4843132602, 14.2487685957),
                    (93.3730323000, -26.5120664000, 1, 64.1522448348, 14.7454637637),
                    (94.4738101000, -25.7105036000, 1, 63.4353774673, 15.7576259828),
                    (95.5665129000, -24.8699941000, 1, 63.0499528069, 16.2725630587),
                    (96.6370311000, -24.0013961000, 1, 62.2268091146, 17.3140704102),
                    (97.6982437000, -23.0942577000, 1, 61.7888072552, 17.8399679520),
                    (98.7361570000, -22.1605424000, 1, 60.8621709672, 18.8962679845),
                    (99.7634736000, -21.1888242000, 1, 60.3736196792, 19.4258757624),
                    (100.7665046000, -20.1920495000, 1, 59.3487565990, 20.4826074432),
                    (101.7576005000, -19.1579492000, 1, 58.8129063160, 21.0088438383),
                    (102.7235441000, -18.1003091000, 1, 57.6974882501, 22.0521104727),
                    (103.6761812000, -17.0061666000, 1, 57.1187612278, 22.5681958761),
                    (104.6029094000, -15.8899881000, 1, 55.9227448674, 23.5848326690),
                    (105.5149409000, -14.7382795000, 1, 55.3066534713, 24.0844076132),
                    (106.4004072000, -13.5660178000, 1, 54.0420944149, 25.0622119671),
                    (107.2697831000, -12.3593488000, 1, 53.3951723826, 25.5394896238),
                    (108.1120276000, -11.1335829000, 1, 52.0760233493, 26.4674632157),
                    (108.9367979000, -9.8746823000, 1, 51.4056658580, 26.9172837070),
                    (109.7339517000, -8.5981105000, 1, 50.0475226567, 27.7858353356),
                    (110.5122713000, -7.2898231000, 1, 49.3618788736, 28.2037934327),
                    (111.2625609000, -5.9652573000, 1, 47.9816845475, 29.0049017534),
                    (112.7015468000, -3.2267108000, 1, 46.9325438400, 29.5846759210),
                    (114.0264090000, -0.4311085000, 1, 44.1744331344, 30.9606651080),
                    (115.2607209000, 2.4708294000, 1, 42.8206702198, 31.5690518920),
                    (116.3789205000, 5.4195033000, 1, 40.1971807413, 32.6228733599),
                    (117.3977560000, 8.4584859000, 1, 38.9488674925, 33.0686275177),
                    (118.2991130000, 11.5343953000, 1, 36.6099104217, 33.8023799761),
                    (119.0936792000, 14.6829334000, 1, 35.5394837077, 34.0941718513),
                    (119.7698600000, 17.8590277000, 1, 33.6231500891, 34.5393728194),
                    (120.3334293000, 21.0887433000, 1, 32.7944479602, 34.6998320816),
                    (120.7780433000, 24.3369897000, 1, 31.4164419933, 34.9140893511),
                    (121.1059933000, 27.6188735000, 1, 30.8798134249, 34.9776096851),
                    (121.3146711000, 30.9105032000, 1, 30.1245731937, 35.0392061485),
                    (120.4049825000, 31.2190126000, 1, 120.8151827682, 30.9331179317),
                    (109.2399375000, 31.4779300000, -1, 114.9082984873, 35.0500000155),
                    (108.3174924000, 31.2351608000, 1, 108.8169254948, 31.2113576686),
                    (108.1458444000, 28.5711994000, -1, 28.5761302273, 35.0356546708),
                    (107.8847533000, 25.9145206000, -1, 29.4137958521, 34.9676004866),
                    (107.5366450000, 23.2878116000, -1, 29.9950418908, 34.9005429790),
                    (107.0991270000, 20.6745227000, -1, 31.4701358903, 34.6796309824),
                    (106.5779939000, 18.1080785000, -1, 32.3492879049, 34.5168280813),
                    (105.9675485000, 15.5614225000, -1, 34.3724622085, 34.0695919334),
                    (105.2783211000, 13.0776435000, -1, 35.4966965261, 33.7789641789),
                    (104.5000068000, 10.6203554000, -1, 37.9472463478, 33.0519874441),
                    (103.6490507000, 8.2408326000, -1, 39.2502958746, 32.6127941284),
                    (102.7094564000, 5.8949378000, -1, 41.9853210403, 31.5777098462),
                    (101.7044970000, 3.6402647000, -1, 43.3925082725, 30.9825548659),
                    (100.6117030000, 1.4268830000, -1, 46.2577296881, 29.6390624371),
                    (99.4617332000, -0.6835198000, -1, 47.6915979347, 28.8948197301),
                    (98.2252645000, -2.7443680000, -1, 50.5329679382, 27.2714319170),
                    (97.5883997000, -3.7327365000, -1, 51.5879644196, 26.6074554349),
                    (96.9302768000, -4.7070740000, -1, 52.9522116943, 25.7076482063),
                    (96.2612111000, -5.6521256000, -1, 53.6207520494, 25.2452934130),
                    (95.5712067000, -6.5819939000, -1, 54.9273261574, 24.2985454500),
                    (94.8714057000, -7.4816778000, -1, 55.5634426320, 23.8152348121),
                    (94.1510552000, -8.3649878000, -1, 56.7981686249, 22.8321487292),
                    (93.4220470000, -9.2173530000, -1, 57.3952505923, 22.3334657801),
                    (92.6729581000, -10.0521186000, -1, 58.5459068928, 21.3257672879),
                    (91.9163292000, -10.8553177000, -1, 59.0983734352, 20.8178056697),
                    (91.1401789000, -11.6396611000, -1, 60.1549121058, 19.7981326664),
                    (90.3575708000, -12.3919527000, -1, 60.6583040436, 19.2873798004),
                    (89.5561024000, -13.1241099000, -1, 61.6130041234, 18.2690293629),
                    (88.7492076000, -13.8238621000, -1, 62.0640761622, 17.7622649660),
                    (87.9242262000, -14.5021873000, -1, 62.9116951993, 16.7589720794),
                    (87.0947850000, -15.1478791000, -1, 63.3084383343, 16.2631146072),
                    (86.2481535000, -15.7708504000, -1, 64.0462683412, 15.2887752474),
                    (85.3979503000, -16.3610744000, -1, 64.3879529670, 14.8107709489),
                    (84.5315853000, -16.9272978000, -1, 65.0158478486, 13.8791838657),
                    (83.6624457000, -17.4607615000, -1, 65.3030186602, 13.4258716428),
                    (82.7783119000, -17.9689760000, -1, 65.8233652869, 12.5504763868),
                    (81.8920999000, -18.4445034000, -1, 66.0578154171, 12.1284585661),
                    (80.9922045000, -18.8935851000, -1, 66.4754535208, 11.3220796034),
                    (80.0908200000, -19.3101182000, -1, 66.6601738390, 10.9375963794),
                    (79.1772070000, -19.6990834000, -1, 66.9822663827, 10.2121984207),
                    (77.3464536000, -20.3805956000, -1, 67.1893969955, 9.7041564118),
                    (75.4766272000, -20.9459001000, -1, 67.5833091277, 8.5374061160),
                    (73.6051115000, -21.3832585000, -1, 67.7126529518, 8.0542780454),
                    (71.7092390000, -21.6984197000, -1, 67.8587313019, 7.3245096026),
                    (69.8129093000, -21.8869017000, -1, 67.8918253206, 7.0750390449),
                    (67.9082985000, -21.9500000000, -1, 67.9082984873, 6.8267170696),
                    (59.9082985000, -21.9500000000),
                    (57.9209603000, -21.8812947000, -1, 59.9082984873, 6.8267908162),
                    (55.9430237000, -21.6761416000, -1, 59.9270055733, 7.0970239542),
                    (53.9656352000, -21.3330112000, -1, 59.9646066962, 7.3686000850),
                    (52.0158515000, -20.8571983000, -1, 60.1302943180, 8.1613107642),
                    (51.0420748000, -20.5673501000, -1, 60.2389650750, 8.5499342883),
                    (50.0783708000, -20.2455759000, -1, 60.4210676895, 9.1264697781),
                    (49.1154432000, -19.8887427000, -1, 60.5326489463, 9.4433474570),
                    (48.1645184000, -19.5010390000, -1, 60.7992092984, 10.1281710888),
                    (47.2148195000, -19.0780462000, -1, 60.9562854758, 10.4965254162),
                    (46.2788946000, -18.6253772000, -1, 61.3195677850, 11.2783865917),
                    (45.3447748000, -18.1372230000, -1, 61.5276834979, 11.6921608331),
                    (44.4260320000, -17.6206954000, -1, 61.9975962943, 12.5583344902),
                    (43.5098036000, -17.0685476000, -1, 62.2610450966, 13.0108058316),
                    (42.6103842000, -16.4894084000, -1, 62.8448841088, 13.9473571490),
                    (41.7143135000, -15.8746004000, -1, 63.1665967671, 14.4312778201),
                    (40.8363156000, -15.2342356000, -1, 63.8688451373, 15.4233518750),
                    (39.9626157000, -14.5582619000, -1, 64.2503076907, 15.9310999116),
                    (39.1080906000, -13.8581942000, -1, 65.0724979850, 16.9632299856),
                    (38.2589143000, -13.1227045000, -1, 65.5137011431, 17.4869665558),
                    (37.4298632000, -12.3645915000, -1, 66.4543422426, 18.5433943605),
                    (36.6072974000, -11.5713857000, -1, 66.9537631072, 19.0752245105),
                    (35.8056675000, -10.7570180000, -1, 68.0083336255, 20.1402253528),
                    (35.0117273000, -9.9080392000, -1, 68.5629501313, 20.6723589668),
                    (34.2394075000, -9.0393376000, -1, 69.7239585355, 21.7305647879),
                    (33.4760319000, -8.1366666000, -1, 70.3292963805, 22.2554745256),
                    (32.7348481000, -7.2156792000, -1, 71.5864065473, 23.2921878281),
                    (32.0038949000, -6.2615276000, -1, 72.2366177308, 23.8027608167),
                    (31.2956058000, -5.2904264000, -1, 73.5768372458, 24.8042527154),
                    (30.5988479000, -4.2871302000, -1, 74.2648086680, 25.2939328511),
                    (29.9251401000, -3.2682068000, -1, 75.6727358414, 26.2477188608),
                    (29.2642617000, -2.2182201000, -1, 76.3902257683, 26.7106367261),
                    (28.6267452000, -1.1538814000, -1, 77.8483500750, 27.6057243931),
                    (28.0033383000, -0.0597691000, -1, 78.5861496564, 28.0368120264),
                    (27.4035422000, 1.0474676000, -1, 80.0751991677, 28.8639151330),
                    (26.8191034000, 2.1830361000, -1, 80.8233156310, 29.2590031847),
                    (26.2584712000, 3.3305489000, -1, 82.3226440068, 30.0107179877),
                    (25.1879678000, 5.7046215000, -1, 83.4581914780, 30.5511124792),
                    (24.2135403000, 8.1197734000, -1, 86.4060182667, 31.8080660827),
                    (23.3141836000, 10.6274375000, -1, 87.8321650933, 32.3512665549),
                    (22.5116028000, 13.1677723000, -1, 90.5573605100, 33.2688630052),
                    (21.7917385000, 15.7844887000, -1, 91.8288347182, 33.6444667302),
                    (21.1690124000, 18.4260388000, -1, 94.1611060752, 34.2392131907),
                    (20.6351904000, 21.1263185000, -1, 95.1973783061, 34.4637135809),
                    (20.1986703000, 23.8440460000, -1, 96.9867642127, 34.7837919932),
                    (19.8556045000, 26.6016957000, -1, 97.7206070193, 34.8883401526),
                    (19.6099007000, 29.3697320000, -1, 98.8498399212, 35.0085167559),
                    (19.4604417000, 32.1581054000, -1, 99.2328987484, 35.0357793703),
                    (19.4083608000, 34.9500000000, -1, 99.6271013139, 35.0499958293),
                    (-20.5917639000, 34.9500000000),
                    (-20.6438449000, 32.1581054000, -1, -100.8105043394, 35.0499958293),
                    (-20.7933038000, 29.3697320000, -1, -100.4163019771, 35.0357755782),
                    (-21.0390076000, 26.6016957000, -1, -100.0332429466, 35.0085167559),
                    (-21.3820736000, 23.8440460000, -1, -98.9040096315, 34.8883434744),
                    (-21.8185936000, 21.1263185000, -1, -98.1701672381, 34.7837919932),
                    (-22.3524156000, 18.4260388000, -1, -96.3807813315, 34.4637135809),
                    (-22.9751416000, 15.7844887000, -1, -95.3445091007, 34.2392131907),
                    (-23.6950059000, 13.1677723000, -1, -93.0122377435, 33.6444667302),
                    (-24.4975868000, 10.6274375000, -1, -91.7407626115, 33.2688659297),
                    (-25.3969435000, 8.1197734000, -1, -89.0155681188, 32.3512665549),
                    (-26.3713710000, 5.7046215000, -1, -87.5894212921, 31.8080660827),
                    (-27.4418743000, 3.3305489000, -1, -84.6415956401, 30.5511099585),
                    (-28.0025064000, 2.1830361000, -1, -83.5060496546, 30.0107126201),
                    (-28.5869452000, 1.0474676000, -1, -82.0067186563, 29.2590031846),
                    (-29.1867413000, -0.0597691000, -1, -81.2586021935, 28.8639151332),
                    (-29.8101482000, -1.1538814000, -1, -79.7695526819, 28.0368120264),
                    (-30.4476647000, -2.2182201000, -1, -79.0317531005, 27.6057243931),
                    (-31.1085431000, -3.2682068000, -1, -77.5736287938, 26.7106367261),
                    (-31.7822509000, -4.2871302000, -1, -76.8561388670, 26.2477188609),
                    (-32.4790088000, -5.2904264000, -1, -75.4482116934, 25.2939328511),
                    (-33.1872979000, -6.2615276000, -1, -74.7602402712, 24.8042527154),
                    (-33.9182511000, -7.2156792000, -1, -73.4200207564, 23.8027608168),
                    (-34.6594349000, -8.1366666000, -1, -72.7698095727, 23.2921878280),
                    (-35.4228105000, -9.0393376000, -1, -71.5126994060, 22.2554745256),
                    (-36.1951304000, -9.9080392000, -1, -70.9073615611, 21.7305647881),
                    (-36.9890705000, -10.7570180000, -1, -69.7463531567, 20.6723589668),
                    (-37.7907004000, -11.5713857000, -1, -69.1917366509, 20.1402253528),
                    (-38.6132662000, -12.3645916000, -1, -68.1371661328, 19.0752245106),
                    (-39.4423173000, -13.1227044000, -1, -67.6377412264, 18.5433980562),
                    (-40.2914936000, -13.8581942000, -1, -66.6971072039, 17.4869639269),
                    (-41.1460187000, -14.5582619000, -1, -66.2559010103, 16.9632299856),
                    (-42.0197187000, -15.2342357000, -1, -65.4337107162, 15.9310999116),
                    (-42.8977166000, -15.8746004000, -1, -65.0522481628, 15.4233518750),
                    (-43.7937873000, -16.4894084000, -1, -64.3499997925, 14.4312778201),
                    (-44.6932066000, -17.0685475000, -1, -64.0282871343, 13.9473571489),
                    (-45.6094350000, -17.6206953000, -1, -63.4444481220, 13.0108058315),
                    (-46.5281779000, -18.1372230000, -1, -63.1809993197, 12.5583344901),
                    (-47.4622977000, -18.6253773000, -1, -62.7110897427, 11.6921591508),
                    (-48.3982226000, -19.0780462000, -1, -62.5029670870, 11.2783883926),
                    (-49.3479215000, -19.5010390000, -1, -62.1396885013, 10.4965254162),
                    (-50.2988463000, -19.8887427000, -1, -61.9826123238, 10.1281710888),
                    (-51.2617739000, -20.2455759000, -1, -61.7160519717, 9.4433474570),
                    (-52.2254779000, -20.5673501000, -1, -61.6044707149, 9.1264697781),
                    (-53.1992546000, -20.8571982000, -1, -61.4223681004, 8.5499342883),
                    (-55.1490382000, -21.3330112000, -1, -61.3136990176, 8.1613103556),
                    (-57.1264268000, -21.6761417000, -1, -61.1480114537, 7.3685997844),
                    (-59.1043634000, -21.8812948000, -1, -61.1104085987, 7.0970239542),
                    (-61.0917015000, -21.9500000000, -1, -61.0917015127, 6.8267908162),
                    (-69.0917015000, -21.9500000000),
                    (-70.8565024000, -21.8958335000, -1, -69.0917015127, 6.8266122399),
                    (-72.6147083000, -21.7339344000, -1, -69.0785915807, 7.0399786949),
                    (-74.3725826000, -21.4633410000, -1, -69.0522640839, 7.2542094911),
                    (-76.1110063000, -21.0873756000, -1, -68.9357573908, 7.8830791427),
                    (-77.8482292000, -20.6017490000, -1, -68.8325159303, 8.2999117450),
                    (-79.5540892000, -20.0152251000, -1, -68.5170169582, 9.3113150642),
                    (-81.2569131000, -19.3177879000, -1, -68.2923110798, 9.9083850408),
                    (-82.9178541000, -18.5256117000, -1, -67.6972936806, 11.2497311965),
                    (-84.5727102000, -17.6214261000, -1, -67.3160291673, 11.9955829434),
                    (-86.1767995000, -16.6298280000, -1, -66.3823181341, 13.5980769593),
                    (-87.7704934000, -15.5257490000, -1, -65.8215730246, 14.4543838022),
                    (-89.3062185000, -14.3422467000, -1, -64.5161197155, 16.2375085746),
                    (-90.8260989000, -13.0468287000, -1, -63.7669512991, 17.1616431233),
                    (-92.2823862000, -11.6802103000, -1, -62.0858269238, 19.0384279742),
                    (-93.7164807000, -10.2035879000, -1, -61.1541395893, 19.9862235799),
                    (-95.0827396000, -8.6639006000, -1, -59.1234923278, 21.8689155437),
                    (-96.4198593000, -7.0176573000, -1, -58.0299224720, 22.7974602128),
                    (-97.6860484000, -5.3161848000, -1, -55.7048463710, 24.6032047142),
                    (-98.9158687000, -3.5132206000, -1, -54.4836733562, 25.4735168880),
                    (-100.0725762000, -1.6624429000, -1, -51.9450693801, 27.1296548855),
                    (-101.1857013000, 0.2831578000, -1, -50.6422098028, 27.9090856702),
                    (-102.2242303000, 2.2696252000, -1, -47.9918563434, 29.3577088491),
                    (-103.2122456000, 4.3427234000, -1, -46.6620581928, 30.0219188209),
                    (-104.1246984000, 6.4502095000, -1, -44.0162259898, 31.2233797779),
                    (-104.9802155000, 8.6347404000, -1, -42.7196380749, 31.7577569115),
                    (-105.7595726000, 10.8476160000, -1, -40.2016795616, 32.6929039760),
                    (-106.4762682000, 13.1267192000, -1, -39.0000202723, 33.0933224767),
                    (-107.1164585000, 15.4285068000, -1, -36.7324306989, 33.7643150505),
                    (-107.6891077000, 17.7846580000, -1, -35.6850124659, 34.0371789441),
                    (-108.1850692000, 20.1581530000, -1, -33.7814952590, 34.4668344114),
                    (-108.6095723000, 22.5732983000, -1, -32.9413740307, 34.6283995932),
                    (-108.9573033000, 25.0006981000, -1, -31.4994428902, 34.8581212768),
                    (-109.9502283000, 24.9929414000, 1, -109.4533029835, 24.9375763476),
                    (-110.3672448000, 21.7738664000, -1, -200.2167011367, 35.0500029178),
                    (-112.6205840000, 19.8500001000, -1, -111.6466073786, 20.9907757823),
                    (-121.3151828000, 19.8500000000),
                    (-120.7792617000, 16.9841089000, 1, -33.9945873842, 34.6958798658),
                    (-120.1484586000, 14.1376118000, 1, -35.5448443516, 34.3794899446),
                    (-119.4301220000, 11.3430311000, 1, -36.4191045510, 34.1703184032),
                    (-118.6174419000, 8.5744346000, 1, -38.3486270152, 33.6397169856),
                    (-117.7220250000, 5.8708840000, 1, -39.3902651049, 33.3144480388),
                    (-116.7330566000, 3.2001618000, 1, -41.6067398090, 32.5379052238),
                    (-115.6671149000, 0.6066631000, 1, -42.7665600237, 32.0849812778),
                    (-114.5087478000, -1.9468607000, 1, -45.1664697802, 31.0487048986),
                    (-113.2799980000, -4.4121146000, 1, -46.3912397661, 30.4659150107),
                    (-111.9603860000, -6.8299088000, 1, -48.8661985989, 29.1753889243),
                    (-110.5776383000, -9.1496743000, 1, -50.1018918650, 28.4702288181),
                    (-109.1061541000, -11.4141415000, 1, -52.5452450229, 26.9503019966),
                    (-107.5792354000, -13.5722350000, 1, -53.7403062136, 26.1397093382),
                    (-105.9664168000, -15.6668503000, 1, -56.0535772092, 24.4334256083),
                    (-104.3060895000, -17.6482446000, 1, -57.1620041709, 23.5429087128),
                    (-102.5635767000, -19.5576918000, 1, -59.2607804935, 21.7091470258),
                    (-100.7814523000, -21.3485993000, 1, -60.2448310140, 20.7713633782),
                    (-98.9219130000, -23.0589045000, 1, -62.0637151740, 18.8814344587),
                    (-97.0303683000, -24.6468439000, 1, -62.8960726607, 17.9343085223),
                    (-95.0674093000, -26.1455074000, 1, -64.3923813683, 16.0677194537),
                    (-93.0795063000, -27.5193573000, 1, -65.0576521011, 15.1522164448),
                    (-91.0275676000, -28.7954771000, 1, -66.2136405455, 13.3918816086),
                    (-88.9569834000, -29.9455188000, 1, -66.7090813698, 12.5495586760),
                    (-86.8312124000, -30.9899061000, 1, -67.5325168684, 10.9767382180),
                    (-84.6921806000, -31.9078626000, 1, -67.8679807313, 10.2472385593),
                    (-82.5082848000, -32.7131372000, 1, -68.3910756966, 8.9365654271),
                    (-80.3155489000, -33.3922151000, 1, -68.5881398343, 8.3551712813),
                    (-78.0896292000, -33.9528747000, 1, -68.8648713994, 7.3700585790),
                    (-75.8583995000, -34.3878202000, 1, -68.9551913044, 6.9654643908),
                    (-73.6067730000, -34.7002732000, 1, -69.0572293676, 6.3542110526),
                    (-71.3526790000, -34.8874419000, 1, -69.0802280706, 6.1466737767),
                    (-69.0917015000, -34.9499999000, 1, -69.0917015127, 5.9394954616),
                    (-61.0917015000, -34.9500000000),
                    (-58.6202446000, -34.8752415000, 1, -61.0917015127, 5.9395846153),
                    (-56.1577705000, -34.6516864000, 1, -61.1066801506, 6.1869490286),
                    (-53.6985540000, -34.2783799000, 1, -61.1367163960, 6.4348095398),
                    (-51.2658926000, -33.7592882000, 1, -61.2696205154, 7.1622559715),
                    (-48.8403231000, -33.0898675000, 1, -61.3871417048, 7.6430004552),
                    (-46.4577624000, -32.2804117000, 1, -61.7458876416, 8.8076583119),
                    (-44.0871678000, -31.3200521000, 1, -62.0007219338, 9.4925460338),
                    (-41.7743609000, -30.2276616000, 1, -62.6739863547, 11.0264499175),
                    (-39.4797038000, -28.9842816000, 1, -63.1039989273, 11.8752587487),
                    (-37.2556059000, -27.6185340000, 1, -64.1534416508, 13.6903286664),
                    (-36.1529867000, -26.8796130000, 1, -64.6184751765, 14.4045127437),
                    (-35.0700704000, -26.1120977000, 1, -65.2946007407, 15.3851125610),
                    (-33.9945894000, -25.3072878000, 1, -65.6584862191, 15.8847144419),
                    (-32.9400117000, -24.4752671000, 1, -66.4364591504, 16.8967902111),
                    (-31.8940023000, -23.6063009000, 1, -66.8508961433, 17.4086687028),
                    (-30.8699709000, -22.7115317000, 1, -67.7287209755, 18.4386217334),
                    (-29.8556993000, -21.7802837000, 1, -68.1921288905, 18.9559834657),
                    (-28.8643612000, -20.8246517000, 1, -69.1655326449, 19.9903217139),
                    (-27.8840213000, -19.8331334000, 1, -69.6752052385, 20.5064921538),
                    (-26.9274586000, -18.8186500000, 1, -70.7376927043, 21.5320756012),
                    (-25.9831666000, -17.7690042000, 1, -71.2898370572, 22.0406194543),
                    (-25.0633923000, -16.6978031000, 1, -72.4327895122, 23.0448977801),
                    (23.8799893000, -16.6978031000, -1, -0.5917015127, -37.2294362303),
                    (24.7997636000, -17.7690042000, 1, 71.2493864867, 23.0448977801),
                    (25.7440555000, -18.8186499000, 1, 70.1064340317, 22.0406194542),
                    (26.7006182000, -19.8331335000, 1, 69.5542937194, 21.5320713159),
                    (27.6809582000, -20.8246518000, 1, 68.4918022130, 20.5064921538),
                    (28.6722963000, -21.7802838000, 1, 67.9821296195, 19.9903217138),
                    (29.6865679000, -22.7115316000, 1, 67.0087196393, 18.9559891817),
                    (30.7105992000, -23.6063009000, 1, 66.5453214225, 18.4386186991),
                ]
            )
        )

    def test_regression_32677_ensure_offset_vanishes(self):
        """Regression test from issue #32677: ensure that offsetting a certain amount results in an empty shape."""
        a = self._make_32677_test_geometry()

        original_scale = area.get_clipper_scale()
        area.set_clipper_scale_and_point_tolerance(1e7)
        try:
            a.Offset(-22.6)
        finally:
            area.set_clipper_scale_and_point_tolerance(original_scale)

        self.assertEqual(a.getCurves(), [], format_area(a, "Expected no offset curves, got"))

    def test_regression_32677_ensure_no_spikes(self):
        """Regression test from issue #32677: offsetting must not produce out-and-back spikes."""
        # Parameters defining an unexpected spike in the output
        tol_base = 1e-6  # max spike base
        tol_height = tol_base * 10  # min spike height

        # Offsets/starts to test. For debugging, leave all_* as is and modify test_* to the desired
        # test/debugging parameters. An assert at the end prevents accidentally shipping code with
        # test_* set to something other than all_*.
        numV = len(self._make_32677_test_geometry().getCurves()[0].getVertices())
        self.assertTrue(numV > 0, "This assert exists so CodeQL knows numV is used")
        all_offsets = (-1.6, -5.1, -8.6, -12.1, -15.6, -19.1)
        all_starts = [0, 111]

        test_offsets = all_offsets  # temporarily set to (val,) to test a specific offset
        test_starts = all_starts  # temporarily set to range(numV) for complete/intensive testing

        canon = {}  # offset -> (startV, canonical area) of the first start vertex tried
        for offset, startV in itertools.product(test_offsets, test_starts):
            with self.subTest(offset=offset, startV=startV):
                a = self._make_32677_test_geometry()
                a = rotate_curve_in_area(a, 0, startV)
                original_scale = area.get_clipper_scale()
                area.set_clipper_scale_and_point_tolerance(1e7)
                try:
                    a.Offset(offset)
                finally:
                    area.set_clipper_scale_and_point_tolerance(original_scale)

                # Detect spikes and label them
                labels = {}
                curves = a.getCurves()
                for curve_i, curve in enumerate(curves):
                    points = [v.p for v in curve.getVertices()]
                    n = len(points)
                    for i in range(n if curve.IsClosed() else n - 2):
                        i1 = i + 1 if i + 1 < n else (i + 2) % n
                        i2 = i + 2 if i + 2 < n else (i + 3) % n
                        p0 = points[i % n]
                        p1 = points[(i1) % n]
                        p2 = points[(i2) % n]

                        spike_height = math.hypot(p1.x - p0.x, p1.y - p0.y)
                        spike_base = math.hypot(p2.x - p0.x, p2.y - p0.y)
                        if spike_height > tol_height and spike_base < tol_base:
                            labels[(curve_i, i1 % n)] = f"SPIKE (height {spike_height:.6f})"

                # Generate debug output and fail the test
                if labels:
                    lines = [f"Offset {offset} produced {len(labels)} spike(s):"]
                    index_width = max([len(str(len(c.getVertices()) - 1)) for c in curves])
                    summary_lines = []
                    for curve_i, curve in enumerate(curves):
                        vertices = list(curve.getVertices())
                        lines.append(f"  Curve {curve_i}: {len(vertices)} vertices")
                        vertex_lines = [
                            f"  [{j:>{index_width}}] "
                            + format_vertex(v, vertices[j - 1], p_decimals=7)
                            for j, v in enumerate(vertices)
                        ]
                        width = max((len(line) for line in vertex_lines), default=0)
                        for j, line in enumerate(vertex_lines):
                            if (curve_i, j) in labels:
                                line = line.ljust(width)
                                line += "  <-- " + labels[(curve_i, j)]
                            lines.append(line)
                            summary_lines.append(f"Curve {curve_i} {line}")

                    lines += ["", f"{len(labels)} spikes found:"]
                    lines += summary_lines

                    self.fail("\n".join(lines))

                # Check that the canonicalized output is the same for every start vertex
                a_canon = canonicalize_area(a)
                if offset in canon:
                    ref_startV, ref_canon = canon[offset]
                    if not areas_equal(a_canon, ref_canon):
                        msg = (
                            f"Offset {offset} result differs when starting at vertex "
                            f"{startV} vs {ref_startV}"
                        )
                        msg += format_area(ref_canon, f"Output (start {ref_startV}, canonical)")
                        msg += format_area(a_canon, f"Output (start {startV}, canonical)")
                        self.fail(msg)
                else:
                    canon[offset] = (startV, a_canon)

        # Ensure the test doesn't pass if some offsets are disabled
        self.assertEqual(
            all_offsets,
            test_offsets,
            "test_offsets pass; restore `test_offsets = all_offsets` to allow the test to pass",
        )
        self.assertEqual(
            all_starts,
            test_starts,
            "test_starts pass; restore `test_starts = all_starts` to allow the test to pass",
        )

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
        input1_summary = format_curves(a1)
        input2_summary = format_curves(a2)

        # Perform the boolean operation
        op_method = getattr(a1, operation)
        op_method(a2)

        curves = a1.getCurves()

        # Count segments, and build detailed output for debugging
        counts = [count_segments(list(curve.getVertices())) for curve in curves]
        total_lines = sum(c[0] for c in counts)
        total_ccw_arcs = sum(c[1] for c in counts)
        total_cw_arcs = sum(c[2] for c in counts)
        result_summary = format_curves(a1, counts=True)

        # Check curve count
        if len(curves) != expected_curves:
            self.fail(
                f"Expected {expected_curves} curves after {operation}, got {len(curves)}\n"
                f"Input area 1:\n" + input1_summary + "\n"
                f"Input area 2:\n" + input2_summary + "\n"
                f"Result curves:\n" + result_summary
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
                f"Input area 1:\n" + input1_summary + "\n"
                f"Input area 2:\n" + input2_summary + "\n"
                f"Result curves:\n" + result_summary
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
        """Helper: Assert that Debug_IntersectOpenPathReversal produces identical results for all reversal permutations.

        Args:
            closed_area: Area containing closed path(s) to intersect with
        """
        # Use the same open curve for all tests
        open_curve = make_curve([(1, 1), (3, 3), (5, 3), (7, 1)])

        a = [make_area(open_curve) for i in range(4)]
        closed_area = make_area(closed_curve)

        # Test all 4 permutations of the two types of reversal
        a[0].Debug_IntersectOpenPathReversal(closed_area, False, False)
        a[1].Debug_IntersectOpenPathReversal(closed_area, False, True)
        a[2].Debug_IntersectOpenPathReversal(closed_area, True, False)
        a[3].Debug_IntersectOpenPathReversal(closed_area, True, True)

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
