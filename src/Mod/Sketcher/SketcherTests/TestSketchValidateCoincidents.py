# **************************************************************************
#   Copyright (c) 2024 Syres                                                    *
#                                                                          *
#   This file is part of the FreeCAD CAx development system.               *
#                                                                          *
#   This program is free software; you can redistribute it and/or modify   *
#   it under the terms of the GNU Lesser General Public License (LGPL)     *
#   as published by the Free Software Foundation; either version 2 of      *
#   the License, or (at your option) any later version.                    *
#   for detail see the LICENCE text file.                                  *
#                                                                          *
#   FreeCAD is distributed in the hope that it will be useful,             *
#   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
#   GNU Library General Public License for more details.                   *
#                                                                          *
#   You should have received a copy of the GNU Library General Public      *
#   License along with FreeCAD; if not, write to the Free Software         *
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307   *
#   USA                                                                    *
# **************************************************************************

import FreeCAD, os, sys, unittest, Part, Sketcher
from FreeCAD import Vector

App = FreeCAD

class TestSketchValidateCoincidents(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("SketchValidateCoincidentsTest")

    def testSingleMissingCoincidentCase(self):
        self.Sketch = self.Doc.addObject('Sketcher::SketchObject', 'Sketch')
        geo0 = self.Sketch.addGeometry(Part.LineSegment(Vector (-47.680691, 18.824165000000004, 0.0), Vector (-47.680691, -27.346279, 0.0)))
        geo1 = self.Sketch.addGeometry(Part.LineSegment(Vector (-47.680691, -27.346279, 0.0), Vector (51.132679, -27.346279, 0.0)))
        geo2 = self.Sketch.addGeometry(Part.LineSegment(Vector (51.132679, -27.346279, 0.0), Vector (51.132679, 18.824165000000004, 0.0)))
        geo3 = self.Sketch.addGeometry(Part.LineSegment(Vector (51.132679, 18.824165, 0.0), Vector (-47.680691, 18.824165, 0.0)))
        self.Sketch.addConstraint(Sketcher.Constraint('Coincident', geo0, 2, geo1, 1))
        self.Sketch.addConstraint(Sketcher.Constraint('Coincident', geo1, 2, geo2, 1))
        self.Sketch.addConstraint(Sketcher.Constraint('Vertical', geo0))
        self.Sketch.addConstraint(Sketcher.Constraint('Vertical', geo2))
        self.Sketch.addConstraint(Sketcher.Constraint('Horizontal', geo1))
        self.Sketch.addConstraint(Sketcher.Constraint('Coincident', geo3, 1, geo2, 2))
        self.Sketch.addConstraint(Sketcher.Constraint('Horizontal', geo3))
        self.Sketch.ViewObject.DiffuseColor = [(0.44, 0.91, 1.00, 0.00)]
        self.Doc.recompute()
        self.assertTrue(self.Sketch.ConstraintCount == 7)
        self.Sketch.makeMissingPointOnPointCoincident()
        self.Doc.recompute()
        self.assertTrue(self.Sketch.ConstraintCount == 8)

    def tearDown(self):
        # closing doc
        FreeCAD.closeDocument(self.Doc.Name)
        # print ("omit closing document for debugging")
