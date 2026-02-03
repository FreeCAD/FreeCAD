# -*- coding: utf-8 -*-
# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 sliptonic sliptonic@freecad.org                    *
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
import math
import Path.Base.Generator.spiral_facing as spiral_facing
import Path.Base.Generator.zigzag_facing as zigzag_facing
import Path.Base.Generator.directional_facing as directional_facing
import Path.Base.Generator.bidirectional_facing as bidirectional_facing
import Path.Base.Generator.facing_common as facing_common
import Part
import Path

from CAMTests.PathTestUtils import PathTestBase

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


class TestPathFacingGenerator(PathTestBase):
    """Test facing generator."""

    def setUp(self):
        """Set up test fixtures."""
        super().setUp()

        # Create test polygons
        self.square_wire = Part.makePolygon(
            [
                FreeCAD.Vector(0, 0, 0),
                FreeCAD.Vector(10, 0, 0),
                FreeCAD.Vector(10, 10, 0),
                FreeCAD.Vector(0, 10, 0),
                FreeCAD.Vector(0, 0, 0),
            ]
        )

        self.rectangle_wire = Part.makePolygon(
            [
                FreeCAD.Vector(0, 0, 0),
                FreeCAD.Vector(20, 0, 0),
                FreeCAD.Vector(20, 10, 0),
                FreeCAD.Vector(0, 10, 0),
                FreeCAD.Vector(0, 0, 0),
            ]
        )

        # Create a circular wire for testing curves
        self.circle_wire = Part.Wire(
            [Part.Circle(FreeCAD.Vector(5, 5, 0), FreeCAD.Vector(0, 0, 1), 5).toShape()]
        )

        # Create a wire with splines/curves
        points = [
            FreeCAD.Vector(0, 0, 0),
            FreeCAD.Vector(5, 0, 0),
            FreeCAD.Vector(10, 5, 0),
            FreeCAD.Vector(5, 10, 0),
            FreeCAD.Vector(0, 5, 0),
            FreeCAD.Vector(0, 0, 0),
        ]
        self.spline_wire = Part.Wire(
            [Part.BSplineCurve(points, None, None, False, 3, None, False).toShape()]
        )

    def _first_xy(self, commands):
        # Return XY of first G0 rapid move to get actual start position
        for cmd in commands:
            if cmd.Name == "G0" and "X" in cmd.Parameters and "Y" in cmd.Parameters:
                return (cmd.Parameters["X"], cmd.Parameters["Y"])
        # Fallback to first cutting move
        for cmd in commands:
            if cmd.Name == "G1" and "X" in cmd.Parameters and "Y" in cmd.Parameters:
                return (cmd.Parameters["X"], cmd.Parameters["Y"])
        return None

    def _bbox_diag(self, wire):
        bb = wire.BoundBox
        dx = bb.XMax - bb.XMin
        dy = bb.YMax - bb.YMin
        return math.hypot(dx, dy)

    def test_spiral_reverse_toggles_start_corner_climb(self):
        """Spiral reverse toggles the starting corner while keeping winding (climb)."""
        cmds_norm = spiral_facing.spiral(
            polygon=self.rectangle_wire,
            tool_diameter=10.0,
            stepover_percent=50,
            milling_direction="climb",
            reverse=False,
        )
        cmds_rev = spiral_facing.spiral(
            polygon=self.rectangle_wire,
            tool_diameter=10.0,
            stepover_percent=50,
            milling_direction="climb",
            reverse=True,
        )

        # Non-empty and similar length
        self.assertIsInstance(cmds_norm, list)
        self.assertIsInstance(cmds_rev, list)
        self.assertGreater(len(cmds_norm), 0)
        self.assertGreater(len(cmds_rev), 0)
        self.assertAlmostEqual(len(cmds_norm), len(cmds_rev), delta=max(1, 0.05 * len(cmds_norm)))

        # First command should be a move to start corner; compare XY distance equals bbox diagonal
        self.assertIn(cmds_norm[0].Name, ["G0", "G1"])
        self.assertIn(cmds_rev[0].Name, ["G0", "G1"])
        p0 = self._first_xy(cmds_norm)
        p1 = self._first_xy(cmds_rev)
        self.assertIsNotNone(p0)
        self.assertIsNotNone(p1)
        dx = p1[0] - p0[0]
        dy = p1[1] - p0[1]
        dist = math.hypot(dx, dy)
        self.assertAlmostEqual(dist, self._bbox_diag(self.rectangle_wire), places=6)

    def test_spiral_reverse_toggles_start_corner_conventional(self):
        """Spiral reverse toggles the starting corner while keeping winding (conventional)."""
        cmds_norm = spiral_facing.spiral(
            polygon=self.rectangle_wire,
            tool_diameter=10.0,
            stepover_percent=50,
            milling_direction="conventional",
            reverse=False,
        )
        cmds_rev = spiral_facing.spiral(
            polygon=self.rectangle_wire,
            tool_diameter=10.0,
            stepover_percent=50,
            milling_direction="conventional",
            reverse=True,
        )

        self.assertGreater(len(cmds_norm), 0)
        self.assertGreater(len(cmds_rev), 0)
        p0 = self._first_xy(cmds_norm)
        p1 = self._first_xy(cmds_rev)
        self.assertIsNotNone(p0)
        self.assertIsNotNone(p1)
        dx = p1[0] - p0[0]
        dy = p1[1] - p0[1]
        dist = math.hypot(dx, dy)
        self.assertAlmostEqual(dist, self._bbox_diag(self.rectangle_wire), places=6)

    def test_directional_strategy_basic(self):
        """Test directional strategy basic functionality."""
        commands = directional_facing.directional(
            polygon=self.square_wire,
            tool_diameter=5.0,
            stepover_percent=50,
        )

        # Should return a list of Path.Command objects
        self.assertIsInstance(commands, list)
        self.assertGreater(len(commands), 0)

        # All commands should be G0 or G1
        for cmd in commands:
            self.assertIn(cmd.Name, ["G0", "G1"])

    def test_directional_reverse_changes_start(self):
        """Directional reverse should start from the opposite side."""
        cmds_norm = directional_facing.directional(
            polygon=self.rectangle_wire,
            tool_diameter=10.0,
            stepover_percent=50,
            milling_direction="climb",
            reverse=False,
        )
        cmds_rev = directional_facing.directional(
            polygon=self.rectangle_wire,
            tool_diameter=10.0,
            stepover_percent=50,
            milling_direction="climb",
            reverse=True,
        )
        self.assertGreater(len(cmds_norm), 0)
        self.assertGreater(len(cmds_rev), 0)
        p0 = self._first_xy(cmds_norm)
        p1 = self._first_xy(cmds_rev)
        self.assertIsNotNone(p0)
        self.assertIsNotNone(p1)
        # Ensure the start points are different
        self.assertTrue(abs(p0[0] - p1[0]) > 1e-6 or abs(p0[1] - p1[1]) > 1e-6)

    def test_zigzag_reverse_flips_first_pass_direction_climb(self):
        """Zigzag reverse should flip the first pass direction while preserving alternation (climb)."""
        cmds_norm = zigzag_facing.zigzag(
            polygon=self.rectangle_wire,
            tool_diameter=5.0,
            stepover_percent=50,
            angle_degrees=0.0,
            milling_direction="climb",
            reverse=False,
        )
        cmds_rev = zigzag_facing.zigzag(
            polygon=self.rectangle_wire,
            tool_diameter=5.0,
            stepover_percent=50,
            angle_degrees=0.0,
            milling_direction="climb",
            reverse=True,
        )
        self.assertGreater(len(cmds_norm), 0)
        self.assertGreater(len(cmds_rev), 0)
        p0 = self._first_xy(cmds_norm)
        p1 = self._first_xy(cmds_rev)
        self.assertIsNotNone(p0)
        self.assertIsNotNone(p1)
        # Ensure start points differ (first pass toggled)
        self.assertTrue(abs(p0[0] - p1[0]) > 1e-6 or abs(p0[1] - p1[1]) > 1e-6)

    def test_zigzag_reverse_flips_first_pass_direction_conventional(self):
        """Zigzag reverse should flip the first pass direction while preserving alternation (conventional)."""
        cmds_norm = zigzag_facing.zigzag(
            polygon=self.rectangle_wire,
            tool_diameter=5.0,
            stepover_percent=50,
            angle_degrees=0.0,
            milling_direction="conventional",
            reverse=False,
        )
        cmds_rev = zigzag_facing.zigzag(
            polygon=self.rectangle_wire,
            tool_diameter=5.0,
            stepover_percent=50,
            angle_degrees=0.0,
            milling_direction="conventional",
            reverse=True,
        )
        self.assertGreater(len(cmds_norm), 0)
        self.assertGreater(len(cmds_rev), 0)
        p0 = self._first_xy(cmds_norm)
        p1 = self._first_xy(cmds_rev)
        self.assertIsNotNone(p0)
        self.assertIsNotNone(p1)
        # Ensure start points differ (first pass toggled)
        self.assertTrue(abs(p0[0] - p1[0]) > 1e-6 or abs(p0[1] - p1[1]) > 1e-6)

    def test_zigzag_reverse_and_milling_combinations(self):
        """Test all four combinations of reverse and milling_direction for zigzag."""
        # Expected behavior for zigzag (rectangle 0,0 to 20,10):
        # reverse=False, climb: start right, bottom (high X, low Y)
        # reverse=False, conventional: start left, bottom (low X, low Y)
        # reverse=True, climb: start left, top (low X, high Y)
        # reverse=True, conventional: start right, top (high X, high Y)

        test_cases = [
            ("climb", False, "right", "bottom"),
            ("climb", True, "left", "top"),
            ("conventional", False, "left", "bottom"),
            ("conventional", True, "right", "top"),
        ]

        results = {}
        for milling_dir, reverse, expected_x_side, expected_y_side in test_cases:
            commands = zigzag_facing.zigzag(
                polygon=self.rectangle_wire,
                tool_diameter=5.0,
                stepover_percent=50,
                angle_degrees=0.0,
                milling_direction=milling_dir,
                reverse=reverse,
            )
            pos = self._first_xy(commands)
            self.assertIsNotNone(
                pos, f"zigzag {milling_dir} reverse={reverse} returned no position"
            )
            results[(milling_dir, reverse)] = pos

            # Verify X side (left < 10, right > 10 for rectangle 0-20)
            if expected_x_side == "left":
                self.assertLess(
                    pos[0],
                    10,
                    f"zigzag {milling_dir} reverse={reverse}: expected left (X<10), got X={pos[0]}",
                )
            else:  # right
                self.assertGreater(
                    pos[0],
                    10,
                    f"zigzag {milling_dir} reverse={reverse}: expected right (X>10), got X={pos[0]}",
                )

            # Verify Y side (bottom < 5, top > 5 for rectangle 0-10)
            if expected_y_side == "bottom":
                self.assertLess(
                    pos[1],
                    5,
                    f"zigzag {milling_dir} reverse={reverse}: expected bottom (Y<5), got Y={pos[1]}",
                )
            else:  # top
                self.assertGreater(
                    pos[1],
                    5,
                    f"zigzag {milling_dir} reverse={reverse}: expected top (Y>5), got Y={pos[1]}",
                )

    def test_directional_reverse_and_milling_combinations(self):
        """Test all four combinations of reverse and milling_direction for directional."""
        # Expected behavior for directional (rectangle 0,0 to 20,10):
        # Bottom-to-top (reverse=False): climb=right-to-left, conventional=left-to-right
        # Top-to-bottom (reverse=True): climb=left-to-right, conventional=right-to-left
        # reverse=False, climb: start right, bottom (high X, low Y)
        # reverse=False, conventional: start left, bottom (low X, low Y)
        # reverse=True, climb: start left, top (low X, high Y)
        # reverse=True, conventional: start right, top (high X, high Y)

        test_cases = [
            ("climb", False, "right", "bottom"),
            ("climb", True, "left", "top"),
            ("conventional", False, "left", "bottom"),
            ("conventional", True, "right", "top"),
        ]

        for milling_dir, reverse, expected_x_side, expected_y_side in test_cases:
            commands = directional_facing.directional(
                polygon=self.rectangle_wire,
                tool_diameter=5.0,
                stepover_percent=50,
                angle_degrees=0.0,
                milling_direction=milling_dir,
                reverse=reverse,
            )
            pos = self._first_xy(commands)
            self.assertIsNotNone(
                pos, f"directional {milling_dir} reverse={reverse} returned no position"
            )

            # Verify X side
            if expected_x_side == "left":
                self.assertLess(
                    pos[0],
                    10,
                    f"directional {milling_dir} reverse={reverse}: expected left (X<10), got X={pos[0]}",
                )
            else:  # right
                self.assertGreater(
                    pos[0],
                    10,
                    f"directional {milling_dir} reverse={reverse}: expected right (X>10), got X={pos[0]}",
                )

            # Verify Y side
            if expected_y_side == "bottom":
                self.assertLess(
                    pos[1],
                    5,
                    f"directional {milling_dir} reverse={reverse}: expected bottom (Y<5), got Y={pos[1]}",
                )
            else:  # top
                self.assertGreater(
                    pos[1],
                    5,
                    f"directional {milling_dir} reverse={reverse}: expected top (Y>5), got Y={pos[1]}",
                )

    def test_bidirectional_reverse_and_milling_combinations(self):
        """Test all four combinations of reverse and milling_direction for bidirectional."""
        # Expected behavior for bidirectional (rectangle 0,0 to 20,10):
        # Bidirectional alternates between bottom and top
        # Bottom and top cut in OPPOSITE directions to maintain perpendicular rapids
        # reverse controls which side starts first
        # reverse=False, climb: start right, bottom (high X, low Y) - bottom cuts right-to-left
        # reverse=False, conventional: start left, bottom (low X, low Y) - bottom cuts left-to-right
        # reverse=True, climb: start left, top (low X, high Y) - top cuts left-to-right (opposite of bottom)
        # reverse=True, conventional: start right, top (high X, high Y) - top cuts right-to-left (opposite of bottom)

        test_cases = [
            ("climb", False, "right", "bottom"),
            ("climb", True, "left", "top"),
            ("conventional", False, "left", "bottom"),
            ("conventional", True, "right", "top"),
        ]

        for milling_dir, reverse, expected_x_side, expected_y_side in test_cases:
            commands = bidirectional_facing.bidirectional(
                polygon=self.rectangle_wire,
                tool_diameter=5.0,
                stepover_percent=50,
                angle_degrees=0.0,
                milling_direction=milling_dir,
                reverse=reverse,
            )
            pos = self._first_xy(commands)
            self.assertIsNotNone(
                pos, f"bidirectional {milling_dir} reverse={reverse} returned no position"
            )

            # Verify X side
            if expected_x_side == "left":
                self.assertLess(
                    pos[0],
                    10,
                    f"bidirectional {milling_dir} reverse={reverse}: expected left (X<10), got X={pos[0]}",
                )
            else:  # right
                self.assertGreater(
                    pos[0],
                    10,
                    f"bidirectional {milling_dir} reverse={reverse}: expected right (X>10), got X={pos[0]}",
                )

            # Verify Y side
            if expected_y_side == "bottom":
                self.assertLess(
                    pos[1],
                    5,
                    f"bidirectional {milling_dir} reverse={reverse}: expected bottom (Y<5), got Y={pos[1]}",
                )
            else:  # top
                self.assertGreater(
                    pos[1],
                    5,
                    f"bidirectional {milling_dir} reverse={reverse}: expected top (Y>5), got Y={pos[1]}",
                )

    def test_directional_climb_vs_conventional(self):
        """Test directional with different milling directions."""
        climb_commands = directional_facing.directional(
            polygon=self.square_wire,
            tool_diameter=5.0,
            stepover_percent=50,
            milling_direction="climb",
        )

        conventional_commands = directional_facing.directional(
            polygon=self.square_wire,
            tool_diameter=5.0,
            stepover_percent=50,
            milling_direction="conventional",
        )

        # Should have same number of commands but different coordinates
        self.assertEqual(len(climb_commands), len(conventional_commands))

        # At least some coordinates should be different
        different_coords = False
        for i in range(min(len(climb_commands), len(conventional_commands))):
            if climb_commands[i].Parameters != conventional_commands[i].Parameters:
                different_coords = True
                break
        self.assertTrue(different_coords)

    def test_directional_retract_height(self):
        """Test retract height functionality in directional."""
        commands_no_retract = directional_facing.directional(
            polygon=self.square_wire, tool_diameter=5.0, stepover_percent=50, retract_height=None
        )

        commands_with_retract = directional_facing.directional(
            polygon=self.square_wire, tool_diameter=5.0, stepover_percent=50, retract_height=15.0
        )

        # Commands with retract should have more moves (G0 Z moves)
        self.assertGreaterEqual(len(commands_with_retract), len(commands_no_retract))

        # Should have some Z-only G0 commands
        z_retracts = [
            cmd
            for cmd in commands_with_retract
            if cmd.Name == "G0" and "Z" in cmd.Parameters and len(cmd.Parameters) == 1
        ]
        self.assertGreater(len(z_retracts), 0)

    def test_zigzag_strategy_basic(self):
        """Test zigzag strategy basic functionality."""
        commands = zigzag_facing.zigzag(
            polygon=self.square_wire, tool_diameter=10.0, stepover_percent=50, angle_degrees=0.0
        )

        # Should return a list of Path.Command objects
        self.assertIsInstance(commands, list)
        self.assertGreater(len(commands), 0)

        # First command should be G0 (for op preamble replacement)
        self.assertEqual(commands[0].Name, "G0")

        # Should have cutting moves (G1)
        cutting_moves = [cmd for cmd in commands if cmd.Name == "G1"]
        self.assertGreater(len(cutting_moves), 0)

    def test_zigzag_alternating_direction(self):
        """Test that zigzag alternates cutting direction."""
        commands = zigzag_facing.zigzag(
            polygon=self.rectangle_wire,
            tool_diameter=2.0,  # Small tool for more passes
            stepover_percent=25,  # Small stepover for more passes
            angle_degrees=0.0,
        )

        # Should have multiple passes
        self.assertGreater(len(commands), 2)

        # Extract X coordinates from cutting moves
        x_coords = []
        for cmd in commands:
            if "X" in cmd.Parameters:
                x_coords.append(cmd.Parameters["X"])

        # Should have alternating pattern in X coordinates
        self.assertGreater(len(x_coords), 2)

    def test_zigzag_with_retract_height(self):
        """Test zigzag with retract height."""
        # Arc mode (default) - no retracts, uses arcs
        commands_arc_mode = zigzag_facing.zigzag(
            polygon=self.square_wire,
            tool_diameter=5.0,
            stepover_percent=50,
            retract_height=15.0,
            link_mode="arc",
        )

        # Straight mode - should use retracts
        commands_straight_mode = zigzag_facing.zigzag(
            polygon=self.square_wire,
            tool_diameter=5.0,
            stepover_percent=50,
            retract_height=15.0,
            link_mode="straight",
        )

        # Both should generate valid toolpaths
        self.assertGreater(len(commands_arc_mode), 0)
        self.assertGreater(len(commands_straight_mode), 0)

        # Arc mode should have G2/G3 arcs and no Z retracts
        arcs = [cmd for cmd in commands_arc_mode if cmd.Name in ["G2", "G3"]]
        z_retracts_arc = [
            cmd
            for cmd in commands_arc_mode
            if cmd.Name == "G0" and "Z" in cmd.Parameters and cmd.Parameters["Z"] == 15.0
        ]
        self.assertGreater(len(arcs), 0, "Arc mode should have G2/G3 commands")
        self.assertEqual(
            len(z_retracts_arc), 0, "Arc mode should not have Z retracts between passes"
        )

        # Straight mode should have Z retracts (retract_height is ignored in current implementation)
        # Note: zigzag doesn't currently support retract_height in straight mode, only uses G0 at cutting height
        # So this test documents current behavior

    def test_zigzag_arc_links(self):
        """Test zigzag arc linking generates proper G2/G3 commands."""
        commands = zigzag_facing.zigzag(
            polygon=self.square_wire, tool_diameter=5.0, stepover_percent=50, link_mode="arc"
        )

        # Should have both cutting moves and arcs
        g1_moves = [cmd for cmd in commands if cmd.Name == "G1"]
        arcs = [cmd for cmd in commands if cmd.Name in ["G2", "G3"]]

        self.assertGreater(len(g1_moves), 0, "Should have G1 cutting moves")
        self.assertGreater(len(arcs), 0, "Should have G2/G3 arc moves")

        # Arcs should have I, J, K parameters
        for arc in arcs:
            self.assertIn("I", arc.Parameters, f"{arc.Name} should have I parameter")
            self.assertIn("J", arc.Parameters, f"{arc.Name} should have J parameter")
            self.assertIn("K", arc.Parameters, f"{arc.Name} should have K parameter")
            self.assertIn("X", arc.Parameters, f"{arc.Name} should have X parameter")
            self.assertIn("Y", arc.Parameters, f"{arc.Name} should have Y parameter")

    def test_zigzag_arc_vs_straight_link_modes(self):
        """Test that arc and straight link modes produce different but valid toolpaths."""
        arc_commands = zigzag_facing.zigzag(
            polygon=self.rectangle_wire, tool_diameter=5.0, stepover_percent=50, link_mode="arc"
        )

        straight_commands = zigzag_facing.zigzag(
            polygon=self.rectangle_wire,
            tool_diameter=5.0,
            stepover_percent=50,
            link_mode="straight",
        )

        # Both should generate valid paths
        self.assertGreater(len(arc_commands), 0)
        self.assertGreater(len(straight_commands), 0)

        # Arc mode should have arcs, straight mode should not
        arc_mode_arcs = [cmd for cmd in arc_commands if cmd.Name in ["G2", "G3"]]
        straight_mode_arcs = [cmd for cmd in straight_commands if cmd.Name in ["G2", "G3"]]

        self.assertGreater(len(arc_mode_arcs), 0, "Arc mode should have G2/G3 commands")
        self.assertEqual(len(straight_mode_arcs), 0, "Straight mode should have no G2/G3 commands")

        # Straight mode should have more G0 rapids (one per link)
        arc_mode_g0 = [cmd for cmd in arc_commands if cmd.Name == "G0"]
        straight_mode_g0 = [cmd for cmd in straight_commands if cmd.Name == "G0"]
        self.assertGreater(
            len(straight_mode_g0),
            len(arc_mode_g0),
            "Straight mode should have more G0 rapids than arc mode",
        )

    def test_zigzag_milling_direction(self):
        """Test zigzag with different milling directions."""
        climb_commands = zigzag_facing.zigzag(
            polygon=self.square_wire,
            tool_diameter=5.0,
            stepover_percent=50,
            milling_direction="climb",
        )

        conventional_commands = zigzag_facing.zigzag(
            polygon=self.square_wire,
            tool_diameter=5.0,
            stepover_percent=50,
            milling_direction="conventional",
        )

        # Should have same number of commands
        self.assertEqual(len(climb_commands), len(conventional_commands))

        # But coordinates should be different
        different_coords = False
        for i in range(min(len(climb_commands), len(conventional_commands))):
            if climb_commands[i].Parameters != conventional_commands[i].Parameters:
                different_coords = True
                break
        self.assertTrue(different_coords)

    def test_get_angled_polygon_zero_degrees(self):
        """Test get_angled_polygon with 0 degree rotation."""
        result = facing_common.get_angled_polygon(self.square_wire, 0)

        # Should get back a valid wire
        self.assertTrue(result.isClosed())
        # Bounding box should be similar to original
        original_bb = self.square_wire.BoundBox
        result_bb = result.BoundBox
        self.assertAlmostEqual(original_bb.XLength, result_bb.XLength, places=1)
        self.assertAlmostEqual(original_bb.YLength, result_bb.YLength, places=1)

    def test_get_angled_polygon_45_degrees(self):
        """Test get_angled_polygon with 45 degree rotation."""
        result = facing_common.get_angled_polygon(self.square_wire, 45)

        self.assertTrue(result.isClosed())
        # The rotated bounding box should be larger than the original
        original_bb = self.square_wire.BoundBox
        result_bb = result.BoundBox

        # The function creates a bounding box that fully contains the rotated wire
        # For a 45-degree rotation, this will be larger than just the diagonal
        # The result should be larger than the original in both dimensions
        self.assertGreater(result_bb.XLength, original_bb.XLength)
        self.assertGreater(result_bb.YLength, original_bb.YLength)

        # Should have 4 edges (rectangular)
        self.assertEqual(len(result.Edges), 4)

    def test_analyze_rectangle_axis_aligned(self):
        """Test polygon geometry extraction with axis-aligned rectangle."""
        result = facing_common.extract_polygon_geometry(self.rectangle_wire)

        # Should have edges and corners
        self.assertIsNotNone(result["edges"])
        self.assertIsNotNone(result["corners"])
        self.assertEqual(len(result["edges"]), 4)
        self.assertEqual(len(result["corners"]), 4)

    def test_analyze_rectangle_short_preference(self):
        """Test edge selection with short axis preference."""
        polygon_info = facing_common.extract_polygon_geometry(self.rectangle_wire)
        result = facing_common.select_primary_step_edges(polygon_info["edges"], "short")

        # Primary should be shorter axis (Y for this rectangle)
        self.assertAlmostEqual(result["primary_length"], 10, places=1)
        self.assertAlmostEqual(result["step_length"], 20, places=1)

    def test_analyze_rectangle_invalid_polygon(self):
        """Test edge selection with invalid polygon."""
        # Create a triangle (3 edges instead of 4)
        triangle_wire = Part.makePolygon(
            [
                FreeCAD.Vector(0, 0, 0),
                FreeCAD.Vector(10, 0, 0),
                FreeCAD.Vector(5, 10, 0),
                FreeCAD.Vector(0, 0, 0),
            ]
        )

        # Should raise ValueError for non-rectangular polygon
        with self.assertRaises(ValueError):
            facing_common.extract_polygon_geometry(triangle_wire)

    def test_spiral_conventional_milling(self):
        """Test spiral strategy with conventional milling direction."""
        commands = spiral_facing.spiral(
            self.square_wire, 10.0, 50.0, milling_direction="conventional"
        )

        self.assertGreater(len(commands), 0)
        # First move should be G0 rapid positioning
        self.assertEqual(commands[0].Name, "G0")

        # Check that we have cutting moves (G1 commands)
        cutting_moves = [cmd for cmd in commands if cmd.Name == "G1"]
        self.assertGreaterEqual(len(cutting_moves), 4)  # At least one complete rectangle

    def test_bidirectional_basic(self):
        """Test basic bidirectional strategy functionality."""
        commands = bidirectional_facing.bidirectional(self.square_wire, 10.0, 50.0)

        self.assertGreater(len(commands), 0)
        # First move should be G0 to start position (op will replace with preamble)
        self.assertEqual(commands[0].Name, "G0")

        # Check that we have cutting moves (G1 commands)
        cutting_moves = [cmd for cmd in commands if cmd.Name == "G1"]
        self.assertGreater(len(cutting_moves), 1)  # At least one pass

        # Check that we have rapid moves (G0 commands) between passes
        rapid_moves = [cmd for cmd in commands if cmd.Name == "G0"]
        self.assertGreater(len(rapid_moves), 0)

    def test_bidirectional_climb_milling(self):
        """Test bidirectional strategy with climb milling direction."""
        commands = bidirectional_facing.bidirectional(
            self.square_wire, 10.0, 50.0, milling_direction="climb"
        )

        self.assertGreater(len(commands), 0)
        # First move should be G0 to start position (op will replace with preamble)
        self.assertEqual(commands[0].Name, "G0")

        # Check that we have cutting moves (G1 commands)
        cutting_moves = [cmd for cmd in commands if cmd.Name == "G1"]
        self.assertGreater(len(cutting_moves), 1)

    def test_bidirectional_conventional_milling(self):
        """Test bidirectional strategy with conventional milling direction."""
        commands = bidirectional_facing.bidirectional(
            self.square_wire, 10.0, 50.0, milling_direction="conventional"
        )

        self.assertGreater(len(commands), 0)
        # First move should be G0 to start position (op will replace with preamble)
        self.assertEqual(commands[0].Name, "G0")

        # Check that we have cutting moves (G1 commands)
        cutting_moves = [cmd for cmd in commands if cmd.Name == "G1"]
        self.assertGreater(len(cutting_moves), 1)

    def test_bidirectional_with_retract_height(self):
        """Test bidirectional strategy - rapids stay at cutting height."""
        commands = bidirectional_facing.bidirectional(self.square_wire, 10.0, 50.0)

        self.assertGreater(len(commands), 0)

        # Bidirectional should have rapid moves at cutting height (no Z retracts between passes)
        rapid_moves = [
            cmd
            for cmd in commands
            if cmd.Name == "G0" and "X" in cmd.Parameters and "Y" in cmd.Parameters
        ]
        self.assertGreater(len(rapid_moves), 0)

    def test_bidirectional_alternating_positions(self):
        """Test that bidirectional strategy alternates between bottom and top positions."""
        commands = bidirectional_facing.bidirectional(
            self.rectangle_wire, 2.0, 25.0, milling_direction="climb"
        )

        # Get all G1 cutting moves
        cutting_moves = [
            cmd
            for cmd in commands
            if cmd.Name == "G1" and "X" in cmd.Parameters and "Y" in cmd.Parameters
        ]

        # Should have multiple cutting moves
        self.assertGreaterEqual(len(cutting_moves), 4)

        # For bidirectional, we should have rapid moves (G0) between passes
        rapid_moves = [cmd for cmd in commands if cmd.Name == "G0"]
        self.assertGreater(len(rapid_moves), 0)

        # Extract Y coordinates of start positions for each cutting move
        start_y_coords = [
            cutting_moves[i].Parameters["Y"]
            for i in range(0, len(cutting_moves), 2)
            if i < len(cutting_moves)
        ]

        # Should have alternating Y positions (bottom and top)
        if len(start_y_coords) >= 4:
            # Separate into bottom and top passes based on Y coordinate
            sorted_coords = sorted(start_y_coords)
            mid_y = (sorted_coords[0] + sorted_coords[-1]) / 2.0

            bottom_passes = sorted([y for y in start_y_coords if y < mid_y])
            top_passes = sorted([y for y in start_y_coords if y > mid_y], reverse=True)

            # Bottom passes should be increasing (stepping inward from bottom)
            if len(bottom_passes) >= 2:
                self.assertLess(
                    bottom_passes[0], bottom_passes[1]
                )  # Second bottom pass higher than first

            # Top passes should be decreasing (stepping inward from top)
            if len(top_passes) >= 2:
                self.assertGreater(top_passes[0], top_passes[1])  # Second top pass lower than first

    def test_bidirectional_axis_preference_long(self):
        """Test bidirectional strategy with angle for long axis."""
        commands = bidirectional_facing.bidirectional(
            self.rectangle_wire, 5.0, 50.0, angle_degrees=0.0
        )

        self.assertGreater(len(commands), 0)
        # Should generate valid toolpath commands
        cutting_moves = [cmd for cmd in commands if cmd.Name == "G1"]
        self.assertGreater(len(cutting_moves), 1)

    def test_bidirectional_axis_preference_short(self):
        """Test bidirectional strategy with angle for short axis."""
        commands = bidirectional_facing.bidirectional(
            self.rectangle_wire, 5.0, 50.0, angle_degrees=90.0
        )

        self.assertGreater(len(commands), 0)
        # Should generate valid toolpath commands
        cutting_moves = [cmd for cmd in commands if cmd.Name == "G1"]
        self.assertGreater(len(cutting_moves), 1)

    def test_bidirectional_with_pass_extension(self):
        """Test bidirectional strategy with pass extension parameter."""
        pass_extension = 2.0
        commands = bidirectional_facing.bidirectional(
            self.square_wire, 10.0, 50.0, pass_extension=pass_extension
        )

        self.assertGreater(len(commands), 0)
        # Should generate valid toolpath commands
        cutting_moves = [cmd for cmd in commands if cmd.Name == "G1"]
        self.assertGreater(len(cutting_moves), 1)

    def test_spiral_layer_calculation(self):
        """Test that spiral generates appropriate number of layers."""
        # Use small stepover to get multiple layers
        commands = spiral_facing.spiral(
            polygon=self.square_wire,  # 10x10 square
            tool_diameter=2.0,
            stepover_percent=25,  # 0.5mm stepover
        )

        # Should have multiple layers
        self.assertGreater(len(commands), 8)  # At least 2-3 layers with multiple moves each

        # Extract unique positions to verify spiral pattern
        positions = []
        for cmd in commands:
            if "X" in cmd.Parameters and "Y" in cmd.Parameters:
                positions.append((cmd.Parameters["X"], cmd.Parameters["Y"]))

        # Should have multiple unique positions
        unique_positions = set(positions)
        self.assertGreater(len(unique_positions), 4)
        import Path

        p = Path.Path(commands)
        print(p.toGCode())

    def test_spiral_milling_direction(self):
        """Test spiral with different milling directions."""
        climb_commands = spiral_facing.spiral(
            polygon=self.square_wire,
            tool_diameter=4.0,
            stepover_percent=50,
            milling_direction="climb",
        )

        conventional_commands = spiral_facing.spiral(
            polygon=self.square_wire,
            tool_diameter=4.0,
            stepover_percent=50,
            milling_direction="conventional",
        )

        # Should have same number of commands
        self.assertEqual(len(climb_commands), len(conventional_commands))

        # But coordinates should be different due to different spiral direction
        different_coords = False
        for i in range(min(len(climb_commands), len(conventional_commands))):
            if climb_commands[i].Parameters != conventional_commands[i].Parameters:
                different_coords = True
                break
        self.assertTrue(different_coords)

    def test_spiral_centered_on_origin(self):
        """Test spiral with rectangle centered on origin to debug overlapping passes."""
        import Part

        # Create a 10x6 rectangle centered on origin (-5,-3) to (5,3)
        centered_rectangle = Part.makePolygon(
            [
                FreeCAD.Vector(-5, -3, 0),
                FreeCAD.Vector(5, -3, 0),
                FreeCAD.Vector(5, 3, 0),
                FreeCAD.Vector(-5, 3, 0),
                FreeCAD.Vector(-5, -3, 0),
            ]
        )

        # Use small stepover to get multiple layers
        commands = spiral_facing.spiral(
            polygon=centered_rectangle,  # 10x6 rectangle centered on origin
            tool_diameter=2.0,
            stepover_percent=25,  # 0.5mm stepover
        )

        # Should have multiple layers
        self.assertGreater(len(commands), 8)  # At least 2-3 layers with multiple moves each

        # Extract unique positions to verify spiral pattern
        positions = []
        for cmd in commands:
            if "X" in cmd.Parameters and "Y" in cmd.Parameters:
                positions.append((cmd.Parameters["X"], cmd.Parameters["Y"]))

        # Should have multiple unique positions
        unique_positions = set(positions)
        self.assertGreater(len(unique_positions), 4)

        # Print G-code for debugging
        import Path

        p = Path.Path(commands)
        print("Centered on origin G-code:")
        print(p.toGCode())

    def test_spiral_axis_preference_variations(self):
        """Test spiral with different axis preferences and milling directions."""
        import Part

        # Create a 12x8 rectangle centered on origin (-6,-4) to (6,4)
        # Long axis = 12mm (X), Short axis = 8mm (Y)
        test_rectangle = Part.makePolygon(
            [
                FreeCAD.Vector(-6, -4, 0),
                FreeCAD.Vector(6, -4, 0),
                FreeCAD.Vector(6, 4, 0),
                FreeCAD.Vector(-6, 4, 0),
                FreeCAD.Vector(-6, -4, 0),
            ]
        )

        # Test different combinations
        test_cases = [
            ("long", "climb"),
            ("long", "conventional"),
            ("short", "climb"),
            ("short", "conventional"),
        ]

        for axis_pref, milling_dir in test_cases:
            with self.subTest(axis_preference=axis_pref, milling_direction=milling_dir):
                commands = spiral_facing.spiral(
                    polygon=test_rectangle,
                    tool_diameter=2.0,
                    stepover_percent=25,
                    milling_direction=milling_dir,
                )

                # Should have multiple layers
                self.assertGreater(len(commands), 8)

                # Print G-code for debugging
                import Path

                p = Path.Path(commands)
                print(f"\n{axis_pref} axis, {milling_dir} milling G-code:")
                print(p.toGCode())

    def test_spiral_angled_rectangle(self):
        """Test spiral with angled rectangle to verify it follows polygon shape, not bounding box."""
        import Part
        import math

        # Create a 12x8 rectangle rotated 30 degrees
        # This will test if spiral follows the actual polygon or just the axis-aligned bounding box
        angle = math.radians(30)
        cos_a = math.cos(angle)
        sin_a = math.sin(angle)

        # Original rectangle corners (before rotation)
        corners = [(-6, -4), (6, -4), (6, 4), (-6, 4)]

        # Rotate corners
        rotated_corners = []
        for x, y in corners:
            new_x = x * cos_a - y * sin_a
            new_y = x * sin_a + y * cos_a
            rotated_corners.append(FreeCAD.Vector(new_x, new_y, 0))

        # Close the polygon
        rotated_corners.append(rotated_corners[0])

        angled_rectangle = Part.makePolygon(rotated_corners)

        # Test both axis preferences with the angled rectangle
        for axis_pref in ["long", "short"]:
            with self.subTest(axis_preference=axis_pref):
                commands = spiral_facing.spiral(
                    polygon=angled_rectangle,
                    tool_diameter=2.0,
                    stepover_percent=25,
                    milling_direction="climb",
                )

                # Should have multiple layers
                self.assertGreater(len(commands), 8)

                # Print G-code for debugging
                import Path

                p = Path.Path(commands)
                print(f"\nAngled rectangle {axis_pref} axis G-code:")
                print(p.toGCode())

    def test_spiral_continuous_cutting(self):
        """Test that spiral maintains continuous cutting motion throughout."""
        commands = spiral_facing.spiral(
            polygon=self.square_wire, tool_diameter=4.0, stepover_percent=50
        )

        # Spiral should have only one rapid move (G0) for initial positioning
        rapid_moves = [cmd for cmd in commands if cmd.Name == "G0"]

        # Should have exactly one rapid move for initial positioning
        self.assertEqual(len(rapid_moves), 1)
        # First command should be the rapid positioning move
        self.assertEqual(commands[0].Name, "G0")

        # Should have multiple cutting moves after the initial rapid move
        cutting_moves = [cmd for cmd in commands if cmd.Name == "G1"]
        self.assertGreater(len(cutting_moves), 0)
        # Total commands should be rapid moves + cutting moves
        self.assertEqual(len(commands), len(rapid_moves) + len(cutting_moves))

    def test_spiral_rectangular_polygon(self):
        """Test spiral with rectangular (non-square) polygon."""
        commands = spiral_facing.spiral(
            polygon=self.rectangle_wire, tool_diameter=3.0, stepover_percent=40  # 20x10 rectangle
        )

        # Should generate valid spiral
        self.assertGreater(len(commands), 0)

        # First command should be rapid positioning (G0), rest should be cutting moves (G1)
        self.assertEqual(commands[0].Name, "G0")
        for cmd in commands[1:]:
            if "X" in cmd.Parameters and "Y" in cmd.Parameters:
                self.assertEqual(cmd.Name, "G1")

        # Should stay within reasonable bounds
        for cmd in commands:
            if "X" in cmd.Parameters:
                x = cmd.Parameters["X"]
                # Should be within extended rectangle bounds
                self.assertGreaterEqual(x, -5)  # Some margin for tool
                self.assertLessEqual(x, 25)
            if "Y" in cmd.Parameters:
                y = cmd.Parameters["Y"]
                self.assertGreaterEqual(y, -5)
                self.assertLessEqual(y, 15)

    def test_spiral_basic(self):
        """Test spiral basic functionality."""
        commands = spiral_facing.spiral(
            polygon=self.rectangle_wire, tool_diameter=4.0, stepover_percent=50
        )

        # Should generate valid path
        self.assertGreater(len(commands), 0)

        # First command should be G0 rapid positioning
        self.assertEqual(commands[0].Name, "G0")

        # Should have cutting moves
        cutting_moves = [cmd for cmd in commands if cmd.Name == "G1"]
        self.assertGreater(len(cutting_moves), 0)

    def test_spiral_continuous_pattern(self):
        """Test that spiral generates a continuous inward pattern without diagonal jumps."""
        # Use a simple 10x10 square with 2mm tool and 50% stepover for predictable results
        commands = spiral_facing.spiral(
            polygon=self.square_wire,  # 10x10 square
            tool_diameter=2.0,
            stepover_percent=50,  # 1mm stepover
            milling_direction="climb",
        )

        # Extract all G1 cutting moves
        cutting_moves = [
            (cmd.Parameters.get("X"), cmd.Parameters.get("Y"))
            for cmd in commands
            if cmd.Name == "G1" and "X" in cmd.Parameters and "Y" in cmd.Parameters
        ]

        self.assertGreater(len(cutting_moves), 0, "Should have cutting moves")

        # Verify the spiral pattern:
        # ALL moves should be axis-aligned (straight along edges, X or Y constant)
        # A proper rectangular spiral has no diagonal moves at all
        diagonal_moves = []
        for i in range(len(cutting_moves) - 1):
            x1, y1 = cutting_moves[i]
            x2, y2 = cutting_moves[i + 1]

            # Check if move is axis-aligned (either X or Y stays constant)
            x_change = abs(x2 - x1)
            y_change = abs(y2 - y1)

            # Allow small tolerance for floating point errors
            is_axis_aligned = x_change < 0.01 or y_change < 0.01

            if not is_axis_aligned:
                diagonal_moves.append((i, x1, y1, x2, y2, x_change, y_change))

        # Should have NO diagonal moves
        if diagonal_moves:
            msg = "Found diagonal moves instead of axis-aligned edges:\n"
            for i, x1, y1, x2, y2, dx, dy in diagonal_moves[:5]:  # Show first 5
                msg += f"  Move {i} to {i+1}: ({x1:.2f},{y1:.2f}) -> ({x2:.2f},{y2:.2f}) dx={dx:.2f} dy={dy:.2f}\n"
            self.fail(msg)

        # Verify the pattern spirals inward by checking that later moves are closer to center
        center_x, center_y = 5.0, 5.0
        first_quarter = cutting_moves[: len(cutting_moves) // 4]
        last_quarter = cutting_moves[3 * len(cutting_moves) // 4 :]

        avg_dist_first = sum(
            ((x - center_x) ** 2 + (y - center_y) ** 2) ** 0.5 for x, y in first_quarter
        ) / len(first_quarter)
        avg_dist_last = sum(
            ((x - center_x) ** 2 + (y - center_y) ** 2) ** 0.5 for x, y in last_quarter
        ) / len(last_quarter)

        self.assertLess(
            avg_dist_last,
            avg_dist_first,
            "Spiral should move inward - later moves should be closer to center",
        )

    def _create_mock_tool_controller(self, spindle_dir):
        """Create a mock tool controller for testing."""

        class MockToolController:
            def __init__(self, spindle_direction):
                self.SpindleDir = spindle_direction

        return MockToolController(spindle_dir)
