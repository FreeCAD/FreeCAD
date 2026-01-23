# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Copyright (c) 2026 Furgo
#
# This file is part of the FreeCAD Arch workbench.
# You can find the full license text in the LICENSE file in the root directory.

import FreeCAD as App
import Arch
import Part
import Draft
from bimtests import TestArchBase


class TestArchCovering(TestArchBase.TestArchBase):
    """Unit tests for the Arch Covering logic object and App-side geometry generation."""

    def setUp(self):
        super().setUp()
        # Create a standard reference box for tiling tests
        self.box = self.document.addObject("Part::Box", "BaseBox")
        self.box.Length = 1000.0
        self.box.Width = 1000.0
        self.box.Height = 1000.0
        self.document.recompute()

    def test_makeCovering_creation(self):
        """Test basic object creation and property defaults."""
        covering = Arch.makeCovering(name="TestCovering")
        self.assertIsNotNone(covering)
        self.assertEqual(covering.Label, "TestCovering")
        self.assertEqual(covering.IfcType, "Covering")

        # Verify default parameters are loaded from preferences (defined in params.py)
        # Based on the implementation, these should be non-zero defaults
        self.assertGreater(covering.TileLength.Value, 0)
        self.assertGreater(covering.TileWidth.Value, 0)
        self.assertIn(covering.FinishMode, ["Solid Tiles", "Parametric Pattern", "Hatch Pattern"])

    def test_covering_geometry_solid_tiles(self):
        """Test geometry generation in Solid Tiles mode."""
        # Link to the top face of the box (Face6)
        base = (self.box, ["Face6"])
        covering = Arch.makeCovering(base)
        covering.FinishMode = "Solid Tiles"
        covering.TileLength = 200.0
        covering.TileWidth = 200.0
        covering.JointWidth = 10.0
        covering.TileThickness = 20.0
        self.document.recompute()

        self.assertFalse(covering.Shape.isNull(), "Covering failed to generate shape.")
        self.assertGreater(
            len(covering.Shape.Solids), 1, "Covering should consist of multiple tile solids."
        )

        # Verify thickness in the bounding box
        # Since it is on the top face of a 1000mm box, ZMin should be 1000 and ZMax 1020
        bb = covering.Shape.BoundBox
        self.assertAlmostEqual(bb.ZLength, 20.0, places=3)
        self.assertAlmostEqual(bb.ZMin, 1000.0, places=3)

    def test_covering_geometry_parametric_pattern(self):
        """Test geometry generation in Parametric Pattern mode."""
        base = (self.box, ["Face6"])
        covering = Arch.makeCovering(base)
        covering.FinishMode = "Parametric Pattern"
        covering.TileLength = 500.0
        covering.TileWidth = 500.0
        covering.JointWidth = 20.0
        self.document.recompute()

        self.assertFalse(covering.Shape.isNull())
        # In pattern mode, the execute method returns a Compound of Wires
        self.assertEqual(covering.Shape.ShapeType, "Compound")
        self.assertGreater(len(covering.Shape.Edges), 0)
        self.assertEqual(len(covering.Shape.Solids), 0, "Pattern mode should not produce solids.")

    def test_tile_counting_logic(self):
        """Test that the object correctly calculates full vs partial tile counts."""
        # Box is 1000x1000. Tiles are 300x300.
        # Use a 1mm joint to ensure the solids are physically discretized.
        base = (self.box, ["Face6"])
        covering = Arch.makeCovering(base)
        covering.TileLength = 300.0
        covering.TileWidth = 300.0
        covering.JointWidth = 1.0
        covering.TileAlignment = "BottomLeft"
        covering.FinishMode = "Solid Tiles"
        covering.TileThickness = 10.0
        self.document.recompute()

        # The step is 301 mm. 301 * 3 = 903 mm.
        # 3x3 full tiles (9 total) will fit within the 1000mm bounds.
        # The remaining space (approx 97 mm) will be filled by partial tiles.
        self.assertEqual(covering.CountFullTiles, 9)
        self.assertGreater(covering.CountPartialTiles, 0)

    def test_ifc_predefined_type(self):
        """Verify the IFC Predefined Type property is available and correct."""
        covering = Arch.makeCovering()
        self.assertTrue(hasattr(covering, "IfcPredefinedType"))

        # Test setting a valid enumeration value
        covering.IfcPredefinedType = "CLADDING"
        self.assertEqual(covering.IfcPredefinedType, "CLADDING")

    def test_rotation_and_alignment_persistence(self):
        """Verify that new objects pick up current user preferences."""
        from draftutils import params

        # Store original alignment and set a test-specific preference
        original_align = params.get_param_arch("CoveringAlignment")
        test_value = "TopRight"
        params.set_param_arch("CoveringAlignment", test_value)

        try:
            # Create a new covering
            covering = Arch.makeCovering()

            # The object should have the value we just set in preferences
            self.assertEqual(
                covering.TileAlignment,
                test_value,
                "Object did not inherit the updated user preference.",
            )
        finally:
            # Cleanup: restore the user's original setting
            params.set_param_arch("CoveringAlignment", original_align)

    def test_hatch_pattern_mode(self):
        """Test the Hatch Pattern mode initialization."""
        # Note: Functional testing of actual hatching depends on external PAT files
        # Only the object state and mode switching logic are tested here
        base = (self.box, ["Face6"])
        covering = Arch.makeCovering(base)
        covering.FinishMode = "Hatch Pattern"
        covering.PatternName = "ANSI31"
        covering.PatternScale = 2.0

        self.assertEqual(covering.FinishMode, "Hatch Pattern")
        # Ensure it doesn't crash on recompute even if the file is missing
        # It should just print a warning
        self.document.recompute()

    def test_division_by_zero_guard(self):
        """Test that zero tile size does not crash the recompute."""
        self.printTestMessage("division by zero guard...")
        base = (self.box, ["Face6"])
        covering = Arch.makeCovering(base)

        # Force increments to zero
        covering.TileLength = 0.0
        covering.TileWidth = 0.0
        covering.JointWidth = 0.0

        # This triggers execute() -> _build_cutters()
        # Without the guard, this raises ZeroDivisionError
        self.document.recompute()

        # If it didn't crash, the shape should simply be empty (null)
        self.assertTrue(covering.Shape.isNull())

    def test_planar_enforcement(self):
        """Verify that coverings are only created on planar surfaces."""
        self.printTestMessage("planar enforcement check...")

        # Create a sphere (non-planar surface)
        sphere = self.document.addObject("Part::Sphere", "Sphere")
        self.document.recompute()

        covering = Arch.makeCovering((sphere, ["Face1"]))
        self.document.recompute()

        # The execute logic should detect the non-planar face and return a null shape
        self.assertTrue(
            covering.Shape.isNull(), "Covering was incorrectly generated on a non-planar face."
        )

    def test_hatch_zero_scale_guard(self):
        """Verify that a zero hatch scale is handled safely."""
        self.printTestMessage("hatch zero scale guard...")
        base = (self.box, ["Face6"])
        covering = Arch.makeCovering(base)
        covering.FinishMode = "Hatch Pattern"

        # Set a zero scale which usually triggers math errors
        covering.PatternScale = 0.0

        # This should not raise an exception
        self.document.recompute()

        # The result should be a safe fallback (e.g. only the base face)
        self.assertFalse(covering.Shape.isNull())

    def test_open_wire_guard(self):
        """Verify that open wires are rejected as base objects."""
        self.printTestMessage("open wire guard...")
        # Create an open Draft Line
        line = Draft.makeLine(App.Vector(0, 0, 0), App.Vector(500, 0, 0))
        self.document.recompute()

        covering = Arch.makeCovering(line)
        self.document.recompute()

        # Should be null because the wire isn't closed
        self.assertTrue(covering.Shape.isNull())

    def test_z_fighting_offset(self):
        """Verify that 2D patterns are slightly offset to prevent Z-fighting."""
        self.printTestMessage("z-fighting offset...")
        base = (self.box, ["Face6"])  # Face6 is at Z=1000
        covering = Arch.makeCovering(base)
        covering.FinishMode = "Parametric Pattern"
        self.document.recompute()

        # Check the Z position of the resulting wires
        # It should be 1000 + our microscopic offset
        z_pos = covering.Shape.BoundBox.ZMin
        self.assertGreater(z_pos, 1000.0)
        self.assertLess(z_pos, 1000.1)  # Ensure it's still "micro"

    def test_parametric_pattern_correctness(self):
        """Verify geometry and count for Parametric Pattern mode."""
        self.printTestMessage("parametric pattern correctness...")
        base = (self.box, ["Face6"])
        covering = Arch.makeCovering(base)
        covering.FinishMode = "Parametric Pattern"

        # Use dimensions that create a predictable 2x2 grid
        covering.TileLength = 400.0
        covering.TileWidth = 400.0
        covering.JointWidth = 100.0
        covering.TileAlignment = "BottomLeft"

        self.document.recompute()

        # Assert shape validity
        self.assertFalse(covering.Shape.isNull())
        self.assertEqual(covering.Shape.ShapeType, "Compound")

        #  Assert correct number of wires
        # A 2x2 grid should produce exactly 4 closed wires.
        self.assertEqual(len(covering.Shape.Wires), 4, "Should produce exactly 4 tile wires.")

        #  Assert geometric content
        # Verify that the final shape is not the same as the base face.
        # This confirms that the cutting and offsetting operations were successful.
        base_face = self.box.getSubObject("Face6")
        self.assertNotEqual(
            covering.Shape.hashCode(),
            base_face.hashCode(),
            "Final shape should be different from the base face.",
        )

    def test_smart_face_detection(self):
        """Test the smart face detection heuristics."""
        self.printTestMessage("smart face detection...")

        # 1. Slab Case
        slab = Arch.makeStructure(length=1000, width=1000, height=200)
        self.document.recompute()

        # Create a temp covering to access the proxy logic
        covering = Arch.makeCovering()

        # View looking DOWN (0,0,-1) at the top face
        # We expect the Top Face (Normal +Z) because it opposes the view vector
        face = covering.Proxy.get_best_face(slab, view_direction=App.Vector(0, 0, -1))

        face_obj = slab.Shape.getElement(face)
        self.assertAlmostEqual(face_obj.normalAt(0, 0).z, 1.0)

        # 2. Wall Case
        wall = Arch.makeWall(length=2000, width=200, height=3000)
        self.document.recompute()

        # View looking "IN" at the front face (Y=1 view vector)
        # We expect the face with Normal Y=-1 (opposing view)
        face_name = covering.Proxy.get_best_face(wall, view_direction=App.Vector(0, 1, 0))
        face_obj_wall = wall.Shape.getElement(face_name)

        self.assertLess(face_obj_wall.normalAt(0, 0).y, -0.9)

    def test_smart_face_detection_planar_filter(self):
        """Test that get_best_face ignores non-planar faces."""
        # Create a Cylinder
        cyl = self.document.addObject("Part::Cylinder", "Cylinder")
        cyl.Radius = 2
        cyl.Height = 10
        self.document.recompute()

        covering = Arch.makeCovering()

        # View from side (looking at the curved face)
        face = covering.Proxy.get_best_face(cyl, view_direction=App.Vector(1, 0, 0))

        # Should be Top or Bottom (planar), NOT Side (curved), despite Side being larger
        face_obj = cyl.Shape.getElement(face)
        self.assertIsNotNone(face_obj.findPlane(), "Selected face must be planar")
