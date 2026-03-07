# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2013 Yorik van Havre <yorik@uncreated.net>              *
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

    def test_wall_makeblocks(self):
        """Test the 'MakeBlocks' feature of Arch Wall.
        This is a regression test for https://github.com/FreeCAD/FreeCAD/issues/26982, and
        a basic, functional test for the MakeBlocks code path.
        """
        operation = "Checking Arch Wall MakeBlocks functional correctness..."
        self.printTestMessage(operation)

        # Block parameters
        L, H, W = 1000.0, 600.0, 200.0
        BL, BH = 400.0, 200.0  # Block Length and Height
        O1, O2 = 0.0, 200.0  # Row offsets

        # Create base line
        p1 = App.Vector(0, 0, 0)
        p2 = App.Vector(L, 0, 0)
        line = Draft.makeLine(p1, p2)
        self.document.recompute()

        # Create wall based on line and block parameters
        wall = Arch.makeWall(line, width=W, height=H)
        wall.BlockLength = BL
        wall.BlockHeight = BH
        wall.Joint = 0  # For test and volume calculation simplicity
        wall.OffsetFirst = O1
        wall.OffsetSecond = O2

        def calc_row(row_start):
            """
            Simulates the 1D block-segmentation logic for a single horizontal course.

            This helper replicates the "sawing" algorithm found in _Wall.execute:
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

        # Enable the feature, triggering the #26982 bug via the code path in _Wall.execute()
        wall.MakeBlocks = True
        self.document.recompute()

        # Regression check: did we crash?
        self.assertFalse(wall.Shape.isNull(), "Wall shape should not be null")

        # Functional check: compare dynamic calculation to property values
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
        expected_vol = L * W * H
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
