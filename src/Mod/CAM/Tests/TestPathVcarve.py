# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2020 sliptonic <shopinthewoods@gmail.com>               *
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
import Path.Op.Vcarve as PathVcarve
import Path.Tool.Bit as PathToolBit
import math

from Tests.PathTestUtils import PathTestBase


class VbitTool(object):
    """Faked out vcarve tool"""

    def __init__(self, dia, angle, tipDia):
        self.Diameter = FreeCAD.Units.Quantity(dia, FreeCAD.Units.Length)
        self.CuttingEdgeAngle = FreeCAD.Units.Quantity(angle, FreeCAD.Units.Angle)
        self.TipDiameter = FreeCAD.Units.Quantity(tipDia, FreeCAD.Units.Length)


Scale45 = 2.414214
Scale60 = math.sqrt(3)


class TestPathVcarve(PathTestBase):
    """Test Vcarve milling basics."""

    def test00(self):
        """Verify 90 deg depth calculation"""
        tool = VbitTool(10, 90, 0)
        geom = PathVcarve._Geometry.FromTool(tool, 0, -10)
        self.assertRoughly(geom.start, 0)
        self.assertRoughly(geom.stop, -5)
        self.assertRoughly(geom.scale, 1)

    def test01(self):
        """Verify 90 deg depth limit"""
        tool = VbitTool(10, 90, 0)
        geom = PathVcarve._Geometry.FromTool(tool, 0, -3)
        self.assertRoughly(geom.start, 0)
        self.assertRoughly(geom.stop, -3)
        self.assertRoughly(geom.scale, 1)

    def test02(self):
        """Verify 60 deg depth calculation"""
        tool = VbitTool(10, 60, 0)
        geom = PathVcarve._Geometry.FromTool(tool, 0, -10)
        self.assertRoughly(geom.start, 0)
        self.assertRoughly(geom.stop, -5 * Scale60)
        self.assertRoughly(geom.scale, Scale60)

    def test03(self):
        """Verify 60 deg depth limit"""
        tool = VbitTool(10, 60, 0)
        geom = PathVcarve._Geometry.FromTool(tool, 0, -3)
        self.assertRoughly(geom.start, 0)
        self.assertRoughly(geom.stop, -3)
        self.assertRoughly(geom.scale, Scale60)

    def test10(self):
        """Verify 90 deg with tip dia depth calculation"""
        tool = VbitTool(10, 90, 2)
        geom = PathVcarve._Geometry.FromTool(tool, 0, -10)
        # in order for the width to be correct the height needs to be shifted
        self.assertRoughly(geom.start, 1)
        self.assertRoughly(geom.stop, -4)
        self.assertRoughly(geom.scale, 1)

    def test11(self):
        """Verify 90 deg with tip dia depth limit calculation"""
        tool = VbitTool(10, 90, 2)
        geom = PathVcarve._Geometry.FromTool(tool, 0, -3)
        # in order for the width to be correct the height needs to be shifted
        self.assertRoughly(geom.start, 1)
        self.assertRoughly(geom.stop, -3)
        self.assertRoughly(geom.scale, 1)

    def test12(self):
        """Verify 45 deg with tip dia depth calculation"""
        tool = VbitTool(10, 45, 2)
        geom = PathVcarve._Geometry.FromTool(tool, 0, -10)
        # in order for the width to be correct the height needs to be shifted
        self.assertRoughly(geom.start, Scale45)
        self.assertRoughly(geom.stop, -4 * Scale45)
        self.assertRoughly(geom.scale, Scale45)

    def test13(self):
        """Verify 45 deg with tip dia depth limit calculation"""
        tool = VbitTool(10, 45, 2)
        geom = PathVcarve._Geometry.FromTool(tool, 0, -3)
        # in order for the width to be correct the height needs to be shifted
        self.assertRoughly(geom.start, Scale45)
        self.assertRoughly(geom.stop, -3)
        self.assertRoughly(geom.scale, Scale45)
