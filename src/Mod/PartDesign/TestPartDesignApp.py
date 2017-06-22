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
        self.Doc = FreeCAD.newDocument("PartDesignTestPad")

    def testBoxCase(self):
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject','SketchPad')
        TestSketcherApp.CreateSlotPlateSet(self.PadSketch)
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad","Pad")
        self.Pad.Profile = self.PadSketch
        self.Doc.recompute()
        self.assertEqual(len(self.Pad.Shape.Faces), 6)

    def testPadToFirstCase(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        # Make first offset cube Pad
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad')
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0, 1), (1, 1))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 1
        self.Doc.recompute()
        # Make second pad on different plane and pad to first
        self.PadSketch1 = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad1')
        self.Body.addObject(self.PadSketch1)
        self.PadSketch1.MapMode = 'FlatFace'
        self.PadSketch1.Support = (App.ActiveDocument.XZ_Plane, [''])
        self.Doc.recompute()
        TestSketcherApp.CreateRectangleSketch(self.PadSketch1, (0, 0), (1, 1))
        self.Doc.recompute()
        self.Pad1 = self.Doc.addObject("PartDesign::Pad", "Pad1")
        self.Body.addObject(self.Pad1)
        self.Pad1.Profile = self.PadSketch1
        self.Pad1.Type = 2
        self.Pad1.Reversed = 1
        self.Doc.recompute()
        self.assertAlmostEqual(self.Pad1.Shape.Volume, 2.0)

    def testPadtoLastCase(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        # Make first offset cube Pad
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad')
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0.5, 1), (0.5, 2))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 1
        self.Doc.recompute()
        # Make second pad on different plane and pad to first
        self.PadSketch1 = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad1')
        self.Body.addObject(self.PadSketch1)
        self.PadSketch1.MapMode = 'FlatFace'
        self.PadSketch1.Support = (App.ActiveDocument.XZ_Plane, [''])
        self.Doc.recompute()
        TestSketcherApp.CreateRectangleSketch(self.PadSketch1, (0, 0), (1, 1))
        self.Doc.recompute()
        self.Pad1 = self.Doc.addObject("PartDesign::Pad", "Pad1")
        self.Body.addObject(self.Pad1)
        self.Pad1.Profile = self.PadSketch1
        self.Pad1.Type = 1
        self.Pad1.Reversed = 1
        self.Doc.recompute()
        self.assertAlmostEqual(self.Pad1.Shape.Volume, 3.0)

    def testPadToFaceCase(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        # Make first offset cube Pad
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad')
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0, 1), (1, 1))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 1
        self.Doc.recompute()
        # Make second pad on different plane and pad to face on first
        self.PadSketch1 = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad1')
        self.Body.addObject(self.PadSketch1)
        self.PadSketch1.MapMode = 'FlatFace'
        self.PadSketch1.Support = (App.ActiveDocument.XZ_Plane, [''])
        self.Doc.recompute()
        TestSketcherApp.CreateRectangleSketch(self.PadSketch1, (0, 0), (1, 1))
        self.Doc.recompute()
        self.Pad1 = self.Doc.addObject("PartDesign::Pad", "Pad1")
        self.Body.addObject(self.Pad1)
        self.Pad1.Profile = self.PadSketch1
        self.Pad1.Type = 3
        self.Pad1.UpToFace = (self.Pad, ["Face3"])
        self.Pad1.Reversed = 1
        self.Doc.recompute()
        self.assertAlmostEqual(self.Pad1.Shape.Volume, 2.0)

    def testPadTwoDimensionsCase(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        # Make first offset cube Pad
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad')
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0, 1), (1, 1))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 1
        self.Doc.recompute()
        # Make second pad on different plane and pad to face on first
        self.PadSketch1 = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad1')
        self.Body.addObject(self.PadSketch1)
        self.PadSketch1.MapMode = 'FlatFace'
        self.PadSketch1.Support = (App.ActiveDocument.XZ_Plane, [''])
        self.Doc.recompute()
        TestSketcherApp.CreateRectangleSketch(self.PadSketch1, (0, 0), (1, 1))
        self.Doc.recompute()
        self.Pad1 = self.Doc.addObject("PartDesign::Pad", "Pad1")
        self.Body.addObject(self.Pad1)
        self.Pad1.Profile = self.PadSketch1
        self.Pad1.Type = 4
        self.Pad1.Length = 2.0
        self.Pad1.Length2 = 1.0
        self.Pad1.Reversed = 1
        self.Doc.recompute()
        self.assertAlmostEqual(self.Pad1.Shape.Volume, 4.0)

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartDesignTestPad")
        #print ("omit closing document for debugging")

class PartDesignRevolveTestCases(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestRevolve")

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
        # depending on if refinement is done we expect 8 or 10 faces
        self.assertIn(len(self.Revolution.Shape.Faces), (8, 10))

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
        self.assertEqual(len(self.Groove.Shape.Faces), 5)

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartDesignTestRevolve")
        # print ("omit closing document for debugging")

class PartDesignMirroredTestCases(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestMirrored")

    def testMirroredSketchCase(self):
        """
        Creates a unit cube cornered at the origin and mirrors it about the Y axis.
        This operation should create a rectangular prism with volume 2.0.
        """
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.Rect = self.Doc.addObject('Sketcher::SketchObject','Rect')
        self.Body.addObject(self.Rect)
        TestSketcherApp.CreateRectangleSketch(self.Rect, (0, 0), (1, 1))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad","Pad")
        self.Pad.Profile = self.Rect
        self.Pad.Length = 1
        self.Body.addObject(self.Pad)
        self.Doc.recompute()
        self.Mirrored = self.Doc.addObject("PartDesign::Mirrored","Mirrored")
        self.Mirrored.Originals = [self.Pad]
        self.Mirrored.MirrorPlane = (self.Rect, ["V_Axis"])
        self.Body.addObject(self.Mirrored)
        self.Doc.recompute()
        self.assertAlmostEqual(self.Mirrored.Shape.Volume, 2.0)

    def testMirroredPrimitiveCase(self):
        """
        Tests the same mirroring scenario as in the sketch case,
        but is designed to ensure the same end result occurs with
        a different base object.
        """
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.Box = self.Doc.addObject('PartDesign::AdditiveBox','Box')
        self.Box.Length=1
        self.Box.Width=1
        self.Box.Height=1
        self.Body.addObject(self.Box)
        self.Doc.recompute()
        self.Mirrored = self.Doc.addObject("PartDesign::Mirrored", "Mirrored")
        self.Mirrored.Originals = [self.Box]
        self.Mirrored.MirrorPlane = (self.Doc.XY_Plane, [""])
        self.Body.addObject(self.Mirrored)
        self.Doc.recompute()
        self.assertAlmostEqual(self.Mirrored.Shape.Volume, 2.0)

    def testMirroredOffsetFailureCase(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.Rect = self.Doc.addObject('Sketcher::SketchObject','Rect')
        self.Body.addObject(self.Rect)
        TestSketcherApp.CreateRectangleSketch(self.Rect, (0, 1), (1, 1))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad","Pad")
        self.Pad.Profile = self.Rect
        self.Pad.Length = 1
        self.Body.addObject(self.Pad)
        self.Doc.recompute()
        self.Mirrored = self.Doc.addObject("PartDesign::Mirrored","Mirrored")
        self.Mirrored.Originals = [self.Pad]
        self.Mirrored.MirrorPlane = (self.Rect, ["H_Axis"])
        self.Body.addObject(self.Mirrored)
        self.Doc.recompute()
        self.assertEqual(self.Mirrored.State, ["Invalid"])

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartDesignTestMirrored")
        #print ("omit closing document for debugging")

class PartDesignPocketTestCases(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestPocket")

    def testPocketDimensionCase(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject', 'PadSketch')
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0, 0), (10, 10))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 1
        self.Pad.Reversed = 1
        self.Doc.recompute()
        self.PocketSketch = self.Doc.addObject('Sketcher::SketchObject', 'PocketSketch')
        self.Body.addObject(self.PocketSketch)
        TestSketcherApp.CreateRectangleSketch(self.PocketSketch, (2.5, 2.5), (5, 5))
        self.Doc.recompute()
        self.Pocket = self.Doc.addObject("PartDesign::Pocket", "Pocket")
        self.Body.addObject(self.Pocket)
        self.Pocket.Profile = self.PocketSketch
        self.Pocket.Length = 1
        self.Doc.recompute()
        self.assertAlmostEqual(self.Pocket.Shape.Volume, 75.0)

    def testPocketThroughAllCase(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject', 'PadSketch')
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0, 0), (10, 10))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 1
        self.Pad.Reversed = 1
        self.Doc.recompute()
        self.PocketSketch = self.Doc.addObject('Sketcher::SketchObject', 'PocketSketch')
        self.Body.addObject(self.PocketSketch)
        TestSketcherApp.CreateRectangleSketch(self.PocketSketch, (2.5, 2.5), (5, 5))
        self.Doc.recompute()
        self.Pocket = self.Doc.addObject("PartDesign::Pocket", "Pocket")
        self.Body.addObject(self.Pocket)
        self.Pocket.Profile = self.PocketSketch
        self.Pocket.Length = 1
        self.Doc.recompute()
        self.PocketSketch1 = self.Doc.addObject('Sketcher::SketchObject', 'PocketSketch')
        self.Body.addObject(self.PocketSketch1)
        self.PocketSketch1.MapMode = 'FlatFace'
        self.PocketSketch1.Support = (App.ActiveDocument.XZ_Plane, [''])
        self.Doc.recompute()
        TestSketcherApp.CreateRectangleSketch(self.PocketSketch1, (2.5, -1), (5, 1))
        self.Doc.recompute()
        self.Pocket001 = self.Doc.addObject("PartDesign::Pocket", "Pocket001")
        self.Body.addObject(self.Pocket001)
        self.Pocket001.Profile = self.PocketSketch1
        self.Pocket001.Type = 1
        self.Doc.recompute()
        self.assertAlmostEqual(self.Pocket001.Shape.Volume, 25.0)

    def testPocketToFirstCase(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject', 'PadSketch')
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0, 0), (10, 10))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 1
        self.Pad.Reversed = 1
        self.Doc.recompute()
        self.PocketSketch = self.Doc.addObject('Sketcher::SketchObject', 'PocketSketch')
        self.Body.addObject(self.PocketSketch)
        TestSketcherApp.CreateRectangleSketch(self.PocketSketch, (2.5, 2.5), (5, 5))
        self.Doc.recompute()
        self.Pocket = self.Doc.addObject("PartDesign::Pocket", "Pocket")
        self.Body.addObject(self.Pocket)
        self.Pocket.Profile = self.PocketSketch
        self.Pocket.Length = 1
        self.Doc.recompute()
        self.PocketSketch1 = self.Doc.addObject('Sketcher::SketchObject', 'PocketSketch')
        self.Body.addObject(self.PocketSketch1)
        self.PocketSketch1.MapMode = 'FlatFace'
        self.PocketSketch1.Support = (App.ActiveDocument.XZ_Plane, [''])
        self.Doc.recompute()
        TestSketcherApp.CreateRectangleSketch(self.PocketSketch1, (2.5, -1), (5, 1))
        self.Doc.recompute()
        self.Pocket001 = self.Doc.addObject("PartDesign::Pocket", "Pocket001")
        self.Body.addObject(self.Pocket001)
        self.Pocket001.Profile = self.PocketSketch1
        self.Pocket001.Type = 2
        self.Doc.recompute()
        self.assertAlmostEqual(self.Pocket001.Shape.Volume, 62.5)

    def testPocketToFaceCase(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject', 'PadSketch')
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0, 0), (10, 10))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 1
        self.Pad.Reversed = 1
        self.Doc.recompute()
        self.PocketSketch = self.Doc.addObject('Sketcher::SketchObject', 'PocketSketch')
        self.Body.addObject(self.PocketSketch)
        TestSketcherApp.CreateRectangleSketch(self.PocketSketch, (2.5, 2.5), (5, 5))
        self.Doc.recompute()
        self.Pocket = self.Doc.addObject("PartDesign::Pocket", "Pocket")
        self.Body.addObject(self.Pocket)
        self.Pocket.Profile = self.PocketSketch
        self.Pocket.Length = 1
        self.Doc.recompute()
        self.PocketSketch1 = self.Doc.addObject('Sketcher::SketchObject', 'PocketSketch')
        self.Body.addObject(self.PocketSketch1)
        self.PocketSketch1.MapMode = 'FlatFace'
        self.PocketSketch1.Support = (App.ActiveDocument.XZ_Plane, [''])
        self.Doc.recompute()
        TestSketcherApp.CreateRectangleSketch(self.PocketSketch1, (0, -1), (10, 1))
        self.Doc.recompute()
        self.Pocket001 = self.Doc.addObject("PartDesign::Pocket", "Pocket001")
        self.Body.addObject(self.Pocket001)
        self.Pocket001.Profile = self.PocketSketch1
        self.Pocket001.Type = 3
        # Handle face-naming inconsistency in OCC < 7
        self.FaceNumber = 7
        self.Pocket001.UpToFace = (self.Pocket, ["Face"+str(self.FaceNumber)])
        self.Doc.recompute()
        while (('Invalid' in self.Pocket001.State or round(self.Pocket001.Shape.Volume, 7) != 50.0) and self.FaceNumber < 11):
            self.FaceNumber += 1
            self.Pocket001.UpToFace = (self.Pocket, ["Face"+str(self.FaceNumber)])
            self.Doc.recompute()
        self.assertAlmostEqual(self.Pocket001.Shape.Volume, 50.0)

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartDesignTestPocket")
        #print ("omit closing document for debugging")
