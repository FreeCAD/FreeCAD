#   (c) Juergen Riegel (FreeCAD@juergen-riegel.net) 2011      LGPL        *
#                                                                         *
#   This file is part of the FreeCAD CAx development system.              *
#                                                                         *
#   This program is free software; you can redistribute it and/or modify  *
#   it under the terms of the GNU Lesser General Public License (LGPL)    *
#   as published by the Free Software Foundation; either version 2 of     *
#   the License, or (at your option) any later version.                   *
#   for detail see the LICENCE text file.                                 *
#                                                                         *
#   FreeCAD is distributed in the hope that it will be useful,            *
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#   GNU Library General Public License for more details.                  *
#                                                                         *
#   You should have received a copy of the GNU Library General Public     *
#   License along with FreeCAD; if not, write to the Free Software        *
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#   USA                                                                   *
#**************************************************************************
import os
import sys
import unittest

import FreeCAD
import Part
import Sketcher
import PartDesign
import TestSketcherApp

App = FreeCAD
#---------------------------------------------------------------------------
# define the test cases to test the FreeCAD PartDesign module
#---------------------------------------------------------------------------


class PartDesignPadTestCases(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTest")

    def testBoxCase(self):
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject','SketchPad')
        TestSketcherApp.CreateSlotPlateSet(self.PadSketch)
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad","Pad")
        self.Pad.Profile = self.PadSketch
        self.Doc.recompute()
        self.failUnless(len(self.Pad.Shape.Faces) == 6)

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartDesignTest")
        # print ("omit closing document for debugging")

class PartDesignRevolveTestCases(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTest")

    def testRevolveFace(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.Box = self.Doc.addObject('PartDesign::AdditiveBox','Box')
        self.Body.addObject(self.Box)
        self.Box.Length=10.00
        self.Box.Width=10.00
        self.Box.Height=10.00
        self.Doc.recompute()
        self.Revolution = self.Doc.addObject("PartDesign::Revolution","Revolution")
        self.Revolution.Profile = (self.Box, ["Face6"])
        self.Revolution.ReferenceAxis = (self.Doc.Y_Axis,[""])
        self.Revolution.Angle = 180.0
        self.Revolution.Reversed = 1
        self.Body.addObject(self.Revolution)
        self.Doc.recompute()
        self.failUnless(len(self.Revolution.Shape.Faces) == 10)

    def testGrooveFace(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.Box = self.Doc.addObject('PartDesign::AdditiveBox','Box')
        self.Body.addObject(self.Box)
        self.Box.Length=10.00
        self.Box.Width=10.00
        self.Box.Height=10.00
        self.Doc.recompute()
        self.Groove = self.Doc.addObject("PartDesign::Groove","Groove")
        self.Groove.Profile = (self.Box, ["Face6"])
        self.Groove.ReferenceAxis = (self.Doc.X_Axis,[""])
        self.Groove.Angle = 180.0
        self.Groove.Reversed = 1
        self.Body.addObject(self.Groove)
        self.Doc.recompute()
        self.failUnless(len(self.Groove.Shape.Faces) == 5)

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartDesignTest")
        # print ("omit closing document for debugging")

class PartDesignMirroredTestCases(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTest")

    def testMirroredCase(self):
        """
        Creates a unit cube at the origin and mirrors it about the Y axis.
        This operation should create a rectangular prism with volume 2.

        The operation is currently broken; this test is inverted:
            self.failUnless(self.Mirrored.Shape.Volume < 2.0)
        Change the final line to " .. > 1.0" and remove this notice.
        """
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.Rect = self.Doc.addObject('Sketcher::SketchObject','Rect')
        try:
            self.Body.addObject(self.Rect)
        except AttributeError:
            pass
        geoList = []
        try:
            geoList.append(Part.LineSegment(App.Vector(0, 0, 0), App.Vector(1, 0, 0)))
            geoList.append(Part.LineSegment(App.Vector(1, 0, 0), App.Vector(1, 1, 0)))
            geoList.append(Part.LineSegment(App.Vector(1, 1, 0), App.Vector(0, 1, 0)))
            geoList.append(Part.LineSegment(App.Vector(0, 1, 0), App.Vector(0, 0, 0)))
        except AttributeError:
            geoList.append(Part.Line(App.Vector(0, 0, 0), App.Vector(1, 0, 0)))
            geoList.append(Part.Line(App.Vector(1, 0, 0), App.Vector(1, 1, 0)))
            geoList.append(Part.Line(App.Vector(1, 1, 0), App.Vector(0, 1, 0)))
            geoList.append(Part.Line(App.Vector(0, 1, 0), App.Vector(0, 0, 0)))
        self.Rect.addGeometry(geoList,False)
        conList = []
        conList.append(Sketcher.Constraint('Coincident',0,2,1,1))
        conList.append(Sketcher.Constraint('Coincident',1,2,2,1))
        conList.append(Sketcher.Constraint('Coincident',2,2,3,1))
        conList.append(Sketcher.Constraint('Coincident',3,2,0,1))
        conList.append(Sketcher.Constraint('Horizontal',0))
        conList.append(Sketcher.Constraint('Horizontal',2))
        conList.append(Sketcher.Constraint('Vertical',1))
        conList.append(Sketcher.Constraint('Vertical',3))
        self.Rect.addConstraint(conList)
        self.Rect.addConstraint(Sketcher.Constraint('Coincident',0,1,-1,1))
        self.Rect.addConstraint(Sketcher.Constraint('DistanceX',0,1,0,2,1))
        self.Rect.setDatum(9,App.Units.Quantity('1.0 mm'))
        self.Rect.addConstraint(Sketcher.Constraint('DistanceY',3,2,3,1,1))
        self.Rect.setDatum(10,App.Units.Quantity('1.0 mm'))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad","Pad")
        try:
            self.Pad.Profile = self.Rect
        except AttributeError:
            self.Pad.Sketch = self.Rect
        self.Pad.Length = 1
        try:
            self.Body.addObject(self.Pad)
        except AttributeError:
            pass
        self.Doc.recompute()
        self.Mirrored = self.Doc.addObject("PartDesign::Mirrored","Mirrored")
        self.Mirrored.Originals = [self.Pad]
        self.Mirrored.MirrorPlane = (self.Rect, ["V_Axis"])
        try:
            self.Body.addObject(self.Mirrored)
        except AttributeError:
            pass
        self.Doc.recompute()
        self.failUnless(self.Mirrored.Shape.Volume < 2.0)

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartDesignTest")
        # print ("omit closing document for debugging")
