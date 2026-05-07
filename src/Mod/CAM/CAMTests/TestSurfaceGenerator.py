# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 Dimitris75 <dimitriospana75@gmail.com>             *
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


import unittest
import Part
import FreeCAD
import math

# Attempt to import the compiled C++ module. This is the System Under Test (SUT).
try:
    import surface_generator as surface_cpp
except ImportError:
    surface_cpp = None


# The decorator will skip all tests in this class if the C++ module is not available.
@unittest.skipIf(
    surface_cpp is None, "The 'surface_generator' C++ module is not available to test."
)
class TestSurfaceGenerator(unittest.TestCase):

    def setUp(self):
        """
        This method is called before each test. We define common test geometries here
        to avoid duplicating code in every test function.
        """
        # A simple 10x10 square boundary, centered at (5,5)
        self.square_boundary = [[(0, 0), (10, 0), (10, 10), (0, 10), (0, 0)]]

        # A more complex boundary: a 20x20 square with a 5x5 square hole in the middle.
        self.boundary_with_hole = [
            [(-10, -10), (10, -10), (10, 10), (-10, 10), (-10, -10)],  # Outer ring
            [
                (-2.5, -2.5),
                (2.5, -2.5),
                (2.5, 2.5),
                (-2.5, 2.5),
                (-2.5, -2.5),
            ],  # Inner hole (wound the same way)
        ]

    # --- Tests for shape_tessellate_fast ---

    def test00(self):
        """Ensures the C++ tessellator can process a simple Part.Shape."""
        box = Part.makeBox(10, 10, 10)
        verts, faces = surface_cpp.shape_tessellate_fast(box, 0.1, 0.5)

        self.assertIsNotNone(verts, "Vertices list should not be None")
        self.assertIsNotNone(faces, "Faces list should not be None")
        self.assertGreater(len(verts), 0, "Tessellator should produce vertices.")
        self.assertEqual(len(faces), 12, "A simple box should always produce 12 triangles.")

    def test10(self):
        """Verifies that a finer deflection setting produces more triangles."""
        cyl = Part.makeCylinder(5, 20)

        # Coarse settings
        verts_coarse, faces_coarse = surface_cpp.shape_tessellate_fast(cyl, 1.0, 15.0)

        # Fine settings
        verts_fine, faces_fine = surface_cpp.shape_tessellate_fast(cyl, 0.01, 1.0)

        self.assertGreater(
            len(faces_fine), len(faces_coarse), "Finer deflection should result in more faces."
        )

    # --- Tests for generate_linear_pattern_cpp ---

    def test20(self):
        """Tests if the C++ line clipper correctly clips a simple horizontal line."""
        # Generate one horizontal line at Y=5.0, angle=0
        endpoints = surface_cpp.generate_linear_pattern_cpp(
            0,
            10,
            0,
            10,  # BBox
            100.0,
            0.0,  # Use a large stepover to get only one line
            False,
            False,  # is_zigzag, reversed
            self.square_boundary,
        )

        self.assertEqual(len(endpoints), 1, "Should produce exactly one clipped segment.")
        self.assertEqual(len(endpoints[0]), 2, "Segment should have a start and end point.")

        start_point, end_point = endpoints[0]
        self.assertAlmostEqual(
            start_point[0], 0.0, places=2, msg="Start X should be on the boundary edge."
        )
        self.assertAlmostEqual(
            end_point[0], 10.0, places=2, msg="End X should be on the boundary edge."
        )
        self.assertAlmostEqual(
            start_point[1], 5.0, places=2, msg="Line should be at the correct Y height."
        )

    def test30(self):
        """Tests the clipper with a 45-degree angled line."""
        endpoints = surface_cpp.generate_linear_pattern_cpp(
            0, 10, 0, 10, 100.0, 45.0, False, False, self.square_boundary
        )
        self.assertEqual(len(endpoints), 1)

        start_point, end_point = endpoints[0]
        # We are asserting that the actual length is within 0.01mm of the ideal length.
        self.assertAlmostEqual(
            math.hypot(end_point[0] - start_point[0], end_point[1] - start_point[1]),
            math.sqrt(200),
            delta=0.01,  # Use a direct delta tolerance
            msg="Length should be very close to the diagonal of a 10x10 box.",
        )

    def test40(self):
        """Verifies that the clipper correctly splits a line when it crosses a hole."""
        endpoints = surface_cpp.generate_linear_pattern_cpp(
            -10, 10, -10, 10, 100.0, 0.0, False, False, self.boundary_with_hole
        )

        # A horizontal line at Y=0 should be split into two segments by the hole
        self.assertEqual(len(endpoints), 2, "Line should be split into two segments by the hole.")

        # Check the coordinates of the first segment
        self.assertAlmostEqual(endpoints[0][0][0], -10.0, places=2)
        self.assertAlmostEqual(endpoints[0][1][0], -2.5, places=2)

        # Check the coordinates of the second segment
        self.assertAlmostEqual(endpoints[1][0][0], 2.5, places=2)
        self.assertAlmostEqual(endpoints[1][1][0], 10.0, places=2)

    def test50(self):
        """Ensures no lines are returned if the pattern is outside the boundary."""
        # Use a bounding box that is far away from the boundary
        endpoints = surface_cpp.generate_linear_pattern_cpp(
            100, 110, 100, 110, 1.0, 0.0, False, False, self.square_boundary
        )
        self.assertEqual(len(endpoints), 0, "Should return an empty list if no intersection.")

    # --- Tests for Radial Patterns (Simplified) ---

    def test60(self):
        """Tests that the spiral generator produces points without a boundary."""
        # Pass an empty list for the boundary polygons
        scan_lines = surface_cpp.generate_spiral_pattern_cpp(
            0,
            10,
            0,
            10,
            5.0,
            5.0,  # BBox, Center
            1.0,
            0.1,
            False,
            [],  # stepover, sample, reversed, empty boundary
        )
        self.assertGreater(len(scan_lines), 0, "Spiral should generate lines.")
        self.assertGreater(len(scan_lines[0]), 10, "Spiral should have multiple points.")

    def test70(self):
        """Tests that the spiral generator respects the boundary."""
        scan_lines = surface_cpp.generate_spiral_pattern_cpp(
            0, 10, 0, 10, 5.0, 5.0, 1.0, 0.1, False, self.square_boundary
        )
        self.assertGreater(len(scan_lines), 0, "Clipped spiral should generate lines.")

        # Check if all points are within the boundary
        for line in scan_lines:
            for point in line:
                self.assertTrue(
                    0.0 <= point[0] <= 10.0 and 0.0 <= point[1] <= 10.0,
                    f"Point {point} is outside the 10x10 boundary.",
                )


if __name__ == "__main__":
    unittest.main()
