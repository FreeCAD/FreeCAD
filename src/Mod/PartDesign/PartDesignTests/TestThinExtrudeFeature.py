# SPDX-License-Identifier: LGPL-2.1-or-later
import math
import os
import tempfile
import unittest
import FreeCAD as App
import Part
import Sketcher


class TestThinExtrudeFeature(unittest.TestCase):
    featureType = "PartDesign::ThinExtrude"

    def setUp(self):
        self.doc = App.newDocument("TestRib")

    def tearDown(self):
        App.closeDocument(self.doc.Name)

    def valid(self, rib):
        self.assertNotIn("Invalid", rib.State, rib.getStatusString())
        self.assertTrue(rib.Shape.isValid())
        self.assertEqual(len(rib.Shape.Solids), 1)

    def web(self):
        body = self.doc.addObject("PartDesign::Body", "Body")
        base = body.newObject("PartDesign::Feature", "Base")
        base.Shape = Part.makeBox(40, 30, 3, App.Vector(0, 0, -3))
        sketch = body.newObject("Sketcher::SketchObject", "Profile")
        sketch.addGeometry(Part.LineSegment(App.Vector(8, 15, 0), App.Vector(32, 15, 0)), False)
        self.doc.recompute()
        rib = body.newObject(self.featureType, "Rib")
        rib.Profile = sketch
        rib.Length = 12
        rib.ThinThickness = 2
        self.doc.recompute()
        return body, base, sketch, rib

    def testWeb(self):
        body, base, sketch, rib = self.web()
        self.valid(rib)
        self.assertAlmostEqual(rib.Shape.Volume, base.Shape.Volume + 24 * 2 * 12, delta=1e-5)

    def bowlWall(self, curved=False, kind=None, cross=False):
        kind = kind or self.featureType
        body = self.doc.addObject("PartDesign::Body", "BowlBody")
        base = body.newObject("PartDesign::Feature", "Bowl")
        if curved:
            inner = Part.makeSphere(30, App.Vector(0, 0, 20)).common(
                Part.makeBox(100, 100, 25, App.Vector(-50, -50, 0))
            )
            outer = Part.makeSphere(33, App.Vector(0, 0, 20)).common(
                Part.makeBox(100, 100, 28, App.Vector(-50, -50, -3))
            )
        else:
            inner = Part.makeCone(20, 35, 30)
            outer = Part.makeCone(23, 38, 30).fuse(Part.makeCylinder(23, 3, App.Vector(0, 0, -3)))
        base.Shape = outer.cut(inner).removeSplitter()
        sketch = body.newObject("Sketcher::SketchObject", "BowlProfile")
        sketch.addGeometry(Part.LineSegment(App.Vector(-18, 0, 0), App.Vector(18, 0, 0)), False)
        if cross:
            sketch.addGeometry(Part.LineSegment(App.Vector(0, -18, 0), App.Vector(0, 18, 0)), False)
        feature = body.newObject(kind, "BowlWeb")
        feature.Profile = sketch
        feature.Thin = True
        feature.ThinSide = "Centered"
        feature.ThinThickness = 2
        feature.Length = 18
        self.doc.recompute()
        self.valid(feature)
        self.assertEqual(feature.ThinExtension, "Off")
        feature.ThinExtension = "Tangent"
        self.doc.recompute()
        self.valid(feature)
        expected = Part.makeBox(100, 2, 18, App.Vector(-50, -1, 0))
        if cross:
            expected = expected.fuse(Part.makeBox(2, 100, 18, App.Vector(-1, -50, 0)))
        expected = expected.common(inner)
        self.assertAlmostEqual(feature.AddSubShape.Volume, expected.Volume, delta=1e-5)
        self.assertAlmostEqual(feature.AddSubShape.cut(expected).Volume, 0, delta=1e-5)
        self.assertAlmostEqual(expected.cut(feature.AddSubShape).Volume, 0, delta=1e-5)
        return body, base, sketch, feature, inner

    def checkBowlRimTermination(self, kind=None):
        body, base, sketch, wall, inner = self.bowlWall(cross=True, kind=kind)
        self.assertAlmostEqual(base.Shape.Faces[1].BoundBox.ZMin, 30, delta=1e-6)
        self.assertGreater(len(base.Shape.Faces[1].Wires), 1)  # Annular Bowl:Face2.
        wall.Type = "UpToFace"
        wall.UpToFace = (base, ["Face2"])
        for extend in (False, True):
            wall.ThinExtension = "Tangent" if extend else "Off"
            for selection in (None, (sketch, ["Edge1"])):
                wall.ThinExtensionEdges = selection
                wall.ThinExtendAll = selection is None
                for height in (30, 27):
                    wall.Offset = height - 30
                    self.doc.recompute()
                    self.valid(wall)
                    width_x = 100 if extend else 36
                    width_y = 100 if extend and selection is None else 36
                    expected = (
                        Part.makeBox(width_x, 2, height, App.Vector(-width_x / 2, -1, 0))
                        .fuse(Part.makeBox(2, width_y, height, App.Vector(-1, -width_y / 2, 0)))
                        .common(inner)
                    )
                    self.assertAlmostEqual(wall.AddSubShape.cut(expected).Volume, 0, delta=1e-5)
                    self.assertAlmostEqual(expected.cut(wall.AddSubShape).Volume, 0, delta=1e-5)
        wall.Offset = 0
        self.doc.recompute()
        volume = wall.Shape.Volume
        body.Placement = App.Placement(
            App.Vector(35, -17, 46), App.Rotation(App.Vector(1, 2, 3), 57)
        )
        wall.touch()
        self.doc.recompute()
        self.valid(wall)
        self.assertAlmostEqual(wall.Shape.Volume, volume, delta=1e-5)

        body.Placement = App.Placement()
        wall.UpToFace = (base, ["Face6"])
        sketch.Placement.Base.z = 30
        wall.Reversed = True
        self.doc.recompute()
        self.valid(wall)
        self.assertAlmostEqual(wall.Shape.Volume, volume, delta=1e-5)

    def checkSelectedExtension(self, kind=None):
        body, base, sketch, web, inner = self.bowlWall(cross=True, kind=kind)
        allEnds = web.AddSubShape.copy()
        web.ThinExtensionEdges = (sketch, ["Edge1"])
        web.ThinExtendAll = False
        self.doc.recompute()
        self.valid(web)
        expected = (
            Part.makeBox(100, 2, 18, App.Vector(-50, -1, 0))
            .fuse(Part.makeBox(2, 36, 18, App.Vector(-1, -18, 0)))
            .common(inner)
        )
        self.assertAlmostEqual(web.AddSubShape.cut(expected).Volume, 0, delta=1e-5)
        self.assertAlmostEqual(expected.cut(web.AddSubShape).Volume, 0, delta=1e-5)
        # Unselected ends retain their cap, even while other edges extend.
        web.ThinCap = "Round"
        self.doc.recompute()
        self.valid(web)
        self.assertGreater(web.AddSubShape.Volume, expected.Volume)
        web.ThinCap = "Flat"
        # A start offset plus reversed growth must transform the selection too.
        sketch.Placement.Base.z = 20
        web.Reversed = True
        web.StartType = "Offset"
        web.StartOffset = 2
        self.doc.recompute()
        self.valid(web)
        self.assertAlmostEqual(web.AddSubShape.Volume, expected.Volume, delta=1e-5)
        body.Placement = App.Placement(
            App.Vector(35, -71, 46), App.Rotation(App.Vector(1, 2, 3), 57)
        )
        web.touch()
        self.doc.recompute()
        self.valid(web)
        self.assertAlmostEqual(web.AddSubShape.Volume, expected.Volume, delta=1e-5)
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "SelectedExtension.FCStd")
            self.doc.saveAs(path)
            App.closeDocument(self.doc.Name)
            self.doc = App.openDocument(path)
            web = self.doc.getObject("BowlWeb")
            web.touch()
            self.doc.recompute()
            self.valid(web)
            self.assertEqual(web.ThinExtensionEdges[1], ["Edge1"])
            self.assertAlmostEqual(web.AddSubShape.Volume, expected.Volume, delta=1e-5)
        web.ThinExtensionEdges = None
        web.ThinExtendAll = True
        self.doc.recompute()
        self.valid(web)
        self.assertAlmostEqual(web.AddSubShape.Volume, allEnds.Volume, delta=1e-5)

    def testSideProfileRib(self):
        body = self.doc.addObject("PartDesign::Body", "Body")
        base = body.newObject("PartDesign::Feature", "Base")
        base.Shape = Part.makeBox(36, 24, 3, App.Vector(0, 0, -3)).fuse(Part.makeBox(3, 24, 24))
        sketch = body.newObject("Sketcher::SketchObject", "Profile")
        sketch.addGeometry(Part.LineSegment(App.Vector(3, 20, 0), App.Vector(29, 0, 0)), False)
        sketch.Placement = App.Placement(
            App.Vector(0, 12, 0), App.Rotation(App.Vector(1, 0, 0), 90)
        )
        self.doc.recompute()
        rib = body.newObject(self.featureType, "Rib")
        rib.Profile = sketch
        rib.RibMode = "Rib"
        rib.ThinThickness = 2
        rib.Type = "UpToFirst"
        self.doc.recompute()
        self.valid(rib)
        self.assertAlmostEqual(rib.Shape.Volume, base.Shape.Volume + 26 * 20, delta=1e-5)
        self.assertTrue(rib.Shape.isInside(App.Vector(10, 12, 5), 1e-7, False))
        self.assertFalse(rib.Shape.isInside(App.Vector(20, 12, 15), 1e-7, False))

    def towardCorner(self):
        self.testSideProfileRib()
        base = self.doc.Base
        rib = self.doc.Rib
        corner = next(
            i
            for i, edge in enumerate(base.Shape.Edges, 1)
            if all(abs(v.Point.x - 3) < 1e-7 and abs(v.Point.z) < 1e-7 for v in edge.Vertexes)
        )
        rib.ReferenceAxis = (base, ["Edge%d" % corner])
        rib.TowardReference = True
        self.doc.recompute()
        self.valid(rib)
        return rib

    def testFiniteSideRibStopsAtBody(self):
        self.testSideProfileRib()
        rib = self.doc.getObject("Rib")
        base = self.doc.getObject("Base")
        rib.Type = "Length"
        rib.Length = 7
        self.doc.recompute()
        self.valid(rib)
        rib.Shape.check(True)
        # Independent integral of min(7, 20*(29-x)/26), x in [3,29].
        self.assertAlmostEqual(
            rib.Shape.Volume - base.Shape.Volume, 2 * 26 * (7 - 49 / 40), delta=1e-5
        )
        self.assertAlmostEqual(rib.Shape.BoundBox.ZMin, -3, delta=1e-7)
        self.assertFalse(rib.Shape.isInside(App.Vector(10, 12, 4), 1e-7, False))
        self.assertTrue(rib.Shape.isInside(App.Vector(10, 12, 12), 1e-7, False))
        self.assertFalse(rib.Shape.isInside(App.Vector(27, 12, -5), 1e-7, False))
        # Short depths need not reach a target along the whole profile.
        rib.Length = 2
        self.doc.recompute()
        self.valid(rib)
        self.assertAlmostEqual(
            rib.Shape.Volume - base.Shape.Volume, 2 * 26 * (2 - 4 / 40), delta=1e-5
        )

    def bossRib(self, mode="Tangent", width=1, placement="Centered"):
        body = self.doc.addObject("PartDesign::Body", "Body")
        base = body.newObject("PartDesign::Feature", "Bosses")
        base.Shape = (
            Part.makeBox(50, 30, 10, App.Vector(0, -15, 0))
            .fuse([Part.makeCylinder(20, 30), Part.makeCylinder(10, 20, App.Vector(50, 0, 0))])
            .removeSplitter()
        )
        sketch = body.newObject("Sketcher::SketchObject", "Profile")
        points = [App.Vector(x, z, 0) for x, z in ((22, 24), (30, 17), (36, 17))]
        sketch.addGeometry([Part.LineSegment(a, b) for a, b in zip(points, points[1:])], False)
        sketch.Placement = App.Placement(App.Vector(), App.Rotation(App.Vector(1, 0, 0), 90))
        rib = body.newObject(self.featureType, "Rib")
        rib.Profile = sketch
        rib.RibMode = "Rib"
        rib.Type = "UpToShape"
        rib.AutoDirection = True
        rib.Extension = mode
        rib.ThinThickness = width
        rib.ThinSide = placement
        self.doc.recompute()
        return body, base, sketch, rib

    def checkBossContact(self, base, rib):
        self.valid(rib)
        material = rib.Shape.cut(base.Shape)
        cylinders = [f for f in base.Shape.Faces if isinstance(f.Surface, Part.Cylinder)]
        self.assertEqual(len(cylinders), 2)
        for face in cylinders:
            self.assertGreater(material.common(face).Area, 1)
        # Check at the rib's sides, not just the centerline where the old flat
        # caps were tangent to the cylinders. These points are outside the
        # base but must be inside the rib right up to the curved interface.
        ymin, ymax = material.BoundBox.YMin, material.BoundBox.YMax
        for fraction in (0.01, 0.25, 0.5, 0.75, 0.99):
            y = ymin + fraction * (ymax - ymin)
            for x in (math.sqrt(20**2 - y * y) + 1e-4, 50 - math.sqrt(10**2 - y * y) - 1e-4):
                p = App.Vector(x, y, 15)
                self.assertFalse(base.Shape.isInside(p, 1e-7, False))
                self.assertTrue(material.isInside(p, 1e-7, True), str(p))

    def testZeroThicknessRecovery(self):
        self.testSideProfileRib()
        rib = self.doc.Rib
        expected = rib.Shape.Volume
        rib.ThinThickness = 0
        self.doc.recompute()
        self.assertIn("Invalid", rib.State)
        self.assertIn("thickness", rib.getStatusString().lower())
        rib.ThinThickness = 2
        self.doc.recompute()
        self.valid(rib)
        self.assertAlmostEqual(rib.Shape.Volume, expected, delta=1e-5)
