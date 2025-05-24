# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2013 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

# Unit tests for the Arch wall module

import os
import Arch
import Draft
import Part
import FreeCAD as App
from bimtests import TestArchBase
from draftutils.messages import _msg

class TestArchRoof(TestArchBase.TestArchBase):

    def testRoof(self):
        operation = "Checking Arch Roof..."
        self.printTestMessage(operation)

        r = Draft.makeRectangle(length=2,height=-1)
        r.recompute() # required before calling Arch.makeRoof
        ro = Arch.makeRoof(r)
        self.assertTrue(ro,"Arch Roof failed")

    def testRoof81Permutations(self):
        """Create 81 roofs using a range of arguments.
        """
        operation = "Arch Roof testRoof81Permutations"
        self.printTestMessage(operation)

        pts = [App.Vector(   0,    0, 0),
               App.Vector(2000,    0, 0),
               App.Vector(4000,    0, 0),
               App.Vector(4000, 4000, 0),
               App.Vector(   0, 4000, 0)]
        ptsMod = [App.Vector(2000,     0, 0),
                  App.Vector(2000, -1000, 0),
                  App.Vector(2000,  1000, 0)]
        angsMod = [[60, 60], [30, 60], [60, 30]]
        runsMod = [[500, 500], [400, 500], [500, 400]]
        overhangsMod = [[100, 100], [100, 200], [200, 100]]
        delta = 6000
        pla = App.Placement()
        for iY in range(9):
            for iX in range(9):
                pts[1] = ptsMod[iY % 3] # to get different edge angles
                angsLst = angsMod[iY // 3] + [90, 90, 90]
                runsLst = runsMod[iX % 3] + [0, 0, 0]
                overhangsLst = overhangsMod[iX // 3] +  [0, 0, 0]
                pla.Base = App.Vector(iX * delta, iY * delta, 0)
                wire = Draft.makeWire(pts, closed = True)
                wire.MakeFace = False
                wire.Placement = pla
                wire.recompute() # required before calling Arch.makeRoof
                roof = Arch.makeRoof(wire,
                                     angles = angsLst,
                                     run = runsLst,
                                     overhang = overhangsLst)
                roof.recompute()
                self.assertFalse(roof.Shape.isNull(),
                                 "'{}' failed".format(operation))
                self.assertTrue(roof.Shape.isValid(),
                                "'{}' failed".format(operation))

    def testRoofAllAngles90(self):
        """Create a roof with the angles of all segments set at 90 degrees.
        This corner case results in a flat roof.
        """
        operation = "Arch Roof testRoofAllAngles90"
        self.printTestMessage(operation)

        pts = [App.Vector(   0,    0, 0),
               App.Vector(2000,    0, 0),
               App.Vector(2000, 2000, 0),
               App.Vector(   0, 2000, 0)]

        wire = Draft.makeWire(pts, closed = True)
        wire.MakeFace = False
        wire.recompute() # required before calling Arch.makeRoof
        roof = Arch.makeRoof(wire,
                             angles = [90, 90, 90, 90])
        roof.recompute()
        self.assertFalse(roof.Shape.isNull(), "'{}' failed".format(operation))
        self.assertTrue(roof.Shape.isValid(), "'{}' failed".format(operation))

    def testRoofApex(self):
        """Create a hipped roof that relies on apex calculation. The roof has
        2 triangular segments with a single apex point.
        """
        operation = "Arch Roof testRoofApex"
        self.printTestMessage(operation)

        rec = Draft.makeRectangle(length = 4000,
                                  height = 3000,
                                  face = False)
        rec.recompute() # required before calling Arch.makeRoof
        roof = Arch.makeRoof(rec,
                             angles = [30, 40, 50, 60],
                             run = [2000, 0, 2000, 0],
                             idrel = [-1, 0, -1, 0],
                             thickness = [50.0],
                             overhang = [100.0])
        roof.recompute()
        self.assertFalse(roof.Shape.isNull(), "'{}' failed".format(operation))
        self.assertTrue(roof.Shape.isValid(), "'{}' failed".format(operation))

    def testRoofSingleEavePoint(self):
        """Create a roof with a single triangular segment that has a single
        eave point.
        """
        operation = "Arch Roof testRoofSingleEavePoint"
        self.printTestMessage(operation)

        pts = [App.Vector(    0,    0, 0),
               App.Vector( 2000,    0, 0),
               App.Vector( 4000, 2000, 0),
               App.Vector(-2000, 2000, 0)]
        wire = Draft.makeWire(pts, closed = True)
        wire.MakeFace = False
        wire.recompute() # required before calling Arch.makeRoof
        roof = Arch.makeRoof(wire,
                             angles = [45, 90, 90, 90],
                             run = [1000, 0, 0, 0],
                             overhang = [1000, 0, 0, 0])
        roof.recompute()
        self.assertFalse(roof.Shape.isNull(), "'{}' failed".format(operation))
        self.assertTrue(roof.Shape.isValid(), "'{}' failed".format(operation))

    def test_makeRoof(self):
        """Test the makeRoof function."""
        operation = "Testing makeRoof function"
        self.printTestMessage(operation)

        roof = Arch.makeRoof(name="TestRoof")
        self.assertIsNotNone(roof, "makeRoof failed to create a roof object.")
        self.assertEqual(roof.Label, "TestRoof", "Roof label is incorrect.")
