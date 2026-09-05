# SPDX-License-Identifier: LGPL-2.1-or-later
"""
Unit tests for Area/Clipper operations.
These tests directly verify Area boolean and offset operations work correctly.
Created for Clipper1 to Clipper2 migration - provides safety net for changes.
"""

import unittest
import area
import math
from CAMTests.TestArcFitting import areas_equal, make_curve, make_area, format_area


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

    def create_square_reversed(self, x, y, size):
        """Helper: Create a clockwise square Area at position (x,y) with given size."""
        a = area.Area()
        c = area.Curve()
        c.append(area.Vertex(area.Point(x, y)))
        c.append(area.Vertex(area.Point(x, y + size)))
        c.append(area.Vertex(area.Point(x + size, y + size)))
        c.append(area.Vertex(area.Point(x + size, y)))
        c.append(area.Vertex(area.Point(x, y)))  # Close the curve
        a.append(c)
        return a

    def create_circle(self, cx, cy, radius):
        """Helper: Create a circular Area (approximated as polygon)."""
        a = area.Area()
        c = area.Curve()
        c.append(area.Vertex(area.Point(cx + radius, cy)))
        c.append(area.Vertex(1, area.Point(cx - radius, cy), area.Point(cx, cy)))
        c.append(area.Vertex(1, area.Point(cx + radius, cy), area.Point(cx, cy)))
        a.append(c)
        return a

    def make_vertex(self, type, p, c=(0, 0)):
        return area.Vertex(type, area.Point(*p), area.Point(*c))

    def assert_areas_equal(self, actual, expected, **kwargs):
        if not areas_equal(actual, expected, **kwargs):
            self.fail(format_area(actual, "Actual") + format_area(expected, "Expected"))

    def assertVertexEquals(self, actual, expected, approx_center=False):
        self.assertEqual(actual.type, expected.type)
        self.assertAlmostEqual(actual.p.x, expected.p.x)
        self.assertAlmostEqual(actual.p.y, expected.p.y)
        if expected.type != 0:
            if approx_center:
                self.assertAlmostEqual(actual.c.x, expected.c.x)
                self.assertAlmostEqual(actual.c.y, expected.c.y)
            else:
                self.assertEqual(actual.c.x, expected.c.x)
                self.assertEqual(actual.c.y, expected.c.y)

    def assertAreaNear(self, area_obj, expected_area, tolerance=None, msg=None):
        """Helper: Assert area is within tolerance of expected value.
        Default tolerance is 1% of expected_area.
        """
        actual = area_obj.GetArea()
        if tolerance is None:
            tolerance = abs(expected_area) * 0.01
        if msg is None:
            msg = f"Area {actual:.2f} not near expected {expected_area:.2f}" + format_area(
                area_obj, "area"
            )
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

        # Should have 1 CCW curve
        curves = a1.getCurves()
        self.assertEqual(len(curves), 1)
        self.assertFalse(curves[0].IsClockwise())

        # Check area
        self.assertAreaNear(a1, 100 + 100 - 50, msg="Union of overlapping squares")

    def test_union_separate_squares(self):
        """Test union of two non-overlapping squares."""
        a1 = self.create_square(0, 0, 10)
        a2 = self.create_square(20, 0, 10)

        a1.Union(a2)

        # Should have 2 separate curves, both CCW
        curves = a1.getCurves()
        self.assertEqual(len(curves), 2, "Union of separate squares should have 2 curves")
        self.assertFalse(curves[0].IsClockwise(), "Both curves should be counter-clockwise")
        self.assertFalse(curves[1].IsClockwise())

        # Check area
        self.assertAreaNear(a1, 100 + 100, msg="Union of separate squares")

    def test_intersect_overlapping_squares(self):
        """Test intersection of two overlapping squares."""
        a1 = self.create_square(0, 0, 10)
        a2 = self.create_square(5, 0, 10)

        a1.Intersect(a2)

        # Should have 1 CCW curve
        curves = a1.getCurves()
        self.assertEqual(len(curves), 1)
        self.assertFalse(curves[0].IsClockwise())

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

        # Should have 2 curves: outer CCW, hole CW
        curves = outer.getCurves()
        self.assertEqual(len(curves), 2, "Subtract should produce 2 curves (outer + hole)")
        self.assertFalse(curves[0].IsClockwise())
        self.assertTrue(curves[1].IsClockwise())

        # Check area
        self.assertAreaNear(outer, 20 * 20 - 10 * 10, msg="Square with hole")

    def test_subtract_complete(self):
        """Test subtracting identical square."""
        a1 = self.create_square(0, 0, 10)
        a2 = self.create_square(0, 0, 10)

        a1.Subtract(a2)

        # Should be empty (no curves to check orientation)
        self.assertEqual(a1.num_curves(), 0, "Complete subtraction should be empty")
        area_val = a1.GetArea()
        self.assertEqual(area_val, 0)

    # ========================================================================
    # Offset Operation Tests
    # ========================================================================

    def test_offset_inward(self):
        """Test offsetting a square inward."""
        a = self.create_square(0, 0, 10)
        a.Offset(-1)
        curves = a.getCurves()
        self.assertEqual(len(curves), 1, "Offset should produce single curve")
        self.assertFalse(curves[0].IsClockwise(), "Offset curve should be counter-clockwise")
        self.assertAreaNear(a, 8 * 8, msg="Offset(-1.0)")

    def test_offset_outward(self):
        """Test offsetting a square outward."""
        a = self.create_square(0, 0, 10)
        a.Offset(1)
        curves = a.getCurves()
        self.assertEqual(len(curves), 1, "Offset should produce single curve")
        self.assertFalse(curves[0].IsClockwise(), "Offset curve should be counter-clockwise")
        self.assertAreaNear(a, 12 * 12, msg="Offset(1.0)")

    def test_offset_inward_reversed(self):
        """Test offsetting a clockwise square inward."""
        a = self.create_square_reversed(0, 0, 10)
        a.Offset(-1)
        curves = a.getCurves()
        self.assertEqual(len(curves), 1, "Offset should produce single curve")
        self.assertFalse(curves[0].IsClockwise(), "Offset curve should be counter-clockwise")
        self.assertAreaNear(a, 8 * 8, msg="Offset(-1.0) on CW input")

    def test_offset_outward_reversed(self):
        """Test offsetting a clockwise square outward."""
        a = self.create_square_reversed(0, 0, 10)
        a.Offset(1)
        curves = a.getCurves()
        self.assertEqual(len(curves), 1, "Offset should produce single curve")
        self.assertFalse(curves[0].IsClockwise(), "Offset curve should be counter-clockwise")
        self.assertAreaNear(a, 12 * 12, msg="Offset(1.0) on CW input")

    def test_offset_circle(self):
        """Test offsetting a circle."""
        a = self.create_circle(0, 0, 10)
        self.assertAreaNear(a, math.pi * 10**2, msg="Original circle")

        # Offset inward by 2 (radius becomes 8)
        a.Offset(-2)

        # Should have 1 curve with at most 3 CVertex (start, most of circle,
        # rest; CVertex doesn't support full-circle arcs)
        self.assertEqual(a.num_curves(), 1, "Offset circle should 1 curve")
        self.assertLess(a.getCurves()[0].getNumVertices(), 4)
        self.assertAreaNear(a, math.pi * 8**2, msg="Offset circle")

    # ========================================================================
    # Open Offset Tests
    # ========================================================================

    def test_open_offset_l_curve(self):
        """Test open offset on an L-shaped open wire."""
        a = area.Area()
        a.append(make_curve([(0, 0), (10, 0), (10, 10)]))

        neg = a.OpenOffset(1.0)

        expected_pos = make_area(make_curve([(0, -1), (10, -1), (11, 0, 1, 10, 0), (11, 10)]))
        expected_neg = make_area(make_curve([(0, 1), (9, 1), (9, 10)]))

        self.assert_areas_equal(a, expected_pos)
        self.assert_areas_equal(neg, expected_neg)

    def test_open_offset_negative(self):
        """Test that open offset with a negative offset produces swapped positive and negative results."""
        a1 = make_area(make_curve([(0, 0), (10, 0), (10, 10)]))
        a2 = area.copy_area(a1)

        neg1 = a1.OpenOffset(1.0)
        neg2 = a2.OpenOffset(-1.0)

        # OpenOffset(-1) should swap: a2 == neg1, neg2 == a1
        self.assert_areas_equal(a2, neg1)
        self.assert_areas_equal(neg2, a1)

    def test_open_offset_direction_flip(self):
        """Test open offset on a path that causes direction flipping"""
        # Construct subject, a P-shape curve that doesn't quite close to itself
        subj = make_curve([(0, 0), (0, 40), (40, 40), (40, 20), (9, 20)])
        a = make_area(subj)

        # Offset by enough to close the gap
        neg = a.OpenOffset(5)

        expected_pos = make_area(
            [
                make_curve([(5, 0), (5, 17)]),
                make_curve([(5, 23), (5, 35), (35, 35), (35, 25), (9, 25)]),
            ]
        )
        expected_neg = make_area(
            make_curve(
                [
                    (-5, 0),
                    (-5, 40),
                    (0, 45, -1, 0, 40),
                    (40, 45),
                    (45, 40, -1, 40, 40),
                    (45, 20),
                    (40, 15, -1, 40, 20),
                    (9, 15),
                ]
            )
        )

        # Compute expected arc fitting accuracy
        # Nominal values: Radius 5, dx = 4, dy = 3 (3/4/5 right triangle)
        # Segment approximation may truncate the circle at most to radius 5 - area.get_accuracy()
        # This creates a vertical offset of the intersection with the x=5 line of area.get_accuracy() / (3/5)
        expected_accuracy = area.get_accuracy() / (3 / 5.0)

        self.assert_areas_equal(a, expected_pos, tol=expected_accuracy)
        self.assert_areas_equal(neg, expected_neg, tol=expected_accuracy)

    def test_open_offset_zigzag(self):
        """Test open offset on a path with point expansion in both directions"""
        subj = make_curve([(-3, -4), (3, 4), (-3, 12), (3, 20)])
        a = make_area(subj)
        neg = a.OpenOffset(5)

        expected_pos = make_area(
            [
                make_curve(
                    [
                        (1, -7),
                        (7, 1),
                        (7, 7, 1, 3, 4),
                        (3.25, 12),
                        (7, 17),
                    ]
                ),
            ]
        )
        expected_neg = make_area(
            make_curve(
                [
                    (-7, -1),
                    (-3.25, 4),
                    (-7, 9),
                    (-7, 15, -1, -3, 12),
                    (-1, 23),
                ]
            )
        )

        self.assert_areas_equal(a, expected_pos)
        self.assert_areas_equal(neg, expected_neg)

    def test_open_offset_antiparallel(self):
        def tan_to_x(y, to):
            """
            Returns an edge tangent to the x axis at the origin
            If abs(y) < 10, returns an arc of center (0, y)
            Else (abs(y) >= 10), returns a line instead of the "large" arc
            If `to`, returns a segment to the origin, tangent to x-
            Else returns a segment from the origin, including start point, tangent to x+
            """
            if abs(y) >= 10:
                return [(10, 0), (0, 0)] if to else [(10, 0)]

            if to:
                return [(0, 2 * y), (0, 0, 1 if y < 0 else -1, 0, y)]
            else:
                return [(0, 2 * y, 1 if y > 0 else -1, 0, y)]

        def test_join_side(y0, y1, pos_side, testName):
            with self.subTest(testName):
                a = make_area(make_curve(tan_to_x(y0, True) + tan_to_x(y1, False)))
                pos = area.copy_area(a)
                neg = pos.OpenOffset(0.1)
                debug_str = (
                    f'{format_area(a, "input")}\n{format_area(pos, "pos")}{format_area(neg, "neg")}'
                )
                self.assertEqual(len(pos.getCurves()), 1, debug_str)
                self.assertEqual(len(neg.getCurves()), 1, debug_str)
                self.assertEqual(
                    pos.getCurves()[0].getNumVertices(), 4 if pos_side else 3, debug_str
                )
                self.assertEqual(
                    neg.getCurves()[0].getNumVertices(), 3 if pos_side else 4, debug_str
                )

        test_join_side(-3, 2, False, "CCW, smaller CCW")
        test_join_side(-3, 3, False, "CCW, equal CCW")
        test_join_side(-3, 4, False, "CCW, bigger CCW")
        test_join_side(-3, 10, False, "CCW, line")
        test_join_side(-3, -4, False, "CCW, bigger CW")
        test_join_side(-3, -2, True, "CCW, smaller CW")

        test_join_side(3, -2, True, "CW, smaller CW")
        test_join_side(3, -3, True, "CW, equal CW")
        test_join_side(3, -4, True, "CW, bigger CW")
        test_join_side(3, 10, True, "CW, line")
        test_join_side(3, 4, True, "CW, bigger CCW")
        test_join_side(3, 2, False, "CW, smaller CCW")

    def test_open_offset_parallel(self):
        def tan_to_x(y, to):
            """
            Returns an edge tangent to the negative x axis at the origin
            If abs(y) < 10, returns an arc of center (0, y)
            Else (abs(y) >= 10), returns a line instead of the "large" arc
            If `to`, returns a segment to the origin
            Else returns a segment from the origin, including start point
            """
            if abs(y) >= 10:
                return [(10, 0), (0, 0)] if to else [(-10, 0)]

            if to:
                return [(0, 2 * y), (0, 0, 1 if y < 0 else -1, 0, y)]
            else:
                return [(0, 2 * y, -1 if y > 0 else 1, 0, y)]

        def test_no_join(y0, y1, testName):
            with self.subTest(testName):
                a = make_area(make_curve(tan_to_x(y0, True) + tan_to_x(y1, False)))
                pos = area.copy_area(a)
                neg = pos.OpenOffset(0.1)
                debug_str = (
                    f'{format_area(a, "input")}\n{format_area(pos, "pos")}{format_area(neg, "neg")}'
                )
                self.assertEqual(len(pos.getCurves()), 1, debug_str)
                self.assertEqual(len(neg.getCurves()), 1, debug_str)
                self.assertEqual(pos.getCurves()[0].getNumVertices(), 3, debug_str)
                self.assertEqual(neg.getCurves()[0].getNumVertices(), 3, debug_str)

        test_no_join(-3, 2, "CCW, smaller CW")
        test_no_join(-3, 3, "CCW, equal CW")
        test_no_join(-3, 4, "CCW, bigger CW")
        test_no_join(-3, 10, "CCW, line")
        test_no_join(-3, -4, "CCW, bigger CCW")
        test_no_join(-3, -2, "CCW, smaller CCW")

        test_no_join(3, -2, "CW, smaller CCW")
        test_no_join(3, -3, "CW, equal CCW")
        test_no_join(3, -4, "CW, bigger CCW")
        test_no_join(3, 10, "CW, line")
        test_no_join(3, 4, "CW, bigger CW")
        test_no_join(3, 2, "CW, smaller CW")

    def test_open_offset_arcs(self):
        """Test open offset on a path with arcs"""
        subj = make_curve(
            [
                (0, 0),
                (0, 6, 1, 0, 3),
                (0, 12, 1, 0, 9),
                (0, 18, -1, 0, 15),
                (0, 24, -1, 0, 21),
                (0, 30, 1, 0, 27),
            ]
        )
        a = make_area(subj)
        neg = a.OpenOffset(2)

        expected_pos = make_area(
            [
                make_curve(
                    [
                        (0, -2),
                        (4, 6, 1, 0, 3),
                        (0, 14, 1, 0, 9),
                        (0, 16, -1, 0, 15),
                        (0, 20, 1, 0, 18),
                        (0, 22, -1, 0, 21),
                        (0, 32, 1, 0, 27),
                    ]
                ),
            ]
        )
        expected_neg = make_area(
            make_curve(
                [
                    (0, 2),
                    (0, 4, 1, 0, 3),
                    (0, 8, -1, 0, 6),
                    (0, 10, 1, 0, 9),
                    (-4, 18, -1, 0, 15),
                    (0, 26, -1, 0, 21),
                    (0, 28, 1, 0, 27),
                ]
            )
        )

        expected_accuracy = area.get_accuracy() / (3 / 5.0)

        self.assert_areas_equal(a, expected_pos, tol=expected_accuracy)
        self.assert_areas_equal(neg, expected_neg, tol=expected_accuracy)

    def test_open_offset_colinear(self):
        """Test that colinear points are not removed while offsetting"""
        a = make_area(make_curve([(0, 0), (0, 1), (0, 2)]))
        neg = a.OpenOffset(1)

        expected_pos = make_area(make_curve([(1, 0), (1, 1), (1, 2)]))
        expected_neg = make_area(make_curve([(-1, 0), (-1, 1), (-1, 2)]))

        self.assert_areas_equal(a, expected_pos)
        self.assert_areas_equal(neg, expected_neg)

    def test_open_offset_collapse_u_curve(self):
        a = make_area(make_curve([(0, 0), (0, -10), (3, -10), (3, 0)]))
        neg = a.OpenOffset(2)
        self.assertEqual(neg.num_curves(), 0)

    def test_open_offset_collapse_o_curve(self):
        a = make_area(make_curve([(0, 0), (0, -10), (3, -10), (3, 0), (0, 0)]))
        neg = a.OpenOffset(2)
        self.assertEqual(neg.num_curves(), 0)

    # ========================================================================
    # Geometry Manipulation Tests
    # ========================================================================

    def test_thicken_closed(self):
        """Test thickening a closed area."""
        a = self.create_square(0, 0, 10)

        # Thicken adds material
        a.Thicken(2.0)

        # Should have result
        curves = a.getCurves()
        self.assertEqual(len(curves), 2, "Thicken should produce 2 curves (outer + hole)")

        # First curve is the outer boundary (CCW)
        self.assertFalse(curves[0].IsClockwise(), "Outer curve should be counter-clockwise")

        # Second curve is the inner hole (CW)
        self.assertTrue(curves[1].IsClockwise(), "Inner hole should be clockwise")

        # Check area
        corners = 4 * (2 * 2 - math.pi * 2 * 2 / 4)
        self.assertAreaNear(a, 14 * 14 - 6 * 6 - corners, msg="Square offset both ways")

    def test_thicken_open(self):
        """Test thickening an open path."""
        x = 5
        r = 2
        a = make_area(make_curve([(0, 0), (x, 0)]))

        a.Thicken(r)

        curves = a.getCurves()
        self.assertEqual(len(curves), 1, format_area(a, "result"))
        self.assertFalse(curves[0].IsClockwise(), format_area(a, "result"))

        expected = x * (2 * r) + math.pi * r**2
        self.assertAreaNear(a, expected)

    def test_reorder(self):
        """Test Reorder doesn't break the area."""
        a = self.create_square(0, 0, 10)
        original_area = a.GetArea()

        # Reorder should not change area
        a.Reorder()

        self.assertAreaNear(a, original_area, msg="Reorder")

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
