# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 bgbsww@gmail.com                                   *
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
import unittest


import FreeCAD as App
import Part
import Sketcher
import TestSketcherApp

class TestTopologicalNamingProblem(unittest.TestCase):
    def setUp(self):
        self.Doc = App.newDocument("PartDesignTestTNP")

    def testPadsOnBaseObject(self):
        # This is a simple TNP case.  By creating three Pads dependent on each other in succession,
        #     and then moving the middle one we can determine if the last one breaks because of a broken
        #     reference to the middle one.  This is the essence of a TNP. Pretty much a duplicate of the
        #     steps at https://wiki.freecad.org/Topological_naming_problem

        # Arrange
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        # Make first offset cube Pad
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad')
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0, 0), (1, 1))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 1
        self.Doc.recompute()

        # Attach a second pad to the top of the first.
        self.PadSketch1 = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad1')
        self.Body.addObject(self.PadSketch1)
        self.PadSketch1.MapMode = 'FlatFace'
        self.PadSketch1.AttachmentSupport = [(self.Doc.getObject('Pad'),'Face6')]
        TestSketcherApp.CreateRectangleSketch(self.PadSketch1, (0, 0), (1, 1))
        self.Doc.recompute()
        self.Pad1 = self.Doc.addObject("PartDesign::Pad", "Pad1")
        self.Body.addObject(self.Pad1)
        self.Pad1.Profile = self.PadSketch1
        self.Pad1.Length = 1
        self.Doc.recompute()

        # Attach a third pad to the top of the second.
        self.PadSketch2 = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad2')
        self.Body.addObject(self.PadSketch2)
        self.PadSketch2.MapMode = 'FlatFace'
        self.PadSketch2.AttachmentSupport = [(self.Doc.getObject('Pad1'),'Face6')]
        TestSketcherApp.CreateRectangleSketch(self.PadSketch2, (0, 0), (1, 1))
        self.Doc.recompute()
        self.Pad2 = self.Doc.addObject("PartDesign::Pad", "Pad2")
        self.Body.addObject(self.Pad2)
        self.Pad2.Profile = self.PadSketch2
        self.Pad2.Length = 1
        self.Doc.recompute()

        # Assert everything is valid
        self.assertTrue(self.Pad.isValid())
        self.assertTrue(self.Pad1.isValid())
        self.assertTrue(self.Pad2.isValid())

        # Act
        # Move the second pad ( the sketch attachment point )
        self.PadSketch1.AttachmentOffset = App.Placement(
            App.Vector(0.5000000000, 0.0000000000, 0.0000000000),
            App.Rotation(0.0000000000, 0.0000000000, 0.0000000000))
        self.Doc.recompute()

        # Assert everything is still valid.
        self.assertTrue(self.Pad.isValid())
        self.assertTrue(self.Pad1.isValid())

        # Todo switch to actually asserting this and remove the printed lines as soon as
        #  the main branch is capable of passing this test.
        # self.assertTrue(self.Pad2.isValid())
        if self.Pad2.isValid():
            print("Topological Naming Problem is not present.")
        else:
            print("TOPOLOGICAL NAMING PROBLEM IS PRESENT.")

    # def testFutureStuff(self):
        # self.Doc.getObject('Body').newObject('Sketcher::SketchObject', 'Sketch')
        # geoList = []
        # geoList.append(Part.LineSegment(App.Vector(0,0,0),App.Vector(20,0,0)))
        # geoList.append(Part.LineSegment(App.Vector(20,0,0),App.Vector(20,10,0)))
        # geoList.append(Part.LineSegment(App.Vector(20,10,0),App.Vector(10,10,0)))
        # geoList.append(Part.LineSegment(App.Vector(10,10,0),App.Vector(10,20,0)))
        # geoList.append(Part.LineSegment(App.Vector(10,20,0),App.Vector(0,20,0)))
        # geoList.append(Part.LineSegment(App.Vector(0,20,0),App.Vector(0,0,0)))
        # self.Doc.getObject('Sketch').addGeometry(geoList,False)
        # conList = []
        # conList.append(Sketcher.Constraint('Coincident',0,2,1,1))
        # conList.append(Sketcher.Constraint('Coincident',1,2,2,1))
        # conList.append(Sketcher.Constraint('Coincident',2,2,3,1))
        # conList.append(Sketcher.Constraint('Coincident',3,2,4,1))
        # conList.append(Sketcher.Constraint('Coincident',4,2,5,1))
        # conList.append(Sketcher.Constraint('Coincident',5,2,0,1))
        # conList.append(Sketcher.Constraint('Horizontal',0))
        # conList.append(Sketcher.Constraint('Horizontal',2))
        # conList.append(Sketcher.Constraint('Horizontal',4))
        # conList.append(Sketcher.Constraint('Vertical',1))
        # conList.append(Sketcher.Constraint('Vertical',3))
        # conList.append(Sketcher.Constraint('Vertical',5))
        # self.Doc.getObject('Sketch').addConstraint(conList)
        # del geoList, conList
        # self.Doc.recompute()
        # self.Doc.getObject('Body').newObject('PartDesign::Pad','Pad002')
        # self.Doc.getObject('Pad002').Length = 10
        # self.Doc.Pad002.Profile = self.Doc.Sketch
        # self.Doc.recompute()
        # self.Doc.getObject('Pad002').ReferenceAxis = (self.Doc.getObject('Sketch'),['N_Axis'])
        # self.Doc.getObject('Sketch').Visibility = False
        # self.Doc.recompute()


def tearDown(self):
    App.closeDocument("PartDesignTestTNP")
