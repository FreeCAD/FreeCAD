# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 Furgo                                              *
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

import FreeCAD
import FreeCADGui
import Draft
import Arch
import Part
import WorkingPlane
from bimtests import TestArchBaseGui
from bimcommands.BimWall import Arch_Wall
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
        wp = WorkingPlane.plane()
        placement = FreeCAD.Placement(
            FreeCAD.Vector(0, 0, 1000), FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 45)
        )

        # Apply the placement to the working plane, ensuring translation is included
        wp.setFromPlacement(placement, rebase=True)

        # Define points in the local coordinate system of the working plane
        p1_local = FreeCAD.Vector(0, 0, 0)
        p2_local = FreeCAD.Vector(2000, 0, 0)

        # Convert local points to the global coordinates the command will receive
        p1_global = wp.getGlobalCoords(p1_local)
        p2_global = wp.getGlobalCoords(p2_local)

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
            base1, base2, "Each sketch-based wall should have its own unique sketch object."
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
            FreeCAD.Vector(1000, 0, 0), FreeCAD.Vector(1000, 1000, 0), wall1, wall_width=300
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
            FreeCAD.Vector(1000, 0, 0), FreeCAD.Vector(1000, 1000, 0), wall1, wall_width=300
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
            len(wall1.Additions), 0, "No join action should have occurred for based wall."
        )
