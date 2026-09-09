# SPDX-License-Identifier: LGPL-2.1-or-later
"""Actual Thin Pad/Pocket tests, independent of the reference constructions."""

import math
import os
import re
import tempfile
import unittest
import zipfile

import FreeCAD as App
import Part
import Sketcher


class TestThinExtrude(unittest.TestCase):
    def setUp(self):
        self.doc = App.newDocument("TestThinExtrude")

    def tearDown(self):
        App.closeDocument(self.doc.Name)

    def feature(self, family="line", pocket=False):
        body = self.doc.addObject("PartDesign::Body", "Body")
        if pocket:
            base = body.newObject("PartDesign::Feature", "Base")
            base.Shape = Part.makeBox(50, 50, 20, App.Vector(-10, -10, -20))
        sketch = body.newObject("Sketcher::SketchObject", "Profile")
        if family == "circle":
            sketch.addGeometry(Part.Circle(App.Vector(), App.Vector(0, 0, 1), 10), False)
        else:
            points = [(0, 0), (30, 0)]
            if family in ("corner", "rectangle"):
                points.append((30, 20))
            if family == "rectangle":
                points += [(0, 20), (0, 0)]
            for a, b in zip(points, points[1:]):
                sketch.addGeometry(Part.LineSegment(App.Vector(*a, 0), App.Vector(*b, 0)), False)
        # Construction geometry must never contribute material.
        sketch.addGeometry(Part.LineSegment(App.Vector(0, 40, 0), App.Vector(30, 40, 0)), True)
        self.doc.recompute()
        obj = body.newObject("PartDesign::Pocket" if pocket else "PartDesign::Pad", "ThinExtrude")
        obj.Profile = sketch
        obj.Thin = True
        obj.ThinThickness = 2
        obj.Length = 12
        self.doc.recompute()
        return body, sketch, obj

    def valid(self, obj):
        self.assertNotIn("Invalid", obj.State, obj.getStatusString())
        self.assertFalse(obj.Shape.isNull(), obj.getStatusString())
        self.assertTrue(obj.Shape.isValid())
        self.assertEqual(len(obj.Shape.Solids), 1)

    def junctionFeature(self, pocket=False):
        body = self.doc.addObject("PartDesign::Body", "Body")
        base = body.newObject("PartDesign::Feature", "Base")
        base.Shape = Part.makeBox(50, 50, 20, App.Vector(-10, -10, -20))
        sketch = body.newObject("Sketcher::SketchObject", "Profile")
        sketch.addGeometry(Part.LineSegment(App.Vector(), App.Vector(30, 0, 0)), False)
        wall = body.newObject("PartDesign::Pocket" if pocket else "PartDesign::Pad", "Wall")
        wall.Profile = sketch
        wall.Thin = True
        wall.ThinThickness = 2
        wall.Length = 12
        self.doc.recompute()
        self.valid(wall)
        return body, base, wall

    def checkJunctionFillet(self, pocket=False):
        body, base, wall = self.junctionFeature(pocket)
        sharp = wall.Shape.copy()
        self.assertEqual(wall.RootFilletRadius.Value, 0)
        wall.RootFilletRadius = 0.5
        self.doc.recompute()
        self.valid(wall)
        material = base.Shape.cut(wall.Shape) if pocket else wall.Shape.cut(base.Shape)
        section = material.common(Part.makeBox(1, 10, 12, App.Vector(15, -5, -12 if pocket else 0)))
        self.assertAlmostEqual(section.Volume, 24 + 2 * 0.5**2 * (1 - math.pi / 4), delta=1e-5)
        # Preview/pattern material must reproduce the dressed result exactly.
        rebuilt = base.Shape.cut(wall.AddSubShape) if pocket else base.Shape.fuse(wall.AddSubShape)
        self.assertAlmostEqual(rebuilt.cut(wall.Shape).Volume, 0, delta=1e-5)
        self.assertAlmostEqual(wall.Shape.cut(rebuilt).Volume, 0, delta=1e-5)
        if pocket:
            # Only the mouth is dressed, not the slot's interior floor/vertical edges.
            lower = Part.makeBox(50, 50, 19, App.Vector(-10, -10, -20))
            self.assertAlmostEqual(
                sharp.common(lower).Volume, wall.Shape.common(lower).Volume, delta=1e-5
            )
        wall.RootFilletRadius = 0
        self.doc.recompute()
        self.valid(wall)
        self.assertAlmostEqual(wall.Shape.Volume, sharp.Volume, delta=1e-5)

    def testThinPadJunctionFillet(self):
        self.checkJunctionFillet()

    def testFaceBasedDocumentWithoutWebProperties(self):
        """Emulate pre-Web property sets for both ordinary Pad and Pocket."""
        for pocket in (False, True):
            with self.subTest(pocket=pocket):
                body, sketch, obj = self.feature("rectangle", pocket)
                obj.Thin = False
                obj.addProperty("App::PropertyLength", "LengthDriver")
                obj.LengthDriver = 12
                obj.setExpression("Length", "LengthDriver")
                self.doc.recompute()
                self.valid(obj)
                expected = obj.Shape.copy()
                self.assertAlmostEqual(expected.Volume, 42800 if pocket else 7200, delta=1e-5)

                # Inactive Web settings cannot affect standard face-based execution.
                obj.ThinThickness = 0
                obj.ThinExtension = "Natural"
                obj.RootFilletRadius = 100
                self.doc.recompute()
                self.valid(obj)
                self.assertAlmostEqual(obj.Shape.Volume, expected.Volume, delta=1e-5)
                name = obj.Name
                with tempfile.TemporaryDirectory() as directory:
                    saved = os.path.join(directory, "Current.FCStd")
                    old = os.path.join(directory, "BeforeWeb.FCStd")
                    self.doc.saveAs(saved)
                    with zipfile.ZipFile(saved) as source, zipfile.ZipFile(old, "w") as output:
                        for info in source.infolist():
                            data = source.read(info.filename)
                            if info.filename == "Document.xml":

                                def strip_web(match):
                                    properties, removed = re.subn(
                                        rb'<Property name="(?:Thin[^"]*|RootFilletRadius)"[^>]*>.*?</Property>',
                                        b"",
                                        match[3],
                                        flags=re.DOTALL,
                                    )
                                    return (
                                        b'<Properties Count="'
                                        + str(int(match[1]) - removed).encode()
                                        + b'"'
                                        + match[2]
                                        + b">"
                                        + properties
                                        + b"</Properties>"
                                    )

                                data = re.sub(
                                    rb'<Properties Count="(\d+)"([^>]*)>(.*?)</Properties>',
                                    strip_web,
                                    data,
                                    flags=re.DOTALL,
                                )
                                self.assertNotIn(b'<Property name="Thin"', data)
                            output.writestr(info, data)
                    App.closeDocument(self.doc.Name)
                    self.doc = App.openDocument(old)
                    obj = self.doc.getObject(name)
                    self.assertFalse(obj.Thin)
                    obj.touch()
                    self.doc.recompute()
                    self.valid(obj)
                    self.assertEqual(len(obj.Shape.Faces), len(expected.Faces))
                    self.assertEqual(len(obj.Shape.Edges), len(expected.Edges))
                    for actual, original in zip(obj.Shape.Edges, expected.Edges):
                        self.assertLess(
                            actual.CenterOfMass.distanceToPoint(original.CenterOfMass), 1e-6
                        )
                        self.assertAlmostEqual(actual.Length, original.Length, delta=1e-6)
                    self.assertAlmostEqual(obj.Shape.cut(expected).Volume, 0, delta=1e-5)
                    self.assertAlmostEqual(expected.cut(obj.Shape).Volume, 0, delta=1e-5)
                    obj.LengthDriver = 10
                    self.doc.recompute()
                    self.valid(obj)
                    self.assertEqual(obj.Length.Value, 10)
                    self.assertAlmostEqual(obj.Shape.Volume, 44000 if pocket else 6000, delta=1e-5)

    def testThinPocketJunctionFillet(self):
        self.checkJunctionFillet(pocket=True)

    def testJunctionFilletPlacementAndPersistence(self):
        body, base, wall = self.junctionFeature()
        wall.RootFilletRadius = 0.5
        self.doc.recompute()
        expected = wall.Shape.copy()
        body.Placement = App.Placement(
            App.Vector(23, -17, 41), App.Rotation(App.Vector(1, 2, 3), 47)
        )
        self.doc.recompute()
        self.valid(wall)
        self.assertAlmostEqual(wall.Shape.Volume, expected.Volume, delta=1e-5)
        with tempfile.TemporaryDirectory() as folder:
            path = os.path.join(folder, "junction.FCStd")
            self.doc.saveAs(path)
            App.closeDocument(self.doc.Name)
            self.doc = App.openDocument(path)
            wall = self.doc.getObject("Wall")
            wall.touch()
            self.doc.recompute()
            self.valid(wall)
            self.assertEqual(wall.RootFilletRadius.Value, 0.5)
            self.assertAlmostEqual(wall.Shape.Volume, expected.Volume, delta=1e-5)

    def testJunctionFilletFailureRecovery(self):
        body, base, wall = self.junctionFeature()
        wall.RootFilletRadius = 100
        self.doc.recompute()
        self.assertIn("Invalid", wall.State)
        wall.RootFilletRadius = 0.5
        self.doc.recompute()
        self.valid(wall)

    def testJunctionFilletNeedsExistingBody(self):
        body, sketch, wall = self.feature()
        wall.RootFilletRadius = 0.5
        self.doc.recompute()
        self.assertIn("Invalid", wall.State)
        self.assertIn("No root edges", wall.getStatusString())
        wall.RootFilletRadius = 0
        self.doc.recompute()
        self.valid(wall)

    def testJunctionFilletIgnoredOutsideThinMode(self):
        for pocket in (False, True):
            body, sketch, feature = self.feature("rectangle", pocket=pocket)
            feature.Thin = False
            self.doc.recompute()
            sharp = feature.Shape.copy()
            feature.RootFilletRadius = 0.5
            self.doc.recompute()
            self.valid(feature)
            self.assertAlmostEqual(feature.Shape.Volume, sharp.Volume, delta=1e-5)

    def testAnalyticalProfilesAndSides(self):
        for family in ("line", "corner", "rectangle", "circle"):
            body, sketch, obj = self.feature(family)
            for side, a, b in (
                ("SideA", 0, 2),
                ("SideB", -2, 0),
                ("Centered", -1, 1),
                ("Two sides", -0.5, 2),
            ):
                obj.ThinSide = side
                obj.ThinThickness2 = 0.5
                for reverse in (False, True):
                    with self.subTest(family=family, side=side, reverse=reverse):
                        obj.Reversed = reverse
                        self.doc.recompute()
                        self.valid(obj)
                        if family == "line":
                            area = 30 * (b - a)
                        elif family == "corner":
                            area = 50 * (b - a) - (b * b - a * a)
                        elif family == "rectangle":
                            area = 100 * (b - a) + 4 * (b * b - a * a)
                        else:
                            area = math.pi * ((10 + b) ** 2 - (10 + a) ** 2)
                        self.assertAlmostEqual(obj.Shape.Volume, area * 12, delta=1e-5)
                        box = obj.Shape.BoundBox
                        self.assertAlmostEqual(box.ZMin, -12 if reverse else 0, delta=1e-6)
                        self.assertAlmostEqual(box.ZMax, 0 if reverse else 12, delta=1e-6)

    def testPocket(self):
        body, sketch, obj = self.feature(pocket=True)
        for side in ("SideA", "SideB", "Centered", "Two sides"):
            obj.ThinSide = side
            obj.ThinThickness2 = 0.5
            self.doc.recompute()
            self.valid(obj)
            self.assertAlmostEqual(
                obj.Shape.Volume, 50000 - 30 * (2.5 if side == "Two sides" else 2) * 12, delta=1e-5
            )
            self.assertTrue(obj.Shape.isInside(App.Vector(15, 0, -19), 1e-7, False))

    def testCenteredPlacementDefault(self):
        for pocket in (False, True):
            body, sketch, obj = self.feature(pocket=pocket)
            self.assertEqual(obj.ThinSide, "Centered")
            self.valid(obj)
            self.assertAlmostEqual(obj.AddSubShape.BoundBox.YMin, -1, delta=1e-5)
            self.assertAlmostEqual(obj.AddSubShape.BoundBox.YMax, 1, delta=1e-5)

    def testSignedTaperIndependentOfGrowthReversal(self):
        body, sketch, obj = self.feature()
        obj.ThinSide = "Centered"
        for angle in (-3, 3):
            for reverse in (False, True):
                obj.TaperAngle = angle
                obj.Reversed = reverse
                self.doc.recompute()
                self.valid(obj)
                expected = 30 * 12 * (2 - 12 * math.tan(math.radians(angle)))
                self.assertAlmostEqual(obj.Shape.Volume, expected, delta=1e-5)
                self.assertAlmostEqual(obj.Shape.BoundBox.ZMin, -12 if reverse else 0, delta=1e-5)

    def testSmoothSpline(self):
        body, sketch, obj = self.feature()
        sketch.delGeometry(1)
        sketch.delGeometry(0)
        curve = Part.BSplineCurve()
        curve.interpolate(
            [App.Vector(0, 0, 0), App.Vector(10, 3, 0), App.Vector(20, -2, 0), App.Vector(30, 0, 0)]
        )
        sketch.addGeometry(curve, False)
        obj.ThinSide = "Centered"
        self.doc.recompute()
        self.valid(obj)
        obj.Shape.check(True)
        # The curvature contributions of equal opposite offsets cancel.
        self.assertAlmostEqual(obj.Shape.Volume, curve.toShape().Length * 2 * 12, delta=1e-3)

    def testPocketThroughAllFromOffsetProfile(self):
        body, sketch, obj = self.feature(pocket=True)
        sketch.Placement.Base.z = 1000
        obj.Type = "ThroughAll"
        self.doc.recompute()
        self.valid(obj)
        self.assertAlmostEqual(obj.Shape.Volume, 50000 - 30 * 2 * 20, delta=1e-5)

    def testRoundCaps(self):
        body, sketch, obj = self.feature()
        for side in ("SideA", "SideB", "Centered", "Two sides"):
            obj.ThinSide = side
            obj.ThinCap = "Round"
            obj.ThinThickness2 = 0.5
            self.doc.recompute()
            self.valid(obj)
            width = 2.5 if side == "Two sides" else 2
            self.assertAlmostEqual(
                obj.Shape.Volume, (30 * width + math.pi * (width / 2) ** 2) * 12, delta=1e-5
            )

    def testCrossingAndTJunction(self):
        for start in (-10, 0):
            body, sketch, obj = self.feature()
            sketch.addGeometry(
                Part.LineSegment(App.Vector(15, start, 0), App.Vector(15, 10, 0)), False
            )
            obj.ThinSide = "Centered"
            self.doc.recompute()
            self.valid(obj)
            expected_area = 60 + (10 - start) * 2 - (4 if start == -10 else 2)
            self.assertAlmostEqual(obj.Shape.Volume, expected_area * 12, delta=1e-5)
            self.assertTrue(obj.Shape.isInside(App.Vector(15, 0, 6), 1e-7, False))
            obj.ThinCap = "Round"
            self.doc.recompute()
            self.valid(obj)
            cap_count = 4 if start == -10 else 3
            self.assertAlmostEqual(
                obj.Shape.Volume, (expected_area + cap_count * math.pi / 2) * 12, delta=1e-5
            )

    def testStartAndGrowthSides(self):
        body, sketch, obj = self.feature()
        obj.ThinSide = "Centered"
        obj.StartType = "Offset"
        obj.StartOffset = 3
        for side, low, high in (("One side", 3, 15), ("Symmetric", -3, 9), ("Two sides", -2, 15)):
            obj.SideType = side
            obj.Length2 = 5
            self.doc.recompute()
            self.valid(obj)
            self.assertAlmostEqual(obj.Shape.Volume, 60 * (high - low), delta=1e-5)
            self.assertAlmostEqual(obj.Shape.BoundBox.ZMin, low, delta=1e-6)
            self.assertAlmostEqual(obj.Shape.BoundBox.ZMax, high, delta=1e-6)

    def testRigidPlacement(self):
        body, sketch, obj = self.feature("corner")
        original = obj.Shape.copy()
        sketch.Placement = App.Placement(
            App.Vector(21, -8, 19), App.Rotation(App.Vector(1, 2, 3), 47)
        )
        self.doc.recompute()
        self.valid(obj)
        expected = original.copy()
        expected.Placement = sketch.Placement.multiply(expected.Placement)
        self.assertAlmostEqual(obj.Shape.Volume, original.Volume, delta=1e-5)
        self.assertAlmostEqual(obj.Shape.common(expected).Volume, original.Volume, delta=1e-5)

    def testOrdinaryPadUnchanged(self):
        body, sketch, obj = self.feature("rectangle")
        obj.Thin = False
        self.doc.recompute()
        self.valid(obj)
        self.assertAlmostEqual(obj.Shape.Volume, 30 * 20 * 12, delta=1e-5)

    def testDisconnectedProfilesRespectBodyPolicy(self):
        body, sketch, obj = self.feature()
        body.AllowCompound = False
        sketch.addGeometry(Part.LineSegment(App.Vector(0, 10, 0), App.Vector(30, 10, 0)), False)
        self.doc.recompute()
        self.assertIn("Invalid", obj.State)
        body.AllowCompound = True
        self.doc.recompute()
        self.assertNotIn("Invalid", obj.State, obj.getStatusString())
        self.assertEqual(len(obj.Shape.Solids), 2)
        self.assertAlmostEqual(obj.Shape.Volume, 1440, delta=1e-5)

    def testSelectionOrder(self):
        body, sketch, obj = self.feature("corner")
        obj.Profile = (sketch, ["Edge1", "Edge2"])
        self.doc.recompute()
        self.valid(obj)
        expected = obj.Shape.copy()
        obj.Profile = (sketch, ["Edge2", "Edge1"])
        self.doc.recompute()
        self.valid(obj)
        self.assertAlmostEqual(obj.Shape.common(expected).Volume, expected.Volume, delta=1e-5)

    def testSmallFaceUsesUnderlyingSurface(self):
        body, sketch, obj = self.feature()
        reference = self.doc.addObject("Part::Feature", "TargetReference")
        reference.Shape = Part.makePlane(5, 5, App.Vector(10, -2, 12))
        target = body.newObject("PartDesign::SubShapeBinder", "Target")
        target.Support = [(reference, ["Face1"])]
        body.Tip = obj
        self.doc.recompute()
        obj.UpToFace = (target, ["Face1"])
        obj.Type = "UpToFace"
        self.doc.recompute()
        self.valid(obj)
        self.assertAlmostEqual(obj.Shape.Volume, 720, delta=1e-5)

    def testSlopedTargetAndSignedOffsets(self):
        body, sketch, obj = self.feature()
        reference = self.doc.addObject("Part::Feature", "TargetReference")
        points = [
            App.Vector(x, y, 10 + 0.2 * x)
            for x, y in ((-10, -5), (40, -5), (40, 5), (-10, 5), (-10, -5))
        ]
        reference.Shape = Part.Face(Part.makePolygon(points))
        target = body.newObject("PartDesign::SubShapeBinder", "Target")
        target.Support = [(reference, ["Face1"])]
        body.Tip = obj
        self.doc.recompute()
        original = reference.Shape.exportBrepToString()
        obj.UpToFace = (target, ["Face1"])
        obj.Type = "UpToFace"
        for offset in (-2, 0, 2):
            obj.Offset = offset
            self.doc.recompute()
            self.valid(obj)
            self.assertAlmostEqual(obj.Shape.Volume, 60 * (13 + offset), delta=1e-5)
        self.assertEqual(reference.Shape.exportBrepToString(), original)

    def testTargetHoleUsesUnderlyingSurface(self):
        body, sketch, obj = self.feature()
        reference = self.doc.addObject("Part::Feature", "TargetReference")
        reference.Shape = Part.makePlane(50, 10, App.Vector(-10, -5, 12)).cut(
            Part.makePlane(2, 2, App.Vector(14, 0, 12))
        )
        target = body.newObject("PartDesign::SubShapeBinder", "Target")
        target.Support = [(reference, ["Face1"])]
        body.Tip = obj
        self.doc.recompute()
        obj.UpToFace = (target, ["Face1"])
        obj.Type = "UpToFace"
        self.doc.recompute()
        self.valid(obj)
        self.assertAlmostEqual(obj.Shape.Volume, 720, delta=1e-5)

    def testCurvedTarget(self):
        body, sketch, obj = self.feature()
        reference = self.doc.addObject("Part::Feature", "TargetReference")
        reference.Shape = Part.makeCylinder(20, 10, App.Vector(15, -5, 25), App.Vector(0, 1, 0))
        target = body.newObject("PartDesign::SubShapeBinder", "Target")
        target.Support = [(reference, ["Face1"])]
        body.Tip = obj
        self.doc.recompute()
        obj.UpToFace = (target, ["Face1"])
        obj.Type = "UpToFace"
        self.doc.recompute()
        self.valid(obj)
        for x in (0.1, 15, 29.9):
            plane = Part.Face(
                Part.makePolygon(
                    [
                        App.Vector(x, y, z)
                        for y, z in ((-20, -10), (20, -10), (20, 80), (-20, 80), (-20, -10))
                    ]
                )
            )
            section = obj.Shape.section(plane)
            self.assertAlmostEqual(
                section.BoundBox.ZMax, 25 - math.sqrt(400 - (x - 15) ** 2), delta=1e-5
            )

    def testSaveReloadAndExpressions(self):
        body, sketch, obj = self.feature()
        obj.setExpression("ThinThickness", "Length / 6")
        obj.ThinSide = "Two sides"
        obj.ThinThickness2 = 0.5
        self.doc.recompute()
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "thin.FCStd")
            self.doc.saveAs(path)
            App.closeDocument(self.doc.Name)
            self.doc = App.openDocument(path)
            obj = self.doc.getObject("ThinExtrude")
            obj.Length = 18
            self.doc.recompute()
            self.valid(obj)
            self.assertAlmostEqual(obj.Shape.Volume, 30 * 3.5 * 18, delta=1e-5)

    def testUndoRedo(self):
        body, sketch, obj = self.feature()
        self.doc.openTransaction("Change thin width")
        obj.ThinThickness = 3
        self.doc.recompute()
        self.doc.commitTransaction()
        self.valid(obj)
        self.assertAlmostEqual(obj.Shape.Volume, 1080, delta=1e-5)
        self.doc.undo()
        self.doc.recompute()
        self.valid(obj)
        self.assertAlmostEqual(obj.ThinThickness.Value, 2)
        self.assertAlmostEqual(obj.Shape.Volume, 720, delta=1e-5)
        self.doc.redo()
        self.doc.recompute()
        self.valid(obj)
        self.assertAlmostEqual(obj.Shape.Volume, 1080, delta=1e-5)

    def testCircularDraft(self):
        body, sketch, obj = self.feature("circle")
        obj.ThinSide = "Centered"
        obj.TaperAngle = 3
        self.doc.recompute()
        self.valid(obj)
        for z in (0.01, 11.99):
            section = obj.Shape.section(Part.makePlane(60, 60, App.Vector(-30, -30, z)))
            radii = sorted(e.Curve.Radius for e in section.Edges)
            self.assertEqual(len(radii), 2)
            delta = z * math.tan(math.radians(3))
            self.assertAlmostEqual(radii[0], 9 + delta, delta=1e-5)
            self.assertAlmostEqual(radii[1], 11 - delta, delta=1e-5)

    def testModelEdgeSupportPlaneAndSelectionOrder(self):
        body = self.doc.addObject("PartDesign::Body", "Body")
        profile = body.newObject("PartDesign::Feature", "EdgeProfile")
        profile.Shape = Part.makePolygon(
            [App.Vector(0, 0, 0), App.Vector(30, 0, 0), App.Vector(30, 20, 0)]
        )
        obj = body.newObject("PartDesign::Pad", "ThinExtrude")
        obj.Profile = (profile, ["Edge1", "Edge2"])
        obj.Thin = True
        obj.ThinThickness = 2
        obj.Length = 12
        self.doc.recompute()
        self.valid(obj)
        expected = obj.Shape.copy()
        obj.Profile = (profile, ["Edge2", "Edge1"])
        self.doc.recompute()
        self.valid(obj)
        self.assertAlmostEqual(expected.cut(obj.Shape).Volume, 0, delta=1e-5)
        self.assertAlmostEqual(obj.Shape.cut(expected).Volume, 0, delta=1e-5)
        placement = App.Placement(App.Vector(20, -15, 9), App.Rotation(App.Vector(1, 2, 3), 67))
        profile.Placement = placement
        self.doc.recompute()
        self.valid(obj)
        expected.Placement = placement.multiply(expected.Placement)
        self.assertAlmostEqual(expected.cut(obj.Shape).Volume, 0, delta=1e-5)
        self.assertAlmostEqual(obj.Shape.cut(expected).Volume, 0, delta=1e-5)
