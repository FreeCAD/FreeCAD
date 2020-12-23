# Unit test for the Arch module

#***************************************************************************
#*   (c) Yorik van Havre <yorik@uncreated.net> 2013                        *
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

import FreeCAD, os, unittest, FreeCADGui, Arch, Draft, Part, Sketcher

class ArchTest(unittest.TestCase):

    def setUp(self):
        # setting a new document to hold the tests
        if FreeCAD.ActiveDocument:
            if FreeCAD.ActiveDocument.Name != "ArchTest":
                FreeCAD.newDocument("ArchTest")
        else:
            FreeCAD.newDocument("ArchTest")
        FreeCAD.setActiveDocument("ArchTest")

    def testWall(self):
        FreeCAD.Console.PrintLog ('Checking Arch Wall...\n')
        l=Draft.makeLine(FreeCAD.Vector(0,0,0),FreeCAD.Vector(-2,0,0))
        w = Arch.makeWall(l)
        self.failUnless(w,"Arch Wall failed")

    def testStructure(self):
        FreeCAD.Console.PrintLog ('Checking Arch Structure...\n')
        s = Arch.makeStructure(length=2,width=3,height=5)
        self.failUnless(s,"Arch Structure failed")

    def testRebar(self):
        FreeCAD.Console.PrintLog ('Checking Arch Rebar...\n')
        s = Arch.makeStructure(length=2,width=3,height=5)
        sk = FreeCAD.ActiveDocument.addObject('Sketcher::SketchObject','Sketch')
        sk.Support = (s,["Face6"])
        sk.addGeometry(Part.LineSegment(FreeCAD.Vector(-0.85,1.25,0),FreeCAD.Vector(0.75,1.25,0)))
        sk.addGeometry(Part.LineSegment(FreeCAD.Vector(0.75,1.25,0),FreeCAD.Vector(0.75,-1.20,0)))
        sk.addGeometry(Part.LineSegment(FreeCAD.Vector(0.75,-1.20,0),FreeCAD.Vector(-0.85,-1.20,0)))
        sk.addGeometry(Part.LineSegment(FreeCAD.Vector(-0.85,-1.20,0),FreeCAD.Vector(-0.85,1.25,0)))
        sk.addConstraint(Sketcher.Constraint('Coincident',0,2,1,1)) 
        sk.addConstraint(Sketcher.Constraint('Coincident',1,2,2,1)) 
        sk.addConstraint(Sketcher.Constraint('Coincident',2,2,3,1)) 
        sk.addConstraint(Sketcher.Constraint('Coincident',3,2,0,1))
        r = Arch.makeRebar(s,sk,diameter=.1,amount=2)
        self.failUnless(r,"Arch Rebar failed")

    def testFloor(self):
        FreeCAD.Console.PrintLog ('Checking Arch Floor...\n')
        s = Arch.makeStructure(length=2,width=3,height=5)
        f = Arch.makeFloor([s])
        self.failUnless(f,"Arch Floor failed")

    def testBuilding(self):
        FreeCAD.Console.PrintLog ('Checking Arch Building...\n')
        s = Arch.makeStructure(length=2,width=3,height=5)
        f = Arch.makeFloor([s])
        b = Arch.makeBuilding([f])
        self.failUnless(b,"Arch Building failed")

    def testSite(self):
        FreeCAD.Console.PrintLog ('Checking Arch Site...\n')
        s = Arch.makeStructure(length=2,width=3,height=5)
        f = Arch.makeFloor([s])
        b = Arch.makeBuilding([f])
        si = Arch.makeSite([b])
        self.failUnless(si,"Arch Site failed")

    def testWindow(self):
        FreeCAD.Console.PrintLog ('Checking Arch Window...\n')
        l=Draft.makeLine(FreeCAD.Vector(0,0,0),FreeCAD.Vector(-2,0,0))
        w = Arch.makeWall(l)
        sk = FreeCAD.ActiveDocument.addObject('Sketcher::SketchObject','Sketch001')
        sk.Support = (w,["Face3"])
        sk.addGeometry(Part.LineSegment(FreeCAD.Vector(-1.80,1.30,0),FreeCAD.Vector(-0.90,1.30,0)))
        sk.addGeometry(Part.LineSegment(FreeCAD.Vector(-0.90,1.30,0),FreeCAD.Vector(-0.90,0.25,0)))
        sk.addGeometry(Part.LineSegment(FreeCAD.Vector(-0.90,0.25,0),FreeCAD.Vector(-1.80,0.25,0)))
        sk.addGeometry(Part.LineSegment(FreeCAD.Vector(-1.80,0.25,0),FreeCAD.Vector(-1.80,1.30,0)))
        sk.addConstraint(Sketcher.Constraint('Coincident',0,2,1,1)) 
        sk.addConstraint(Sketcher.Constraint('Coincident',1,2,2,1)) 
        sk.addConstraint(Sketcher.Constraint('Coincident',2,2,3,1)) 
        sk.addConstraint(Sketcher.Constraint('Coincident',3,2,0,1)) 
        win = Arch.makeWindow(sk)
        Arch.removeComponents(win,host=w)
        self.failUnless(win,"Arch Window failed")

    def testRoof(self):
        FreeCAD.Console.PrintLog ('Checking Arch Roof...\n')
        r = Draft.makeRectangle(length=2,height=-1)
        ro = Arch.makeRoof(r)
        self.failUnless(ro,"Arch Roof failed")

    def testAxis(self):
        FreeCAD.Console.PrintLog ('Checking Arch Axis...\n')
        a = Arch.makeAxis()
        self.failUnless(a,"Arch Axis failed") 

    def testSection(self):
        FreeCAD.Console.PrintLog ('Checking Arch Section...\n')
        s = Arch.makeSectionPlane([])
        v = Arch.makeSectionView(s)
        self.failUnless(v,"Arch Section failed")

    def testSpace(self):
        FreeCAD.Console.PrintLog ('Checking Arch Space...\n')
        sb = Part.makeBox(1,1,1)
        b = FreeCAD.ActiveDocument.addObject('Part::Feature','Box')
        b.Shape = sb
        s = Arch.makeSpace([b])
        self.failUnless(s,"Arch Space failed")

    def testStairs(self):
        FreeCAD.Console.PrintLog ('Checking Arch Stairs...\n')
        s = Arch.makeStairs()
        self.failUnless(s,"Arch Stairs failed")

    def testFrame(self):
        FreeCAD.Console.PrintLog ('Checking Arch Frame...\n')
        l=Draft.makeLine(FreeCAD.Vector(0,0,0),FreeCAD.Vector(-2,0,0))
        p = Draft.makeRectangle(length=.5,height=.5)
        f = Arch.makeFrame(l,p)
        self.failUnless(f,"Arch Frame failed")

    def testAdd(self):
        FreeCAD.Console.PrintLog ('Checking Arch Add...\n')
        l=Draft.makeLine(FreeCAD.Vector(0,0,0),FreeCAD.Vector(2,0,0))
        w = Arch.makeWall(l,width=0.2,height=2)
        sb = Part.makeBox(1,1,1)
        b = FreeCAD.ActiveDocument.addObject('Part::Feature','Box')
        b.Shape = sb
        FreeCAD.ActiveDocument.recompute()
        Arch.addComponents(b,w)
        FreeCAD.ActiveDocument.recompute()
        r = (w.Shape.Volume > 1.5)
        self.failUnless(r,"Arch Add failed")

    def testRemove(self):
        FreeCAD.Console.PrintLog ('Checking Arch Remove...\n')
        l=Draft.makeLine(FreeCAD.Vector(0,0,0),FreeCAD.Vector(2,0,0))
        w = Arch.makeWall(l,width=0.2,height=2)
        sb = Part.makeBox(1,1,1)
        b = FreeCAD.ActiveDocument.addObject('Part::Feature','Box')
        b.Shape = sb
        FreeCAD.ActiveDocument.recompute()
        Arch.removeComponents(b,w)
        FreeCAD.ActiveDocument.recompute()
        r = (w.Shape.Volume < 0.75)
        self.failUnless(r,"Arch Remove failed")

    def tearDown(self):
        FreeCAD.closeDocument("ArchTest")
        pass

