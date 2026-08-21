# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2025 Dimitrios Pana <dimitriospana75@gmail.com>
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
import unittest
import CAMTests.PathTestUtils as PathTestUtils

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())

# Check if OCL is available
_ocl_available = False
try:
    try:
        import ocl

        _ocl_available = True
    except ImportError:
        import opencamlib as ocl

        _ocl_available = True
except ImportError:
    pass


@unittest.skipUnless(_ocl_available, "OpenCamLib not available")
class TestSurfacePostprocess(PathTestUtils.PathTestBase):
    """Tests for surface_postprocess: multi-pass, filtering, and G-code generation."""

    def setUp(self):
        """Create common test geometry and data for post-processing tests."""
        # A flat box is perfect for testing transitions on a known surface
        from Path.Base.Generator.surface_mesh import _shape_to_stl
        from Path.Base.Generator.surface_common import make_ocl_cutter

        box = Part.makeBox(100, 100, 10)
        self.flat_stl = _shape_to_stl(box, 0.1, 0.5)
        self.cutter = make_ocl_cutter("endmill", 6.0, edge_height=20.0)

        # Common scan_lines_to_gcode() parameters shared by the G-code
        # transition tests below -- each test passes this as a base and
        # overrides/extends only what it actually needs to vary.
        self.options = {
            "depth_offset": 0.0,
            "optimize_transitions": False,
            "optimize_ratio": 2,
            "safe_stl": None,
            "cutter": self.cutter,
            "force_keep_down": False,
            "use_smart_leads": False,
            "lead_feed_percent": 75,
            "lift_lead_z": 1.00,
            "volumetric_percent": 25,
            "is_multipass": False,
        }

    # -- Multi-pass Tests --

    def test00_apply_multipass(self):
        """
        Tests that a full-depth toolpath is correctly sliced into multiple Z-layers.

        INPUT:
        - Function: apply_multipass()
        - Parameters: A single scan line going from Z=10 down to Z=2, with a 3mm step down.
        - Input data: A path that machines an 8mm deep slope.

        EXPECTED OUTPUT:
        - Returns multiple scan lines, corresponding to passes at Z=7, Z=4, and Z=2.
        - The points in each new scan line should have their Z-values clamped to the
          pass-specific depth, preventing gouging.
        """
        from Path.Base.Generator.surface_postprocess import apply_multipass

        # A single line that slopes from Z=10 down to Z=2
        full_depth_line = [[(0, 0, 10), (10, 0, 6), (20, 0, 2)]]
        start_depth, final_depth, step_down = 10.0, 2.0, 3.0

        multi_pass_lines = apply_multipass(full_depth_line, start_depth, final_depth, step_down)

        # Expected depths: 10->7, 7->4, 4->2. This should generate 3 sets of paths.
        self.assertGreaterEqual(
            len(multi_pass_lines), 3, "Expected at least 3 layers for the given depths"
        )

        # Verify that each pass respects its minimum Z depth
        pass_depths = [7.0, 4.0, 2.0]
        # Note: The actual number of generated segments can be more than the number of passes
        # if the path moves in and out of the cutting zone. We check the first 3.
        for i, depth in enumerate(pass_depths):
            if i < len(multi_pass_lines):
                pass_line = multi_pass_lines[i]
                z_values = [p[2] for p in pass_line]
                # Check that no point in this pass gouges below the target depth
                self.assertTrue(
                    all(z >= depth - 0.01 for z in z_values),
                    f"Pass for Z={depth} contains points below its target depth.",
                )

        # Verify the final pass reaches the final depth
        z_values_final_pass = [p[2] for p in multi_pass_lines[-1]]
        self.assertTrue(
            any(abs(z - final_depth) < 0.01 for z in z_values_final_pass),
            "The final pass did not reach the target final_depth.",
        )

    # -- Path Filtering Tests --

    def test10_filter_collinear_points(self):
        """
        Tests that redundant, co-linear points are removed from a path.

        INPUT:
        - Function: filter_cl_points()
        - Input data: A list of 5 points lying on a perfectly straight line.

        EXPECTED OUTPUT:
        - Returns a list containing only 2 points: the start and end of the line.
        - This optimization reduces G-code file size and can improve machine performance.
        """
        from Path.Base.Generator.surface_postprocess import filter_cl_points

        points = [(0, 0, 0), (10, 0, 0), (20, 0, 0), (30, 0, 0), (40, 0, 0)]
        filtered = filter_cl_points(points, tolerance=0.001)

        self.assertEqual(len(filtered), 2, "Filter should remove all intermediate co-linear points")
        self.assertEqual(filtered[0], (0, 0, 0))
        self.assertEqual(filtered[1], (40, 0, 0))

    def test11_filter_preserves_corners(self):
        """
        Tests that the filter does NOT remove essential corner points.

        INPUT:
        - Function: filter_cl_points()
        - Input data: A list of points forming a zig-zag path (no three points are co-linear).

        EXPECTED OUTPUT:
        - Returns a list with the same number of points as the input.
        - The filter should be smart enough to preserve the intended shape of the path.
        """
        from Path.Base.Generator.surface_postprocess import filter_cl_points

        points = [(0, 0, 0), (10, 10, 0), (20, 0, 0), (30, 10, 0)]
        filtered = filter_cl_points(points, tolerance=0.001)

        self.assertEqual(
            len(filtered), len(points), "Filter should not remove any points from a zig-zag path"
        )

    # -- G-code Generation and Transition Tests --

    def test20_gcode_standard_transition(self):
        """
        Verifies that a standard G-code path includes a retract to safe height between segments.

        INPUT:
        - Function: scan_lines_to_gcode()
        - Parameters: Two scan lines, `optimize_transitions=False`.
        - Input data: Standard toolpath data.

        EXPECTED OUTPUT:
        - The generated commands should include a G0 move to the specified safe_z
          height between the G1 moves of the first line and the G1 moves of the second.
        """
        from Path.Base.Generator.surface_postprocess import scan_lines_to_gcode

        line1 = [(10, 10, 10), (20, 10, 10)]
        line2 = [(10, 20, 10), (20, 20, 10)]
        safe_z, clearance_z = 25.0, 30.0

        cmds = scan_lines_to_gcode(
            [line1, line2],
            sample_interval=1.0,
            horiz_feed=300,
            vert_feed=150,
            vert_rapid=1000,
            horiz_rapid=1000,
            safe_z=safe_z,
            clearance_z=clearance_z,
            start_z=15.0,
            final_z=10.0,
            step_down=5.0,
            options=self.options,
        )

        # Find the command index for the end of line1
        end_of_line1_idx = -1
        for i, cmd in enumerate(cmds):
            if cmd.Name == "G1" and cmd.Parameters.get("X") == 20 and cmd.Parameters.get("Y") == 10:
                end_of_line1_idx = i
                break

        self.assertNotEqual(end_of_line1_idx, -1, "End of first line not found in G-code")

        # The next command should be the G0 retract to safe height
        retract_cmd = cmds[end_of_line1_idx + 1]
        self.assertEqual(retract_cmd.Name, "G0")
        self.assertAlmostEqual(retract_cmd.Parameters.get("Z"), safe_z)

    def test21_gcode_optimized_short_transition(self):
        """
        Tests the 'Keep Tool Down' feature for a short transition between scan lines.

        INPUT:
        - Function: scan_lines_to_gcode()
        - Parameters: Two nearby lines, `optimize_transitions=True`, a safe_stl and cutter.
        - Input data: The distance between lines is less than 2x the cutter diameter.

        EXPECTED OUTPUT:
        - The G-code should NOT contain a G0 retract to safe_z between the lines.
        - Instead, the transition should be composed of surface-following G1 moves.
        """
        from Path.Base.Generator.surface_postprocess import scan_lines_to_gcode

        line1 = [(10, 10, 10), (90, 10, 10)]  # End point: (90, 10, 10)
        line2 = [(90, 12, 10), (10, 12, 10)]  # Start point: (90, 12, 10) -> a 2mm transition
        safe_z, clearance_z = 25.0, 30.0

        cmds = scan_lines_to_gcode(
            [line1, line2],
            sample_interval=1.0,
            horiz_feed=300,
            vert_feed=150,
            vert_rapid=1000,
            horiz_rapid=1000,
            safe_z=safe_z,
            clearance_z=clearance_z,
            start_z=15.0,
            final_z=10.0,
            step_down=5.0,
            options={
                **self.options,
                "optimize_transitions": True,
                "safe_stl": self.flat_stl,
            },
        )

        # Check for the ABSENCE of a retract to safe_z during the transition
        has_retract = False
        for cmd in cmds[len(line1) : -len(line2)]:  # Check commands between the two lines
            if cmd.Name == "G0" and cmd.Parameters.get("Z") == safe_z:
                has_retract = True
                break

        self.assertFalse(
            has_retract, "Optimized short transition should not retract to safe height"
        )

    def test22_gcode_optimized_long_transition_fallback(self):
        """
        Tests that 'Keep Tool Down' falls back to a safe retract for long transitions.

        INPUT:
        - Function: scan_lines_to_gcode()
        - Parameters: Two distant lines, `optimize_transitions=True`.
        - Input data: Distance between lines is much greater than 2x cutter diameter.

        EXPECTED OUTPUT:
        - The G-code should fall back to the standard behavior and perform a
          G0 retract to safe_z, as it's safer and faster for long moves.
        """
        from Path.Base.Generator.surface_postprocess import scan_lines_to_gcode

        line1 = [(10, 10, 10), (90, 10, 10)]
        line2 = [(10, 80, 10), (90, 80, 10)]  # 70mm transition
        safe_z, clearance_z = 25.0, 30.0

        cmds = scan_lines_to_gcode(
            [line1, line2],
            sample_interval=1.0,
            horiz_feed=300,
            vert_feed=150,
            vert_rapid=1000,
            horiz_rapid=1000,
            safe_z=safe_z,
            clearance_z=clearance_z,
            start_z=15.0,
            final_z=10.0,
            step_down=5.0,
            options=self.options,
        )

        # Check for the PRESENCE of a retract, because the distance is too great
        has_retract = False
        for cmd in cmds[len(line1) : -len(line2)]:  # Check commands between the two lines
            if cmd.Name == "G0" and cmd.Parameters.get("Z") == safe_z:
                has_retract = True
                break

        self.assertTrue(has_retract, "Optimized long transition should fall back to a safe retract")

    # -- Smart Lead-In / Lead-Out Tests --

    def test30_probe_surface_z_valid_point(self):
        """
        Tests _probe_surface_z returns a valid Z height for a point on the mesh.

        INPUT:
        - Function: _probe_surface_z()
        - A flat box STL (top face at Z=10), probe at center (50, 50).
        - reference_z = 20.0 (above the surface).

        EXPECTED OUTPUT:
        - Returns a float close to 10.0 (the top face Z).
        - Does not return None (OCL found a hit).
        """
        from Path.Base.Generator.surface_postprocess import _probe_surface_z, _make_safe_pdc

        safe_pdc = _make_safe_pdc(self.flat_stl, self.cutter, 0.0, 0.5)
        result = _probe_surface_z((50.0, 50.0), 20.0, safe_pdc)

        self.assertIsNotNone(result, "_probe_surface_z should return a value for an on-mesh point")
        self.assertAlmostEqual(
            result, 10.0, delta=0.2, msg="Probed Z should be close to the box top face at Z=10"
        )

    def test31_generate_lead_arc_geometry(self):
        """
        Tests _generate_lead_arc produces valid G2/G3 arc commands at constant Z.

        INPUT:
        - Function: _generate_lead_arc()
        - A scan line starting at the edge of the flat box (one clear side).
        - is_lead_in=True.

        EXPECTED OUTPUT:
        - Returns a non-empty list of Path.Commands containing G2 or G3.
        - The arc endpoint (entry_point) is at the same Z as line[0] (constant Z).
        - I and J offsets are non-zero (arc has valid geometry).
        """
        from Path.Base.Generator.surface_postprocess import (
            _generate_lead_arc,
            _make_safe_pdc,
        )

        safe_pdc = _make_safe_pdc(self.flat_stl, self.cutter, 0.0, 0.5)

        # Line starts near the edge — one perpendicular side is off the model
        line = [
            (3.0, 50.0, 10.0),
            (50.0, 50.0, 10.0),
            (97.0, 50.0, 10.0),
        ]

        cmds, entry_point = _generate_lead_arc(
            line, safe_pdc, self.cutter, lead_feed=300.0, lift_lead_z=0.0, is_lead_in=True
        )

        if not cmds:
            self.skipTest("No clear arc side found for this geometry — acceptable for flat box")

        self.assertGreater(len(cmds), 0)
        self.assertIn(cmds[0].Name, ("G2", "G3"), "Lead-in arc should be G2 or G3")
        self.assertIsNotNone(entry_point)
        self.assertAlmostEqual(
            entry_point[2],
            line[0][2],
            places=3,
            msg="Arc entry point must be at constant Z (same as cut Z)",
        )

        # Verify I and J are present and non-trivial
        params = cmds[0].Parameters
        self.assertIn("I", params)
        self.assertIn("J", params)
        i_offset = params["I"]
        j_offset = params["J"]
        arc_radius = (i_offset**2 + j_offset**2) ** 0.5
        expected_radius = self.cutter.getDiameter() / 2.0
        self.assertAlmostEqual(
            arc_radius, expected_radius, delta=0.1, msg="Arc radius should match cutter radius"
        )

    def test32_attempt_lead_arc_fallback_strategies(self):
        """
        Tests _attempt_lead_arc tries multiple strategies and returns first success.

        INPUT:
        - Function: _attempt_lead_arc()
        - A scan line near the model edge — forward arc should succeed (strategy 1).

        EXPECTED OUTPUT:
        - Returns non-empty commands and a valid entry point.
        - If all strategies fail, returns ([], None) without raising an exception.
        """
        from Path.Base.Generator.surface_postprocess import (
            _attempt_lead_arc,
            _make_safe_pdc,
        )

        safe_pdc = _make_safe_pdc(self.flat_stl, self.cutter, 0.0, 0.5)

        line = [
            (3.0, 50.0, 10.0),
            (50.0, 50.0, 10.0),
            (97.0, 50.0, 10.0),
        ]

        cmds, entry_point = _attempt_lead_arc(line, safe_pdc, self.cutter, 300.0, 0.0, True)

        # Either a strategy succeeds or all fail gracefully
        if cmds:
            self.assertIn(cmds[0].Name, ("G2", "G3"))
            self.assertIsNotNone(entry_point)
            self.assertAlmostEqual(entry_point[2], line[0][2], places=3)
        else:
            self.assertIsNone(entry_point, "If no commands, entry_point should be None")

    # -- Volumetric Feed Tests --

    def test40_segment_target_feed_height_and_plunge(self):
        """
        Tests _get_segment_target_feed's core volumetric behavior: a speed
        boost near the top of the layer that decays to the base feed at
        the bottom, and a penalty that blends toward vert_feed on a
        genuine vertical plunge.
        """
        from Path.Base.Generator.surface_postprocess import _get_segment_target_feed

        horiz_feed, vert_feed = 300.0, 50.0
        layer_start_z, layer_target_z = 10.0, 0.0
        boost_factor = 1.5

        top_feed = _get_segment_target_feed(
            (0, 0, layer_start_z),
            (10, 0, layer_start_z),
            horiz_feed,
            vert_feed,
            layer_start_z,
            layer_target_z,
            boost_factor,
        )
        self.assertAlmostEqual(top_feed, horiz_feed * boost_factor, places=3)

        bottom_feed = _get_segment_target_feed(
            (0, 0, layer_target_z),
            (10, 0, layer_target_z),
            horiz_feed,
            vert_feed,
            layer_start_z,
            layer_target_z,
            boost_factor,
        )
        self.assertAlmostEqual(bottom_feed, horiz_feed, places=3)

        plunge_feed = _get_segment_target_feed(
            (0, 0, layer_start_z),
            (0, 0, layer_target_z),
            horiz_feed,
            vert_feed,
            layer_start_z,
            layer_target_z,
            boost_factor,
        )
        self.assertAlmostEqual(plunge_feed, vert_feed, places=3)

    def test41_generate_volumetric_cut_commands(self):
        """
        Tests _generate_volumetric_cut_commands end-to-end.
        """
        from Path.Base.Generator.surface_postprocess import _generate_volumetric_cut_commands

        layer_start_z, layer_target_z = 10.0, 0.0
        line = [
            (0, 0, 10.0),
            (10, 0, 10.0),
            (20, 0, 10.0),
            (30, 0, 0.0),
            (40, 0, 0.0),
        ]

        cmds = _generate_volumetric_cut_commands(
            line,
            depth_offset=0.0,
            horiz_feed=300.0,
            vert_feed=50.0,
            layer_start_z=layer_start_z,
            layer_target_z=layer_target_z,
            volumetric_percent=50.0,
        )

        self.assertEqual(len(cmds), len(line))
        self.assertTrue(all(c.Name == "G1" for c in cmds))
        self.assertIn("F", cmds[0].Parameters)
        self.assertGreater(cmds[0].Parameters["F"], 300.0)
        self.assertFalse(all("F" in c.Parameters for c in cmds))
        self.assertIn("F", cmds[-1].Parameters)
        self.assertAlmostEqual(cmds[-1].Parameters["F"], 300.0, delta=1.0)
