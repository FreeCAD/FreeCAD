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

        # Compare exhaustively
        self.assertEqual(
            len(orig), len(result), f"Curve should have same number of vertices after round-trip"
        )

        for vert_idx, (orig_v, result_v) in enumerate(zip(orig, result)):
            self.assertEqual(orig_v.type, result_v.type, f"Vertex {vert_idx}: type mismatch")
            self.assertEqual(orig_v.p, result_v.p, f"Vertex {vert_idx}: p mismatch")
            self.assertEqual(orig_v.c, result_v.c, f"Vertex {vert_idx}: c mismatch")

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


if __name__ == "__main__":
    unittest.main()
