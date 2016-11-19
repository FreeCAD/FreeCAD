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
        self.assertTrue(pointsCoincide(tag.originAt(3), FreeCAD.Vector(77, 13, 3)))
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
        self.assertIsNotNone(tag.core)

        self.assertCylinderAt(tag.solid, FreeCAD.Vector(100, 200, 17), 2, 5)
        self.assertCylinderAt(tag.core, FreeCAD.Vector(100, 200, 17), 2, 5)

    def assertCylinderAt(self, solid, pt, r, h):
        """Verify that the argument is a cylinder at the specified location."""
        lid = solid.Edges[0].Curve
        self.assertTrue(type(lid), Part.Circle)
        self.assertEqual(lid.Center, FreeCAD.Vector(pt.x, pt.y, pt.z+h))
        self.assertEqual(lid.Radius, r)

        hull = solid.Edges[1].Curve
        self.assertTrue(type(hull), Part.Line)
        self.assertTrue(pointsCoincide(hull.StartPoint, FreeCAD.Vector(pt.x+r, pt.y, pt.z)))
        self.assertTrue(pointsCoincide(hull.EndPoint, FreeCAD.Vector(pt.x+r, pt.y, pt.z+h)))

        base = solid.Edges[2].Curve
        self.assertTrue(type(base), Part.Circle)
        self.assertEqual(base.Center, FreeCAD.Vector(pt.x, pt.y, pt.z))
        self.assertEqual(base.Radius, r)

