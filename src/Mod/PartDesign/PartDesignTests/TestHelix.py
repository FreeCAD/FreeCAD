#***************************************************************************
#*   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
#*   Copyright (c) 2023 <bgbsww@gmail.com>                                 *
#*                                                                         *
#*   This file is part of FreeCAD.                                         *
#*                                                                         *
#*   FreeCAD is free software: you can redistribute it and/or modify it    *
#*   under the terms of the GNU Lesser General Public License as           *
#*   published by the Free Software Foundation, either version 2.1 of the  *
#*   License, or (at your option) any later version.                       *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful, but        *
#*   WITHOUT ANY WARRANTY; without even the implied warranty of            *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
#*   Lesser General Public License for more details.                       *
#*                                                                         *
#*   You should have received a copy of the GNU Lesser General Public      *
#*   License along with FreeCAD. If not, see                               *
#*   <https://www.gnu.org/licenses/>.                                      *
#*                                                                         *
#***************************************************************************

from math import pi
import unittest

import FreeCAD
from FreeCAD import Base
import Part
import Sketcher
import TestSketcherApp

""" Test various helixes """
class TestHelix(unittest.TestCase):
    """ Test various helixes """
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestHelix")

    def testHelicalTubeCase(self):
        body = self.Doc.addObject('PartDesign::Body','Body')
        sketch = body.newObject('Sketcher::SketchObject','Sketch')
        sketch.AttachmentSupport = (self.Doc.getObject('XY_Plane'),[''])
        sketch.MapMode = 'FlatFace'

        geoList = []
        geoList.append(Part.Circle(Base.Vector(-40.0, 0.0, 0.0), Base.Vector(0.0, 0.0, 1.0), 7.5))
        geoList.append(Part.Circle(Base.Vector(-40.0, 0.0, 0.0), Base.Vector(0.0, 0.0, 1.0), 10.0))
        sketch.addGeometry(geoList, False)
        del geoList

        sketch.addConstraint(Sketcher.Constraint('PointOnObject', 0, 3, -1))
        sketch.addConstraint(Sketcher.Constraint('Coincident', 1, 3, 0, 3))
        self.Doc.recompute()

        helix = body.newObject('PartDesign::AdditiveHelix','AdditiveHelix')
        helix.Profile = sketch
        helix.ReferenceAxis = (sketch, ['V_Axis'])
        helix.Mode = 0
        helix.Pitch = 35
        helix.Height = 100
        helix.Turns = 3
        helix.Angle = 0
        helix.Growth = 0
        helix.LeftHanded = 0
        helix.Reversed = 0

        self.Doc.recompute()
        self.assertEqual(len(helix.Shape.Solids), 1)


    def testCircleQ1(self):
        """ Test helix based on circle in Quadrant 1 """
        body = self.Doc.addObject('PartDesign::Body','Body')
        profileSketch = self.Doc.addObject('Sketcher::SketchObject', 'ProfileSketch')
        body.addObject(profileSketch)
        TestSketcherApp.CreateCircleSketch(profileSketch, (2, 0), 1)
        self.Doc.recompute()
        helix = self.Doc.addObject("PartDesign::AdditiveHelix","AdditiveHelix")
        body.addObject(helix)
        helix.Profile = profileSketch
        helix.ReferenceAxis = (profileSketch,"V_Axis")
        helix.Placement = FreeCAD.Placement(FreeCAD.Vector(0,0,0), 
                                            FreeCAD.Rotation(FreeCAD.Vector(0,0,1),0),
                                            FreeCAD.Vector(0,0,0))
        helix.Pitch = 3
        helix.Height = 9
        helix.Turns = 2
        helix.Angle = 0
        helix.Mode = 1
        self.Doc.recompute()
        self.assertAlmostEqual(helix.Shape.Volume, 78.95687956849457,places=5)

        helix.Angle = 25
        self.Doc.recompute()
        self.assertAlmostEqual(helix.Shape.Volume, 134.17450779511307,places=5)

        profileSketch.addGeometry(Part.Circle(FreeCAD.Vector(2, 0, 0), FreeCAD.Vector(0,0,1), 0.5) )
        self.Doc.recompute()
        self.assertAlmostEqual(helix.Shape.Volume, 100.63088079046352,places=5)


    def testRectangle(self):
        """ Test helix based on a rectangle """
        body = self.Doc.addObject('PartDesign::Body','GearBody')
        gearSketch = self.Doc.addObject('Sketcher::SketchObject', 'GearSketch')
        body.addObject(gearSketch)
        TestSketcherApp.CreateRectangleSketch(gearSketch, (0, 0), (5, 5))
        self.Doc.recompute()

        # xz_plane = body.Origin.OriginFeatures[4]
        # coneSketch.AttachmentSupport = xz_plane
        # coneSketch.MapMode = 'FlatFace'
        helix = self.Doc.addObject("PartDesign::AdditiveHelix","AdditiveHelix")
        body.addObject(helix)
        helix.Profile = gearSketch
        helix.ReferenceAxis = (gearSketch,"V_Axis")
        helix.Placement = FreeCAD.Placement(FreeCAD.Vector(0,0,0), FreeCAD.Rotation(FreeCAD.Vector(0,0,1),0), FreeCAD.Vector(0,0,0))

        helix.Pitch = 50
        helix.Height = 150
        helix.Turns = 3
        helix.Angle = 0
        helix.Mode = 0 
        self.Doc.recompute()
        bbox = helix.Shape.BoundBox
        self.assertAlmostEqual(bbox.YMin,0)
        # Computed exact value
        # with r = radius, l = length of square, t = turns
        # pi * r**2 * l * t
        expected = pi * 25 * 5 * 3
        self.assertAlmostEqual(helix.Shape.Volume, expected, places=2)


    def testCone(self):
        """ Test helix following a cone """
        body = self.Doc.addObject('PartDesign::Body','ConeBody')
        coneSketch = self.Doc.addObject('Sketcher::SketchObject', 'ConeSketch')
        body.addObject(coneSketch)

        geoList = []
        geoList.append(Part.LineSegment(FreeCAD.Vector(-5, -5, 0), FreeCAD.Vector(-3, 0, 0)) )
        geoList.append(Part.LineSegment(FreeCAD.Vector(-3, 0, 0), FreeCAD.Vector(-2, 0, 0)) )
        geoList.append(Part.LineSegment(FreeCAD.Vector(-2, 0, 0), FreeCAD.Vector(-4, -5, 0)) )
        geoList.append(Part.LineSegment(FreeCAD.Vector(-4, -5, 0), FreeCAD.Vector(-5, -5, 0)))
        (l1, l2, l3, l4) = coneSketch.addGeometry(geoList)

        conList = []
        conList.append(Sketcher.Constraint("Coincident", 0, 2, 1, 1))
        conList.append(Sketcher.Constraint("Coincident", 1, 2, 2, 1))
        conList.append(Sketcher.Constraint("Coincident", 2, 2, 3, 1))
        conList.append(Sketcher.Constraint("Coincident", 3, 2, 0, 1))
        conList.append(Sketcher.Constraint("Horizontal", 1))
        conList.append(Sketcher.Constraint("Angle", l3, 1, -2, 2, FreeCAD.Units.Quantity("30.000000 deg")))
        conList.append(Sketcher.Constraint("DistanceX", 1, 2, -5))
        conList.append(Sketcher.Constraint("DistanceY", 1, 2, 0))
        conList.append(Sketcher.Constraint("Equal", 0, 2))
        conList.append(Sketcher.Constraint("Equal", 1, 3))
        conList.append(Sketcher.Constraint("DistanceY", 0, 50))
        conList.append(Sketcher.Constraint("DistanceX", 1, 10))
        coneSketch.addConstraint(conList)

        xz_plane = body.Origin.OriginFeatures[4]
        coneSketch.AttachmentSupport = xz_plane
        coneSketch.MapMode = 'FlatFace'
        helix = self.Doc.addObject("PartDesign::AdditiveHelix","AdditiveHelix")
        body.addObject(helix)
        helix.Profile = coneSketch
        helix.ReferenceAxis = (coneSketch,"V_Axis")
        helix.Placement = FreeCAD.Placement(FreeCAD.Vector(0,0,0), FreeCAD.Rotation(FreeCAD.Vector(0,0,1),0), FreeCAD.Vector(0,0,0))

        helix.Pitch = 50
        helix.Height = 110
        helix.Turns = 2.2
        helix.Angle = 30
        helix.Mode = 0 
        helix.Reversed = True
        self.Doc.recompute()
        self.assertAlmostEqual(helix.Shape.Volume, 388285.4117047924,places=5)

    def tearDown(self):
        FreeCAD.closeDocument("PartDesignTestHelix")

