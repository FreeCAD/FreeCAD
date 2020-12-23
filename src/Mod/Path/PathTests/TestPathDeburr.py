# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
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
import PathScripts.PathDeburr as PathDeburr
import PathScripts.PathLog as PathLog
import PathTests.PathTestUtils as PathTestUtils

PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())
#PathLog.trackModule(PathLog.thisModule())


class TestPathDeburr(PathTestUtils.PathTestBase):

    def test00(self):
        '''Verify chamfer depth and offset for an end mill.'''
        tool = Path.Tool()
        tool.Diameter = 20
        tool.FlatRadius = 0
        tool.CuttingEdgeAngle = 180

        (depth, offset) = PathDeburr.toolDepthAndOffset(1, 0.01, tool)
        self.assertRoughly(0.01, depth)
        self.assertRoughly(9, offset)

        # legacy tools - no problem, same result
        tool.CuttingEdgeAngle = 0

        (depth, offset) = PathDeburr.toolDepthAndOffset(1, 0.01, tool)
        self.assertRoughly(0.01, depth)
        self.assertRoughly(9, offset)

    def test01(self):
        '''Verify chamfer depth and offset for a 90° v-bit.'''
        tool = Path.Tool()
        tool.FlatRadius = 0
        tool.CuttingEdgeAngle = 90

        (depth, offset) = PathDeburr.toolDepthAndOffset(1, 0, tool)
        self.assertRoughly(1, depth)
        self.assertRoughly(0, offset)

        (depth, offset) = PathDeburr.toolDepthAndOffset(1, 0.2, tool)
        self.assertRoughly(1.2, depth)
        self.assertRoughly(0.2, offset)

    def test02(self):
        '''Verify chamfer depth and offset for a 90° v-bit with non 0 flat radius.'''
        tool = Path.Tool()
        tool.FlatRadius = 0.3
        tool.CuttingEdgeAngle = 90

        (depth, offset) = PathDeburr.toolDepthAndOffset(1, 0, tool)
        self.assertRoughly(1, depth)
        self.assertRoughly(0.3, offset)

        (depth, offset) = PathDeburr.toolDepthAndOffset(2, 0.2, tool)
        self.assertRoughly(2.2, depth)
        self.assertRoughly(0.5, offset)

    def test03(self):
        '''Verify chamfer depth and offset for a 60° v-bit with non 0 flat radius.'''
        tool = Path.Tool()
        tool.FlatRadius = 10
        tool.CuttingEdgeAngle = 60

        td = 1.73205

        (depth, offset) = PathDeburr.toolDepthAndOffset(1, 0, tool)
        self.assertRoughly(td, depth)
        self.assertRoughly(10, offset)

        (depth, offset) = PathDeburr.toolDepthAndOffset(3, 1, tool)
        self.assertRoughly(td * 3 + 1, depth)
        self.assertRoughly(10 + td, offset)
