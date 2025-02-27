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

class TestLinearPattern(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestLinearPattern")

    def testXAxisLinearPattern(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.Box = self.Doc.addObject('PartDesign::AdditiveBox','Box')
        self.Body.addObject(self.Box)
        self.Box.Length=10.00
        self.Box.Width=10.00
        self.Box.Height=10.00
        self.Doc.recompute()
        self.LinearPattern = self.Doc.addObject("PartDesign::LinearPattern","LinearPattern")
        self.LinearPattern.Originals = [self.Box]
        self.LinearPattern.Direction = (self.Doc.X_Axis,[""])
        self.LinearPattern.Length = 90.0
        self.LinearPattern.Occurrences = 10
        self.LinearPattern.Refine = True
        self.Body.addObject(self.LinearPattern)
        self.Doc.recompute()
        self.assertAlmostEqual(self.LinearPattern.Shape.Volume, 1e4)
        # 44 + 84 + 42 = 170.  44 - 8 = 36 / 9 = 4.  84-12 = 72 / 9 = 8.  42 - 6 = 36 / 9 = 4
        # We have the original 26 from the first shape, plus 4 more vertices, 8 more edges and
        # 4 more faces for each additional copy.  Since they have to touch ( single shape rule ),
        # We're adding 4 points to define each additional prism's new points, 8 edges makes sense,
        # and 4 faces makes sense since we're defining essentially a tube, not a box for each copy.
        # self.assertNotEqual(self.LinearPattern.Shape.ElementReverseMap["Vertex44"], "")
        # self.assertNotEqual(self.LinearPattern.Shape.ElementReverseMap["Edge84"], "")
        # self.assertNotEqual(self.LinearPattern.Shape.ElementReverseMap["Face42"], "")
        #
        # self.assertEqual(self.LinearPattern.Shape.ElementMapSize, 170)    # TODO
        self.assertEqual(self.LinearPattern.Shape.ElementMapSize, 26)

    def testYAxisLinearPattern(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.Box = self.Doc.addObject('PartDesign::AdditiveBox','Box')
        self.Body.addObject(self.Box)
        self.Box.Length=10.00
        self.Box.Width=10.00
        self.Box.Height=10.00
        self.Doc.recompute()
        self.LinearPattern = self.Doc.addObject("PartDesign::LinearPattern","LinearPattern")
        self.LinearPattern.Originals = [self.Box]
        self.LinearPattern.Direction = (self.Doc.Y_Axis,[""])
        self.LinearPattern.Length = 90.0
        self.LinearPattern.Occurrences = 10
        self.LinearPattern.Refine = True
        self.Body.addObject(self.LinearPattern)
        self.Doc.recompute()
        self.assertAlmostEqual(self.LinearPattern.Shape.Volume, 1e4)
        # self.assertEqual(self.LinearPattern.Shape.ElementMapSize, 170)    # TODO
        self.assertEqual(self.LinearPattern.Shape.ElementMapSize, 26)

    def testZAxisLinearPattern(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.Box = self.Doc.addObject('PartDesign::AdditiveBox','Box')
        self.Body.addObject(self.Box)
        self.Box.Length=10.00
        self.Box.Width=10.00
        self.Box.Height=10.00
        self.Doc.recompute()
        self.LinearPattern = self.Doc.addObject("PartDesign::LinearPattern","LinearPattern")
        self.LinearPattern.Originals = [self.Box]
        self.LinearPattern.Direction = (self.Doc.Z_Axis,[""])
        self.LinearPattern.Length = 90.0
        self.LinearPattern.Occurrences = 10
        self.LinearPattern.Refine = True
        self.Body.addObject(self.LinearPattern)
        self.Doc.recompute()
        self.assertAlmostEqual(self.LinearPattern.Shape.Volume, 1e4)
        # self.assertEqual(self.LinearPattern.Shape.ElementMapSize, 170)    # TODO
        self.assertEqual(self.LinearPattern.Shape.ElementMapSize, 26)

    def testNormalSketchAxisLinearPattern(self):
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
        self.LinearPattern = self.Doc.addObject("PartDesign::LinearPattern","LinearPattern")
        self.LinearPattern.Originals = [self.Pad]
        self.LinearPattern.Direction = (self.PadSketch,["N_Axis"])
        self.LinearPattern.Length = 90.0
        self.LinearPattern.Occurrences = 10
        self.LinearPattern.Refine = True
        self.Body.addObject(self.LinearPattern)
        self.Doc.recompute()
        self.assertAlmostEqual(self.LinearPattern.Shape.Volume, 1e4)
        # self.assertEqual(self.LinearPattern.Shape.ElementMapSize, 170)    # TODO
        self.assertEqual(self.LinearPattern.Shape.ElementMapSize, 26)

    def testVerticalSketchAxisLinearPattern(self):
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
        self.LinearPattern = self.Doc.addObject("PartDesign::LinearPattern","LinearPattern")
        self.LinearPattern.Originals = [self.Pad]
        self.LinearPattern.Direction = (self.PadSketch,["V_Axis"])
        self.LinearPattern.Length = 90.0
        self.LinearPattern.Occurrences = 10
        self.LinearPattern.Refine = True
        self.Body.addObject(self.LinearPattern)
        self.Doc.recompute()
        self.assertAlmostEqual(self.LinearPattern.Shape.Volume, 1e4)
        # self.assertEqual(self.LinearPattern.Shape.ElementMapSize, 170)    # TODO
        self.assertEqual(self.LinearPattern.Shape.ElementMapSize, 26)

    def testHorizontalSketchAxisLinearPattern(self):
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
        self.LinearPattern = self.Doc.addObject("PartDesign::LinearPattern","LinearPattern")
        self.LinearPattern.Originals = [self.Pad]
        self.LinearPattern.Direction = (self.PadSketch,["H_Axis"])
        self.LinearPattern.Length = 90.0
        self.LinearPattern.Occurrences = 10
        self.LinearPattern.Refine = True
        self.Body.addObject(self.LinearPattern)
        self.Doc.recompute()
        self.assertAlmostEqual(self.LinearPattern.Shape.Volume, 1e4)
        # self.assertEqual(self.LinearPattern.Shape.ElementMapSize, 170)    # TODO
        # self.assertEqual(len(self.LinearPattern.Shape.ElementReverseMap), 170)
        self.assertEqual(self.LinearPattern.Shape.ElementMapSize, 26)

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartDesignTestLinearPattern")
        # print ("omit closing document for debugging")

