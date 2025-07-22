# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

__title__ = "Surface unit tests"
__author__ = "Werner Mayer"
__url__ = "https://www.freecad.org"

import sys
import unittest
from os.path import join

import FreeCAD
from FreeCAD import Base
import Surface

vec = Base.Vector


class TestBlendCurve(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    def assertAlmostCoincide(self, pt1, pt2, places=None, msg=None, delta=None):
        self.assertAlmostEqual(pt1.x, pt2.x, places, msg, delta)
        self.assertAlmostEqual(pt1.y, pt2.y, places, msg, delta)
        self.assertAlmostEqual(pt1.z, pt2.z, places, msg, delta)

    def test_blend_curve(self):
        # Create C0 BlendPoint at origin
        b1 = Surface.BlendPoint()
        # Create G1 BlendPoint
        b2 = Surface.BlendPoint([vec(10, 3, 6), vec(2, 5, 6)])
        # BlendCurve between the two BlendPoints
        bc = Surface.BlendCurve(b1, b2)
        # Compute the interpolating BezierCurve
        curve1 = bc.compute()

        # Create G2 BlendPoint at the end of previous BlendCurve
        b1 = Surface.BlendPoint(curve1.toShape(), 1, 2)
        # Create G1 BlendPoint
        b2 = Surface.BlendPoint([vec(5, 6, 2), vec(2, 5, 6)])

        bc = Surface.BlendCurve(b1, b2)
        # Compute the interpolating BezierCurve
        curve2 = bc.compute()

        d1 = curve1.getD2(1)
        d2 = curve2.getD2(0)

        self.assertEqual(len(d1), len(d2))
        self.assertAlmostCoincide(d1[0], d2[0])
        self.assertAlmostCoincide(d1[1], d2[1])
        self.assertAlmostCoincide(d1[2], d2[2])
