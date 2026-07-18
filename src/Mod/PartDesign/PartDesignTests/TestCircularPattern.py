# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

import FreeCAD


class TestCircularPattern(unittest.TestCase):
    def setUp(self):
        self.doc = FreeCAD.newDocument("PartDesignTestCircularPattern")

    def tearDown(self):
        FreeCAD.closeDocument(self.doc.Name)

    def testCircularPatternProperties(self):
        pattern = self.doc.addObject("PartDesign::CircularPattern", "CircularPattern")

        self.assertEqual(pattern.RadialDistance, 50)
        self.assertEqual(pattern.TangentialDistance, 25)
        self.assertEqual(pattern.getTypeIdOfProperty("RadialDistance"), "App::PropertyLength")
        self.assertEqual(pattern.getTypeIdOfProperty("TangentialDistance"), "App::PropertyLength")
        pattern.RadialDistance = 0.00054
        self.assertAlmostEqual(pattern.RadialDistance, 0.00054)
        self.assertEqual(pattern.NumberCircles, 3)
        self.assertEqual(pattern.Symmetry, 1)
        self.assertNotIn("Angle", pattern.PropertiesList)
        self.assertNotIn("Occurrences", pattern.PropertiesList)

    def testCircularPatternCreatesConcentricCopies(self):
        body = self.doc.addObject("PartDesign::Body", "Body")
        box = self.doc.addObject("PartDesign::AdditiveBox", "Box")
        body.addObject(box)
        box.Length = 10
        box.Width = 10
        box.Height = 10
        self.doc.recompute()

        pattern = self.doc.addObject("PartDesign::CircularPattern", "CircularPattern")
        pattern.Originals = [box]
        pattern.Axis = (self.doc.Z_Axis, [""])
        pattern.RadialDistance = 10
        pattern.TangentialDistance = 15
        pattern.NumberCircles = 2
        pattern.Symmetry = 4
        body.addObject(pattern)
        self.doc.recompute()

        self.assertEqual(pattern.getStatusString(), "Valid")
        self.assertFalse(pattern.Shape.isNull())
        self.assertGreater(pattern.Shape.Volume, box.Shape.Volume)

    def testCircularPatternInMultiTransform(self):
        body = self.doc.addObject("PartDesign::Body", "Body")
        box = self.doc.addObject("PartDesign::AdditiveBox", "Box")
        body.addObject(box)
        box.Length = 10
        box.Width = 10
        box.Height = 10
        self.doc.recompute()

        multi = self.doc.addObject("PartDesign::MultiTransform", "MultiTransform")
        multi.Originals = [box]
        multi.Shape = box.Shape
        body.addObject(multi)

        circular = self.doc.addObject("PartDesign::CircularPattern", "CircularPattern")
        circular.Axis = (self.doc.Z_Axis, [""])
        circular.RadialDistance = 20
        circular.TangentialDistance = 20
        circular.NumberCircles = 2
        circular.Symmetry = 4
        body.addObject(circular)

        multi.Transformations = [circular]
        self.doc.recompute()

        self.assertEqual(multi.getStatusString(), "Valid")
        self.assertAlmostEqual(multi.Shape.Volume, 5 * box.Shape.Volume)
