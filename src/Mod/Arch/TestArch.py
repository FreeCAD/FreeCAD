#***************************************************************************
#*   Copyright (c) 2013 Yorik van Havre <yorik@uncreated.net>              *
#*                                                                         *
#*   This file is part of the FreeCAD CAx development system.              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful,            *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with FreeCAD; if not, write to the Free Software        *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************/

# Unit test for the Arch module

import os
import unittest

import FreeCAD as App

import Arch
import Draft
import Part
import Sketcher

from draftutils.messages import _msg

if App.GuiUp:
    import FreeCADGui

brepArchiCAD = """
DBRep_DrawableShape

CASCADE Topology V1, (c) Matra-Datavision
Locations 3
1
              1               0               0               0
              0               1               0               0
              0               0               1               0
1
              0               1               0               0
             -1               0               0               0
              0               0               1               0
2  2 -1 0
Curve2ds 0
Curves 12
1 0 0 0 1 0 0
1 3000 0 0 0 0 1
1 3000 0 3000 -1 0 0
1 0 0 3000 0 0 -1
1 0 0 0 0 1 0
1 0 5000 0 1 0 0
1 3000 5000 0 0 -1 0
1 3000 5000 0 0 0 1
1 3000 5000 3000 0 -1 0
1 3000 5000 3000 -1 0 0
1 0 5000 3000 0 -1 0
1 0 5000 3000 0 0 -1
Polygon3D 0
PolygonOnTriangulations 24
2 1 2
p 18.3333333333333 1 0 3000
2 1 4
p 18.3333333333333 1 0 3000
2 2 3
p 18.3333333333333 1 0 3000
2 2 4
p 18.3333333333333 1 0 3000
2 3 4
p 18.3333333333333 1 0 3000
2 1 2
p 18.3333333333333 1 0 3000
2 4 1
p 18.3333333333333 1 0 3000
2 3 1
p 18.3333333333333 1 0 3000
2 1 2
p 18.3333333333333 1 0 5000
2 1 2
p 18.3333333333333 1 0 5000
2 2 3
p 18.3333333333333 1 0 3000
2 1 2
p 18.3333333333333 1 0 3000
2 3 4
p 18.3333333333333 1 0 5000
2 1 2
p 18.3333333333333 1 0 5000
2 1 3
p 18.3333333333333 1 0 3000
2 2 4
p 18.3333333333333 1 0 3000
2 3 4
p 18.3333333333333 1 0 5000
2 3 1
p 18.3333333333333 1 0 5000
2 3 4
p 18.3333333333333 1 0 3000
2 4 3
p 18.3333333333333 1 0 3000
2 4 2
p 18.3333333333333 1 0 5000
2 4 3
p 18.3333333333333 1 0 5000
2 4 2
p 18.3333333333333 1 0 3000
2 3 1
p 18.3333333333333 1 0 3000
Surfaces 6
1 1500 0 1500 -0 -1 -0 0 0 -1 1 0 0
1 1500 2500 0 -0 -0 -1 -1 0 0 0 1 0
1 3000 2500 1500 1 0 0 0 0 1 0 -1 0
1 1500 2500 3000 0 0 1 1 0 0 0 1 0
1 0 2500 1500 -1 -0 -0 0 0 -1 0 -1 0
1 1500 5000 1500 0 1 0 0 0 1 1 0 0
Triangulations 6
4 2 1 18.3333333333333
0 0 0 3000 0 0 3000 0 3000 0 0 3000 1500 -1500 1500 1500 -1500 1500 -1500 -1500 3 4 1 2 3 1
4 2 1 18.3333333333333
0 0 0 0 5000 0 3000 5000 0 3000 0 0 1500 -2500 1500 2500 -1500 2500 -1500 -2500 2 3 4 2 4 1
4 2 1 18.3333333333333
3000 5000 0 3000 0 0 3000 5000 3000 3000 0 3000 -1500 -2500 -1500 2500 1500 -2500 1500 2500 4 2 1 4 1 3
4 2 1 18.3333333333333
3000 0 3000 0 0 3000 3000 5000 3000 0 5000 3000 1500 -2500 -1500 -2500 1500 2500 -1500 2500 3 2 1 3 4 2
4 2 1 18.3333333333333
0 0 0 0 5000 0 0 0 3000 0 5000 3000 1500 2500 1500 -2500 -1500 2500 -1500 -2500 1 3 4 1 4 2
4 2 1 18.3333333333333
0 5000 0 3000 5000 0 0 5000 3000 3000 5000 3000 -1500 -1500 -1500 1500 1500 -1500 1500 1500 3 2 1 4 2 3

TShapes 35
Ve
0.1
0 0 0
0 0

0101101
*
Ve
0.1
0 -3000 0
0 0

0101101
*
Ed
 0.0001 1 1 0
1  1 0 0 3000
6  1 1 0
6  2 2 0
0

0101000
+35 3 -34 3 *
Ve
0.1
0 -3000 3000
0 0

0101101
*
Ed
 0.0001 1 1 0
1  2 0 0 3000
6  3 1 0
6  4 3 0
0

0101000
+34 3 -32 3 *
Ve
0.1
0 0 3000
0 0

0101101
*
Ed
 0.0001 1 1 0
1  3 0 0 3000
6  5 1 0
6  6 4 0
0

0101000
+32 3 -30 3 *
Ed
 0.0001 1 1 0
1  4 0 0 3000
6  7 1 0
6  8 5 0
0

0101000
+30 3 -35 3 *
Wi

0101100
+33 0 +31 0 +29 0 +28 0 *
Fa
0  0.1 1 0
2  1
0111000
+27 0 *
Ve
0.1
5000 0 0
0 0

0101101
*
Ed
 0.0001 1 1 0
1  5 0 0 5000
6  9 2 0
6  10 5 0
0

0101000
+35 3 -25 3 *
Ve
0.1
5000 -3000 0
0 0

0101101
*
Ed
 0.0001 1 1 0
1  6 0 0 3000
6  11 2 0
6  12 6 0
0

0101000
+25 3 -23 3 *
Ed
 0.0001 1 1 0
1  7 0 0 5000
6  13 2 0
6  14 3 0
0

0101000
+23 3 -34 3 *
Wi

0101100
+24 0 +22 0 +21 0 -33 0 *
Fa
0  0.1 2 0
2  2
0111000
+20 0 *
Ve
0.1
5000 -3000 3000
0 0

0101101
*
Ed
 0.0001 1 1 0
1  8 0 0 3000
6  15 3 0
6  16 6 0
0

0101000
+23 3 -18 3 *
Ed
 0.0001 1 1 0
1  9 0 0 5000
6  17 3 0
6  18 4 0
0

0101000
+18 3 -32 3 *
Wi

0101100
-21 0 +17 0 +16 0 -31 0 *
Fa
0  0.1 3 0
2  3
0111000
+15 0 *
Ve
0.1
5000 0 3000
0 0

0101101
*
Ed
 0.0001 1 1 0
1  10 0 0 3000
6  19 4 0
6  20 6 0
0

0101000
+18 3 -13 3 *
Ed
 0.0001 1 1 0
1  11 0 0 5000
6  21 4 0
6  22 5 0
0

0101000
+13 3 -30 3 *
Wi

0101100
-29 0 -16 0 +12 0 +11 0 *
Fa
0  0.1 4 0
2  4
0111000
+10 0 *
Ed
 0.0001 1 1 0
1  12 0 0 3000
6  23 5 0
6  24 6 0
0

0101000
+13 3 -25 3 *
Wi

0101100
-24 0 -28 0 -11 0 +8 0 *
Fa
0  0.1 5 0
2  5
0111000
+7 0 *
Wi

0101100
-22 0 -8 0 -12 0 -17 0 *
Fa
0  0.1 6 0
2  6
0111000
+5 0 *
Sh

0101100
+26 0 +19 0 +14 0 +9 0 +6 0 +4 0 *
So

0100000
+3 0 *
Co

1100000
+2 2 *

+1 1
"""

def like(a, b):
    return abs(a-b) < 0.001

def checkBB(a, b):
    return like(a.XMin, b.XMin) and like(a.YMin, b.YMin) and like(a.ZMin, b.ZMin) and like(a.XMax, b.XMax) and like(a.YMax, b.YMax) and like(a.ZMax, b.ZMax)


class ArchTest(unittest.TestCase):

    def setUp(self):
        # setting a new document to hold the tests
        if App.ActiveDocument:
            if App.ActiveDocument.Name != "ArchTest":
                App.newDocument("ArchTest")
        else:
            App.newDocument("ArchTest")
        App.setActiveDocument("ArchTest")

    def testWall(self):
        App.Console.PrintLog ('Checking Arch Wall...\n')
        l=Draft.makeLine(App.Vector(0,0,0),App.Vector(-2,0,0))
        w = Arch.makeWall(l)
        self.failUnless(w,"Arch Wall failed")

    def testWallMultiMatAlign(self):
        App.Console.PrintLog ('Checking Arch Wall with MultiMaterial and 3 alignments...\n')
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
                self.failUnless(ptMin.isEqual(checkLst[i][0], 1e-8),
                                "Arch Wall with MultiMaterial and 3 alignments failed")
                ptMax = App.Vector(box.XMax, box.YMax, 0)
                self.failUnless(ptMax.isEqual(checkLst[i][1], 1e-8),
                                "Arch Wall with MultiMaterial and 3 alignments failed")

    def testStructure(self):
        App.Console.PrintLog ('Checking Arch Structure...\n')
        s = Arch.makeStructure(length=2,width=3,height=5)
        self.failUnless(s,"Arch Structure failed")

    def testRebar(self):
        App.Console.PrintLog ('Checking Arch Rebar...\n')
        s = Arch.makeStructure(length=2,width=3,height=5)
        sk = App.ActiveDocument.addObject('Sketcher::SketchObject','Sketch')
        sk.Support = (s,["Face6"])
        sk.addGeometry(Part.LineSegment(App.Vector(-0.85,1.25,0),App.Vector(0.75,1.25,0)))
        sk.addGeometry(Part.LineSegment(App.Vector(0.75,1.25,0),App.Vector(0.75,-1.20,0)))
        sk.addGeometry(Part.LineSegment(App.Vector(0.75,-1.20,0),App.Vector(-0.85,-1.20,0)))
        sk.addGeometry(Part.LineSegment(App.Vector(-0.85,-1.20,0),App.Vector(-0.85,1.25,0)))
        sk.addConstraint(Sketcher.Constraint('Coincident',0,2,1,1))
        sk.addConstraint(Sketcher.Constraint('Coincident',1,2,2,1))
        sk.addConstraint(Sketcher.Constraint('Coincident',2,2,3,1))
        sk.addConstraint(Sketcher.Constraint('Coincident',3,2,0,1))
        r = Arch.makeRebar(s,sk,diameter=.1,amount=2)
        self.failUnless(r,"Arch Rebar failed")

    def testFloor(self):
        App.Console.PrintLog ('Checking Arch Floor...\n')
        s = Arch.makeStructure(length=2,width=3,height=5)
        f = Arch.makeFloor([s])
        self.failUnless(f,"Arch Floor failed")

    def testBuilding(self):
        App.Console.PrintLog ('Checking Arch Building...\n')
        s = Arch.makeStructure(length=2,width=3,height=5)
        f = Arch.makeFloor([s])
        b = Arch.makeBuilding([f])
        self.failUnless(b,"Arch Building failed")

    def testSite(self):
        App.Console.PrintLog ('Checking Arch Site...\n')
        s = Arch.makeStructure(length=2,width=3,height=5)
        f = Arch.makeFloor([s])
        b = Arch.makeBuilding([f])
        si = Arch.makeSite([b])
        self.failUnless(si,"Arch Site failed")

    def testWindow(self):
        operation = "Arch Window"
        _msg("  Test '{}'".format(operation))
        line = Draft.makeLine(App.Vector(0, 0, 0), App.Vector(3000, 0, 0))
        wall = Arch.makeWall(line)
        sk = App.ActiveDocument.addObject("Sketcher::SketchObject", "Sketch001")
        sk.Placement.Rotation = App.Rotation(App.Vector(1, 0, 0), 90)
        sk.addGeometry(Part.LineSegment(App.Vector( 500,  800, 0), App.Vector(1500,  800, 0)))
        sk.addGeometry(Part.LineSegment(App.Vector(1500,  800, 0), App.Vector(1500, 2000, 0)))
        sk.addGeometry(Part.LineSegment(App.Vector(1500, 2000, 0), App.Vector( 500, 2000, 0)))
        sk.addGeometry(Part.LineSegment(App.Vector( 500, 2000, 0), App.Vector( 500,  800, 0)))
        sk.addConstraint(Sketcher.Constraint('Coincident', 0, 2, 1, 1))
        sk.addConstraint(Sketcher.Constraint('Coincident', 1, 2, 2, 1))
        sk.addConstraint(Sketcher.Constraint('Coincident', 2, 2, 3, 1))
        sk.addConstraint(Sketcher.Constraint('Coincident', 3, 2, 0, 1))
        App.ActiveDocument.recompute()
        win = Arch.makeWindow(sk)
        Arch.removeComponents(win, host=wall)
        App.ActiveDocument.recompute()
        self.assertTrue(win, "'{}' failed".format(operation))

    def testRoof(self):
        App.Console.PrintLog ('Checking Arch Roof...\n')
        r = Draft.makeRectangle(length=2,height=-1)
        r.recompute() # required before calling Arch.makeRoof
        ro = Arch.makeRoof(r)
        self.failUnless(ro,"Arch Roof failed")

    def testRoof81Permutations(self):
        """Create 81 roofs using a range of arguments.
        """
        operation = "Arch Roof testRoof81Permutations"
        _msg("  Test '{}'".format(operation))
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
        _msg("  Test '{}'".format(operation))
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
        _msg("  Test '{}'".format(operation))
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
        _msg("  Test '{}'".format(operation))
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

    def testAxis(self):
        App.Console.PrintLog ('Checking Arch Axis...\n')
        a = Arch.makeAxis()
        self.failUnless(a,"Arch Axis failed")

    def testSection(self):
        App.Console.PrintLog ('Checking Arch Section...\n')
        s = Arch.makeSectionPlane([])
        self.failUnless(s,"Arch Section failed")

    def testSpace(self):
        App.Console.PrintLog ('Checking Arch Space...\n')
        sb = Part.makeBox(1,1,1)
        b = App.ActiveDocument.addObject('Part::Feature','Box')
        b.Shape = sb
        s = Arch.makeSpace([b])
        self.failUnless(s,"Arch Space failed")

    def testSpaceBBox(self):
        shape = Part.Shape()
        shape.importBrepFromString(brepArchiCAD)
        bborig = shape.BoundBox
        App.Console.PrintLog ("Original BB: "+str(bborig))
        baseobj = App.ActiveDocument.addObject("Part::Feature","brepArchiCAD_body")
        baseobj.Shape = shape
        space = Arch.makeSpace(baseobj)
        space.recompute()
        bbnew = space.Shape.BoundBox
        App.Console.PrintLog ("New BB: "+str(bbnew))
        self.failUnless(checkBB(bborig,bbnew),"Arch Space has wrong Placement")

    def testStairs(self):
        App.Console.PrintLog ('Checking Arch Stairs...\n')
        s = Arch.makeStairs()
        self.failUnless(s,"Arch Stairs failed")

    def testFrame(self):
        App.Console.PrintLog ('Checking Arch Frame...\n')
        l=Draft.makeLine(App.Vector(0,0,0),App.Vector(-2,0,0))
        p = Draft.makeRectangle(length=.5,height=.5)
        f = Arch.makeFrame(l,p)
        self.failUnless(f,"Arch Frame failed")

    def testEquipment(self):
        App.Console.PrintLog ('Checking Arch Equipment...\n')
        box = App.ActiveDocument.addObject("Part::Box", "Box")
        box.Length = 500
        box.Width = 2000
        box.Height = 600
        equip = Arch.makeEquipment(box)
        self.failUnless(equip,"Arch Equipment failed")

    def testPipe(self):
        App.Console.PrintLog ('Checking Arch Pipe...\n')
        pipe = Arch.makePipe(diameter=120, length=3000)
        self.failUnless(pipe,"Arch Pipe failed")

    def testAdd(self):
        App.Console.PrintLog ('Checking Arch Add...\n')
        l=Draft.makeLine(App.Vector(0,0,0),App.Vector(2,0,0))
        w = Arch.makeWall(l,width=0.2,height=2)
        sb = Part.makeBox(1,1,1)
        b = App.ActiveDocument.addObject('Part::Feature','Box')
        b.Shape = sb
        App.ActiveDocument.recompute()
        Arch.addComponents(b,w)
        App.ActiveDocument.recompute()
        r = (w.Shape.Volume > 1.5)
        self.failUnless(r,"Arch Add failed")

    def testRemove(self):
        App.Console.PrintLog ('Checking Arch Remove...\n')
        l=Draft.makeLine(App.Vector(0,0,0),App.Vector(2,0,0))
        w = Arch.makeWall(l,width=0.2,height=2,align="Right")
        sb = Part.makeBox(1,1,1)
        b = App.ActiveDocument.addObject('Part::Feature','Box')
        b.Shape = sb
        App.ActiveDocument.recompute()
        Arch.removeComponents(b,w)
        App.ActiveDocument.recompute()
        r = (w.Shape.Volume < 0.75)
        self.failUnless(r,"Arch Remove failed")

    def testBuildingPart(self):
        """Create a BuildingPart from a wall with a window and check its shape.
        """
        # Also regression test for:
        # https://github.com/FreeCAD/FreeCAD/issues/6178
        operation = "Arch BuildingPart"
        _msg("  Test '{}'".format(operation))
        # Most of the code below taken from testWindow function.
        line = Draft.makeLine(App.Vector(0, 0, 0), App.Vector(3000, 0, 0))
        wall = Arch.makeWall(line)
        sk = App.ActiveDocument.addObject("Sketcher::SketchObject", "Sketch001")
        sk.Placement.Rotation = App.Rotation(App.Vector(1, 0, 0), 90)
        sk.addGeometry(Part.LineSegment(App.Vector( 500,  800, 0), App.Vector(1500,  800, 0)))
        sk.addGeometry(Part.LineSegment(App.Vector(1500,  800, 0), App.Vector(1500, 2000, 0)))
        sk.addGeometry(Part.LineSegment(App.Vector(1500, 2000, 0), App.Vector( 500, 2000, 0)))
        sk.addGeometry(Part.LineSegment(App.Vector( 500, 2000, 0), App.Vector( 500,  800, 0)))
        sk.addConstraint(Sketcher.Constraint('Coincident', 0, 2, 1, 1))
        sk.addConstraint(Sketcher.Constraint('Coincident', 1, 2, 2, 1))
        sk.addConstraint(Sketcher.Constraint('Coincident', 2, 2, 3, 1))
        sk.addConstraint(Sketcher.Constraint('Coincident', 3, 2, 0, 1))
        App.ActiveDocument.recompute()
        win = Arch.makeWindow(sk)
        Arch.removeComponents(win, host=wall)
        App.ActiveDocument.recompute()
        bp = Arch.makeBuildingPart()
        bp.Group = [wall]
        App.ActiveDocument.recompute()
        self.assertTrue(len(bp.Shape.Faces) == 16, "'{}' failed".format(operation))

    def tearDown(self):
        App.closeDocument("ArchTest")
        pass
