# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2013 Yorik van Havre <yorik@uncreated.net>              *
# *   Copyright (c) 2025 Furgo                                              *
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

import os
import FreeCAD as App
import Arch
import Draft
import Part
import Sketcher
import Arch
from bimtests import TestArchBase
from draftutils.messages import _msg

# TODO: move these tests to their relevant modules, remove this file
class TestArch(TestArchBase.TestArchBase):

    def testStructure(self):
        App.Console.PrintLog ('Checking BIM Structure...\n')
        s = Arch.makeStructure(length=2,width=3,height=5)
        self.assertTrue(s,"BIM Structure failed")

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

    def testAxis(self):
        App.Console.PrintLog ('Checking Arch Axis...\n')
        a = Arch.makeAxis()
        self.assertTrue(a,"Arch Axis failed")

    def testSection(self):
        App.Console.PrintLog ('Checking Arch Section...\n')
        s = Arch.makeSectionPlane([])
        self.assertTrue(s,"Arch Section failed")

    def testStairs(self):
        App.Console.PrintLog ('Checking Arch Stairs...\n')
        s = Arch.makeStairs()
        self.assertTrue(s,"Arch Stairs failed")

    def testFrame(self):
        App.Console.PrintLog ('Checking Arch Frame...\n')
        l=Draft.makeLine(App.Vector(0,0,0),App.Vector(-2,0,0))
        p = Draft.makeRectangle(length=.5,height=.5)
        f = Arch.makeFrame(l,p)
        self.assertTrue(f,"Arch Frame failed")

    def testEquipment(self):
        App.Console.PrintLog ('Checking Arch Equipment...\n')
        box = App.ActiveDocument.addObject("Part::Box", "Box")
        box.Length = 500
        box.Width = 2000
        box.Height = 600
        equip = Arch.makeEquipment(box)
        self.assertTrue(equip,"Arch Equipment failed")

    def testPipe(self):
        App.Console.PrintLog ('Checking Arch Pipe...\n')
        pipe = Arch.makePipe(diameter=120, length=3000)
        self.assertTrue(pipe,"Arch Pipe failed")

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
        self.assertTrue(r,"Arch Add failed")

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
        self.assertTrue(r,"Arch Remove failed")

    def testViewGeneration(self):
        """Tests the whole TD view generation workflow"""

        operation = "View generation"
        _msg("  Test '{}'".format(operation))

        # Create a few objects
        points = [App.Vector(0.0, 0.0, 0.0), App.Vector(2000.0, 0.0, 0.0)]
        line = Draft.make_wire(points)
        wall = Arch.makeWall(line, height=2000)
        wpl = App.Placement(App.Vector(500,0,1500), App.Vector(1,0,0),-90)
        win = Arch.makeWindowPreset('Fixed', width=1000.0, height=1000.0, h1=50.0, h2=50.0, h3=50.0, w1=100.0, w2=50.0, o1=0.0, o2=50.0, placement=wpl)
        win.Hosts = [wall]
        profile = Arch.makeProfile([169, 'HEA', 'HEA100', 'H', 100.0, 96.0, 5.0, 8.0])
        column = Arch.makeStructure(profile, height=2000.0)
        column.Profile = "HEA100"
        column.Placement.Base = App.Vector(500.0, 600.0, 0.0)
        level = Arch.makeFloor()
        level.addObjects([wall, column])
        App.ActiveDocument.recompute()

        # Create a drawing view
        section = Arch.makeSectionPlane(level)
        drawing = Arch.make2DDrawing()
        view = Draft.make_shape2dview(section)
        cut = Draft.make_shape2dview(section)
        cut.InPlace = False
        cut.ProjectionMode = "Cutfaces"
        drawing.addObjects([view, cut])
        App.ActiveDocument.recompute()

        # Create a TD page
        tpath = os.path.join(App.getResourceDir(),"Mod","TechDraw","Templates","A3_Landscape_blank.svg")
        page = App.ActiveDocument.addObject("TechDraw::DrawPage", "Page")
        template = App.ActiveDocument.addObject("TechDraw::DrawSVGTemplate", "Template")
        template.Template = tpath
        page.Template = template
        view = App.ActiveDocument.addObject("TechDraw::DrawViewDraft", "DraftView")
        view.Source = drawing
        page.addView(view)
        view.Scale = 1.0
        view.X = "20cm"
        view.Y = "15cm"
        App.ActiveDocument.recompute()
        assert True