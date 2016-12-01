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
import math
import unittest

from PathScripts.PathGeom import Side

class PathTestBase(unittest.TestCase):
    """Base test class with some addtional asserts."""

    def assertRoughly(self, f1, f2):
        """Verify that two float values are approximately the same."""
        self.assertTrue(math.fabs(f1 - f2) < 0.00001, "%f != %f" % (f1, f2))

    def assertCoincide(self, pt1, pt2):
        """Verify that two points coincide - roughly speaking."""
        self.assertRoughly(pt1.x, pt2.x)
        self.assertRoughly(pt1.y, pt2.y)
        self.assertRoughly(pt1.z, pt2.z)

    def assertLine(self, edge, pt1, pt2):
        """Verify that edge is a line from pt1 to pt2."""
        self.assertIs(type(edge.Curve), Part.Line)
        self.assertCoincide(edge.Curve.StartPoint, pt1)
        self.assertCoincide(edge.Curve.EndPoint, pt2)

    def assertArc(self, edge, pt1, pt2, direction = 'CW'):
        """Verify that edge is an arc between pt1 and pt2 with the given direction."""
        # If an Arc is wrapped into edge, then it's curve is represented as a circle
        # and not as an Arc (GeomTrimmedCurve)
        #self.assertIs(type(edge.Curve), Part.Arc)
        self.assertIs(type(edge.Curve), Part.Circle)
        self.assertCoincide(edge.valueAt(edge.FirstParameter), pt1)
        self.assertCoincide(edge.valueAt(edge.LastParameter), pt2)
        ptm = edge.valueAt((edge.LastParameter + edge.FirstParameter)/2)
        side = Side.of(pt2 - pt1, ptm - pt1)
        #print("(%.2f, %.2f)  (%.2f, %.2f)  (%.2f, %.2f)" % (pt1.x, pt1.y, ptm.x, ptm.y, pt2.x, pt2.y))
        #print("    (%.2f, %.2f)  (%.2f, %.2f)  ->  %s" % ((pt2-pt1).x, (pt2-pt1).y, (ptm-pt1).x, (ptm-pt1).y, Side.toString(side)))
        #print("    (%.2f, %.2f)  (%.2f, %.2f)  ->  (%.2f, %.2f)" % (pf.x,pf.y, pl.x,pl.y, pm.x, pmy))
        if 'CW' == direction:
            self.assertEqual(side, Side.Left)
        else:
            self.assertEqual(side, Side.Right)


    def assertCurve(self, edge, p1, p2, p3):
        """Verify that the edge goes through the given 3 points, representing start, mid and end point respectively."""
        self.assertCoincide(edge.valueAt(edge.FirstParameter), p1)
        self.assertCoincide(edge.valueAt(edge.LastParameter), p3)
        self.assertCoincide(edge.valueAt((edge.FirstParameter + edge.LastParameter)/2), p2)

