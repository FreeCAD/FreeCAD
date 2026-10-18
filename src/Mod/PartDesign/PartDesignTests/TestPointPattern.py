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
                Part.Vertex(FreeCAD.Vector(5, 5, 10)),
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
        pattern.RelativeToFirst = True
        self.doc.recompute()

        self.assertEqual(pattern.getStatusString(), "Valid")
        self.assertFalse(pattern.Shape.isNull())
        self.assertAlmostEqual(pattern.Shape.Volume, 3 * box.Shape.Volume)
        self.assertAlmostEqual(pattern.Shape.BoundBox.XMin, 5)
        self.assertAlmostEqual(pattern.Shape.BoundBox.YMin, 5)
        self.assertAlmostEqual(pattern.Shape.BoundBox.ZMin, 5)
        self.assertAlmostEqual(pattern.Shape.BoundBox.XMax, 12)
        self.assertAlmostEqual(pattern.Shape.BoundBox.YMax, 12)
        self.assertAlmostEqual(pattern.Shape.BoundBox.ZMax, 12)

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
        pattern.RelativeToFirst = True
        self.doc.recompute()

        self.assertEqual(pattern.getStatusString(), "Valid")
        self.assertAlmostEqual(pattern.Shape.Volume, 2 * box.Shape.Volume)
        bounds = pattern.Shape.BoundBox
        #        print((bounds.XMin, bounds.YMin, bounds.ZMin, bounds.XMax, bounds.YMax, bounds.ZMax))
        for actual, expected in zip(
            (bounds.XMin, bounds.YMin, bounds.ZMin, bounds.XMax, bounds.YMax, bounds.ZMax),
            (14.0, 35.0, 45.0, 15.0, 42.0, 46.0),
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
        point_pattern.RelativeToFirst = True
        multi.Transformations = [point_pattern]
        self.doc.recompute()

        self.assertEqual(multi.getStatusString(), "Valid")
        self.assertAlmostEqual(multi.Shape.Volume, 2 * box.Shape.Volume)
        self.assertAlmostEqual(multi.Shape.BoundBox.XMin, 5)
        self.assertAlmostEqual(multi.Shape.BoundBox.YMin, 5)
        self.assertAlmostEqual(multi.Shape.BoundBox.ZMin, 5)
        self.assertAlmostEqual(multi.Shape.BoundBox.XMax, 11)
        self.assertAlmostEqual(multi.Shape.BoundBox.YMax, 6)
        self.assertAlmostEqual(multi.Shape.BoundBox.ZMax, 6)

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
        helper.RelativeToFirst = True
        pattern.Transformations = [helper]
        self.doc.recompute()

        self.assertEqual(pattern.getStatusString(), "Valid")
        self.assertAlmostEqual(pattern.Shape.Volume, 2 * box.Shape.Volume)
        bounds = pattern.Shape.BoundBox
        #        print((bounds.XMin, bounds.YMin, bounds.ZMin, bounds.XMax, bounds.YMax, bounds.ZMax))
        for actual, expected in zip(
            (bounds.XMin, bounds.YMin, bounds.ZMin, bounds.XMax, bounds.YMax, bounds.ZMax),
            (14.0, 35.0, 45.0, 15.0, 42.0, 46.0),
        ):
            self.assertAlmostEqual(actual, expected)
