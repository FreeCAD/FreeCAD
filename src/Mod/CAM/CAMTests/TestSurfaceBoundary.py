#!/usr/bin/env python3

"""Unit tests for Surface boundary creation logic."""

import unittest
import sys
import os
import time

# Add the CAM module path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "..", "src", "Mod", "CAM"))

import Part
import FreeCAD
import Path.Base.Generator.surface_common as surface_common


class TestSurfaceBoundary(unittest.TestCase):
    """Test boundary creation logic for Surface operation."""

    def setUp(self):
        """Set up test fixtures."""
        # Create test geometry - a simple rectangular face
        self.test_face = Part.makeFace(
            [
                Part.makePolygon(
                    [
                        FreeCAD.Vector(0, 0, 0),
                        FreeCAD.Vector(10, 0, 0),
                        FreeCAD.Vector(10, 10, 0),
                        FreeCAD.Vector(0, 10, 0),
                        FreeCAD.Vector(0, 0, 0),
                    ]
                )
            ]
        )

    def test_boundary_creation_basic(self):
        """Test basic boundary creation with TechDraw + libarea."""
        tool_radius = 2.5
        boundary_adjustment = 0.0

        boundary_face = surface_common.make_boundary_face(
            [self.test_face], tool_radius, boundary_adjustment
        )

        # Verify boundary face was created
        self.assertIsNotNone(boundary_face)
        self.assertEqual(boundary_face.ShapeType, "Face")

        # Verify boundary is larger than original face (due to tool offset)
        original_bb = self.test_face.BoundBox
        boundary_bb = boundary_face.BoundBox

        # Boundary should be expanded by tool_radius in all directions
        self.assertLess(boundary_bb.XMin, original_bb.XMin)
        self.assertGreater(boundary_bb.XMax, original_bb.XMax)
        self.assertLess(boundary_bb.YMin, original_bb.YMin)
        self.assertGreater(boundary_bb.YMax, original_bb.YMax)

    def test_boundary_creation_with_adjustment(self):
        """Test boundary creation with boundary adjustment."""
        tool_radius = 2.5
        boundary_adjustment = 1.0  # Additional offset

        boundary_face = surface_common.make_boundary_face(
            [self.test_face], tool_radius, boundary_adjustment
        )

        # Verify boundary face was created
        self.assertIsNotNone(boundary_face)

        # Verify boundary accounts for both tool radius and adjustment
        original_bb = self.test_face.BoundBox
        boundary_bb = boundary_face.BoundBox

        # Boundary should be expanded by tool_radius + adjustment
        expected_offset = tool_radius + boundary_adjustment
        self.assertAlmostEqual(boundary_bb.XMin, original_bb.XMin - expected_offset, places=1)
        self.assertAlmostEqual(boundary_bb.XMax, original_bb.XMax + expected_offset, places=1)
        self.assertAlmostEqual(boundary_bb.YMin, original_bb.YMin - expected_offset, places=1)
        self.assertAlmostEqual(boundary_bb.YMax, original_bb.YMax + expected_offset, places=1)

    def test_boundary_creation_no_faces(self):
        """Test boundary creation with no faces returns None."""
        boundary_face = surface_common.make_boundary_face([], 2.5, 0.0)
        self.assertIsNone(boundary_face)


if __name__ == "__main__":
    unittest.main()
