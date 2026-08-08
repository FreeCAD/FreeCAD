# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

import FreeCAD
import Part


class TestPointPattern(unittest.TestCase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("PartDesignTestPointPattern")

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def testPointPatternCreatesCopiesAtPointCoordinates(self):
        body = self.doc.addObject("PartDesign::Body", "Body")
        points = body.newObject("PartDesign::Feature", "Points")
        points.Shape = Part.makeCompound(
            [
                Part.Vertex(FreeCAD.Vector(5, 5, 5)),
                Part.Vertex(FreeCAD.Vector(10, 5, 5)),
                Part.Vertex(FreeCAD.Vector(5, 10, 5)),
            ]
        )

        box = body.newObject("PartDesign::AdditiveBox", "Box")
        box.Length = 2
        box.Width = 2
        box.Height = 2
        self.doc.recompute()

        pattern = body.newObject("PartDesign::PointPattern", "PointPattern")
        pattern.Originals = [box]
        pattern.PointObject = points
        self.doc.recompute()

        self.assertEqual(pattern.getStatusString(), "Valid")
        self.assertFalse(pattern.Shape.isNull())
        self.assertAlmostEqual(pattern.Shape.Volume, 3 * box.Shape.Volume)
        self.assertAlmostEqual(pattern.Shape.BoundBox.XMin, 5)
        self.assertAlmostEqual(pattern.Shape.BoundBox.YMin, 5)
        self.assertAlmostEqual(pattern.Shape.BoundBox.ZMin, 5)
        self.assertAlmostEqual(pattern.Shape.BoundBox.XMax, 12)
        self.assertAlmostEqual(pattern.Shape.BoundBox.YMax, 12)
        self.assertAlmostEqual(pattern.Shape.BoundBox.ZMax, 7)

    def testPointPatternInMultiTransform(self):
        body = self.doc.addObject("PartDesign::Body", "Body")
        points = body.newObject("PartDesign::Feature", "Points")
        points.Shape = Part.makeCompound(
            [
                Part.Vertex(FreeCAD.Vector()),
                Part.Vertex(FreeCAD.Vector(5, 0, 0)),
            ]
        )

        box = body.newObject("PartDesign::AdditiveBox", "Box")
        box.Length = 1
        box.Width = 1
        box.Height = 1
        self.doc.recompute()

        multi = body.newObject("PartDesign::MultiTransform", "MultiTransform")
        multi.Originals = [box]
        multi.Shape = box.Shape

        point_pattern = body.newObject("PartDesign::PointPattern", "PointPattern")
        point_pattern.PointObject = points
        multi.Transformations = [point_pattern]
        self.doc.recompute()

        self.assertEqual(multi.getStatusString(), "Valid")
        self.assertAlmostEqual(multi.Shape.Volume, 2 * box.Shape.Volume)
