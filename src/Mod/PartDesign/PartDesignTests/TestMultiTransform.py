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

App = FreeCAD

class TestMultiTransform(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestMultiTransform")

    def testMultiTransform(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        # Make first offset cube Pad
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad')
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0, 0), (10, 10))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 10
        self.Doc.recompute()
        self.MultiTransform = self.Doc.addObject("PartDesign::MultiTransform","MultiTransform")
        self.Doc.recompute()
        self.MultiTransform.Originals = [self.Pad]
        self.MultiTransform.Shape = self.Pad.Shape
        self.Body.addObject(self.MultiTransform)
        self.Doc.recompute()
        self.Mirrored = self.Doc.addObject("PartDesign::Mirrored","Mirrored")
        self.Mirrored.MirrorPlane = (self.PadSketch, ["H_Axis"])
        self.Body.addObject(self.Mirrored)
        self.LinearPattern = self.Doc.addObject("PartDesign::LinearPattern","LinearPattern")
        self.LinearPattern.Direction = (self.PadSketch, ["H_Axis"])
        self.LinearPattern.Length = 20
        self.LinearPattern.Occurrences = 3
        self.Body.addObject(self.LinearPattern)
        self.PolarPattern = self.Doc.addObject("PartDesign::PolarPattern","PolarPattern")
        self.PolarPattern.Axis = (self.PadSketch, ["N_Axis"])
        self.PolarPattern.Angle = 360
        self.PolarPattern.Occurrences = 4
        self.Body.addObject(self.PolarPattern)
        self.MultiTransform.Transformations = [self.Mirrored,self.LinearPattern,self.PolarPattern]
        self.Doc.recompute()
        self.assertAlmostEqual(self.MultiTransform.Shape.Volume, 20000)

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartDesignTestMultiTransform")
        #print ("omit closing document for debugging")

