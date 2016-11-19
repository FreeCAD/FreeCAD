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
from PathScripts.PathDressupHoldingTags import Tag

slack = 0.0000001

def pointsCoincide(pt1, pt2):
    pt = pt1 - pt2
    if math.fabs(pt.x) > slack:
        return False
    if math.fabs(pt.y) > slack:
        return False
    if math.fabs(pt.z) > slack:
        return False
    return True

class PathDressupHoldingTagsTestCases(unittest.TestCase):
    """Unit tests for the HoldingTags dressup."""

    def testTagBasics(self):
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


    def testTagSolidBasic(self):
        """For a 90 degree tag the core and solid are both defined and identical cylinders."""
        tag = Tag(100, 200, 4, 5, 90, True)
        tag.createSolidsAt(17)

        self.assertIsNotNone(tag.solid)
        self.assertCylinderAt(tag.solid, Vector(100, 200, 17), 2, 5)

        self.assertIsNotNone(tag.core)
        self.assertCylinderAt(tag.core, Vector(100, 200, 17), 2, 5)

    def testTagSolidFlatCone(self):
        """Tests a Tag that has an angle leaving a flat face on top of the cone."""
        tag = Tag(0, 0, 18, 5, 45, True)
        tag.createSolidsAt(0)

        self.assertIsNotNone(tag.solid)
        self.assertConeAt(tag.solid, Vector(0,0,0), 9, 4, 5)

        self.assertIsNotNone(tag.core)
        self.assertCylinderAt(tag.core, Vector(0,0,0), 4, 5)

    def testTagSolidCone(self):
        """Tests a Tag who's angled sides coincide at the tag's height."""
        tag = Tag(0, 0, 10, 5, 45, True)
        tag.createSolidsAt(0)
        self.assertIsNotNone(tag.solid)
        self.assertConeAt(tag.solid, Vector(0,0,0), 5, 0, 5)

        self.assertIsNone(tag.core)

    def testTagSolidShortCone(self):
        """Tests a Tag that's not wide enough to reach full height."""
        tag = Tag(0, 0, 5, 17, 60, True)
        tag.createSolidsAt(0)
        self.assertIsNotNone(tag.solid)
        self.assertConeAt(tag.solid, Vector(0,0,0), 2.5, 0, 2.5 * math.tan((60/180.0)*math.pi))

        self.assertIsNone(tag.core)

    def assertCylinderAt(self, solid, pt, r, h):
        """Verify that solid is a cylinder at the specified location."""
        self.assertEqual(len(solid.Edges), 3)

        lid  = solid.Edges[0]
        hull = solid.Edges[1]
        base = solid.Edges[2]

        self.assertCircle(lid, Vector(pt.x, pt.y, pt.z+h), r)
        self.assertLine(hull, Vector(pt.x+r, pt.y, pt.z), Vector(pt.x+r, pt.y, pt.z+h))
        self.assertCircle(base, Vector(pt.x, pt.y, pt.z), r)

    def assertConeAt(self, solid, pt, r1, r2, h):
        """Verify that solid is a cone at the specified location."""
        self.assertEqual(len(solid.Edges), 3)

        lid  = solid.Edges[0]
        hull = solid.Edges[1]
        base = solid.Edges[2]

        self.assertCircle(lid, Vector(pt.x, pt.y, pt.z+h), r2)
        self.assertLine(hull, Vector(pt.x+r1, pt.y, pt.z), Vector(pt.x+r2, pt.y, pt.z+h))
        self.assertCircle(base, Vector(pt.x, pt.y, pt.z), r1)

    def assertCircle(self, edge, pt, r):
        """Verivy that edge is a circle at given location."""
        curve = edge.Curve
        self.assertTrue(type(curve), Part.Circle)
        self.assertCoincide(curve.Center, Vector(pt.x, pt.y, pt.z))
        self.assertAbout(curve.Radius, r)

    def assertLine(self, edge, pt1, pt2):
        """Verify that edge is a line from pt1 to pt2."""
        curve = edge.Curve
        self.assertTrue(type(curve), Part.Line)
        self.assertCoincide(curve.StartPoint, pt1)
        self.assertCoincide(curve.EndPoint, pt2)

    def assertCoincide(self, pt1, pt2):
        """Verify that 2 points coincide (with tolerance)."""
        self.assertAbout(pt1.x, pt2.x)
        self.assertAbout(pt1.y, pt2.y)
        self.assertAbout(pt1.z, pt2.z)

    def assertAbout(self, v1, v2):
        """Verify that 2 values are the same (accounting for float imprecision)."""
        #print("assertAbout(%f, %f)" % (v1, v2))
        self.assertTrue(math.fabs(v1 - v2) < slack)
