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


class TestArcFitting(PathTestBase):
    """Tests for FitArcs and UnfitArcs operations."""

    def make_curve(self, vertices):
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
        c = self.make_curve([(1, 1)])
        self.assert_curve_unchanged_by_roundtrip(c)

    def test_line_roundtrip(self):
        """Test that round-trip preserves a simple line."""
        c = self.make_curve([(0, 0), (10, 10)])
        self.assert_curve_unchanged_by_roundtrip(c)

    def test_small_arc_ccw_roundtrip(self):
        """Test that round-trip preserves a 90-degree CCW arc."""
        c = self.make_curve([(10, 0), (0, 10, 1, 0, 0)])
        self.assert_curve_unchanged_by_roundtrip(c)

    def test_small_arc_cw_roundtrip(self):
        """Test that round-trip preserves a 90-degree CW arc."""
        c = self.make_curve([(10, 0), (0, -10, -1, 0, 0)])
        self.assert_curve_unchanged_by_roundtrip(c)

    def test_180_arc_ccw_roundtrip(self):
        """Test that round-trip preserves a 180-degree CCW arc."""
        c = self.make_curve([(10, 0), (-10, 0, 1, 0, 0)])
        self.assert_curve_unchanged_by_roundtrip(c)

    def test_180_arc_cw_roundtrip(self):
        """Test that round-trip preserves a 180-degree CW arc."""
        c = self.make_curve([(-10, 0), (10, 0, -1, 0, 0)])
        self.assert_curve_unchanged_by_roundtrip(c)

    def test_closed_line_before_arc_ccw_roundtrip(self):
        """Test closed curve: line then CCW arc back to start."""
        c = self.make_curve([(0, 0), (10, 0), (0, 0, 1, 5, 0)])
        self.assert_curve_unchanged_by_roundtrip(c)

    def test_closed_line_before_arc_cw_roundtrip(self):
        """Test closed curve: line then CW arc back to start."""
        c = self.make_curve([(0, 0), (10, 0), (0, 0, -1, 5, 0)])
        self.assert_curve_unchanged_by_roundtrip(c)

    def test_closed_line_after_arc_ccw_roundtrip(self):
        """Test closed curve: CCW arc then line back to start."""
        c = self.make_curve([(0, 0), (10, 0, 1, 5, 0), (0, 0)])
        self.assert_curve_unchanged_by_roundtrip(c)

    def test_closed_line_after_arc_cw_roundtrip(self):
        """Test closed curve: CW arc then line back to start."""
        c = self.make_curve([(0, 0), (10, 0, -1, 5, 0), (0, 0)])
        self.assert_curve_unchanged_by_roundtrip(c)

    def make_regular_polygon(self, num_sides, radius, subdivisions=1):
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

        return self.make_curve(vertices)

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
                    c = self.make_regular_polygon(num_sides, radius, subdivisions)
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


if __name__ == "__main__":
    unittest.main()
