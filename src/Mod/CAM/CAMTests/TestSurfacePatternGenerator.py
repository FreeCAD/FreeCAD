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


import math
import Path
import Part
import FreeCAD
import unittest
import CAMTests.PathTestUtils as PathTestUtils

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())

# Check for optional C++ dependency
try:
    from Path.Base.Generator.surface_pattern import _pattern_cpp

    _HAS_CPP = True
except ImportError:
    _HAS_CPP = False


class MockFeature:
    """A mock object to simulate a FreeCAD document object for testing Base property."""

    def __init__(self, shape, label="Mock"):
        self.Shape = shape
        self.Label = label

    def getElement(self, subname):
        return self.Shape.getElement(subname)


class TestSurfacePattern(PathTestUtils.PathTestBase):
    """Tests for surface_pattern: feature selection, path reconstruction, and pattern generation."""

    def setUp(self):
        """Create common test geometry."""
        # A simple square face for general pattern testing
        self.square_face = Part.makeFace(
            Part.makePolygon(
                [
                    FreeCAD.Vector(0, 0, 0),
                    FreeCAD.Vector(20, 0, 0),
                    FreeCAD.Vector(20, 20, 0),
                    FreeCAD.Vector(0, 20, 0),
                    FreeCAD.Vector(0, 0, 0),
                ]
            )
        )
        # A more complex face with a hole for offset testing
        outer = Part.makePolygon(
            [
                FreeCAD.Vector(0, 0, 0),
                FreeCAD.Vector(40, 0, 0),
                FreeCAD.Vector(40, 40, 0),
                FreeCAD.Vector(0, 40, 0),
                FreeCAD.Vector(0, 0, 0),
            ]
        )
        inner = Part.makePolygon(
            [
                FreeCAD.Vector(10, 10, 0),
                FreeCAD.Vector(30, 10, 0),
                FreeCAD.Vector(30, 30, 0),
                FreeCAD.Vector(10, 30, 0),
                FreeCAD.Vector(10, 10, 0),
            ]
        )
        self.face_with_hole = Part.makeFace([outer, inner])

    # -- Feature Selection and Grouping Tests --

    def test00_split_features(self):
        """
        Tests the splitting of selected faces into 'cutting' and 'avoid' groups.

        INPUT:
        - Function: split_selected_features()
        - Parameters: A mock Base property and an avoid_count of 2.
        - Input data: A simulated user selection of 5 faces.

        EXPECTED OUTPUT:
        - Returns two lists: `cutting_faces` with 3 faces and `avoid_faces` with 2.
        - This logic allows users to easily designate check surfaces from their selection.
        """
        from Path.Base.Generator.surface_pattern import split_selected_features

        # Create a compound of 5 distinct faces to simulate a selection
        faces = []
        for i in range(5):
            face = Part.makePlane(10, 10, FreeCAD.Vector(i * 15, 0, 0))
            faces.append(face)
        compound_shape = Part.Compound(faces)

        # Create a list of subnames (e.g., 'Face1', 'Face2', etc.)
        subnames = [f"Face{i+1}" for i in range(len(compound_shape.Faces))]

        mock_obj = MockFeature(compound_shape)
        mock_base_prop = [(mock_obj, subnames)]

        cutting, avoid = split_selected_features(mock_base_prop, avoid_count=2)

        self.assertEqual(len(cutting), 3, "Expected 3 faces in the cutting list")
        self.assertEqual(len(avoid), 2, "Expected 2 faces in the avoid list")

    def test01_group_features(self):
        """
        Tests the grouping of features for 'Collectively' vs 'Individually' processing.

        INPUT:
        - Function: group_features()
        - Parameters: A list of 3 faces, tested with both "Collectively" and "Individually".
        - Input data: A simple list of shape objects.

        EXPECTED OUTPUT:
        - "Collectively": Returns a list containing one list `[[f1, f2, f3]]`.
        - "Individually": Returns a list of lists `[[f1], [f2], [f3]]`.
        - This controls the main loop of the SurfacePattern strategy.
        """
        from Path.Base.Generator.surface_pattern import group_features

        faces = [
            Part.makePlane(10, 10, FreeCAD.Vector(0, 0, 0)),
            Part.makePlane(10, 10, FreeCAD.Vector(15, 0, 0)),
            Part.makePlane(10, 10, FreeCAD.Vector(30, 0, 0)),
        ]

        collective = group_features(faces, "Collectively")
        self.assertEqual(len(collective), 1, "Collective mode should produce one group")
        self.assertEqual(len(collective[0]), 3)

        individual = group_features(faces, "Individually")
        self.assertEqual(len(individual), 3, "Individual mode should produce multiple groups")
        self.assertEqual(len(individual[0]), 1)

    # -- Path Reconstruction Test --

    def test02_reconstruct_lines(self):
        """
        Tests reconstruction of a flat point list back into gapped scan lines.

        INPUT:
        - Function: reconstruct_scan_lines()
        - Parameters: A list of points representing two separate lines, with a large gap.
        - Input data: A continuous stream of points as returned by OCL.

        EXPECTED OUTPUT:
        - Returns a nested list containing two separate line lists.
        - This function is crucial for correctly handling rapids between cutting areas.
        """
        from Path.Base.Generator.surface_pattern import reconstruct_scan_lines

        flat_points = [
            (0, 0, 10),
            (1, 0, 10),
            (2, 0, 10),  # Line 1
            (100, 50, 10),
            (101, 50, 10),  # Line 2 (after a big jump)
        ]
        gap_threshold = 10.0  # A gap larger than this starts a new line

        lines = reconstruct_scan_lines(flat_points, gap_threshold)

        self.assertEqual(len(lines), 2, "Expected the flat list to be split into two lines")
        self.assertEqual(len(lines[0]), 3)
        self.assertEqual(len(lines[1]), 2)

    # -- Pattern Generation Tests --

    def test10_generate_offset_pattern(self):
        """
        Tests the pure Python 'Offset' pattern generator.

        INPUT:
        - Function: generate_offset_scan_lines()
        - Parameters: A 20x20 square face with a 4mm stepover.
        - Input data: A simple boundary face.

        EXPECTED OUTPUT:
        - Returns a list of concentric, shrinking scan lines.
        - For a 20x20 area with a 4mm stepover, it should produce 3 offset rings
          (at offsets 0, -4, -8).
        - This pattern is generated using the robust ClipperLib engine.
        """
        from Path.Base.Generator.surface_pattern import generate_offset_scan_lines

        # tool_diam=4.0 was implicit before; pass explicitly now.
        lines = generate_offset_scan_lines(
            self.square_face, stepover=4.0, tool_diam=4.0, sample_interval=1.0
        )

        self.assertIsNotNone(lines)
        # 20mm width, tool radius implicitly half of stepover.
        # Center is at 10. (10 - 2) / 4 = 2 steps in. Plus the original boundary. Total 3.
        self.assertEqual(len(lines), 3, "Expected 3 concentric offset lines")
        # Check that paths are getting shorter (shrinking inwards)
        self.assertGreater(len(lines[0]), len(lines[1]))
        self.assertGreater(len(lines[1]), len(lines[2]))

    def test11_generate_offset_with_hole(self):
        """
        Tests the 'Offset' pattern generator on a shape with an internal boundary (a hole).

        INPUT:
        - Function: generate_offset_scan_lines()
        - Parameters: A 40x40 face with a 20x20 hole, 5mm stepover.
        - Input data: A complex boundary face.

        EXPECTED OUTPUT:
        - Returns offset lines for both the outer and inner boundaries.
        - The number of generated lines should be greater than for a simple square,
          as both boundaries are being offset simultaneously.
        """
        from Path.Base.Generator.surface_pattern import generate_offset_scan_lines

        lines = generate_offset_scan_lines(
            self.face_with_hole, stepover=5.0, tool_diam=5.0, sample_interval=1.0
        )
        self.assertIsNotNone(lines)
        # Outer: 40x40 -> 2 passes. Inner: 20x20 -> 1 pass. Total 3 wires * 2 passes/wire = 6 lines
        self.assertGreaterEqual(
            len(lines), 3, "Expected at least 3 offset rings for a shape with a hole"
        )

    @unittest.skipUnless(_HAS_CPP, "C++ surface_generator module not available")
    def test20_fast_generate_linear(self):
        """
        Tests the C++ bridge for generating a linear (Line/ZigZag) pattern.

        INPUT:
        - Function: fast_generate_pattern()
        - Parameters: "Line" pattern, 20x20 boundary, 5mm stepover.
        - Input data: Standard pattern parameters.

        EXPECTED OUTPUT:
        - Returns a list of clipped line endpoints.
        - Should produce 3 parallel lines across the 20mm boundary with a 5mm stepover.
        - This verifies the Python-to-C++ data bridge is working correctly.
        """
        from Path.Base.Generator.surface_pattern import fast_generate_pattern, BBox

        scan_bb = BBox.from_bbox(self.square_face.BoundBox)

        lines = fast_generate_pattern(
            pattern_type="Line",
            bbox=scan_bb,
            center=(0, 0),
            stepover=5.0,
            sample_interval=1.0,
            angle=0.0,
            is_zigzag=False,
            reversed_pattern=False,
            boundary_face=self.square_face,
        )

        self.assertEqual(len(lines), 4, "Expected 4 scan lines for a 20mm area with 5mm stepover")
        # Check that the line is clipped correctly to the 0-20 X boundary, allowing for a small tolerance
        self.assertAlmostEqual(lines[0][0][0], 0.0, delta=0.01)
        self.assertAlmostEqual(lines[0][1][0], 20.0, delta=0.01)

    @unittest.skipUnless(_HAS_CPP, "C++ surface_generator module not available")
    def test21_fast_generate_spiral(self):
        """
        Tests the C++ bridge for generating a radial (Spiral) pattern.

        INPUT:
        - Function: fast_generate_pattern()
        - Parameters: "Spiral" pattern, 20x20 boundary.
        - Input data: Standard pattern parameters.

        EXPECTED OUTPUT:
        - Returns a list containing one continuous scan line.
        - The scan line should contain many discretized points forming a spiral.
        - All points in the spiral must be inside the 20x20 boundary.
        """
        from Path.Base.Generator.surface_pattern import fast_generate_pattern, BBox

        scan_bb = BBox.from_bbox(self.square_face.BoundBox)
        center = scan_bb.center

        lines = fast_generate_pattern(
            pattern_type="Spiral",
            bbox=scan_bb,
            center=center,
            stepover=2.0,
            sample_interval=0.5,
            angle=0.0,
            is_zigzag=False,
            reversed_pattern=False,
            boundary_face=self.square_face,
        )

        self.assertGreaterEqual(
            len(lines), 1, "Spiral pattern should produce at least one path segment"
        )

        total_points = sum(len(line) for line in lines)
        self.assertGreater(total_points, 50, "Spiral path should be discretized into many points")

        # Verify all points are clipped within the boundary
        for line in lines:
            for x, y, z in line:
                self.assertTrue(
                    0 <= x <= 20 and 0 <= y <= 20, f"Point ({x},{y}) is outside the boundary"
                )
