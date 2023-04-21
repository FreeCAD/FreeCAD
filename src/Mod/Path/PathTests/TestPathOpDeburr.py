# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2018 sliptonic <shopinthewoods@gmail.com>               *
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
import Path.Op.Deburr as PathDeburr
import Path.Tool.Bit as PathToolBit
import PathTests.PathTestUtils as PathTestUtils

Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
# Path.Log.trackModule(Path.Log.thisModule())


class MockToolBit(object):
    def __init__(self, name="t1", diameter=5.0):
        self.Diameter = diameter
        self.FlatRadius = 0
        self.CuttingEdgeAngle = 60


class TestPathOpDeburr(PathTestUtils.PathTestBase):
    def test00(self):
        """Verify chamfer depth and offset for an end mill."""
        tool = MockToolBit()
        tool.Diameter = 20
        tool.FlatRadius = 0
        tool.CuttingEdgeAngle = 180

        (depth, offset, __, info) = PathDeburr.toolDepthAndOffset(1, 0.01, tool, True)
        self.assertRoughly(0.01, depth)
        self.assertRoughly(9, offset)
        self.assertFalse(info)

        # legacy tools - no problem, same result
        tool.CuttingEdgeAngle = 0

        (depth, offset, __, info) = PathDeburr.toolDepthAndOffset(1, 0.01, tool, True)
        self.assertRoughly(0.01, depth)
        self.assertRoughly(9, offset)
        self.assertFalse(info)

    def test01(self):
        """Verify chamfer depth and offset for a 90 deg v-bit."""
        tool = MockToolBit()
        tool.FlatRadius = 0
        tool.CuttingEdgeAngle = 90

        (depth, offset, __, info) = PathDeburr.toolDepthAndOffset(1, 0, tool, True)
        self.assertRoughly(1, depth)
        self.assertRoughly(0, offset)
        self.assertFalse(info)

        (depth, offset, __, info) = PathDeburr.toolDepthAndOffset(1, 0.2, tool, True)
        self.assertRoughly(1.2, depth)
        self.assertRoughly(0.2, offset)
        self.assertFalse(info)

    def test02(self):
        """Verify chamfer depth and offset for a 90 deg v-bit with non 0 flat radius."""
        tool = MockToolBit()
        tool.FlatRadius = 0.3
        tool.CuttingEdgeAngle = 90

        (depth, offset, __, info) = PathDeburr.toolDepthAndOffset(1, 0, tool, True)
        self.assertRoughly(1, depth)
        self.assertRoughly(0.3, offset)
        self.assertFalse(info)

        (depth, offset, __, info) = PathDeburr.toolDepthAndOffset(2, 0.2, tool, True)
        self.assertRoughly(2.2, depth)
        self.assertRoughly(0.5, offset)
        self.assertFalse(info)

    def test03(self):
        """Verify chamfer depth and offset for a 60 deg v-bit with non 0 flat radius."""
        tool = MockToolBit()
        tool.FlatRadius = 0.1
        tool.CuttingEdgeAngle = 60

        (depth, offset, __, info) = PathDeburr.toolDepthAndOffset(1, 0.5, tool, True)
        self.assertRoughly(2.232051, depth)
        self.assertRoughly(0.388675, offset)
        self.assertFalse(info)

        (depth, offset, __, info) = PathDeburr.toolDepthAndOffset(3, 1, tool, True)
        self.assertRoughly(6.196153, depth)
        self.assertRoughly(0.677350, offset)
        self.assertFalse(info)

    def test04(self):
        """Verify chamfer depth and offset for a 30 deg v-bit with non 0 flat radius."""
        tool = MockToolBit()
        tool.FlatRadius = 0.1
        tool.CuttingEdgeAngle = 30

        (depth, offset, __, info) = PathDeburr.toolDepthAndOffset(1, 0.5, tool, True)
        self.assertRoughly(4.232051, depth)
        self.assertRoughly(0.233975, offset)
        self.assertFalse(info)

        (depth, offset, __, info) = PathDeburr.toolDepthAndOffset(3, 1, tool, True)
        self.assertRoughly(12.196155, depth)
        self.assertRoughly(0.367949, offset)
        self.assertFalse(info)

    def test10(self):
        """Verify missing cutting edge angle info prints only once."""

        class FakeEndmill(object):
            def __init__(self, dia):
                self.Diameter = dia

        tool = FakeEndmill(10)
        (depth, offset, __, info) = PathDeburr.toolDepthAndOffset(1, 0.1, tool, True)
        self.assertRoughly(0.1, depth)
        self.assertRoughly(4, offset)
        self.assertTrue(info)
        (depth, offset, __, info) = PathDeburr.toolDepthAndOffset(
            1, 0.1, tool, not info
        )
        self.assertRoughly(0.1, depth)
        self.assertRoughly(4, offset)
        self.assertTrue(info)
        (depth, offset, __, info) = PathDeburr.toolDepthAndOffset(
            1, 0.1, tool, not info
        )
        self.assertRoughly(0.1, depth)
        self.assertRoughly(4, offset)
        self.assertTrue(info)

    def test11(self):
        """Verify missing tip diameter info prints only once."""

        class FakePointyBit(object):
            def __init__(self, dia, angle):
                self.Diameter = dia
                self.CuttingEdgeAngle = angle

        tool = FakePointyBit(10, 90)
        (depth, offset, __, info) = PathDeburr.toolDepthAndOffset(1, 0.1, tool, True)
        self.assertRoughly(1.1, depth)
        self.assertRoughly(0.1, offset)
        self.assertTrue(info)
        (depth, offset, __, info) = PathDeburr.toolDepthAndOffset(
            1, 0.1, tool, not info
        )
        self.assertRoughly(1.1, depth)
        self.assertRoughly(0.1, offset)
        self.assertTrue(info)
        (depth, offset, __, info) = PathDeburr.toolDepthAndOffset(
            1, 0.1, tool, not info
        )
        self.assertRoughly(1.1, depth)
        self.assertRoughly(0.1, offset)
        self.assertTrue(info)
