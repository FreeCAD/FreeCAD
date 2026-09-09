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

    def testFullHeightConicalWall(self):
        body, base, sketch, web, inner = self.bowlWall()
        for z in (0.1, 9, 17.9):
            section = web.AddSubShape.section(Part.makePlane(100, 100, App.Vector(-50, -50, z)))
            self.assertAlmostEqual(section.BoundBox.XLength, 2 * (20 + z / 2), delta=1e-5)
            self.assertAlmostEqual(section.BoundBox.YLength, 2, delta=1e-5)

    def testFullHeightCurvedWall(self):
        self.bowlWall(curved=True)

    def testFullHeightThinPad(self):
        self.bowlWall(kind="PartDesign::Pad")

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

    def testBowlRimTerminationWithDraft(self):
        body, base, sketch, wall, inner = self.bowlWall()
        wall.Type = "UpToFace"
        wall.UpToFace = (base, ["Face2"])
        wall.TaperAngle = 1
        self.doc.recompute()
        self.valid(wall)
        self.assertAlmostEqual(wall.AddSubShape.cut(inner).Volume, 0, delta=1e-5)
        self.assertAlmostEqual(wall.AddSubShape.BoundBox.ZMax, 30, delta=1e-5)

    def testBowlRimTermination(self):
        self.checkBowlRimTermination()

    def testThinPadBowlRimTermination(self):
        self.checkBowlRimTermination(kind="PartDesign::Pad")

    def testWallExtensionToSmallObliqueFace(self):
        body, base, sketch, wall, inner = self.bowlWall(cross=True)
        reference = self.doc.addObject("Part::Feature", "ObliqueReference")
        reference.Shape = Part.Face(
            Part.makePolygon(
                [
                    App.Vector(x, y, 25 + 0.1 * x)
                    for x, y in ((-2, -2), (2, -2), (2, 2), (-2, 2), (-2, -2))
                ]
            )
        )
        target = body.newObject("PartDesign::SubShapeBinder", "ObliqueTarget")
        target.Support = [(reference, ["Face1"])]
        body.Tip = wall
        self.doc.recompute()
        wall.Type = "UpToFace"
        wall.UpToFace = (target, ["Face1"])
        self.doc.recompute()
        self.valid(wall)
        below = Part.Face(
            Part.makePolygon(
                [
                    App.Vector(x, -50, z)
                    for x, z in ((-50, -1), (50, -1), (50, 30), (-50, 20), (-50, -1))
                ]
            )
        ).extrude(App.Vector(0, 100, 0))
        expected = (
            Part.makeBox(100, 2, 40, App.Vector(-50, -1, 0))
            .fuse(Part.makeBox(2, 100, 40, App.Vector(-1, -50, 0)))
            .common(inner)
            .common(below)
        )
        self.assertAlmostEqual(wall.AddSubShape.cut(expected).Volume, 0, delta=1e-5)
        self.assertAlmostEqual(expected.cut(wall.AddSubShape).Volume, 0, delta=1e-5)
        # OCCT currently rejects this drafted oblique network. Preserve a clear
        # failure instead of accepting a stale or invalid shape as a new result.
        wall.TaperAngle = 1
        self.doc.recompute()
        self.assertIn("Invalid", wall.State)
        self.assertIn("draft", wall.getStatusString().lower())

    def testFullHeightNetwork(self):
        self.bowlWall(cross=True)

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

    def testSelectedFullHeightExtension(self):
        self.checkSelectedExtension()

    def testThinPadSelectedFullHeightExtension(self):
        self.checkSelectedExtension(kind="PartDesign::Pad")

    def testExtensionSelectionRejectsForeignEdges(self):
        body, base, sketch, web, inner = self.bowlWall(cross=True)
        for selection in (
            (base, ["Edge1"]),
            (sketch, ["Vertex1"]),
            (sketch, ["Edge999"]),
            (sketch, [""]),
        ):
            web.ThinExtensionEdges = selection
            web.ThinExtendAll = selection is None
            self.doc.recompute()
            self.assertIn("Invalid", web.State)
        # A valid edge in the same sketch but outside the selected profile is invalid too.
        web.Profile = (sketch, ["Edge1"])
        web.ThinExtensionEdges = (sketch, ["Edge2"])
        web.ThinExtendAll = False
        self.doc.recompute()
        self.assertIn("Invalid", web.State)
        self.assertIn("contained", web.getStatusString())

    def testExtensionSelectionRejectsInteriorEdge(self):
        body, base, sketch, web, inner = self.bowlWall()
        sketch.delGeometry(0)
        for x1, x2 in ((-18, -6), (-6, 6), (6, 18)):
            sketch.addGeometry(Part.LineSegment(App.Vector(x1, 0, 0), App.Vector(x2, 0, 0)), False)
        web.ThinExtensionEdges = (sketch, ["Edge2"])
        web.ThinExtendAll = False
        self.doc.recompute()
        self.assertIn("Invalid", web.State)
        self.assertIn("no free endpoints", web.getStatusString())
        web.ThinExtensionEdges = (sketch, ["Edge1"])
        web.ThinExtendAll = False
        self.doc.recompute()
        self.valid(web)
        section = web.AddSubShape.section(Part.makePlane(100, 100, App.Vector(-50, -50, 9)))
        self.assertAlmostEqual(section.BoundBox.XMin, -24.5, delta=1e-5)
        self.assertAlmostEqual(section.BoundBox.XMax, 18, delta=1e-5)

    def testFullHeightPlacedAndReversed(self):
        body, base, sketch, web, inner = self.bowlWall()
        expected = web.Shape.copy()
        sketch.Placement.Base.z = 18
        web.Reversed = True
        self.doc.recompute()
        self.valid(web)
        self.assertAlmostEqual(web.Shape.cut(expected).Volume, 0, delta=1e-5)
        self.assertAlmostEqual(expected.cut(web.Shape).Volume, 0, delta=1e-5)
        body.Placement = App.Placement(
            App.Vector(135, -71, 46), App.Rotation(App.Vector(1, 2, 3), 57)
        )
        web.touch()
        self.doc.recompute()
        self.valid(web)
        self.assertAlmostEqual(web.Shape.Volume, expected.Volume, delta=1e-5)

    def testFullHeightStartAndTwoSides(self):
        body, base, sketch, web, inner = self.bowlWall()
        expected = web.Shape.copy()
        web.StartType = "Offset"
        web.StartOffset = 9
        web.SideType = "Symmetric"
        self.doc.recompute()
        self.valid(web)
        self.assertAlmostEqual(expected.cut(web.Shape).Volume, 0, delta=1e-5)
        self.assertAlmostEqual(web.Shape.cut(expected).Volume, 0, delta=1e-5)
        web.SideType = "Two sides"
        web.Length = 9
        web.Length2 = 9
        self.doc.recompute()
        self.valid(web)
        self.assertAlmostEqual(web.Shape.Volume, expected.Volume, delta=1e-5)

    def testFullHeightRejectsEscapingEnds(self):
        body, base, sketch, web, inner = self.bowlWall()
        web.Length = 32
        self.doc.recompute()
        self.assertIn("Invalid", web.State)
        self.assertIn("not bounded", web.getStatusString())
        web.ThinExtension = "Off"
        self.doc.recompute()
        self.valid(web)

    def testFullHeightThicknessSidesAndDraft(self):
        body, base, sketch, web, inner = self.bowlWall(curved=True)
        for side in ("SideA", "SideB", "Two sides"):
            web.ThinSide = side
            web.ThinThickness2 = 1
            self.doc.recompute()
            self.valid(web)
            self.assertAlmostEqual(web.AddSubShape.cut(inner).Volume, 0, delta=1e-5)
        web.ThinSide = "Centered"
        web.TaperAngle = 2
        self.doc.recompute()
        self.valid(web)
        self.assertAlmostEqual(web.AddSubShape.cut(inner).Volume, 0, delta=1e-5)
        for z in (1, 17):
            section = web.AddSubShape.section(Part.makePlane(100, 100, App.Vector(-50, -50, z)))
            self.assertAlmostEqual(
                section.BoundBox.XLength, 2 * math.sqrt(900 - (z - 20) ** 2), delta=1e-5
            )
            self.assertAlmostEqual(
                section.BoundBox.YLength, 2 - 2 * z * math.tan(math.radians(2)), delta=1e-5
            )

    def testFullHeightRejectsUnsupportedInputs(self):
        body, base, sketch, web, inner = self.bowlWall()
        web.Type = "UpToFirst"
        self.doc.recompute()
        self.assertIn("Invalid", web.State)
        self.assertIn("finite-length", web.getStatusString())
        web.Type = "Length"
        web.RibMode = 1
        self.doc.recompute()
        self.assertIn("Invalid", web.State)
        self.assertIn("plan profile", web.getStatusString())
        web.RibMode = 0
        sketch.delGeometry(0)
        sketch.addGeometry(Part.Circle(App.Vector(0, 0, 0), App.Vector(0, 0, 1), 12), False)
        self.doc.recompute()
        self.assertIn("Invalid", web.State)
        self.assertIn("free endpoints", web.getStatusString())

    def testFullHeightCurvedProfileAndObliqueDirection(self):
        body, base, sketch, web, inner = self.bowlWall(curved=True)
        sketch.delGeometry(0)
        sketch.addGeometry(
            Part.Arc(App.Vector(-18, 0, 0), App.Vector(0, 5, 0), App.Vector(18, 0, 0)), False
        )
        self.doc.recompute()
        self.valid(web)
        web.UseCustomVector = True
        web.AlongSketchNormal = False
        web.Direction = App.Vector(0.1, 0.1, 1)
        self.doc.recompute()
        self.valid(web)
        self.assertAlmostEqual(web.AddSubShape.cut(inner).Volume, 0, delta=1e-5)

    def testFullHeightPersistence(self):
        body, base, sketch, web, inner = self.bowlWall()
        volume = web.Shape.Volume
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "BodyBoundedWeb.FCStd")
            self.doc.saveAs(path)
            App.closeDocument(self.doc.Name)
            self.doc = App.openDocument(path)
            web = self.doc.getObject("BowlWeb")
            web.touch()
            self.doc.recompute()
            self.assertEqual(web.ThinExtension, "Tangent")
            self.valid(web)
            self.assertAlmostEqual(web.Shape.Volume, volume, delta=1e-5)

    def testThinExtensionModesAndSelection(self):
        body, base, sketch, pad, inner = self.bowlWall(kind="PartDesign::Pad", cross=True)
        self.assertEqual(pad.ThinExtension, "Tangent")
        for mode in ("Tangent", "Natural"):
            for selected in (None, (sketch, ["Edge1"])):
                pad.ThinExtension = mode
                pad.ThinExtensionEdges = selected
                pad.ThinExtendAll = selected is None
                self.doc.recompute()
                self.valid(pad)
                y = 100 if selected is None else 36
                expected = (
                    Part.makeBox(100, 2, 18, App.Vector(-50, -1, 0))
                    .fuse(Part.makeBox(2, y, 18, App.Vector(-1, -y / 2, 0)))
                    .common(inner)
                )
                self.assertAlmostEqual(pad.AddSubShape.cut(expected).Volume, 0, delta=1e-5)
                self.assertAlmostEqual(expected.cut(pad.AddSubShape).Volume, 0, delta=1e-5)
        pad.ThinExtension = "Off"
        self.assertEqual(pad.ThinExtensionEdges[1], ["Edge1"])
        self.doc.recompute()
        self.valid(pad)
        self.assertAlmostEqual(pad.AddSubShape.BoundBox.XLength, 36, delta=1e-5)

    def testExplicitAllFreeEnds(self):
        body, base, sketch, pad, inner = self.bowlWall(kind="PartDesign::Pad", cross=True)
        self.assertTrue(pad.ThinExtendAll)
        all_volume = pad.AddSubShape.Volume
        pad.ThinExtendAll = False
        self.doc.recompute()
        self.valid(pad)
        none_volume = pad.AddSubShape.Volume
        self.assertLess(none_volume, all_volume)
        pad.ThinExtensionEdges = (sketch, ["Edge1"])
        pad.ThinExtendAll = False
        self.doc.recompute()
        self.valid(pad)
        self.assertFalse(pad.ThinExtendAll)
        self.assertGreater(pad.AddSubShape.Volume, none_volume)
        self.assertLess(pad.AddSubShape.Volume, all_volume)
        pad.ThinExtendAll = True
        self.assertEqual(pad.ThinExtensionEdges[1], ["Edge1"])
        self.doc.recompute()
        self.assertAlmostEqual(pad.AddSubShape.Volume, all_volume, delta=1e-5)
        pad.ThinExtendAll = False
        pad.ThinExtensionEdges = None
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "NoSelectedEnds.FCStd")
            self.doc.recompute()
            self.doc.saveAs(path)
            App.closeDocument(self.doc.Name)
            self.doc = App.openDocument(path)
            pad = self.doc.BowlWeb
            self.assertFalse(pad.ThinExtendAll)
            pad.touch()
            self.doc.recompute()
            self.valid(pad)
            self.assertAlmostEqual(pad.AddSubShape.Volume, none_volume, delta=1e-5)

    def testThinPocketExtensionModes(self):
        body = self.doc.addObject("PartDesign::Body", "PocketBody")
        base = body.newObject("PartDesign::Feature", "Block")
        base.Shape = Part.makeBox(40, 30, 20, App.Vector(-20, -15, 0))
        sketch = body.newObject("Sketcher::SketchObject", "CutProfile")
        sketch.Placement.Base.z = 20
        sketch.addGeometry(
            [
                Part.LineSegment(App.Vector(-8, 0, 0), App.Vector(8, 0, 0)),
                Part.LineSegment(App.Vector(0, -5, 0), App.Vector(0, 5, 0)),
            ],
            False,
        )
        pocket = body.newObject("PartDesign::Pocket", "Pocket")
        pocket.Profile = sketch
        pocket.Thin = True
        pocket.ThinThickness = 2
        pocket.Length = 5
        for mode in ("Off", "Tangent", "Natural"):
            for selected in (None, (sketch, ["Edge1"])):
                pocket.ThinExtension = mode
                pocket.ThinExtensionEdges = selected
                pocket.ThinExtendAll = selected is None
                self.doc.recompute()
                self.valid(pocket)
                x = 16 if mode == "Off" else 40
                y = 30 if mode != "Off" and selected is None else 10
                cut = Part.makeBox(x, 2, 5, App.Vector(-x / 2, -1, 15)).fuse(
                    Part.makeBox(2, y, 5, App.Vector(-1, -y / 2, 15))
                )
                expected = base.Shape.cut(cut)
                self.assertAlmostEqual(pocket.Shape.cut(expected).Volume, 0, delta=1e-5)
                self.assertAlmostEqual(expected.cut(pocket.Shape).Volume, 0, delta=1e-5)

    def testThinC2SplineWall(self):
        body, base, sketch, pad, inner = self.bowlWall(kind="PartDesign::Pad")
        sketch.delGeometry(0)
        curve = Part.BSplineCurve()
        curve.buildFromPolesMultsKnots(
            [
                App.Vector(-18, 0, 0),
                App.Vector(-6, 4, 0),
                App.Vector(6, 4, 0),
                App.Vector(18, 0, 0),
            ],
            [4, 4],
            [0.0, 1.0],
            False,
            3,
        )
        sketch.addGeometry(curve, False)
        self.doc.recompute()
        source = sketch.Shape.copy()
        volumes = []
        for mode in ("Tangent", "Natural"):
            pad.ThinExtension = mode
            self.doc.recompute()
            self.valid(pad)
            self.assertAlmostEqual(pad.AddSubShape.cut(inner).Volume, 0, delta=1e-5)
            self.assertAlmostEqual(sketch.Shape.Length, source.Length, delta=1e-7)
            volumes.append(pad.AddSubShape.Volume)
        self.assertNotAlmostEqual(*volumes, places=4)
        volume = pad.Shape.Volume
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "C2Web.FCStd")
            self.doc.saveAs(path)
            App.closeDocument(self.doc.Name)
            self.doc = App.openDocument(path)
            pad = self.doc.BowlWeb
            self.assertEqual(pad.ThinExtension, "Natural")
            pad.touch()
            self.doc.recompute()
            self.valid(pad)
            self.assertAlmostEqual(pad.Shape.Volume, volume, delta=1e-5)

    def testThinPocketC2SplineAndReversedFaceExtent(self):
        self.testThinPocketExtensionModes()
        pocket = self.doc.Pocket
        sketch = self.doc.CutProfile
        sketch.delGeometry(1)
        sketch.delGeometry(0)
        curve = Part.BSplineCurve()
        curve.buildFromPolesMultsKnots(
            [App.Vector(-8, 0, 0), App.Vector(-3, 2, 0), App.Vector(3, 2, 0), App.Vector(8, 0, 0)],
            [4, 4],
            [0.0, 1.0],
            False,
            3,
        )
        sketch.addGeometry(curve, False)
        pocket.ThinExtensionEdges = None
        pocket.ThinExtendAll = True
        volumes = []
        for mode in ("Tangent", "Natural"):
            pocket.ThinExtension = mode
            self.doc.recompute()
            self.valid(pocket)
            self.assertAlmostEqual(pocket.Shape.cut(self.doc.Block.Shape).Volume, 0, delta=1e-5)
            volumes.append(pocket.Shape.Volume)
        self.assertNotAlmostEqual(*volumes, places=4)
        sketch.Placement.Base.z = 0
        pocket.Reversed = True
        self.doc.recompute()
        self.valid(pocket)
        self.assertAlmostEqual(pocket.Shape.Volume, volumes[-1], delta=1e-5)
        target = self.doc.addObject("Part::Feature", "Termination")
        target.Shape = Part.makePlane(60, 50, App.Vector(-30, -25, 7))
        pocket.Type = "UpToFace"
        pocket.UpToFace = (target, ["Face1"])
        self.doc.recompute()
        self.valid(pocket)
        removed = self.doc.Block.Shape.cut(pocket.Shape)
        # Default spline bounds include control-polygon/deflection padding.
        bounds = removed.optimalBoundingBox(False, False)
        self.assertAlmostEqual(bounds.ZMin, 0, delta=1e-5)
        self.assertAlmostEqual(bounds.ZMax, 7, delta=1e-5)
        permitted = Part.makeBox(40, 30, 7, App.Vector(-20, -15, 0))
        self.assertAlmostEqual(removed.cut(permitted).Volume, 0, delta=1e-5)

    def testCrossPocketDraft(self):
        self.testThinPocketExtensionModes()
        pocket = self.doc.Pocket
        pocket.ThinExtensionEdges = None
        pocket.ThinExtendAll = True
        source_length = self.doc.CutProfile.Shape.Length
        height, thickness = 5, 2
        for mode in ("Off", "Tangent", "Natural"):
            pocket.ThinExtension = mode
            total_length = 26 if mode == "Off" else 70
            for neutral in ("Root", "Top"):
                pocket.ThinDraftReference = neutral
                sign = -1 if neutral == "Root" else 1
                for angle in (-2, 2):
                    pocket.TaperAngle = angle
                    self.doc.recompute()
                    self.valid(pocket)
                    slope = math.tan(math.radians(angle))
                    # Integrate the cross area (Lx+Ly)*w-w*w, where
                    # w varies linearly with depth about the neutral plane.
                    width_integral = thickness * height + sign * height**2 * slope
                    width2_integral = (
                        thickness**2 * height
                        + sign * 2 * thickness * height**2 * slope
                        + 4 / 3 * height**3 * slope**2
                    )
                    expected_cut = total_length * width_integral - width2_integral
                    self.assertAlmostEqual(
                        self.doc.Block.Shape.Volume - pocket.Shape.Volume, expected_cut, delta=1e-5
                    )
                    self.assertAlmostEqual(
                        self.doc.CutProfile.Shape.Length, source_length, delta=1e-7
                    )
        volume = pocket.Shape.Volume
        self.doc.PocketBody.Placement = App.Placement(
            App.Vector(17, 23, -9), App.Rotation(App.Vector(1, 2, 3), 37)
        )
        pocket.touch()
        self.doc.recompute()
        self.valid(pocket)
        self.assertAlmostEqual(pocket.Shape.Volume, volume, delta=1e-5)

    def testCrossPadDraft(self):
        body, base, sketch, pad, inner = self.bowlWall(kind="PartDesign::Pad", cross=True)
        for angle in (-2, 2):
            pad.TaperAngle = angle
            self.doc.recompute()
            self.valid(pad)
            top_width = 2 - 36 * math.tan(math.radians(angle))

            def strip(swap):
                sections = []
                for z, width in ((0, 2), (18, top_width)):
                    points = [
                        (-100, -width / 2),
                        (100, -width / 2),
                        (100, width / 2),
                        (-100, width / 2),
                    ]
                    points.append(points[0])
                    sections.append(
                        Part.makePolygon(
                            [
                                App.Vector(y, x, z) if swap else App.Vector(x, y, z)
                                for x, y in points
                            ]
                        )
                    )
                return Part.makeLoft(sections, True, True)

            expected = strip(False).fuse(strip(True)).common(inner)
            self.assertAlmostEqual(pad.AddSubShape.cut(expected).Volume, 0, delta=1e-5)
            self.assertAlmostEqual(expected.cut(pad.AddSubShape).Volume, 0, delta=1e-5)

    def testDisconnectedPocketDraft(self):
        self.testThinPocketExtensionModes()
        pocket = self.doc.Pocket
        sketch = self.doc.CutProfile
        sketch.delGeometry(1)
        sketch.delGeometry(0)
        for y in (-6, 6):
            sketch.addGeometry(Part.LineSegment(App.Vector(-8, y, 0), App.Vector(8, y, 0)), False)
        pocket.ThinExtension = "Off"
        pocket.ThinExtensionEdges = None
        pocket.ThinExtendAll = True
        for angle in (-2, 2):
            pocket.TaperAngle = angle
            self.doc.recompute()
            self.valid(pocket)
            expected_cut = 32 * (10 - 25 * math.tan(math.radians(angle)))
            self.assertAlmostEqual(
                self.doc.Block.Shape.Volume - pocket.Shape.Volume, expected_cut, delta=1e-5
            )

    def testThinPadBowlRimJunctionFillet(self):
        body, base, sketch, wall, inner = self.bowlWall(cross=True, kind="PartDesign::Pad")
        wall.Type = "UpToFace"
        wall.UpToFace = (base, ["Face2"])
        self.doc.recompute()
        self.valid(wall)
        sharp = wall.Shape.copy()
        wall.RootFilletRadius = 0.5
        self.doc.recompute()
        self.valid(wall)
        self.assertGreater(wall.Shape.Volume, sharp.Volume)
        # Spline control-polygon bounds may overestimate the finished fillet.
        self.assertAlmostEqual(wall.Shape.optimalBoundingBox(False, False).ZMax, 30, delta=1e-5)
        above = Part.makeBox(200, 200, 30, App.Vector(-100, -100, 30))
        self.assertAlmostEqual(wall.Shape.common(above).Volume, 0, delta=1e-5)
        self.assertAlmostEqual(sharp.cut(wall.Shape).Volume, 0, delta=1e-5)
        rebuilt = base.Shape.fuse(wall.AddSubShape)
        self.assertAlmostEqual(rebuilt.cut(wall.Shape).Volume, 0, delta=1e-5)
        self.assertAlmostEqual(wall.Shape.cut(rebuilt).Volume, 0, delta=1e-5)

    def testSpatialSharpGrowthAndWidths(self):
        body = self.doc.addObject("PartDesign::Body", "Body")
        profile = body.newObject("PartDesign::Feature", "SpatialProfile")
        points = [App.Vector(*p) for p in ((0, 0, 0), (30, 0, 3), (30, 20, 0), (10, 25, 4))]
        profile.Shape = Part.makePolygon(points)
        self.doc.recompute()
        rib = body.newObject(self.featureType, "Rib")
        rib.Profile = profile
        rib.RibMode = "SpatialWeb"
        rib.FillDirection = App.Vector(0, 0, 1)
        rib.Length = 12
        rib.ThinThickness = 2
        for side, a, b in (
            ("SideA", 0, 2),
            ("SideB", -2, 0),
            ("Centered", -1, 1),
            ("Two sides", -0.5, 2),
        ):
            rib.ThinSide = side
            rib.ThinThickness2 = 0.5
            for reversed in (False, True):
                rib.Reversed = reversed
                self.doc.recompute()
                self.valid(rib)
                height = -6 if reversed else 6
                self.assertLess(rib.Shape.getTolerance(1), 1e-4)
                section = rib.Shape.section(Part.makePlane(100, 100, App.Vector(-20, -20, height)))
                area = Part.Face(Part.Wire(Part.sortEdges(section.Edges)[0])).Area
                expected = (50 + math.sqrt(425)) * (b - a) - (1 + (math.sqrt(425) - 5) / 20) * (
                    b * b - a * a
                )
                self.assertAlmostEqual(area, expected, delta=1e-5)

    def testSpatialStartAndSymmetricDepth(self):
        self.testSpatialSharpGrowthAndWidths()
        rib = self.doc.getObject("Rib")
        rib.Reversed = False
        rib.ThinSide = "Centered"
        self.doc.recompute()
        self.valid(rib)
        expected = rib.Shape.copy()
        expected.translate(App.Vector(0, 0, -3))
        rib.StartType = "Offset"
        rib.StartOffset = 3
        rib.SideType = "Symmetric"
        self.doc.recompute()
        self.valid(rib)
        self.assertAlmostEqual(rib.Shape.Volume, expected.Volume, delta=1e-5)
        self.assertAlmostEqual(rib.Shape.common(expected).Volume, expected.Volume, delta=1e-5)

    def testSpatialDirectionReferences(self):
        self.testSpatialSharpGrowthAndWidths()
        rib = self.doc.getObject("Rib")
        body = self.doc.getObject("Body")
        rib.Reversed = False
        rib.ThinSide = "Centered"
        self.doc.recompute()
        expected = rib.Shape.copy()
        rib.UseCustomVector = True
        rib.Direction = App.Vector(0, 0, 1)
        self.doc.recompute()
        self.valid(rib)
        self.assertAlmostEqual(rib.Shape.cut(expected).Volume, 0, delta=1e-5)
        self.assertAlmostEqual(expected.cut(rib.Shape).Volume, 0, delta=1e-5)
        rib.UseCustomVector = False
        axis = next(o for o in body.Origin.OriginFeatures if o.Name.startswith("Z_Axis"))
        rib.ReferenceAxis = (axis, [""])
        self.doc.recompute()
        self.valid(rib)
        self.assertAlmostEqual(rib.Shape.cut(expected).Volume, 0, delta=1e-5)
        self.assertAlmostEqual(expected.cut(rib.Shape).Volume, 0, delta=1e-5)

    def testSpatialNetwork(self):
        body = self.doc.addObject("PartDesign::Body", "Body")
        profile = body.newObject("PartDesign::Feature", "SpatialProfile")
        profile.Shape = Part.makeCompound(
            [
                Part.makeLine(App.Vector(0, 0, 0), App.Vector(30, 0, 3)),
                Part.makeLine(App.Vector(15, -10, 1.5), App.Vector(15, 10, 1.5)),
                Part.makeLine(App.Vector(15, 10, 1.5), App.Vector(25, 15, 4)),
            ]
        )
        rib = body.newObject(self.featureType, "Rib")
        rib.Profile = profile
        rib.RibMode = "SpatialWeb"
        rib.FillDirection = App.Vector(0, 0, 1)
        rib.Length = 12
        rib.ThinThickness = 2
        for cap in ("Flat", "Round"):
            rib.ThinCap = cap
            self.doc.recompute()
            self.valid(rib)
            self.assertTrue(rib.Shape.isInside(App.Vector(15, 0, 6), 1e-7, False))
            rib.Shape.check(True)
            # Intersections between curved caps may need sub-micron tolerance,
            # but must not hide a macroscopic gap at the network junction.
            self.assertLess(rib.Shape.getTolerance(1), 1e-4)

    def testSpatialTwoIndependentTargets(self):
        body = self.doc.addObject("PartDesign::Body", "Body")
        profile = body.newObject("PartDesign::Feature", "SpatialProfile")
        profile.Shape = Part.makePolygon(
            [App.Vector(0, 0, 0), App.Vector(20, 0, 1), App.Vector(20, 15, 0)]
        )
        targets = []
        for name, z in (("Upper", 10), ("Lower", -8)):
            reference = self.doc.addObject("Part::Feature", name)
            reference.Shape = Part.makePlane(60, 60, App.Vector(-20, -20, z))
            binder = body.newObject("PartDesign::SubShapeBinder", name + "Binder")
            binder.Support = [(reference, ["Face1"])]
            targets.append(binder)
        body.Tip = profile
        self.doc.recompute()
        rib = body.newObject(self.featureType, "Rib")
        rib.Profile = profile
        rib.RibMode = "SpatialWeb"
        rib.FillDirection = App.Vector(0, 0, 1)
        rib.ThinThickness = 2
        rib.SideType = "Two sides"
        rib.Type = rib.Type2 = "UpToFace"
        rib.UpToFace = (targets[0], ["Face1"])
        rib.UpToFace2 = (targets[1], ["Face1"])
        self.doc.recompute()
        self.valid(rib)
        rib.Shape.check(True)
        self.assertAlmostEqual(rib.Shape.Volume, 35 * 2 * 18, delta=1e-5)
        self.assertAlmostEqual(rib.Shape.BoundBox.ZMin, -8, delta=1e-6)
        self.assertAlmostEqual(rib.Shape.BoundBox.ZMax, 10, delta=1e-6)

    def testSpatialSpline(self):
        body = self.doc.addObject("PartDesign::Body", "Body")
        profile = body.newObject("PartDesign::Feature", "SpatialProfile")
        curve = Part.BSplineCurve()
        curve.interpolate(
            [App.Vector(0, 0, 0), App.Vector(10, 3, 2), App.Vector(20, -2, 1), App.Vector(30, 0, 4)]
        )
        profile.Shape = curve.toShape()
        rib = body.newObject(self.featureType, "Rib")
        rib.Profile = profile
        rib.RibMode = "SpatialWeb"
        rib.FillDirection = App.Vector(0, 0, 1)
        rib.Length = 12
        rib.ThinThickness = 2
        rib.ThinSide = "Centered"
        self.doc.recompute()
        self.valid(rib)
        rib.Shape.check(True)
        self.assertLess(rib.Shape.getTolerance(1), 1e-4)
        points = profile.Shape.discretize(5000)
        projected_length = sum(math.hypot(b.x - a.x, b.y - a.y) for a, b in zip(points, points[1:]))
        self.assertAlmostEqual(rib.Shape.Volume, projected_length * 2 * 12, delta=1e-2)

    def testSpatialRoundJoins(self):
        self.testSpatialSharpGrowthAndWidths()
        rib = self.doc.getObject("Rib")
        rib.ThinJoin = "Round"
        for side in ("SideA", "SideB", "Centered", "Two sides"):
            for reverse in (False, True):
                with self.subTest(side=side, reverse=reverse):
                    rib.ThinSide = side
                    rib.Reversed = reverse
                    self.doc.recompute()
                    self.valid(rib)
                    rib.Shape.check(True)

    def testRoundEndsWithReversedSourceEdge(self):
        self.testSpatialSharpGrowthAndWidths()
        rib = self.doc.getObject("Rib")
        source = self.doc.getObject("SpatialProfile")
        rib.Reversed = False
        rib.ThinSide = "SideA"
        rib.ThinCap = "Round"
        self.doc.recompute()
        self.valid(rib)
        expected = rib.Shape.copy()
        edges = source.Shape.Edges
        edge = edges[-1]
        edges[-1] = Part.makeLine(edge.Vertexes[-1].Point, edge.Vertexes[0].Point)
        source.Shape = Part.makeCompound(edges)
        self.doc.recompute()
        self.valid(rib)
        self.assertAlmostEqual(expected.cut(rib.Shape).Volume, 0, delta=1e-5)
        self.assertAlmostEqual(rib.Shape.cut(expected).Volume, 0, delta=1e-5)

    def testSpatialRoundEnds(self):
        self.testSpatialSharpGrowthAndWidths()
        rib = self.doc.getObject("Rib")
        for side in ("SideA", "SideB", "Centered", "Two sides"):
            for reverse in (False, True):
                with self.subTest(side=side, reverse=reverse):
                    rib.ThinSide = side
                    rib.Reversed = reverse
                    rib.ThinCap = "Flat"
                    self.doc.recompute()
                    self.valid(rib)
                    uncapped = rib.Shape.copy()
                    rib.ThinCap = "Round"
                    self.doc.recompute()
                    self.valid(rib)
                    width = 2.5 if side == "Two sides" else 2
                    self.assertAlmostEqual(uncapped.cut(rib.Shape).Volume, 0, delta=1e-5)
                    self.assertAlmostEqual(
                        rib.Shape.cut(uncapped).Volume, math.pi * (width / 2) ** 2 * 12, delta=1e-5
                    )

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

    def testExtendedWebNetwork(self):
        body = self.doc.addObject("PartDesign::Body", "Body")
        base = body.newObject("PartDesign::Feature", "Base")
        base.Shape = Part.makeBox(40, 30, 18).cut(Part.makeBox(36, 26, 18, App.Vector(2, 2, 3)))
        sketch = body.newObject("Sketcher::SketchObject", "Profile")
        sketch.addGeometry(Part.LineSegment(App.Vector(8, 15, 0), App.Vector(32, 15, 0)), False)
        sketch.addGeometry(Part.LineSegment(App.Vector(20, 8, 0), App.Vector(20, 23, 0)), False)
        sketch.Placement.Base.z = 13
        self.doc.recompute()
        rib = body.newObject(self.featureType, "Rib")
        rib.Profile = sketch
        rib.ThinThickness = 2
        rib.Reversed = True
        rib.Type = "UpToFirst"
        for mode in ("Tangent", "Natural"):
            rib.Extension = mode
            self.doc.recompute()
            self.valid(rib)
            self.assertAlmostEqual(rib.Shape.Volume, base.Shape.Volume + 1200, delta=1e-5)

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
