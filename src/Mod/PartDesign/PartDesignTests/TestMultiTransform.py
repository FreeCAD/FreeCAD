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
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "True")

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

    def testMultiTransformDressup(self):
        # Arrange
        Doc = self.Doc
        Body = Doc.addObject('PartDesign::Body','Body')
        Body.AllowCompound = False
        # Make first offset cube Pad
        PadSketch = Doc.addObject('Sketcher::SketchObject', 'SketchPad')
        Body.addObject(PadSketch)
        TestSketcherApp.CreateRectangleSketch(PadSketch, (0, 0), (10, 10))
        Doc.recompute()
        Pad = Doc.addObject("PartDesign::Pad", "Pad")
        Body.addObject(Pad)
        Pad.Profile = PadSketch
        Pad.Length = 10
        Doc.recompute()

        PadSketch2 = Doc.addObject('Sketcher::SketchObject', 'SketchPad')
        PadSketch2.AttachmentSupport = (Pad, ('Face6',))
        Body.addObject(PadSketch2)
        TestSketcherApp.CreateRectangleSketch(PadSketch, (9, 9), (1, 1))
        Doc.recompute()
        Pad2 = Doc.addObject("PartDesign::Pad", "Pad2")
        Body.addObject(Pad2)
        Pad2.Profile = PadSketch2
        Pad2.Length = 10
        Doc.recompute()


        MultiTransform = Doc.addObject("PartDesign::MultiTransform","MultiTransform")
        Doc.recompute()
        MultiTransform.Originals = [Pad]
        MultiTransform.Shape = Pad.Shape
        Body.addObject(MultiTransform)
        Doc.recompute()
        Mirrored = Doc.addObject("PartDesign::Mirrored","Mirrored")
        Mirrored.MirrorPlane = (Doc.getObject('XY_Plane'), [''])
        Body.addObject(Mirrored)
        Mirrored2 = Doc.addObject("PartDesign::Mirrored","Mirrored")
        Mirrored2.MirrorPlane = (Doc.getObject('XZ_Plane'), [""])
        Body.addObject(Mirrored2)
        MultiTransform.Transformations = [Mirrored,Mirrored2]
        Doc.recompute()
        Fillet = Doc.addObject("PartDesign::Fillet","Fillet")
        Fillet.Base = (MultiTransform, ['Face'+str(i+1) for i in range(2)])
        Fillet.Radius = 3
        Body.addObject(Fillet)
        # Add a fillet here.
        # Now do that copy thing...
        Link = Doc.addObject('App::Link','Link001')
        Link.setLink(Doc.Body)
        Link.Label='Body001'
        # Act
        #  There are properties on those objects with values
        # Link.addProperty("App::PropertyInteger","test2","Table2")
        # Assert
        self.assertAlmostEqual(Body.Shape.Volume, 990)
        self.assertAlmostEqual(Link.Shape.Volume, 990)

    def testMultiTransformBody(self):
        pass
        # TODO:  Someone who understands the second mode added to transform needs to
        #  write a test here.  Maybe close to  testMultiTransform but with
        #  self.Mirrored.TransformMode="Transform body"  instead of
        #  self.Mirrored.TransformMode="Transform tools"

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartDesignTestMultiTransform")
        FreeCAD.ConfigSet("SuppressRecomputeRequiredDialog", "")
        #print ("omit closing document for debugging")

