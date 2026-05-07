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
Unit tests for arc fitting/unfitting operations.
Tests verify that FitArcs/UnfitArcs preserve geometry through round-trips
and that operations (offset, boolean) handle arcs correctly.
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


class TestArcFittingRoundTrip(PathTestBase):
    """Tests for FitArcs and UnfitArcs round-trip operations."""

    def assert_curve_unchanged_by_roundtrip(self, c):
        """Helper: Assert that UnFitArcs->FitArcs doesn't change the given Curve."""
        # Store original vertices
        orig = list(c.getVertices())
        c.UnFitArcs()
        c.FitArcs(False)
        result = list(c.getVertices())

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
        self.assert_curve_unchanged_by_roundtrip(c)

    def test_single_vertex_roundtrip(self):
        """Test that round-trip preserves a single-vertex curve."""
        c = make_curve([(1, 1)])
        self.assert_curve_unchanged_by_roundtrip(c)

    def test_line_roundtrip(self):
        """Test that round-trip preserves a simple line."""
        c = make_curve([(0, 0), (10, 10)])
        self.assert_curve_unchanged_by_roundtrip(c)

    def test_small_arc_ccw_roundtrip(self):
        """Test that round-trip preserves a 90-degree CCW arc."""
        c = make_curve([(10, 0), (0, 10, 1, 0, 0)])
        self.assert_curve_unchanged_by_roundtrip(c)

    def test_small_arc_cw_roundtrip(self):
        """Test that round-trip preserves a 90-degree CW arc."""
        c = make_curve([(10, 0), (0, -10, -1, 0, 0)])
        self.assert_curve_unchanged_by_roundtrip(c)

    def test_180_arc_ccw_roundtrip(self):
        """Test that round-trip preserves a 180-degree CCW arc."""
        c = make_curve([(10, 0), (-10, 0, 1, 0, 0)])
        self.assert_curve_unchanged_by_roundtrip(c)

    def test_180_arc_cw_roundtrip(self):
        """Test that round-trip preserves a 180-degree CW arc."""
        c = make_curve([(-10, 0), (10, 0, -1, 0, 0)])
        self.assert_curve_unchanged_by_roundtrip(c)

    def test_closed_line_before_arc_ccw_roundtrip(self):
        """Test closed curve: line then CCW arc back to start."""
        c = make_curve([(0, 0), (10, 0), (0, 0, 1, 5, 0)])
        self.assert_curve_unchanged_by_roundtrip(c)

    def test_closed_line_before_arc_cw_roundtrip(self):
        """Test closed curve: line then CW arc back to start."""
        c = make_curve([(0, 0), (10, 0), (0, 0, -1, 5, 0)])
        self.assert_curve_unchanged_by_roundtrip(c)

    def test_closed_line_after_arc_ccw_roundtrip(self):
        """Test closed curve: CCW arc then line back to start."""
        c = make_curve([(0, 0), (10, 0, 1, 5, 0), (0, 0)])
        self.assert_curve_unchanged_by_roundtrip(c)

    def test_closed_line_after_arc_cw_roundtrip(self):
        """Test closed curve: CW arc then line back to start."""
        c = make_curve([(0, 0), (10, 0, -1, 5, 0), (0, 0)])
        self.assert_curve_unchanged_by_roundtrip(c)

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
                    self.assert_curve_unchanged_by_roundtrip(c)
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
        self, a, offset_distance, expected_curves, expected_lines, expected_arcs
    ):
        """Helper: Assert that offsetting an area produces expected line and arc counts.

        Args:
            a: Area object to offset
            offset_distance: Distance to offset (positive = outward for CCW curves)
            expected_curves: Expected number of curves after offset
            expected_lines: Expected total number of line segments across all curves
            expected_arcs: Expected total number of arc segments across all curves
        """
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

        a.OffsetWithClipper(offset_distance)

        # Get the resulting curves
        curves = a.getCurves()

        # Build detailed output for debugging
        result_summary = []
        total_lines = 0
        total_arcs = 0

        for i, curve in enumerate(curves):
            vertices = list(curve.getVertices())
            lines = [v for v in vertices[1:] if v.type == 0]
            arcs = [v for v in vertices[1:] if v.type != 0]
            total_lines += len(lines)
            total_arcs += len(arcs)

            result_summary.append(
                f"  Curve {i}: {len(vertices)} vertices ({len(lines)} lines, {len(arcs)} arcs)"
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
        if total_lines != expected_lines or total_arcs != expected_arcs:
            self.fail(
                f"Expected {expected_lines} lines and {expected_arcs} arcs, "
                f"got {total_lines} lines and {total_arcs} arcs\n"
                f"Offset distance: {offset_distance}\n"
                f"Original curves:\n" + "\n".join(orig_summary) + "\n"
                f"Result curves:\n" + "\n".join(result_summary)
            )

    def test_square_offset_outward(self):
        """Test that offsetting a square outward produces 4 lines and 4 arcs."""
        a = make_area(make_curve([(0, 0), (10, 0), (10, 10), (0, 10), (0, 0)]))
        self.assert_offset_line_and_arc_count(a, 1.0, 1, 4, 4)  # 1 curve, 4 lines, 4 arcs

    def test_square_offset_inward(self):
        """Test that offsetting a square inward produces 4 lines and no arcs."""
        a = make_area(make_curve([(0, 0), (10, 0), (10, 10), (0, 10), (0, 0)]))
        self.assert_offset_line_and_arc_count(a, -1.0, 1, 4, 0)  # 1 curve, 4 lines, 0 arcs

    def test_square_offset_inward_collapse(self):
        """Test that offsetting a square inward past its half-width produces no curves."""
        a = make_area(make_curve([(0, 0), (10, 0), (10, 10), (0, 10), (0, 0)]))
        self.assert_offset_line_and_arc_count(a, -6.0, 0, 0, 0)  # 0 curves

    def test_square_with_semicircle_top_offset_outward(self):
        """Test offsetting a square with semicircular top outward."""
        a = make_area(make_curve([(0, 0), (10, 0), (10, 10), (0, 10, 1, 5, 10), (0, 0)]))
        self.assert_offset_line_and_arc_count(a, 1.0, 1, 3, 3)  # 1 curve, 3 lines, 3 arcs

    def test_square_with_semicircle_top_offset_inward(self):
        """Test offsetting a square with semicircular top inward."""
        a = make_area(make_curve([(0, 0), (10, 0), (10, 10), (0, 10, 1, 5, 10), (0, 0)]))
        self.assert_offset_line_and_arc_count(a, -1.0, 1, 3, 1)  # 1 curve, 3 lines, 1 arc

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
        self.assert_offset_line_and_arc_count(a, 1.0, 2, 7, 7)  # 2 curves, 7 lines, 7 arcs

        # Offset by 2.0 (merges --> +1 line, +1 arc)
        self.assert_offset_line_and_arc_count(a2, 2.0, 1, 8, 8)  # 1 curve, 8 lines, 8 arcs


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
        self, a1, a2, operation, expected_curves, expected_lines, expected_arcs
    ):
        """Helper: Perform boolean operation and assert line and arc counts.

        Args:
            a1: First area object
            a2: Second area object
            operation: Boolean operation name ("Union", "Subtract", "Intersect", "Xor")
            expected_curves: Expected number of curves after operation
            expected_lines: Expected total number of line segments across all curves
            expected_arcs: Expected total number of arc segments across all curves
        """
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
        total_arcs = 0

        for i, curve in enumerate(curves):
            vertices = list(curve.getVertices())
            lines = [v for v in vertices[1:] if v.type == 0]
            arcs = [v for v in vertices[1:] if v.type != 0]
            total_lines += len(lines)
            total_arcs += len(arcs)

            result_summary.append(
                f"  Curve {i}: {len(vertices)} vertices ({len(lines)} lines, {len(arcs)} arcs)"
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
        if total_lines != expected_lines or total_arcs != expected_arcs:
            self.fail(
                f"Expected {expected_lines} lines and {expected_arcs} arcs after {operation}, "
                f"got {total_lines} lines and {total_arcs} arcs\n"
                f"Input area 1:\n" + "\n".join(input1_summary) + "\n"
                f"Input area 2:\n" + "\n".join(input2_summary) + "\n"
                f"Result curves:\n" + "\n".join(result_summary)
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
        self.assert_boolean_line_and_arc_count(self.square, self.semicircle, "Subtract", 1, 5, 1)


if __name__ == "__main__":
    unittest.main()
