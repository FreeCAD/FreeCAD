# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Copyright (c) 2026 Furgo
#
# This file is part of the FreeCAD BIM workbench.
# You can find the full license text in the LICENSE file in the root directory.

import FreeCAD as App
import Arch
import ArchCovering
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
        # Isolate tests by saving and restoring global parameters.
        # CoveringRotation is saved/restored because interactive use can leave a
        # non-zero preference that breaks tests which do not explicitly set
        # covering.Rotation = 0.
        self.original_joint_width = params.get_param_arch("CoveringJoint", ret_default=False)
        self.original_rotation = params.get_param_arch("CoveringRotation", ret_default=False)
        self.original_alignment = params.get_param_arch("CoveringAlignment", ret_default=False)
        params.set_param_arch("CoveringRotation", 0)
        self.document.recompute()

    def tearDown(self):
        # Restore the global parameters to prevent test pollution
        params.set_param_arch("CoveringJoint", self.original_joint_width)
        params.set_param_arch("CoveringRotation", self.original_rotation)
        params.set_param_arch("CoveringAlignment", self.original_alignment)
        super().tearDown()

    def test_makeCovering_creation(self):
        """Test basic object creation and property defaults."""
        covering = Arch.makeCovering(name="TestCovering")
        self.assertIsNotNone(covering)
        self.assertEqual(covering.Label, "TestCovering")
        self.assertEqual(covering.IfcType, "Covering")

        # Verify default parameters are loaded from preferences
        self.assertGreater(covering.TileLength.Value, 0)
        self.assertGreater(covering.TileWidth.Value, 0)
        self.assertIn(
            covering.FinishMode,
            ["Solid Tiles", "Parametric Pattern", "Monolithic", "Hatch Pattern"],
        )

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
        # Since it is on the top face of a 1000 mm box, ZMin should be 1000 and ZMax 1020
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
        # In pattern mode the shape is a Compound: one solid substrate plus pattern wires.
        self.assertEqual(covering.Shape.ShapeType, "Compound")
        self.assertGreater(len(covering.Shape.Edges), 0)
        self.assertEqual(
            len(covering.Shape.Solids), 1, "Pattern mode should produce one solid substrate."
        )

    def test_tile_counting_logic(self):
        """Test that the object correctly calculates full vs partial tile counts."""
        # Box is 1000x1000. Tiles are 300x300.
        # Use a 1 mm joint so each tile is produced as a separate solid.
        base = (self.box, ["Face6"])
        covering = Arch.makeCovering(base)
        covering.TileLength = 300.0
        covering.TileWidth = 300.0
        covering.JointWidth = 1.0
        covering.TileAlignment = "Bottom Left"
        covering.FinishMode = "Solid Tiles"
        covering.TileThickness = 10.0
        self.document.recompute()

        # Verify the shape spans the full 1000 mm face in X (world-space shape, identity placement)
        self.assertAlmostEqual(covering.Shape.BoundBox.XLength, 1000.0, places=1)

        # The step is 301 mm. 301 * 3 = 903 mm.
        # 3x3 full tiles (9 total) will fit within the 1000 mm bounds.
        # The remaining space (approx 97 mm) will be filled by partial tiles.
        self.assertEqual(covering.CountFullTiles, 9)
        self.assertGreater(covering.CountPartialTiles, 0)

    def test_ifc_predefined_type(self):
        """Verify the IFC Predefined Type property is available and correct."""
        covering = Arch.makeCovering()
        self.assertTrue(hasattr(covering, "PredefinedType"))

        # Test setting a valid enumeration value
        covering.PredefinedType = "CLADDING"
        self.assertEqual(covering.PredefinedType, "CLADDING")

    def test_rotation_and_alignment_persistence(self):
        """Verify that new objects pick up current user preferences."""
        params.set_param_arch("CoveringAlignment", "Top Right")
        covering = Arch.makeCovering()
        self.assertEqual(
            covering.TileAlignment,
            "Top Right",
            "Object did not inherit the updated user preference.",
        )

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

        # BoundBox on a Part::FeaturePython already includes the object's Placement, so
        # BoundBox.ZMin is directly comparable to the world Z of the base face.
        z_pos = covering.Shape.BoundBox.ZMin
        self.assertGreaterEqual(z_pos, 1000.0)
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
        covering.TileAlignment = "Bottom Left"

        self.document.recompute()

        # Assert shape validity
        self.assertFalse(covering.Shape.isNull())
        self.assertEqual(covering.Shape.ShapeType, "Compound")

        # Account for the 12 edges of the solid substrate plus the pattern wires
        # For a 2x2 grid on a box, total wires = 6 (box faces) + 4 (tile pattern)
        self.assertGreaterEqual(len(covering.Shape.Wires), 4, "Should produce at least 4 wires.")

        # Assert geometric content
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

        # View looking DOWN (0,0,-1) at the top face
        # We expect the Top Face (Normal +Z) because it opposes the view vector
        face = Arch.pickMainFaceName(slab, view_vector=App.Vector(0, 0, -1))

        face_obj = slab.Shape.getElement(face)
        self.assertAlmostEqual(face_obj.normalAt(0, 0).z, 1.0)

        # 2. Wall Case
        wall = Arch.makeWall(length=2000, width=200, height=3000)
        self.document.recompute()

        # View looking "IN" at the front face (Y=1 view vector)
        # We expect the face with Normal Y=-1 (opposing view)
        face_name = Arch.pickMainFaceName(wall, view_vector=App.Vector(0, 1, 0))
        face_obj_wall = wall.Shape.getElement(face_name)

        self.assertLess(face_obj_wall.normalAt(0, 0).y, -0.9)

    def test_smart_face_detection_planar_filter(self):
        """Test that get_best_face ignores non-planar faces."""
        # Create a Cylinder
        cyl = self.document.addObject("Part::Cylinder", "Cylinder")
        cyl.Radius = 2
        cyl.Height = 10
        self.document.recompute()

        # View from side (looking at the curved face)
        face = Arch.pickMainFaceName(cyl, view_vector=App.Vector(1, 0, 0))

        # Should be Top or Bottom (planar), not Side (curved), despite Side being larger
        face_obj = cyl.Shape.getElement(face)
        self.assertIsNotNone(face_obj.findPlane(), "Selected face must be planar")

    def test_tile_offset_exclusivity(self):
        """Smoke test: AlignmentOffset with both X and Y components set does not crash."""
        self.printTestMessage("tile offset exclusivity...")
        base = (self.box, ["Face6"])
        covering = Arch.makeCovering(base)
        covering.FinishMode = "Solid Tiles"
        covering.TileThickness = 20.0
        covering.TileLength = 200.0
        covering.TileWidth = 200.0
        covering.JointWidth = 10.0

        covering.AlignmentOffset = App.Vector(100, 50, 0)
        self.document.recompute()

        self.assertFalse(covering.Shape.isNull())
        self.assertGreater(len(covering.Shape.Solids), 0)

    def test_quantities_hatch_mode(self):
        """Verify quantities in Hatch Pattern mode: the covering is a single unit
        matching the substrate, with no tile joints."""
        self.printTestMessage("hatch mode quantities...")
        base = (self.box, ["Face6"])
        covering = Arch.makeCovering(base)
        covering.FinishMode = "Hatch Pattern"
        self.document.recompute()

        self.assertAlmostEqual(covering.NetArea.Value, 1000000.0, places=1)
        self.assertAlmostEqual(covering.GrossArea.Value, 1000000.0, places=1)
        self.assertEqual(covering.TotalJointLength.Value, 0)
        self.assertEqual(covering.CountFullTiles, 1)
        self.assertEqual(covering.CountPartialTiles, 0)

    def test_quantities_positive_waste(self):
        """Verify calculation where GrossArea > NetArea (positive waste)."""
        self.printTestMessage("quantities positive waste...")
        # 1000x1000 face
        base = (self.box, ["Face6"])
        covering = Arch.makeCovering(base)
        covering.FinishMode = "Solid Tiles"
        # 300x300 tiles, 10 mm joints -> 310 mm step
        # 1000 / 310 = 3.22 -> 4 tiles per row (16 total)
        covering.TileLength = 300.0
        covering.TileWidth = 300.0
        covering.JointWidth = 10.0
        covering.TileThickness = 10.0
        covering.TileAlignment = "Bottom Left"
        self.document.recompute()

        # NetArea: 1,000,000
        self.assertAlmostEqual(covering.NetArea.Value, 1000000.0, places=1)
        # GrossArea: 16 tiles * 90,000 = 1,440,000
        self.assertAlmostEqual(covering.GrossArea.Value, 1440000.0, places=1)
        # Waste: 440,000
        self.assertAlmostEqual(covering.WasteArea.Value, 440000.0, places=1)
        # JointLength: 3 horiz + 3 vert lines * 1000 = 6000
        self.assertAlmostEqual(covering.TotalJointLength.Value, 6000.0, places=1)
        # OuterArea equals NetArea when the base face has no holes; HoleAreas is empty.
        self.assertAlmostEqual(covering.OuterArea.Value, 1000000.0, places=1)
        self.assertEqual(list(covering.HoleAreas), [])

    def test_quantities_outer_area_and_hole_areas(self):
        """Verify OuterArea and HoleAreas on a base face that has holes."""
        self.printTestMessage("outer area and hole areas...")
        # Build a 1000x1000 face with two rectangular holes: 200x200 and 100x100.
        outer = Part.makePlane(1000, 1000)
        hole_large = Part.makePlane(200, 200, App.Vector(100, 100, 0))
        hole_small = Part.makePlane(100, 100, App.Vector(500, 500, 0))
        cut_shape = outer.cut(Part.Compound([hole_large, hole_small]))
        holed_face = cut_shape.Faces[0]
        feature = self.document.addObject("Part::Feature", "HoledFace")
        feature.Shape = holed_face
        self.document.recompute()

        base = (feature, ["Face1"])
        covering = Arch.makeCovering(base)
        # Monolithic mode keeps the substrate as a single solid so the quantities reflect the
        # face geometry directly, independent of tile-layout rounding.
        covering.FinishMode = "Monolithic"
        covering.TileThickness = 10.0
        self.document.recompute()

        # OuterArea is the full outer rectangle, ignoring holes.
        self.assertAlmostEqual(covering.OuterArea.Value, 1000000.0, places=1)
        # HoleAreas is sorted descending: 200x200 first, then 100x100.
        self.assertEqual(len(covering.HoleAreas), 2)
        self.assertAlmostEqual(covering.HoleAreas[0], 40000.0, places=1)
        self.assertAlmostEqual(covering.HoleAreas[1], 10000.0, places=1)
        # Invariant: OuterArea = NetArea + sum(HoleAreas).
        self.assertAlmostEqual(
            covering.OuterArea.Value,
            covering.NetArea.Value + sum(covering.HoleAreas),
            places=1,
        )

    def test_setback_with_interior_holes(self):
        """apply_setback must return a face, not a shell, when the base face has interior holes.

        When BorderSetback > 0 and the base face contains inner wires (holes) that do not intersect
        the shrunk outer boundary, the boolean cut inside apply_setback previously returned a Shell.
        That Shell caused a crash downstream when _compute_face_areas tried to read OuterWire on it.
        """
        self.printTestMessage("setback with interior holes...")

        # 1000x1000 face with a 200x200 hole that lies entirely inside the setback margin.
        outer = Part.makePolygon(
            [
                App.Vector(0, 0, 0),
                App.Vector(1000, 0, 0),
                App.Vector(1000, 1000, 0),
                App.Vector(0, 1000, 0),
                App.Vector(0, 0, 0),
            ],
            True,
        )
        inner = Part.makePolygon(
            [
                App.Vector(400, 400, 0),
                App.Vector(600, 400, 0),
                App.Vector(600, 600, 0),
                App.Vector(400, 600, 0),
                App.Vector(400, 400, 0),
            ],
            True,
        )
        holed_face = Part.Face([outer, inner])
        feature = self.document.addObject("Part::Feature", "HoledFaceSetback")
        feature.Shape = holed_face
        self.document.recompute()

        covering = Arch.makeCovering((feature, ["Face1"]))
        covering.FinishMode = "Monolithic"
        covering.TileThickness = 10.0
        covering.BorderSetback = 50.0
        self.document.recompute()

        # A shell would cause Shape to be null or raise during recompute; verify it did not.
        self.assertFalse(covering.Shape.isNull())

        # The setback reduces each edge by 50 mm: tiling area is 900x900 = 810 000 mm².
        # The hole (200x200 = 40 000 mm²) lies inside and is preserved.
        # NetArea = 810 000 - 40 000 = 770 000 mm².
        self.assertAlmostEqual(covering.OuterArea.Value, 810000.0, delta=500.0)
        self.assertEqual(len(covering.HoleAreas), 1)
        self.assertAlmostEqual(covering.HoleAreas[0], 40000.0, delta=500.0)
        self.assertAlmostEqual(
            covering.OuterArea.Value,
            covering.NetArea.Value + sum(covering.HoleAreas),
            places=1,
        )

    def test_setback_hole_outside_shrunk_boundary(self):
        """apply_setback must handle holes that land outside the shrunk outer boundary.

        When a large setback shrinks the tiling face so that a hole wire now falls completely
        outside the tiled region, the cut is a no-op and the shrunk face is returned as-is.
        """
        self.printTestMessage("setback hole outside shrunk boundary...")

        outer = Part.makePolygon(
            [
                App.Vector(0, 0, 0),
                App.Vector(1000, 0, 0),
                App.Vector(1000, 1000, 0),
                App.Vector(0, 1000, 0),
                App.Vector(0, 0, 0),
            ],
            True,
        )
        # Hole near the corner — a large setback will push the tiling area away from it.
        inner = Part.makePolygon(
            [
                App.Vector(10, 10, 0),
                App.Vector(60, 10, 0),
                App.Vector(60, 60, 0),
                App.Vector(10, 60, 0),
                App.Vector(10, 10, 0),
            ],
            True,
        )
        holed_face = Part.Face([outer, inner])
        feature = self.document.addObject("Part::Feature", "HoledFaceCornerHole")
        feature.Shape = holed_face
        self.document.recompute()

        covering = Arch.makeCovering((feature, ["Face1"]))
        covering.FinishMode = "Monolithic"
        covering.TileThickness = 10.0
        # Setback of 100 mm moves tiling area to [100,900]x[100,900]; hole is at [10,60]x[10,60],
        # entirely outside the shrunk region.
        covering.BorderSetback = 100.0
        self.document.recompute()

        # Should not crash; tiling area is 800x800 = 640 000 mm² with no preserved hole.
        self.assertFalse(covering.Shape.isNull())
        self.assertAlmostEqual(covering.OuterArea.Value, 640000.0, delta=500.0)

    def test_setback_returns_face_not_shell(self):
        """apply_setback must return a Face (not a Shell) so OuterWire is accessible.

        Directly exercises the fixed code path: a face with an interior hole processed through
        apply_setback with a positive setback value.
        """
        self.printTestMessage("setback returns face not shell...")

        outer = Part.makePlane(1000, 1000)
        hole = Part.makePlane(200, 200, App.Vector(400, 400, 0))
        face_with_hole = outer.cut(hole).Faces[0]

        result = ArchCovering.apply_setback(face_with_hole, 50.0)

        self.assertEqual(result.ShapeType, "Face", "apply_setback must return a Face, not a Shell.")
        self.assertTrue(hasattr(result, "OuterWire"))
        # Shrunk outer: 900x900 = 810 000. Hole 200x200 = 40 000. Net = 770 000.
        self.assertAlmostEqual(result.Area, 770000.0, delta=1.0)

    def test_setback_collapse_fallback(self):
        """apply_setback returns the original face when the setback is larger than the face."""
        self.printTestMessage("setback collapse fallback...")

        face = Part.makePlane(100, 100)
        result = ArchCovering.apply_setback(face, 60.0)

        self.assertFalse(result.isNull())
        self.assertAlmostEqual(
            result.Area, 10000.0, delta=0.1, msg="Should fall back to the original face."
        )

    def test_setback_hole_partially_outside_shrunk_boundary(self):
        """apply_setback cuts only the portion of a hole that overlaps the shrunk tiling area."""
        self.printTestMessage("setback hole partially outside shrunk boundary...")

        outer = Part.makePlane(1000, 1000)
        # Hole at (10,10)-(110,110); straddles the 100mm setback boundary.
        hole = Part.makePlane(100, 100, App.Vector(10, 10, 0))
        face_with_hole = outer.cut(hole).Faces[0]

        # 100mm setback: shrunk outer is (100,100)-(900,900), area = 640 000.
        # Overlap of hole with shrunk region: (100,100)-(110,110) = 10x10 = 100 mm².
        result = ArchCovering.apply_setback(face_with_hole, 100.0)

        self.assertIsInstance(result, Part.Face)
        self.assertAlmostEqual(result.Area, 639900.0, delta=1.0)

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
        covering.TileAlignment = "Bottom Left"
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

        # Sum vertex coordinates as a simple layout signature: the bounding box center stays put
        # when joints shift, but individual vertex positions do not.
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
        covering.TileAlignment = "Bottom Left"

        # Setting JointWidth to 0 triggers the clamping to MIN_DIMENSION
        covering.JointWidth = 0.0
        self.document.recompute()

        # The 0.1 mm joints produce tiles marginally smaller than the nominal size. They must still
        # be classified as full, not partial, thanks to the tolerance in the full-tile threshold.
        self.assertEqual(covering.CountFullTiles, 5)
        self.assertEqual(covering.CountPartialTiles, 0)

    def test_butt_joint_analytical_mode(self):
        """Verify that 0mm joint uses analytical mode with correct math counts."""
        self.printTestMessage("butt joint analytical mode...")
        base = (self.box, ["Face6"])  # 1000x1000
        covering = Arch.makeCovering(base)
        covering.FinishMode = "Solid Tiles"
        covering.TileLength = 200.0
        covering.TileWidth = 200.0
        covering.JointWidth = 0.0
        covering.TileThickness = 10.0
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
        covering.FinishMode = "Solid Tiles"
        covering.TileLength = 100.0
        covering.TileWidth = 100.0
        # Use a safe joint width so we don't trigger the butt-joint logic
        covering.JointWidth = 5.0
        covering.TileThickness = 10.0
        self.document.recompute()

        # Should be monolithic due to guard (cutters suppressed)
        self.assertEqual(len(covering.Shape.Solids), 1)
        # Count should still be calculated
        self.assertGreater(covering.CountFullTiles, 10000)

    def test_visual_limit_suppression(self):
        """Verify that layout lines are suppressed for extremely high counts."""
        self.printTestMessage("visual limit suppression...")
        # 100mm tiles on 40 m face = 400x400 grid = 160,000 units (> 100k)
        large_box = self.document.addObject("Part::Box", "XLargeBox")
        large_box.Length = 40000.0
        large_box.Width = 40000.0
        large_box.Height = 100.0
        self.document.recompute()

        covering = Arch.makeCovering((large_box, ["Face6"]))
        covering.FinishMode = "Solid Tiles"
        covering.TileLength = 100.0
        covering.TileWidth = 100.0
        covering.TileThickness = 10.0
        self.document.recompute()

        # Compound should only contain the solid, no edges for centerlines.
        # A standard Box solid has 12 edges.
        self.assertEqual(len(covering.Shape.Edges), 12)

    def test_resolveFace_with_sketch_hole(self):
        """Verify that the face resolver correctly handles a sketch with a hole (using makeFace)."""
        self.printTestMessage("resolveFace with sketch hole...")
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
        face = Arch.resolveFace(sketch)
        self.assertIsNotNone(face)
        # Expected area: 100*100 - 50*50 = 7500
        self.assertAlmostEqual(face.Area, 7500.0, places=3)
        self.assertEqual(len(face.Wires), 2, "Face should have one outer wire and one hole wire.")

    def test_getFaceFrame_singularity_hardening(self):
        """Verify that the UV basis extractor handles zero-area faces gracefully."""
        self.printTestMessage("getFaceFrame singularity hardening...")
        # Create an extremely narrow face to stress normalization
        degenerate_face = Part.makePlane(100, 0.001)

        # A zero-area face can produce a zero-length cross product whose .normalize() raises
        # FreeCADError; the hardened path should return a valid frame instead.
        try:
            frame = Arch.getFaceFrame(degenerate_face)
            self.assertEqual(len(frame), 4)
        except Exception as e:
            self.fail(f"getFaceFrame crashed on degenerate geometry: {e}")

    def test_isolated_tessellator_math(self):
        """Verify the RectangularTessellator algorithm without a DocumentObject."""
        self.printTestMessage("isolated tessellator math...")
        import ArchTessellation

        # 1000x1000 substrate
        substrate = Part.makePlane(1000, 1000)
        u, v, n, c = Arch.getFaceFrame(substrate)
        origin = ArchCovering.get_tile_grid_origin(substrate, c, u, v, "Bottom Left", 0, 0)

        # Scenario: 200x200 tiles, 50 mm joint.
        # Step is 250 mm. 1000/250 = 4 tiles per side. Total = 16.
        cfg = ArchTessellation.TileConfig(
            finish_mode="Solid Tiles", length=200, width=200, thickness=10, joint=50
        )
        tessellator = ArchTessellation.RectangularTessellator(cfg)
        result = tessellator.compute(substrate, origin, u, n)

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
        u, _v, n, _c = Arch.getFaceFrame(substrate)
        origin = App.Vector(0, 0, 0)

        # Case 1: invalid dimensions (< 1.0mm)
        cfg1 = ArchTessellation.TileConfig(
            finish_mode="Solid Tiles", length=0.5, width=100, thickness=0, joint=0
        )
        t1 = ArchTessellation.RectangularTessellator(cfg1)
        res1 = t1.compute(substrate, origin, u, n)
        self.assertEqual(res1.status, ArchTessellation.TessellationStatus.INVALID_DIMENSIONS)

        # Case 2: too many tiles (> 10,000)
        # 1000mm face / 5mm step = 200 divisions. 200^2 = 40,000 tiles.
        large_substrate = Part.makePlane(1000, 1000)
        cfg2 = ArchTessellation.TileConfig(
            finish_mode="Solid Tiles", length=4, width=4, thickness=0, joint=1
        )
        t2 = ArchTessellation.RectangularTessellator(cfg2)
        res2 = t2.compute(large_substrate, origin, u, n)
        self.assertEqual(res2.status, ArchTessellation.TessellationStatus.COUNT_TOO_HIGH)
        # In high-count mode the result is a single solid plus a grid overlay, not 40,000 separate
        # tile solids.
        self.assertIsNotNone(res2.geometry)
        self.assertGreater(res2.quantities.count_full, 10000)

    def test_resolveFace_indirection_suite(self):
        """Verify face resolution through various indirection types (Links, Clones, Binders)."""
        self.printTestMessage("resolveFace indirection suite...")
        box = self.document.addObject("Part::Box", "SourceBox")
        box.Length = box.Width = box.Height = 100.0
        self.document.recompute()
        expected_area = 10000.0

        # Draft Clone
        clone = Draft.make_clone(box)
        # App::Link
        link = self.document.addObject("App::Link", "AppLink")
        link.LinkedObject = box
        # SubShapeBinder
        binder = self.document.addObject("PartDesign::SubShapeBinder", "Binder")
        binder.Support = (box, ["Face1"])
        self.document.recompute()

        test_cases = [
            ("Draft Clone", clone),
            ("App::Link", link),
            ("LinkSub Tuple", (link, ["Face6"])),
            ("SubShapeBinder", binder),
        ]

        for label, target in test_cases:
            with self.subTest(indirection=label):
                face = Arch.resolveFace(target)
                self.assertIsNotNone(face, f"Failed to resolve face through {label}")
                self.assertIsInstance(face, Part.Face, f"Result through {label} is not a face")
                self.assertAlmostEqual(
                    face.Area,
                    expected_area,
                    places=3,
                    msg=f"Incorrect area resolved through {label}",
                )

    def test_3d_orientation_robustness(self):
        """Verify tiling accuracy on sloped/rotated 3D surfaces."""
        self.printTestMessage("3D orientation robustness...")
        # Create a face rotated 45 degrees around X
        base_wire = Draft.make_rectangle(1000, 1000)
        base_wire.Placement.Rotation = App.Rotation(App.Vector(1, 0, 0), 45)
        self.document.recompute()

        covering = Arch.makeCovering(base_wire)
        covering.TileLength, covering.TileWidth = 200.0, 200.0
        covering.JointWidth, covering.TileThickness = 10.0, 20.0
        self.document.recompute()

        # Area and Volume calculations should be invariant to rotation
        self.assertAlmostEqual(covering.NetArea.Value, 1000000.0, places=1)
        self.assertFalse(covering.Shape.isNull())
        # Bounding box should reflect the 45 degree tilt
        self.assertAlmostEqual(
            covering.Shape.BoundBox.ZLength, (1000 * 0.707) + (20 * 0.707), delta=5.0
        )

    def test_alignment_and_offset_grid(self):
        """Verify that Running Bond offsets and grid alignments shift the geometry correctly."""
        self.printTestMessage("alignment and offset grid...")
        # Integration test: verify the physical shift in the result.
        # Solid Tiles mode is required here because each tile is a separate solid; Parametric
        # Pattern mode produces one shared substrate solid whose center of mass is always the
        # face center regardless of alignment.
        covering = Arch.makeCovering((self.box, ["Face6"]))
        covering.FinishMode = "Solid Tiles"
        covering.TileLength, covering.TileWidth, covering.JointWidth = 200.0, 200.0, 10.0
        covering.TileThickness = 10.0

        covering.TileAlignment = "Bottom Left"
        self.document.recompute()
        com_bl = covering.Shape.Solids[0].CenterOfMass

        covering.TileAlignment = "Center"
        self.document.recompute()
        com_c = covering.Shape.Solids[0].CenterOfMass

        # Shift in joints changes material distribution, shifting the Center of Mass
        self.assertNotAlmostEqual(
            com_bl.x, com_c.x, places=3, msg="Center of Mass should shift when grid is re-aligned."
        )

    def test_quantity_takeoff_integrity(self):
        """Verify consistency between geometric results and BIM quantities including
        OuterPerimeter."""
        self.printTestMessage("quantity take-off integrity...")

        # Base is 1000x1000 Face (Face6 of 1000^3 box)
        base = (self.box, ["Face6"])
        perimeter_expected = 4000.0

        test_cases = [
            {
                "msg": "Positive Waste Scenario",
                # 300x300 tiles, 10mm joints. Step 310.
                # 1000 / 310 = 3.22 -> 4 tiles/row. 16 total.
                # 3 full tiles (930mm) + 1 partial (70mm) per row.
                # Full: 3x3=9. Partial: 16-9=7.
                # Gross = 16 * 90000 = 1,440,000.
                # Net = 1,000,000. Waste = 440,000.
                # Joints: 3 internal lines per direction. 3 * 1000 * 2 = 6000.
                "TileLength": 300.0,
                "TileWidth": 300.0,
                "JointWidth": 10.0,
                "TileAlignment": "Bottom Left",
                "expected_joints": 6000.0,
                "expected_perimeter": perimeter_expected,
                "expected_gross": 1440000.0,
                "expected_waste": 440000.0,
                "expected_full": 9,
                "expected_partial": 7,
                "check_waste": True,
            },
            {
                "msg": "Clamped Waste Scenario",
                # 400x400 tiles, 100mm joints. Step 500.
                # 1000 / 500 = 2 tiles/row. 4 total. Perfect fit.
                # Gross = 4 * 160000 = 640,000.
                # Net = 1,000,000. Waste = 0 (Clamped).
                # Joints: 2 internal lines per direction (centers at 450, 950).
                # 2 * 1000 * 2 = 4000.
                "TileLength": 400.0,
                "TileWidth": 400.0,
                "JointWidth": 100.0,
                "TileAlignment": "Bottom Left",
                "expected_joints": 4000.0,
                "expected_perimeter": perimeter_expected,
                "expected_gross": 640000.0,
                "expected_waste": 0.0,
                "expected_full": 4,
                "expected_partial": 0,
                "check_waste": True,
            },
            {
                "msg": "Butt Joint Geometric (Center Alignment)",
                # 200x200 tiles, 0mm joint. Center alignment shifts origin by -100,-100 so the local
                # substrate spans (-400,-400) to (600,600).
                # Grid line formula: pos = i*step + tile_dim = i*200 + 200.
                # Across the local substrate, lines land at -400, -200, 0, 200, 400, 600 — 6 per
                # direction.
                # The outermost lines (-400 and 600) sit exactly on the face boundary, and
                # Part.Shape.common() includes on-boundary edges, so all 6 lines count: 6 * 1000 * 2
                # = 12000.
                "TileLength": 200.0,
                "TileWidth": 200.0,
                "JointWidth": 0.0,
                "TileAlignment": "Center",
                "expected_joints": 12000.0,
                "expected_perimeter": perimeter_expected,
                "expected_full": 25,
                "expected_partial": 0,
                "check_waste": False,
            },
        ]

        for case in test_cases:
            with self.subTest(msg=case["msg"]):
                covering = Arch.makeCovering(base)
                covering.FinishMode = "Solid Tiles"
                covering.TileLength = case["TileLength"]
                covering.TileWidth = case["TileWidth"]
                covering.JointWidth = case["JointWidth"]
                covering.TileThickness = 10.0
                covering.TileAlignment = case["TileAlignment"]
                self.document.recompute()

                # Joint and Perimeter Verification
                self.assertAlmostEqual(
                    covering.TotalJointLength.Value,
                    case["expected_joints"],
                    delta=1.0,
                    msg=f"{case['msg']}: Joint length mismatch",
                )
                self.assertAlmostEqual(
                    covering.OuterPerimeter.Value,
                    case["expected_perimeter"],
                    delta=1.0,
                    msg=f"{case['msg']}: Perimeter length mismatch",
                )

                # Tile Counts
                self.assertEqual(
                    covering.CountFullTiles,
                    case["expected_full"],
                    msg=f"{case['msg']}: Full tile count mismatch",
                )
                self.assertEqual(
                    covering.CountPartialTiles,
                    case["expected_partial"],
                    msg=f"{case['msg']}: Partial tile count mismatch",
                )

                # Areas
                if case["check_waste"]:
                    self.assertAlmostEqual(
                        covering.GrossArea.Value,
                        case["expected_gross"],
                        delta=1.0,
                        msg=f"{case['msg']}: Gross area mismatch",
                    )
                    self.assertAlmostEqual(
                        covering.WasteArea.Value,
                        case["expected_waste"],
                        delta=1.0,
                        msg=f"{case['msg']}: Waste area mismatch",
                    )

    def test_extreme_mode_analytical_fallback(self):
        """Verify joint length and perimeter in analytical fallback mode (>100k tiles)."""
        self.printTestMessage("extreme mode analytical fallback...")
        large_box = self.document.addObject("Part::Box", "LargeBox")
        large_box.Length = 20000.0
        large_box.Width = 20000.0
        large_box.Height = 100.0
        self.document.recompute()

        # 50x50 tiles, 0 joint -> 160,000 tiles (triggers >100k threshold)
        face_area = 400000000.0  # 20m x 20m
        face_perimeter = 80000.0
        tile_l = 50.0
        tile_w = 50.0

        covering = Arch.makeCovering((large_box, ["Face6"]))
        covering.FinishMode = "Solid Tiles"
        covering.TileLength = tile_l
        covering.TileWidth = tile_w
        covering.JointWidth = 0.0
        covering.TileAlignment = "Bottom Left"
        self.document.recompute()

        # Analytical joint length: sum of all grid lines minus the perimeter.
        # Grid density = (Area / Step_U) + (Area / Step_V) = 8M + 8M = 16,000,000 mm.
        total_grid = (face_area / tile_l) + (face_area / tile_w)
        expected_joints = total_grid - face_perimeter

        self.assertAlmostEqual(
            covering.TotalJointLength.Value,
            expected_joints,
            delta=1000.0,  # Float tolerance for large numbers
        )
        self.assertAlmostEqual(covering.OuterPerimeter.Value, face_perimeter, delta=1.0)

    def test_monolithic_mode(self):
        """Verify that Monolithic mode produces a single solid with correct quantities."""
        self.printTestMessage("monolithic mode...")
        base = (self.box, ["Face6"])
        covering = Arch.makeCovering(base)
        covering.FinishMode = "Monolithic"
        covering.TileThickness = 15.0
        self.document.recompute()

        self.assertFalse(covering.Shape.isNull())
        self.assertEqual(len(covering.Shape.Solids), 1, "Monolithic should be a single solid.")
        # Thickness is applied along the face normal
        self.assertAlmostEqual(covering.Shape.BoundBox.ZLength, 15.0, places=3)
        # Net area equals the base face area; tile counts are 1 full, 0 partial
        self.assertAlmostEqual(covering.NetArea.Value, 1000000.0, places=1)
        self.assertEqual(covering.CountFullTiles, 1)
        self.assertEqual(covering.CountPartialTiles, 0)

    def test_running_bond_stagger(self):
        """Verify that running bond shifts alternating rows by the configured offset."""
        self.printTestMessage("running bond stagger...")
        base = (self.box, ["Face6"])

        # Baseline: stacked (no stagger)
        covering_stacked = Arch.makeCovering(base)
        covering_stacked.FinishMode = "Solid Tiles"
        covering_stacked.TileLength = 200.0
        covering_stacked.TileWidth = 100.0
        covering_stacked.JointWidth = 5.0
        covering_stacked.TileThickness = 10.0
        covering_stacked.StaggerType = "Stacked (None)"
        self.document.recompute()

        # Half Bond: every other row is shifted by tile_length / 2
        covering_bond = Arch.makeCovering(base)
        covering_bond.FinishMode = "Solid Tiles"
        covering_bond.TileLength = 200.0
        covering_bond.TileWidth = 100.0
        covering_bond.JointWidth = 5.0
        covering_bond.TileThickness = 10.0
        covering_bond.StaggerType = "Half Bond (1/2)"
        self.document.recompute()

        # The vertex positions must differ because alternating rows are offset
        sum_stacked = sum(v.Point.x for v in covering_stacked.Shape.Vertexes)
        sum_bond = sum(v.Point.x for v in covering_bond.Shape.Vertexes)
        self.assertNotAlmostEqual(
            sum_stacked,
            sum_bond,
            places=1,
            msg="Half Bond stagger should shift vertex positions relative to Stacked.",
        )

    def test_third_bond_stagger_geometry(self):
        """Verify that Third Bond shifts consecutively across a 3-row cycle, not just alternating."""
        self.printTestMessage("third bond stagger geometry...")

        # Use a non-square substrate to guarantee U=X and V=Y orientation
        base_face = Part.makePlane(2000, 1000)
        feature = self.document.addObject("Part::Feature", "TestPlane")
        feature.Shape = base_face
        self.document.recompute()

        covering = Arch.makeCovering((feature, ["Face1"]))

        # Tile=300, Joint=1 -> Step=301. Stagger offset = 300/3 = 100.
        covering.FinishMode = "Solid Tiles"
        covering.TileLength = 300.0
        covering.TileWidth = 100.0
        covering.JointWidth = 1.0
        covering.TileThickness = 10.0
        covering.StaggerType = "Third Bond (1/3)"
        covering.TileAlignment = "Bottom Left"
        self.document.recompute()

        row_centers = {}
        for solid in covering.Shape.Solids:
            # STRICT FILTER: A perfect full tile is exactly 300 * 100 * 10 = 300,000 mm^3.
            # This rejects the 295,000 mm^3 partial tile at the right edge of Row 2.
            if solid.Volume > 299999:
                com = solid.CenterOfMass
                row_idx = int(com.y // 101)  # step_v is 101
                if row_idx not in row_centers:
                    row_centers[row_idx] = []
                row_centers[row_idx].append(com.x)

        self.assertIn(0, row_centers, "Row 0 should have full tiles")
        self.assertIn(1, row_centers, "Row 1 should have full tiles")
        self.assertIn(2, row_centers, "Row 2 should have full tiles")

        # We only check the first full tile found in each row
        x_mod_row0 = row_centers[0][0] % 301
        self.assertAlmostEqual(
            x_mod_row0, 150.0, delta=1.0, msg="Row 0 centers should be unshifted."
        )

        x_mod_row1 = row_centers[1][0] % 301
        self.assertAlmostEqual(
            x_mod_row1, 250.0, delta=1.0, msg="Row 1 should be shifted by 1/3 (100mm)."
        )

        x_mod_row2 = row_centers[2][0] % 301
        self.assertAlmostEqual(
            x_mod_row2,
            49.0,
            delta=1.0,
            msg="Row 2 should be shifted by 2/3 (200mm). Fails if stagger only alternates every 2 rows.",
        )

    def test_custom_stagger_geometry_cycle(self):
        """Verify that Custom stagger correctly identifies fractional cycles and shifts accordingly."""
        self.printTestMessage("custom stagger geometry cycle...")

        # Use a non-square substrate to guarantee U=X and V=Y orientation
        base_face = Part.makePlane(2000, 1000)
        feature = self.document.addObject("Part::Feature", "TestPlaneCustom")
        feature.Shape = base_face
        self.document.recompute()

        covering = Arch.makeCovering((feature, ["Face1"]))

        # Tile=199, Joint=1 -> Step_u=200.
        # Custom Stagger=40 -> 40/200 = 0.2 (1/5th period). This creates a 5-row cycle.
        covering.FinishMode = "Solid Tiles"
        covering.TileLength = 199.0
        covering.TileWidth = 49.0  # Step_v=50
        covering.JointWidth = 1.0
        covering.TileThickness = 10.0
        covering.StaggerType = "Custom"
        covering.StaggerCustom = 40.0
        covering.TileAlignment = "Bottom Left"
        self.document.recompute()

        row_centers = {}
        for solid in covering.Shape.Solids:
            # Strict filter: 199 * 49 * 10 = 97,510 mm^3.
            if solid.Volume > 97500:
                com = solid.CenterOfMass
                row_idx = int(com.y // 50)  # step_v is 50
                if row_idx not in row_centers:
                    row_centers[row_idx] = []
                row_centers[row_idx].append(com.x)

        # Check that we generated full tiles across the full 5-row cycle
        for i in range(5):
            self.assertIn(i, row_centers, f"Row {i} should have full tiles")

        # Base X-center for an unshifted tile starting at 0 is 199/2 = 99.5
        # The period is 200.
        expected_centers = {
            0: 99.5,  # 0 shift
            1: 139.5,  # +40
            2: 179.5,  # +80
            3: 19.5,  # +120 -> 219.5 % 200 = 19.5
            4: 59.5,  # +160 -> 259.5 % 200 = 59.5
        }

        for i in range(5):
            x_mod = row_centers[i][0] % 200
            self.assertAlmostEqual(
                x_mod,
                expected_centers[i],
                delta=1.0,
                msg=f"Row {i} center mismatch. Expected {expected_centers[i]}, got {x_mod}.",
            )

    def test_border_setback_integration(self):
        """Verify that BorderSetback reduces the tiling area."""
        self.printTestMessage("border setback integration...")
        base = (self.box, ["Face6"])  # 1000x1000 face

        # Without setback
        covering_no_sb = Arch.makeCovering(base)
        covering_no_sb.FinishMode = "Monolithic"
        covering_no_sb.TileThickness = 5.0
        covering_no_sb.BorderSetback = 0.0
        self.document.recompute()

        # With 100 mm setback: tiling area shrinks to 800x800
        covering_sb = Arch.makeCovering(base)
        covering_sb.FinishMode = "Monolithic"
        covering_sb.TileThickness = 5.0
        covering_sb.BorderSetback = 100.0
        self.document.recompute()

        self.assertFalse(covering_sb.Shape.isNull())
        # Net area should reflect the inset face (800x800 = 640,000)
        self.assertAlmostEqual(covering_sb.NetArea.Value, 640000.0, delta=10.0)
        self.assertLess(
            covering_sb.NetArea.Value,
            covering_no_sb.NetArea.Value,
            msg="Setback should reduce the tiled area.",
        )

    def test_apply_setback_preserves_holes(self):
        """Verify that apply_setback shrinks the outer perimeter but keeps inner holes."""
        self.printTestMessage("apply_setback preserves holes...")
        # 500x500 face with a 100x100 hole in the centre
        outer = Part.makePolygon(
            [
                App.Vector(0, 0, 0),
                App.Vector(500, 0, 0),
                App.Vector(500, 500, 0),
                App.Vector(0, 500, 0),
                App.Vector(0, 0, 0),
            ]
        )
        # Inner wire must be CW (reversed winding) so OCCT treats it as a hole.
        # CCW winding would cause the area to be added instead of subtracted.
        inner = Part.makePolygon(
            [
                App.Vector(200, 200, 0),
                App.Vector(200, 300, 0),
                App.Vector(300, 300, 0),
                App.Vector(300, 200, 0),
                App.Vector(200, 200, 0),
            ]
        )
        face_with_hole = Part.Face([outer, inner])
        # Original area: 500*500 - 100*100 = 240,000
        self.assertAlmostEqual(face_with_hole.Area, 240000.0, places=1)

        # Apply a 50 mm setback. Outer shrinks to 400x400, hole stays at 100x100.
        # Expected area: 400*400 - 100*100 = 150,000
        result = ArchCovering.apply_setback(face_with_hole, 50.0)
        self.assertFalse(result.isNull())
        self.assertAlmostEqual(result.Area, 150000.0, delta=10.0)
        self.assertEqual(
            len(result.Wires),
            2,
            "Setback result should retain one outer wire and one hole wire.",
        )

    def test_tile_grid_rotation(self):
        """A non-zero Rotation shifts the tile grid.

        Checked against three angles: 0 (baseline), 45 (layout should differ), and 90 (on a square
        face with square tiles, the 90-degree layout matches the 0-degree one, so vertex count and
        bounding box must be identical).
        """
        self.printTestMessage("tile grid rotation...")
        base = (self.box, ["Face6"])

        covering_0 = Arch.makeCovering(base)
        covering_0.FinishMode = "Solid Tiles"
        covering_0.TileLength = 200.0
        covering_0.TileWidth = 200.0
        covering_0.JointWidth = 5.0
        covering_0.TileThickness = 10.0
        covering_0.Rotation = 0.0
        self.document.recompute()

        covering_45 = Arch.makeCovering(base)
        covering_45.FinishMode = "Solid Tiles"
        covering_45.TileLength = 200.0
        covering_45.TileWidth = 200.0
        covering_45.JointWidth = 5.0
        covering_45.TileThickness = 10.0
        covering_45.Rotation = 45.0
        self.document.recompute()

        covering_90 = Arch.makeCovering(base)
        covering_90.FinishMode = "Solid Tiles"
        covering_90.TileLength = 200.0
        covering_90.TileWidth = 200.0
        covering_90.JointWidth = 5.0
        covering_90.TileThickness = 10.0
        covering_90.Rotation = 90.0
        self.document.recompute()

        self.assertFalse(covering_45.Shape.isNull())
        self.assertFalse(covering_90.Shape.isNull())

        # 45 degrees: vertex coordinate-sum must differ from the 0-degree baseline.
        sum_0 = sum(v.Point.x + v.Point.y for v in covering_0.Shape.Vertexes)
        sum_45 = sum(v.Point.x + v.Point.y for v in covering_45.Shape.Vertexes)
        self.assertNotAlmostEqual(
            sum_0,
            sum_45,
            places=1,
            msg="45-degree rotation should produce different tile geometry.",
        )

        # 90 degrees on a square face with square tiles: the layout matches 0 degrees,
        # so vertex count and bounding box must be identical.
        self.assertEqual(len(covering_0.Shape.Vertexes), len(covering_90.Shape.Vertexes))
        bb_0 = covering_0.Shape.BoundBox
        bb_90 = covering_90.Shape.BoundBox
        self.assertAlmostEqual(bb_0.XLength, bb_90.XLength, places=2)
        self.assertAlmostEqual(bb_0.YLength, bb_90.YLength, places=2)
        self.assertAlmostEqual(bb_0.ZLength, bb_90.ZLength, places=2)

    def test_resolve_pd_object_passthrough(self):
        """Verify that resolve_pd_object returns a non-PartDesign object unchanged."""
        self.printTestMessage("resolve_pd_object passthrough...")
        import ArchCommands

        # A plain Part::Box has no PartDesign::Body parent — must return unchanged.
        result = ArchCommands.resolve_pd_object(self.box)
        self.assertIs(result, self.box)

    def test_resolve_pd_object_with_body(self):
        """resolve_pd_object returns the parent Body when given a PartDesign feature."""
        self.printTestMessage("resolve_pd_object with body...")
        import ArchCommands

        body = self.document.addObject("PartDesign::Body", "TestBody")
        feature = self.document.addObject("PartDesign::AdditiveBox", "PadBox")
        feature.Length, feature.Width, feature.Height = 100.0, 100.0, 100.0
        body.addObject(feature)
        self.document.recompute()

        # Given the feature, the body must be returned; given the body directly, it is unchanged.
        self.assertIs(ArchCommands.resolve_pd_object(feature), body)
        self.assertIs(ArchCommands.resolve_pd_object(body), body)

    def test_get_tile_grid_origin_all_presets(self):
        """Verify get_tile_grid_origin places the grid anchor correctly for all presets."""
        self.printTestMessage("get_tile_grid_origin all presets...")
        # 1500x1000 face at the document origin. The 1500 mm edges are the longest, so
        # getFaceFrame reliably returns U=+X, V=+Y (edge-aligned, unambiguous for non-square).
        substrate = Part.makePlane(1500, 1000)
        u, v, n, c = Arch.getFaceFrame(substrate)
        tile_l, tile_w = 200.0, 150.0

        origin_bl = ArchCovering.get_tile_grid_origin(
            substrate, c, u, v, "Bottom Left", tile_l, tile_w
        )
        origin_br = ArchCovering.get_tile_grid_origin(
            substrate, c, u, v, "Bottom Right", tile_l, tile_w
        )
        origin_tl = ArchCovering.get_tile_grid_origin(
            substrate, c, u, v, "Top Left", tile_l, tile_w
        )
        origin_tr = ArchCovering.get_tile_grid_origin(
            substrate, c, u, v, "Top Right", tile_l, tile_w
        )
        origin_ce = ArchCovering.get_tile_grid_origin(substrate, c, u, v, "Center", tile_l, tile_w)
        origin_cu = ArchCovering.get_tile_grid_origin(
            substrate, c, u, v, "Custom", tile_l, tile_w, App.Vector(100, 50, 0)
        )

        # Bottom Left: tile corner at (0, 0) — no shift.
        self.assertAlmostEqual(origin_bl.x, 0.0, places=3)
        self.assertAlmostEqual(origin_bl.y, 0.0, places=3)

        # Bottom Right: right tile edge at x=1500, so origin at x = 1500 - tile_l = 1300.
        self.assertAlmostEqual(origin_br.x, 1500.0 - tile_l, places=3)
        self.assertAlmostEqual(origin_br.y, 0.0, places=3)

        # Top Left: top tile edge at y=1000, so origin at y = 1000 - tile_w = 850.
        self.assertAlmostEqual(origin_tl.x, 0.0, places=3)
        self.assertAlmostEqual(origin_tl.y, 1000.0 - tile_w, places=3)

        # Top Right: both shifts applied.
        self.assertAlmostEqual(origin_tr.x, 1500.0 - tile_l, places=3)
        self.assertAlmostEqual(origin_tr.y, 1000.0 - tile_w, places=3)

        # Center: tile centre lands at face centre (750, 500).
        self.assertAlmostEqual(origin_ce.x, 750.0 - tile_l / 2, places=3)
        self.assertAlmostEqual(origin_ce.y, 500.0 - tile_w / 2, places=3)

        # Custom: absolute offset from face centre.
        self.assertAlmostEqual(origin_cu.x, c.x + 100.0, places=3)
        self.assertAlmostEqual(origin_cu.y, c.y + 50.0, places=3)

        # All five presets must produce distinct origins.
        presets = [origin_bl, origin_br, origin_tl, origin_tr, origin_ce]
        for i, a in enumerate(presets):
            for j, b in enumerate(presets):
                if i != j:
                    self.assertFalse(
                        a.isEqual(b, 0.1),
                        msg=f"Preset origins[{i}] and [{j}] should not coincide.",
                    )

    def test_get_face_frame_edge_aligned(self):
        """getFaceFrame U axis follows the longest boundary edge, not world axes.

        Regression test for the edge-aligned grid change. A 1000×200 rectangle
        rotated 30° in XY is used because the old world-axis implementation would
        snap U to +X (dominant axis), while the correct edge-aligned implementation
        must follow the long edges at 30°.
        """
        import math

        self.printTestMessage("getFaceFrame edge alignment...")
        angle = math.radians(30)
        cos_a, sin_a = math.cos(angle), math.sin(angle)
        long_dir = App.Vector(cos_a, sin_a, 0)
        short_dir = App.Vector(-sin_a, cos_a, 0)
        L, W = 1000.0, 200.0

        p0 = App.Vector(0, 0, 0)
        p1 = p0 + long_dir * L
        p2 = p1 + short_dir * W
        p3 = p0 + short_dir * W

        wire = Part.makePolygon([p0, p1, p2, p3, p0])
        face = Part.Face(wire)

        u, v, n, c = Arch.getFaceFrame(face)

        # U must be parallel to the long-edge direction (abs handles ±stabilisation).
        alignment = abs(u.dot(long_dir))
        self.assertAlmostEqual(
            alignment,
            1.0,
            places=3,
            msg=f"U ({u}) should align with the 30° long edge, got dot={alignment:.4f}",
        )
        # V must be perpendicular to U and parallel to the short-edge direction.
        v_alignment = abs(v.dot(short_dir))
        self.assertAlmostEqual(
            v_alignment,
            1.0,
            places=3,
            msg=f"V ({v}) should align with the short edge (120°), got dot={v_alignment:.4f}",
        )

    def test_grid_origin_uv_space_portrait(self):
        """get_tile_grid_origin uses UV-space coordinates for corner presets.

        A portrait face (longer in Y) has U=+Y and V=−X in the edge-aligned basis. "Bottom Left"
        = min-U, min-V. In world terms, min-U corresponds to min-Y (south) and min-V corresponds
        to max-X (east), so the anchor lands at the face's south-east world corner. Labels are
        face-relative, not world-relative.
        """
        self.printTestMessage("grid origin UV-space (portrait face)...")

        # 200 × 1000 portrait face: longer in Y → edge-aligned gives U=+Y, V=−X.
        substrate = Part.makePlane(200, 1000)
        u, v, n, c = Arch.getFaceFrame(substrate)

        # Confirm the basis is actually portrait (U dominates Y, V dominates X).
        self.assertGreater(abs(u.y), abs(u.x), "Expected U along Y for portrait face")

        tile_l, tile_w = 100.0, 100.0

        # "Bottom Left" in UV space = min-U, min-V. For this portrait face:
        # min-U corresponds to vertex at y=0; min-V corresponds to vertex at x=200.
        # The min-U/min-V vertex is (200, 0) — the face's south-east world corner.
        origin_bl = ArchCovering.get_tile_grid_origin(
            substrate, c, u, v, "Bottom Left", tile_l, tile_w
        )
        self.assertAlmostEqual(origin_bl.x, 200.0, places=2)
        self.assertAlmostEqual(origin_bl.y, 0.0, places=2)

        # "Top Right" in UV space = max-U, max-V, shifted inward by one tile.
        # max-U/max-V vertex is (0, 1000) — the face's north-west world corner.
        # Tile extends in +U (+Y) and +V (-X) from the anchor, so origin is shifted by
        # tile_l in -U (−Y) and tile_w in -V (+X).
        origin_tr = ArchCovering.get_tile_grid_origin(
            substrate, c, u, v, "Top Right", tile_l, tile_w
        )
        self.assertAlmostEqual(origin_tr.x, 0.0 + tile_w, places=2)
        self.assertAlmostEqual(origin_tr.y, 1000.0 - tile_l, places=2)

    def test_grid_origin_rotation_invariant(self):
        """Corner-preset anchor follows the base face under rotation, not the world.

        ArchCovering stores ReferenceDirection in base-local coordinates. When the base is
        rotated, execute() reconstructs the world U as ``base.Placement.Rotation.multVec(stored)``
        so the UV basis rotates with the base. Vertices and basis rotate together, the UV
        projections are invariant, and the same physical corner of the face remains the anchor.
        This test simulates that behavior at the function level.
        """
        self.printTestMessage("grid origin rotation invariance...")

        substrate = Part.makePlane(1500, 1000)
        tile_l, tile_w = 200.0, 150.0

        # Seed the stored reference in base-local coordinates. The initial base has identity
        # rotation, so local == world for this seed.
        u0, v0, _, c0 = Arch.getFaceFrame(substrate)
        stored_local_ref = App.Vector(u0)
        origin_before = ArchCovering.get_tile_grid_origin(
            substrate, c0, u0, v0, "Bottom Left", tile_l, tile_w
        )

        # Rotate the base 45 degrees about Z. In real usage execute() reconstructs the world
        # reference as base_rotation.multVec(stored_local_ref).
        rot = App.Placement(App.Vector(0, 0, 0), App.Rotation(App.Vector(0, 0, 1), 45))
        rotated = substrate.copy()
        rotated.Placement = rot.multiply(rotated.Placement)
        world_ref = rot.Rotation.multVec(stored_local_ref)

        u_rot, v_rot, _, c_rot = Arch.getFaceFrame(rotated, reference_direction=world_ref)
        origin_after = ArchCovering.get_tile_grid_origin(
            rotated, c_rot, u_rot, v_rot, "Bottom Left", tile_l, tile_w
        )

        # The anchor should follow the face: rotating the pre-rotation anchor by the same
        # rotation should match the post-rotation anchor.
        expected = rot.multVec(origin_before)
        self.assertAlmostEqual(
            origin_after.x,
            expected.x,
            places=2,
            msg=f"Rotated anchor X: got {origin_after.x}, expected {expected.x}",
        )
        self.assertAlmostEqual(
            origin_after.y,
            expected.y,
            places=2,
            msg=f"Rotated anchor Y: got {origin_after.y}, expected {expected.y}",
        )

    def test_getFaceFrame_reference_direction_stability(self):
        """A stored reference direction keeps U stable when the longest edge changes.

        Without a reference direction, a 1500×1000 face has U along X (the 1500 mm edge).
        Changing the face to 1000×1500 would flip U to Y. With the original U stored as a
        reference direction, U must remain along X.
        """
        self.printTestMessage("getFaceFrame reference direction stability...")

        # Initial face: longest edge along X → U ≈ +X.
        face_landscape = Part.makePlane(1500, 1000)
        u_initial, _, _, _ = Arch.getFaceFrame(face_landscape)
        self.assertGreater(abs(u_initial.x), 0.9, "Initial U should be along X")

        # Changed face: longest edge now along Y.
        face_portrait = Part.makePlane(1000, 1500)

        # Without reference: U flips to Y.
        u_no_ref, _, _, _ = Arch.getFaceFrame(face_portrait)
        self.assertGreater(
            abs(u_no_ref.y), 0.9, "Without reference, U should follow longest edge (Y)"
        )

        # With stored reference: U stays along X.
        u_with_ref, _, _, _ = Arch.getFaceFrame(face_portrait, reference_direction=u_initial)
        self.assertGreater(
            abs(u_with_ref.x),
            0.9,
            f"With reference, U should stay along X, got {u_with_ref}",
        )

    def test_getFaceFrame_reference_direction_degenerate_fallback(self):
        """A reference direction parallel to the face normal falls back to longest-edge."""
        self.printTestMessage("getFaceFrame reference direction degenerate fallback...")

        # XY plane face: normal is +Z.
        face = Part.makePlane(1500, 1000)
        # Reference direction along Z is parallel to the normal — should be rejected.
        u, _, _, _ = Arch.getFaceFrame(face, reference_direction=App.Vector(0, 0, 1))
        # Fallback: longest edge along X.
        self.assertGreater(
            abs(u.x), 0.9, f"Degenerate reference should fall back to longest edge, got {u}"
        )

    def test_reference_direction_roundtrip(self):
        """get/set_reference_direction round-trip world U even when the base is rotated.

        The stored ReferenceDirection is in base-local coordinates, so persisting a world
        vector and reading it back must survive the base's rotation. An unseeded covering
        (zero-length stored vector) must return None. Execute() normally seeds the property,
        so the test resets it to zero before checking the unseeded branch.
        """
        self.printTestMessage("reference direction round-trip...")

        # Rotate the base 30 degrees about Z so local and world coordinates differ.
        self.box.Placement = App.Placement(
            App.Vector(0, 0, 0), App.Rotation(App.Vector(0, 0, 1), 30)
        )
        self.document.recompute()

        covering = Arch.makeCovering((self.box, ["Face6"]))
        self.document.recompute()

        # Unseeded: execute() seeds on recompute, so explicitly clear to test the None branch.
        covering.ReferenceDirection = App.Vector(0, 0, 0)
        self.assertIsNone(ArchCovering.get_reference_direction(covering))

        # Round-trip a world U vector. The stored property must be in base-local coordinates
        # (differing from the world input under a 30° base rotation), and
        # get_reference_direction must reconstruct the original world vector.
        world_u = App.Vector(1, 0, 0)
        ArchCovering.set_reference_direction(covering, world_u)

        stored = covering.ReferenceDirection
        self.assertFalse(
            stored.isEqual(world_u, 1e-6),
            msg="Stored ReferenceDirection should be in base-local coordinates, not world.",
        )

        recovered = ArchCovering.get_reference_direction(covering)
        self.assertIsNotNone(recovered)
        self.assertAlmostEqual(recovered.x, world_u.x, places=6)
        self.assertAlmostEqual(recovered.y, world_u.y, places=6)
        self.assertAlmostEqual(recovered.z, world_u.z, places=6)

    def test_hatch_rotation(self):
        """Hatch pattern mode applies the Rotation scalar, producing different geometry."""
        self.printTestMessage("hatch rotation...")
        base = (self.box, ["Face6"])

        covering_0 = Arch.makeCovering(base)
        covering_0.FinishMode = "Hatch Pattern"
        covering_0.TileLength = 150.0
        covering_0.TileWidth = 150.0
        covering_0.Rotation = 0.0
        self.document.recompute()

        covering_45 = Arch.makeCovering(base)
        covering_45.FinishMode = "Hatch Pattern"
        covering_45.TileLength = 150.0
        covering_45.TileWidth = 150.0
        covering_45.Rotation = 45.0
        self.document.recompute()

        self.assertFalse(covering_0.Shape.isNull())
        self.assertFalse(covering_45.Shape.isNull())

        # The hatch lines differ geometrically between the two rotations. Use a simple
        # vertex coordinate-sum invariant — it changes when lines rotate, even when the
        # overall bounding box is similar.
        sum_0 = sum(v.Point.x + v.Point.y for v in covering_0.Shape.Vertexes)
        sum_45 = sum(v.Point.x + v.Point.y for v in covering_45.Shape.Vertexes)
        self.assertNotAlmostEqual(
            sum_0,
            sum_45,
            places=1,
            msg="Hatch rotation should change generated line geometry.",
        )
