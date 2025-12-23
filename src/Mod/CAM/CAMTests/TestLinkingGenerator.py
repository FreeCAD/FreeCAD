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
