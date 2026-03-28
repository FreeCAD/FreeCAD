# SPDX-License-Identifier: LGPL-2.1-or-later

import math
import unittest

import FreeCAD
import Part
import Sketcher


class TestExtrusion(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartExtrusionTest")

    def tearDown(self):
        FreeCAD.closeDocument(self.Doc.Name)

    def _makeOverlappingSketch(self):
        """Create a sketch with overlapping rectangle (-20,-15)-(15,15) + circle at (10,0) r=15."""
        sk = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        i = int(sk.GeometryCount)
        sk.addGeometry(Part.LineSegment(FreeCAD.Vector(-20, -15, 0), FreeCAD.Vector(15, -15, 0)))
        sk.addGeometry(Part.LineSegment(FreeCAD.Vector(15, -15, 0), FreeCAD.Vector(15, 15, 0)))
        sk.addGeometry(Part.LineSegment(FreeCAD.Vector(15, 15, 0), FreeCAD.Vector(-20, 15, 0)))
        sk.addGeometry(Part.LineSegment(FreeCAD.Vector(-20, 15, 0), FreeCAD.Vector(-20, -15, 0)))
        sk.addConstraint(Sketcher.Constraint("Coincident", i, 2, i + 1, 1))
        sk.addConstraint(Sketcher.Constraint("Coincident", i + 1, 2, i + 2, 1))
        sk.addConstraint(Sketcher.Constraint("Coincident", i + 2, 2, i + 3, 1))
        sk.addConstraint(Sketcher.Constraint("Coincident", i + 3, 2, i, 1))
        sk.addGeometry(Part.Circle(FreeCAD.Vector(10, 0, 0), FreeCAD.Vector(0, 0, 1), 15), False)
        self.Doc.recompute()
        return sk

    def testExtrudeOverlappingSolid(self):
        """Part Extrude of overlapping sketch should produce a single valid solid."""
        sk = self._makeOverlappingSketch()
        ext = self.Doc.addObject("Part::Extrusion", "Extrude")
        ext.Base = sk
        ext.LengthFwd = 10
        ext.Solid = True
        self.Doc.recompute()
        self.assertFalse(ext.Shape.isNull())
        self.assertEqual(len(ext.Shape.Solids), 1, "Should produce exactly 1 solid")
        self.assertTrue(ext.Shape.Solids[0].isValid())

    def testExtrudeOverlappingVolume(self):
        """Extruded volume should equal union area × height, not sum of individual areas."""
        sk = self._makeOverlappingSketch()
        ext = self.Doc.addObject("Part::Extrusion", "Extrude")
        ext.Base = sk
        ext.LengthFwd = 10
        ext.Solid = True
        self.Doc.recompute()
        # Compute expected union area via Part boolean
        rect = Part.Face(
            Part.makePolygon(
                [
                    FreeCAD.Vector(-20, -15, 0),
                    FreeCAD.Vector(15, -15, 0),
                    FreeCAD.Vector(15, 15, 0),
                    FreeCAD.Vector(-20, 15, 0),
                    FreeCAD.Vector(-20, -15, 0),
                ]
            )
        )
        circle = Part.Face(Part.Wire(Part.makeCircle(15, FreeCAD.Vector(10, 0, 0))))
        union = rect.fuse(circle)
        expected_volume = sum(f.Area for f in union.Faces) * 10
        self.assertAlmostEqual(ext.Shape.Volume, expected_volume, delta=1.0)
        # Volume must be less than naive sum (no double counting)
        naive_sum = (35 * 30 + math.pi * 15 * 15) * 10
        self.assertLess(ext.Shape.Volume, naive_sum)

    def testExtrudeSimpleRectangle(self):
        """Part Extrude of a simple rectangle should produce correct volume."""
        sk = self.Doc.addObject("Sketcher::SketchObject", "Sketch")
        i = int(sk.GeometryCount)
        sk.addGeometry(Part.LineSegment(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(10, 0, 0)))
        sk.addGeometry(Part.LineSegment(FreeCAD.Vector(10, 0, 0), FreeCAD.Vector(10, 10, 0)))
        sk.addGeometry(Part.LineSegment(FreeCAD.Vector(10, 10, 0), FreeCAD.Vector(0, 10, 0)))
        sk.addGeometry(Part.LineSegment(FreeCAD.Vector(0, 10, 0), FreeCAD.Vector(0, 0, 0)))
        sk.addConstraint(Sketcher.Constraint("Coincident", i, 2, i + 1, 1))
        sk.addConstraint(Sketcher.Constraint("Coincident", i + 1, 2, i + 2, 1))
        sk.addConstraint(Sketcher.Constraint("Coincident", i + 2, 2, i + 3, 1))
        sk.addConstraint(Sketcher.Constraint("Coincident", i + 3, 2, i, 1))
        self.Doc.recompute()
        ext = self.Doc.addObject("Part::Extrusion", "Extrude")
        ext.Base = sk
        ext.LengthFwd = 10
        ext.Solid = True
        self.Doc.recompute()
        self.assertEqual(len(ext.Shape.Solids), 1)
        self.assertAlmostEqual(ext.Shape.Volume, 1000.0, places=1)

    def testExtrudeNonSolid(self):
        """Non-solid extrude should not be affected by solid fuse logic."""
        sk = self._makeOverlappingSketch()
        ext = self.Doc.addObject("Part::Extrusion", "Extrude")
        ext.Base = sk
        ext.LengthFwd = 10
        ext.Solid = False
        self.Doc.recompute()
        self.assertFalse(ext.Shape.isNull())
        self.assertEqual(len(ext.Shape.Solids), 0)
