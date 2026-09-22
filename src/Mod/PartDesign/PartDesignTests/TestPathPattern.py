# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

import FreeCAD
import Part


class TestPathPattern(unittest.TestCase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("PartDesignTestPathPattern")

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def testPathPatternCreatesCopiesAlongPath(self):
        body = self.doc.addObject("PartDesign::Body", "Body")
        path = self.doc.addObject("PartDesign::Feature", "Path")
        path.Shape = Part.makePolygon(
            [FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(4, 0, 0), FreeCAD.Vector(4, 4, 0)]
        )
        body.addObject(path)

        box = self.doc.addObject("PartDesign::AdditiveBox", "Box")
        body.addObject(box)
        box.Length = 2
        box.Width = 2
        box.Height = 2

        self.doc.recompute()

        pattern = self.doc.addObject("PartDesign::PathPattern", "PathPattern")
        pattern.Originals = [box]
        pattern.Path = (path, ["Edge1", "Edge2"])
        pattern.Count = 3
        body.addObject(pattern)
        self.doc.recompute()

        self.assertEqual(pattern.getStatusString(), "Valid")
        self.assertFalse(pattern.Shape.isNull())
        self.assertGreater(pattern.Shape.Volume, box.Shape.Volume)

    def testReverseAndAlignUseRelativePathFrames(self):
        body = self.doc.addObject("PartDesign::Body", "Body")
        path = self.doc.addObject("PartDesign::Feature", "Path")
        path.Shape = Part.makePolygon(
            [FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(10, 0, 0), FreeCAD.Vector(10, 10, 0)]
        )
        body.addObject(path)

        box = self.doc.addObject("PartDesign::AdditiveBox", "Box")
        body.addObject(box)
        box.Length = 1
        box.Width = 1
        box.Height = 1
        self.doc.recompute()

        pattern = self.doc.addObject("PartDesign::PathPattern", "PathPattern")
        pattern.Originals = [box]
        pattern.Path = (path, ["Edge1", "Edge2"])
        pattern.Count = 3
        pattern.ReversePath = True
        pattern.Align = True
        body.addObject(pattern)
        self.doc.recompute()

        self.assertEqual(pattern.getStatusString(), "Valid")
        self.assertFalse(pattern.Shape.isNull())

    def testSubShapeBinderCanProvidePath(self):
        body = self.doc.addObject("PartDesign::Body", "Body")
        path = body.newObject("PartDesign::Feature", "Path")
        path.Shape = Part.makePolygon(
            [FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(5, 0, 0), FreeCAD.Vector(5, 5, 5)]
        )
        binder = body.newObject("PartDesign::SubShapeBinder", "PathBinder")
        binder.Support = [(path, ("Edge1", "Edge2"))]

        box = body.newObject("PartDesign::AdditiveBox", "Box")
        box.Length = 1
        box.Width = 1
        box.Height = 1
        self.doc.recompute()

        pattern = body.newObject("PartDesign::PathPattern", "PathPattern")
        pattern.Originals = [box]
        pattern.Path = binder
        pattern.Count = 3
        self.doc.recompute()

        self.assertEqual(pattern.getStatusString(), "Valid")
        self.assertFalse(pattern.Shape.isNull())
