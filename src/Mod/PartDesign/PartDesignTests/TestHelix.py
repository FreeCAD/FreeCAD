#***************************************************************************
#*   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

import unittest

import FreeCAD
from FreeCAD import Base
import Part
import Sketcher

class TestHelix(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestHelix")

    def testHelicalTubeCase(self):
        body = self.Doc.addObject('PartDesign::Body','Body')
        sketch = body.newObject('Sketcher::SketchObject','Sketch')
        sketch.Support = (self.Doc.getObject('XY_Plane'),[''])
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


    def tearDown(self):
        FreeCAD.closeDocument("PartDesignTestHelix")

