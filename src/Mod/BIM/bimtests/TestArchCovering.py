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
from draftutils import params
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
        # Isolate tests by saving and restoring the global parameter
        self.original_joint_width = params.get_param_arch("CoveringJoint", ret_default=False)
        self.document.recompute()

    def tearDown(self):
        # Restore the global parameter to prevent test pollution
        params.set_param_arch("CoveringJoint", self.original_joint_width)
        super().tearDown()

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
        face = Arch.getFaceName(slab, view_vector=App.Vector(0, 0, -1))

        face_obj = slab.Shape.getElement(face)
        self.assertAlmostEqual(face_obj.normalAt(0, 0).z, 1.0)

        # 2. Wall Case
        wall = Arch.makeWall(length=2000, width=200, height=3000)
        self.document.recompute()

        # View looking "IN" at the front face (Y=1 view vector)
        # We expect the face with Normal Y=-1 (opposing view)
        face_name = Arch.getFaceName(wall, view_vector=App.Vector(0, 1, 0))
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
        face = Arch.getFaceName(cyl, view_vector=App.Vector(1, 0, 0))

        # Should be Top or Bottom (planar), NOT Side (curved), despite Side being larger
        face_obj = cyl.Shape.getElement(face)
        self.assertIsNotNone(face_obj.findPlane(), "Selected face must be planar")

    def test_tile_offset_exclusivity(self):
        """Test that TileOffset x and y are mutually exclusive in geometry generation."""
        self.printTestMessage("tile offset exclusivity...")
        base = (self.box, ["Face6"])
        covering = Arch.makeCovering(base)
        covering.FinishMode = "Solid Tiles"
        covering.TileThickness = 20.0
        covering.TileLength = 200.0
        covering.TileWidth = 200.0
        covering.JointWidth = 10.0

        # Set both X and Y offsets. Logic should prioritize X and ignore Y.
        covering.TileOffset = App.Vector(100, 50, 0)
        self.document.recompute()

        # Verify the object recomputed successfully without error
        self.assertFalse(covering.Shape.isNull())
        self.assertGreater(len(covering.Shape.Solids), 0)

    def test_quantities_hatch_mode_reset(self):
        """Verify that quantities are reset to 0 in Hatch Pattern mode."""
        self.printTestMessage("hatch mode quantity reset...")
        base = (self.box, ["Face6"])
        covering = Arch.makeCovering(base)
        covering.FinishMode = "Hatch Pattern"
        self.document.recompute()

        self.assertEqual(covering.NetArea.Value, 0)
        self.assertEqual(covering.TotalJointLength.Value, 0)
        self.assertEqual(covering.CountFullTiles, 0)

    def test_quantities_positive_waste(self):
        """Verify calculation where GrossArea > NetArea (positive waste)."""
        self.printTestMessage("quantities positive waste...")
        # 1000x1000 face
        base = (self.box, ["Face6"])
        covering = Arch.makeCovering(base)
        covering.FinishMode = "Solid Tiles"
        # 300x300 tiles, 10mm joints -> 310mm step
        # 1000 / 310 = 3.22 -> 4 tiles per row (16 total)
        covering.TileLength = 300.0
        covering.TileWidth = 300.0
        covering.JointWidth = 10.0
        covering.TileThickness = 10.0
        covering.TileAlignment = "BottomLeft"
        self.document.recompute()

        # NetArea: 1,000,000
        self.assertAlmostEqual(covering.NetArea.Value, 1000000.0, places=1)
        # GrossArea: 16 tiles * 90,000 = 1,440,000
        self.assertAlmostEqual(covering.GrossArea.Value, 1440000.0, places=1)
        # Waste: 440,000
        self.assertAlmostEqual(covering.WasteArea.Value, 440000.0, places=1)
        # JointLength: 3 horiz + 3 vert lines * 1000 = 6000
        self.assertAlmostEqual(covering.TotalJointLength.Value, 6000.0, places=1)

    def test_quantities_clamped_waste(self):
        """Verify that WasteArea is clamped to 0 when GrossArea < NetArea."""
        self.printTestMessage("quantities clamped waste...")
        # 1000x1000 face
        base = (self.box, ["Face6"])
        covering = Arch.makeCovering(base)
        covering.FinishMode = "Solid Tiles"
        # 400x400 tiles, 100mm joints -> 500mm step
        # 1000 / 500 = 2 -> 2 tiles per row (perfect fit)
        covering.TileLength = 400.0
        covering.TileWidth = 400.0
        covering.JointWidth = 100.0
        covering.TileThickness = 10.0
        covering.TileAlignment = "BottomLeft"
        self.document.recompute()

        # NetArea: 1,000,000
        self.assertAlmostEqual(covering.NetArea.Value, 1000000.0, places=1)
        # GrossArea: 4 tiles * 160,000 = 640,000
        self.assertAlmostEqual(covering.GrossArea.Value, 640000.0, places=1)
        # Waste: Clamped to 0
        self.assertEqual(covering.WasteArea.Value, 0.0)
        # JointLength: 2 horiz + 2 vert lines * 1000 = 4000
        self.assertAlmostEqual(covering.TotalJointLength.Value, 4000.0, places=1)

    def test_alignment_offset(self):
        """Test that AlignmentOffset shifts the internal grid and ignores Z."""
        self.printTestMessage("alignment offset...")
        base = (self.box, ["Face6"])
        covering = Arch.makeCovering(base)
        covering.FinishMode = "Solid Tiles"
        covering.TileLength = 200.0
        covering.TileWidth = 200.0
        # Use a significant joint width to ensure tiling logic is executed
        covering.JointWidth = 2.0
        covering.TileThickness = 20.0
        covering.TileAlignment = "Center"
        self.document.recompute()

        # Capture initial geometric fingerprint using vertex coordinates.
        # Bounding box center is invariant, but vertex positions change when joints shift.
        base_sum = sum(v.Point.x + v.Point.y for v in covering.Shape.Vertexes)

        # Apply offset: X=100, Y=50
        covering.AlignmentOffset = App.Vector(100, 50, 0)
        self.document.recompute()

        offset_sum = sum(v.Point.x + v.Point.y for v in covering.Shape.Vertexes)
        self.assertNotAlmostEqual(base_sum, offset_sum, places=3)

        # Apply large Z component (should be ignored)
        covering.AlignmentOffset = App.Vector(100, 50, 5000)
        self.document.recompute()

        z_ignored_sum = sum(v.Point.x + v.Point.y for v in covering.Shape.Vertexes)
        self.assertAlmostEqual(offset_sum, z_ignored_sum, places=3)

        # Ensure tiles did not lift off the face (Face6 is at Z=1000)
        self.assertAlmostEqual(covering.Shape.BoundBox.ZMin, 1000.0, places=3)

    def test_butt_joint_tile_classification(self):
        """Verify that 'almost full' tiles are counted as full for butt joints."""
        self.printTestMessage("butt joint tile classification...")
        base = (self.box, ["Face6"])  # 1000x1000 face
        covering = Arch.makeCovering(base)
        covering.FinishMode = "Solid Tiles"
        covering.TileLength = 200.0  # 1000 / 200 = 5 tiles
        covering.TileWidth = 1000.0
        covering.TileThickness = 10.0
        covering.TileAlignment = "BottomLeft"

        # Setting JointWidth to 0 triggers the clamping to MIN_DIMENSION
        covering.JointWidth = 0.0
        self.document.recompute()

        # Despite tiny cuts from the 0.1mm joints, the forgiving tolerance
        # should classify all tiles as full.
        self.assertEqual(covering.CountFullTiles, 5)
        self.assertEqual(covering.CountPartialTiles, 0)

    def test_butt_joint_analytical_mode(self):
        """Verify that 0mm joint uses analytical mode with correct math counts."""
        self.printTestMessage("butt joint analytical mode...")
        base = (self.box, ["Face6"])  # 1000x1000
        covering = Arch.makeCovering(base)
        covering.TileLength = 200.0
        covering.TileWidth = 200.0
        covering.JointWidth = 0.0
        self.document.recompute()

        # Should be monolithic (1 solid)
        self.assertEqual(len(covering.Shape.Solids), 1)
        # Math count: 1,000,000 / 40,000 = 25
        self.assertEqual(covering.CountFullTiles, 25)
        self.assertEqual(covering.CountPartialTiles, 0)

    def test_tile_size_junk_protection(self):
        """Verify that tiny tiles (<1mm) trigger hard failure warning."""
        self.printTestMessage("tile size junk protection...")
        covering = Arch.makeCovering(self.box)
        covering.TileLength = 0.5
        self.document.recompute()
        self.assertTrue(covering.Shape.isNull())

    def test_performance_guard_fallback(self):
        """Verify fallback to analytical mode when tile count > 10,000."""
        self.printTestMessage("performance guard fallback...")
        # Create a large surface 20m x 20m
        large_box = self.document.addObject("Part::Box", "LargeBox")
        large_box.Length = 20000.0
        large_box.Width = 20000.0
        large_box.Height = 100.0
        self.document.recompute()

        covering = Arch.makeCovering((large_box, ["Face6"]))
        # 100mm tiles on 20m face = 200x200 grid = 40,000 units (> 10k)
        covering.TileLength = 100.0
        covering.TileWidth = 100.0
        # Use a safe joint width so we don't trigger the butt-joint logic
        covering.JointWidth = 5.0
        self.document.recompute()

        # Should be monolithic due to guard (cutters suppressed)
        self.assertEqual(len(covering.Shape.Solids), 1)
        # Count should still be calculated
        self.assertGreater(covering.CountFullTiles, 10000)

    def test_visual_limit_suppression(self):
        """Verify that layout lines are suppressed for extremely high counts."""
        self.printTestMessage("visual limit suppression...")
        # 100mm tiles on 40m face = 400x400 grid = 160,000 units (> 100k)
        large_box = self.document.addObject("Part::Box", "XLargeBox")
        large_box.Length = 40000.0
        large_box.Width = 40000.0
        large_box.Height = 100.0
        self.document.recompute()

        covering = Arch.makeCovering((large_box, ["Face6"]))
        covering.TileLength = 100.0
        covering.TileWidth = 100.0
        self.document.recompute()

        # Compound should only contain the solid, no edges for centerlines.
        # A standard Box solid has 12 edges.
        self.assertEqual(len(covering.Shape.Edges), 12)

    def test_getFaceGeometry_with_sketch_hole(self):
        """Verify that the face resolver correctly handles a sketch with a hole (using makeFace)."""
        self.printTestMessage("getFaceGeometry with sketch hole...")
        sketch = self.document.addObject("Sketcher::SketchObject", "HoleSketch")
        # Create a 100x100 outer square
        sketch.addGeometry(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(100, 0, 0)))
        sketch.addGeometry(Part.LineSegment(App.Vector(100, 0, 0), App.Vector(100, 100, 0)))
        sketch.addGeometry(Part.LineSegment(App.Vector(100, 100, 0), App.Vector(0, 100, 0)))
        sketch.addGeometry(Part.LineSegment(App.Vector(0, 100, 0), App.Vector(0, 0, 0)))
        # Create a 50x50 inner square (the hole)
        sketch.addGeometry(Part.LineSegment(App.Vector(25, 25, 0), App.Vector(75, 25, 0)))
        sketch.addGeometry(Part.LineSegment(App.Vector(75, 25, 0), App.Vector(75, 75, 0)))
        sketch.addGeometry(Part.LineSegment(App.Vector(75, 75, 0), App.Vector(25, 75, 0)))
        sketch.addGeometry(Part.LineSegment(App.Vector(25, 75, 0), App.Vector(25, 25, 0)))
        self.document.recompute()

        # Resolve the geometry
        face = Arch.getFaceGeometry(sketch)
        self.assertIsNotNone(face)
        # Expected area: 100*100 - 50*50 = 7500
        self.assertAlmostEqual(face.Area, 7500.0, places=3)
        self.assertEqual(len(face.Wires), 2, "Face should have one outer wire and one hole wire.")

    def test_getFaceUV_singularity_hardening(self):
        """Verify that the UV basis extractor handles zero-area faces gracefully."""
        self.printTestMessage("getFaceUV singularity hardening...")
        # Create an extremely narrow face to stress normalization
        degenerate_face = Part.makePlane(100, 0.001)

        # This should return a valid set of axes without raising a C++
        # normalization exception.
        try:
            basis = Arch.getFaceUV(degenerate_face)
            self.assertEqual(len(basis), 4)
        except Exception as e:
            self.fail(f"getFaceUV crashed on degenerate geometry: {e}")

    def test_isolated_tessellator_math(self):
        """Verify the RectangularTessellator algorithm without a DocumentObject."""
        self.printTestMessage("isolated tessellator math...")
        import ArchTessellation

        # 1000x1000 substrate
        substrate = Part.makePlane(1000, 1000)
        u, v, n, c = Arch.getFaceUV(substrate)
        origin = Arch.getFaceGridOrigin(substrate, c, u, v, alignment="BottomLeft")

        # Scenario: 200x200 tiles, 50mm joint.
        # Step is 250mm. 1000/250 = 4 tiles precisely per side. Total = 16.
        tessellator = ArchTessellation.RectangularTessellator(
            length=200, width=200, thickness=10, joint=50
        )
        result = tessellator.compute(substrate, origin, u, v, n)

        self.assertEqual(result.status, ArchTessellation.TessellationStatus.OK)
        self.assertEqual(result.quantities.count_full, 16)
        self.assertEqual(result.quantities.count_partial, 0)
        self.assertAlmostEqual(result.unit_area, 40000.0)
        self.assertAlmostEqual(result.quantities.area_net, 1000000.0)

    def test_tessellator_threshold_guards(self):
        """Verify that the tessellator correctly returns status enums for boundary cases."""
        self.printTestMessage("tessellator threshold guards...")
        import ArchTessellation

        substrate = Part.makePlane(100, 100)
        u, v, n, c = Arch.getFaceUV(substrate)
        origin = App.Vector(0, 0, 0)

        # Case 1: Invalid Dimensions (< 1.0mm)
        t1 = ArchTessellation.RectangularTessellator(0.5, 100, 0, 0)
        res1 = t1.compute(substrate, origin, u, v, n)
        self.assertEqual(res1.status, ArchTessellation.TessellationStatus.INVALID_DIMENSIONS)

        # Case 2: Too many tiles (> 10,000)
        # 1000mm face / 5mm step = 200 divisions. 200^2 = 40,000 tiles.
        large_substrate = Part.makePlane(1000, 1000)
        t2 = ArchTessellation.RectangularTessellator(4, 4, 0, 1)
        res2 = t2.compute(large_substrate, origin, u, v, n)
        self.assertEqual(res2.status, ArchTessellation.TessellationStatus.COUNT_TOO_HIGH)
        # In high-count mode, the geometry should be a monolithic compound (solid + grid)
        # but it should not have been physically discretized into 40,000 solids.
        self.assertIsNotNone(res2.geometry)
        self.assertGreater(res2.quantities.count_full, 10000)
