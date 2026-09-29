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
        area.set_clipper_scale(1e7)
        try:
            self.assert_offset_line_and_arc_count(a, 2.5, 1, 1, 6, 3)
        finally:
            area.set_clipper_scale(original_scale)

    def _make_32677_test_geometry(self):
        """Reproduce test geometry from cam_mill_face_issue.FCStd in issue #32677"""
        return make_area(
            make_curve(
                [
                    (30.71059920, -23.60630090),
                    (31.75660860, -24.47526710, 1, 65.66749306, 17.40866870),
                    (32.81118630, -25.30728780, 1, 65.25305607, 16.89679021),
                    (33.88666730, -26.11209770, 1, 64.47508314, 15.88471444),
                    (34.96958370, -26.87961300, 1, 64.11119766, 15.38511256),
                    (36.07220290, -27.61853400, 1, 63.43507209, 14.40451274),
                    (38.29630070, -28.98428160, 1, 62.97003857, 13.69032866),
                    (40.59095770, -30.22766150, 1, 61.92059585, 11.87525875),
                    (42.90376480, -31.32005210, 1, 61.49058327, 11.02644992),
                    (45.27435930, -32.28041190, 1, 60.81732265, 9.49254449),
                    (47.65691990, -33.08986760, 1, 60.56248250, 8.80765901),
                    (50.08248960, -33.75928830, 1, 60.20373862, 7.64300045),
                    (52.51515100, -34.27837990, 1, 60.08621540, 7.16225640),
                    (54.97436750, -34.65168640, 1, 59.95331331, 6.43480954),
                    (57.43684160, -34.87524150, 1, 59.92327707, 6.18694903),
                    (59.90829850, -34.95000000, 1, 59.90829843, 5.93958461),
                    (67.90829850, -34.95000000),
                    (70.43659790, -34.87175980, 1, 67.90829843, 5.93961291),
                    (72.95528250, -34.63782810, 1, 67.89226435, 6.19843223),
                    (75.47046890, -34.24716090, 1, 67.86010617, 6.45778948),
                    (77.95725130, -33.70409660, 1, 67.71793584, 7.21821684),
                    (80.43639410, -33.00367550, 1, 67.59225340, 7.72052537),
                    (82.86959170, -32.15713400, 1, 67.20899699, 8.93568867),
                    (85.28985330, -31.15264110, 1, 66.93695056, 9.64952847),
                    (87.64848920, -30.01073770, 1, 66.21916963, 11.24529889),
                    (88.82050680, -29.38084380, 1, 65.87978250, 11.89870293),
                    (89.97452010, -28.71852990, 1, 65.37380744, 12.80913632),
                    (91.12269110, -28.01680450, 1, 65.09462794, 13.28041751),
                    (92.25132040, -27.28405170, 1, 64.48431320, 14.24876859),
                    (93.37303230, -26.51206640, 1, 64.15224478, 14.74546376),
                    (94.47381010, -25.71050360, 1, 63.43537741, 15.75762598),
                    (95.56651300, -24.86999420, 1, 63.04995607, 16.27256561),
                    (96.63703110, -24.00139610, 1, 62.22680567, 17.31406766),
                    (97.69824360, -23.09425770, 1, 61.78880720, 17.83996795),
                    (98.73615690, -22.16054240, 1, 60.86217091, 18.89626798),
                    (99.76347360, -21.18882420, 1, 60.37361962, 19.42587576),
                    (100.76650460, -20.19204940, 1, 59.34875654, 20.48260744),
                    (101.75760040, -19.15794910, 1, 58.81290626, 21.00884384),
                    (102.72354400, -18.10030900, 1, 57.69748819, 22.05211047),
                    (103.67618110, -17.00616660, 1, 57.11876117, 22.56819587),
                    (104.60290940, -15.88998810, 1, 55.92274481, 23.58483267),
                    (105.51494100, -14.73827950, 1, 55.30665629, 24.08441124),
                    (106.40040720, -13.56601780, 1, 54.04209436, 25.06221197),
                    (107.26978300, -12.35934880, 1, 53.39517233, 25.53948962),
                    (108.11202760, -11.13358290, 1, 52.07602329, 26.46746321),
                    (108.93679780, -9.87468230, 1, 51.40566580, 26.91728371),
                    (109.73395160, -8.59811050, 1, 50.04752260, 27.78583533),
                    (110.51227120, -7.28982310, 1, 49.36187882, 28.20379343),
                    (111.26256080, -5.96525730, 1, 47.98168449, 29.00490175),
                    (112.70154670, -3.22671080, 1, 46.93254378, 29.58467592),
                    (114.02640880, -0.43110850, 1, 44.17443308, 30.96066511),
                    (115.26072080, 2.47082940, 1, 42.82067016, 31.56905189),
                    (116.37892040, 5.41950330, 1, 40.19718068, 32.62287336),
                    (117.39775600, 8.45848590, 1, 38.94886744, 33.06862752),
                    (118.29911300, 11.53439530, 1, 36.60991037, 33.80237997),
                    (119.09367910, 14.68293340, 1, 35.53948365, 34.09417185),
                    (119.76985990, 17.85902770, 1, 33.62315003, 34.53937282),
                    (120.33342920, 21.08874330, 1, 32.79444790, 34.69983208),
                    (120.77804330, 24.33698970, 1, 31.41644239, 34.91409269),
                    (121.10599320, 27.61887350, 1, 30.87981337, 34.97760968),
                    (121.31467100, 30.91050320, 1, 30.12457314, 35.03920615),
                    (120.40498240, 31.21901250, 1, 120.81518271, 30.93311793),
                    (109.23993740, 31.47793000, -1, 114.90829843, 35.05000001),
                    (108.31749240, 31.23516080, 1, 108.81692544, 31.21135767),
                    (108.14584440, 28.57119940, -1, 28.57613017, 35.03565467),
                    (107.88475320, 25.91452060, -1, 29.41379580, 34.96760049),
                    (107.53664490, 23.28781160, -1, 29.99504183, 34.90054298),
                    (107.09912690, 20.67452270, -1, 31.47013583, 34.67963098),
                    (106.57799380, 18.10807850, -1, 32.34928785, 34.51682808),
                    (105.96754850, 15.56142250, -1, 34.37246215, 34.06959193),
                    (105.27832120, 13.07764350, -1, 35.49669558, 33.77896096),
                    (104.50000680, 10.62035540, -1, 37.94724728, 33.05199055),
                    (103.64905070, 8.24083260, -1, 39.25029582, 32.61279413),
                    (102.70945630, 5.89493780, -1, 41.98532212, 31.57771269),
                    (101.70449690, 3.64026470, -1, 43.39250954, 30.98255783),
                    (100.61170290, 1.42688300, -1, 46.25772963, 29.63906244),
                    (99.46173310, -0.68351980, -1, 47.69159788, 28.89481973),
                    (98.22526450, -2.74436800, -1, 50.53296788, 27.27143192),
                    (97.58839970, -3.73273650, -1, 51.58796436, 26.60745543),
                    (96.93027670, -4.70707400, -1, 52.95221164, 25.70764820),
                    (96.26121100, -5.65212560, -1, 53.62075199, 25.24529341),
                    (95.57120670, -6.58199390, -1, 54.92732610, 24.29854545),
                    (94.87140570, -7.48167770, -1, 55.56344258, 23.81523481),
                    (94.15105510, -8.36498770, -1, 56.79816857, 22.83214873),
                    (93.42204700, -9.21735300, -1, 57.39524648, 22.33346104),
                    (92.67295800, -10.05211860, -1, 58.54590684, 21.32576729),
                    (91.91632910, -10.85531760, -1, 59.09837338, 20.81780567),
                    (91.14017900, -11.63966110, -1, 60.15490748, 19.79812805),
                    (90.35757080, -12.39195280, -1, 60.65830399, 19.28737980),
                    (89.55610250, -13.12411010, -1, 61.61300018, 18.26902581),
                    (88.74920770, -13.82386210, -1, 62.06407920, 17.76226765),
                    (87.92422620, -14.50218740, -1, 62.91169514, 16.75897208),
                    (87.09478500, -15.14787920, -1, 63.30843828, 16.26311461),
                    (86.24815360, -15.77085040, -1, 64.04626828, 15.28877525),
                    (85.39795040, -16.36107440, -1, 64.38795291, 14.81077095),
                    (84.53158540, -16.92729780, -1, 65.01584779, 13.87918386),
                    (83.66244580, -17.46076150, -1, 65.30301860, 13.42587164),
                    (82.77831180, -17.96897610, -1, 65.82336523, 12.55047639),
                    (81.89209980, -18.44450350, -1, 66.05781536, 12.12845856),
                    (80.99220440, -18.89358520, -1, 66.47545346, 11.32207960),
                    (80.09082010, -19.31011830, -1, 66.66017023, 10.93759473),
                    (79.17720700, -19.69908340, -1, 66.98227022, 10.21220008),
                    (77.34645350, -20.38059560, -1, 67.18939694, 9.70415641),
                    (75.47662730, -20.94590000, -1, 67.58330907, 8.53740611),
                    (73.60511150, -21.38325850, -1, 67.71265112, 8.05427763),
                    (71.70923910, -21.69841980, -1, 67.85872939, 7.32450929),
                    (69.81290940, -21.88690170, -1, 67.89182691, 7.07503921),
                    (67.90829850, -21.95000000, -1, 67.90829843, 6.82671707),
                    (59.90829850, -21.95000000),
                    (57.92096040, -21.88129480, -1, 59.90829843, 6.82679081),
                    (55.94302380, -21.67614160, -1, 59.92700760, 7.09702374),
                    (53.96563520, -21.33301110, -1, 59.96460837, 7.36859978),
                    (52.01585160, -20.85719820, -1, 60.13029426, 8.16131076),
                    (51.04207480, -20.56735000, -1, 60.23896502, 8.54993429),
                    (50.07837080, -20.24557590, -1, 60.42106763, 9.12646978),
                    (49.11544320, -19.88874270, -1, 60.53264889, 9.44334746),
                    (48.16451840, -19.50103900, -1, 60.79920924, 10.12817109),
                    (47.21481950, -19.07804640, -1, 60.95628245, 10.49652674),
                    (46.27889460, -18.62537720, -1, 61.31957184, 11.27838460),
                    (45.34477490, -18.13722310, -1, 61.52768344, 11.69216083),
                    (44.42603190, -17.62069540, -1, 61.99759624, 12.55833449),
                    (43.50980360, -17.06854750, -1, 62.26104865, 13.01080366),
                    (42.61038420, -16.48940850, -1, 62.84488006, 13.94735972),
                    (41.71431360, -15.87460040, -1, 63.16660045, 14.43127525),
                    (40.83631560, -15.23423560, -1, 63.86884508, 15.42335187),
                    (39.96261570, -14.55826180, -1, 64.25030763, 15.93109991),
                    (39.10809060, -13.85819420, -1, 65.07249375, 16.96323341),
                    (38.25891420, -13.12270450, -1, 65.51370109, 17.48696655),
                    (37.42986320, -12.36459160, -1, 66.45434219, 18.54339436),
                    (36.60729730, -11.57138570, -1, 66.95376305, 19.07522451),
                    (35.80566750, -10.75701800, -1, 68.00833357, 20.14022535),
                    (35.01172730, -9.90803920, -1, 68.56295008, 20.67235897),
                    (34.23940740, -9.03933760, -1, 69.72395848, 21.73056479),
                    (33.47603190, -8.13666660, -1, 70.32929632, 22.25547452),
                    (32.73484800, -7.21567920, -1, 71.58640649, 23.29218783),
                    (32.00389480, -6.26152770, -1, 72.23661767, 23.80276082),
                    (31.29560580, -5.29042640, -1, 73.57684033, 24.80424841),
                    (30.59884780, -4.28713020, -1, 74.26480861, 25.29393285),
                    (29.92514000, -3.26820680, -1, 75.67273579, 26.24771886),
                    (29.26426160, -2.21822010, -1, 76.39022571, 26.71063672),
                    (28.62674520, -1.15388140, -1, 77.84835002, 27.60572439),
                    (28.00333830, -0.05976910, -1, 78.58614960, 28.03681202),
                    (27.40354230, 1.04746760, -1, 80.07520163, 28.86391049),
                    (26.81910340, 2.18303610, -1, 80.82331557, 29.25900318),
                    (26.25847110, 3.33054890, -1, 82.32264395, 30.01071799),
                    (25.18796780, 5.70462150, -1, 83.45819256, 30.55110996),
                    (24.21354030, 8.11977340, -1, 86.40601821, 31.80806608),
                    (23.31418370, 10.62743750, -1, 87.83216504, 32.35126655),
                    (22.51160270, 13.16777230, -1, 90.55735953, 33.26886593),
                    (21.79173850, 15.78448870, -1, 91.82883466, 33.64446673),
                    (21.16901240, 18.42603880, -1, 94.16110602, 34.23921319),
                    (20.63519040, 21.12631850, -1, 95.19737825, 34.46371358),
                    (20.19867040, 23.84404600, -1, 96.98676416, 34.78379199),
                    (19.85560450, 26.60169570, -1, 97.72060696, 34.88834015),
                    (19.60990060, 29.36973200, -1, 98.84983986, 35.00851675),
                    (19.46044170, 32.15810540, -1, 99.23289890, 35.03577558),
                    (19.40836080, 34.95000000, -1, 99.62710126, 35.04999583),
                    (-20.59176380, 34.95000000),
                    (-20.64384480, 32.15810540, -1, -100.81050440, 35.04999583),
                    (-20.79330370, 29.36973200, -1, -100.41630203, 35.03577558),
                    (-21.03900760, 26.60169570, -1, -100.03324300, 35.00851675),
                    (-21.38207350, 23.84404600, -1, -98.90401010, 34.88834015),
                    (-21.81859360, 21.12631850, -1, -98.17016729, 34.78379199),
                    (-22.35241550, 18.42603880, -1, -96.38078139, 34.46371358),
                    (-22.97514160, 15.78448870, -1, -95.34450916, 34.23921319),
                    (-23.69500590, 13.16777230, -1, -93.01223780, 33.64446673),
                    (-24.49758670, 10.62743750, -1, -91.74076360, 33.26886299),
                    (-25.39694350, 8.11977340, -1, -89.01556714, 32.35126943),
                    (-26.37137100, 5.70462150, -1, -87.58942135, 31.80806608),
                    (-27.44187420, 3.33054890, -1, -84.64159570, 30.55110996),
                    (-28.00250630, 2.18303610, -1, -83.50604969, 30.01071266),
                    (-28.58694520, 1.04746760, -1, -82.00671871, 29.25900318),
                    (-29.18674140, -0.05976910, -1, -81.25860225, 28.86391513),
                    (-29.81014830, -1.15388140, -1, -79.76955274, 28.03681202),
                    (-30.44766480, -2.21822010, -1, -79.03175316, 27.60572439),
                    (-31.10854310, -3.26820680, -1, -77.57362885, 26.71063672),
                    (-31.78225090, -4.28713020, -1, -76.85613892, 26.24771886),
                    (-32.47900890, -5.29042640, -1, -75.44821175, 25.29393285),
                    (-33.18729800, -6.26152760, -1, -74.76024033, 24.80425271),
                    (-33.91825120, -7.21567910, -1, -73.42002081, 23.80276082),
                    (-34.65943500, -8.13666660, -1, -72.76980963, 23.29218783),
                    (-35.42281060, -9.03933750, -1, -71.51269946, 22.25547452),
                    (-36.19513040, -9.90803920, -1, -70.90736469, 21.73056133),
                    (-36.98907060, -10.75701800, -1, -69.74635321, 20.67235897),
                    (-37.79070050, -11.57138570, -1, -69.19173671, 20.14022535),
                    (-38.61326620, -12.36459150, -1, -68.13716619, 19.07522451),
                    (-39.44231730, -13.12270450, -1, -67.63774532, 18.54339436),
                    (-40.29149370, -13.85819420, -1, -66.69710422, 17.48696655),
                    (-41.14601880, -14.55826190, -1, -66.25590107, 16.96322998),
                    (-42.01971870, -15.23423570, -1, -65.43371077, 15.93109991),
                    (-42.89771650, -15.87460040, -1, -65.05224822, 15.42335187),
                    (-43.79378720, -16.48940840, -1, -64.34999985, 14.43127782),
                    (-44.69320670, -17.06854760, -1, -64.02828719, 13.94735715),
                    (-45.60943510, -17.62069540, -1, -63.44444818, 13.01080583),
                    (-46.52817790, -18.13722310, -1, -63.18099938, 12.55833449),
                    (-47.46229760, -18.62537720, -1, -62.71108658, 11.69216083),
                    (-48.39822260, -19.07804630, -1, -62.50297087, 11.27838659),
                    (-49.34792140, -19.50103900, -1, -62.13968856, 10.49652541),
                    (-50.29884620, -19.88874270, -1, -61.98261238, 10.12817109),
                    (-51.26177390, -20.24557590, -1, -61.71605203, 9.44334746),
                    (-52.22547790, -20.56735010, -1, -61.60447077, 9.12646978),
                    (-53.19925460, -20.85719830, -1, -61.42236816, 8.54993429),
                    (-55.14903820, -21.33301120, -1, -61.31369740, 8.16131076),
                    (-57.12642670, -21.67614170, -1, -61.14801151, 7.36859978),
                    (-59.10436330, -21.88129480, -1, -61.11040865, 7.09702395),
                    (-61.09170150, -21.95000010, -1, -61.09170157, 6.82679081),
                    (-69.09170150, -21.95000000),
                    (-70.85650240, -21.89583350, -1, -69.09170157, 6.82661224),
                    (-72.61470830, -21.73393440, -1, -69.07859164, 7.03997869),
                    (-74.37258250, -21.46334100, -1, -69.05226414, 7.25420949),
                    (-76.11100650, -21.08737550, -1, -68.93575745, 7.88307914),
                    (-77.84822920, -20.60174910, -1, -68.83251839, 8.29991241),
                    (-79.55408940, -20.01522520, -1, -68.51701701, 9.31131506),
                    (-81.25691310, -19.31778780, -1, -68.29230699, 9.90838334),
                    (-82.91785410, -18.52561170, -1, -67.69729558, 11.24973207),
                    (-84.57271030, -17.62142620, -1, -67.31603093, 11.99558387),
                    (-86.17679960, -16.62982800, -1, -66.38231591, 13.59807555),
                    (-87.77049350, -15.52574900, -1, -65.82157308, 14.45438380),
                    (-89.30621860, -14.34224670, -1, -64.51612140, 16.23750983),
                    (-90.82609900, -13.04682870, -1, -63.76695136, 17.16164312),
                    (-92.28238630, -11.68021030, -1, -62.08582698, 19.03842797),
                    (-93.71648070, -10.20358790, -1, -61.15413965, 19.98622358),
                    (-95.08273960, -8.66390060, -1, -59.12349238, 21.86891554),
                    (-96.41985930, -7.01765730, -1, -58.02992253, 22.79746021),
                    (-97.68604840, -5.31618480, -1, -55.70484643, 24.60320471),
                    (-98.91586870, -3.51322060, -1, -54.48367341, 25.47351689),
                    (-100.07257620, -1.66244290, -1, -51.94506944, 27.12965488),
                    (-101.18570140, 0.28315790, -1, -50.64220986, 27.90908567),
                    (-102.22423030, 2.26962520, -1, -47.99185640, 29.35770885),
                    (-103.21224560, 4.34272340, -1, -46.66205825, 30.02191882),
                    (-104.12469830, 6.45020950, -1, -44.01622467, 31.22337659),
                    (-104.98021550, 8.63474040, -1, -42.71963943, 31.75776021),
                    (-105.75957270, 10.84761600, -1, -40.20167962, 32.69290397),
                    (-106.47626830, 13.12671920, -1, -39.00002033, 33.09332248),
                    (-107.11645860, 15.42850680, -1, -36.73243165, 33.76431826),
                    (-107.68910780, 17.78465800, -1, -35.68501252, 34.03717894),
                    (-108.18506930, 20.15815300, -1, -33.78149532, 34.46683441),
                    (-108.60957240, 22.57329830, -1, -32.94137409, 34.62839959),
                    (-108.95730340, 25.00069810, -1, -31.49944295, 34.85812128),
                    (-109.95022830, 24.99294140, 1, -109.45330304, 24.93757635),
                    (-110.36724470, 21.77386640, -1, -200.21670119, 35.05000292),
                    (-112.62058410, 19.85000010, -1, -111.64660743, 20.99077578),
                    (-121.31518270, 19.85000000),
                    (-120.77926170, 16.98410890, 1, -33.99458744, 34.69587986),
                    (-120.14845860, 14.13761180, 1, -35.54484441, 34.37948994),
                    (-119.43012200, 11.34303110, 1, -36.41910461, 34.17031840),
                    (-118.61744190, 8.57443460, 1, -38.34862817, 33.63972074),
                    (-117.72202500, 5.87088400, 1, -39.39026516, 33.31444804),
                    (-116.73305660, 3.20016180, 1, -41.60673987, 32.53790522),
                    (-115.66711490, 0.60666310, 1, -42.76656008, 32.08498128),
                    (-114.50874780, -1.94686070, 1, -45.16646984, 31.04870490),
                    (-113.27999790, -4.41211460, 1, -46.39124110, 30.46591757),
                    (-111.96038600, -6.82990880, 1, -48.86619738, 29.17538659),
                    (-110.57763830, -9.14967430, 1, -50.10189192, 28.47022882),
                    (-109.10615410, -11.41414150, 1, -52.54524508, 26.95030200),
                    (-107.57923550, -13.57223500, 1, -53.74030627, 26.13970934),
                    (-105.96641690, -15.66685030, 1, -56.05357727, 24.43342561),
                    (-104.30608960, -17.64824470, 1, -57.16200423, 23.54290871),
                    (-102.56357680, -19.55769180, 1, -59.26078055, 21.70914702),
                    (-100.78145230, -21.34859920, 1, -60.24483436, 20.77136668),
                    (-98.92191300, -23.05890450, 1, -62.06371523, 18.88143446),
                    (-97.03036840, -24.64684400, 1, -62.89607272, 17.93430852),
                    (-95.06740940, -26.14550740, 1, -64.39238367, 16.06772116),
                    (-93.07950640, -27.51935730, 1, -65.05765278, 15.15221687),
                    (-91.02756760, -28.79547720, 1, -66.21364060, 13.39188161),
                    (-88.95698340, -29.94551880, 1, -66.70908320, 12.54955966),
                    (-86.83121230, -30.98990620, 1, -67.53251692, 10.97673822),
                    (-84.69218070, -31.90786250, 1, -67.86798276, 10.24723940),
                    (-82.50828480, -32.71313710, 1, -68.39107575, 8.93656543),
                    (-80.31554890, -33.39221520, 1, -68.58813733, 8.35517049),
                    (-78.08962920, -33.95287480, 1, -68.86487146, 7.37005858),
                    (-75.85839960, -34.38782030, 1, -68.95519136, 6.96546439),
                    (-73.60677300, -34.70027320, 1, -69.05723171, 6.35421137),
                    (-71.35267890, -34.88744200, 1, -69.08022813, 6.14667378),
                    (-69.09170150, -34.94999990, 1, -69.09170384, 5.93949552),
                    (-61.09170150, -34.95000000),
                    (-58.62024450, -34.87524150, 1, -61.09170157, 5.93958461),
                    (-56.15777050, -34.65168640, 1, -61.10668021, 6.18694903),
                    (-53.69855410, -34.27837990, 1, -61.13671645, 6.43480954),
                    (-51.26589250, -33.75928840, 1, -61.26961664, 7.16225681),
                    (-48.84032300, -33.08986760, 1, -61.38714176, 7.64300045),
                    (-46.45776250, -32.28041180, 1, -61.74588770, 8.80765831),
                    (-44.08716790, -31.32005220, 1, -62.00072199, 9.49254603),
                    (-41.77436090, -30.22766160, 1, -62.67398641, 11.02644992),
                    (-39.47970390, -28.98428170, 1, -63.10399898, 11.87525875),
                    (-37.25560580, -27.61853410, 1, -64.15343970, 13.69032990),
                    (-36.15298670, -26.87961300, 1, -64.61847955, 14.40450985),
                    (-35.07007040, -26.11209770, 1, -65.29460080, 15.38511256),
                    (-33.99458940, -25.30728780, 1, -65.65848628, 15.88471444),
                    (-32.94001180, -24.47526710, 1, -66.43645921, 16.89679021),
                    (-31.89400230, -23.60630090, 1, -66.85089620, 17.40866870),
                    (-30.86997090, -22.71153170, 1, -67.72872103, 18.43862173),
                    (-29.85569930, -21.78028370, 1, -68.19212895, 18.95598346),
                    (-28.86436130, -20.82465170, 1, -69.16553270, 19.99032171),
                    (-27.88402140, -19.83313330, 1, -69.67520529, 20.50649215),
                    (-26.92745870, -18.81864980, 1, -70.73769276, 21.53207560),
                    (-25.98316670, -17.76900420, 1, -71.28983310, 22.04062392),
                    (-25.06339250, -16.69780310, 1, -72.43278957, 23.04489778),
                    (23.87998930, -16.69780310, -1, -0.59170157, -37.22943623),
                    (24.79976350, -17.76900420, 1, 71.24938643, 23.04489778),
                    (25.74405550, -18.81865010, 1, 70.10643398, 22.04061945),
                    (26.70061830, -19.83313340, 1, 69.55428543, 21.53208005),
                    (27.68095810, -20.82465170, 1, 68.49180216, 20.50649215),
                    (28.67229620, -21.78028370, 1, 67.98212956, 19.99032171),
                    (29.68656780, -22.71153170, 1, 67.00872581, 18.95598346),
                    (30.71059920, -23.60630090, 1, 66.54531789, 18.43862173),
                ]
            )
        )

    def test_regression_32677_ensure_offset_vanishes(self):
        """Regression test from issue #32677: ensure that offsetting a certain amount results in an empty shape."""
        a = self._make_32677_test_geometry()

        original_scale = area.get_clipper_scale()
        area.set_clipper_scale(1e7)
        try:
            a.Offset(-22.6)
        finally:
            area.set_clipper_scale(original_scale)

        self.assertEqual(a.getCurves(), [], format_area(a, "Expected no offset curves, got"))

    def test_regression_32677_ensure_no_spikes(self):
        """Regression test from issue #32677: offsetting must not produce out-and-back spikes."""
        # Parameters defining an unexpected spike in the output
        tol_base = 1e-6  # max spike base
        tol_height = tol_base * 10  # min spike height

        # Offsets to test. Set test_offsets = (val,) for focused debugging
        all_offsets = (-1.6, -5.1, -8.6, -12.1, -15.6, -19.1)
        test_offsets = all_offsets

        for offset in test_offsets:
            with self.subTest(offset=offset):
                a = self._make_32677_test_geometry()
                original_scale = area.get_clipper_scale()
                area.set_clipper_scale(1e7)
                try:
                    a.Offset(offset)
                finally:
                    area.set_clipper_scale(original_scale)

                # Detect spikes and label them
                labels = {}
                curves = a.getCurves()
                for curve_i, curve in enumerate(curves):
                    points = [v.p for v in curve.getVertices()]
                    n = len(points)
                    for i in range(n if curve.IsClosed() else n - 2):
                        p0 = points[i % n]
                        p1 = points[(i + 1) % n]
                        p2 = points[(i + 2) % n]

                        spike_height = math.hypot(p1.x - p0.x, p1.y - p0.y)
                        spike_base = math.hypot(p2.x - p0.x, p2.y - p0.y)
                        if spike_height > tol_height and spike_base < tol_base:
                            labels[(curve_i, (i + 1) % n)] = f"SPIKE (height {spike_height:.6f})"

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
                                line += "  <-- " + ", ".join(labels[(curve_i, j)])
                            lines.append(line)
                            summary_lines.append(f"Curve {curve_i} {line}")

                    lines += ["", f"{len(labels)} spikes found:"]
                    lines += summary_lines

                    self.fail("\n".join(lines))

        # Ensure the test doesn't pass if some offsets are disabled
        self.assertEqual(
            all_offsets,
            test_offsets,
            "test_offsets pass; restore `test_offsets = all_offsets` to allow the test to pass",
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
