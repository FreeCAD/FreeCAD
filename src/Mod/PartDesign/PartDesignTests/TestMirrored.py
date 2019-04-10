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
import unittest

import FreeCAD
import TestSketcherApp

class TestMirrored(unittest.TestCase):
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
        self.Body.SingleSolid = True
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
        self.assertIn("Invalid", self.Mirrored.State)

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartDesignTestMirrored")
        #print ("omit closing document for debugging")
