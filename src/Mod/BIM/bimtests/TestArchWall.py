# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2013 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

# Unit tests for the Arch wall module

import os
import Arch
import Draft
import Part
import FreeCAD as App
from bimtests import TestArchBase


class TestArchWall(TestArchBase.TestArchBase):

    def testWall(self):
        operation = "Checking Arch Wall..."
        self.printTestMessage(operation)

        l = Draft.makeLine(App.Vector(0, 0, 0), App.Vector(-2, 0, 0))
        w = Arch.makeWall(l)
        self.assertTrue(w, "Arch Wall failed")

    def testWallMultiMatAlign(self):
        operation = "Checking Arch Wall with MultiMaterial and 3 alignments..."
        self.printTestMessage(operation)

        matA = Arch.makeMaterial()
        matB = Arch.makeMaterial()
        matMulti = Arch.makeMultiMaterial()
        matMulti.Materials = [matA, matB]
        matMulti.Thicknesses = [100, 200]  # total width different from default 200
        pts = [
            App.Vector(0, 0, 0),
            App.Vector(1000, 0, 0),
            App.Vector(1000, 1000, 0),
            App.Vector(2000, 1000, 0),
        ]
        # wall based on wire:
        wire = Draft.makeWire(pts)
        wallWire = Arch.makeWall(wire)
        wallWire.Material = matMulti
        # wall based on sketch:
        sketch = App.activeDocument().addObject("Sketcher::SketchObject", "Sketch")
        sketch.addGeometry(
            [
                Part.LineSegment(pts[0], pts[1]),
                Part.LineSegment(pts[1], pts[2]),
                Part.LineSegment(pts[2], pts[3]),
            ]
        )
        wallSketch = Arch.makeWall(sketch)
        wallSketch.Material = matMulti

        alignLst = ["Left", "Center", "Right"]
        checkLst = [
            [App.Vector(0, -300, 0), App.Vector(2000, 1000, 0)],
            [App.Vector(0, -150, 0), App.Vector(2000, 1150, 0)],
            [App.Vector(0, 0, 0), App.Vector(2000, 1300, 0)],
        ]
        for i in range(3):
            wallWire.Align = alignLst[i]
            wallSketch.Align = alignLst[i]
            App.ActiveDocument.recompute()
            for box in [wallWire.Shape.BoundBox, wallSketch.Shape.BoundBox]:
                ptMin = App.Vector(box.XMin, box.YMin, 0)
                self.assertTrue(
                    ptMin.isEqual(checkLst[i][0], 1e-8),
                    "Arch Wall with MultiMaterial and 3 alignments failed",
                )
                ptMax = App.Vector(box.XMax, box.YMax, 0)
                self.assertTrue(
                    ptMax.isEqual(checkLst[i][1], 1e-8),
                    "Arch Wall with MultiMaterial and 3 alignments failed",
                )

    def test_makeWall(self):
        """Test the makeWall function."""
        operation = "Testing makeWall function"
        self.printTestMessage(operation)

        wall = Arch.makeWall(length=5000, width=200, height=3000)
        self.assertIsNotNone(wall, "makeWall failed to create a wall object.")
        self.assertEqual(wall.Label, "Wall", "Wall label is incorrect.")

    def test_joinWalls(self):
        """Test the joinWalls function."""
        operation = "Testing joinWalls function"
        self.printTestMessage(operation)

        base_line1 = Draft.makeLine(App.Vector(0, 0, 0), App.Vector(5000, 0, 0))
        base_line2 = Draft.makeLine(App.Vector(5000, 0, 0), App.Vector(5000, 3000, 0))
        wall1 = Arch.makeWall(base_line1, width=200, height=3000)
        wall2 = Arch.makeWall(base_line2, width=200, height=3000)
        joined_wall = Arch.joinWalls([wall1, wall2])
        self.assertIsNotNone(joined_wall, "joinWalls failed to join walls.")

    def test_remove_base_from_wall_without_host(self):
        """
        Tests that removing a debasable wall's base using removeComponents
        successfully unlinks the base.
        """
        self.printTestMessage("Testing removal of a wall's base component...")

        # 1. Arrange: Create a wall with a base
        line = Draft.makeLine(App.Vector(0, 0, 0), App.Vector(2000, 0, 0))
        # Ensure the base object's shape is computed, making the wall debasable.
        line.recompute()
        wall = Arch.makeWall(line)
        self.document.recompute()  # Ensure wall is fully formed
        self.assertIsNotNone(wall.Base, "Pre-condition failed: Wall should have a base.")
        self.assertTrue(
            Arch.is_debasable(wall), "Pre-condition failed: The test wall is not debasable."
        )

        # 2. Act: Call removeComponents on the base.
        # This will trigger the is_debasable -> True -> debaseWall() path.
        Arch.removeComponents([wall.Base])
        self.document.recompute()

        # 3. Assert: The base should now be None because debaseWall was successful.
        self.assertIsNone(wall.Base, "The wall's Base property was not cleared after removal.")

    def test_is_debasable_with_valid_line_base(self):
        """Tests that a wall based on a single Draft.Line is debasable."""
        self.printTestMessage("Checking is_debasable with Draft.Line...")
        line = Draft.makeLine(App.Vector(0, 0, 0), App.Vector(1000, 0, 0))
        line.recompute()
        wall = Arch.makeWall(line)
        self.document.recompute()
        self.assertTrue(Arch.is_debasable(wall), "Wall on Draft.Line should be debasable.")

    def test_is_debasable_with_valid_sketch_base(self):
        """Tests that a wall based on a Sketch with a single line is debasable."""
        self.printTestMessage("Checking is_debasable with single-line Sketch...")
        sketch = self.document.addObject("Sketcher::SketchObject", "SingleLineSketch")
        sketch.addGeometry(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(1000, 0, 0)))
        self.document.recompute()
        wall = Arch.makeWall(sketch)
        self.assertTrue(Arch.is_debasable(wall), "Wall on single-line Sketch should be debasable.")

    def test_is_debasable_with_multi_edge_base(self):
        """Tests that a wall based on a multi-segment wire is not debasable."""
        self.printTestMessage("Checking is_debasable with multi-segment Wire...")
        wire = Draft.makeWire(
            [App.Vector(0, 0, 0), App.Vector(1000, 0, 0), App.Vector(1000, 1000, 0)]
        )
        wall = Arch.makeWall(wire)
        self.assertFalse(
            Arch.is_debasable(wall), "Wall on multi-segment wire should not be debasable."
        )

    def test_is_debasable_with_curved_base(self):
        """Tests that a wall based on an arc is not debasable."""
        self.printTestMessage("Checking is_debasable with curved base...")
        arc = Draft.make_circle(radius=500, startangle=0, endangle=90)
        self.document.recompute()
        wall = Arch.makeWall(arc)
        self.document.recompute()
        self.assertFalse(Arch.is_debasable(wall), "Wall on curved base should not be debasable.")

    def test_is_debasable_with_no_base(self):
        """Tests that a baseless wall is not debasable."""
        self.printTestMessage("Checking is_debasable with no base...")
        wall = Arch.makeWall(length=1000)
        self.assertFalse(Arch.is_debasable(wall), "Baseless wall should not be debasable.")

    def test_debase_wall_preserves_global_position(self):
        """
        Tests that debaseWall correctly transfers the base's placement to the
        wall, preserving its global position and dimensions.
        """
        self.printTestMessage("Checking debaseWall preserves global position...")

        # 1. Arrange: Create a rotated and translated line, and a wall from it.
        pl = App.Placement(App.Vector(1000, 500, 200), App.Rotation(App.Vector(0, 0, 1), 45))
        line = Draft.makeLine(App.Vector(0, 0, 0), App.Vector(2000, 0, 0))
        line.Placement = pl
        line.recompute()  # Use object-level recompute

        wall = Arch.makeWall(line, width=200, height=1500, align="Left")
        self.document.recompute()

        # Store the wall's original state
        original_bb = wall.Shape.BoundBox
        original_volume = wall.Shape.Volume
        original_length = wall.Length.Value

        # 2. Act: Debase the wall
        success = Arch.debaseWall(wall)
        self.document.recompute()

        # 3. Assert
        self.assertTrue(success, "debaseWall should return True for a valid wall.")
        self.assertIsNone(wall.Base, "Wall's Base should be None after debasing.")

        # Core assertions for preserving geometry and placement
        self.assertAlmostEqual(
            original_volume,
            wall.Shape.Volume,
            delta=1e-6,
            msg="Wall volume should not change after debasing.",
        )

        # Compare individual properties of the BoundBox with a tolerance
        final_bb = wall.Shape.BoundBox
        self.assertAlmostEqual(
            original_bb.XMin, final_bb.XMin, delta=1e-6, msg="Bounding box XMin does not match."
        )
        self.assertAlmostEqual(
            original_bb.XMax, final_bb.XMax, delta=1e-6, msg="Bounding box XMax does not match."
        )
        self.assertAlmostEqual(
            original_bb.YMin, final_bb.YMin, delta=1e-6, msg="Bounding box YMin does not match."
        )
        self.assertAlmostEqual(
            original_bb.YMax, final_bb.YMax, delta=1e-6, msg="Bounding box YMax does not match."
        )
        self.assertAlmostEqual(
            original_bb.ZMin, final_bb.ZMin, delta=1e-6, msg="Bounding box ZMin does not match."
        )
        self.assertAlmostEqual(
            original_bb.ZMax, final_bb.ZMax, delta=1e-6, msg="Bounding box ZMax does not match."
        )

        # Check parametric integrity
        self.assertAlmostEqual(
            wall.Length.Value,
            original_length,
            delta=1e-6,
            msg="Wall's Length property should be preserved.",
        )

        # Verify it remains parametric by changing a property
        wall.Height = 2000
        self.document.recompute()
        self.assertNotAlmostEqual(
            original_volume,
            wall.Shape.Volume,
            delta=1e-6,
            msg="Wall should remain parametric and its volume should change with height.",
        )
