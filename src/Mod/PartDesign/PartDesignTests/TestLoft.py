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
from FreeCAD import Base
from FreeCAD import Units
import Part
import Sketcher
import TestSketcherApp

class TestLoft(unittest.TestCase):
    """ Loft Tests """
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestLoft")

    def testSimpleAdditiveLoftCase(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.ProfileSketch = self.Doc.addObject('Sketcher::SketchObject', 'ProfileSketch')
        self.Body.addObject(self.ProfileSketch)
        TestSketcherApp.CreateRectangleSketch(self.ProfileSketch, (0, 0), (1, 1))
        self.Doc.recompute()
        self.LoftSketch = self.Doc.addObject('Sketcher::SketchObject', 'LoftSketch')
        self.Body.addObject(self.LoftSketch)
        self.LoftSketch.MapMode = 'FlatFace'
        self.LoftSketch.AttachmentSupport = (self.Doc.XZ_Plane, [''])
        self.Doc.recompute()
        TestSketcherApp.CreateRectangleSketch(self.LoftSketch, (0, 1), (1, 1))
        self.Doc.recompute()
        self.AdditiveLoft = self.Doc.addObject("PartDesign::AdditiveLoft","AdditiveLoft")
        self.Body.addObject(self.AdditiveLoft)
        self.AdditiveLoft.Profile = self.ProfileSketch
        self.AdditiveLoft.Sections = [self.LoftSketch]
        self.Doc.recompute()
        self.assertAlmostEqual(self.AdditiveLoft.Shape.Volume, 1)

    def testSimpleSubtractiveLoftCase(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad')
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0, 0), (1, 1))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 2
        self.Doc.recompute()
        self.ProfileSketch = self.Doc.addObject('Sketcher::SketchObject', 'ProfileSketch')
        self.Body.addObject(self.ProfileSketch)
        TestSketcherApp.CreateRectangleSketch(self.ProfileSketch, (0, 0), (1, 1))
        self.Doc.recompute()
        self.LoftSketch = self.Doc.addObject('Sketcher::SketchObject', 'LoftSketch')
        self.Body.addObject(self.LoftSketch)
        self.LoftSketch.MapMode = 'FlatFace'
        self.LoftSketch.AttachmentSupport = (self.Doc.XZ_Plane, [''])
        self.Doc.recompute()
        TestSketcherApp.CreateRectangleSketch(self.LoftSketch, (0, 1), (1, 1))
        self.Doc.recompute()
        self.SubtractiveLoft = self.Doc.addObject("PartDesign::SubtractiveLoft","SubtractiveLoft")
        self.Body.addObject(self.SubtractiveLoft)
        self.SubtractiveLoft.Profile = self.ProfileSketch
        self.SubtractiveLoft.Sections = [self.LoftSketch]
        self.Doc.recompute()
        self.assertAlmostEqual(self.SubtractiveLoft.Shape.Volume, 1)

    def testClosedAdditiveLoftCase(self):
        """ Test issue #6156: Loft tool "Closed" option not working """
        body = self.Doc.addObject('PartDesign::Body','Body')

        sketch1 = body.newObject('Sketcher::SketchObject','Sketch')
        sketch1.AttachmentSupport = (self.Doc.XZ_Plane,[''])
        sketch1.MapMode = 'FlatFace'
        sketch1.addGeometry(Part.Circle(Base.Vector(-40.0,0.0,0.0),Base.Vector(0,0,1),10.0), False)
        sketch1.addConstraint(Sketcher.Constraint('PointOnObject',0,3,-1))
        sketch1.addConstraint(Sketcher.Constraint('Diameter',0,20.0))
        sketch1.setDatum(1,Units.Quantity('20.000000 mm'))
        sketch1.addConstraint(Sketcher.Constraint('Distance',-1,1,0,3,40.0))
        sketch1.setDatum(2,Units.Quantity('40.000000 mm'))

        sketch2 = body.newObject('Sketcher::SketchObject','Sketch001')
        sketch2.AttachmentSupport = (self.Doc.YZ_Plane,'')
        sketch2.MapMode = 'FlatFace'
        sketch2.addGeometry(Part.Circle(Base.Vector(-10.0,0.0,0.0),Base.Vector(0,0,1),10.0),False)
        sketch2.addConstraint(Sketcher.Constraint('PointOnObject',0,3,-1))
        sketch2.addConstraint(Sketcher.Constraint('Diameter',0,20.0))
        sketch2.setDatum(1,Units.Quantity('20.000000 mm'))
        sketch2.addConstraint(Sketcher.Constraint('Distance',-1,1,0,3,40.0))
        sketch2.setDatum(2,Units.Quantity('40.000000 mm'))

        sketch3 = body.newObject('Sketcher::SketchObject','Sketch002')
        sketch3.AttachmentSupport = (self.Doc.getObject('YZ_Plane'),'')
        sketch3.MapMode = 'FlatFace'
        sketch3.addGeometry(Part.Circle(Base.Vector(40.0,0.0,0.0),Base.Vector(0,0,1),10.0),False)
        sketch3.addConstraint(Sketcher.Constraint('PointOnObject',0,3,-1))
        sketch3.addConstraint(Sketcher.Constraint('Distance',-1,1,0,3,40.0))
        sketch3.setDatum(1,Units.Quantity('40.000000 mm'))
        sketch3.addConstraint(Sketcher.Constraint('Diameter',0,20.0))
        sketch3.setDatum(2,Units.Quantity('20.000000 mm'))

        sketch4 = body.newObject('Sketcher::SketchObject','Sketch003')
        sketch4.AttachmentSupport = (self.Doc.XZ_Plane,'')
        sketch4.MapMode = 'FlatFace'
        sketch4.addGeometry(Part.Circle(Base.Vector(40.0,0.0,0.0),Base.Vector(0,0,1),10.0),False)
        sketch4.addConstraint(Sketcher.Constraint('PointOnObject',0,3,-1))
        sketch4.addConstraint(Sketcher.Constraint('Distance',-1,1,0,3,40.0))
        sketch4.setDatum(1,Units.Quantity('40.000000 mm'))
        sketch4.addConstraint(Sketcher.Constraint('Diameter',0,20.0))
        sketch4.setDatum(2,Units.Quantity('20.000000 mm'))

        self.Doc.recompute()

        loft = body.newObject('PartDesign::AdditiveLoft','AdditiveLoft')
        loft.Profile = sketch1
        loft.Sections = [sketch2, sketch4, sketch3]
        loft.Closed = True

        sketch1.Visibility = False
        sketch2.Visibility = False
        sketch3.Visibility = False
        sketch4.Visibility = False

        self.Doc.recompute()

        self.assertGreater(loft.Shape.Volume, 80000.0) # 85105.5788704151

    def testPadOnLoftBetweenCones(self):
        """  Test issue #15138 adapted from a script by chennes """
        body = self.Doc.addObject('PartDesign::Body', 'Body')
        body.Label = 'Body'
        coneBottomSketch = body.newObject('Sketcher::SketchObject', 'ConeBottomSketch')
        coneBottomSketch.AttachmentSupport = (self.Doc.getObject('XY_Plane'), [''])
        coneBottomSketch.MapMode = 'FlatFace'

        geoList = []
        geoList.append(Part.Circle(Base.Vector(0.000000, 0.000000, 0.000000),
                                   Base.Vector(0.000000, 0.000000, 1.000000),
                                   25.000000))
        coneBottomSketch.addGeometry(geoList, False)
        del geoList

        constraintList = []
        coneBottomSketch.addConstraint(Sketcher.Constraint('Diameter', 0, 25.000000))
        coneBottomSketch.addConstraint(Sketcher.Constraint('Coincident', 0, 3, -1, 1))

        geoList = []
        geoList.append(Part.Circle(Base.Vector(0.000000, 0.000000, 0.000000),
                                   Base.Vector(0.000000, 0.000000, 1.000000),
                                   40.000000))
        coneBottomSketch.addGeometry(geoList, False)
        del geoList

        constraintList = []
        coneBottomSketch.addConstraint(Sketcher.Constraint('Diameter', 1, 40.000000))
        coneBottomSketch.addConstraint(Sketcher.Constraint('Coincident', 1, 3, 0, 3))

        coneTopSketch = body.newObject('Sketcher::SketchObject', 'ConeTopSketch')
        coneTopSketch.AttachmentSupport = (self.Doc.getObject('XY_Plane'), [''])
        coneTopSketch.MapMode = 'FlatFace'

        geoList = []
        geoList.append(
            Part.Circle(Base.Vector(0.000000, 0.000000, 0.000000),
                        Base.Vector(0.000000, 0.000000, 1.000000),
                        8.000000))
        coneTopSketch.addGeometry(geoList, False)
        del geoList

        constraintList = []
        coneTopSketch.addConstraint(Sketcher.Constraint('Diameter', 0, 8.000000))
        coneTopSketch.addConstraint(Sketcher.Constraint('Coincident', 0, 3, -1, 1))

        geoList = []
        geoList.append(Part.Circle(Base.Vector(0.000000, 0.000000, 0.000000),
                                   Base.Vector(0.000000, 0.000000, 1.000000),
                                   15.000000))
        coneTopSketch.addGeometry(geoList, False)
        del geoList

        constraintList = []
        coneTopSketch.addConstraint(Sketcher.Constraint('Diameter', 1, 15.000000))
        coneTopSketch.addConstraint(Sketcher.Constraint('Coincident', 1, 3, 0, 3))
        coneTopSketch.AttachmentOffset = Base.Placement(Base.Vector(0, 0, 20),
                                                        Base.Rotation(Base.Vector(0, 0, 1), 0))
        self.Doc.recompute()

        cone = body.newObject('PartDesign::AdditiveLoft', 'Cone')
        cone.Profile = coneBottomSketch
        cone.Sections = [(coneTopSketch, [''])]
        coneBottomSketch.Visibility = False
        coneTopSketch.Visibility = False
        self.Doc.recompute()

        pad = body.newObject('PartDesign::Pad', 'Pad')
        pad.Profile = (cone, ['Face3', ])
        pad.Length = 10.000000
        pad.TaperAngle = 0.000000
        pad.UseCustomVector = 0
        pad.Direction = (0, 0, 1)
        pad.ReferenceAxis = None
        pad.AlongSketchNormal = 1
        pad.Type = 0
        pad.UpToFace = None
        pad.Reversed = False
        pad.Midplane = 0
        pad.Offset = 0
        cone.Visibility = True
        self.Doc.recompute()

        outerConeBottomRadius = 40 / 2
        outerConeTopRadius = 15 / 2
        innerConeBottomRadius = 25 / 2
        innerConeTopRadius = 8 / 2
        coneHeight = 20.0
        padHeight = 10.0
        # Frustum volumes
        outerConeVolume = 1.0 / 3.0 * math.pi * coneHeight * (
                    outerConeBottomRadius ** 2 + outerConeTopRadius ** 2 +
                    outerConeBottomRadius * outerConeTopRadius )
        innerConeVolume = 1.0 / 3.0 * math.pi * coneHeight * (
                    innerConeBottomRadius ** 2 + innerConeTopRadius ** 2 +
                    innerConeBottomRadius * innerConeTopRadius )
        frustumVolume = outerConeVolume - innerConeVolume
        topArea = math.pi * (outerConeTopRadius ** 2) - math.pi * (innerConeTopRadius ** 2)
        bottomArea = math.pi * (outerConeBottomRadius ** 2) - math.pi * (innerConeBottomRadius ** 2)
        padVolume = topArea * padHeight
        self.assertEqual(len(cone.Shape.Faces), 4)  # Bottom, Inner, Top, Outer
        self.assertAlmostEqual(cone.Shape.Faces[2].Area, topArea)
        self.assertAlmostEqual(cone.Shape.Faces[0].Area, bottomArea)
        self.assertAlmostEqual(cone.Shape.Volume, frustumVolume)
        self.assertAlmostEqual(pad.Shape.Volume, frustumVolume + padVolume)     # contains volume of previous in Body
        self.assertAlmostEqual(body.Shape.Volume, frustumVolume + padVolume)    # Overall body volume matches

    def testTwoFacesAdditiveLoftCase(self):
        """Test issue #19183: Loft tool "Loft between faces no longer works"""
        body = self.Doc.addObject("PartDesign::Body", "Body")

        sketch1 = body.newObject("Sketcher::SketchObject", "Sketch")

        sketch1.addGeometry(
            Part.LineSegment(
                Base.Vector(-2.060394, -1.332045, 0),
                Base.Vector(-19.922129, -27.589359, 0),
            ),
            False,
        )
        sketch1.addGeometry(
            Part.LineSegment(
                Base.Vector(1.940183, -1.501086, 0),
                Base.Vector(16.928263, -28.265512, 0),
            ),
            False,
        )
        sketch1.addGeometry(
            Part.ArcOfCircle(
                Part.Circle(
                    Base.Vector(0.418837, -1.275699, 0), Base.Vector(0, 0, 1), 32.879378
                ),
                -1.751723,
                -1.454683,
            ),
            False,
        )
        sketch1.addGeometry(
            Part.ArcOfCircle(
                Part.Circle(
                    Base.Vector(-9.385396, -30.124937, 0),
                    Base.Vector(0, 0, 1),
                    8.359959,
                ),
                2.847554,
                4.831818,
            ),
            False,
        )
        sketch1.addGeometry(
            Part.ArcOfCircle(
                Part.Circle(
                    Base.Vector(3.236143, -29.279745, 0),
                    Base.Vector(0, 0, 1),
                    11.449505,
                ),
                -1.076432,
                0.044306,
            ),
            False,
        )
        sketch1.addConstraint(Sketcher.Constraint("Coincident", 0, 1, 1, 1))
        sketch1.addConstraint(Sketcher.Constraint("Coincident", 0, 1, 2, 3))
        sketch1.addConstraint(Sketcher.Constraint("Coincident", 0, 1, -1, 1))
        sketch1.addConstraint(Sketcher.Constraint("Tangent", 0, 2, 3, 1))
        sketch1.addConstraint(Sketcher.Constraint("Tangent", 1, 2, 4, 2))
        sketch1.addConstraint(Sketcher.Constraint("Tangent", 2, 1, 3, 2))
        sketch1.addConstraint(Sketcher.Constraint("Tangent", 2, 2, 4, 1))
        sketch1.addConstraint(Sketcher.Constraint("Symmetric", 0, 2, 1, 2, -2))
        sketch1.addConstraint(Sketcher.Constraint("Symmetric", 2, 1, 2, 2, -2))
        sketch1.delConstraint(8)
        sketch1.addConstraint(Sketcher.Constraint("Radius", 2, 39.936694))
        sketch1.setDatum(8, Units.Quantity("40.000000 mm"))
        sketch1.addConstraint(Sketcher.Constraint("DistanceX", 2, 1, 2, 2, 16.771341))
        sketch1.setDatum(9, Units.Quantity("10.000000 mm"))
        sketch1.addConstraint(Sketcher.Constraint("Distance", 0, 30.965710))
        sketch1.setDatum(10, Units.Quantity("30.000000 mm"))

        sketch2 = body.newObject("Sketcher::SketchObject", "Sketch001")
        sketch2.addGeometry(
            Part.LineSegment(
                Base.Vector(-2.060394, -1.332045, 0),
                Base.Vector(-19.922129, -27.589359, 0),
            ),
            False,
        )
        sketch2.addGeometry(
            Part.LineSegment(
                Base.Vector(1.940183, -1.501086, 0),
                Base.Vector(16.928263, -28.265512, 0),
            ),
            False,
        )
        sketch2.addGeometry(
            Part.ArcOfCircle(
                Part.Circle(
                    Base.Vector(0.418837, -1.275699, 0), Base.Vector(0, 0, 1), 32.879378
                ),
                -1.751723,
                -1.454683,
            ),
            False,
        )
        sketch2.addGeometry(
            Part.ArcOfCircle(
                Part.Circle(
                    Base.Vector(-9.385396, -30.124937, 0),
                    Base.Vector(0, 0, 1),
                    8.359959,
                ),
                2.847554,
                4.831818,
            ),
            False,
        )
        sketch2.addGeometry(
            Part.ArcOfCircle(
                Part.Circle(
                    Base.Vector(3.236143, -29.279745, 0),
                    Base.Vector(0, 0, 1),
                    11.449505,
                ),
                -1.076432,
                0.044306,
            ),
            False,
        )
        sketch2.addConstraint(Sketcher.Constraint("Coincident", 0, 1, 1, 1))
        sketch2.addConstraint(Sketcher.Constraint("Coincident", 0, 1, 2, 3))
        sketch2.addConstraint(Sketcher.Constraint("Coincident", 0, 1, -1, 1))
        sketch2.addConstraint(Sketcher.Constraint("Tangent", 0, 2, 3, 1))
        sketch2.addConstraint(Sketcher.Constraint("Tangent", 1, 2, 4, 2))
        sketch2.addConstraint(Sketcher.Constraint("Tangent", 2, 1, 3, 2))
        sketch2.addConstraint(Sketcher.Constraint("Tangent", 2, 2, 4, 1))
        sketch2.addConstraint(Sketcher.Constraint("Symmetric", 0, 2, 1, 2, -2))
        sketch2.addConstraint(Sketcher.Constraint("Symmetric", 2, 1, 2, 2, -2))
        sketch2.delConstraint(8)
        sketch2.addConstraint(Sketcher.Constraint("Radius", 2, 39.936694))
        sketch2.setDatum(8, Units.Quantity("30.000000 mm"))
        sketch2.addConstraint(Sketcher.Constraint("Radius", 3, 1.020288))
        sketch2.setDatum(9, Units.Quantity("3.600000 mm"))
        sketch2.addConstraint(Sketcher.Constraint("DistanceX", 2, 1, 2, 2, 10.926759))
        sketch2.setDatum(10, Units.Quantity("10.000000 mm"))
        self.Doc.recompute()

        pad1 = body.newObject("PartDesign::Pad", "Pad")

        pad1.Profile = sketch1
        pad1.Length = 10.000000
        pad1.TaperAngle = 0.000000
        pad1.Reversed = 1
        self.Doc.recompute()
        sketch1.Visibility = False

        pad2 = body.newObject("PartDesign::Pad", "Pad001")

        pad2.Profile = sketch2
        pad2.Length = 10.000000
        pad2.TaperAngle = 0.000000
        self.Doc.recompute()
        sketch2.Visibility = False
        pad1.Visibility = False
        pad2.Visibility = True
        body.Tip = pad2
        self.assertGreater(pad2.Shape.Volume, 8715.0)  # 8720.024151557787 pre-Loft

        loft = body.newObject("PartDesign::AdditiveLoft", "AdditiveLoft")
        loft.Profile = (
            self.Doc.getObject("Pad001"),
            [
                "Face7",
            ],
        )
        loft.Sections = [
            (
                self.Doc.getObject("Pad001"),
                [
                    "Face10",
                ],
            )
        ]
        loft.Closed = False

        self.Doc.recompute()
        body.Tip = loft
        self.assertGreater(body.Shape.Volume, 9220.0)  # 9221.776241582389 post-Loft
        self.Doc.recompute()

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartDesignTestLoft")
        # print ("omit closing document for debugging")
