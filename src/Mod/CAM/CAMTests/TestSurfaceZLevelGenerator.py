# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *   Copyright (c) 2026 Dimitris75 <dimitriospana75@gmail.com>               *
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

import FreeCAD
import Part
import Path
import math
import CAMTests.PathTestUtils as PathTestUtils
from Path.Base.Generator import surface_zlevel


class TestSurfaceZLevelGenerator(PathTestUtils.PathTestBase):
    """Tests for surface_zlevel: categorization, stack generation, and G-code conversion."""

    def _setup_test_model(self):
        """Creates a test part: a box with a central island and a small hole."""
        # Main body (20mm thick)
        base = Part.makeBox(50, 50, 20)
        # Pocket floor at Z=10
        pocket = Part.makeBox(40, 40, 10, FreeCAD.Vector(5, 5, 10))
        # Central island inside the pocket (rising from Z=10 to Z=20)
        island = Part.makeCylinder(5, 10, FreeCAD.Vector(25, 25, 10))
        # Small hole (to test IgnoreHoles logic in the future)
        hole = Part.makeCylinder(2, 20, FreeCAD.Vector(10, 10, 0))

        shape = base.cut(pocket).fuse(island).cut(hole)
        return shape.removeSplitter()

    def _get_mock_tool_params(self, profile="Ballend"):
        """Returns standardized tool parameters for testing."""
        return {
            "radius": 3.0,
            "c_rad": 3.0 if profile == "Ballend" else 0.0,
            "profile": profile,
            "is_threeD": profile in ["Ballend", "Bullnose"],
        }

    def _get_test_boundaries(self):
        """Returns standard border and trim faces used across multiple tests."""
        # Standard Border (50x50)
        border = Part.makeFace(
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
        # Standard Trim (75x75)
        trim = Part.makeFace(
            Part.makePolygon(
                [
                    FreeCAD.Vector(-12, -12, 0),
                    FreeCAD.Vector(62, -12, 0),
                    FreeCAD.Vector(62, 62, 0),
                    FreeCAD.Vector(-12, 62, 0),
                    FreeCAD.Vector(-12, -12, 0),
                ]
            )
        )
        return border, trim

    def test00_categorization(self):
        """Verifies that model floors are correctly identified and categorized."""
        shape = self._setup_test_model()

        # Test range from Z=20 (top) to Z=0 (bottom)
        steps = surface_zlevel.categorize_floor_steps(shape, 20.0, 0.0, 5.0)

        # Expected sequence: 20.0, 15.0, 10.0, 5.0, 0.0
        self.assertTrue(len(steps) >= 5)

        # Verify the pocket floor at Z=10 was identified as Mixed
        step_10 = [s for s in steps if abs(s[0] - 10.0) < 0.001]
        self.assertTrue(step_10, "Z=10.0 step not found")
        self.assertEqual(
            step_10[0][1], "Mixed", "Pocket floor at Z=10 should be categorized as Mixed"
        )

    def test10_stack_generation(self):
        """Verifies geometric stack generation via C++ Path.Area engine."""
        shape = self._setup_test_model()
        params = self._get_mock_tool_params("Ballend")
        border, trim = self._get_test_boundaries()
        wpc = Part.makeCircle(2.0, FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(0, 0, 1))

        # Generator context
        steps = surface_zlevel.categorize_floor_steps(shape, 20.0, 0.0, 5.0)

        stack = surface_zlevel.zlevel_hybrid_stack(
            shape=shape,
            categorizedSteps=steps,
            borderFace=border,
            trimFace=trim,
            tool_params=params,
            stock_to_leave=0.0,
            accuracy_val=4,
            z_offset=0.0,
            wpc=wpc,
        )

        self.assertTrue(len(stack) > 0, "Stack should contain multiple layers")
        # Validate tuple structure: (z_target, cutArea, status)
        self.assertEqual(len(stack[0]), 3, "Stack tuple should contain 3 elements")
        self.assertIsInstance(stack[0][1], Part.Shape, "Layer output must be a Part.Shape")

    def test20_gcode_conversion(self):
        """Verifies conversion of geometry stack to Path commands with clearing patterns."""
        shape = self._setup_test_model()
        params = self._get_mock_tool_params("Endmill")
        border, _ = self._get_test_boundaries()
        wpc = Part.makeCircle(2.0)

        # Mock standard operation parameters
        pattern_opts = {
            "cut_climb": True,
            "cut_pattern": "ZigZag",
            "pattern_angle": 45.0,
            "reverse_pattern": False,
        }

        height_opts = {"safe_hght": 10.0, "clearance_hght": 25.0}

        feed_opts = {
            "horizFeed": 1000.0,
            "vertFeed": 300.0,
            "horizRapid": 2500.0,
            "vertRapid": 1500.0,
        }

        # Create a single-layer stack for testing
        steps = surface_zlevel.categorize_floor_steps(shape, 10.0, 10.0, 1.0)
        stack = surface_zlevel.zlevel_hybrid_stack(
            shape, steps, border, None, params, 0.0, 4, 0.0, wpc
        )

        cmds = surface_zlevel.zlevel_hybrid_to_gcode(
            stack=stack,
            feed_params=feed_opts,
            height_params=height_opts,
            pattern_options=pattern_opts,
            ignore_outer=False,
            clear_planar_only=True,
            step_over=50.0,
            radius=params["radius"],
        )

        self.assertTrue(len(cmds) > 0, "No G-code commands generated")

        # Verify basic G-code types
        command_names = [c.Name for c in cmds]
        self.assertIn("G0", command_names, "Should contain rapid moves")
        self.assertIn("G1", command_names, "Should contain cutting moves")

        # Specific check for vertical safety: No move should be below Z=10.0
        for c in cmds:
            if "Z" in c.Parameters:
                self.assertTrue(c.Parameters["Z"] >= 10.0, "Tool dipped below target depth")
