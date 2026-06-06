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

    def test_wall_from_issue_29701_left_align(self):
        """Regression test for a sketch-based left-aligned wall that previously truncated."""
        operation = "Checking Arch Wall left-align regression from issue 29701..."
        self.printTestMessage(operation)

        point_data = [
            (0.0, 0.0),
            (9842.5, 0.0),
            (9842.5, -393.7),
            (9740.9, -393.7),
            (9740.9, -3657.6),
            (13004.8, -3657.6),
            (13004.8, -393.7),
            (12903.2, -393.7),
            (12903.2, 0.0),
            (17278.35, 0.0),
            (17278.35, 3873.5),
            (17179.925, 3873.5),
            (17179.925, 4267.2),
            (17278.35, 4267.2),
            (17278.35, 7737.475),
            (10369.55, 7737.475),
            (10369.55, 6924.675),
            (10077.45, 6924.675),
            (10077.45, 7318.375),
            (10175.875, 7318.375),
            (10175.875, 11191.875),
            (5908.675, 11191.875),
            (5908.675, 11801.475),
            (4079.875, 11801.475),
            (4079.875, 10556.875),
            (3787.775, 10556.875),
            (3787.775, 10950.575),
            (3886.2, 10950.575),
            (3886.2, 14030.637),
            (3095.937, 14820.9),
            (787.4, 14820.9),
            (0.0, 14033.5),
            (0.0, 10972.8),
            (98.425, 10972.8),
            (98.425, 10579.1),
            (0.0, 10579.1),
            (0.0, 8515.35),
            (98.425, 8515.35),
            (98.425, 8121.65),
            (0.0, 8121.65),
            (0.0, 5683.25),
            (98.425, 5683.25),
            (98.425, 5289.55),
            (0.0, 5289.55),
        ]
        points = [App.Vector(x, y, 0) for x, y in point_data]
        sketch = self.document.addObject("Sketcher::SketchObject", "Issue29701Sketch")
        sketch.addGeometry(
            [Part.LineSegment(start, end) for start, end in zip(points, points[1:] + points[:1])]
        )
        self.document.recompute()

        wall = Arch.makeWall(sketch, width=193.675, height=2641.6, align="Left")
        self.document.recompute()

        self.assertTrue(wall.Shape.isValid(), "The wall shape should be valid.")
        self.assertEqual(len(wall.Shape.Solids), 1, "The wall should produce one solid.")
        self.assertGreater(
            wall.Shape.BoundBox.XLength,
            17000.0,
            "The left-aligned wall should span the full sketch width.",
        )
        self.assertGreater(
            wall.Shape.BoundBox.YLength,
            18000.0,
            "The left-aligned wall should span the full sketch height.",
        )
        self.assertGreater(
            wall.Shape.Volume,
            3.8e10,
            "The left-aligned wall volume should not collapse after binding the segments.",
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
        Tests that removing a wall's base using removeComponents(host=None)
        does not crash and successfully unlinks the base.
        This is the non-regression test for the 'list' has no attribute 'Base' bug.
        https://github.com/FreeCAD/FreeCAD/issues/24532
        """
        self.printTestMessage("Testing removal of a wall's base component...")

        # 1. Arrange: Create a wall with a base
        line = Draft.makeLine(App.Vector(0, 0, 0), App.Vector(2000, 0, 0))
        wall = Arch.makeWall(line)
        self.assertIsNotNone(wall.Base, "Pre-condition failed: Wall should have a base.")

        # 2. Act: Call removeComponents on the base, simulating the failing workflow
        # Before the fix, this will raise an AttributeError.
        # After the fix, it should complete without error.
        Arch.removeComponents([wall.Base])
        self.document.recompute()

        # 3. Assert: The base should now be None
        self.assertIsNone(wall.Base, "The wall's Base property was not cleared after removal.")
