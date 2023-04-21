#***************************************************************************
#*   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

import unittest

import FreeCAD
import TestSketcherApp

class TestPolarPattern(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestPolarPattern")

    def testXAxisPolarPattern(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.Box = self.Doc.addObject('PartDesign::AdditiveBox','Box')
        self.Body.addObject(self.Box)
        self.Box.Length=10.00
        self.Box.Width=10.00
        self.Box.Height=10.00
        self.Doc.recompute()
        self.PolarPattern = self.Doc.addObject("PartDesign::PolarPattern","PolarPattern")
        self.PolarPattern.Originals = [self.Box]
        self.PolarPattern.Axis = (self.Doc.X_Axis,[""])
        self.PolarPattern.Angle = 360
        self.PolarPattern.Occurrences = 4
        self.Body.addObject(self.PolarPattern)
        self.Doc.recompute()
        self.assertAlmostEqual(self.PolarPattern.Shape.Volume, 4000)

    def testYAxisPolarPattern(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.Box = self.Doc.addObject('PartDesign::AdditiveBox','Box')
        self.Body.addObject(self.Box)
        self.Box.Length=10.00
        self.Box.Width=10.00
        self.Box.Height=10.00
        self.Doc.recompute()
        self.PolarPattern = self.Doc.addObject("PartDesign::PolarPattern","PolarPattern")
        self.PolarPattern.Originals = [self.Box]
        self.PolarPattern.Axis = (self.Doc.Y_Axis,[""])
        self.PolarPattern.Angle = 360
        self.PolarPattern.Occurrences = 4
        self.Body.addObject(self.PolarPattern)
        self.Doc.recompute()
        self.assertAlmostEqual(self.PolarPattern.Shape.Volume, 4000)

    def testZAxisPolarPattern(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.Box = self.Doc.addObject('PartDesign::AdditiveBox','Box')
        self.Body.addObject(self.Box)
        self.Box.Length=10.00
        self.Box.Width=10.00
        self.Box.Height=10.00
        self.Doc.recompute()
        self.PolarPattern = self.Doc.addObject("PartDesign::PolarPattern","PolarPattern")
        self.PolarPattern.Originals = [self.Box]
        self.PolarPattern.Axis = (self.Doc.Z_Axis,[""])
        self.PolarPattern.Angle = 360
        self.PolarPattern.Occurrences = 4
        self.Body.addObject(self.PolarPattern)
        self.Doc.recompute()
        self.assertAlmostEqual(self.PolarPattern.Shape.Volume, 4000)

    def testNormalSketchAxisPolarPattern(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad')
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0, 0), (10, 10))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 10
        self.Doc.recompute()
        self.PolarPattern = self.Doc.addObject("PartDesign::PolarPattern","PolarPattern")
        self.PolarPattern.Originals = [self.Pad]
        self.PolarPattern.Axis = (self.PadSketch,["N_Axis"])
        self.PolarPattern.Angle = 360
        self.PolarPattern.Occurrences = 4
        self.Body.addObject(self.PolarPattern)
        self.Doc.recompute()
        self.assertAlmostEqual(self.PolarPattern.Shape.Volume, 4000)

    def testVerticalSketchAxisPolarPattern(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad')
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0, 0), (10, 10))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 10
        self.Doc.recompute()
        self.PolarPattern = self.Doc.addObject("PartDesign::PolarPattern","PolarPattern")
        self.PolarPattern.Originals = [self.Pad]
        self.PolarPattern.Axis = (self.PadSketch,["V_Axis"])
        self.PolarPattern.Angle = 360
        self.PolarPattern.Occurrences = 4
        self.Body.addObject(self.PolarPattern)
        self.Doc.recompute()
        self.assertAlmostEqual(self.PolarPattern.Shape.Volume, 4000)

    def testHorizontalSketchAxisPolarPattern(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad')
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0, 0), (10, 10))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 10
        self.Doc.recompute()
        self.PolarPattern = self.Doc.addObject("PartDesign::PolarPattern","PolarPattern")
        self.PolarPattern.Originals = [self.Pad]
        self.PolarPattern.Axis = (self.PadSketch,["H_Axis"])
        self.PolarPattern.Angle = 360
        self.PolarPattern.Occurrences = 4
        self.Body.addObject(self.PolarPattern)
        self.Doc.recompute()
        self.assertAlmostEqual(self.PolarPattern.Shape.Volume, 4000)

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartDesignTestPolarPattern")
        # print ("omit closing document for debugging")

