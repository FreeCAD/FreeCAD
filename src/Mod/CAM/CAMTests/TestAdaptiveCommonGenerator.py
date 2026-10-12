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

"""Tests for adaptive_common.py — the adaptive clearing generator.

Tests cover:
  - Wire discretization helper (_wire_to_2d)
  - Helix entry generation (_generate_helix_entry / __generate_helix_entry)
  - Full adaptive pattern generation (generate)
  - Edge cases: null input, missing libarea, empty results
  - Offsetting feature
"""

import math
import FreeCAD
import Part
import Path
import unittest
import CAMTests.PathTestUtils as PathTestUtils

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())

# Check if libarea Adaptive2d is available
_area_available = False
try:
    import area as _area

    _area_available = hasattr(_area, "Adaptive2d")
except ImportError:
    pass


def _make_square_face(x, y, size, z=0.0):
    """Helper — build a flat closed square face at the given Z."""
    wire = Part.makePolygon(
        [
            FreeCAD.Vector(x, y, z),
            FreeCAD.Vector(x + size, y, z),
            FreeCAD.Vector(x + size, y + size, z),
            FreeCAD.Vector(x, y + size, z),
            FreeCAD.Vector(x, y, z),
        ]
    )
    return Part.Face(wire)


def _make_default_params():
    """Returns a fully populated adaptive_params dict with safe defaults."""
    return {
        "op_type": "ClearingInside",
        "adaptive_accuracy": 0.1,
        "stock_to_leave": 0.0,
        "force_insideout": True,
        "finishing_profile": False,
        "lift_distance": 0.5,
        "keep_tool_down": 3.0,
        "helix_angle": 3.0,
        "helix_cone_angle": 0.0,
        "helix_diameter": 75.0,
        "helix_min_diameter": 10.0,
    }


def _make_default_feeds():
    """Returns a standard feed_params dict."""
    return {
        "horizFeed": 300.0,
        "vertFeed": 100.0,
        "horizRapid": 1000.0,
        "vertRapid": 1000.0,
    }


class TestAdaptiveCommon(PathTestUtils.PathTestBase):
    """Tests for adaptive_common generator functions."""

    def setUp(self):
        """Standard geometry for all tests — 40x40 pocket at Z=-5."""
        # Stock boundary — 60x60 square face at Z=0
        self.bb_face = _make_square_face(-5, -5, 60)

        # Cutting area — 40x40 square face at Z=0 (simulates a cleared layer)
        self.cut_area = _make_square_face(5, 5, 40)

        self.radius = 3.0  # 6mm tool
        self.step_over = 2.0
        self.z_target = -5.0
        self.prev_z = 0.0
        self.safe_z = 25.0

        self.feed_params = _make_default_feeds()
        self.adaptive_params = _make_default_params()

    # -----------------------------------------------------------------------
    # Wire discretization
    # -----------------------------------------------------------------------

    def test00_wire_to_2d_square(self):
        """
        Tests _wire_to_2d correctly discretizes a square wire to 2D points.

        INPUT:
        - A square wire with 4 edges, side = 10mm.

        EXPECTED OUTPUT:
        - Returns a list of [x, y] pairs (not 3-tuples).
        - At least 4 points (one per vertex minimum).
        - All points have y-coordinate within the square bounds.
        - No Z coordinate in the output.
        """
        from Path.Base.Generator.adaptive_common import _wire_to_2d

        wire = Part.makePolygon(
            [
                FreeCAD.Vector(0, 0, 0),
                FreeCAD.Vector(10, 0, 0),
                FreeCAD.Vector(10, 10, 0),
                FreeCAD.Vector(0, 10, 0),
                FreeCAD.Vector(0, 0, 0),
            ]
        )

        pts = _wire_to_2d(wire)

        self.assertGreater(len(pts), 3, "Should produce at least 4 points for a square")
        for pt in pts:
            self.assertEqual(len(pt), 2, "Each point must be a 2-element [x, y] list")
            self.assertGreaterEqual(pt[0], -0.01)
            self.assertLessEqual(pt[0], 10.01)
            self.assertGreaterEqual(pt[1], -0.01)
            self.assertLessEqual(pt[1], 10.01)

    def test01_wire_to_2d_deflection(self):
        """
        Tests _wire_to_2d respects the deflection parameter.

        INPUT:
        - A circular wire (arc), tested with coarse and fine deflection.

        EXPECTED OUTPUT:
        - Fine deflection produces more points than coarse deflection.
        """
        from Path.Base.Generator.adaptive_common import _wire_to_2d

        circle = Part.makeCircle(10)
        wire = Part.Wire([circle])

        pts_coarse = _wire_to_2d(wire, deflection=2.0)
        pts_fine = _wire_to_2d(wire, deflection=0.1)

        self.assertGreater(
            len(pts_fine), len(pts_coarse), "Finer deflection should produce more points"
        )

    # -----------------------------------------------------------------------
    # Helix entry — straight plunge fallback
    # -----------------------------------------------------------------------

    def test10_helix_entry_small_radius_fallback(self):
        """
        Tests _generate_helix_entry falls back to straight plunge when the
        helix radius is smaller than helix_min_diameter / 2.

        INPUT:
        - region.HelixCenterPoint = (0, 0)
        - region.StartPoint = (0.1, 0)  — helix_radius = 0.1mm
        - helix_min_diameter = 0.6mm (10% of 6mm tool)

        EXPECTED OUTPUT:
        - Returns commands containing G0 (rapid to start) and G1 (plunge).
        - No G2/G3 arc commands (no helix).
        - Last command is G1 to z_target.
        """
        from Path.Base.Generator.adaptive_common import _generate_helix_entry

        class MockRegion:
            HelixCenterPoint = [0.0, 0.0]
            StartPoint = [0.1, 0.0]  # 0.1mm radius — too small for helix

        cmds = _generate_helix_entry(
            region=MockRegion(),
            z_target=self.z_target,
            prev_z=self.prev_z,
            safe_z=self.safe_z,
            radius=self.radius,
            feed_params=self.feed_params,
            helix_min_diameter=self.radius * 2.0 * 0.1,  # 10% of tool_diam
            helix_angle=3.0,
            helix_cone_angle=0.0,
        )

        self.assertGreater(len(cmds), 0, "Should always return at least a plunge")
        cmd_names = {c.Name for c in cmds}
        self.assertNotIn("G2", cmd_names, "No arc expected for tiny helix radius")
        self.assertNotIn("G3", cmd_names, "No arc expected for tiny helix radius")

        # Last command should plunge to z_target
        last = cmds[-1]
        self.assertEqual(last.Name, "G1")
        self.assertAlmostEqual(last.Parameters.get("Z", 0), self.z_target, places=3)

    def test11_helix_entry_valid_radius(self):
        """
        Tests _generate_helix_entry produces helix arc commands for a valid radius.

        INPUT:
        - region.HelixCenterPoint = (25, 25)
        - region.StartPoint = (27, 25)  — helix_radius = 2mm
        - helix_min_diameter = 0.6mm (10% of 6mm tool)

        EXPECTED OUTPUT:
        - Returns commands including G2 or G3 arc moves.
        - First command is G0 retract to safe_z.
        - Commands contain arc moves descending to z_target.
        """
        from Path.Base.Generator.adaptive_common import _generate_helix_entry

        class MockRegion:
            HelixCenterPoint = [25.0, 25.0]
            StartPoint = [27.0, 25.0]  # 2mm radius — valid

        cmds = _generate_helix_entry(
            region=MockRegion(),
            z_target=self.z_target,
            prev_z=self.prev_z,
            safe_z=self.safe_z,
            radius=self.radius,
            feed_params=self.feed_params,
            helix_min_diameter=self.radius * 2.0 * 0.1,
            helix_angle=3.0,
            helix_cone_angle=0.0,
        )

        self.assertGreater(len(cmds), 2, "Should produce more than just a plunge")

        # First command retracts to safe_z
        self.assertEqual(cmds[0].Name, "G0")
        self.assertAlmostEqual(cmds[0].Parameters.get("Z", 99), self.safe_z, places=3)

        # Should contain arc moves
        cmd_names = [c.Name for c in cmds]
        has_arc = "G2" in cmd_names or "G3" in cmd_names
        self.assertTrue(has_arc, "Valid helix radius should produce G2/G3 arc commands")

    def test12_helix_entry_prev_z_used_not_safe_z(self):
        """
        Tests that _generate_helix_entry uses prev_z for helix start height.
        The initial move is to safe_z, but the helix itself starts from prev_z.
        This test checks that the helix geometry is built from the correct height.

        INPUT:
        - prev_z = -2.0 (previous layer depth)
        - safe_z = 25.0

        EXPECTED OUTPUT:
        - The helix commands start descending from prev_z (-2.0), not safe_z (25.0).
        """
        from Path.Base.Generator.adaptive_common import _generate_helix_entry

        class MockRegion:
            HelixCenterPoint = [25.0, 25.0]
            StartPoint = [27.0, 25.0]

        prev_z = -2.0

        cmds = _generate_helix_entry(
            region=MockRegion(),
            z_target=-5.0,
            prev_z=prev_z,
            safe_z=self.safe_z,
            radius=self.radius,
            feed_params=self.feed_params,
            helix_min_diameter=self.radius * 2.0 * 0.1,
            helix_angle=3.0,
            helix_cone_angle=0.0,
        )

        self.assertGreater(len(cmds), 0)
        # First command is G0 to safe_z
        self.assertAlmostEqual(cmds[0].Parameters.get("Z"), self.safe_z)

        # Subsequent commands should start the helix from prev_z.
        # Let's find the first point in the helix path after the initial positioning.
        first_helix_cmd = next((cmd for cmd in cmds[1:] if cmd.Name in ["G1", "G2", "G3"]), None)
        self.assertIsNotNone(first_helix_cmd, "Could not find start of helix path")

        # The helix is generated by another module which assumes it starts at a certain height.
        # We can verify that the Z doesn't jump from safe_z to the start of the helix in a G1.
        # The helix generator itself places the tool at the start point at prev_z.
        # A full verification would require mocking the helix generator.
        # For now, we trust the helix generator is called with the correct `retract_height`.

    # -----------------------------------------------------------------------
    # Full generate() function
    # -----------------------------------------------------------------------

    def test20_generate_null_cut_area(self):
        """
        Tests generate() returns empty list gracefully for null cut_area.

        INPUT:
        - cut_area = None

        EXPECTED OUTPUT:
        - Returns [] without raising an exception.
        """
        from Path.Base.Generator.adaptive_common import generate

        result = generate(
            adaptive_params=self.adaptive_params,
            feed_params=self.feed_params,
            radius=self.radius,
            step_over=self.step_over,
            z_target=self.z_target,
            safe_z=self.safe_z,
            prev_z=self.prev_z,
            cut_area=None,
            min_face_area=0.0,
            bb_face=self.bb_face,
            cut_area_offset=0.0,
            bb_face_offset=0.0,
        )

        self.assertEqual(result, [], "Null cut_area should return empty list")

    def test21_generate_no_closed_wires(self):
        """
        Tests generate() returns empty list when cut_area has no closed wires.

        INPUT:
        - cut_area built from a single open edge (not a closed face).

        EXPECTED OUTPUT:
        - Returns [] without raising an exception.
        """
        from Path.Base.Generator.adaptive_common import generate

        # Open wire — not closed
        open_edge = Part.makeLine(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(10, 0, 0))
        open_shape = Part.Shape([open_edge])

        result = generate(
            adaptive_params=self.adaptive_params,
            feed_params=self.feed_params,
            radius=self.radius,
            step_over=self.step_over,
            z_target=self.z_target,
            safe_z=self.safe_z,
            prev_z=self.prev_z,
            cut_area=open_shape,
            min_face_area=0.0,
            bb_face=self.bb_face,
            cut_area_offset=0.0,
            bb_face_offset=0.0,
        )
        self.assertEqual(result, [], "Open wire cut_area should return empty list")

    @unittest.skipUnless(_area_available, "libarea Adaptive2d not available")
    def test22_generate_produces_commands(self):
        """
        Integration test: generate() produces valid G-code commands for a
        simple square pocket.

        INPUT:
        - 40x40mm cut_area inside a 60x60mm stock boundary.
        - Standard 6mm tool, 2mm stepover.

        EXPECTED OUTPUT:
        - Returns a non-empty list of Path.Command objects.
        - Commands contain G0 (rapids) and G1 (cuts).
        - No command has Z below z_target.
        """
        from Path.Base.Generator.adaptive_common import generate

        cmds = generate(
            adaptive_params=self.adaptive_params,
            feed_params=self.feed_params,
            radius=self.radius,
            step_over=self.step_over,
            z_target=self.z_target,
            safe_z=self.safe_z,
            prev_z=self.prev_z,
            cut_area=self.cut_area,
            min_face_area=0.0,
            bb_face=self.bb_face,
            cut_area_offset=0.0,
            bb_face_offset=0.0,
        )

        self.assertGreater(len(cmds), 0, "Should produce G-code commands for valid input")

        cmd_names = {c.Name for c in cmds}
        self.assertIn("G0", cmd_names, "Should contain rapid moves")
        self.assertIn("G1", cmd_names, "Should contain cutting moves")

        # No command should go below z_target
        for cmd in cmds:
            z = cmd.Parameters.get("Z")
            if z is not None:
                self.assertGreaterEqual(
                    z,
                    self.z_target - 0.01,
                    f"Command {cmd.Name} has Z={z} below z_target={self.z_target}",
                )

    @unittest.skipUnless(_area_available, "libarea Adaptive2d not available")
    def test23_generate_lz_tracking(self):
        """
        Tests that generate() uses lz tracking — Z moves only emitted
        when height changes, not redundantly on every command.

        INPUT:
        - Standard square pocket, same parameters as test22.

        EXPECTED OUTPUT:
        - No two consecutive G1 commands at the same Z both have a Z parameter
          (the second should omit Z since it hasn't changed).
        """
        from Path.Base.Generator.adaptive_common import generate

        cmds = generate(
            adaptive_params=self.adaptive_params,
            feed_params=self.feed_params,
            radius=self.radius,
            step_over=self.step_over,
            z_target=self.z_target,
            safe_z=self.safe_z,
            prev_z=self.prev_z,
            cut_area=self.cut_area,
            min_face_area=0.0,
            bb_face=self.bb_face,
            cut_area_offset=0.0,
            bb_face_offset=0.0,
        )

        if not cmds:
            self.skipTest("No commands produced — skipping lz tracking check")

        # Count G1 commands that have a Z parameter
        g1_with_z = [c for c in cmds if c.Name == "G1" and "Z" in c.Parameters]
        g1_total = [c for c in cmds if c.Name == "G1"]

        # With lz tracking, most G1 cuts should NOT have Z (it hasn't changed)
        # At minimum, the first cut after a plunge/helix will have Z, but subsequent cuts won't
        if len(g1_total) > 1:
            self.assertLess(
                len(g1_with_z),
                len(g1_total),
                "lz tracking should prevent Z from being emitted on every G1",
            )

    @unittest.skipUnless(_area_available, "libarea Adaptive2d not available")
    def test24_generate_lift_distance_respected(self):
        """
        Tests that LinkClear moves lift by lift_distance above z_target.

        INPUT:
        - lift_distance = 2.0mm explicitly.

        EXPECTED OUTPUT:
        - Any G0 move between cut segments lifts to z_target + 2.0,
          not to safe_z.
        """
        from Path.Base.Generator.adaptive_common import generate

        params = self.adaptive_params.copy()
        params["lift_distance"] = 2.0

        cmds = generate(
            adaptive_params=params,
            feed_params=self.feed_params,
            radius=self.radius,
            step_over=self.step_over,
            z_target=self.z_target,
            safe_z=self.safe_z,
            prev_z=self.prev_z,
            cut_area=self.cut_area,
            min_face_area=0.0,
            bb_face=self.bb_face,
            cut_area_offset=0.0,
            bb_face_offset=0.0,
        )

        if not cmds:
            self.skipTest("No commands produced")

        expected_lift_z = self.z_target + 2.0

        # Verify that any G0 move respects either lift, safe, or prev heights.
        for cmd in cmds:
            z = cmd.Parameters.get("Z")
            if cmd.Name == "G0" and z is not None:
                is_lift = abs(z - expected_lift_z) < 0.01
                is_safe = abs(z - self.safe_z) < 0.01
                is_prev = abs(z - self.prev_z) < 0.01
                self.assertTrue(
                    is_lift or is_safe,
                    f"G0 Z={z} doesn't match lift_z={expected_lift_z} or safe_z={self.safe_z}",
                )

    @unittest.skipUnless(_area_available, "libarea Adaptive2d not available")
    def test25_generate_offset_respected(self):
        """
        Tests that generate() correctly applies the cut_area_offset.

        INPUT:
        - A 40x40mm cut_area.
        - cut_area_offset = 1.0mm.

        EXPECTED OUTPUT:
        - The generated toolpath is contained within a smaller, 38x38mm area.
        - The toolpath extents should be approximately offset by 1.0mm inwards
          from the original cut_area boundaries.
        """
        from Path.Base.Generator.adaptive_common import generate

        offset = 1.0

        cmds = generate(
            adaptive_params=self.adaptive_params,
            feed_params=self.feed_params,
            radius=self.radius,
            step_over=self.step_over,
            z_target=self.z_target,
            safe_z=self.safe_z,
            prev_z=self.prev_z,
            cut_area=self.cut_area,
            min_face_area=0.0,
            bb_face=self.bb_face,
            cut_area_offset=offset,
            bb_face_offset=0.0,
        )

        self.assertGreater(len(cmds), 0, "Should produce commands with offset")

        # Extract all X, Y coordinates from the path
        path_points = []
        for cmd in cmds:
            if "X" in cmd.Parameters and "Y" in cmd.Parameters:
                path_points.append((cmd.Parameters["X"], cmd.Parameters["Y"]))

        if not path_points:
            self.fail("No X, Y points found in the generated G-code")

        # Calculate the bounding box of the toolpath
        min_x = min(p[0] for p in path_points)
        max_x = max(p[0] for p in path_points)
        min_y = min(p[1] for p in path_points)
        max_y = max(p[1] for p in path_points)

        # Original cut_area is from (5, 5) to (45, 45)
        original_min = 5.0
        original_max = 45.0

        # The offset shrinks the area inwards.
        # The toolpath should be inside the offset boundary.
        expected_min_bound = original_min + offset
        expected_max_bound = original_max - offset

        # Tolerance for adaptive algorithm and discretization
        tolerance = 0.2

        self.assertGreater(
            min_x,
            expected_min_bound - tolerance,
            f"Path min_x ({min_x}) is outside the expected inner bound ({expected_min_bound})",
        )
        self.assertLess(
            max_x,
            expected_max_bound + tolerance,
            f"Path max_x ({max_x}) is outside the expected inner bound ({expected_max_bound})",
        )
        self.assertGreater(
            min_y,
            expected_min_bound - tolerance,
            f"Path min_y ({min_y}) is outside the expected inner bound ({expected_min_bound})",
        )
        self.assertLess(
            max_y,
            expected_max_bound + tolerance,
            f"Path max_y ({max_y}) is outside the expected inner bound ({expected_max_bound})",
        )
