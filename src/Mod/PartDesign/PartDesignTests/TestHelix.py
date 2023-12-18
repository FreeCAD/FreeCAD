#***************************************************************************
#*   Copyright (c) 2023 <bgbsww@gmail.com>                                 *
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

from math import pi
import unittest

import FreeCAD
import Part
import Sketcher
import TestSketcherApp

class TestHelix(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestHelix")

    def testCircleQ1(self):
        body = self.Doc.addObject('PartDesign::Body','Body')
        profileSketch = self.Doc.addObject('Sketcher::SketchObject', 'ProfileSketch')
        body.addObject(profileSketch)
        TestSketcherApp.CreateCircleSketch(profileSketch, (2, 0), 1)
        self.Doc.recompute()
        helix = self.Doc.addObject("PartDesign::AdditiveHelix","AdditiveHelix")
        body.addObject(helix)
        helix.Profile = profileSketch
        helix.ReferenceAxis = (profileSketch,"V_Axis")
        helix.Placement = FreeCAD.Placement(FreeCAD.Vector(0,0,0), FreeCAD.Rotation(FreeCAD.Vector(0,0,1),0), FreeCAD.Vector(0,0,0))
        helix.Pitch = 3
        helix.Height = 9
        helix.Turns = 2
        helix.Angle = 0
        helix.Mode = 1 
        self.Doc.recompute()
        self.assertAlmostEqual(helix.Shape.Volume, 78.95687956849457,places=5)

        helix.Angle = 25
        self.Doc.recompute()
        self.assertAlmostEqual(helix.Shape.Volume, 134.17451071237386,places=5)

        profileSketch.addGeometry(Part.Circle(FreeCAD.Vector(2, 0, 0), FreeCAD.Vector(0,0,1), 0.5) )
        self.Doc.recompute()
        self.assertAlmostEqual(helix.Shape.Volume, 100.63088303433108,places=5)


    def testRectangle(self):
        body = self.Doc.addObject('PartDesign::Body','GearBody')
        gearSketch = self.Doc.addObject('Sketcher::SketchObject', 'ConeSketch')
        body.addObject(gearSketch)
        TestSketcherApp.CreateRectangleSketch(gearSketch, (0, 0), (5, 5))
        self.Doc.recompute()

        # xz_plane = body.Origin.OriginFeatures[4]
        # coneSketch.Support = xz_plane
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
        self.assertAlmostEqual(helix.Shape.Volume, 1178.0961742825648,places=5)


    def testCone(self):
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
        coneSketch.Support = xz_plane
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
        self.assertAlmostEqual(helix.Shape.Volume, 388285.4117046908,places=5)

    def tearDown(self):
        FreeCAD.closeDocument("PartDesignTestHelix")

