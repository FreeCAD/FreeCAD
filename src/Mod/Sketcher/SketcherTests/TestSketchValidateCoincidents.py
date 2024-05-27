# **************************************************************************
#   Copyright (c) 2024 Syres                                               *
#                                                                          *
#   This file is part of the FreeCAD CAx development system.               *
#                                                                          *
#   FreeCAD is free software: you can redistribute it and/or modify it     *
#   under the terms of the GNU Lesser General Public License as            *
#   published by the Free Software Foundation, either version 2.1 of the   *
#   License, or (at your option) any later version.                        *
#                                                                          *
#   FreeCAD is distributed in the hope that it will be useful, but         *
#   WITHOUT ANY WARRANTY; without even the implied warranty of             *
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#   Lesser General Public License for more details.                        *
#                                                                          *
#   You should have received a copy of the GNU Lesser General Public       *
#   License along with FreeCAD. If not, see                                *
#   <https://www.gnu.org/licenses/>.                                       *
# **************************************************************************

import unittest
import FreeCAD
import Part
import Sketcher
from FreeCAD import Vector

App = FreeCAD


class TestSketchValidateCoincidents(unittest.TestCase):
    """Sketch validation test"""

    def setUp(self):
        self.Doc = FreeCAD.newDocument("SketchValidateCoincidentsTest")

    def testSingleMissingCoincidentCase(self):
        sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        v0 = Vector(-47.680691, 18.824165000000004, 0.0)
        v1 = Vector(-47.680691, -27.346279, 0.0)
        v2 = Vector(51.132679, -27.346279, 0.0)
        v3 = Vector(51.132679, 18.824165000000004, 0.0)
        v4 = Vector(51.132679, 18.824165, 0.0)
        v5 = Vector(-47.680691, 18.824165, 0.0)

        geo0 = sketch.addGeometry(Part.LineSegment(v0, v1))
        geo1 = sketch.addGeometry(Part.LineSegment(v1, v2))
        geo2 = sketch.addGeometry(Part.LineSegment(v2, v3))
        geo3 = sketch.addGeometry(Part.LineSegment(v4, v5))

        sketch.addConstraint(Sketcher.Constraint("Coincident", geo0, 2, geo1, 1))
        sketch.addConstraint(Sketcher.Constraint("Coincident", geo1, 2, geo2, 1))
        sketch.addConstraint(Sketcher.Constraint("Vertical", geo0))
        sketch.addConstraint(Sketcher.Constraint("Vertical", geo2))
        sketch.addConstraint(Sketcher.Constraint("Horizontal", geo1))
        sketch.addConstraint(Sketcher.Constraint("Coincident", geo3, 1, geo2, 2))
        sketch.addConstraint(Sketcher.Constraint("Horizontal", geo3))
        self.Doc.recompute()
        self.assertEqual(sketch.ConstraintCount, 7)
        sketch.detectMissingPointOnPointConstraints()
        sketch.makeMissingPointOnPointCoincident()
        self.Doc.recompute()
        self.assertEqual(sketch.ConstraintCount, 8)
        del v0, v1, v2, v3, v4, v5
        del geo0, geo1, geo2, geo3
        del sketch

    def testDegenratedGeometryCase(self):
        sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        v0 = Vector(-47.680691, 18.824165000000004, 0.0)
        v1 = Vector(-47.680691, -27.346279, 0.0)
        v2 = Vector(-47.680691, -27.34627900001, 0.0)

        geo0 = sketch.addGeometry(Part.LineSegment(v0, v1))
        geo1 = sketch.addGeometry(Part.LineSegment(v1, v2))
        sketch.addConstraint(Sketcher.Constraint("Coincident", geo0, 2, geo1, 1))
        self.Doc.recompute()
        tol = 1.0e-8
        self.assertEqual(sketch.ConstraintCount, 1)
        self.assertEqual(sketch.detectDegeneratedGeometries(tol), 1)
        self.assertEqual(sketch.removeDegeneratedGeometries(tol), 1)
        self.assertEqual(sketch.detectDegeneratedGeometries(tol), 0)
        self.assertEqual(sketch.ConstraintCount, 0)
        del v0, v1, v2
        del geo0, geo1
        del sketch

    def testDeleteConstraintsToExternalCase(self):
        box = self.Doc.addObject("Part::Box", "Box")
        sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        self.Doc.recompute()
        sketch.addExternal("Box", "Edge12")

        v0 = Vector(-47.680691, 18.824165000000004, 0.0)
        v1 = Vector(-47.680691, -27.346279, 0.0)
        geo0 = sketch.addGeometry(Part.LineSegment(v0, v1))
        sketch.addConstraint(Sketcher.Constraint("Horizontal", geo0))
        sketch.addConstraint(Sketcher.Constraint("Equal", -3, geo0))
        self.Doc.recompute()
        self.assertEqual(sketch.ConstraintCount, 2)
        sketch.delConstraintsToExternal()
        self.Doc.recompute()
        self.assertEqual(sketch.ConstraintCount, 1)
        del v0, v1
        del geo0
        del sketch
        del box

    def testValidateConstraintsCase(self):
        sketch = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        v0 = Vector(-47.680691, 18.824165000000004, 0.0)
        v1 = Vector(-47.680691, -27.346279, 0.0)
        v2 = Vector(51.132679, -27.346279, 0.0)
        geo0 = sketch.addGeometry(Part.LineSegment(v0, v1))
        geo1 = sketch.addGeometry(Part.LineSegment(v1, v2))
        sketch.addConstraint(Sketcher.Constraint("Coincident", geo0, 2, geo1, 1))

        c = sketch.Constraints[0]
        c.First = 2
        sketch.Constraints = [c]
        self.assertEqual(sketch.ConstraintCount, 1)
        self.assertFalse(sketch.evaluateConstraints())

        sketch.validateConstraints()

        self.assertEqual(sketch.ConstraintCount, 0)
        self.assertTrue(sketch.evaluateConstraints())
        del v0, v1, v2
        del geo0, geo1
        del sketch, c

    def tearDown(self):
        # closing doc
        FreeCAD.closeDocument(self.Doc.Name)
