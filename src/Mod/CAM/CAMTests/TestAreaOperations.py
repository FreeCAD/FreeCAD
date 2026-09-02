# SPDX-License-Identifier: LGPL-2.1-or-later
"""
Unit tests for Area/Clipper operations.
These tests directly verify Area boolean and offset operations work correctly.
Created for Clipper1 to Clipper2 migration - provides safety net for changes.

NOTE: Only tests methods that are exposed to Python via pybind11.
C++-only methods (Xor, Clip, OffsetWithClipper) are not tested here.
"""

import unittest
import area
import math


class TestAreaOperations(unittest.TestCase):
    """Direct tests for Area boolean and offset operations exposed to Python."""

    def create_square(self, x, y, size):
        """Helper: Create a square Area at position (x,y) with given size."""
        a = area.Area()
        c = area.Curve()
        c.append(area.Vertex(area.Point(x, y)))
        c.append(area.Vertex(area.Point(x + size, y)))
        c.append(area.Vertex(area.Point(x + size, y + size)))
        c.append(area.Vertex(area.Point(x, y + size)))
        c.append(area.Vertex(area.Point(x, y)))  # Close the curve
        a.append(c)
        return a

    def create_circle(self, cx, cy, radius, segments=16):
        """Helper: Create a circular Area (approximated as polygon)."""
        a = area.Area()
        c = area.Curve()
        for i in range(segments):
            angle = 2 * math.pi * i / segments
            x = cx + radius * math.cos(angle)
            y = cy + radius * math.sin(angle)
            c.append(area.Vertex(area.Point(x, y)))
        # Close the curve
        c.append(area.Vertex(area.Point(cx + radius, cy)))
        a.append(c)
        return a

    def assertAreaNear(self, area_obj, expected_area, tolerance=None, msg=None):
        """Helper: Assert area is within tolerance of expected value.
        Default tolerance is 1% of expected_area.
        """
        actual = abs(area_obj.GetArea())  # Note: GetArea() not Area()
        if tolerance is None:
            tolerance = abs(expected_area) * 0.01
        if msg is None:
            msg = f"Area {actual:.2f} not near expected {expected_area:.2f}"
        self.assertAlmostEqual(actual, expected_area, delta=tolerance, msg=msg)

    # ========================================================================
    # Boolean Operation Tests
    # ========================================================================

    def test_union_overlapping_squares(self):
        """Test union of two overlapping squares."""
        # Two 10x10 squares with 5-unit overlap
        a1 = self.create_square(0, 0, 10)
        a2 = self.create_square(5, 0, 10)

        a1.Union(a2)

        # Should have result
        self.assertGreater(a1.num_curves(), 0, "Union should produce curves")

        # Check unioned area
        self.assertAreaNear(a1, 100 + 100 - 50, msg="Union of overlapping squares")

    def test_union_separate_squares(self):
        """Test union of two non-overlapping squares."""
        a1 = self.create_square(0, 0, 10)
        a2 = self.create_square(20, 0, 10)

        a1.Union(a2)

        # Should have 2 separate curves
        self.assertEqual(a1.num_curves(), 2, "Union of separate squares should have 2 curves")

        # Check area
        self.assertAreaNear(a1, 100 + 100, msg="Union of separate squares")

    def test_intersect_overlapping_squares(self):
        """Test intersection of two overlapping squares."""
        a1 = self.create_square(0, 0, 10)
        a2 = self.create_square(5, 0, 10)

        a1.Intersect(a2)

        # Should have result
        self.assertGreater(a1.num_curves(), 0, "Intersect should produce curves")

        # Check area
        self.assertAreaNear(a1, 5 * 10, msg="Intersection area")

    def test_intersect_no_overlap(self):
        """Test intersection of two non-overlapping squares (edge case)."""
        a1 = self.create_square(0, 0, 10)
        a2 = self.create_square(20, 0, 10)

        a1.Intersect(a2)

        # Should be empty (no overlap)
        self.assertEqual(a1.num_curves(), 0, "Intersect of non-overlapping should be empty")

    def test_subtract_hole(self):
        """Test subtracting small square from large square (creates hole)."""
        outer = self.create_square(0, 0, 20)
        hole = self.create_square(5, 5, 10)

        outer.Subtract(hole)

        # Should have curves (outer boundary + hole)
        self.assertGreater(outer.num_curves(), 0, "Subtract should produce curves")

        # Check area
        self.assertAreaNear(outer, 20 * 20 - 10 * 10, msg="Square with hole")

    def test_subtract_complete(self):
        """Test subtracting identical square."""
        a1 = self.create_square(0, 0, 10)
        a2 = self.create_square(0, 0, 10)

        a1.Subtract(a2)

        # Should be empty
        area_val = abs(a1.GetArea())
        self.assertEqual(area_val, 0)

    # ========================================================================
    # Offset Operation Tests
    # ========================================================================

    def test_offset_inward(self):
        """Test offsetting a square inward."""
        a = self.create_square(0, 0, 10)

        # Offset inward by 1
        a.Offset(1.0)

        # Should have result
        self.assertGreater(a.num_curves(), 0, "Offset should produce curves")

        # Check area
        self.assertAreaNear(a, 8 * 8, msg="Inward offset by 1")

    def test_offset_outward(self):
        """Test offsetting a square outward (negative offset)."""
        a = self.create_square(0, 0, 10)

        # Offset outward by -1 (negative means outward in Area convention)
        a.Offset(-1.0)

        # Should have result
        self.assertGreater(a.num_curves(), 0, "Offset should produce curves")

        # Check area
        self.assertAreaNear(a, 12 * 12, msg="Outward offset by -1")

    def test_offset_circle(self):
        """Test offsetting a circle."""
        # Create circle with radius 10
        a = self.create_circle(0, 0, 10, segments=32)
        self.assertAreaNear(a, math.pi * 10**2, msg="Original circle")

        # Offset inward by 2 (radius becomes 8)
        a.Offset(2.0)

        # Should have result
        self.assertGreater(a.num_curves(), 0, "Offset circle should produce curves")

        # Check area
        self.assertAreaNear(a, math.pi * 8**2, msg="Offset circle")

    # ========================================================================
    # Geometry Manipulation Tests
    # ========================================================================

    def test_thicken(self):
        """Test thickening an area."""
        a = self.create_square(0, 0, 10)

        # Thicken adds material
        a.Thicken(2.0)

        # Should have result
        self.assertGreater(a.num_curves(), 0, "Thicken should produce curves")

        # Check area
        corners = 4 * (2 * 2 - math.pi * 2 * 2 / 4)
        self.assertAreaNear(a, 14 * 14 - 6 * 6 - corners, msg="Square offset both ways")

    def test_reorder(self):
        """Test Reorder doesn't break the area."""
        a = self.create_square(0, 0, 10)
        original_area = abs(a.GetArea())

        # Reorder should not change area
        a.Reorder()

        self.assertAreaNear(a, original_area, msg="Reorder")

    def test_fitarcs(self):
        """Test FitArcs doesn't break the area."""
        a = self.create_circle(0, 0, 10, segments=16)
        original_area = abs(a.GetArea())
        original_curves = a.num_curves()

        # FitArcs might change representation but not area significantly
        a.FitArcs()

        self.assertEqual(a.num_curves(), original_curves, "FitArcs should maintain curve count")
        # Check area
        self.assertAreaNear(a, original_area, msg="FitArcs")

    def test_multiple_holes(self):
        """Test square with multiple holes."""
        # Large outer square
        outer = self.create_square(0, 0, 30)

        # Multiple holes
        hole1 = self.create_square(5, 5, 5)
        hole2 = self.create_square(20, 5, 5)
        hole3 = self.create_square(12, 15, 5)

        outer.Subtract(hole1)
        outer.Subtract(hole2)
        outer.Subtract(hole3)

        # Should have multiple curves (outer + 3 holes)
        self.assertGreater(outer.num_curves(), 0, "Complex shape should have curves")

        # Area should be 900 - 3*25 = 825
        self.assertAreaNear(outer, 30 * 30 - 3 * 5 * 5, msg="Square with 3 holes")

    def test_getcurves(self):
        """Test getCurves method returns curve list."""
        a = self.create_square(0, 0, 10)

        curves = a.getCurves()

        self.assertIsInstance(curves, list, "getCurves should return list")
        self.assertEqual(len(curves), 1, "Square should have 1 curve")

    def test_nearestpoint(self):
        """Test NearestPoint method."""
        a = self.create_square(0, 0, 10)

        # Point outside square
        test_point = area.Point(15, 15)
        nearest = a.NearestPoint(test_point)

        # Should return a Point
        self.assertIsInstance(nearest, area.Point, "NearestPoint should return Point")

        # Nearest point should be on the square (10,10)
        self.assertEqual(nearest.x, 10, "Nearest x should be on the corner of the square")
        self.assertEqual(nearest.y, 10, "Nearest y should be on the corner of the square")


if __name__ == "__main__":
    # Allow running this test file directly
    unittest.main()
