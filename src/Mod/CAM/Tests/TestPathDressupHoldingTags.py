# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
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

import Tests.PathTestUtils as PathTestUtils
import math

from FreeCAD import Vector
from Path.Dressup.Tags import Tag


class TestHoldingTags(PathTestUtils.PathTestBase):
    """Unit tests for the HoldingTags dressup."""

    def test00(self):
        """Check Tag origin."""
        tag = Tag(0, 77, 13, 4, 5, 90, True)
        self.assertCoincide(tag.originAt(3), Vector(77, 13, 3))

    def test01(self):
        """Verify solid for a 90 degree tag is a cylinder."""
        tag = Tag(0, 100, 200, 4, 5, 90, 0, True)
        tag.createSolidsAt(17, 0)

        self.assertIsNotNone(tag.solid)
        self.assertCylinderAt(tag.solid, Vector(100, 200, 17 - 5 * 0.01), 2, 5 * 1.01)

    def test02(self):
        """Verify trapezoidal tag has a cone shape with a lid."""
        tag = Tag(0, 0, 0, 18, 5, 45, 0, True)
        tag.createSolidsAt(0, 0)

        self.assertIsNotNone(tag.solid)
        self.assertConeAt(tag.solid, Vector(0, 0, -0.05), 9, 3.95, 5.05)

    def test03(self):
        """Verify pointy cone shape of tag with pointy end if width, angle and height match up."""
        tag = Tag(0, 0, 0, 10, 5, 45, 0, True)
        tag.createSolidsAt(0, 0)
        self.assertIsNotNone(tag.solid)
        h = 5 * 1.01
        self.assertConeAt(tag.solid, Vector(0, 0, -h * 0.01), 5, 0, h)

    def test04(self):
        """Verify height adjustment if tag isn't wide eough for angle."""
        tag = Tag(0, 0, 0, 5, 17, 60, 0, True)
        tag.createSolidsAt(0, 0)
        self.assertIsNotNone(tag.solid)
        h = 2.5 * math.tan((60 / 180.0) * math.pi) * 1.01
        print(h)
        self.assertConeAt(tag.solid, Vector(0, 0, -h * 0.01), 2.5, 0, h)
