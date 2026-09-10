# SPDX-License-Identifier: LGPL-2.1-or-later
"""Lifecycle checks for the Rib template; geometry is not implemented yet."""

import os
import tempfile
import unittest

import FreeCAD as App
import Part


class TestRib(unittest.TestCase):
    def setUp(self):
        self.doc = App.newDocument("TestRib")
        self.body = self.doc.addObject("PartDesign::Body", "Body")
        self.base = self.body.newObject("PartDesign::Feature", "Base")
        self.base.Shape = Part.makeBox(20, 20, 5)
        self.profile = self.body.newObject("Sketcher::SketchObject", "Profile")
        self.profile.addGeometry(
            Part.LineSegment(App.Vector(2, 2, 5), App.Vector(18, 18, 5)), False
        )
        self.doc.recompute()
        self.rib = self.body.newObject("PartDesign::Rib", "Rib")
        self.rib.Profile = (self.profile, [""])

    def tearDown(self):
        App.closeDocument(self.doc.Name)

    def testFeatureContract(self):
        self.assertTrue(self.rib.isDerivedFrom("PartDesign::ProfileBased"))
        self.assertTrue(self.rib.isDerivedFrom("PartDesign::FeatureAddSub"))
        self.assertFalse(self.rib.isDerivedFrom("PartDesign::FeatureExtrude"))
        self.assertEqual(self.rib.BaseFeature, self.base)
        self.assertEqual(self.rib.Operation, "Union")
        for name in ("ThinThickness", "Extension", "AutoDirection", "RootFilletRadius"):
            self.assertNotIn(name, self.rib.PropertiesList)

    def testConstructionIsExplicitlyUnimplemented(self):
        before = self.base.Shape.exportBrepToString()
        self.doc.recompute()
        self.assertIn("Invalid", self.rib.State)
        self.assertIn("not implemented", self.rib.getStatusString())
        self.assertTrue(self.rib.Shape.isNull())
        self.assertTrue(self.rib.AddSubShape.isNull())
        self.assertEqual(before, self.base.Shape.exportBrepToString())

    def testProfileLinkAndRecompute(self):
        self.doc.recompute()
        self.rib.purgeTouched()
        self.rib.Profile = (self.profile, ["Edge1"])
        self.assertIn("Touched", self.rib.State)
        self.doc.recompute()
        self.assertEqual(self.rib.Profile[1], ["Edge1"])
        self.assertIn("not implemented", self.rib.getStatusString())

    def testSaveRestore(self):
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "RibTemplate.FCStd")
            self.doc.saveAs(path)
            App.closeDocument(self.doc.Name)
            self.doc = App.openDocument(path)
            rib = self.doc.getObject("Rib")
            self.assertEqual(rib.TypeId, "PartDesign::Rib")
            self.assertEqual(rib.Profile[0], self.doc.Profile)
            rib.touch()
            self.doc.recompute()
            self.assertIn("not implemented", rib.getStatusString())
