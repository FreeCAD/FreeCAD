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

import FreeCAD
from bimtests import TestArchBase
import Arch
import ArchWindow  # For ArchWindow._Window proxy class
import Part
import Draft
import Sketcher


class TestArchWindow(TestArchBase.TestArchBase):

    def test_create_no_args(self):
        """Test creating a window with no arguments."""
        window = Arch.makeWindow(name="Window_NoArgs")
        self.assertIsNotNone(window)
        self.assertTrue(
            hasattr(window, "Proxy") and isinstance(window.Proxy, ArchWindow._Window),
            "Window proxy is not of expected ArchWindow._Window type",
        )
        self.assertEqual(window.Label, "Window_NoArgs")
        self.assertEqual(window.IfcType, "Window")
        self.assertIsNone(window.Base)
        self.assertEqual(len(window.WindowParts), 0)
        self.assertTrue(window.Shape.isNull())
        self.document.recompute()
        self.assertTrue(window.Shape.isNull())

    def test_create_from_sketch_single_wire_default_parts(self):
        """Test creating a window from a single-wire sketch, relying on default parts."""
        sketch = self._create_sketch_with_wires("SketchSingle", [(0, 0, 1000, 1200)])
        window = Arch.makeWindow(baseobj=sketch, name="Window_SketchSingle_Default")
        self.document.recompute()

        self.assertEqual(window.Base, sketch)
        self.assertTrue(
            len(window.WindowParts) >= 5, "Should have at least one default part (5 elements)."
        )
        self.assertEqual(window.WindowParts[0], "Default")
        self.assertEqual(
            window.WindowParts[1],
            "Solid panel",
            "Default part type incorrect for single-wire sketch.",
        )
        self.assertIn("Wire0", window.WindowParts[2])
        self.assertFalse(window.Shape.isNull())
        self.assertGreater(len(window.Shape.Solids), 0)

    def test_create_from_sketch_two_wires_default_parts(self):
        """Test creating a window from two-wire sketch (concentric), relying on default parts."""
        sketch = self._create_sketch_with_wires(
            "SketchTwoWires", [(0, 0, 1000, 1200), (100, 100, 800, 1000)]
        )
        window = Arch.makeWindow(baseobj=sketch, name="Window_SketchTwo_Default")
        self.document.recompute()

        self.assertEqual(window.Base, sketch)
        self.assertGreaterEqual(len(window.WindowParts), 5)
        self.assertEqual(window.WindowParts[0], "Default")
        self.assertEqual(
            window.WindowParts[1], "Frame", "Default type for multi-wire should be Frame."
        )
        self.assertIn("Wire0", window.WindowParts[2])
        self.assertIn("Wire1", window.WindowParts[2])
        self.assertFalse(window.Shape.isNull())
        self.assertGreater(len(window.Shape.Solids), 0)

    def test_sketch_named_constraints_driven_by_window_props(self):
        """Test that window Width/Height properties drive sketch's named constraints."""
        sketch_width, sketch_height = 800.0, 1000.0
        sketch = self._create_sketch_with_named_constraints(
            "SketchNamed", sketch_width, sketch_height
        )

        window = Arch.makeWindow(baseobj=sketch, name="Window_NamedSketch")
        # Set window Width/Height to ensure they become the drivers if sketch also has the constraints.
        # They need to be set explicitly after creating the window, as they are not automatically initialized
        # from the sketch.
        window.Width = sketch_width
        window.Height = sketch_height
        self.document.recompute()

        self.assertEqual(sketch.getDatum("Width").Value, sketch_width)
        self.assertEqual(sketch.getDatum("Height").Value, sketch_height)

        new_win_width = 1200.0
        window.Width = new_win_width
        self.document.recompute()
        self.assertEqual(
            sketch.getDatum("Width").Value,
            new_win_width,
            "Window.Width should drive sketch 'Width' constraint.",
        )

        new_win_height = 1500.0
        window.Height = new_win_height
        self.document.recompute()
        self.assertEqual(
            sketch.getDatum("Height").Value,
            new_win_height,
            "Window.Height should drive sketch 'Height' constraint.",
        )

    def test_create_from_sketch_with_custom_parts(self):
        """Test creating a window from sketch with explicit custom parts."""
        sketch = self._create_sketch_with_wires(
            "SketchCustom", [(0, 0, 1200, 1000), (100, 100, 1000, 800)]
        )
        custom_parts = [
            "OuterFrame",
            "Frame",
            "Wire0,Wire1",
            "70",
            "0",
            "GlassPane",
            "Glass panel",
            "Wire1",
            "20",
            "25",
        ]
        window = Arch.makeWindow(baseobj=sketch, parts=custom_parts, name="Window_Custom")
        self.document.recompute()

        self.assertEqual(window.Base, sketch)
        self.assertEqual(list(window.WindowParts), custom_parts)
        self.assertFalse(window.Shape.isNull())
        self.assertEqual(len(window.Shape.Solids), 2, "Expected two solids for frame and glass.")

    def test_custom_parts_with_plus_v_references(self):
        """Test creating a window with custom parts using '+V' to reference window.Frame and window.Offset."""
        sketch = self._create_sketch_with_wires("SketchPlusV", [(0, 0, 1000, 800)])
        frame_val = 60.0
        offset_val = 10.0

        custom_parts_plus_v = ["MainFrame", "Frame", "Wire0", "50+V", "5+V"]
        window = Arch.makeWindow(baseobj=sketch, parts=custom_parts_plus_v, name="Window_PlusV")
        window.Frame = frame_val
        window.Offset = offset_val
        self.document.recompute()

        self.assertFalse(window.Shape.isNull())
        self.assertGreater(len(window.Shape.Solids), 0)

    def test_create_with_width_height_no_baseobj_initially(self):
        """
        Test Arch.makeWindow(width, height) current behavior regarding window.Base.
        It verifies that window.Base is not automatically created and remains None,
        and that Width/Height properties are correctly set on the window object.
        """
        win_w, win_h = 1000.0, 1200.0
        window_name = "Window_W_H_NoAutoBase"
        window = Arch.makeWindow(width=win_w, height=win_h, name=window_name)

        # 1. Check initial state after makeWindow call
        self.assertEqual(window.Label, window_name)
        self.assertIsNone(
            window.Base, "Immediately after makeWindow(W,H), window.Base should be None."
        )
        self.assertTrue(
            window.Shape.isNull(), "Initially, window.Shape should be null as no Base or Parts yet."
        )
        self.assertAlmostEqual(
            window.Width.Value,
            win_w,
            places=5,
            msg="Window.Width property not correctly set by makeWindow.",
        )
        self.assertAlmostEqual(
            window.Height.Value,
            win_h,
            places=5,
            msg="Window.Height property not correctly set by makeWindow.",
        )

        # 2. Perform a document recompute
        # This should trigger window.execute() -> ArchComponent.ensureBase()
        self.document.recompute()

        # 3. Assert the CURRENT BEHAVIOR: window.Base remains None
        # The original ArchComponent.ensureBase does not create a sketch if Base is None
        # and only Width/Height are provided to the object.
        self.assertIsNone(
            window.Base,
            "Current Behavior: window.Base should remain None even after recomputes for makeWindow(W,H).",
        )

        # 4. Consequently, window.Shape should still be null.
        self.assertTrue(
            window.Shape.isNull(), "window.Shape should remain null if window.Base was not created."
        )

        # 5. Attempting to set parts that rely on a Base sketch (e.g., "Wire0")
        #    should not produce a shape if Base is None.
        window.WindowParts = ["Default", "Frame", "Wire0", "50", "0"]
        self.document.recompute()
        self.assertTrue(
            window.Shape.isNull(),
            "window.Shape should still be null if WindowParts reference Wire0 from a non-existent Base.",
        )

    def test_window_in_wall_creates_opening(self):
        """
        Test if a window hosted in a wall creates a geometric opening,
        verifying changes in Volume and VerticalArea.
        Manually sets win.Width and win.Height after Arch.makeWindow(sk)
        as current Arch.makeWindow(sk) behavior does not initialize these
        properties from the sketch. It expects win.Placement to remain identity.
        """
        # 1. Create Wall
        wall_length = 3000.0
        wall_thickness = 200.0
        wall_height = 2400.0

        line = Draft.makeLine(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(wall_length, 0, 0))
        self.document.recompute()

        wall = Arch.makeWall(
            line,
            width=wall_thickness,
            height=wall_height,
            align="Left",
            name="TestWall_ForOpening_Args",
        )
        self.document.recompute()

        initial_wall_volume = wall.Shape.Volume
        initial_wall_vertical_area = wall.VerticalArea.Value

        # 2. Create Sketch for Window profile
        sketch_profile_width = 800.0
        sketch_profile_height = 1000.0
        sk = self._create_sketch_with_wires(
            "WindowSketch_Args", [(0, 0, sketch_profile_width, sketch_profile_height)]
        )

        # Position and orient the sketch in 3D space
        sk.Placement.Rotation = FreeCAD.Rotation(FreeCAD.Vector(1, 0, 0), 90)
        win_global_x_start = (wall.Length.Value - sketch_profile_width) / 2
        win_global_z_start = 900.0
        sk.Placement.Base = FreeCAD.Vector(win_global_x_start, 0, win_global_z_start)
        self.document.recompute()

        # 3. Create Window from sketch
        win = Arch.makeWindow(sk, name="WindowInWall_Args")

        # Manually set Width and Height to match the sketch profile dimensions
        win.Width = sketch_profile_width
        win.Height = sketch_profile_height
        # win.Placement remains identity

        win.HoleDepth = 0  # Use "smart" hole depth calculation
        win.WindowParts = ["DefaultFrame", "Frame", "Wire0", "60", "0"]

        win.recompute()
        self.document.recompute()

        self.assertFalse(win.Shape.isNull(), "Window must have a valid shape.")
        self.assertEqual(
            win.Placement,
            FreeCAD.Placement(),
            f"Window.Placement should be identity; got {win.Placement}",
        )
        self.assertAlmostEqual(win.Width.Value, sketch_profile_width, 5)
        self.assertAlmostEqual(win.Height.Value, sketch_profile_height, 5)

        # 4. Add Window to Wall
        Arch.addComponents(win, host=wall)
        self.document.recompute()

        # 5. Assertions
        # Wall.Subtractions PropertyLinkList check (informational, expected to be empty)
        wall_subtractions_prop = wall.Subtractions if hasattr(wall, "Subtractions") else []
        self.assertEqual(
            len(wall_subtractions_prop),
            0,
            f"Wall.Subtractions property list expected to be empty, but: "
            f"{wall_subtractions_prop}",
        )

        # Geometric checks
        current_wall_volume = wall.Shape.Volume
        self.assertLess(
            current_wall_volume,
            initial_wall_volume,
            f"Wall volume should decrease. Before: {initial_wall_volume}, "
            f"After: {current_wall_volume}",
        )

        ideal_removed_volume = win.Width.Value * win.Height.Value * wall.Width.Value
        self.assertAlmostEqual(
            current_wall_volume,
            initial_wall_volume - ideal_removed_volume,
            places=3,
            msg=f"Wall volume cut not as expected. Initial: {initial_wall_volume}, "
            f"Current: {current_wall_volume}, IdealRemoved: {ideal_removed_volume}",
        )

        # Check VerticalArea
        current_wall_vertical_area = wall.VerticalArea.Value

        window_opening_area_on_one_face = win.Width.Value * win.Height.Value
        area_of_side_reveals = 2 * win.Height.Value * wall.Width.Value
        expected_wall_vertical_area_after = (
            initial_wall_vertical_area
            - (2 * window_opening_area_on_one_face)
            + area_of_side_reveals
        )

        self.assertAlmostEqual(
            current_wall_vertical_area,
            expected_wall_vertical_area_after,
            places=3,
            msg=f"Wall VerticalArea change not as expected. Initial: "
            f"{initial_wall_vertical_area}, Current: {current_wall_vertical_area}, "
            f"ExpectedAfter: {expected_wall_vertical_area_after}",
        )

    def test_clone_window(self):
        """Test cloning an Arch.Window object.

        Notes:
        - The clone's name is automatically generated, the `name` argument is ignored.
        - The clone's WindowParts, SillHeight and other properties are always empty, despite the original having them.

        """
        sketch = self._create_sketch_with_wires("OriginalSketch", [(0, 0, 600, 800)])
        original_parts = ["MainFrame", "Frame", "Wire0", "50", "10"]
        original_window = Arch.makeWindow(
            baseobj=sketch, parts=original_parts, name="OriginalWindow"
        )
        original_window.Frame = 60.0
        original_window.SillHeight = 100.0
        self.document.recompute()

        self.assertEqual(original_window.Label, "OriginalWindow")

        cloned_window = Arch.makeWindow(baseobj=original_window)
        self.document.recompute()

        self.assertIsNotNone(cloned_window)
        self.assertIsNotNone(cloned_window.CloneOf)
        self.assertEqual(cloned_window.CloneOf, original_window)

        self.assertEqual(cloned_window.IfcType, original_window.IfcType)

        self.assertFalse(cloned_window.Shape.isNull())
        self.assertAlmostEqual(
            cloned_window.Shape.Volume,
            original_window.Shape.Volume,
            delta=1e-5,
            msg="Cloned window volume should match original.",
        )

    def test_create_window_on_xz_plane(self):
        """Test creating a window oriented on the XZ (vertical) plane."""

        # Create a a frame-like profile.
        sketch = self._create_sketch_with_wires(
            "Sketch_XZ_Plane", [(0, 0, 1000, 1200), (100, 100, 800, 1000)]
        )

        # Orient the sketch to the XZ plane.
        sketch.Placement.Rotation = FreeCAD.Rotation(FreeCAD.Vector(1, 0, 0), 90)
        self.document.recompute()

        # Create the window using explicit parts.
        window = Arch.makeWindow(baseobj=sketch, name="Window_XZ_Plane")
        window.WindowParts = [
            "Frame",
            "Frame",
            "Wire0,Wire1",
            "60",
            "0",  # A 60mm thick frame
            "Glass",
            "Glass panel",
            "Wire1",
            "10",
            "25",  # A 10mm thick glass pane, offset by 25mm
        ]
        self.document.recompute()

        # Check the resulting geometry's orientation and dimensions.
        self.assertFalse(window.Shape.isNull(), "Window shape should not be null.")
        self.assertEqual(
            len(window.Shape.Solids), 2, "Window should contain two solids (frame and glass)."
        )

        bb = window.Shape.BoundBox

        # The window's overall "thickness" is determined by the frame (60mm).
        # Its "width" (X) and "height" (Z) should be larger.
        self.assertGreater(
            bb.XLength,
            bb.YLength,
            "Window XLength (width) should be greater than YLength (thickness).",
        )
        self.assertGreater(
            bb.ZLength,
            bb.YLength,
            "Window ZLength (height) should be greater than YLength (thickness).",
        )

        # Verify the overall thickness is correct (60mm, from the Frame component).
        self.assertAlmostEqual(
            bb.YLength, 60.0, places=5, msg="Window thickness (YLength) is incorrect."
        )

    def test_opening_property_rotates_component(self):
        """Test that setting the Opening property rotates a hinged component."""
        # Create a window from a preset with a hinge
        window = Arch.makeWindowPreset(
            "Open 1-pane", width=1000, height=1200, h1=50, h2=50, h3=0, w1=100, w2=50, o1=0, o2=50
        )
        self.document.recompute()

        # The preset creates an outer frame, an inner opening frame, and glass.
        self.assertEqual(len(window.Shape.Solids), 3, "Preset should create three solids.")

        # Solid[1] is the inner, opening frame.
        opening_pane = window.Shape.Solids[1]

        # Get initial position
        initial_center = opening_pane.CenterOfMass

        # Set opening to 50%. The 'Opening' property is App::PropertyPercent, which expects an integer.
        window.Opening = 50

        self.document.recompute()

        # Get new position of the same component
        new_opening_pane = window.Shape.Solids[1]
        new_center = new_opening_pane.CenterOfMass

        # Assert that the Z-coordinate has changed, indicating rotation around a horizontal hinge.
        self.assertNotAlmostEqual(
            initial_center.z,
            new_center.z,
            places=3,
            msg="The Z-coordinate of the opening pane's center should change upon rotation.",
        )
        # The Y-coordinate should remain largely unchanged for a bottom-hinged (awning-style) window.
        self.assertAlmostEqual(initial_center.y, new_center.y, places=3)

    def test_symbol_plan_creates_wire_geometry(self):
        """Test that enabling SymbolPlan adds 2D wire geometry to the window's shape."""
        # Create a window with an opening mode
        window = Arch.makeWindowPreset(
            "Open 1-pane", width=1000, height=1200, h1=50, h2=50, h3=0, w1=100, w2=50, o1=0, o2=50
        )

        # SymbolPlan is True by default for presets with hinges, but we set it explicitly
        window.SymbolPlan = True

        self.document.recompute()

        # Assert that the shape contains both solids and the 2D symbol edges.
        self.assertIsInstance(window.Shape, Part.Compound, "Window shape should be a compound.")

        # Find symbol edges by finding edges that do not belong to any face in the shape
        face_edge_hashes = {e.hashCode() for face in window.Shape.Faces for e in face.Edges}
        all_edge_hashes = {e.hashCode() for e in window.Shape.Edges}
        symbol_edge_hashes = all_edge_hashes - face_edge_hashes

        self.assertEqual(len(window.Shape.Solids), 3, "Window shape should contain 3 solids.")
        self.assertEqual(
            len(symbol_edge_hashes), 2, "Expected 2 edges for the plan symbol (arc and line)."
        )

    def test_custom_subvolume_creates_opening(self):
        """Test that a custom Subvolume shape correctly creates an opening in a host wall."""
        # Create a wall and store its initial state
        wall_base = Draft.makeLine(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(4000, 0, 0))
        wall = Arch.makeWall(wall_base, width=200, height=3000, align="Left")
        self.document.recompute()
        initial_wall_volume = wall.Shape.Volume
        initial_wall_shape = (
            wall.Shape.copy()
        )  # Store initial shape for accurate intersection calculation

        # Create a simple window
        window_sketch = self._create_sketch_with_wires("WinSketch", [(0, 0, 10, 10)])
        window = Arch.makeWindow(window_sketch)

        # Create a custom cutting box as the window's Subvolume
        cutting_box = self.document.addObject("Part::Box", "CuttingBox")
        cutting_box.Length = 800
        cutting_box.Width = 300  # Wider than wall
        cutting_box.Height = 1000
        cutting_box.Placement.Base = FreeCAD.Vector(1600, -50, 800)
        self.document.recompute()

        window.Subvolume = cutting_box

        # Add window to wall to create opening using Arch.addComponents
        Arch.addComponents(window, host=wall)
        self.document.recompute()

        # Assert volume has decreased by the intersecting part of the box's volume
        expected_volume_change = cutting_box.Shape.common(initial_wall_shape).Volume
        final_wall_volume = wall.Shape.Volume
        self.assertAlmostEqual(
            final_wall_volume,
            initial_wall_volume - expected_volume_change,
            places=3,
            msg="Wall volume did not decrease correctly after applying custom Subvolume.",
        )

    def test_door_preset_sets_ifctype_door(self):
        """Test that creating a window from a 'door' preset correctly sets its IfcType to 'Door'."""
        # Create a door using a preset with non-zero values for all required parameters
        door = Arch.makeWindowPreset(
            "Simple door", width=900, height=2100, h1=50, h2=50, h3=0, w1=100, w2=40, o1=0, o2=0
        )
        self.assertIsNotNone(door, "makeWindowPreset for a door should create an object.")
        self.document.recompute()

        # Assert IfcType is "Door"
        self.assertEqual(door.IfcType, "Door", "IfcType should be 'Door' for a door preset.")

        # Assert component count
        # A simple door preset has an outer frame and a solid door panel
        self.assertEqual(
            len(door.Shape.Solids),
            2,
            "A simple door should have 2 solid components (frame and panel).",
        )

    def test_cloned_window_in_wall_creates_opening(self):
        """Tests if a cloned Arch.Window, when hosted in a wall, creates a geometric opening."""

        # Create the host wall
        wall_length = 3000.0
        wall_thickness = 200.0
        wall_height = 2500.0
        wall_base = Draft.makeLine(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(wall_length, 0, 0))
        wall = Arch.makeWall(
            wall_base, width=wall_thickness, height=wall_height, name="WallForClonedWindow"
        )
        self.document.recompute()
        initial_wall_volume = wall.Shape.Volume
        self.assertGreater(initial_wall_volume, 0, "Wall should have a positive volume initially.")

        # Create the original window's sketch
        window_width = 800.0
        window_height = 1200.0
        original_sketch = self._create_sketch_with_wires(
            "OriginalWinSketch", [(0, 0, window_width, window_height)]
        )

        # Orient the sketch to be vertical before creating the window
        # This mimics the behavior of the interactive Arch_Window tool.
        original_sketch.Placement.Rotation = FreeCAD.Rotation(FreeCAD.Vector(1, 0, 0), 90)
        self.document.recompute()

        # Create the original window from the now-oriented sketch
        original_window = Arch.makeWindow(baseobj=original_sketch, name="OriginalWindow")
        original_window.Width = window_width  # Explicitly set properties
        original_window.Height = window_height
        self.document.recompute()
        self.assertFalse(
            original_window.Shape.isNull(), "Original window shape should not be null."
        )

        # Create the clone
        cloned_window = Draft.clone(original_window)
        cloned_window.Label = "ClonedWindow"

        # Position the clone inside the wall
        clone_x = 1100.0  # Center of the clone in the wall's length
        clone_y = wall_thickness / 2  # Center of the clone in the wall's thickness
        clone_z = 800.0  # Sill height
        cloned_window.Placement.Base = FreeCAD.Vector(clone_x, clone_y, clone_z)
        self.document.recompute()

        self.assertIsNotNone(
            cloned_window.CloneOf, "Cloned window should have CloneOf property set."
        )
        self.assertEqual(
            cloned_window.CloneOf, original_window, "CloneOf should point to the original window."
        )
        self.assertAlmostEqual(cloned_window.Shape.Volume, original_window.Shape.Volume, delta=1e-5)

        # Add the clone to the wall's hosts
        Arch.addComponents(cloned_window, host=wall)
        self.document.recompute()

        self.assertIn(
            wall, cloned_window.Hosts, "Wall should be in the cloned window's Hosts list."
        )

        final_wall_volume = wall.Shape.Volume
        self.assertLess(
            final_wall_volume,
            initial_wall_volume,
            "Wall volume should have decreased after hosting the cloned window.",
        )

        expected_removed_volume = window_width * window_height * wall.Width.Value
        self.assertAlmostEqual(
            initial_wall_volume - final_wall_volume,
            expected_removed_volume,
            delta=1e-5,
            msg="The volume removed from the wall by the cloned window is incorrect.",
        )

    def test_addComponents_window_to_wall(self):
        """
        Tests the Arch.addComponents function for adding a window to a wall.
        Verifies that the Hosts, InList, and OutList properties are correctly
        updated, and that the geometric opening is created.
        """
        self.printTestMessage("Testing Arch.addComponents for window-wall hosting...")

        # Create the wall and window
        wall_base = Draft.makeLine(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(3000, 0, 0))
        wall = Arch.makeWall(wall_base, width=200, height=2500, name="HostWall")
        self.document.recompute()
        initial_wall_volume = wall.Shape.Volume

        window_width = 800.0
        window_height = 1200.0
        window_sketch = self._create_sketch_with_wires(
            "WindowSketchForAdd", [(0, 0, window_width, window_height)]
        )
        window_sketch.Placement.Rotation = FreeCAD.Rotation(FreeCAD.Vector(1, 0, 0), 90)
        window_sketch.Placement.Base = FreeCAD.Vector(1100, 100, 800)  # Position it within the wall
        self.document.recompute()

        window = Arch.makeWindow(baseobj=window_sketch, name="HostedWindow")
        window.Width = window_width
        window.Height = window_height
        self.document.recompute()

        # Initial state verification
        self.assertEqual(len(window.Hosts), 0, "Window.Hosts should be empty initially.")
        self.assertNotIn(window, wall.InList, "Window should not be in wall's InList initially.")

        # Add the window to the wall
        Arch.addComponents(window, wall)
        self.document.recompute()

        # Check all relationships and the final geometry
        # Property assertions
        self.assertIn(wall, window.Hosts, "Wall should be in window.Hosts after addComponents.")
        self.assertEqual(len(window.Hosts), 1, "Window.Hosts should contain exactly one host.")

        # Dependency graph assertions
        self.assertIn(window, wall.InList, "Window should be in wall's InList after hosting.")
        self.assertIn(wall, window.OutList, "Wall should be in window's OutList after hosting.")

        # Negative assertion
        self.assertNotIn(
            window,
            wall.Subtractions,
            "Window should not be in wall's Subtractions list for this hosting mechanism.",
        )

        # Geometric assertion
        final_wall_volume = wall.Shape.Volume
        self.assertLess(
            final_wall_volume,
            initial_wall_volume,
            "Wall volume should decrease after hosting the window.",
        )

        expected_removed_volume = window_width * window_height * wall.Width.Value
        self.assertAlmostEqual(
            initial_wall_volume - final_wall_volume,
            expected_removed_volume,
            delta=1e-5,
            msg="The volume removed from the wall is incorrect.",
        )

    def _create_sketch_with_wires(
        self, name: str, wire_definitions: list[tuple[float, float, float, float]]
    ) -> "FreeCAD.DocumentObject":
        """
        Helper to create a sketch with one or more specified rectangular wires.

        Each rectangle is defined in the sketch's local XY plane. The sketch
        is created in the current document (`self.doc`). After adding all
        geometry and basic coincident constraints for each rectangle, the
        document is recomputed.

        Parameters
        ----------
        name : str
            The name for the new Sketcher.SketchObject.
        wire_definitions : list[tuple[float, float, float, float]]
            A list where each element defines one rectangular wire.
            Each tuple within the list should be in the format `(x, y, w, h)`:
            - `x` (float): The x-coordinate of the bottom-left corner of the rectangle
                           in the sketch's local coordinate system.
            - `y` (float): The y-coordinate of the bottom-left corner of the rectangle
                           in the sketch's local coordinate system.
            - `w` (float): The width of the rectangle (along the sketch's local X-axis).
            - `h` (float): The height of the rectangle (along the sketch's local Y-axis).
            For example, `[(0, 0, 100, 200)]` creates one 100x200 rectangle at the
            sketch origin. `[(0,0,100,100), (10,10,80,80)]` creates two concentric
            rectangles if used as outer and inner boundaries. The order of wires
            in this list corresponds to `Wire0`, `Wire1`, etc., when used by
            Arch objects. The sketch is recomputed and returned.

        Returns
        -------
        FreeCAD.DocumentObject
            The created Sketcher object (specifically, an object of type "Sketcher::SketchObject").
        """
        sketch: FreeCAD.DocumentObject = self.document.addObject("Sketcher::SketchObject", name)

        for i, (x, y, w, h) in enumerate(wire_definitions):
            sketch.addGeometry(
                Part.LineSegment(FreeCAD.Vector(x, y, 0), FreeCAD.Vector(x + w, y, 0))
            )
            sketch.addGeometry(
                Part.LineSegment(FreeCAD.Vector(x + w, y, 0), FreeCAD.Vector(x + w, y + h, 0))
            )
            sketch.addGeometry(
                Part.LineSegment(FreeCAD.Vector(x + w, y + h, 0), FreeCAD.Vector(x, y + h, 0))
            )
            sketch.addGeometry(
                Part.LineSegment(FreeCAD.Vector(x, y + h, 0), FreeCAD.Vector(x, y, 0))
            )
            base_idx = i * 4
            sketch.addConstraint(Sketcher.Constraint("Coincident", base_idx, 2, base_idx + 1, 1))
            sketch.addConstraint(
                Sketcher.Constraint("Coincident", base_idx + 1, 2, base_idx + 2, 1)
            )
            sketch.addConstraint(
                Sketcher.Constraint("Coincident", base_idx + 2, 2, base_idx + 3, 1)
            )
            sketch.addConstraint(Sketcher.Constraint("Coincident", base_idx + 3, 2, base_idx, 1))

        self.document.recompute()

        return sketch

    def _create_sketch_with_named_constraints(
        self, name: str, initial_width: float, initial_height: float
    ) -> "FreeCAD.DocumentObject":
        """
        Helper to create a rectangular sketch with "Width" and "Height" named constraints.

        The sketch is created in the current document (`self.doc`) on the
        default XY plane. It consists of a single rectangle defined by four
        line segments, with its bottom-left corner at (0,0,0) in the
        sketch's local coordinates.

        Parameters
        ----------
        name : str
            The name for the new Sketcher.SketchObject.
        initial_width : float
            The initial value for the "Width" constraint (along sketch's X-axis).
        initial_height : float
            The initial value for the "Height" constraint (along sketch's Y-axis).

        Returns
        -------
        FreeCAD.DocumentObject
            The created Sketcher object (specifically, an object of type "Sketcher::SketchObject").
        """
        sketch: FreeCAD.DocumentObject = self.document.addObject("Sketcher::SketchObject", name)

        sketch.addGeometry(
            Part.LineSegment(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(initial_width, 0, 0)), False
        )
        sketch.addGeometry(
            Part.LineSegment(
                FreeCAD.Vector(initial_width, 0, 0),
                FreeCAD.Vector(initial_width, initial_height, 0),
            ),
            False,
        )
        sketch.addGeometry(
            Part.LineSegment(
                FreeCAD.Vector(initial_width, initial_height, 0),
                FreeCAD.Vector(0, initial_height, 0),
            ),
            False,
        )
        sketch.addGeometry(
            Part.LineSegment(FreeCAD.Vector(0, initial_height, 0), FreeCAD.Vector(0, 0, 0)), False
        )
        sketch.addConstraint(Sketcher.Constraint("Coincident", 0, 2, 1, 1))
        sketch.addConstraint(Sketcher.Constraint("Coincident", 1, 2, 2, 1))
        sketch.addConstraint(Sketcher.Constraint("Coincident", 2, 2, 3, 1))
        sketch.addConstraint(Sketcher.Constraint("Coincident", 3, 2, 0, 1))
        sketch.addConstraint(Sketcher.Constraint("Horizontal", 0))
        sketch.addConstraint(Sketcher.Constraint("Vertical", 1))
        sketch.addConstraint(Sketcher.Constraint("Horizontal", 2))
        sketch.addConstraint(Sketcher.Constraint("Vertical", 3))

        sketch.addConstraint(Sketcher.Constraint("DistanceX", 0, 1, 0, 2, initial_width))
        sketch.renameConstraint(sketch.ConstraintCount - 1, "Width")
        sketch.addConstraint(Sketcher.Constraint("DistanceY", 1, 1, 1, 2, initial_height))
        sketch.renameConstraint(sketch.ConstraintCount - 1, "Height")

        self.document.recompute()

        return sketch
