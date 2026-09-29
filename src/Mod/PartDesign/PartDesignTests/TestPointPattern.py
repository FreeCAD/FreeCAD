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

    def testPointPatternPreservesSourceOrientation(self):
        body = self.doc.addObject("PartDesign::Body", "Body")
        points = body.newObject("PartDesign::Feature", "Points")
        points.Shape = Part.makeCompound(
            [Part.Vertex(FreeCAD.Vector(5, 5, 5)), Part.Vertex(FreeCAD.Vector(10, 5, 5))]
        )
        box = body.newObject("PartDesign::AdditiveBox", "Box")
        box.Length = 2
        box.Width = 1
        box.Height = 1
        box.Placement = FreeCAD.Placement(
            FreeCAD.Vector(20, 30, 40), FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 90)
        )
        self.doc.recompute()
        pattern = body.newObject("PartDesign::PointPattern", "PointPattern")
        pattern.Originals = [box]
        pattern.PointObject = points
        self.doc.recompute()

        self.assertEqual(pattern.getStatusString(), "Valid")
        self.assertAlmostEqual(pattern.Shape.Volume, 2 * box.Shape.Volume)
        bounds = pattern.Shape.BoundBox
        for actual, expected in zip(
            (bounds.XMin, bounds.YMin, bounds.ZMin, bounds.XMax, bounds.YMax, bounds.ZMax),
            (4, 5, 5, 10, 7, 6),
        ):
            self.assertAlmostEqual(actual, expected)

    def testPointPatternInMultiTransform(self):
        body = self.doc.addObject("PartDesign::Body", "Body")
        points = body.newObject("PartDesign::Feature", "Points")
        points.Shape = Part.makeCompound(
            [
                Part.Vertex(FreeCAD.Vector(5, 5, 5)),
                Part.Vertex(FreeCAD.Vector(10, 5, 5)),
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
        self.assertAlmostEqual(multi.Shape.BoundBox.XMin, 5)
        self.assertAlmostEqual(multi.Shape.BoundBox.YMin, 5)
        self.assertAlmostEqual(multi.Shape.BoundBox.ZMin, 5)

    def testPointPatternInMultiTransformPreservesSourceOrientation(self):
        body = self.doc.addObject("PartDesign::Body", "Body")
        points = body.newObject("PartDesign::Feature", "Points")
        points.Shape = Part.makeCompound(
            [Part.Vertex(FreeCAD.Vector(5, 5, 5)), Part.Vertex(FreeCAD.Vector(10, 5, 5))]
        )
        box = body.newObject("PartDesign::AdditiveBox", "Box")
        box.Length = 2
        box.Width = 1
        box.Height = 1
        box.Placement = FreeCAD.Placement(
            FreeCAD.Vector(20, 30, 40), FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 90)
        )
        self.doc.recompute()
        pattern = body.newObject("PartDesign::MultiTransform", "MultiTransform")
        pattern.Originals = [box]
        pattern.Shape = box.Shape
        helper = body.newObject("PartDesign::PointPattern", "PointPattern")
        helper.PointObject = points
        pattern.Transformations = [helper]
        self.doc.recompute()

        self.assertEqual(pattern.getStatusString(), "Valid")
        self.assertAlmostEqual(pattern.Shape.Volume, 2 * box.Shape.Volume)
        bounds = pattern.Shape.BoundBox
        for actual, expected in zip(
            (bounds.XMin, bounds.YMin, bounds.ZMin, bounds.XMax, bounds.YMax, bounds.ZMax),
            (4, 5, 5, 10, 7, 6),
        ):
            self.assertAlmostEqual(actual, expected)
