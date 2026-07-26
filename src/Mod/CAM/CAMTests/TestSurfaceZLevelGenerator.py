# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 Dimitris75 <dimitriospana75@gmail.com>             *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************


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
            self.test_model,
            start_z=20.0,
            final_z=0.0,
            step_down=7.5,
            clear_planar_only=False,
        )

        # Expected Z-levels: 20 -> 12.5 (Pure) -> 10 (Extra) -> 5 (Mixed, as 12.5-7.5=5 lands near 5) -> 0
        self.assertGreaterEqual(len(steps), 4)

        # Create a dictionary for easy lookup
        categorized_z = {round(s[0], 2): s[1] for s in steps}

        self.assertEqual(categorized_z.get(12.5), "Pure")
        self.assertEqual(categorized_z.get(10.0), "Extra")
        # Note: 12.5 - 7.5 = 5.0 lands exactly on the physical floor, so 5 becomes Mixed.
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
        critical_heights = {17.0}  # A floor 2mm above current z_target of 15
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
            categorizedSteps=steps,
            borderFace=self.border_face,
            trimFace=self.trim_face,
            tool_params=tool,
            stock_to_leave=0.0,
            accuracy_val="4",
            z_offset=0.0,
            wpc=self.wpc,
            start_z=20.0,
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
            tool,
            0.0,
            "4",
            0.0,
            self.wpc,
            start_z=20.0,
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
            radius=tool["radius"],
        )

        self.assertGreater(len(cmds), 0, "G-code generation produced no commands")
        cmd_names = {c.Name for c in cmds}
        self.assertIn("G0", cmd_names, "G-code should contain rapid moves")
        self.assertIn("G1", cmd_names, "G-code should contain cutting moves")
