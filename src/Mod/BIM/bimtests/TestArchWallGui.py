# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 Furgo                                              *
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

"""GUI tests for the ArchWall module."""

import ArchWallRelation
import ArchWallJointGui
import FreeCAD
import FreeCADGui
import Draft
import Arch
import Part
import WorkingPlane
from bimtests import TestArchBaseGui
from bimcommands.BimWall import Arch_Wall
from bimcommands.BimJoin import (
    BIM_EditWallJoint,
    BIM_Join_Butt,
    BIM_Join_Junction,
    BIM_Join_Miter,
    BIM_Join_Tee,
    BIM_Unjoin,
)
from unittest.mock import patch


class MockTracker:
    """A dummy tracker to absorb GUI calls during logic tests."""

    def off(self):
        pass

    def on(self):
        pass

    def finalize(self):
        pass

    def update(self, points):
        pass

    def setorigin(self, arg):
        pass


class TestArchWallGui(TestArchBaseGui.TestArchBaseGui):

    def setUp(self):
        """Set up the test environment by activating the BIM workbench and setting preferences."""
        super().setUp()
        self.params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")
        self.original_wall_base = self.params.GetInt("WallBaseline", 1)  # Default to 1 (line)

    def tearDown(self):
        """Restore original preferences after the test."""
        self.params.SetInt("WallBaseline", self.original_wall_base)
        super().tearDown()

    def test_create_baseless_wall_interactive_mode(self):
        """
        Tests the interactive creation of a baseless wall by simulating the
        Arch_Wall command's internal logic.
        """
        from draftguitools import gui_trackers  # Import the tracker module

        self.printTestMessage("Testing interactive creation of a baseless wall...")

        # 1. Arrange: Set preference to "No baseline" mode
        self.params.SetInt("WallBaseline", 0)

        # 2. Arrange: Simulate the state of the command after two clicks
        cmd = Arch_Wall()
        cmd.doc = self.document
        cmd.wp = WorkingPlane.get_working_plane()
        cmd.points = [FreeCAD.Vector(1000, 1000, 0), FreeCAD.Vector(3000, 1000, 0)]
        cmd.Align = "Center"
        cmd.Width = 200.0
        cmd.Height = 2500.0
        cmd.MultiMat = None
        cmd.existing = []
        cmd.tracker = gui_trackers.boxTracker()

        initial_object_count = len(self.document.Objects)

        # 3. Act: Call the internal method that processes the points
        cmd.create_wall()

        # 4. Assert
        self.assertEqual(
            len(self.document.Objects),
            initial_object_count + 1,
            "Exactly one new object should have been created.",
        )

        wall = self.document.Objects[-1]
        self.assertEqual(Draft.get_type(wall), "Wall", "The created object is not a wall.")

        self.assertIsNone(wall.Base, "A baseless wall should have its Base property set to None.")

        self.assertAlmostEqual(
            wall.Length.Value, 2000.0, delta=1e-6, msg="Wall length is incorrect."
        )

        # Verify the placement is correct
        expected_center = FreeCAD.Vector(2000, 1000, 0)
        self.assertTrue(
            wall.Placement.Base.isEqual(expected_center, 1e-6),
            f"Wall center {wall.Placement.Base} does not match expected {expected_center}",
        )

        # Verify the rotation is correct (aligned with global X-axis, so no rotation)
        self.assertAlmostEqual(
            wall.Placement.Rotation.Angle,
            0.0,
            delta=1e-6,
            msg="Wall rotation should be zero for a horizontal line.",
        )

    def test_create_draft_line_baseline_wall_interactive(self):
        """Tests the interactive creation of a wall with a Draft.Line baseline."""
        from draftguitools import gui_trackers

        self.printTestMessage("Testing interactive creation of a Draft.Line based wall...")

        # 1. Arrange: Set preference to "Draft line" mode
        self.params.SetInt("WallBaseline", 1)  # Corresponds to WallBaselineMode.DRAFT_LINE

        cmd = Arch_Wall()
        cmd.doc = self.document
        cmd.wp = WorkingPlane.get_working_plane()
        cmd.points = [FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(2000, 0, 0)]
        cmd.Align = "Center"
        cmd.Width = 200.0
        cmd.Height = 2500.0
        cmd.MultiMat = None
        cmd.existing = []
        cmd.tracker = gui_trackers.boxTracker()

        initial_object_count = len(self.document.Objects)

        # 2. Act
        cmd.create_wall()

        # 3. Assert
        self.assertEqual(
            len(self.document.Objects),
            initial_object_count + 2,
            "Should have created a Wall and a Draft Line.",
        )

        # The wall is created after the base, so it's the last object
        wall = self.document.Objects[-1]
        base = self.document.Objects[-2]

        self.assertEqual(Draft.get_type(wall), "Wall")
        self.assertEqual(Draft.get_type(base), "Wire")
        self.assertEqual(wall.Base, base, "The wall's Base should be the newly created line.")

    def test_create_sketch_baseline_wall_interactive(self):
        """Tests the interactive creation of a wall with a Sketch baseline."""
        from draftguitools import gui_trackers

        self.printTestMessage("Testing interactive creation of a Sketch based wall...")

        # 1. Arrange: Set preference to "Sketch" mode
        self.params.SetInt("WallBaseline", 2)  # Corresponds to WallBaselineMode.SKETCH

        cmd = Arch_Wall()
        cmd.doc = self.document
        cmd.wp = WorkingPlane.get_working_plane()
        cmd.points = [FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(2000, 0, 0)]
        cmd.Align = "Center"
        cmd.Width = 200.0
        cmd.Height = 2500.0
        cmd.MultiMat = None
        cmd.existing = []
        cmd.tracker = gui_trackers.boxTracker()

        initial_object_count = len(self.document.Objects)

        # 2. Act
        cmd.create_wall()

        # 3. Assert
        self.assertEqual(
            len(self.document.Objects),
            initial_object_count + 2,
            "Should have created a Wall and a Sketch.",
        )

        wall = self.document.Objects[-1]
        base = self.document.Objects[-2]

        self.assertEqual(Draft.get_type(wall), "Wall")
        self.assertEqual(base.TypeId, "Sketcher::SketchObject")
        self.assertEqual(wall.Base, base, "The wall's Base should be the newly created sketch.")

    def test_stretch_rotated_baseless_wall(self):
        """Tests that the Draft_Stretch tool correctly handles a rotated baseless wall."""
        self.printTestMessage("Testing stretch on a rotated baseless wall...")

        from draftguitools.gui_stretch import Stretch

        # 1. Arrange: Create a rotated baseless wall
        wall = Arch.makeWall(length=2000, width=200, height=1500)

        rotation = FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 45)
        placement = FreeCAD.Placement(FreeCAD.Vector(1000, 1000, 0), rotation)
        wall.Placement = placement
        self.document.recompute()

        # Ensure the view is scaled to the object so selection logic works correctly
        FreeCADGui.ActiveDocument.ActiveView.fitAll()

        # Get initial state for assertion later
        initial_endpoints = wall.Proxy.calc_endpoints(wall)
        p_start_initial = initial_endpoints[0]
        p_end_initial = initial_endpoints[1]

        # 2. Act: Simulate the Stretch command
        cmd = Stretch()
        cmd.doc = self.document
        FreeCADGui.Selection.addSelection(self.document.Name, wall.Name)

        # Activate the command. It will detect the existing selection and
        # call proceed() internally after performing necessary setup.
        cmd.Activated()

        # Simulate user clicks:
        # Define a selection rectangle that encloses only the end point
        cmd.addPoint(FreeCAD.Vector(p_end_initial.x - 1, p_end_initial.y - 1, 0))
        cmd.addPoint(FreeCAD.Vector(p_end_initial.x + 1, p_end_initial.y + 1, 0))

        # Manually inject the selection state to bypass the view-dependent tracker,
        # which acts inconsistently in a headless test environment.
        # [False, True] selects the end point while keeping the start point anchored.
        cmd.ops = [[wall, [False, True]]]

        # Define the displacement vector
        displacement_vector = FreeCAD.Vector(500, -500, 0)
        cmd.addPoint(FreeCAD.Vector(0, 0, 0))  # Start of displacement
        cmd.addPoint(displacement_vector)  # End of displacement

        # Allow the GUI command's macro to be processed
        self.pump_gui_events()

        # 3. Assert: Verify the new position of the endpoints
        final_endpoints = wall.Proxy.calc_endpoints(wall)
        p_start_final = final_endpoints[0]
        p_end_final = final_endpoints[1]

        # Calculate the error vector for diagnosis
        diff = p_start_final.sub(p_start_initial)

        error_message = (
            f"\nThe unselected start point moved!\n"
            f"Initial:  {p_start_initial}\n"
            f"Final:    {p_start_final}\n"
            f"Diff Vec: {diff}\n"
            f"Error Mag: {diff.Length:.12f}"
        )

        # The start point should not have moved
        self.assertTrue(p_start_final.isEqual(p_start_initial, 1e-6), error_message)

        # The end point should have moved by the global displacement vector
        expected_end_point = p_end_initial.add(displacement_vector)
        self.assertTrue(
            p_end_final.isEqual(expected_end_point, 1e-6),
            f"Stretched endpoint {p_end_final} does not match expected {expected_end_point}",
        )

    def test_create_baseless_wall_on_rotated_working_plane(self):
        """Tests that a baseless wall respects the current working plane."""
        import Part

        self.printTestMessage("Testing baseless wall creation on a rotated working plane...")

        # Arrange: Create a non-standard working plane (rotated and elevated)
        wp = WorkingPlane.get_working_plane()
        placement = FreeCAD.Placement(
            FreeCAD.Vector(0, 0, 1000), FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 45)
        )

        # Apply the placement to the working plane, ensuring translation is included
        wp.align_to_placement(placement)

        # Define points in the local coordinate system of the working plane
        p1_local = FreeCAD.Vector(0, 0, 0)
        p2_local = FreeCAD.Vector(2000, 0, 0)

        # Convert local points to the global coordinates the command will receive
        p1_global = wp.get_global_coords(p1_local)
        p2_global = wp.get_global_coords(p2_local)

        self.params.SetInt("WallBaseline", 0)

        cmd = Arch_Wall()
        cmd.doc = self.document
        cmd.wp = wp
        cmd.points = [p1_global, p2_global]
        cmd.Align = "Center"
        cmd.Width = 200.0
        cmd.Height = 1500.0
        cmd.MultiMat = None

        # Use a mock tracker to isolate logic tests from the 3D view environment
        cmd.tracker = MockTracker()
        cmd.existing = []

        # Act
        cmd.create_wall()

        # Assert
        wall = self.document.ActiveObject
        self.assertEqual(Draft.get_type(wall), "Wall")

        # Calculate the expected global placement
        midpoint_local = (p1_local + p2_local) * 0.5
        direction_local = (p2_local - p1_local).normalize()
        rotation_local = FreeCAD.Rotation(FreeCAD.Vector(1, 0, 0), direction_local)
        placement_local = FreeCAD.Placement(midpoint_local, rotation_local)

        # The wall's final placement must be the local placement transformed by the WP
        expected_placement = wp.get_placement().multiply(placement_local)

        # Compare Position (Vector)
        self.assertTrue(
            wall.Placement.Base.isEqual(expected_placement.Base, Part.Precision.confusion()),
            f"Wall position {wall.Placement.Base} does not match expected {expected_placement.Base}",
        )

        # Compare Orientation (Rotation)
        self.assertTrue(
            wall.Placement.Rotation.isSame(expected_placement.Rotation, Part.Precision.confusion()),
            f"Wall rotation {wall.Placement.Rotation.Q} does not match expected {expected_placement.Rotation.Q}",
        )

    def test_create_multiple_sketch_based_walls(self):
        """Tests that creating multiple sketch-based walls uses separate sketches."""
        self.printTestMessage("Testing creation of multiple sketch-based walls...")

        self.params.SetInt("WallBaseline", 2)

        cmd = Arch_Wall()
        cmd.doc = self.document
        cmd.wp = WorkingPlane.get_working_plane()
        cmd.Align = "Left"
        cmd.Width = 200.0
        cmd.Height = 1500.0
        cmd.MultiMat = None
        cmd.tracker = MockTracker()
        cmd.existing = []

        initial_object_count = len(self.document.Objects)

        # Act: Create the first wall
        cmd.points = [FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(1000, 0, 0)]
        cmd.create_wall()
        base1 = self.document.getObject("Wall").Base

        # Act again: Create the second wall
        cmd.points = [FreeCAD.Vector(0, 1000, 0), FreeCAD.Vector(1000, 1000, 0)]
        cmd.create_wall()

        # Retrieve the last object to ensure we get the newest wall
        wall2 = self.document.ActiveObject
        base2 = wall2.Base

        # Assert
        self.assertEqual(
            len(self.document.Objects),
            initial_object_count + 4,
            "Should have created two Walls and two Sketches.",
        )
        self.assertIsNotNone(base1, "First wall should have a base sketch.")
        self.assertIsNotNone(base2, "Second wall should have a base sketch.")

        self.assertNotEqual(
            base1,
            base2,
            "Each sketch-based wall should have its own unique sketch object.",
        )

    def _get_mock_side_effect(self, **kwargs):
        """
        Creates a side_effect function for mocking params.get_param.
        This reads the actual system parameter dictionary (draftutils.params.PARAM_DICT)
        to populate defaults, ensuring all parameters expected by Draft/BIM tools are present.
        It then overrides specific values as needed for the test.
        """

        from draftutils import params

        def side_effect(name, path=None, ret_default=False, silent=False):
            # Start with a comprehensive dictionary built from the real parameter definitions
            defaults = {}
            # Flatten PARAM_DICT: iterate over all groups ('Mod/Draft', 'View', etc.)
            for group_name, group_params in params.PARAM_DICT.items():
                for param_name, param_data in group_params.items():
                    # param_data is (type, value)
                    defaults[param_name] = param_data[1]

            # Add or Override with test-specific values and missing parameters
            # Some parameters might be dynamic or not yet in PARAM_DICT in the environment
            overrides = {
                # Arch Wall specific overrides for tests
                "joinWallSketches": False,
                "autoJoinWalls": False,
                "WallBaseline": 0,
            }
            defaults.update(overrides)

            # Apply any kwargs passed specifically to this side_effect call (from tests)
            if name in kwargs:
                return kwargs[name]

            val = defaults.get(name)

            return val

        return side_effect

    def _simulate_interactive_wall_creation(self, p1, p2, existing_wall, wall_width=200.0):
        """
        Simulates the core logic of the Arch_Wall command's interactive mode.
        """
        try:
            cmd = Arch_Wall()

            # This calls the real Activated() method, but the mock intercepts the
            # calls to params.get_param, allowing us to control the outcome.
            FreeCADGui.Selection.clearSelection()
            cmd.Activated()

            # Override interactive parts of the command instance
            cmd.doc = self.document
            cmd.wp = WorkingPlane.get_working_plane()
            cmd.points = [p1, p2]
            cmd.Width = wall_width
            cmd.existing = [existing_wall] if existing_wall else []
            cmd.tracker = MockTracker()

            # This is the core action being tested
            cmd.create_wall()
            return self.document.Objects[-1]  # Return the newly created wall
        finally:
            # Clean up the global command state to ensure test isolation
            # We put this here to ensure cleanup even if the wall creation fails
            if FreeCAD.activeDraftCommand is cmd:
                FreeCAD.activeDraftCommand = None

    def _make_baseless_wall_between(self, p1, p2, width=200.0, height=1500.0):
        """Create a baseless wall between two global points."""
        line_vector = p2.sub(p1)
        wall = Arch.makeWall(length=line_vector.Length, width=width, height=height)
        wall.Placement = FreeCAD.Placement(
            (p1 + p2) * 0.5,
            FreeCAD.Rotation(FreeCAD.Vector(1, 0, 0), line_vector.normalize()),
        )
        self.document.recompute()
        return wall

    def _make_line_based_wall_between(self, p1, p2, width=200.0, height=1500.0):
        """Create a wall from a Draft line baseline."""
        line = Draft.makeLine(p1, p2)
        self.document.recompute()
        wall = Arch.makeWall(line, width=width, height=height)
        self.document.recompute()
        return wall

    def _activate_join_command(self, command, *objects):
        """Select objects and run a join command."""
        FreeCADGui.Selection.clearSelection()
        for obj in objects:
            FreeCADGui.Selection.addSelection(self.document.Name, obj.Name)
        command.Activated()
        self.pump_gui_events()
        self.document.recompute()
        FreeCADGui.Selection.clearSelection()

    @staticmethod
    def _is_identity_placement(placement, tol=1e-9):
        return placement.Base.Length < tol and placement.Rotation.Angle < tol

    @staticmethod
    def _placements_match(left, right, tol=1e-9):
        return left.Base.isEqual(right.Base, tol) and left.Rotation.isSame(right.Rotation, tol)

    def _assert_miter_walls_closed(self, wall1, wall2):
        self.assertTrue(wall1.Shape.isValid(), "First wall became invalid after miter join.")
        self.assertTrue(wall2.Shape.isValid(), "Second wall became invalid after miter join.")
        self.assertAlmostEqual(
            wall1.Shape.distToShape(wall2.Shape)[0],
            0.0,
            delta=1e-6,
            msg="Mitered walls should touch.",
        )

    def _get_wall_joints(self):
        return [
            obj
            for obj in self.document.Objects
            if getattr(getattr(obj, "Proxy", None), "Type", None) == "WallJoint"
        ]

    def _get_wall_junctions(self):
        return [
            obj
            for obj in self.document.Objects
            if getattr(getattr(obj, "Proxy", None), "Type", None) == "WallJunction"
        ]

    def _activate_edit_command(self, command, joint):
        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addSelection(self.document.Name, joint.Name)
        command.Activated()
        self.pump_gui_events()

    def test_bim_edit_wall_joint_command_enters_edit_mode(self):
        """Tests that the edit command opens the selected joint in task-panel edit mode."""
        self.printTestMessage("Testing BIM edit wall joint command...")

        wall1 = self._make_baseless_wall_between(
            FreeCAD.Vector(-1000, 0, 0), FreeCAD.Vector(0, 0, 0)
        )
        wall2 = self._make_baseless_wall_between(
            FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 1000, 0)
        )
        joint = Arch.makeWallJoint(wall1, wall2, "Miter")
        self.document.recompute()

        self._activate_edit_command(BIM_EditWallJoint(), joint)

        in_edit = FreeCADGui.ActiveDocument.getInEdit()
        self.assertIsNotNone(in_edit)
        self.assertEqual(getattr(in_edit, "Object", None), joint)
        FreeCADGui.ActiveDocument.resetEdit()
        self.pump_gui_events()

    def test_wall_joint_task_panel_applies_joint_settings(self):
        """Tests that the wall-joint task panel applies edited settings cleanly."""
        self.printTestMessage("Testing BIM wall joint task panel applies settings...")

        wall1 = self._make_baseless_wall_between(
            FreeCAD.Vector(-1000, 0, 0), FreeCAD.Vector(0, 0, 0)
        )
        wall2 = self._make_baseless_wall_between(
            FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 1000, 0)
        )
        joint = Arch.makeWallJoint(wall1, wall2, "Miter")
        self.document.recompute()

        panel = ArchWallJointGui.WallJointTaskPanel(joint)
        panel._set_combo_value(panel.joint_type_combo, "JointType", "Butt")
        panel._set_combo_value(panel.end_a_combo, "EndA", "End")
        panel._set_combo_value(panel.end_b_combo, "EndB", "None")
        panel._set_combo_value(panel.butt_trimmed_combo, "ButtTrimmed", "WallA")
        panel._refresh_preview()
        self.assertTrue(panel.preview_group.isHidden())
        self.assertTrue(panel.preview_message.isHidden())
        self.assertFalse(panel.butt_trimmed_combo.isHidden())
        self.assertTrue(panel.tee_stem_combo.isHidden())

        panel.accept()
        self.pump_gui_events()

        self.assertEqual(joint.JointType, "Butt")
        self.assertEqual(joint.EndA, "End")
        self.assertEqual(joint.EndB, "None")
        self.assertEqual(joint.ButtTrimmed, "WallA")
        self.assertEqual(joint.Status, "OK")

    def test_wall_joint_task_panel_previews_conflicts(self):
        """Tests that the wall-joint task panel previews blocking conflicts before applying."""
        self.printTestMessage("Testing BIM wall joint task panel conflict preview...")

        wall1 = self._make_baseless_wall_between(
            FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(2000, 0, 0)
        )
        wall2 = self._make_baseless_wall_between(
            FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 1000, 0)
        )
        wall3 = self._make_baseless_wall_between(
            FreeCAD.Vector(100, 0, 0), FreeCAD.Vector(100, 1000, 0)
        )

        blocker = Arch.makeWallJoint(wall1, wall2, "Miter")
        candidate = Arch.makeWallJoint(wall1, wall3, "Miter")
        self.document.recompute()

        panel = ArchWallJointGui.WallJointTaskPanel(candidate)
        panel._set_combo_value(panel.joint_type_combo, "JointType", "Tee")
        panel._set_combo_value(panel.tee_stem_combo, "TeeStem", "WallA")
        panel._set_combo_value(panel.end_a_combo, "EndA", "Start")
        panel._refresh_preview()

        self.assertFalse(panel.preview_group.isHidden())
        self.assertEqual(panel.preview_title.text(), "Conflict")
        self.assertFalse(panel.preview_message.isHidden())
        self.assertIn("Start", panel.preview_message.text())
        self.assertIn(blocker.Label, panel.preview_message.text())
        self.assertTrue(panel.butt_trimmed_combo.isHidden())
        self.assertFalse(panel.tee_stem_combo.isHidden())

    def test_wall_joint_task_panel_explains_required_extension(self):
        """Tests that the wall-joint task panel explains unsupported extension cases."""
        self.printTestMessage("Testing BIM wall joint task panel extension preview...")

        wall1 = self._make_baseless_wall_between(
            FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(1000, 0, 0)
        )
        wall2 = self._make_baseless_wall_between(
            FreeCAD.Vector(2000, 1000, 0), FreeCAD.Vector(2000, 2000, 0)
        )
        joint = Arch.makeWallJoint(wall1, wall2, "Miter")
        self.document.recompute()

        panel = ArchWallJointGui.WallJointTaskPanel(joint)

        self.assertEqual(joint.Status, "RequiresExtension")
        self.assertFalse(panel.preview_group.isHidden())
        self.assertEqual(panel.preview_title.text(), "Cannot create this join")
        self.assertIn("extended to meet", panel.preview_message.text())
        self.assertIn("not supported yet", panel.preview_message.text())
        self.assertNotIn("finite segments", panel.preview_message.text())

    def test_bim_join_miter_creates_wall_joint(self):
        """Tests that the miter join creates a wall-joint relation and trims the walls."""
        self.printTestMessage("Testing BIM miter join...")

        wall1 = self._make_baseless_wall_between(
            FreeCAD.Vector(-1000, 0, 0), FreeCAD.Vector(0, 0, 0)
        )
        wall2 = self._make_baseless_wall_between(
            FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 1000, 0)
        )

        self._activate_join_command(BIM_Join_Miter(), wall1, wall2)

        joints = self._get_wall_joints()
        self.assertEqual(
            len(joints), 1, "The miter join should create exactly one relation object."
        )
        joint = joints[0]

        self.assertEqual(joint.JointType, "Miter")
        self.assertEqual(joint.Status, "OK")
        self.assertEqual(joint.WallA, wall1)
        self.assertEqual(joint.WallB, wall2)
        self.assertFalse(
            self._is_identity_placement(joint.ResolvedPlaneA),
            "The relation should solve a cutting plane for the first wall.",
        )
        self.assertTrue(
            self._is_identity_placement(wall1.EndingStart)
            and self._is_identity_placement(wall1.EndingEnd)
            and self._is_identity_placement(wall2.EndingStart)
            and self._is_identity_placement(wall2.EndingEnd),
            "Manual wall endings should remain untouched by the join command.",
        )
        self._assert_miter_walls_closed(wall1, wall2)

    def test_bim_join_butt_creates_wall_joint(self):
        """Tests that the butt join creates a wall-joint relation with the expected role."""
        self.printTestMessage("Testing BIM butt join...")

        wall1 = self._make_baseless_wall_between(
            FreeCAD.Vector(-1000, 0, 0), FreeCAD.Vector(0, 0, 0)
        )
        wall2 = self._make_baseless_wall_between(
            FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 1000, 0)
        )

        self._activate_join_command(BIM_Join_Butt(), wall1, wall2)

        joints = self._get_wall_joints()
        self.assertEqual(len(joints), 1)
        self.assertEqual(joints[0].JointType, "Butt")
        self.assertEqual(joints[0].ButtTrimmed, "WallB")
        self.assertEqual(joints[0].Status, "OK")
        self.assertTrue(
            self._is_identity_placement(wall1.EndingStart)
            and self._is_identity_placement(wall1.EndingEnd)
            and self._is_identity_placement(wall2.EndingStart)
            and self._is_identity_placement(wall2.EndingEnd),
            "Manual endings should stay untouched when the butt relation drives the trim.",
        )
        self.assertTrue(wall1.Shape.isValid(), "First wall became invalid after butt join.")
        self.assertTrue(wall2.Shape.isValid(), "Second wall became invalid after butt join.")

    def test_bim_join_reuses_existing_relation(self):
        """Tests that re-running the same join command updates the existing relation instead of duplicating it."""
        self.printTestMessage("Testing BIM join relation reuse...")

        wall1 = self._make_baseless_wall_between(
            FreeCAD.Vector(-1000, 0, 0), FreeCAD.Vector(0, 0, 0)
        )
        wall2 = self._make_baseless_wall_between(
            FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 1000, 0)
        )

        self._activate_join_command(BIM_Join_Miter(), wall1, wall2)
        joints = self._get_wall_joints()
        self.assertEqual(len(joints), 1)
        joint_name = joints[0].Name

        self._activate_join_command(BIM_Join_Miter(), wall2, wall1)
        joints = self._get_wall_joints()

        self.assertEqual(len(joints), 1, "Repeated joins should reuse the existing relation.")
        self.assertEqual(joints[0].Name, joint_name)
        self.assertEqual(joints[0].JointType, "Miter")

    def test_bim_unjoin_removes_relation_between_selected_walls(self):
        """Tests that Unjoin removes the relation between two selected walls."""
        self.printTestMessage("Testing BIM unjoin from wall selection...")

        wall1 = self._make_baseless_wall_between(
            FreeCAD.Vector(-1000, 0, 0), FreeCAD.Vector(0, 0, 0)
        )
        wall2 = self._make_baseless_wall_between(
            FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 1000, 0)
        )
        original_volume1 = wall1.Shape.Volume
        original_volume2 = wall2.Shape.Volume

        self._activate_join_command(BIM_Join_Miter(), wall1, wall2)
        joints = self._get_wall_joints()
        self.assertEqual(len(joints), 1)

        self._activate_join_command(BIM_Unjoin(), wall1, wall2)

        self.assertEqual(len(self._get_wall_joints()), 0)
        self.assertAlmostEqual(wall1.Shape.Volume, original_volume1, delta=1e-6)
        self.assertAlmostEqual(wall2.Shape.Volume, original_volume2, delta=1e-6)

    def test_bim_unjoin_removes_selected_joint_object(self):
        """Tests that Unjoin removes a directly selected wall-joint relation."""
        self.printTestMessage("Testing BIM unjoin from joint selection...")

        wall1 = self._make_baseless_wall_between(
            FreeCAD.Vector(-1000, 0, 0), FreeCAD.Vector(0, 0, 0)
        )
        wall2 = self._make_baseless_wall_between(
            FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 1000, 0)
        )
        original_volume1 = wall1.Shape.Volume
        original_volume2 = wall2.Shape.Volume

        joint = Arch.makeWallJoint(wall1, wall2, "Miter")
        self.document.recompute()
        self.assertEqual(joint.Status, "OK")

        self._activate_join_command(BIM_Unjoin(), joint)

        self.assertEqual(len(self._get_wall_joints()), 0)
        self.assertAlmostEqual(wall1.Shape.Volume, original_volume1, delta=1e-6)
        self.assertAlmostEqual(wall2.Shape.Volume, original_volume2, delta=1e-6)

    def test_bim_join_conflict_reports_blocker_and_recovers_after_unjoin(self):
        """Tests that conflicting joins record the blocking relation and recover after Unjoin."""
        self.printTestMessage("Testing BIM join conflict reporting and recovery...")

        wall1 = self._make_baseless_wall_between(
            FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(2000, 0, 0)
        )
        wall2 = self._make_baseless_wall_between(
            FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 1000, 0)
        )
        wall3 = self._make_baseless_wall_between(
            FreeCAD.Vector(100, 0, 0), FreeCAD.Vector(100, 1000, 0)
        )

        self._activate_join_command(BIM_Join_Miter(), wall1, wall2)
        joints = self._get_wall_joints()
        self.assertEqual(len(joints), 1)
        blocker = joints[0]
        self.assertEqual(blocker.Status, "OK")
        losing_branch_volume = wall3.Shape.Volume

        self._activate_join_command(BIM_Join_Miter(), wall1, wall3)
        joints = self._get_wall_joints()
        self.assertEqual(len(joints), 2)

        conflicted = next(joint for joint in joints if joint != blocker)
        self.assertEqual(conflicted.Status, "Conflict")
        self.assertEqual(conflicted.ConflictJointLabelA, blocker.Label)
        self.assertIn("Start", conflicted.ConflictMessageA)
        self.assertIn(blocker.Label, conflicted.ConflictMessageA)
        self.assertAlmostEqual(
            wall3.Shape.Volume,
            losing_branch_volume,
            delta=1e-6,
            msg="A conflicted GUI relation must not partially trim its uncontested wall.",
        )

        self._activate_join_command(BIM_Unjoin(), blocker)

        self.assertEqual(len(self._get_wall_joints()), 1)
        self.assertEqual(conflicted.Status, "OK")
        self.assertEqual(conflicted.ConflictJointLabelA, "")
        self.assertEqual(conflicted.ConflictMessageA, "")

    def test_bim_join_tee_trims_only_stem_wall(self):
        """Tests that the tee join trims only the stem wall and leaves the top wall clear."""
        self.printTestMessage("Testing BIM tee join...")

        stem_wall = self._make_baseless_wall_between(
            FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 1000, 0)
        )
        top_wall = self._make_baseless_wall_between(
            FreeCAD.Vector(-1000, 0, 0), FreeCAD.Vector(1000, 0, 0)
        )
        initial_stem_volume = stem_wall.Shape.Volume
        initial_top_volume = top_wall.Shape.Volume

        self._activate_join_command(BIM_Join_Tee(), stem_wall, top_wall)

        joints = self._get_wall_joints()
        self.assertEqual(len(joints), 1)
        joint = joints[0]

        self.assertEqual(joint.JointType, "Tee")
        self.assertEqual(joint.Status, "OK")
        self.assertEqual(joint.TeeStem, "WallA")
        self.assertEqual(joint.ResolvedEndA, "Start")
        self.assertEqual(joint.ResolvedEndB, "None")
        self.assertTrue(
            self._is_identity_placement(stem_wall.EndingStart)
            and self._is_identity_placement(stem_wall.EndingEnd)
            and self._is_identity_placement(top_wall.EndingStart)
            and self._is_identity_placement(top_wall.EndingEnd),
            "Manual endings should stay clear after a tee relation is created.",
        )
        self.assertTrue(stem_wall.Shape.isValid(), "Stem wall became invalid after tee join.")
        self.assertTrue(top_wall.Shape.isValid(), "Top wall became invalid after tee join.")
        self.assertLess(stem_wall.Shape.Volume, initial_stem_volume)
        self.assertAlmostEqual(top_wall.Shape.Volume, initial_top_volume, delta=1e-6)

    def test_bim_join_tee_preserves_existing_top_wall_endings(self):
        """Tests that a tee join does not clear pre-existing trims on the top wall."""
        self.printTestMessage("Testing BIM tee join preserves top wall endings...")

        existing_join_wall = self._make_line_based_wall_between(
            FreeCAD.Vector(-1000, 0, 0), FreeCAD.Vector(-1000, -1000, 0)
        )
        top_wall = self._make_line_based_wall_between(
            FreeCAD.Vector(-1000, 0, 0), FreeCAD.Vector(1000, 0, 0)
        )

        existing_path = ArchWallRelation.get_join_path(existing_join_wall)
        top_path = ArchWallRelation.get_join_path(top_wall)
        intersection, _, top_end_name = ArchWallRelation.find_best_intersection(
            existing_path,
            top_path,
        )
        _, top_plane = ArchWallRelation.calculate_butt_cutting_planes(
            existing_path,
            top_path,
            intersection,
            ArchWallRelation.get_join_section(existing_join_wall),
            ArchWallRelation.get_join_section(top_wall),
        )
        relative_top_placement = top_wall.Placement.inverse().multiply(top_plane)
        setattr(top_wall, "Ending" + top_end_name, relative_top_placement)
        self.document.removeObject(existing_join_wall.Name)
        self.document.recompute()

        self.assertFalse(
            self._is_identity_placement(top_wall.EndingStart),
            "Precondition failed: the top wall should already have a preserved ending.",
        )

        preserved_start = FreeCAD.Placement(
            top_wall.EndingStart.Base, top_wall.EndingStart.Rotation
        )
        preserved_end = FreeCAD.Placement(top_wall.EndingEnd.Base, top_wall.EndingEnd.Rotation)
        top_volume_before_tee = top_wall.Shape.Volume

        stem_wall = self._make_line_based_wall_between(
            FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 1000, 0)
        )

        self._activate_join_command(BIM_Join_Tee(), stem_wall, top_wall)

        joints = self._get_wall_joints()
        self.assertEqual(len(joints), 1)
        self.assertEqual(joints[0].JointType, "Tee")
        self.assertTrue(
            self._placements_match(top_wall.EndingStart, preserved_start),
            "The top wall start ending should be preserved across the tee join.",
        )
        self.assertTrue(
            self._placements_match(top_wall.EndingEnd, preserved_end),
            "The top wall end ending should be preserved across the tee join.",
        )
        self.assertAlmostEqual(top_wall.Shape.Volume, top_volume_before_tee, delta=1e-6)

    def test_bim_join_junction_creates_wall_junction(self):
        """Tests that the junction command creates a wall-junction relation."""
        self.printTestMessage("Testing BIM wall junction command...")

        carrier_wall = self._make_baseless_wall_between(
            FreeCAD.Vector(-1000, 0, 0), FreeCAD.Vector(1000, 0, 0)
        )
        branch_up = self._make_baseless_wall_between(
            FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 1000, 0)
        )
        branch_down = self._make_baseless_wall_between(
            FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, -1000, 0)
        )
        carrier_volume = carrier_wall.Shape.Volume
        branch_up_volume = branch_up.Shape.Volume
        branch_down_volume = branch_down.Shape.Volume

        self._activate_join_command(BIM_Join_Junction(), carrier_wall, branch_up, branch_down)

        junctions = self._get_wall_junctions()
        self.assertEqual(len(junctions), 1, "The wall junction command should create one relation.")
        junction = junctions[0]
        self.assertEqual(junction.Status, "OK")
        self.assertEqual(junction.ResolvedCarrierWall, carrier_wall)
        self.assertEqual(
            {wall.Name for wall in junction.ResolvedBranchWalls},
            {branch_up.Name, branch_down.Name},
        )
        self.assertEqual(len(self._get_wall_joints()), 0)
        self.assertAlmostEqual(carrier_wall.Shape.Volume, carrier_volume, delta=1e-6)
        self.assertLess(branch_up.Shape.Volume, branch_up_volume)
        self.assertLess(branch_down.Shape.Volume, branch_down_volume)

    def test_bim_join_junction_rejects_too_few_selected_walls(self):
        """Tests that the junction command rejects selections with fewer than three walls."""
        self.printTestMessage("Testing BIM wall junction command selection validation...")

        wall1 = self._make_baseless_wall_between(
            FreeCAD.Vector(-1000, 0, 0), FreeCAD.Vector(0, 0, 0)
        )
        wall2 = self._make_baseless_wall_between(
            FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 1000, 0)
        )
        wall1_volume = wall1.Shape.Volume
        wall2_volume = wall2.Shape.Volume

        self._activate_join_command(BIM_Join_Junction(), wall1, wall2)

        self.assertEqual(len(self._get_wall_junctions()), 0)
        self.assertEqual(len(self._get_wall_joints()), 0)
        self.assertAlmostEqual(wall1.Shape.Volume, wall1_volume, delta=1e-6)
        self.assertAlmostEqual(wall2.Shape.Volume, wall2_volume, delta=1e-6)

    def test_bim_unjoin_removes_selected_wall_junction(self):
        """Tests that Unjoin removes a directly selected wall-junction relation."""
        self.printTestMessage("Testing BIM unjoin from junction selection...")

        carrier_wall = self._make_baseless_wall_between(
            FreeCAD.Vector(-1000, 0, 0), FreeCAD.Vector(1000, 0, 0)
        )
        branch_up = self._make_baseless_wall_between(
            FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 1000, 0)
        )
        branch_down = self._make_baseless_wall_between(
            FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, -1000, 0)
        )
        branch_up_volume = branch_up.Shape.Volume
        branch_down_volume = branch_down.Shape.Volume

        self._activate_join_command(BIM_Join_Junction(), carrier_wall, branch_up, branch_down)
        junctions = self._get_wall_junctions()
        self.assertEqual(len(junctions), 1)
        self.assertEqual(junctions[0].Status, "OK")

        self._activate_join_command(BIM_Unjoin(), junctions[0])

        self.assertEqual(len(self._get_wall_junctions()), 0)
        self.assertAlmostEqual(branch_up.Shape.Volume, branch_up_volume, delta=1e-6)
        self.assertAlmostEqual(branch_down.Shape.Volume, branch_down_volume, delta=1e-6)

    def test_bim_join_junction_reports_blocking_joint_conflict(self):
        """Tests that a GUI-created junction reports a blocking wall-joint relation."""
        self.printTestMessage("Testing BIM wall junction command conflict reporting...")

        carrier_wall = self._make_baseless_wall_between(
            FreeCAD.Vector(-1000, 0, 0), FreeCAD.Vector(1000, 0, 0)
        )
        branch_up = self._make_baseless_wall_between(
            FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 1000, 0)
        )
        branch_down = self._make_baseless_wall_between(
            FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, -1000, 0)
        )

        self._activate_join_command(BIM_Join_Tee(), branch_up, carrier_wall)
        joints = self._get_wall_joints()
        self.assertEqual(len(joints), 1)
        blocker = joints[0]
        self.assertEqual(blocker.Status, "OK")

        self._activate_join_command(BIM_Join_Junction(), carrier_wall, branch_up, branch_down)

        junctions = self._get_wall_junctions()
        self.assertEqual(len(junctions), 1)
        junction = junctions[0]
        self.assertEqual(junction.Status, "Conflict")
        self.assertIn(branch_up, junction.ConflictWalls)
        self.assertIn(blocker.Label, junction.ConflictRelationLabels)
        self.assertTrue(any(blocker.Label in message for message in junction.ConflictMessages))

    def test_bim_join_rejects_multi_segment_based_wall(self):
        """Tests that join commands reject multi-segment based walls without modifying either wall."""
        self.printTestMessage("Testing BIM join rejects multi-segment wall baselines...")

        wire = Draft.makeWire(
            [
                FreeCAD.Vector(0, 0, 0),
                FreeCAD.Vector(1000, 0, 0),
                FreeCAD.Vector(1000, 1000, 0),
            ]
        )
        self.document.recompute()
        unsupported_wall = Arch.makeWall(wire, width=200, height=1500)
        supported_wall = self._make_line_based_wall_between(
            FreeCAD.Vector(1000, 0, 0), FreeCAD.Vector(1000, -1000, 0)
        )

        self._activate_join_command(BIM_Join_Miter(), unsupported_wall, supported_wall)

        self.assertEqual(
            len(self._get_wall_joints()),
            0,
            "Rejected joins should not create a relation.",
        )
        self.assertTrue(
            self._is_identity_placement(unsupported_wall.EndingStart)
            and self._is_identity_placement(unsupported_wall.EndingEnd),
            "Unsupported multi-segment walls should not receive join trims.",
        )
        self.assertTrue(
            self._is_identity_placement(supported_wall.EndingStart)
            and self._is_identity_placement(supported_wall.EndingEnd),
            "The supported wall should remain untouched when the other wall is rejected.",
        )

    # Section 1: Baseless wall joining

    @patch("draftutils.params.get_param")
    def test_baseless_wall_autojoins_as_addition(self, mock_get_param):
        """Verify baseless wall becomes an 'Addition' when AUTOJOIN is on."""
        mock_get_param.side_effect = self._get_mock_side_effect(autoJoinWalls=True, WallBaseline=0)
        self.printTestMessage("Testing baseless wall with AUTOJOIN=True...")

        wall1 = Arch.makeWall(length=1000)
        self.document.recompute()
        initial_object_count = len(self.document.Objects)

        wall2 = self._simulate_interactive_wall_creation(
            FreeCAD.Vector(1000, 0, 0), FreeCAD.Vector(1000, 1000, 0), wall1
        )

        self.assertEqual(len(self.document.Objects), initial_object_count + 1)
        self.assertIn(wall2, wall1.Additions, "New baseless wall should be in wall1's Additions.")

    @patch("draftutils.params.get_param")
    def test_baseless_wall_does_not_join_when_autojoin_is_off(self, mock_get_param):
        """Verify no relationship is created for baseless wall when AUTOJOIN is off."""
        mock_get_param.side_effect = self._get_mock_side_effect(autoJoinWalls=False, WallBaseline=0)
        self.printTestMessage("Testing baseless wall with AUTOJOIN=False...")

        wall1 = Arch.makeWall(length=1000)
        self.document.recompute()

        self._simulate_interactive_wall_creation(
            FreeCAD.Vector(1000, 0, 0), FreeCAD.Vector(1000, 1000, 0), wall1
        )

        self.assertEqual(len(wall1.Additions), 0, "No join action should have occurred.")

    # Section 2: Draft-Line-based wall joining

    @patch("draftutils.params.get_param")
    def test_line_based_wall_merges_with_joinWallSketches(self, mock_get_param):
        """Verify line-based wall performs a destructive merge."""
        mock_get_param.side_effect = self._get_mock_side_effect(
            joinWallSketches=True, WallBaseline=1
        )
        self.printTestMessage("Testing line-based wall with JOIN_SKETCHES=True...")

        line1 = Draft.makeLine(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(1000, 0, 0))
        wall1 = Arch.makeWall(line1)
        self.document.recompute()
        base1_initial_edges = len(wall1.Base.Shape.Edges)

        self._simulate_interactive_wall_creation(
            FreeCAD.Vector(1000, 0, 0), FreeCAD.Vector(1000, 1000, 0), wall1
        )

        self.assertEqual(
            len(self.document.Objects),
            3,  # In theory objects should be 2, but wall merging does not delete the original baseline
            "The new wall and its line should have been deleted.",
        )
        self.assertEqual(
            wall1.Base.TypeId,
            "Sketcher::SketchObject",
            "The base of wall1 should have been converted to a Sketch.",
        )
        self.assertGreater(
            len(wall1.Base.Shape.Edges),
            base1_initial_edges,
            "The base sketch should have more edges after the merge.",
        )

    @patch("draftutils.params.get_param")
    def test_line_based_wall_uses_autojoin_when_joinWallSketches_is_off(self, mock_get_param):
        """Verify line-based wall uses AUTOJOIN when sketch joining is off."""
        mock_get_param.side_effect = self._get_mock_side_effect(
            joinWallSketches=False, autoJoinWalls=True, WallBaseline=1
        )
        self.printTestMessage("Testing line-based wall with JOIN_SKETCHES=False, AUTOJOIN=True...")

        line1 = Draft.makeLine(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(1000, 0, 0))
        wall1 = Arch.makeWall(line1)
        self.document.recompute()
        initial_object_count = len(self.document.Objects)

        wall2 = self._simulate_interactive_wall_creation(
            FreeCAD.Vector(1000, 0, 0), FreeCAD.Vector(1000, 1000, 0), wall1
        )

        self.assertEqual(
            len(self.document.Objects),
            initial_object_count + 2,
            "A new wall and its baseline should have been created.",
        )
        self.assertIn(wall2, wall1.Additions, "The new wall should be an Addition to the first.")

    @patch("draftutils.params.get_param")
    def test_line_based_wall_falls_back_to_autojoin_on_incompatible_walls(self, mock_get_param):
        """Verify fallback to AUTOJOIN for incompatible line-based walls."""
        mock_get_param.side_effect = self._get_mock_side_effect(
            joinWallSketches=True, autoJoinWalls=True, WallBaseline=1
        )
        self.printTestMessage("Testing line-based wall fallback to AUTOJOIN...")

        line1 = Draft.makeLine(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(1000, 0, 0))
        wall1 = Arch.makeWall(line1, width=200)  # Incompatible width
        self.document.recompute()

        wall2 = self._simulate_interactive_wall_creation(
            FreeCAD.Vector(1000, 0, 0),
            FreeCAD.Vector(1000, 1000, 0),
            wall1,
            wall_width=300,
        )

        self.assertIn(wall2, wall1.Additions, "Fallback failed; wall should be an Addition.")

    # Section 3: Sketch-based wall joining

    @patch("draftutils.params.get_param")
    def test_sketch_based_wall_merges_with_joinWallSketches(self, mock_get_param):
        """Verify sketch-based wall performs a destructive merge."""
        mock_get_param.side_effect = self._get_mock_side_effect(
            joinWallSketches=True, WallBaseline=2
        )
        self.printTestMessage("Testing sketch-based wall with JOIN_SKETCHES=True...")

        sketch1 = self.document.addObject("Sketcher::SketchObject", "Sketch1")
        sketch1.addGeometry(Part.LineSegment(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(1000, 0, 0)))
        wall1 = Arch.makeWall(sketch1)
        self.document.recompute()
        base1_initial_edges = len(wall1.Base.Shape.Edges)

        self._simulate_interactive_wall_creation(
            FreeCAD.Vector(1000, 0, 0), FreeCAD.Vector(1000, 1000, 0), wall1
        )

        self.assertEqual(
            len(self.document.Objects),
            2,
            "The new wall and its sketch should have been deleted.",
        )
        self.assertGreater(
            len(wall1.Base.Shape.Edges),
            base1_initial_edges,
            "The base sketch should have more edges after the merge.",
        )

    @patch("draftutils.params.get_param")
    def test_sketch_based_wall_uses_autojoin_when_joinWallSketches_is_off(self, mock_get_param):
        """Verify sketch-based wall uses AUTOJOIN when sketch joining is off."""
        mock_get_param.side_effect = self._get_mock_side_effect(
            joinWallSketches=False, autoJoinWalls=True, WallBaseline=2
        )
        self.printTestMessage(
            "Testing sketch-based wall with JOIN_SKETCHES=False, AUTOJOIN=True..."
        )

        sketch1 = self.document.addObject("Sketcher::SketchObject", "Sketch1")
        sketch1.addGeometry(Part.LineSegment(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(1000, 0, 0)))
        wall1 = Arch.makeWall(sketch1)
        self.document.recompute()

        wall2 = self._simulate_interactive_wall_creation(
            FreeCAD.Vector(1000, 0, 0), FreeCAD.Vector(1000, 1000, 0), wall1
        )

        self.assertIn(wall2, wall1.Additions, "The new wall should be an Addition to the first.")

    @patch("draftutils.params.get_param")
    def test_sketch_based_wall_falls_back_to_autojoin_on_incompatible_walls(self, mock_get_param):
        """Verify fallback to AUTOJOIN for incompatible sketch-based walls."""
        mock_get_param.side_effect = self._get_mock_side_effect(
            joinWallSketches=True, autoJoinWalls=True, WallBaseline=2
        )
        self.printTestMessage("Testing sketch-based wall fallback to AUTOJOIN...")

        sketch1 = self.document.addObject("Sketcher::SketchObject", "Sketch1")
        sketch1.addGeometry(Part.LineSegment(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(1000, 0, 0)))
        wall1 = Arch.makeWall(sketch1, width=200)  # Incompatible width
        self.document.recompute()

        wall2 = self._simulate_interactive_wall_creation(
            FreeCAD.Vector(1000, 0, 0),
            FreeCAD.Vector(1000, 1000, 0),
            wall1,
            wall_width=300,
        )

        self.assertIn(wall2, wall1.Additions, "Fallback failed; wall should be an Addition.")

    @patch("draftutils.params.get_param")
    def test_no_join_action_when_prefs_are_off(self, mock_get_param):
        """Verify no join action occurs when both preferences are off."""
        mock_get_param.side_effect = self._get_mock_side_effect(
            joinWallSketches=False, autoJoinWalls=False, WallBaseline=1
        )
        self.printTestMessage("Testing no join action when preferences are off...")

        # Test with a based wall
        line1 = Draft.makeLine(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(1000, 0, 0))
        wall1 = Arch.makeWall(line1)
        self.document.recompute()
        initial_object_count = len(self.document.Objects)

        self._simulate_interactive_wall_creation(
            FreeCAD.Vector(1000, 0, 0), FreeCAD.Vector(1000, 1000, 0), wall1
        )

        self.assertEqual(len(self.document.Objects), initial_object_count + 2)
        self.assertEqual(
            len(wall1.Additions),
            0,
            "No join action should have occurred for based wall.",
        )
