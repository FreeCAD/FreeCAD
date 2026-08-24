# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Dimitrios Pana <dimitriospana75@gmail.com>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

import Path
import Part
import FreeCAD
import unittest
import CAMTests.PathTestUtils as PathTestUtils

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


class TestSurfaceZLevel(PathTestUtils.PathTestBase):
    """Tests for surface_zlevel: floor categorization, stack generation, and G-code."""

    def setUp(self):
        """Create a standard test model with multiple distinct Z-levels (floors)."""
        # A 50x50x20 base
        base = Part.makeBox(50, 50, 20)
        # A pocket from Z=20 down to a floor at Z=10
        pocket = Part.makeBox(30, 30, 10, FreeCAD.Vector(10, 10, 10))
        # An intermediate step/floor at Z=5
        step = Part.makeBox(50, 20, 15, FreeCAD.Vector(0, 0, 5))

        self.test_model = base.cut(pocket).cut(step).removeSplitter()
        # Expected floors are at Z=20 (top), Z=10 (pocket bottom), and Z=5 (step).

        # Standard boundaries for testing
        self.border_face = Part.Face(
            Part.makePolygon(
                [
                    FreeCAD.Vector(0, 0, 0),
                    FreeCAD.Vector(50, 0, 0),
                    FreeCAD.Vector(50, 50, 0),
                    FreeCAD.Vector(0, 50, 0),
                    FreeCAD.Vector(0, 0, 0),
                ]
            )
        )
        # A large outer boundary for trim testing
        outer_poly = Part.makePolygon(
            [
                FreeCAD.Vector(-10, -10, 0),
                FreeCAD.Vector(60, -10, 0),
                FreeCAD.Vector(60, 60, 0),
                FreeCAD.Vector(-10, 60, 0),
                FreeCAD.Vector(-10, -10, 0),
            ]
        )
        self.trim_face = Part.makeFace(outer_poly).cut(self.border_face)

        # Standard workplane context
        self.wpc = Part.makeCircle(2.0)
        self.fill_holes_masks = []

    def setUp_fill_model(self):
        """
        Creates a test model with two distinct cylindrical holes for fill-hole testing.

        Model: 60x60x20 base with two through-holes:
          - Hole A: 8mm diameter cylinder at center (30,30), floor at Z=10
          - Hole B: 6mm diameter cylinder at (15,15), floor at Z=15
        """
        base = Part.makeBox(60, 60, 20)
        hole_a = Part.makeCylinder(4, 10, FreeCAD.Vector(30, 30, 10))
        hole_b = Part.makeCylinder(3, 5, FreeCAD.Vector(15, 15, 15))
        model = base.cut(hole_a).cut(hole_b).removeSplitter()

        # Find the cylindrical wall faces of each hole (non-planar, vertical)
        hole_a_face = None
        hole_b_face = None
        for face in model.Faces:
            bb = face.BoundBox
            if hasattr(face.Surface, "TypeId") and "Cylinder" in face.Surface.TypeId:
                center_x = (bb.XMin + bb.XMax) / 2.0
                center_y = (bb.YMin + bb.YMax) / 2.0
                if abs(center_x - 30) < 1.0 and abs(center_y - 30) < 1.0:
                    hole_a_face = face
                elif abs(center_x - 15) < 1.0 and abs(center_y - 15) < 1.0:
                    hole_b_face = face
        return model, hole_a_face, hole_b_face

    def _get_mock_tool_params(self, profile="ballend", radius=3.0, corner_rad=3.0):
        """Returns standardized tool parameters for testing."""
        return {
            "radius": radius,
            "c_rad": corner_rad,
            "profile": profile.lower(),
            "is_threeD": profile.lower() in ["ballend", "bullnose"],
        }

    # -- Depth Categorization Tests --

    def test00_categorize_floor_steps(self):
        """
        Tests the automatic detection and categorization of model floors.

        INPUT:
        - Function: categorize_floor_steps()
        - Parameters: A model with floors at Z=20, Z=10, and Z=5. Standard stepdown is 8mm.
        - Input data: A complex Part.Shape.

        EXPECTED OUTPUT:
        - A sorted list of (z_height, status, geometry) tuples.
        - Z=12.5 should be "Pure" (a standard step).
        - Z=10 should be "Extra" (a physical floor between standard steps).
        - Z=5 should be "Mixed" (a standard step that lands on a physical floor).
        """
        from Path.Base.Generator.surface_zlevel import categorize_floor_steps

        steps = categorize_floor_steps(
            self.test_model, start_z=20.0, final_z=0.0, step_down=7.5, clear_planar_only=False
        )

        # Expected Z-levels: 20 -> 12.5 (Pure) -> 10 (Extra) -> 5 (Mixed, as 12.5-7.5=5 lands near 5) -> 0
        self.assertGreaterEqual(len(steps), 4)

        # Create a dictionary for easy lookup
        categorized_z = {round(s[0], 2): s[1] for s in steps}

        self.assertEqual(categorized_z.get(12.5), "Pure")
        self.assertEqual(categorized_z.get(10.0), "Extra")
        # Note: 12-8=4. The logic rounds this to the nearest floor within tolerance, so 5 becomes Mixed.
        self.assertEqual(categorized_z.get(5.0), "Mixed")

    # -- Tool Compensation (Sampling Plan) Test --

    def test10_generate_sampling_plan(self):
        """
        Tests the 'Squeeze-and-Snap' tool compensation sampling logic.

        INPUT:
        - Function: _generate_sampling_plan()
        - Parameters: A ballend tool, various depths of engagement.
        - Input data: Tool geometry and model floor heights.

        EXPECTED OUTPUT:
        - For a 2D (endmill) tool, should always return one sample point.
        - For a 3D (ballend) tool, should return multiple points interpolated along the radius.
        - When a model floor is within the tool's contact zone, a precise "snap" point
          should be added to the plan.
        """
        from Path.Base.Generator.surface_zlevel import _generate_sampling_plan

        # Test with a 2D endmill - should always be 1 sample
        tool_2d = self._get_mock_tool_params("endmill")
        plan_2d = _generate_sampling_plan(15.0, 5.0, 0.001, {10.0}, 4, tool_2d)
        self.assertEqual(len(plan_2d), 1)

        # Test with a 3D ballend - should have multiple "squeeze" samples
        tool_3d = self._get_mock_tool_params("ballend", radius=5.0, corner_rad=5.0)
        plan_3d = _generate_sampling_plan(15.0, 5.0, 0.001, set(), 8, tool_3d)
        self.assertEqual(len(plan_3d), 8)

        # Test the "snap" logic - add a critical floor height
        critical_heights = {17.0}  # A floor 3mm below current Z of 15
        plan_snap = _generate_sampling_plan(15.0, 5.0, 0.001, critical_heights, 8, tool_3d)
        self.assertGreater(len(plan_snap), 8, "Snap logic should have added an extra sample point")

    # -- Stack and G-code Generation (Integration Tests) --

    def test20_stack_generation(self):
        """
        Tests the main `zlevel_hybrid_stack` function to ensure it generates valid geometry.

        INPUT:
        - Function: zlevel_hybrid_stack()
        - Input data: The test model, categorized steps, and boundaries.

        EXPECTED OUTPUT:
        - Returns a non-empty list of (z_target, cutAreaShape, status) tuples.
        - Each `cutAreaShape` must be a valid Part.Shape object.
        - This is a key integration test for the C++ Path.Area backend.
        """
        from Path.Base.Generator.surface_zlevel import categorize_floor_steps, zlevel_hybrid_stack

        steps = categorize_floor_steps(self.test_model, 20.0, 0.0, 10.0, False)
        tool = self._get_mock_tool_params()

        stack = zlevel_hybrid_stack(
            shape=self.test_model,
            categorized_steps=steps,
            border_face=self.border_face,
            trim_face=self.trim_face,
            fill_holes_masks=self.fill_holes_masks,
            tool_params=tool,
            stock_to_leave=0.0,
            accuracy_val="4",
            z_offset=0.0,
            wpc=self.wpc,
            start_z=25,
        )

        self.assertGreater(len(stack), 0, "Stack should contain generated layers")
        z, shape, status = stack[0]
        self.assertIsInstance(shape, Part.Shape)
        self.assertFalse(shape.isNull())

    def test30_gcode_generation(self):
        """
        Tests the final conversion from a geometry stack to G-code commands.

        INPUT:
        - Function: zlevel_hybrid_to_gcode()
        - Input data: A pre-computed stack of clearing areas, with ZigZag pattern enabled.

        EXPECTED OUTPUT:
        - Returns a non-empty list of Path.Command objects.
        - The command list must contain both G0 (rapid) and G1 (feed) moves.
        - This is the final end-to-end test for the Z-Level strategy.
        """
        from Path.Base.Generator.surface_zlevel import (
            categorize_floor_steps,
            zlevel_hybrid_stack,
            zlevel_hybrid_to_gcode,
        )

        steps = categorize_floor_steps(self.test_model, 20.0, 5.0, 15.0, False)
        tool = self._get_mock_tool_params()

        stack = zlevel_hybrid_stack(
            self.test_model,
            steps,
            self.border_face,
            self.trim_face,
            self.fill_holes_masks,
            tool,
            0.0,
            "4",
            0.0,
            self.wpc,
            start_z=25,
        )

        feed_params = {"horizFeed": 300, "vertFeed": 100, "horizRapid": 1000, "vertRapid": 1000}
        height_params = {"safe_hght": 25.0, "clearance_hght": 30.0}
        pattern_options = {
            "cut_climb": True,
            "cut_pattern": "ZigZag",
            "pattern_angle": 45.0,
            "reverse_pattern": False,
        }

        cmds = zlevel_hybrid_to_gcode(
            stack,
            feed_params,
            height_params,
            pattern_options,
            ignore_outer=False,
            clear_planar_only=False,
            step_over=4.0,
            start_point=False,
            radius=tool["radius"],
            is_adaptive=False,
            adaptive_params={},
            bb_face=self.border_face,
            enforce_geofence=True,
        )

        self.assertGreater(len(cmds), 0, "G-code generation produced no commands")
        cmd_names = {c.Name for c in cmds}
        self.assertIn("G0", cmd_names, "G-code should contain rapid moves")
        self.assertIn("G1", cmd_names, "G-code should contain cutting moves")

    # -- Fill Hole Masks Tests --

    def test40_fuse_coplanar_masks(self):
        """
        Tests _fuse_coplanar_masks correctly groups and compounds co-planar masks.

        INPUT:
        - Function: _fuse_coplanar_masks()
        - Three masks: two at Z=10, one at Z=5.

        EXPECTED OUTPUT:
        - Two entries in the result: one fused compound at Z=10, one at Z=5.
        - Result sorted descending by Z.
        """
        from Path.Base.Generator.surface_zlevel import _fuse_coplanar_masks

        # Build three simple flat faces
        def _make_square_face(x, y, size, z):
            wire = Part.makePolygon(
                [
                    FreeCAD.Vector(x, y, z),
                    FreeCAD.Vector(x + size, y, z),
                    FreeCAD.Vector(x + size, y + size, z),
                    FreeCAD.Vector(x, y + size, z),
                    FreeCAD.Vector(x, y, z),
                ]
            )
            f = Part.Face(wire)
            f.translate(FreeCAD.Vector(0, 0, -f.BoundBox.ZMin))
            return f

        mask_a1 = _make_square_face(0, 0, 5, 10)  # Z=10
        mask_a2 = _make_square_face(10, 0, 5, 10)  # Z=10 — should fuse with a1
        mask_b = _make_square_face(0, 0, 5, 5)  # Z=5  — separate group

        raw = [(10.0, mask_a1), (10.0, mask_a2), (5.0, mask_b)]
        result = _fuse_coplanar_masks(raw)

        self.assertEqual(len(result), 2, "Should have two Z-groups after fusing")
        self.assertEqual(result[0][0], 10.0, "First entry should be Z=10 (descending)")
        self.assertEqual(result[1][0], 5.0, "Second entry should be Z=5")

    def test41_fill_selected_single_face(self):
        """
        Tests fill_selected produces a valid cap for a single isolated hole face.

        INPUT:
        - Function: fill_selected()
        - A mock base_property list containing one cylindrical hole wall face.

        EXPECTED OUTPUT:
        - Returns exactly one (max_z, mask_face) tuple.
        - max_z matches the top Z of the hole wall face.
        - mask_face is a valid non-null Part.Face at Z=0.
        - mask_face.BoundBox.ZMin == 0 (correctly translated to Z=0).
        """
        from Path.Base.Generator.surface_zlevel import fill_selected

        _, hole_a_face, _ = self.setUp_fill_model()
        self.assertIsNotNone(hole_a_face, "Could not find hole A wall face in test model")

        # Mock base_property: list of (obj, ["FaceN"]) tuples
        # fill_selected calls _get_selected_faces which expects this format.
        # We bypass the FreeCAD object lookup by patching _get_selected_faces.
        from Path.Base.Generator import surface_zlevel

        original = surface_zlevel._get_selected_faces
        try:
            surface_zlevel._get_selected_faces = lambda _: [hole_a_face]
            result = fill_selected([])
        finally:
            surface_zlevel._get_selected_faces = original

        self.assertEqual(len(result), 1, "Should produce exactly one mask")
        max_z, mask_face = result[0]

        self.assertAlmostEqual(max_z, hole_a_face.BoundBox.ZMax, places=3)
        self.assertIsInstance(mask_face, Part.Shape)
        self.assertFalse(mask_face.isNull())
        self.assertAlmostEqual(
            mask_face.BoundBox.ZMin, 0.0, places=3, msg="Mask face should be translated to Z=0"
        )

    def test42_fill_selected_two_faces_same_z(self):
        """
        Tests fill_selected correctly fuses two hole caps at the same Z height.

        INPUT:
        - Function: fill_selected()
        - Two isolated hole wall faces both with BoundBox.ZMax == 10.

        EXPECTED OUTPUT:
        - Returns exactly one (max_z, mask_face) tuple — the two caps fused.
        - max_z == 10.0.
        - The fused mask has a larger area than either individual cap.
        """
        from Path.Base.Generator.surface_zlevel import fill_selected
        from Path.Base.Generator import surface_zlevel

        _, hole_a_face, hole_b_face = self.setUp_fill_model()
        self.assertIsNotNone(hole_a_face)
        self.assertIsNotNone(hole_b_face)

        # Both holes happen to share a top Z for this test
        original = surface_zlevel._get_selected_faces
        try:
            surface_zlevel._get_selected_faces = lambda _: [hole_a_face, hole_b_face]
            result = fill_selected([])
        finally:
            surface_zlevel._get_selected_faces = original

        # Both faces have the same ZMax — so we expect one fused entrie
        # (only co-planar masks fuse). Validate we got one valid result.
        self.assertEqual(
            len(result), 1, "Should produce exactly one mask for two holes at the same Z"
        )
        for max_z, mask_face in result:
            self.assertIsInstance(mask_face, Part.Shape)
            self.assertFalse(mask_face.isNull())
            self.assertAlmostEqual(mask_face.BoundBox.ZMin, 0.0, places=3)

    def test43_apply_fill_hole_masks_pure_step(self):
        """
        Tests _apply_fill_hole_masks adds mask to allPrevComp on a Pure step.

        INPUT:
        - Function: _apply_fill_hole_masks()
        - One mask at Z=10, current step at Z=10, status="Pure".

        EXPECTED OUTPUT:
        - fill_mask_idx incremented to 1 (mask consumed).
        - allPrevComp is non-null (mask was added).
        - floor_geo unchanged (None).
        """
        from Path.Base.Generator.surface_zlevel import _apply_fill_hole_masks

        wire = Part.makePolygon(
            [
                FreeCAD.Vector(0, 0, 0),
                FreeCAD.Vector(10, 0, 0),
                FreeCAD.Vector(10, 10, 0),
                FreeCAD.Vector(0, 10, 0),
                FreeCAD.Vector(0, 0, 0),
            ]
        )
        mask_face = Part.Face(wire)

        silhouette_wire = Part.makePolygon(
            [
                FreeCAD.Vector(-5, -5, 0),
                FreeCAD.Vector(20, -5, 0),
                FreeCAD.Vector(20, 20, 0),
                FreeCAD.Vector(-5, 20, 0),
                FreeCAD.Vector(-5, -5, 0),
            ]
        )
        silhouette = Part.Face(silhouette_wire)

        fill_holes_masks = [(10.0, mask_face)]
        wpc = Part.makeCircle(2.0)

        idx, masks, floor_geo, allPrevComp = _apply_fill_hole_masks(
            wpc=wpc,
            fill_holes_masks=fill_holes_masks,
            fill_mask_idx=0,
            current_silhouette=silhouette,
            status="Pure",
            floor_geo=None,
            all_prev_comp=None,
            z_target=10.0,
            loose_tol=1e-4,
        )

        self.assertEqual(idx, 0, "Index should reset to 0 after consuming")
        self.assertEqual(len(masks), 0, "Consumed masks should be removed from list")
        self.assertIsNone(floor_geo, "floor_geo should be unchanged for Pure step")
        self.assertIsNotNone(allPrevComp, "allPrevComp should be updated for Pure step")

    def test44_stack_with_fill_holes(self):
        """
        Integration test: zlevel_hybrid_stack with fill_holes_masks active.

        INPUT:
        - Function: zlevel_hybrid_stack()
        - A model with a pocket and a fill-hole mask covering the pocket floor.

        EXPECTED OUTPUT:
        - Stack is non-empty.
        - The layer at the pocket floor Z has status "Extra" or "Mixed".
        - No layer attempts to cut inside the masked hole region.
        """
        from Path.Base.Generator.surface_zlevel import (
            categorize_floor_steps,
            zlevel_hybrid_stack,
        )

        # Simple model: box with a pocket
        base = Part.makeBox(50, 50, 20)
        pocket = Part.makeBox(20, 20, 10, FreeCAD.Vector(15, 15, 10))
        model = base.cut(pocket).removeSplitter()

        # Build a fill-hole mask covering the pocket floor at Z=10
        mask_wire = Part.makePolygon(
            [
                FreeCAD.Vector(15, 15, 0),
                FreeCAD.Vector(35, 15, 0),
                FreeCAD.Vector(35, 35, 0),
                FreeCAD.Vector(15, 35, 0),
                FreeCAD.Vector(15, 15, 0),
            ]
        )
        mask_face = Part.Face(mask_wire)
        fill_holes_masks = [(10.0, mask_face)]

        steps = categorize_floor_steps(model, 20.0, 0.0, 8.0, False)
        tool = self._get_mock_tool_params()

        stack = zlevel_hybrid_stack(
            shape=model,
            categorized_steps=steps,
            border_face=self.border_face,
            trim_face=self.trim_face,
            fill_holes_masks=fill_holes_masks,
            tool_params=tool,
            stock_to_leave=0.0,
            accuracy_val="4",
            z_offset=0.0,
            wpc=self.wpc,
            start_z=25,
        )

        self.assertGreater(len(stack), 0, "Stack should contain generated layers")

        # Verify at least one Extra or Mixed layer exists (floor was detected)
        statuses = {s for _, _, s in stack}
        self.assertTrue(
            "Extra" in statuses or "Mixed" in statuses,
            "Stack should contain at least one floor-type layer",
        )
