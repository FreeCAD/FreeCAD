# SPDX-FileNotice: Part of the FreeCAD project.

# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2025 Brad Collette                                      *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify *
# *   it under the terms of the GNU Lesser General Public License (LGPL)   *
# *   as published by the Free Software Foundation; either version 2 of    *
# *   the License, or (at your option) any later version.                  *
# *   for detail see the LICENCE text file.                                *
# *                                                                         *
# ***************************************************************************

import FreeCAD
import Part
import CAMTests.PathTestUtils as PathTestUtils
import Path.Base.Generator.linking as generator
import unittest


class TestGetLinkingMoves(PathTestUtils.PathTestBase):
    def setUp(self):
        self.start = FreeCAD.Vector(0, 0, 0)
        self.target = FreeCAD.Vector(10, 0, 0)
        self.local_clearance = 2.0
        self.global_clearance = 5.0
        self.tool = Part.makeCylinder(1, 5)

    def test_simple_move(self):
        cmds = generator.get_linking_moves(
            start_position=self.start,
            target_position=self.target,
            local_clearance=self.local_clearance,
            global_clearance=self.global_clearance,
            tool_shape=self.tool,
            solids=[],
        )
        self.assertGreater(len(cmds), 0)
        self.assertEqual(cmds[0].Name, "G0")

    def test_same_position_returns_empty(self):
        cmds = generator.get_linking_moves(
            start_position=self.start,
            target_position=self.start,
            local_clearance=self.local_clearance,
            global_clearance=self.global_clearance,
            tool_shape=self.tool,
            solids=[],
        )
        self.assertEqual(len(cmds), 0)

    def test_negative_retract_offset_raises(self):
        with self.assertRaises(ValueError):
            generator.get_linking_moves(
                start_position=self.start,
                target_position=self.target,
                local_clearance=self.local_clearance,
                global_clearance=self.global_clearance,
                tool_shape=self.tool,
                retract_height_offset=-1,
            )

    def test_clearance_violation_raises(self):
        with self.assertRaises(ValueError):
            generator.get_linking_moves(
                start_position=self.start,
                target_position=self.target,
                local_clearance=10.0,
                global_clearance=5.0,
                tool_shape=self.tool,
            )

    def test_path_blocked_by_solid(self):
        blocking_box = Part.makeBox(20, 20, 10)
        blocking_box.translate(FreeCAD.Vector(-5, -5, 0))
        with self.assertRaises(RuntimeError):
            generator.get_linking_moves(
                start_position=self.start,
                target_position=self.target,
                local_clearance=self.local_clearance,
                global_clearance=self.global_clearance,
                tool_shape=self.tool,
                solids=[blocking_box],
            )

    def test_plunge_to_zero_depth(self):
        """Test that plunge moves correctly go to Z=0 (regression test for depth==0 bug)"""
        start = FreeCAD.Vector(0, 0, 1)  # Start below clearance
        target = FreeCAD.Vector(10, 10, 0)  # Target depth is 0

        cmds = generator.get_linking_moves(
            start_position=start,
            target_position=target,
            local_clearance=self.local_clearance,
            global_clearance=self.global_clearance,
            tool_shape=self.tool,
            solids=[],
        )

        # Verify we got commands
        self.assertGreater(len(cmds), 0)

        # All commands should have complete XYZ coordinates
        for cmd in cmds:
            self.assertIn("X", cmd.Parameters, "Command missing X coordinate")
            self.assertIn("Y", cmd.Parameters, "Command missing Y coordinate")
            self.assertIn("Z", cmd.Parameters, "Command missing Z coordinate")

        # The last command should be the plunge to target depth (Z=0)
        last_cmd = cmds[-1]
        self.assertAlmostEqual(last_cmd.Parameters["X"], target.x, places=5)
        self.assertAlmostEqual(last_cmd.Parameters["Y"], target.y, places=5)
        self.assertAlmostEqual(
            last_cmd.Parameters["Z"],
            target.z,
            places=5,
            msg="Final plunge should go to target Z=0, not clearance height",
        )

    def test_plunge_to_negative_depth(self):
        """Test that plunge moves correctly go to negative Z depths"""
        start = FreeCAD.Vector(0, 0, 1)  # Start below clearance
        target = FreeCAD.Vector(10, 10, -2)  # Target depth is negative

        cmds = generator.get_linking_moves(
            start_position=start,
            target_position=target,
            local_clearance=self.local_clearance,
            global_clearance=self.global_clearance,
            tool_shape=self.tool,
            solids=[],
        )

        # The last command should be the plunge to target depth (Z=-2)
        last_cmd = cmds[-1]
        self.assertAlmostEqual(
            last_cmd.Parameters["Z"],
            target.z,
            places=5,
            msg="Final plunge should go to target Z=-2",
        )

    @unittest.skip("not yet implemented")
    def test_zero_retract_offset_uses_local_clearance(self):
        cmds = generator.get_linking_moves(
            start_position=self.start,
            target_position=FreeCAD.Vector(10, 0, 5),
            local_clearance=self.local_clearance,
            global_clearance=self.global_clearance,
            tool_shape=self.tool,
            retract_height_offset=0,
        )
        self.assertTrue(any(cmd for cmd in cmds if cmd.Parameters.get("Z") == self.local_clearance))

    @unittest.skip("not yet implemented")
    def test_path_generated_without_local_safe(self):
        pass
