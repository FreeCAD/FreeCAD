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
import unittest
import Arch
import Draft
import Part
import FreeCAD as App
from bimtests import TestArchBase


class TestArchWall(TestArchBase.TestArchBase):

    def testWall(self):
        operation = "Checking Arch Wall..."
        self.printTestMessage(operation)

        l=Draft.makeLine(App.Vector(0,0,0),App.Vector(-2,0,0))
        w = Arch.makeWall(l)
        self.assertTrue(w,"Arch Wall failed")

    def testWallMultiMatAlign(self):
        operation = "Checking Arch Wall with MultiMaterial and 3 alignments..."
        self.printTestMessage(operation)

        matA = Arch.makeMaterial()
        matB = Arch.makeMaterial()
        matMulti = Arch.makeMultiMaterial()
        matMulti.Materials = [matA, matB]
        matMulti.Thicknesses = [100, 200] # total width different from default 200
        pts = [App.Vector(   0,    0, 0),
               App.Vector(1000,    0, 0),
               App.Vector(1000, 1000, 0),
               App.Vector(2000, 1000, 0)]
        # wall based on wire:
        wire = Draft.makeWire(pts)
        wallWire = Arch.makeWall(wire)
        wallWire.Material = matMulti
        # wall based on sketch:
        sketch = App.activeDocument().addObject('Sketcher::SketchObject','Sketch')
        sketch.addGeometry([Part.LineSegment(pts[0], pts[1]),
                            Part.LineSegment(pts[1], pts[2]),
                            Part.LineSegment(pts[2], pts[3])])
        wallSketch = Arch.makeWall(sketch)
        wallSketch.Material = matMulti

        alignLst = ["Left", "Center", "Right"]
        checkLst = [[App.Vector(0, -300, 0), App.Vector(2000, 1000, 0)],
                    [App.Vector(0, -150, 0), App.Vector(2000, 1150, 0)],
                    [App.Vector(0,    0, 0), App.Vector(2000, 1300, 0)]]
        for i in range(3):
            wallWire.Align = alignLst[i]
            wallSketch.Align = alignLst[i]
            App.ActiveDocument.recompute()
            for box in [wallWire.Shape.BoundBox, wallSketch.Shape.BoundBox]:
                ptMin = App.Vector(box.XMin, box.YMin, 0)
                self.assertTrue(ptMin.isEqual(checkLst[i][0], 1e-8),
                                "Arch Wall with MultiMaterial and 3 alignments failed")
                ptMax = App.Vector(box.XMax, box.YMax, 0)
                self.assertTrue(ptMax.isEqual(checkLst[i][1], 1e-8),
                                "Arch Wall with MultiMaterial and 3 alignments failed")
