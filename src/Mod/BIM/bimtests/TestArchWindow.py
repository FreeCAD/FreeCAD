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
import ArchWindow # For ArchWindow._Window proxy class
import Part
import Draft
import Sketcher

class TestArchWindow(TestArchBase.TestArchBase):

    def test_create_no_args(self):
        """Test creating a window with no arguments."""
        window = Arch.makeWindow(name="Window_NoArgs")
        self.assertIsNotNone(window)
        self.assertTrue(hasattr(window, "Proxy") and isinstance(window.Proxy, ArchWindow._Window),
                        "Window proxy is not of expected ArchWindow._Window type")
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
        self.assertTrue(len(window.WindowParts) >= 5, "Should have at least one default part (5 elements).")
        self.assertEqual(window.WindowParts[0], "Default")
        self.assertEqual(window.WindowParts[1], "Solid panel", "Default part type incorrect for single-wire sketch.")
        self.assertIn("Wire0", window.WindowParts[2])
        self.assertFalse(window.Shape.isNull())
        self.assertTrue(len(window.Shape.Solids) > 0)

    def test_create_from_sketch_two_wires_default_parts(self):
        """Test creating a window from two-wire sketch (concentric), relying on default parts."""
        sketch = self._create_sketch_with_wires("SketchTwoWires", [(0, 0, 1000, 1200), (100, 100, 800, 1000)])
        window = Arch.makeWindow(baseobj=sketch, name="Window_SketchTwo_Default")
        self.document.recompute()

        self.assertEqual(window.Base, sketch)
        self.assertTrue(len(window.WindowParts) >= 5)
        self.assertEqual(window.WindowParts[0], "Default")
        self.assertEqual(window.WindowParts[1], "Frame", "Default type for multi-wire should be Frame.")
        self.assertIn("Wire0", window.WindowParts[2])
        self.assertIn("Wire1", window.WindowParts[2])
        self.assertFalse(window.Shape.isNull())
        self.assertTrue(len(window.Shape.Solids) > 0)

    def test_sketch_named_constraints_driven_by_window_props(self):
        """Test that window Width/Height properties drive sketch's named constraints."""
        sketch_width, sketch_height = 800.0, 1000.0
        sketch = self._create_sketch_with_named_constraints("SketchNamed", sketch_width, sketch_height)

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
        self.assertEqual(sketch.getDatum("Width").Value, new_win_width, "Window.Width should drive sketch 'Width' constraint.")

        new_win_height = 1500.0
        window.Height = new_win_height
        self.document.recompute()
        self.assertEqual(sketch.getDatum("Height").Value, new_win_height, "Window.Height should drive sketch 'Height' constraint.")

    def test_create_from_sketch_with_custom_parts(self):
        """Test creating a window from sketch with explicit custom parts."""
        sketch = self._create_sketch_with_wires("SketchCustom", [(0,0,1200,1000), (100,100,1000,800)])
        custom_parts = [
            "OuterFrame", "Frame", "Wire0,Wire1", "70", "0",
            "GlassPane", "Glass panel", "Wire1", "20", "25"
        ]
        window = Arch.makeWindow(baseobj=sketch, parts=custom_parts, name="Window_Custom")
        self.document.recompute()

        self.assertEqual(window.Base, sketch)
        self.assertEqual(list(window.WindowParts), custom_parts)
        self.assertFalse(window.Shape.isNull())
        self.assertEqual(len(window.Shape.Solids), 2, "Expected two solids for frame and glass.")

    def test_custom_parts_with_plus_v_references(self):
        """Test creating a window with custom parts using '+V' to reference window.Frame and window.Offset."""
        sketch = self._create_sketch_with_wires("SketchPlusV", [(0,0,1000,800)])
        frame_val = 60.0
        offset_val = 10.0

        custom_parts_plus_v = [
            "MainFrame", "Frame", "Wire0", "50+V", "5+V"
        ]
        window = Arch.makeWindow(baseobj=sketch, parts=custom_parts_plus_v, name="Window_PlusV")
        window.Frame = frame_val
        window.Offset = offset_val
        self.document.recompute()

        self.assertFalse(window.Shape.isNull())
        self.assertTrue(len(window.Shape.Solids) > 0)

# In class TestArchWindow(TestArchBase.TestArchBase):

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
        self.assertIsNone(window.Base, 
                          "Immediately after makeWindow(W,H), window.Base should be None.")
        self.assertTrue(window.Shape.isNull(), 
                        "Initially, window.Shape should be null as no Base or Parts yet.")
        self.assertAlmostEqual(window.Width.Value, win_w, places=5, 
                               msg="Window.Width property not correctly set by makeWindow.")
        self.assertAlmostEqual(window.Height.Value, win_h, places=5, 
                               msg="Window.Height property not correctly set by makeWindow.")

        # 2. Perform a document recompute
        # This should trigger window.execute() -> ArchComponent.ensureBase()
        self.document.recompute() 

        # 3. Assert the CURRENT BEHAVIOR: window.Base remains None
        # The original ArchComponent.ensureBase does not create a sketch if Base is None
        # and only Width/Height are provided to the object.
        self.assertIsNone(window.Base, 
                          "Current Behavior: window.Base should remain None even after recomputes for makeWindow(W,H).")
        
        # 4. Consequently, window.Shape should still be null.
        self.assertTrue(window.Shape.isNull(), 
                        "window.Shape should remain null if window.Base was not created.")

        # 5. Attempting to set parts that rely on a Base sketch (e.g., "Wire0")
        #    should not produce a shape if Base is None.
        window.WindowParts = ["Default", "Frame", "Wire0", "50", "0"]
        self.document.recompute()
        self.assertTrue(window.Shape.isNull(),
                        "window.Shape should still be null if WindowParts reference Wire0 from a non-existent Base.")

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
        
        wall = Arch.makeWall(line, 
                             width=wall_thickness, 
                             height=wall_height, 
                             align="Left", 
                             name="TestWall_ForOpening_Args")
        self.document.recompute()

        initial_wall_volume = wall.Shape.Volume
        initial_wall_vertical_area = wall.VerticalArea.Value 

        # 2. Create Sketch for Window profile
        sketch_profile_width = 800.0
        sketch_profile_height = 1000.0
        sk = self._create_sketch_with_wires("WindowSketch_Args", [(0, 0, sketch_profile_width, sketch_profile_height)])
        
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
        
        win.HoleDepth = 0 # Use "smart" hole depth calculation
        win.WindowParts = ["DefaultFrame", "Frame", "Wire0", "60", "0"]
        
        win.recompute() 
        self.document.recompute() 

        self.assertFalse(win.Shape.isNull(), "Window must have a valid shape.")
        self.assertEqual(win.Placement, FreeCAD.Placement(),
                 f"Window.Placement should be identity; got {win.Placement}")
        self.assertAlmostEqual(win.Width.Value, sketch_profile_width, 5)
        self.assertAlmostEqual(win.Height.Value, sketch_profile_height, 5)


        # 4. Add Window to Wall
        Arch.addComponents(win, host=wall)
        self.document.recompute() 

        # 5. Assertions
        # Wall.Subtractions PropertyLinkList check (informational, expected to be empty)
        wall_subtractions_prop = wall.Subtractions if hasattr(wall, "Subtractions") else []
        self.assertEqual(
            len(wall_subtractions_prop), 0,
            f"Wall.Subtractions property list expected to be empty, but: "
            f"{wall_subtractions_prop}"
        )

        # Geometric checks
        current_wall_volume = wall.Shape.Volume
        self.assertLess(
            current_wall_volume, initial_wall_volume,
            f"Wall volume should decrease. Before: {initial_wall_volume}, "
            f"After: {current_wall_volume}"
        )

        ideal_removed_volume = win.Width.Value * win.Height.Value * wall.Width.Value 
        self.assertAlmostEqual(
            current_wall_volume, initial_wall_volume - ideal_removed_volume, places=3,
            msg=f"Wall volume cut not as expected. Initial: {initial_wall_volume}, "
                f"Current: {current_wall_volume}, IdealRemoved: {ideal_removed_volume}"
        )
        
        # Check VerticalArea
        current_wall_vertical_area = wall.VerticalArea.Value
        
        window_opening_area_on_one_face = win.Width.Value * win.Height.Value
        area_of_side_reveals = 2 * win.Height.Value * wall.Width.Value 
        expected_wall_vertical_area_after = (initial_wall_vertical_area -
            (2 * window_opening_area_on_one_face) + area_of_side_reveals)

        self.assertAlmostEqual(
            current_wall_vertical_area, expected_wall_vertical_area_after, places=3,
            msg=f"Wall VerticalArea change not as expected. Initial: "
                f"{initial_wall_vertical_area}, Current: {current_wall_vertical_area}, "
                f"ExpectedAfter: {expected_wall_vertical_area_after}"
        )

    def test_clone_window(self):
        """Test cloning an Arch.Window object.
        
        Notes:
        - The clone's name is automatically generated, the `name` argument is ignored.
        - The clone's WindowParts, Sill and other properties are always empty, despite the original having them.
        
        """
        sketch = self._create_sketch_with_wires("OriginalSketch", [(0, 0, 600, 800)])
        original_parts = ["MainFrame", "Frame", "Wire0", "50", "10"]
        original_window = Arch.makeWindow(baseobj=sketch, parts=original_parts, name="OriginalWindow")
        original_window.Frame = 60.0
        original_window.Sill = 100.0
        self.document.recompute()

        self.assertEqual(original_window.Label, "OriginalWindow")


        cloned_window = Arch.makeWindow(baseobj=original_window)
        self.document.recompute()

        self.assertIsNotNone(cloned_window)
        self.assertIsNotNone(cloned_window.CloneOf)
        self.assertEqual(cloned_window.CloneOf, original_window)

        self.assertEqual(cloned_window.IfcType, original_window.IfcType)

        self.assertFalse(cloned_window.Shape.isNull())
        self.assertAlmostEqual(cloned_window.Shape.Volume, original_window.Shape.Volume, delta=1e-5,
                               msg="Cloned window volume should match original.")

    def test_create_window_on_xz_plane(self):
        """Test creating a window oriented on the XZ (vertical) plane."""
        sketch = self._create_sketch_with_wires("Sketch_XZ_Plane",
                                                [(0,0,1000,1200), (100,100,800,1000)])

        sketch.Placement.Rotation = FreeCAD.Rotation(FreeCAD.Vector(1,0,0), 90)
        self.document.recompute()

        window = Arch.makeWindow(baseobj=sketch, name="Window_XZ_Plane")
        window.WindowParts = [
            "Frame", "Frame", "Wire0,Wire1", "60", "0",
            "Glass", "Glass panel", "Wire1", "10", "25"
        ]
        self.document.recompute()

        self.assertFalse(window.Shape.isNull())
        self.assertTrue(len(window.Shape.Solids) > 0)

        expected_normal_y = 1.0

        self.assertAlmostEqual(window.Normal.x, 0.0, places=5)
        self.assertAlmostEqual(window.Normal.y, expected_normal_y, places=5,
                               msg=f"Window normal Y component incorrect. Expected approx {expected_normal_y}, got {window.Normal.y}")
        self.assertAlmostEqual(window.Normal.z, 0.0, places=5)

        bb = window.Shape.BoundBox
        self.assertGreater(bb.XLength, bb.YLength,
                           "Window XLength (width) should be greater than YLength (thickness).")
        self.assertGreater(bb.ZLength, bb.YLength,
                           "Window ZLength (height) should be greater than YLength (thickness).")

    def _create_sketch_with_wires(self, name: str, wire_definitions: list[tuple[float, float, float, float]]) -> "FreeCAD.DocumentObject":
        """
        Helper to create a sketch with one or more specified rectangular wires.

        Each rectangle is defined in the sketch's local XY plane. The sketch
        is created in the current document (`self.document`). After adding all
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
            sketch.addGeometry(Part.LineSegment(FreeCAD.Vector(x, y, 0), FreeCAD.Vector(x + w, y, 0)))
            sketch.addGeometry(Part.LineSegment(FreeCAD.Vector(x + w, y, 0), FreeCAD.Vector(x + w, y + h, 0)))
            sketch.addGeometry(Part.LineSegment(FreeCAD.Vector(x + w, y + h, 0), FreeCAD.Vector(x, y + h, 0)))
            sketch.addGeometry(Part.LineSegment(FreeCAD.Vector(x, y + h, 0), FreeCAD.Vector(x, y, 0)))
            base_idx = i * 4
            sketch.addConstraint(Sketcher.Constraint('Coincident', base_idx, 2, base_idx + 1, 1))
            sketch.addConstraint(Sketcher.Constraint('Coincident', base_idx + 1, 2, base_idx + 2, 1))
            sketch.addConstraint(Sketcher.Constraint('Coincident', base_idx + 2, 2, base_idx + 3, 1))
            sketch.addConstraint(Sketcher.Constraint('Coincident', base_idx + 3, 2, base_idx, 1))

        self.document.recompute()

        return sketch

    def _create_sketch_with_named_constraints(self, name: str, initial_width: float, initial_height: float) -> "FreeCAD.DocumentObject":
        """
        Helper to create a rectangular sketch with "Width" and "Height" named constraints.

        The sketch is created in the current document (`self.document`) on the
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

        sketch.addGeometry(Part.LineSegment(FreeCAD.Vector(0,0,0), FreeCAD.Vector(initial_width,0,0)), False)
        sketch.addGeometry(Part.LineSegment(FreeCAD.Vector(initial_width,0,0), FreeCAD.Vector(initial_width,initial_height,0)), False)
        sketch.addGeometry(Part.LineSegment(FreeCAD.Vector(initial_width,initial_height,0), FreeCAD.Vector(0,initial_height,0)), False)
        sketch.addGeometry(Part.LineSegment(FreeCAD.Vector(0,initial_height,0), FreeCAD.Vector(0,0,0)), False)
        sketch.addConstraint(Sketcher.Constraint('Coincident',0,2,1,1))
        sketch.addConstraint(Sketcher.Constraint('Coincident',1,2,2,1))
        sketch.addConstraint(Sketcher.Constraint('Coincident',2,2,3,1))
        sketch.addConstraint(Sketcher.Constraint('Coincident',3,2,0,1))
        sketch.addConstraint(Sketcher.Constraint('Horizontal',0)); sketch.addConstraint(Sketcher.Constraint('Vertical',1))
        sketch.addConstraint(Sketcher.Constraint('Horizontal',2)); sketch.addConstraint(Sketcher.Constraint('Vertical',3))

        sketch.addConstraint(Sketcher.Constraint('DistanceX',0,1,0,2,initial_width))
        sketch.renameConstraint(sketch.ConstraintCount - 1, "Width")
        sketch.addConstraint(Sketcher.Constraint('DistanceY',1,1,1,2,initial_height))
        sketch.renameConstraint(sketch.ConstraintCount - 1, "Height")

        self.document.recompute()

        return sketch
