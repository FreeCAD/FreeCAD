# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
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

import FreeCAD
import Part
import Path
import PathScripts
import math
import unittest

from FreeCAD import Vector
from PathScripts.PathDressupHoldingTags import *
from PathTests.PathTestUtils import PathTestBase

class TestHoldingTags(PathTestBase):
    """Unit tests for the HoldingTags dressup."""

    def test00(self):
        """Check Tag origin, serialization and de-serialization."""
        tag = Tag(77, 13, 4, 5, 90, True)
        self.assertCoincide(tag.originAt(3), Vector(77, 13, 3))
        s = tag.toString()
        tagCopy = Tag.FromString(s)
        self.assertEqual(tag.x, tagCopy.x)
        self.assertEqual(tag.y, tagCopy.y)
        self.assertEqual(tag.height, tagCopy.height)
        self.assertEqual(tag.width, tagCopy.width)
        self.assertEqual(tag.enabled, tagCopy.enabled)


    def test01(self):
        """Verify solid for a 90 degree tag is a cylinder."""
        tag = Tag(100, 200, 4, 5, 90, True)
        tag.createSolidsAt(17, 0)

        self.assertIsNotNone(tag.solid)
        self.assertCylinderAt(tag.solid, Vector(100, 200, 17), 2, 5)

    def test02(self):
        """Verify trapezoidal tag has a cone shape with a lid."""
        tag = Tag(0, 0, 18, 5, 45, True)
        tag.createSolidsAt(0, 0)

        self.assertIsNotNone(tag.solid)
        self.assertConeAt(tag.solid, Vector(0,0,0), 9, 4, 5)

    def test03(self):
        """Verify pointy cone shape of tag with pointy end if width, angle and height match up."""
        tag = Tag(0, 0, 10, 5, 45, True)
        tag.createSolidsAt(0, 0)
        self.assertIsNotNone(tag.solid)
        self.assertConeAt(tag.solid, Vector(0,0,0), 5, 0, 5)

    def test04(self):
        """Verify height adjustment if tag isn't wide eough for angle."""
        tag = Tag(0, 0, 5, 17, 60, True)
        tag.createSolidsAt(0, 0)
        self.assertIsNotNone(tag.solid)
        self.assertConeAt(tag.solid, Vector(0,0,0), 2.5, 0, 2.5 * math.tan((60/180.0)*math.pi))

