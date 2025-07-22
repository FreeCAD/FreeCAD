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
import math

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
        xw = yw = zw = 10
        TestSketcherApp.CreateRectangleSketch(PadSketch, (0, 0), (xw, yw))
        Doc.recompute()
        Pad = Doc.addObject("PartDesign::Pad", "Pad")
        Body.addObject(Pad)
        Pad.Profile = PadSketch
        Pad.Length = zw
        Doc.recompute()
        MultiTransform = Doc.addObject("PartDesign::MultiTransform","MultiTransform")
        Doc.recompute()
        MultiTransform.Originals = [Pad]
        MultiTransform.Shape = Pad.Shape
        Body.addObject(MultiTransform)
        Doc.recompute()
        Mirrored = Doc.addObject("PartDesign::Mirrored","Mirrored")
        Mirrored.MirrorPlane = (Doc.getObject('XY_Plane'), [''])
        Mirrored.Refine = True
        Body.addObject(Mirrored)
        Mirrored2 = Doc.addObject("PartDesign::Mirrored","Mirrored")
        Mirrored2.MirrorPlane = (Doc.getObject('XZ_Plane'), [""])
        Mirrored2.Refine = True
        Body.addObject(Mirrored2)
        MultiTransform.Transformations = [Mirrored,Mirrored2]
        MultiTransform.Refine = True
        Doc.recompute()
        Fillet = Doc.addObject("PartDesign::Fillet","Fillet")
        Fillet.Base = (MultiTransform, [ "Face1", "Face2" ])
        radius = 3
        Fillet.Radius = radius
        Body.addObject(Fillet)
        # Broken out calculation of volume with two adjacent filleted faces = 5 long edges, 2 short edges,
        # 2 fully rounded corners and 4 corners with only 2 fillets meeting
        cubeVolume = xw * yw * zw * 2 * 2   # Mirrored and mirrored again.
        filletOuter = radius ** 2 * ( xw - radius * 2 )     # Volume of the rect prisms the fillets are in.
        filletCorner = radius ** 3  # Volume of the rect prism corners
        qRoundArea = math.pi * radius ** 2 / 4  # Area of the quarter round fillet profile
        filletPrism = qRoundArea * ( xw  - radius * 2 )    # Volume of fillet minus corners
        fillet3Corner = math.pi * radius ** 3 * 4 / 3 / 8    # Volume of a fully rounded corner ( Sphere / 8 )
        fillet2Corner = radius ** 2 * 2   # Volume of corner with two fillets intersecting
        fillet1Corner = math.pi * radius ** 2 / 4 * radius  # Volume of corner with stopped single fillet
        extraFillet = qRoundArea * xw # extra fillet in a mirrored direction
        filletOuterExt = radius ** 2 * 10   # extra rect prim surrounding fillet
        rectBox = cubeVolume - (4 + 3) * (filletOuter + filletCorner ) - 5 * filletOuterExt + filletCorner
        fillets = ( 4 + 3 ) * filletPrism  + 5 * extraFillet + fillet3Corner * 2 + fillet2Corner * 4 + fillet1Corner * 0
        volume = rectBox + fillets
        # Act
        Link = Doc.addObject('App::Link','Link001')
        Link.setLink(Doc.Body)
        Link.Label='Body001'
        Doc.recompute()
        # Assert
        self.assertAlmostEqual(abs(Body.Shape.Volume), volume, 6)
        self.assertAlmostEqual(abs(Link.Shape.Volume), volume, 6)

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

