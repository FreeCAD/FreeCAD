# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
#   Copyright (c) 2026 FreeCAD Project Association
#
#   This file is part of the FreeCAD CAx development system.
#
#   This library is free software; you can redistribute it and/or
#   modify it under the terms of the GNU Lesser General Public License
#   as published by the Free Software Foundation; either version 2.1 of
#   the License, or (at your option) any later version.
#
#   This library is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
#   Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public
#   License along with this library; see the file COPYING.LIB. If not,
#   write to the Free Software Foundation, Inc., 59 Temple Place,
#   Suite 330, Boston, MA 02111-1307, USA
# ***************************************************************************

import unittest

import FreeCAD as App
import Part


class TestLinkArrayCircular(unittest.TestCase):
    def setUp(self):
        self.doc = App.newDocument("TestLinkArrayCircular")
        source = self.doc.addObject("Part::Box", "Source")
        self.array = self.doc.addObject("Part::LinkArrayCircular", "Array")
        self.array.LinkedObject = source
        self.array.RadialDistance = 10
        self.array.TangentialDistance = 10
        self.array.NumberCircles = 3
        self.array.Symmetry = 4

    def tearDown(self):
        App.closeDocument(self.doc.Name)

    def testRingPopulationIsRoundedToSymmetry(self):
        self.assertEqual(
            self.array.getTypeIdOfProperty("RadialDistance"), "App::PropertyLength"
        )
        self.assertEqual(
            self.array.getTypeIdOfProperty("TangentialDistance"), "App::PropertyLength"
        )
        self.doc.recompute()

        self.assertEqual(self.array.getStatusString(), "Valid")
        # floor(2*pi*10/10) -> 6 -> 4, floor(2*pi*20/10) -> 12.
        self.assertEqual(self.array.ElementCount, 1 + 4 + 12)
        self.assertEqual(len(self.array.PlacementList), self.array.ElementCount)
        self.assertEqual(self.array.PlacementList[0], App.Placement())
        self.assertAlmostEqual(self.array.PlacementList[1].Base.Length, 10)
        self.assertAlmostEqual(self.array.PlacementList[5].Base.Length, 20)

    def testAxisReferenceOrientsTheCircles(self):
        axis = self.doc.addObject("Part::Feature", "Axis")
        axis.Shape = Part.makeLine(App.Vector(5, 0, 0), App.Vector(5, 10, 0))
        self.array.Axis = (axis, ["Edge1"])

        self.doc.recompute()

        self.assertEqual(self.array.getStatusString(), "Valid")
        for placement in self.array.PlacementList:
            self.assertAlmostEqual(placement.Base.y, 0)

        # The selected edge supplies the center as well as the axis direction.
        self.assertAlmostEqual(self.array.PlacementList[2].Base.x, -5)
