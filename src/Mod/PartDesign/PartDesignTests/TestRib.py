# SPDX-License-Identifier: LGPL-2.1-or-later
"""Lifecycle, input-validation, and geometry checks for the Rib feature."""

import math
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
        self.assertEqual(self.rib.ExtendType, "C1")
        self.assertEqual(self.rib.PlacementType, "Centered")
        self.assertEqual(self.rib.ExtentType, "Shape")
        self.assertEqual(self.rib.Thickness.Value, 2.0)
        self.assertEqual(self.rib.getTypeIdOfProperty("Thickness"), "App::PropertyLength")
        self.assertEqual(self.rib.Distance, 0.0)
        self.assertEqual(self.rib.getTypeIdOfProperty("Distance"), "App::PropertyFloat")
        self.assertEqual(self.rib.Direction, App.Vector(0, 0, -1))
        self.assertFalse(self.rib.UseCustomPullDirection)
        self.assertEqual(self.rib.PullDirection, App.Vector(0, 0, 1))
        for name in (
            "Extend", "ThinThickness", "Extension", "AutoDirection",
            "RootFilletRadius",
        ):
            self.assertNotIn(name, self.rib.PropertiesList)

    def testEnumTables(self):
        for name, values in (
            ("ExtendType", ["Off", "C1", "C2"]),
            ("PlacementType", ["Side A", "Side B", "Centered"]),
            ("ExtentType", ["Shape", "Distance"]),
        ):
            with self.subTest(property=name):
                self.assertEqual(self.rib.getTypeIdOfProperty(name), "App::PropertyEnumeration")
                self.assertEqual(self.rib.getEnumerationsOfProperty(name), values)

    def testShapeDirectionNormalToSketchRejected(self):
        before = self.base.Shape.exportBrepToString()
        self.doc.recompute()
        self.assertIn("Invalid", self.rib.State)
        self.assertIn("Rib direction must lie in the sketch plane", self.rib.getStatusString())
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
        self.assertIn("Rib direction must lie in the sketch plane", self.rib.getStatusString())

    def testExtendTypeTriggersRecompute(self):
        for value in ("Off", "C2", "C1"):
            with self.subTest(extendType=value):
                self.doc.recompute()
                self.rib.purgeTouched()
                self.rib.ExtendType = value
                self.assertIn("Touched", self.rib.State)
                self.doc.recompute()
                self.assertEqual(self.rib.ExtendType, value)
                self.assertIn(
                    "Rib direction must lie in the sketch plane",
                    self.rib.getStatusString(),
                )

    def assertExtensionReachesDirectionValidation(self, extendTypes=("C1", "C2")):
        # Extension runs before sweep validation. Require this specific later error
        # rather than relying on unrelated cut/fuse validity to prove acceptance.
        self.rib.Direction = App.Vector(0, 0, -1)
        before = self.base.Shape.exportBrepToString()
        for continuity in extendTypes:
            with self.subTest(continuity=continuity):
                self.rib.ExtendType = continuity
                self.doc.recompute()
                self.assertIn("Invalid", self.rib.State)
                self.assertIn(
                    "Rib direction must lie in the sketch plane",
                    self.rib.getStatusString(),
                )
                self.assertTrue(self.rib.Shape.isNull())
                self.assertTrue(self.rib.AddSubShape.isNull())
                self.assertEqual(before, self.base.Shape.exportBrepToString())

    def testExtensionAcceptsEndpointsOnBody(self):
        self.profile.delGeometry(0)
        self.profile.Placement.Base = App.Vector(0, 0, 2.5)
        # Each outward portion leaves the box immediately: only its original
        # endpoint supplies contact, which must count for both C1 and C2.
        self.profile.addGeometry(
            Part.LineSegment(App.Vector(0, 10, 0), App.Vector(20, 10, 0)), False
        )
        self.assertExtensionReachesDirectionValidation()

    def testExtensionAcceptsOutwardContact(self):
        floor = Part.makeBox(20, 20, 5, App.Vector(0, 0, -5))
        left = Part.makeBox(2, 20, 5)
        right = Part.makeBox(2, 20, 5, App.Vector(18, 0, 0))
        self.base.Shape = floor.fuse(left).fuse(right).removeSplitter()
        self.assertTrue(self.base.Shape.isValid())
        self.assertEqual(len(self.base.Shape.Solids), 1)
        self.profile.delGeometry(0)
        self.profile.Placement.Base = App.Vector(0, 0, 2.5)
        # The original edge is clear of the body. Both extensions must cross a
        # wall, with their full-reach endpoints far beyond those walls.
        self.profile.addGeometry(
            Part.LineSegment(App.Vector(5, 10, 0), App.Vector(15, 10, 0)), False
        )
        original = Part.makeLine(App.Vector(5, 10, 2.5), App.Vector(15, 10, 2.5))
        self.assertGreater(original.distToShape(self.base.Shape)[0], 1.0)
        self.assertExtensionReachesDirectionValidation()

    def prepareExtensionContactProfile(self, startX, endX):
        self.profile.delGeometry(0)
        self.profile.Placement.Base = App.Vector(0, 0, 2.5)
        self.profile.addGeometry(
            Part.LineSegment(App.Vector(startX, 10, 0), App.Vector(endX, 10, 0)), False
        )

    def assertExtensionContactRejected(self, startX, endX, endpoint):
        self.prepareExtensionContactProfile(startX, endX)
        self.rib.Direction = App.Vector(0, 0, -1)
        before = self.base.Shape.exportBrepToString()
        message = f"Rib profile {endpoint} extension does not intersect the body within reach"
        for continuity in ("C1", "C2"):
            with self.subTest(continuity=continuity, startX=startX, endX=endX):
                self.rib.ExtendType = continuity
                self.doc.recompute()
                # These fixtures deliberately contact the body along the original
                # edge; that must not hide a missing outward extension contact.
                self.assertAlmostEqual(self.profile.Shape.distToShape(self.base.Shape)[0], 0.0)
                self.assertIn("Invalid", self.rib.State)
                self.assertIn(message, self.rib.getStatusString())
                self.assertTrue(self.rib.Shape.isNull())
                self.assertTrue(self.rib.AddSubShape.isNull())
                self.assertEqual(before, self.base.Shape.exportBrepToString())

    def testExtensionRejectsOriginalEdgeOnlyContact(self):
        # Both endpoints are outside the box and both extensions point away.
        self.assertExtensionContactRejected(-5, 25, "start")

    def testExtensionRejectsMissingStartContact(self):
        # The end touches x=20; only the start extension misses.
        self.assertExtensionContactRejected(-5, 20, "start")

    def testExtensionRejectsMissingEndContact(self):
        # The start touches x=0; only the end extension misses.
        self.assertExtensionContactRejected(0, 25, "end")

    def testExtensionDiagnosticsFollowReversedSketchEndpoints(self):
        # Reverse the single sketch segment's traversal without moving its locus.
        # The missing contact must change from start to end, and vice versa.
        self.assertExtensionContactRejected(20, -5, "end")
        self.assertExtensionContactRejected(25, 0, "start")

    def testExtensionOffSkipsContactValidation(self):
        self.prepareExtensionContactProfile(-5, 25)
        self.assertExtensionReachesDirectionValidation(("Off",))

    def testThicknessTriggersRecompute(self):
        self.doc.recompute()
        self.rib.purgeTouched()
        self.rib.Thickness = "3.5 mm"
        self.assertIn("Touched", self.rib.State)
        self.assertEqual(self.rib.Thickness.Value, 3.5)

    def testZeroThicknessRejected(self):
        self.rib.Thickness = 0
        self.doc.recompute()
        self.assertIn("Invalid", self.rib.State)
        self.assertIn("Rib thickness must be positive and finite", self.rib.getStatusString())

    def testDistanceInputValidation(self):
        self.rib.ExtentType = "Distance"
        self.rib.ExtendType = "Off"
        for direction, distance, message in (
            (App.Vector(), 10, "Rib sweep direction must be nonzero and finite"),
            (App.Vector(0, 0, -1), 10, "Rib direction must lie in the sketch plane"),
            (App.Vector(1, 0, 0), 0, "Rib sweep distance must be positive and finite"),
            (App.Vector(1, 0, 0), -1, "Rib sweep distance must be positive and finite"),
        ):
            with self.subTest(direction=direction, distance=distance):
                self.rib.Direction = direction
                self.rib.Distance = distance
                self.doc.recompute()
                self.assertIn("Invalid", self.rib.State)
                self.assertIn(message, self.rib.getStatusString())

    def prepareDraftRib(self):
        self.base.Shape = Part.makeBox(30, 20, 5, App.Vector(0, -10, -5))
        self.profile.delGeometry(0)
        self.profile.Placement = App.Placement(
            App.Vector(), App.Rotation(App.Vector(1, 0, 0), 90)
        )
        self.profile.addGeometry(
            Part.LineSegment(App.Vector(5, 20, 0), App.Vector(25, 20, 0)), False
        )
        self.rib.ExtentType = "Distance"
        self.rib.Direction = App.Vector(0, 0, -1)
        self.rib.Distance = 25
        self.rib.ExtendType = "Off"
        self.rib.Thickness = 4
        self.rib.FilletRadius = 0

    def sectionWidth(self, corners):
        points = [App.Vector(*corner) for corner in corners]
        plane = Part.Face(Part.makePolygon(points + [points[0]]))
        section = self.rib.AddSubShape.section(plane)
        self.assertFalse(section.isNull())
        self.assertGreater(len(section.Edges), 0)
        return section.BoundBox.YLength

    def testDefaultDraftWidthsFollowHeight(self):
        self.prepareDraftRib()
        # The unpadded retained box's free end is z=20. An unused custom vector
        # must not affect its neutral plane or the automatic pull direction.
        for pull in (App.Vector(0, 0, 1), App.Vector(1, 0, 0)):
            self.rib.PullDirection = pull
            self.assertFalse(self.rib.UseCustomPullDirection)
            for angle in (3, -3):
                with self.subTest(pull=pull, angle=angle):
                    self.rib.DraftAngle = angle
                    self.doc.recompute()
                    self.assertNotIn("Invalid", self.rib.State, self.rib.getStatusString())
                    self.assertTrue(self.rib.Shape.isValid())
                    self.assertEqual(len(self.rib.Shape.Solids), 1)
                    widths = {}
                    for z in (5, 15, 20):
                        widths[z] = self.sectionWidth(
                            [(0, -10, z), (30, -10, z), (30, 10, z), (0, 10, z)]
                        )
                        expected = 4 + 2 * (20 - z) * math.tan(math.radians(angle))
                        self.assertAlmostEqual(widths[z], expected, delta=1e-5)
                    self.assertGreater((widths[5] - widths[20]) * angle, 0)

    def testCustomDraftWidthsFollowPullDirection(self):
        self.prepareDraftRib()
        self.rib.UseCustomPullDirection = True
        self.rib.PullDirection = App.Vector(1, 0, 0)
        for angle in (3, -3):
            with self.subTest(angle=angle):
                self.rib.DraftAngle = angle
                self.doc.recompute()
                self.assertNotIn("Invalid", self.rib.State, self.rib.getStatusString())
                self.assertTrue(self.rib.Shape.isValid())
                self.assertEqual(len(self.rib.Shape.Solids), 1)
                # Custom pull aligns the box with X; its free/top end is x=25.
                for bottom, top in ((5, 10), (15, 19)):
                    widths = {}
                    for x in (5, 15, 25):
                        widths[x] = self.sectionWidth(
                            [(x, -10, bottom), (x, 10, bottom),
                             (x, 10, top), (x, -10, top)]
                        )
                        expected = 4 + 2 * (25 - x) * math.tan(math.radians(angle))
                        self.assertAlmostEqual(widths[x], expected, delta=1e-5)
                    self.assertGreater((widths[5] - widths[25]) * angle, 0)

    def testDraftCompactsLargeOvershoot(self):
        self.prepareDraftRib()
        # The useful rib is 20 mm high. A kilometre-long construction prism
        # must not prevent an inward draft which is valid over those 20 mm.
        self.rib.Distance = 1000000
        self.rib.DraftAngle = -5.5
        baseBefore = self.base.Shape.copy()
        self.doc.recompute()
        self.assertNotIn("Invalid", self.rib.State, self.rib.getStatusString())
        self.assertEqual(len(self.rib.Shape.Solids), 1)
        self.assertTrue(self.rib.Shape.isValid())
        self.assertAlmostEqual(self.rib.AddSubShape.BoundBox.ZMin, 0, delta=1e-5)
        self.assertAlmostEqual(self.rib.AddSubShape.BoundBox.ZMax, 20, delta=1e-5)
        expected = self.rib.Shape.cut(self.base.Shape)
        self.assertAlmostEqual(self.rib.AddSubShape.Volume, expected.Volume, delta=1e-4)
        self.assertLess(baseBefore.cut(self.base.Shape).Volume, 1e-7)
        self.assertLess(self.base.Shape.cut(baseBefore).Volume, 1e-7)

    def testDraftAlignedWithRotatedProfile(self):
        self.prepareDraftRib()
        self.rib.ExtentType = "Shape"
        self.rib.DraftAngle = 3
        self.doc.recompute()
        self.assertNotIn("Invalid", self.rib.State, self.rib.getStatusString())
        original = self.rib.Shape.copy()
        transform = App.Placement(
            App.Vector(71, -23, 42), App.Rotation(App.Vector(1, 2, 3), 47)
        )
        self.base.Shape = self.base.Shape.transformed(transform.toMatrix())
        self.profile.Placement = transform.multiply(self.profile.Placement)
        self.doc.recompute()
        self.assertNotIn("Invalid", self.rib.State, self.rib.getStatusString())
        self.assertTrue(self.rib.Shape.isValid())
        expected = original.transformed(transform.toMatrix())
        self.assertLess(expected.cut(self.rib.Shape).Volume, 1e-5)
        self.assertLess(self.rib.Shape.cut(expected).Volume, 1e-5)

    def testShapeDirectionUnaffectedByAsymmetricBosses(self):
        self.prepareDraftRib()
        floor = Part.makeBox(70, 40, 5, App.Vector(-30, -20, -5))
        largeBoss = Part.makeCylinder(12, 60, App.Vector(-15, 0, -5))
        smallBoss = Part.makeCylinder(4, 12, App.Vector(34, 0, -5))
        self.base.Shape = floor.fuse([largeBoss, smallBoss]).removeSplitter()
        self.rib.ExtentType = "Shape"
        self.rib.Direction = App.Vector(0, 0, -1)
        self.rib.DraftAngle = 3
        self.doc.recompute()
        self.assertNotIn("Invalid", self.rib.State, self.rib.getStatusString())
        self.assertTrue(self.rib.Shape.isValid())
        self.assertEqual(len(self.rib.Shape.Solids), 1)

        # A tall off-centre boss shifts the body's centroid, but the selected
        # downward vector must still give horizontal neutral planes and caps.
        for z in (5, 15, 20):
            width = self.sectionWidth(
                [(4, -10, z), (26, -10, z), (26, 10, z), (4, 10, z)]
            )
            expected = 4 + 2 * (20 - z) * math.tan(math.radians(3))
            self.assertAlmostEqual(width, expected, delta=1e-5)
        self.assertAlmostEqual(self.rib.AddSubShape.BoundBox.ZMin, 0, delta=1e-5)
        self.assertAlmostEqual(self.rib.AddSubShape.BoundBox.ZMax, 20, delta=1e-5)

        # Editing the vector must trigger recomputation even in Shape mode.
        self.rib.purgeTouched()
        self.rib.Direction = App.Vector()
        self.assertIn("Touched", self.rib.State)
        self.doc.recompute()
        self.assertIn("Rib sweep direction must be nonzero and finite", self.rib.getStatusString())

    def testDraftOnSlopingRoot(self):
        self.prepareDraftRib()
        # The attachment varies in height across the rib. The box's root plane
        # is not the attachment surface, so draft needs overshoot before recutting.
        floor = Part.makeBox(60, 40, 5, App.Vector(-15, -20, -5))
        floor.rotate(App.Vector(), App.Vector(0, 1, 0), -10)
        self.base.Shape = floor
        self.rib.Distance = 100
        for angle in (3, -3):
            with self.subTest(angle=angle):
                self.rib.DraftAngle = angle
                self.doc.recompute()
                self.assertNotIn("Invalid", self.rib.State, self.rib.getStatusString())
                self.assertTrue(self.rib.Shape.isValid())
                self.assertEqual(len(self.rib.Shape.Solids), 1)
                # AddSubShape is Rib-local; the feature placement follows the
                # rotated base. Put the addition in the body's world frame.
                addition = self.rib.AddSubShape.copy()
                addition.Placement = self.rib.Placement.multiply(addition.Placement)
                self.assertLess(addition.common(floor).Volume, 1e-7)
                expected = self.rib.Shape.cut(floor)
                self.assertAlmostEqual(addition.Volume, expected.Volume, delta=1e-4)

    def testJunctionFillet(self):
        # An XZ rib joins a floor and wall. Use a finite sweep so the tool stays
        # small enough to exercise both signed draft angles as well as filleting.
        floor = Part.makeBox(40, 20, 5, App.Vector(-5, -10, -5))
        wall = Part.makeBox(5, 20, 40, App.Vector(-5, -10, -5))
        self.base.Shape = floor.fuse(wall).removeSplitter()
        self.profile.delGeometry(0)
        self.profile.Placement = App.Placement(
            App.Vector(), App.Rotation(App.Vector(1, 0, 0), 90)
        )
        self.profile.addGeometry(
            Part.LineSegment(App.Vector(0, 20, 0), App.Vector(20, 0, 0)), False
        )
        self.rib.ExtendType = "Off"
        self.rib.ExtentType = "Distance"
        self.rib.Distance = 8
        self.rib.Direction = App.Vector(-1, 0, -1)
        self.rib.Thickness = 4
        for draft in (0, 1, -1):
            with self.subTest(draft=draft):
                self.rib.DraftAngle = draft
                self.rib.FilletRadius = 0
                self.doc.recompute()
                self.assertNotIn("Invalid", self.rib.State, self.rib.getStatusString())
                unfilleted = self.rib.Shape.copy()
                baseBefore = self.base.Shape.copy()
                self.rib.FilletRadius = 0.5
                self.doc.recompute()
                self.assertNotIn("Invalid", self.rib.State, self.rib.getStatusString())
                self.assertTrue(self.rib.Shape.isValid())
                self.assertEqual(len(self.rib.Shape.Solids), 1)
                self.assertGreater(self.rib.Shape.Volume, unfilleted.Volume)
                # AddSubShape must include the material added by the root rounds.
                expected = self.rib.Shape.cut(self.base.Shape)
                self.assertAlmostEqual(self.rib.AddSubShape.Volume, expected.Volume, places=5)
                # Compare geometry, not BREP serialization: OCCT can update shape flags.
                self.assertAlmostEqual(baseBefore.Volume, self.base.Shape.Volume, places=7)
                self.assertEqual(len(baseBefore.Faces), len(self.base.Shape.Faces))
                self.assertLess(baseBefore.cut(self.base.Shape).Volume, 1e-7)
                self.assertLess(self.base.Shape.cut(baseBefore).Volume, 1e-7)

    def testSaveRestore(self):
        self.rib.Thickness = "3.5 mm"
        self.rib.ExtendType = "C2"
        self.rib.PlacementType = "Side B"
        self.rib.ExtentType = "Distance"
        self.rib.Distance = 12.5
        self.rib.Direction = App.Vector(-2, 3, -4)
        self.rib.UseCustomPullDirection = True
        self.rib.PullDirection = App.Vector(1, 0, 0)
        self.rib.DraftAngle = -3
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "RibTemplate.FCStd")
            self.doc.saveAs(path)
            App.closeDocument(self.doc.Name)
            self.doc = App.openDocument(path)
            rib = self.doc.getObject("Rib")
            self.assertEqual(rib.TypeId, "PartDesign::Rib")
            self.assertEqual(rib.Profile[0], self.doc.Profile)
            self.assertEqual(rib.Thickness.Value, 3.5)
            self.assertEqual(rib.ExtendType, "C2")
            self.assertEqual(rib.PlacementType, "Side B")
            self.assertEqual(rib.ExtentType, "Distance")
            self.assertEqual(rib.Distance, 12.5)
            self.assertEqual(rib.Direction, App.Vector(-2, 3, -4))
            self.assertTrue(rib.UseCustomPullDirection)
            self.assertEqual(rib.PullDirection, App.Vector(1, 0, 0))
            self.assertEqual(rib.DraftAngle.Value, -3)
            for name, values in (
                ("ExtendType", ["Off", "C1", "C2"]),
                ("PlacementType", ["Side A", "Side B", "Centered"]),
                ("ExtentType", ["Shape", "Distance"]),
            ):
                self.assertEqual(rib.getEnumerationsOfProperty(name), values)
            rib.touch()
            self.doc.recompute()
            self.assertIn(
                "Rib direction must lie in the sketch plane", rib.getStatusString()
            )
