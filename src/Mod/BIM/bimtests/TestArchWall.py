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
import tempfile
import Arch
import ArchWallEndCondition
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

    def test_makeWall_baseless_alignment(self):
        """
        Tests that Arch.makeWall correctly creates a baseless wall with the
        specified alignment.
        """
        self.printTestMessage("Checking baseless wall alignment from makeWall...")

        # Define the test cases: (Alignment Mode, Expected final Y-center)
        test_cases = [
            ("Center", 0.0),
            ("Left", -100.0),
            ("Right", 100.0),
        ]

        for align_mode, expected_y_center in test_cases:
            with self.subTest(alignment=align_mode):
                # 1. Arrange & Act: Create a baseless wall using the API call.
                wall = Arch.makeWall(length=2000, width=200, height=1500, align=align_mode)
                self.document.recompute()

                # 2. Assert Geometry: Verify the shape is valid.
                self.assertFalse(
                    wall.Shape.isNull(), msg=f"[{align_mode}] Shape should not be null."
                )
                expected_volume = 2000 * 200 * 1500
                self.assertAlmostEqual(
                    wall.Shape.Volume,
                    expected_volume,
                    delta=1e-6,
                    msg=f"[{align_mode}] Wall volume is incorrect.",
                )

                # 3. Assert Placement and Alignment.
                # The wall's Placement should be at the origin.
                self.assertTrue(
                    wall.Placement.Base.isEqual(App.Vector(0, 0, 0), 1e-6),
                    msg=f"[{align_mode}] Default placement Base should be at the origin.",
                )
                self.assertAlmostEqual(
                    wall.Placement.Rotation.Angle,
                    0.0,
                    delta=1e-6,
                    msg=f"[{align_mode}] Default placement Rotation should be zero.",
                )

                # The shape's center should be offset according to the alignment.
                shape_center = wall.Shape.BoundBox.Center
                expected_center = App.Vector(0, expected_y_center, 750)

                self.assertTrue(
                    shape_center.isEqual(expected_center, 1e-5),
                    msg=f"For '{align_mode}' align, wall center {shape_center} does not match expected {expected_center}",
                )

    def test_baseless_wall_stretch_api(self):
        """
        Tests the proxy methods for graphically editing baseless walls:
        calc_endpoints() and set_from_endpoints().
        """
        self.printTestMessage("Checking baseless wall stretch API...")

        # 1. Arrange: Create a baseless wall and then set its placement.
        initial_placement = App.Placement(
            App.Vector(1000, 1000, 0), App.Rotation(App.Vector(0, 0, 1), 45)
        )
        # Create wall first, then set its placement.
        wall = Arch.makeWall(length=2000)
        wall.Placement = initial_placement
        self.document.recompute()

        # 2. Test calc_endpoints()
        endpoints = wall.Proxy.calc_endpoints(wall)
        self.assertEqual(len(endpoints), 2, "calc_endpoints should return two points.")

        # Verify the calculated endpoints against manual calculation
        half_len_vec_x = App.Vector(1000, 0, 0)
        rotated_half_vec = initial_placement.Rotation.multVec(half_len_vec_x)
        expected_p1 = initial_placement.Base - rotated_half_vec
        expected_p2 = initial_placement.Base + rotated_half_vec

        self.assertTrue(endpoints[0].isEqual(expected_p1, 1e-6), "Start point is incorrect.")
        self.assertTrue(endpoints[1].isEqual(expected_p2, 1e-6), "End point is incorrect.")

        # 3. Test set_from_endpoints()
        new_p1 = App.Vector(0, 0, 0)
        new_p2 = App.Vector(4000, 0, 0)
        wall.Proxy.set_from_endpoints(wall, [new_p1, new_p2])
        self.document.recompute()

        # Assert that the wall's properties have been updated correctly
        self.assertAlmostEqual(
            wall.Length.Value, 4000.0, delta=1e-6, msg="Length was not updated correctly."
        )

        expected_center = App.Vector(2000, 0, 0)
        self.assertTrue(
            wall.Placement.Base.isEqual(expected_center, 1e-6),
            "Placement.Base (center) was not updated correctly.",
        )

        # Check rotation (should now be zero as the new points are on the X-axis)
        self.assertAlmostEqual(
            wall.Placement.Rotation.Angle,
            0.0,
            delta=1e-6,
            msg="Placement.Rotation was not updated correctly.",
        )

    def test_based_wall_stretch_api_debases_straight_wall(self):
        """Straight line base walls should join the endpoint-editing API."""
        self.printTestMessage("Checking based wall stretch API on straight base...")

        base = Draft.makeLine(App.Vector(100, 200, 0), App.Vector(2100, 200, 0))
        wall = Arch.makeWall(base, width=200, height=1500)
        self.document.recompute()

        wall.Proxy.set_from_endpoints(wall, [App.Vector(0, 0, 0), App.Vector(4000, 0, 0)])
        self.document.recompute()

        self.assertIsNone(wall.Base, "Straight base wall should debase on first endpoint edit.")
        self.assertAlmostEqual(wall.Length.Value, 4000.0, delta=1e-6)
        self.assertTrue(wall.Placement.Base.isEqual(App.Vector(2000, 0, 0), 1e-6))

    def test_based_wall_stretch_rejects_coincident_endpoints_without_mutation(self):
        """Invalid endpoint edits must not debase or otherwise change a wall."""
        base = Draft.makeLine(App.Vector(100, 200, 0), App.Vector(2100, 200, 0))
        wall = Arch.makeWall(base, width=200, height=1500)
        self.document.recompute()
        original_base = wall.Base
        original_length = wall.Length.Value
        original_placement = wall.Placement.copy()

        with self.assertRaises(ValueError):
            wall.Proxy.set_from_endpoints(wall, [App.Vector(0, 0, 0), App.Vector(0, 0, 0)])

        self.assertIs(wall.Base, original_base)
        self.assertAlmostEqual(wall.Length.Value, original_length, delta=1e-6)
        self.assertTrue(wall.Placement.Base.isEqual(original_placement.Base, 1e-6))

    def test_get_global_baseline_for_based_wall(self):
        """Tests that get_global_baseline returns based-wall geometry."""
        self.printTestMessage("Checking get_global_baseline for based walls...")

        line = Draft.makeLine(App.Vector(100, 200, 0), App.Vector(2100, 200, 0))
        self.document.recompute()
        wall = Arch.makeWall(line, width=200, height=1500)
        self.document.recompute()

        baseline = wall.Proxy.get_global_baseline(wall)
        self.assertIsNotNone(baseline, "Based wall baseline should not be None.")
        self.assertTrue(baseline.normal.isEqual(App.Vector(0, 0, 1), 1e-6))

        expected_start = line.Start
        expected_end = line.End
        self.assertTrue(
            baseline.start_point.isEqual(expected_start, 1e-6),
            "Baseline start point does not match the base shape.",
        )
        self.assertTrue(
            baseline.end_point.isEqual(expected_end, 1e-6),
            "Baseline end point does not match the base shape.",
        )

    def test_get_global_baseline_preserves_provider_orientation(self):
        """Draft lines and sketches define stable Start/End semantics."""
        line = Draft.makeLine(App.Vector(1000, 0, 0), App.Vector(0, 0, 0))
        wall_from_line = Arch.makeWall(line, width=200, height=1500)

        sketch = self.document.addObject("Sketcher::SketchObject", "ReversedWallSketch")
        sketch.addGeometry(Part.LineSegment(App.Vector(1000, 200, 0), App.Vector(0, 200, 0)))
        wall_from_sketch = Arch.makeWall(sketch, width=200, height=1500)
        self.document.recompute()

        line_baseline = wall_from_line.Proxy.get_global_baseline(wall_from_line)
        sketch_baseline = wall_from_sketch.Proxy.get_global_baseline(wall_from_sketch)
        self.assertTrue(line_baseline.start_point.isEqual(line.Start, 1e-6))
        self.assertTrue(line_baseline.end_point.isEqual(line.End, 1e-6))
        self.assertTrue(sketch_baseline.start_point.isEqual(App.Vector(1000, 200, 0), 1e-6))
        self.assertTrue(sketch_baseline.end_point.isEqual(App.Vector(0, 200, 0), 1e-6))

    def test_get_global_baseline_matches_transformed_wall_geometry(self):
        """Baseline coordinates include base, wall, and explicit normal transforms."""
        base_placement = App.Placement(
            App.Vector(100, 500, 20), App.Rotation(App.Vector(0, 0, 1), 45)
        )
        wall_placement = App.Placement(
            App.Vector(300, -200, 40), App.Rotation(App.Vector(0, 0, 1), 30)
        )
        line = Draft.makeLine(App.Vector(0, 0, 0), App.Vector(2000, 0, 0))
        line.Placement = base_placement
        wall = Arch.makeWall(line, width=200, height=1500)
        wall.Placement = wall_placement
        wall.Normal = App.Vector(0, 1, 1)
        self.document.recompute()

        baseline = wall.Proxy.get_global_baseline(wall)
        expected_start = wall_placement.multVec(line.Start)
        expected_end = wall_placement.multVec(line.End)
        expected_normal = wall_placement.Rotation.multVec(wall.Normal)
        expected_normal.normalize()
        self.assertTrue(baseline.start_point.isEqual(expected_start, 1e-6))
        self.assertTrue(baseline.end_point.isEqual(expected_end, 1e-6))
        self.assertTrue(baseline.normal.isEqual(expected_normal, 1e-6))
        expected_edge = Part.makeLine(expected_start, expected_end)
        self.assertAlmostEqual(wall.Shape.distToShape(expected_edge)[0], 0.0, delta=1e-6)
        self.assertTrue(wall.Shape.BoundBox.isValid())

    def test_get_global_baseline_preserves_orientation_after_round_trip(self):
        """Provider endpoint orientation survives document serialization."""
        line = Draft.makeLine(App.Vector(1000, 0, 0), App.Vector(0, 0, 0))
        wall = Arch.makeWall(line, width=200, height=1500)
        self.document.recompute()
        wall_name = wall.Name

        fd, path = tempfile.mkstemp(suffix=".FCStd")
        os.close(fd)
        try:
            document_name = self.document.Name
            self.document.saveAs(path)
            App.closeDocument(document_name)
            self.document = App.openDocument(path)
            self.document.recompute()
            restored_wall = self.document.getObject(wall_name)
            baseline = restored_wall.Proxy.get_global_baseline(restored_wall)
            self.assertTrue(baseline.start_point.isEqual(App.Vector(1000, 0, 0), 1e-6))
            self.assertTrue(baseline.end_point.isEqual(App.Vector(0, 0, 0), 1e-6))
        finally:
            if self.document:
                App.closeDocument(self.document.Name)
                self.document = None
            if os.path.exists(path):
                os.unlink(path)

    def test_get_global_baseline_rejects_unsupported_based_walls(self):
        """Multi-edge and curved wall bases are not join baselines."""
        wire = Draft.makeWire(
            [App.Vector(0, 0, 0), App.Vector(1000, 0, 0), App.Vector(1000, 1000, 0)]
        )
        self.document.recompute()
        wire_wall = Arch.makeWall(wire, width=200, height=1500)
        self.document.recompute()
        self.assertIsNone(wire_wall.Proxy.get_global_baseline(wire_wall))

        arc = Draft.make_circle(radius=500, startangle=0, endangle=90)
        self.document.recompute()
        curved_wall = Arch.makeWall(arc, width=200, height=1500)
        self.document.recompute()
        self.assertIsNone(curved_wall.Proxy.get_global_baseline(curved_wall))

    def test_resolved_section_matches_visible_shape_bounds(self):
        """Invisible material layers move the cursor but never become faces."""
        material_a = Arch.makeMaterial()
        material_b = Arch.makeMaterial()
        material = Arch.makeMultiMaterial()
        material.Materials = [material_a, material_b]
        material.Thicknesses = [100, -50]

        wall = Arch.makeWall(length=2000, width=150, height=1000, align="Center")
        wall.Material = material
        self.document.recompute()

        section = wall.Proxy.get_resolved_section(wall)
        self.assertEqual([layer.raw_thickness for layer in section.layers], [100, -50])
        self.assertAlmostEqual(wall.Shape.BoundBox.YMin, section.y_min, delta=1e-6)
        self.assertAlmostEqual(wall.Shape.BoundBox.YMax, section.y_max, delta=1e-6)

    def test_resolved_section_applies_wall_overrides_and_defaults(self):
        """Short overrides resolve each segment against the wall defaults."""
        wire = Draft.makeWire(
            [App.Vector(0, 0, 0), App.Vector(1000, 0, 0), App.Vector(1000, 1000, 0)]
        )
        wall = Arch.makeWall(wire, width=100, height=1000, align="Center", offset=10)
        wall.OverrideWidth = [300]
        wall.OverrideAlign = ["Left"]
        wall.OverrideOffset = [25]

        first = wall.Proxy.get_resolved_section(wall, segment_index=0)
        second = wall.Proxy.get_resolved_section(wall, segment_index=1)

        self.assertEqual([layer.raw_thickness for layer in first.layers], [300])
        self.assertEqual([layer.raw_thickness for layer in second.layers], [100])
        self.assertEqual(wall.Proxy.get_width(wall, widths=False), 100.0)
        self.assertEqual(wall.Proxy.get_width(wall), (100.0, [300]))

        self.document.recompute()
        section = wall.Proxy.get_resolved_section(wall)
        self.assertAlmostEqual(section.y_min, -325.0)
        self.assertAlmostEqual(section.y_max, -25.0)

        first_segment = wall.Shape.common(Part.makeBox(2, 2000, 1000, App.Vector(499, -1000, 0)))
        self.assertAlmostEqual(first_segment.BoundBox.YMin, section.y_min, delta=1e-6)
        self.assertAlmostEqual(first_segment.BoundBox.YMax, section.y_max, delta=1e-6)

        second_section = wall.Proxy.get_resolved_section(wall, segment_index=1)
        second_segment = wall.Shape.common(Part.makeBox(2000, 2, 1000, App.Vector(-500, 499, 0)))
        self.assertAlmostEqual(
            second_segment.BoundBox.XMin, 1000 + second_section.y_min, delta=1e-6
        )
        self.assertAlmostEqual(
            second_segment.BoundBox.XMax, 1000 + second_section.y_max, delta=1e-6
        )

    def test_resolved_material_layers_use_wall_width_for_all_segments(self):
        """Variable material layers use the wall width, not segment overrides."""
        wire = Draft.makeWire(
            [App.Vector(0, 0, 0), App.Vector(1000, 0, 0), App.Vector(1000, 1000, 0)]
        )
        material = Arch.makeMultiMaterial()
        material.Materials = [Arch.makeMaterial(), Arch.makeMaterial()]
        material.Thicknesses = [50, 0]
        wall = Arch.makeWall(wire, width=100, height=1000, align="Center")
        wall.Material = material
        wall.OverrideWidth = [300, 100]
        self.document.recompute()

        first = wall.Proxy.get_resolved_section(wall, segment_index=0)
        second = wall.Proxy.get_resolved_section(wall, segment_index=1)
        self.assertEqual([layer.raw_thickness for layer in first.layers], [50, 50])
        self.assertEqual([layer.raw_thickness for layer in second.layers], [50, 50])

        second_segment = wall.Shape.common(Part.makeBox(2000, 2, 1000, App.Vector(-500, 499, 0)))
        self.assertAlmostEqual(second_segment.BoundBox.XMin, 1000 + second.y_min, delta=1e-6)
        self.assertAlmostEqual(second_segment.BoundBox.XMax, 1000 + second.y_max, delta=1e-6)

    def test_resolved_section_rejects_invalid_alignment_values(self):
        """Invalid alignment sources fall back to wall.Align."""
        wall = Arch.makeWall(length=2000, width=100, height=1000, align="Center")
        wall.OverrideAlign = ["Bogus"]
        section = wall.Proxy.get_resolved_section(wall)
        self.assertAlmostEqual(section.y_min, -50.0)
        self.assertAlmostEqual(section.y_max, 50.0)

        class InvalidArchSketchProvider:
            Type = "ArchSketch"

            @staticmethod
            def getWidths(_obj, **_kwargs):
                return [100]

            @staticmethod
            def getAligns(_obj, **_kwargs):
                return ["Bogus"]

            @staticmethod
            def getOffsets(_obj, **_kwargs):
                return [0]

        base = self.document.addObject("Part::FeaturePython", "InvalidArchSketchBase")
        base.Proxy = InvalidArchSketchProvider()
        provider_wall = Arch.makeWall(length=2000, width=100, height=1000, align="Center")
        provider_wall.Base = base
        provider_wall.ArchSketchData = True
        provider_section = provider_wall.Proxy.get_resolved_section(provider_wall)
        self.assertAlmostEqual(provider_section.y_min, -50.0)
        self.assertAlmostEqual(provider_section.y_max, 50.0)

    def test_wall_ending_properties_trim_wall(self):
        """Tests that EndingStart/EndingEnd placements trim and restore a wall shape."""
        self.printTestMessage("Checking wall ending properties...")

        wall = Arch.makeWall(length=2000, width=200, height=1000)
        self.document.recompute()
        initial_volume = wall.Shape.Volume
        self.assertGreater(initial_volume, 0)

        wall.EndingEnd = App.Placement(
            App.Vector(1000, 0, 0),
            App.Rotation(App.Vector(0, 0, 1), 45) * App.Rotation(App.Vector(0, 1, 0), 90),
        )
        self.document.recompute()

        self.assertTrue(wall.Shape.isValid(), "Wall shape became invalid after trimming.")
        self.assertLess(wall.Shape.Volume, initial_volume)
        self.assertLess(wall.Shape.BoundBox.XMax, 1000.01)

        wall.EndingEnd = App.Placement()
        self.document.recompute()
        self.assertAlmostEqual(wall.Shape.Volume, initial_volume, delta=1e-6)

    def test_wall_end_condition_selector_orders_sources(self):
        """Tests that end-condition selection follows the configured order."""
        manual = ArchWallEndCondition.WallEndCondition(
            source="Manual",
            placement=App.Placement(App.Vector(100, 0, 0), App.Rotation()),
        )
        relation = ArchWallEndCondition.WallEndCondition(
            source="Relation",
            placement=App.Placement(App.Vector(200, 0, 0), App.Rotation()),
            is_global=True,
            extension=12.5,
        )

        order = ["Manual", "Relation", "Manual", "Bogus"]
        active = ArchWallEndCondition.select_end_condition([relation, manual], order)
        self.assertEqual(
            ArchWallEndCondition.normalize_end_condition_order(order), ["Manual", "Relation"]
        )
        self.assertEqual(active.source, "Manual")

        active = ArchWallEndCondition.select_end_condition(
            [relation, manual], ["Relation", "Manual"]
        )
        self.assertEqual(active.source, "Relation")
        self.assertTrue(active.is_global)
        self.assertAlmostEqual(active.extension, 12.5)

        inactive_relation = ArchWallEndCondition.WallEndCondition(source="Relation", extension=25.0)
        self.assertIs(
            ArchWallEndCondition.select_end_condition(
                [inactive_relation, manual], ["Relation", "Manual"]
            ),
            manual,
        )

    def test_wall_end_conditions_require_brep_export(self):
        """Walls with processed end planes must not use untrimmed IFC extrusions."""
        self.printTestMessage("Checking IFC representation selection for wall endings...")

        wall = Arch.makeWall(length=2000, width=200, height=1000)
        self.document.recompute()
        self.assertFalse(wall.Proxy.requires_brep_export(wall))
        self.assertTrue(wall.Proxy.isStandardCase(wall))

        wall.EndingEnd = App.Placement(
            App.Vector(1000, 0, 0),
            App.Rotation(App.Vector(0, 0, 1), 45) * App.Rotation(App.Vector(0, 1, 0), 90),
        )
        self.document.recompute()

        self.assertTrue(
            wall.Proxy.requires_brep_export(wall),
            "A wall with an active end condition must export its processed shape.",
        )
        self.assertFalse(
            wall.Proxy.isStandardCase(wall),
            "A BREP-exported wall must not be classified as IfcWallStandardCase.",
        )

        wall.EndingEnd = App.Placement()
        self.document.recompute()
        self.assertFalse(wall.Proxy.requires_brep_export(wall))
        self.assertTrue(wall.Proxy.isStandardCase(wall))

    def test_wall_end_condition_order_changes_active_trim(self):
        """Tests that the configured order changes which trim drives the wall end."""
        support_wall = Arch.makeWall(length=2000, width=200, height=1000)
        trimmed_wall = Arch.makeWall(length=1000, width=200, height=1000)
        trimmed_wall.Placement = App.Placement(
            App.Vector(1000, 500, 0),
            App.Rotation(App.Vector(1, 0, 0), App.Vector(0, 1, 0)),
        )
        self.document.recompute()

        joint = Arch.makeWallJoint(support_wall, trimmed_wall, "Butt")
        joint.ButtTrimmed = "WallB"
        support_wall.EndingEnd = App.Placement(
            App.Vector(600, 0, 0), App.Rotation(App.Vector(0, 1, 0), 90)
        )
        self.document.recompute()
        joint_first_xmax = support_wall.Shape.BoundBox.XMax
        self.assertGreater(joint_first_xmax, 800.0)

        support_wall.EndConditionOrderEnd = ["Manual", "Relation"]
        self.document.recompute()
        self.assertAlmostEqual(support_wall.Shape.BoundBox.XMax, 600.0, delta=1e-4)

    def test_wall_makeblocks(self):
        """Test the 'MakeBlocks' feature for both based and baseless Arch Walls.
        This is a regression test for https://github.com/FreeCAD/FreeCAD/issues/26982,
        a unit test for https://github.com/FreeCAD/FreeCAD/issues/27817, and a basic, functional test for the
        MakeBlocks code path.
        """
        operation = "Checking Arch Wall MakeBlocks functional correctness..."
        self.printTestMessage(operation)

        # Block parameters
        L, H, W = 1000.0, 600.0, 200.0
        BL, BH = 400.0, 200.0  # Block Length and Height
        O1, O2 = 0.0, 200.0  # Row offsets

        def calc_row(row_start):
            """
            Simulates the 1D block-segmentation logic for a single horizontal course.

            This helper replicates the "sawing" algorithm found in _Wall._make_blocks:
            1. It places the first vertical joint at 'row_start'.
            2. It advances the cutting position by 'BlockLength' (BL).
            3. It measures the resulting segments between joints.
            4. It classifies segments equal to 'BL' as 'Entire' and any
               remainder (at the start or end of the row) as 'Broken'.

            Args:
                row_start (float): The distance from the start of the wall to the first vertical
                                   joint.

            Returns:
                tuple (int, int): A pair of integers (entire_count, broken_count)
                                  predicted for this specific row.
            """
            row_entire, row_broken = 0, 0
            current_pos = row_start
            last_pos = 0.0

            # Mimic the logic in ArchWall:
            # while offset < (Length - Joint): create cut at offset
            # Perform the cuts and record the resulting segments
            segments = []
            while current_pos < L:
                if current_pos > 0:
                    segments.append(current_pos - last_pos)
                    last_pos = current_pos
                current_pos += BL
            if last_pos < L:
                segments.append(L - last_pos)

            # Classify segments
            for seg_len in segments:
                if abs(seg_len - BL) < 0.1:  # Threshold for "Entire"
                    row_entire += 1
                else:
                    row_broken += 1
            return row_entire, row_broken

        # Calculate expectations based on total courses (rows)
        num_rows = int(H // BH)
        expected_entire = 0
        expected_broken = 0

        # Effectively "lay the bricks" in a running bond pattern: alternate offsets per even/odd row
        for r in range(num_rows):
            ent, brk = calc_row(O1 if r % 2 == 0 else O2)
            expected_entire += ent
            expected_broken += brk

        expected_vol = L * W * H

        # Create both wall variants: one with a base line, one baseless
        line = Draft.makeLine(App.Vector(0, 0, 0), App.Vector(L, 0, 0))
        self.document.recompute()
        based_wall = Arch.makeWall(line, width=W, height=H)
        baseless_wall = Arch.makeWall(None, length=L, width=W, height=H)

        walls = {"based": based_wall, "baseless": baseless_wall}
        for wall in walls.values():
            wall.BlockLength = BL
            wall.BlockHeight = BH
            wall.Joint = 0  # For test and volume calculation simplicity
            wall.OffsetFirst = O1
            wall.OffsetSecond = O2
            wall.MakeBlocks = True
        self.document.recompute()

        for label, wall in walls.items():
            with self.subTest(wall=label):
                # Regression check
                self.assertFalse(wall.Shape.isNull(), "Wall shape should not be null")

                # Functional check: block counts and volume correctness
                self.assertEqual(
                    wall.CountEntire,
                    expected_entire,
                    f"Mismatch in Entire blocks. Expected {expected_entire}, got {wall.CountEntire}",
                )
                self.assertEqual(
                    wall.CountBroken,
                    expected_broken,
                    f"Mismatch in Broken blocks. Expected {expected_broken}, got {wall.CountBroken}",
                )

                # Integrity check: volume correctness
                self.assertAlmostEqual(wall.Shape.Volume, expected_vol, places=3)

    def test_debase_wall_stationary_children(self):
        """Test that debasing a wall does not shift its children in world space."""
        self.printTestMessage("Arch.debaseWall stationary children...")

        # Create a base line offset by 5 meters for a clear distinction between local (0,0,0) and
        # global coordinates.
        # Line start/end: (5000,0,0) / (7000,0,0). Midpoint/new placement: (6000,0,0).
        line = Draft.makeLine(App.Vector(0, 0, 0), App.Vector(2000, 0, 0))
        line.Placement.Base = App.Vector(5000, 0, 0)
        self.document.recompute()

        # Create the wall. Initially, wall.Placement is (0,0,0).
        wall = Arch.makeWall(line, width=200, height=3000)
        self.document.recompute()

        # Create a set of different types of children, with different properties

        # Child A: a standard Part::Box primitive. Lacks MoveWithHost property. It should be handled
        # by the default move logic.
        box = self.document.addObject("Part::Box", "ChildBox")
        box.Placement.Base = App.Vector(5250, -150, 500)

        # Child B: an ArchComponent with MoveWithHost=True. This should be explicitly included in
        # the move logic.
        comp_true_base = self.document.addObject("Part::Box", "CompTrueBase")
        comp_true = Arch.makeComponent(comp_true_base, name="CompWithMove")
        comp_true.MoveWithHost = True
        comp_true.Placement.Base = App.Vector(5750, -150, 500)

        # Child C: an ArchComponent with MoveWithHost=False. This should be ignored by the move
        # logic.
        comp_false_base = self.document.addObject("Part::Box", "CompFalseBase")
        comp_false = Arch.makeComponent(comp_false_base, name="CompNoMove")
        comp_false.MoveWithHost = False
        comp_false.Placement.Base = App.Vector(6250, -150, 500)

        # Child D: a hosted Arch.Window. This is not an Addition but is found via InList by
        # getMovableChildren.
        win_base = Draft.makeRectangle(length=500, height=800)
        win_base.Placement.Rotation = App.Rotation(App.Vector(1, 0, 0), 90)  # Orient vertically
        win_base.Placement.Base = App.Vector(6500, 100, 1000)  # Position in wall center
        self.document.recompute()
        window = Arch.makeWindow(win_base)

        # Add all children/guests to the wall
        wall.Additions = [box, comp_true, comp_false]
        window.Hosts = [wall]
        self.document.recompute()

        # Record initial global placements for all children/hosts before debasing.
        initial_placements = {
            "Box": box.Placement.copy(),
            "CompTrue": comp_true.Placement.copy(),
            "CompFalse": comp_false.Placement.copy(),
            "Window": window.Placement.copy(),
        }

        # Perform the debase operation. This will reset wall.Placement from (0,0,0) to (6000,0,0)
        # and trigger the onChanged -> getMovableChildren -> move logic.
        Arch.debaseWall(wall)
        self.document.recompute()

        # All children must remain at their original global coordinates. Use subtests to get a clear
        # report for each child type.
        with self.subTest(child_type="Part Primitive (Box)"):
            self.assertTrue(
                box.Placement.Base.isEqual(initial_placements["Box"].Base, 1e-6),
                f"Part Primitive Box position shifted! Expected {initial_placements['Box'].Base}, got {box.Placement.Base}",
            )

        with self.subTest(child_type="ArchComponent (MoveWithHost=True)"):
            self.assertTrue(
                comp_true.Placement.Base.isEqual(initial_placements["CompTrue"].Base, 1e-6),
                f"Component with MoveWithHost=True position shifted! Expected {initial_placements['CompTrue'].Base}, got {comp_true.Placement.Base}",
            )

        with self.subTest(child_type="ArchComponent (MoveWithHost=False)"):
            self.assertTrue(
                comp_false.Placement.Base.isEqual(initial_placements["CompFalse"].Base, 1e-6),
                f"Component with MoveWithHost=False position shifted! Expected {initial_placements['CompFalse'].Base}, got {comp_false.Placement.Base}",
            )

        with self.subTest(child_type="Hosted Window"):
            self.assertTrue(
                window.Placement.Base.isEqual(initial_placements["Window"].Base, 1e-6),
                f"Hosted Window position shifted! Expected {initial_placements['Window'].Base}, got {window.Placement.Base}",
            )

        # Final verification that the wall itself was correctly debased
        self.assertIsNone(wall.Base, "Wall was not successfully debased (Base still exists).")
        self.assertAlmostEqual(wall.Placement.Base.x, 6000.0, places=3)

    def test_baseless_wall_offset(self):
        """Test that the Offset property shifts the geometry of a baseless wall.

        Regression test for https://github.com/FreeCAD/FreeCAD/issues/29256.
        """
        self.printTestMessage("Checking baseless wall Offset property...")

        length, width, height, offset = 2000.0, 200.0, 3000.0, 1000.0

        # Left alignment: wall body is in -Y direction. Offset shifts it further in -Y.
        wall_left = Arch.makeWall(
            length=length, width=width, height=height, align="Left", offset=offset
        )
        self.assertIsNone(wall_left.Base, "Left: wall should be baseless")
        self.document.recompute()
        bb = wall_left.Shape.BoundBox
        self.assertAlmostEqual(bb.YMax, -offset, delta=1e-6, msg="Left: YMax should be -offset")
        self.assertAlmostEqual(
            bb.YMin, -width - offset, delta=1e-6, msg="Left: YMin should be -(width+offset)"
        )

        # Right alignment: wall body is in +Y direction. Offset shifts it further in +Y.
        wall_right = Arch.makeWall(
            length=length, width=width, height=height, align="Right", offset=offset
        )
        self.assertIsNone(wall_right.Base, "Right: wall should be baseless")
        self.document.recompute()
        bb = wall_right.Shape.BoundBox
        self.assertAlmostEqual(bb.YMin, offset, delta=1e-6, msg="Right: YMin should be offset")
        self.assertAlmostEqual(
            bb.YMax, width + offset, delta=1e-6, msg="Right: YMax should be width+offset"
        )
