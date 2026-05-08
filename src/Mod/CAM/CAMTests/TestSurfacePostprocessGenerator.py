# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2025 sliptonic <shopinthewoods@gmail.com>               *
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
            horiz_feed=300,
            vert_rapid=1000,
            horiz_rapid=1000,
            safe_z=safe_z,
            step_down=5.0,
            sample_interval=1.0,
            clearance_z=clearance_z,
            start_z=15.0,
            final_z=10.0,
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
            horiz_feed=300,
            vert_rapid=1000,
            horiz_rapid=1000,
            safe_z=safe_z,
            step_down=5.0,
            sample_interval=1.0,
            clearance_z=clearance_z,
            start_z=15.0,
            final_z=10.0,
            optimize_transitions=True,
            safe_stl=self.flat_stl,
            cutter=self.cutter,
        )

        # Find the last command of line1 (G1 to its final point)
        end_of_line1_idx = -1
        for i, cmd in enumerate(cmds):
            if cmd.Name == "G1" and cmd.Parameters.get("X") == 90 and cmd.Parameters.get("Y") == 10:
                end_of_line1_idx = i

        # Find the first command of line2 (G1 to its first point)
        start_of_line2_idx = -1
        for i, cmd in enumerate(cmds):
            if cmd.Name == "G1" and cmd.Parameters.get("X") == 90 and cmd.Parameters.get("Y") == 12:
                start_of_line2_idx = i
                break

        self.assertNotEqual(end_of_line1_idx, -1, "End of line1 not found in G-code")
        self.assertNotEqual(start_of_line2_idx, -1, "Start of line2 not found in G-code")

        # Check for the ABSENCE of a retract to safe_z during the transition
        has_retract = False
        for cmd in cmds[end_of_line1_idx + 1 : start_of_line2_idx]:
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
            horiz_feed=300,
            vert_rapid=1000,
            horiz_rapid=1000,
            safe_z=safe_z,
            step_down=5.0,
            sample_interval=1.0,
            clearance_z=clearance_z,
            start_z=15.0,
            final_z=10.0,
            optimize_transitions=True,
            safe_stl=self.flat_stl,
            cutter=self.cutter,
        )

        # Find the last command of line1 (G1 to its final point)
        end_of_line1_idx = -1
        for i, cmd in enumerate(cmds):
            if cmd.Name == "G1" and cmd.Parameters.get("X") == 90 and cmd.Parameters.get("Y") == 10:
                end_of_line1_idx = i

        # Find the first command of line2 (G1 to its first point)
        start_of_line2_idx = -1
        for i, cmd in enumerate(cmds):
            if cmd.Name == "G1" and cmd.Parameters.get("X") == 10 and cmd.Parameters.get("Y") == 80:
                start_of_line2_idx = i
                break

        self.assertNotEqual(end_of_line1_idx, -1, "End of line1 not found in G-code")
        self.assertNotEqual(start_of_line2_idx, -1, "Start of line2 not found in G-code")

        # Check for the PRESENCE of a retract, because the distance is too great
        has_retract = False
        for cmd in cmds[end_of_line1_idx + 1 : start_of_line2_idx]:
            if cmd.Name == "G0" and cmd.Parameters.get("Z") == safe_z:
                has_retract = True
                break

        self.assertTrue(has_retract, "Optimized long transition should fall back to a safe retract")
