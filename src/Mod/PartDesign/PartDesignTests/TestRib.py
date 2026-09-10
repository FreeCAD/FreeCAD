# SPDX-License-Identifier: LGPL-2.1-or-later
import math
import os
import tempfile
import unittest
import FreeCAD as App
import Part
import Sketcher


class TestRib(unittest.TestCase):
    featureType = "PartDesign::Rib"

    def setUp(self):
        self.doc = App.newDocument("TestRib")

    def tearDown(self):
        App.closeDocument(self.doc.Name)

    def valid(self, rib):
        self.assertNotIn("Invalid", rib.State, rib.getStatusString())
        self.assertTrue(rib.Shape.isValid())
        self.assertEqual(len(rib.Shape.Solids), 1)

    def testIsolatedFeatureAndDefaults(self):
        rib = self.doc.addObject(self.featureType, "Rib")
        self.assertTrue(rib.isDerivedFrom("PartDesign::ProfileBased"))
        self.assertFalse(rib.isDerivedFrom("PartDesign::FeatureExtrude"))
        self.assertFalse(rib.isDerivedFrom("PartDesign::Pad"))
        self.assertEqual(rib.Type, "UpToShape")
        self.assertEqual(rib.Extension, "Tangent")
        self.assertEqual(rib.ThinSide, "Centered")
        self.assertTrue(rib.AutoDirection)
        self.assertNotIn("RibMode", rib.PropertiesList)
        self.assertNotIn("ThinExtension", rib.PropertiesList)
        for kind in ("Pad", "Pocket"):
            feature = self.doc.addObject("PartDesign::" + kind, kind)
            for name in ("Thin", "ThinThickness", "ThinExtension", "RootFilletRadius"):
                self.assertNotIn(name, feature.PropertiesList)

    def testSourceGeometryIsNotMutated(self):
        self.testRegularSplineC2Extension()
        rib, sketch, base = self.doc.Rib, self.doc.Profile, self.doc.Base
        before = sketch.Shape.exportBrepToString(), base.Shape.exportBrepToString()
        for width in (1, 3, 2):
            rib.ThinThickness = width
            self.doc.recompute()
            self.valid(rib)
            self.assertEqual(before[0], sketch.Shape.exportBrepToString())
            self.assertEqual(before[1], base.Shape.exportBrepToString())
        rib.ThinThickness = 0
        self.doc.recompute()
        self.assertIn("Invalid", rib.State)
        self.assertEqual(before[0], sketch.Shape.exportBrepToString())
        self.assertEqual(before[1], base.Shape.exportBrepToString())

    def testPatternIncludesRootFillets(self):
        self.testSideProfileRib()
        rib, base, body = self.doc.Rib, self.doc.Base, self.doc.Body
        rib.RootFilletRadius = 0.3
        self.doc.recompute()
        self.valid(rib)
        material = rib.AddSubShape.copy()
        pattern = body.newObject("PartDesign::LinearPattern", "RibPattern")
        pattern.Originals = [rib]
        pattern.Direction = (self.doc.Profile, ["N_Axis"])
        pattern.Length = 6
        pattern.Occurrences = 2
        self.doc.recompute()
        self.valid(pattern)
        self.assertAlmostEqual(
            pattern.Shape.Volume, base.Shape.Volume + 2 * material.Volume, delta=1e-5
        )
        body.Placement = App.Placement(
            App.Vector(23, -17, 41), App.Rotation(App.Vector(1, 2, 3), 47)
        )
        rib.ThinThickness = 3
        self.doc.recompute()
        self.valid(pattern)
        self.assertAlmostEqual(
            pattern.Shape.Volume, base.Shape.Volume + 2 * rib.AddSubShape.Volume, delta=1e-5
        )

    def testUndoRedoThickness(self):
        self.testSideProfileRib()
        rib = self.doc.Rib
        original = rib.Shape.Volume
        self.doc.openTransaction("Change Rib Thickness")
        rib.ThinThickness = 4
        self.doc.recompute()
        self.doc.commitTransaction()
        wider = rib.Shape.Volume
        self.assertGreater(wider, original)
        self.doc.undo()
        self.doc.recompute()
        self.valid(rib)
        self.assertAlmostEqual(rib.Shape.Volume, original, delta=1e-5)
        self.doc.redo()
        self.doc.recompute()
        self.valid(rib)
        self.assertAlmostEqual(rib.Shape.Volume, wider, delta=1e-5)

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
        rib.AutoDirection = False
        rib.Extension = "Off"
        rib.Type = "Length"
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

    def testTowardCornerEdge(self):
        rib = self.towardCorner()
        expected = App.Vector(-13, 0, -10).normalize()
        self.assertLess((rib.Direction - expected).Length, 1e-7)
        self.assertAlmostEqual(rib.Shape.Volume - self.doc.Base.Shape.Volume, 26 * 20, delta=1e-5)
        self.assertAlmostEqual(rib.Shape.optimalBoundingBox().ZMin, -3, delta=1e-7)

    def testTowardCornerPlaced(self):
        rib = self.towardCorner()
        expected = rib.Shape.copy()
        placement = App.Placement(App.Vector(23, -17, 41), App.Rotation(App.Vector(1, 2, 3), 47))
        self.doc.Body.Placement = placement
        self.doc.recompute()
        self.valid(rib)
        expected.Placement = placement.multiply(expected.Placement)
        self.assertAlmostEqual(self.doc.Body.Shape.cut(expected).Volume, 0, delta=1e-5)
        self.assertAlmostEqual(expected.cut(self.doc.Body.Shape).Volume, 0, delta=1e-5)

    def testTowardReferenceReversedAndCustom(self):
        rib = self.towardCorner()
        original = rib.Shape.copy()
        # Existing custom-vector precedence and reversal are unchanged.
        direction = rib.Direction
        rib.UseCustomVector = True
        rib.Direction = -direction
        rib.Reversed = True
        self.doc.recompute()
        self.valid(rib)
        self.assertAlmostEqual(original.cut(rib.Shape).Volume, 0, delta=1e-5)
        self.assertAlmostEqual(rib.Shape.cut(original).Volume, 0, delta=1e-5)

    def testTowardReferenceMissingAndInvalid(self):
        self.testSideProfileRib()
        rib = self.doc.Rib
        self.assertFalse(rib.TowardReference)
        rib.TowardReference = True
        self.doc.recompute()
        self.assertIn("Invalid", rib.State)
        empty = self.doc.addObject("PartDesign::Feature", "EmptyReference")
        rib.ReferenceAxis = empty
        self.doc.recompute()
        self.assertIn("Invalid", rib.State)
        rib.TowardReference = False
        rib.ReferenceAxis = None
        self.doc.recompute()
        self.valid(rib)

    def testReferenceGeometryTypes(self):
        self.testSideProfileRib()
        rib = self.doc.Rib
        expected = rib.Shape.Volume
        center = App.Vector(16, 12, 0)
        down = App.Vector(0, 0, -1)
        refs = []
        for name, shape in (
            ("VertexReference", Part.Vertex(center)),
            ("EdgeReference", Part.makeLine(App.Vector(0, 12, 0), App.Vector(32, 12, 0))),
            ("FaceReference", Part.makePlane(40, 24)),
            ("SolidReference", Part.makeBox(36, 24, 3, App.Vector(0, 0, -3))),
        ):
            obj = self.doc.addObject("PartDesign::Feature", name)
            obj.Shape = shape
            refs.append((obj, [""]))
            refs.append(
                (
                    obj,
                    [
                        (
                            "Vertex1"
                            if name == "VertexReference"
                            else "Edge1" if name == "EdgeReference" else "Face1"
                        )
                    ],
                )
            )
        for typeName in (
            "PartDesign::Point",
            "PartDesign::Line",
            "PartDesign::Plane",
            "App::Point",
            "App::Line",
            "App::Plane",
        ):
            obj = self.doc.addObject(typeName, "DatumReference")
            obj.Placement.Base = center
            if typeName.endswith("Line"):
                obj.Placement.Rotation = App.Rotation(
                    App.Vector(1, 0, 0) if typeName == "App::Line" else App.Vector(0, 0, 1),
                    App.Vector(1, 0, 0),
                )
            refs.append((obj, [""]))
        rib.TowardReference = True
        for ref in refs:
            with self.subTest(reference=ref[0].Name, sub=ref[1]):
                rib.ReferenceAxis = ref
                self.doc.recompute()
                # Face1 of a box is a different (but valid) nearest-face direction.
                if ref[0].Name == "SolidReference" and ref[1] == ["Face1"]:
                    self.valid(rib)
                    self.assertLess(
                        (rib.Direction - App.Vector(-16, 0, -10).normalize()).Length, 1e-7
                    )
                    continue
                self.valid(rib)
                self.assertLess((rib.Direction - down).Length, 1e-7)
                self.assertAlmostEqual(rib.Shape.Volume, expected, delta=1e-5)
                rib.TaperAngle = 0.5
                rib.DraftPullMode = "TowardReference"
                rib.DraftPullDirection = ref
                self.doc.recompute()
                self.valid(rib)
                rib.TaperAngle = 0
                rib.DraftPullMode = "Automatic"
                rib.DraftPullDirection = None

    def testAxisReferenceGeometryTypes(self):
        self.testSideProfileRib()
        rib = self.doc.Rib
        expected = rib.Shape.Volume
        refs = []
        for name, shape in (
            ("LineReference", Part.makeLine(App.Vector(0, 0, 0), App.Vector(0, 0, 10))),
            ("CircleReference", Part.makeCircle(4)),
            ("FaceReference", Part.makePlane(40, 24)),
            ("CylinderReference", Part.makeCylinder(4, 10).Faces[0]),
            (
                "WireReference",
                Part.Wire([Part.makeLine(App.Vector(0, 0, 0), App.Vector(0, 0, 10))]),
            ),
        ):
            obj = self.doc.addObject("PartDesign::Feature", name)
            obj.Shape = shape
            refs.append((obj, [""]))
        for typeName in ("PartDesign::Line", "PartDesign::Plane", "App::Line", "App::Plane"):
            obj = self.doc.addObject(typeName, "AxisDatum")
            if typeName == "App::Line":
                obj.Placement.Rotation = App.Rotation(App.Vector(1, 0, 0), App.Vector(0, 0, 1))
            refs.append((obj, [""]))
        refs.extend(
            (o, [""])
            for o in self.doc.Body.Origin.OriginFeatures
            if o.Name.startswith(("Z_Axis", "XY_Plane"))
        )
        rib.Reversed = True
        rib.TowardReference = False
        for ref in refs:
            with self.subTest(reference=ref[0].Name):
                rib.ReferenceAxis = ref
                self.doc.recompute()
                self.valid(rib)
                self.assertLess((rib.Direction - App.Vector(0, 0, 1)).Length, 1e-7)
                self.assertAlmostEqual(rib.Shape.Volume, expected, delta=1e-5)
                rib.DraftPullDirection = ref
                rib.DraftPullMode = "ParallelToReference"
                rib.TaperAngle = 0.5
                self.doc.recompute()
                self.valid(rib)
                rib.TaperAngle = 0
                rib.DraftPullMode = "Automatic"
                rib.DraftPullDirection = None

    def testAmbiguousReferenceDirections(self):
        self.testSideProfileRib()
        rib = self.doc.Rib
        reference = self.doc.addObject("PartDesign::Feature", "Reference")
        for toward, shape, message in (
            (False, Part.Vertex(App.Vector(16, 12, 0)), "no unique axis"),
            (False, Part.makeBox(2, 2, 2), "multiple axis directions"),
            (True, Part.Vertex(App.Vector(16, 12, 10)), "does not define a direction"),
            (
                True,
                Part.makeCompound(
                    [Part.Vertex(App.Vector(16, 12, 0)), Part.Vertex(App.Vector(16, 12, 20))]
                ),
                "multiple nearest points",
            ),
        ):
            with self.subTest(toward=toward, message=message):
                reference.Shape = shape
                rib.TowardReference = toward
                rib.ReferenceAxis = reference
                self.doc.recompute()
                self.assertIn("Invalid", rib.State)
                self.assertIn(message, rib.getStatusString())

    def testTowardReferencePersistence(self):
        rib = self.towardCorner()
        expected = rib.Shape.Volume
        reference = rib.ReferenceAxis[1]
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "TowardCorner.FCStd")
            self.doc.saveAs(path)
            App.closeDocument(self.doc.Name)
            self.doc = App.openDocument(path)
            self.doc.Rib.touch()
            self.doc.recompute()
            self.valid(self.doc.Rib)
            self.assertTrue(self.doc.Rib.TowardReference)
            self.assertEqual(self.doc.Rib.ReferenceAxis[1], reference)
            self.assertAlmostEqual(self.doc.Rib.Shape.Volume, expected, delta=1e-5)

    def testTowardReferenceEdgeOrientationAndMotion(self):
        rib = self.towardCorner()
        base, subs = rib.ReferenceAxis
        reference = self.doc.addObject("PartDesign::Feature", "DirectionReference")
        reference.Shape = base.Shape.getElement(subs[0]).reversed()
        rib.ReferenceAxis = (reference, ["Edge1"])
        self.doc.recompute()
        self.valid(rib)
        self.assertLess((rib.Direction - App.Vector(-13, 0, -10).normalize()).Length, 1e-7)
        reference.Placement.Base.x = -2
        self.doc.recompute()
        self.valid(rib)
        self.assertLess((rib.Direction - App.Vector(-15, 0, -10).normalize()).Length, 1e-7)
        # A line through the center cannot determine which way to grow.
        reference.Placement.Base = App.Vector(13, 0, 10)
        self.doc.recompute()
        self.assertIn("Invalid", rib.State)

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

    def testFiniteSideRibPlacedAndReversed(self):
        self.testFiniteSideRibStopsAtBody()
        rib = self.doc.getObject("Rib")
        body = self.doc.getObject("Body")
        expected = rib.Shape.copy()
        # The opposite vector plus Reversed must describe exactly the same cut.
        rib.FillDirection = App.Vector(0, 1, 0)
        rib.Reversed = True
        body.Placement = App.Placement(
            App.Vector(23, -17, 41), App.Rotation(App.Vector(1, 2, 3), 47)
        )
        self.doc.recompute()
        self.valid(rib)
        expected.Placement = body.Placement.multiply(expected.Placement)
        actual = body.Shape
        self.assertAlmostEqual(actual.cut(expected).Volume, 0, delta=1e-5)
        self.assertAlmostEqual(expected.cut(actual).Volume, 0, delta=1e-5)

    def testFiniteSideRibDraftNeutralPlane(self):
        self.testFiniteSideRibStopsAtBody()
        rib = self.doc.getObject("Rib")
        rib.Length = 7
        rib.TaperAngle = 1
        rib.ThinDraftReference = "Root"
        self.doc.recompute()
        self.valid(rib)
        # The root reference is the trimmed floor at z=0, not the original
        # finite prism's z=-7 endpoint. Measure within the right-hand foot.
        section = rib.Shape.common(Part.makeBox(1, 24, 0.001, App.Vector(27, 0, 0.5)))
        expected = 2 - 2 * 0.5 * math.tan(math.radians(1))
        self.assertAlmostEqual(section.BoundBox.YLength, expected, delta=1e-5)
        self.assertAlmostEqual(rib.Shape.optimalBoundingBox(False).ZMin, -3, delta=1e-6)

    def testFiniteSideRibCurvedBody(self):
        body = self.doc.addObject("PartDesign::Body", "Body")
        base = body.newObject("PartDesign::Feature", "Base")
        base.Shape = Part.makeCylinder(20, 10, App.Vector(0, -5, 0), App.Vector(0, 1, 0))
        sketch = body.newObject("Sketcher::SketchObject", "Profile")
        sketch.addGeometry(Part.LineSegment(App.Vector(-10, 25, 0), App.Vector(10, 25, 0)), False)
        sketch.Placement.Rotation = App.Rotation(App.Vector(1, 0, 0), 90)
        rib = body.newObject(self.featureType, "Rib")
        rib.Profile = sketch
        rib.AutoDirection = False
        rib.Extension = "Off"
        rib.Type = "Length"
        rib.Length = 50
        rib.ThinThickness = 2
        self.doc.recompute()
        self.valid(rib)
        # No re-emergence below a curved body, and the actual circular root
        # remains intact (no world-axis or planar bounding-box clipping).
        self.assertAlmostEqual(rib.Shape.BoundBox.ZMin, -20, delta=1e-6)
        self.assertFalse(rib.Shape.isInside(App.Vector(9, 0, -23), 1e-7, False))
        integral = 10 * math.sqrt(300) + 400 * math.asin(0.5)
        self.assertAlmostEqual(
            rib.Shape.Volume - base.Shape.Volume, 2 * (20 * 25 - integral), delta=1e-4
        )

    def testFiniteSideRibTwoDirections(self):
        body = self.doc.addObject("PartDesign::Body", "Body")
        base = body.newObject("PartDesign::Feature", "Base")
        base.Shape = Part.makeBox(36, 24, 3, App.Vector(0, 0, -8)).fuse(
            [
                Part.makeBox(36, 24, 3, App.Vector(0, 0, 6)),
                Part.makeBox(3, 24, 17, App.Vector(0, 0, -8)),
            ]
        )
        sketch = body.newObject("Sketcher::SketchObject", "Profile")
        sketch.addGeometry(Part.LineSegment(App.Vector(3, 0, 0), App.Vector(29, 0, 0)), False)
        sketch.Placement = App.Placement(
            App.Vector(0, 12, 0), App.Rotation(App.Vector(1, 0, 0), 90)
        )
        rib = body.newObject(self.featureType, "Rib")
        rib.Profile = sketch
        rib.AutoDirection = False
        rib.Extension = "Off"
        rib.Type = "Length"
        rib.SideType = "Two sides"
        rib.Type = "Length"
        rib.Type2 = "Length"
        rib.Length = 14
        rib.Length2 = 17
        rib.ThinThickness = 2
        self.doc.recompute()
        self.valid(rib)
        self.assertAlmostEqual(rib.Shape.Volume - base.Shape.Volume, 26 * 2 * (5 + 6), delta=1e-5)
        self.assertAlmostEqual(rib.Shape.BoundBox.ZMin, -8, delta=1e-6)
        self.assertAlmostEqual(rib.Shape.BoundBox.ZMax, 9, delta=1e-6)

    def testFiniteSideRibManualProfiles(self):
        # Curved and sharp profiles from the illustrated gusset family. Check
        # the complete result rather than only the first straight wall.
        self.testSideProfileRib()
        body = self.doc.getObject("Body")
        base = self.doc.getObject("Base")
        rib = self.doc.getObject("Rib")
        rib.Type = "Length"
        rib.Length = 7
        curve = Part.BSplineCurve()
        curve.interpolate(
            [App.Vector(3, 20, 0), App.Vector(7, 18, 0), App.Vector(13, 8, 0), App.Vector(29, 0, 0)]
        )
        for i, geometry in enumerate(
            (
                [curve],
                [
                    Part.LineSegment(App.Vector(3, 20, 0), App.Vector(10, 20, 0)),
                    Part.LineSegment(App.Vector(10, 20, 0), App.Vector(29, 3, 0)),
                ],
            )
        ):
            sketch = body.newObject("Sketcher::SketchObject", "AdditionalProfile")
            sketch.addGeometry(geometry, False)
            sketch.Placement = App.Placement(
                App.Vector(0, 5 + 14 * i, 0), App.Rotation(App.Vector(1, 0, 0), 90)
            )
            rib = body.newObject(self.featureType, "AdditionalRib")
            rib.Profile = sketch
            rib.AutoDirection = False
            rib.Extension = "Off"
            rib.Type = "Length"
            rib.Length = 7
            rib.ThinThickness = 2
            self.doc.recompute()
            self.valid(rib)
        rib.Shape.check(True)
        below = Part.makeBox(60, 40, 20, App.Vector(-10, -5, -23))
        self.assertAlmostEqual(rib.Shape.common(below).Volume, 0, delta=1e-6)
        self.assertAlmostEqual(base.Shape.cut(rib.Shape).Volume, 0, delta=1e-6)
        # Finishing must operate on the body-limited wall, not reintroduce its
        # original below-floor prism.
        rib.RootFilletRadius = 0.3
        self.doc.recompute()
        self.valid(rib)
        self.assertAlmostEqual(rib.Shape.common(below).Volume, 0, delta=1e-6)

    def testSingularSplineEndpointExtension(self):
        self.testSideProfileRib()
        rib, base, sketch = self.doc.Rib, self.doc.Base, self.doc.Profile
        base.Shape = Part.makeBox(50, 24, 3, App.Vector(0, 0, -3)).fuse(Part.makeBox(3, 24, 45))
        spline = Part.BSplineCurve()
        spline.buildFromPolesMultsKnots(
            [
                App.Vector(x, y, 0)
                for x, y in ((10, 30), (12, 27), (32, 16), (41, 13), (42, 9), (42, 9))
            ],
            [4, 1, 1, 4],
            [0, 1, 2, 3],
            False,
            3,
        )
        sketch.delGeometry(0)
        sketch.addGeometry(spline, False)
        rib.Type = "UpToShape"
        rib.ThinThickness = 2
        poles = sketch.Geometry[0].getPoles()
        for mode in ("Off", "Tangent"):
            rib.Extension = mode
            self.doc.recompute()
            self.valid(rib)
            self.assertEqual(sketch.Geometry[0].getPoles(), poles)
        extended = rib.Shape.Volume
        rib.Extension = "Natural"
        self.doc.recompute()
        self.assertIn("Invalid", rib.State)
        self.assertIn("C2 extension requires a regular spline endpoint", rib.getStatusString())
        rib.Extension = "Tangent"
        self.doc.recompute()
        self.valid(rib)
        self.assertAlmostEqual(rib.Shape.Volume, extended, delta=1e-5)
        added = rib.Shape.cut(base.Shape)
        floor = next(
            f
            for f in base.Shape.Faces
            if f.normalAt(0, 0).z > 0.99 and abs(f.CenterOfMass.z) < 1e-6
        )
        wall = next(
            f
            for f in base.Shape.Faces
            if f.normalAt(0, 0).x > 0.99 and abs(f.CenterOfMass.x - 3) < 1e-6
        )
        self.assertGreater(added.common(floor).Area, 1)
        self.assertGreater(added.common(wall).Area, 1)
        self.assertAlmostEqual(added.BoundBox.ZMin, 0, delta=1e-6)
        self.assertAlmostEqual(added.BoundBox.XMin, 3, delta=1e-6)

    def testRegularSplineC2Extension(self):
        self.testSideProfileRib()
        rib, base, sketch = self.doc.Rib, self.doc.Base, self.doc.Profile
        base.Shape = Part.makeBox(80, 24, 3, App.Vector(0, 0, -3)).fuse(Part.makeBox(3, 24, 45))
        curve = Part.BSplineCurve()
        curve.buildFromPolesMultsKnots(
            [App.Vector(x, y, 0) for x, y in ((8, 30), (12, 25), (25, 15), (38, 10))],
            [4, 4],
            [0, 1],
            False,
            3,
        )
        sketch.delGeometry(0)
        sketch.addGeometry(curve, False)
        rib.Type = "UpToShape"
        for mode in ("Tangent", "Natural"):
            rib.Extension = mode
            self.doc.recompute()
            self.valid(rib)
            self.assertEqual(sketch.Geometry[0].getPoles(), curve.getPoles())

    def testSmoothSplineRibTopology(self):
        self.testRegularSplineC2Extension()
        rib, base, sketch = self.doc.Rib, self.doc.Base, self.doc.Profile
        poles = sketch.Geometry[0].getPoles()
        for mode, continuity in (("Tangent", "C1"), ("Natural", "C2")):
            for placement in ("SideA", "SideB", "Centered"):
                for refine in (False, True):
                    with self.subTest(mode=mode, placement=placement, refine=refine):
                        rib.Extension = mode
                        rib.ThinSide = placement
                        rib.Refine = refine
                        self.doc.recompute()
                        self.valid(rib)
                        # One smooth roof, two sides, and two body-contact faces.
                        # This must be true of the tool, not just a refined result.
                        self.assertEqual(len(rib.AddSubShape.Faces), 5)
                        roofs = [
                            f
                            for f in rib.AddSubShape.Faces
                            if isinstance(f.Surface, Part.SurfaceOfExtrusion)
                        ]
                        self.assertEqual(len(roofs), 1)
                        roof = roofs[0]
                        self.assertEqual(len(roof.Edges), 4)
                        curve = next(
                            e.Curve for e in roof.Edges if isinstance(e.Curve, Part.BSplineCurve)
                        )
                        self.assertEqual(curve.Continuity, continuity)
                        for point in sketch.Shape.Edges[0].discretize(41):
                            self.assertLess(Part.Vertex(point).distToShape(roof)[0], 1e-7)
                        # The cheap spline control-polygon box overestimates
                        # trimmed curves; measure the actual geometric bounds.
                        bounds = rib.AddSubShape.optimalBoundingBox(False)
                        self.assertAlmostEqual(bounds.YLength, 2, delta=1e-6)
                        self.assertAlmostEqual(bounds.XMin, 3, delta=1e-6)
                        self.assertAlmostEqual(bounds.ZMin, 0, delta=1e-6)
                        self.assertAlmostEqual(
                            rib.AddSubShape.common(base.Shape).Volume, 0, delta=1e-6
                        )
                        if refine:
                            self.assertEqual(len(rib.Shape.Faces), 11)
        self.assertEqual(sketch.Geometry[0].getPoles(), poles)
        rib.Shape.check(True)
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "SmoothSplineRib.FCStd")
            self.doc.saveAs(path)
            App.closeDocument(self.doc.Name)
            self.doc = App.openDocument(path)
            self.doc.Rib.touch()
            self.doc.recompute()
            self.valid(self.doc.Rib)
            self.assertEqual(len(self.doc.Rib.AddSubShape.Faces), 5)
            self.assertEqual(len(self.doc.Rib.Shape.Faces), 11)

    def testPlanarRibPreservesSharpCorner(self):
        self.testRegularSplineC2Extension()
        rib, sketch = self.doc.Rib, self.doc.Profile
        sketch.delGeometry(0)
        points = [App.Vector(x, z, 0) for x, z in ((8, 30), (22, 22), (38, 10))]
        sketch.addGeometry([Part.LineSegment(a, b) for a, b in zip(points, points[1:])], False)
        for mode in ("Tangent", "Natural"):
            rib.Extension = mode
            self.doc.recompute()
            self.valid(rib)
            # A genuine corner still needs two roof faces and two ridge vertices.
            self.assertEqual(len(rib.AddSubShape.Faces), 6)
            corner = [
                v
                for v in rib.AddSubShape.Vertexes
                if abs(v.Point.x - 22) < 1e-7 and abs(v.Point.z - 22) < 1e-7
            ]
            self.assertEqual(len(corner), 2)

    def testAutomaticSideRibDirection(self):
        self.testSideProfileRib()
        rib = self.doc.Rib
        rib.AutoDirection = True
        rib.Type = "UpToShape"
        self.doc.recompute()
        self.valid(rib)
        self.assertLess(rib.Direction.z, 0)
        self.assertAlmostEqual(rib.Direction.y, 0, delta=1e-9)
        volume = rib.Shape.Volume
        self.doc.Body.Placement = App.Placement(
            App.Vector(15, -27, 36), App.Rotation(App.Vector(1, 2, 3), 47)
        )
        rib.touch()
        self.doc.recompute()
        self.valid(rib)
        self.assertAlmostEqual(rib.Shape.Volume, volume, delta=1e-5)

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
        rib.AutoDirection = False
        rib.Extension = "Off"
        rib.Type = "Length"
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

    def testBossRibAutomaticDirectionAndFullWidthContact(self):
        body, base, sketch, rib = self.bossRib()
        self.checkBossContact(base, rib)
        self.assertAlmostEqual(rib.Direction.x, 0, delta=1e-9)
        self.assertAlmostEqual(rib.Direction.z, -1, delta=1e-9)
        volume = rib.Shape.Volume
        for mode in ("Tangent", "Natural"):
            for width in (1, 4):
                for placement in ("Centered", "SideA", "SideB", "Two sides"):
                    with self.subTest(mode=mode, width=width, placement=placement):
                        rib.ThinThickness = width
                        rib.ThinThickness2 = 1.5
                        rib.ThinSide = placement
                        rib.Extension = "Off"
                        self.doc.recompute()
                        self.valid(rib)
                        unextended = rib.Shape.cut(base.Shape).BoundBox
                        rib.Extension = mode
                        self.doc.recompute()
                        self.checkBossContact(base, rib)
                        extended = rib.Shape.cut(base.Shape).BoundBox
                        self.assertAlmostEqual(extended.YMin, unextended.YMin, delta=1e-6)
                        self.assertAlmostEqual(extended.YMax, unextended.YMax, delta=1e-6)
        rib.Extension = "Tangent"
        rib.ThinThickness = 1
        rib.ThinSide = "Centered"
        self.doc.recompute()
        body.Placement = App.Placement(
            App.Vector(12, -34, 56), App.Rotation(App.Vector(1, 2, 3), 47)
        )
        rib.touch()
        self.doc.recompute()
        self.valid(rib)
        self.assertAlmostEqual(rib.Shape.Volume, volume, delta=1e-5)

    def testBossRibRejectsUnboundedWidth(self):
        body, base, sketch, rib = self.bossRib(width=22)
        self.assertIn("Invalid", rib.State)
        self.assertTrue(
            any(
                text in rib.getStatusString()
                for text in (
                    "not bounded by the body across the full thickness",
                    "footprint misses the next body boundary",
                )
            )
        )

    def testBossRibOffDoesNotExtend(self):
        body, base, sketch, rib = self.bossRib(mode="Off")
        self.valid(rib)
        material = rib.Shape.cut(base.Shape)
        self.assertAlmostEqual(material.BoundBox.XMin, 22, delta=1e-6)
        self.assertAlmostEqual(material.BoundBox.XMax, 36, delta=1e-6)

    def testBossRibAlreadyTouchingEndpoints(self):
        body, base, sketch, rib = self.bossRib()
        sketch.delGeometry(1)
        sketch.delGeometry(0)
        points = [App.Vector(x, z, 0) for x, z in ((20, 25.75), (30, 17), (40, 17))]
        sketch.addGeometry([Part.LineSegment(a, b) for a, b in zip(points, points[1:])], False)
        self.doc.recompute()
        self.checkBossContact(base, rib)

    def testSideRibPeriodicC2Extension(self):
        body = self.doc.addObject("PartDesign::Body", "Body")
        base = body.newObject("PartDesign::Feature", "Base")
        base.Shape = (
            Part.makeBox(24, 20, 3, App.Vector(-12, -10, -3))
            .fuse(
                [
                    Part.makeBox(4, 20, 15, App.Vector(-12, -10, 0)),
                    Part.makeBox(4, 20, 15, App.Vector(8, -10, 0)),
                ]
            )
            .removeSplitter()
        )
        sketch = body.newObject("Sketcher::SketchObject", "Profile")
        sketch.addGeometry(
            Part.ArcOfCircle(
                Part.Circle(App.Vector(), App.Vector(0, 0, 1), 10),
                math.radians(60),
                math.radians(120),
            ),
            False,
        )
        sketch.Placement = App.Placement(App.Vector(), App.Rotation(App.Vector(1, 0, 0), 90))
        rib = body.newObject(self.featureType, "Rib")
        rib.Profile = sketch
        rib.AutoDirection = False
        rib.Extension = "Off"
        rib.Type = "Length"
        rib.Type = "UpToShape"
        rib.Extension = "Natural"
        rib.ThinThickness = 2
        self.doc.recompute()
        self.valid(rib)
        added = rib.Shape.cut(base.Shape)
        self.assertAlmostEqual(added.BoundBox.XMin, -8, delta=1e-6)
        self.assertAlmostEqual(added.BoundBox.XMax, 8, delta=1e-6)
        self.assertAlmostEqual(added.BoundBox.ZMin, 0, delta=1e-6)

    def testSignedRibDraft(self):
        self.testSideProfileRib()
        rib, base = self.doc.Rib, self.doc.Base
        direction = rib.Direction
        for angle in (1, -1):
            rib.TaperAngle = angle
            self.doc.recompute()
            self.valid(rib)
            self.assertEqual(rib.Direction, direction)
            self.assertFalse(rib.Reversed)
            material = rib.Shape.cut(base.Shape)
            for height in (1, 10):
                section = Part.makeCompound(material.slice(App.Vector(0, 0, 1), height))
                expected = 2 - 2 * height * math.tan(math.radians(angle))
                self.assertAlmostEqual(section.BoundBox.YLength, expected, delta=1e-6)

    def testSignedBossRibDraft(self):
        body, base, sketch, rib = self.bossRib(width=4)
        original = rib.Shape.Volume
        direction = rib.Direction
        for angle in (0.5, -0.5):
            rib.TaperAngle = angle
            self.doc.recompute()
            self.valid(rib)
            self.assertEqual(rib.Direction, direction)
            self.assertFalse(rib.Reversed)
            self.assertGreater((original - rib.Shape.Volume) * angle, 0)
            material = rib.Shape.cut(base.Shape)
            for face in base.Shape.Faces:
                if isinstance(face.Surface, Part.Cylinder):
                    self.assertGreater(material.common(face).Area, 1)

    def testDraftPullModes(self):
        self.testSideProfileRib()
        rib, base = self.doc.Rib, self.doc.Base
        rib.TaperAngle = 1
        self.doc.recompute()
        automatic = rib.Shape.Volume
        growth = rib.Direction
        self.assertEqual(rib.DraftPullMode, "Automatic")
        for mode, vector in (
            ("Vector", App.Vector(0, 0, 1)),
            ("ProfileLocalVector", App.Vector(0, 1, 0)),
        ):
            rib.DraftPullMode = mode
            rib.DraftPullVector = vector
            self.doc.recompute()
            self.valid(rib)
            self.assertAlmostEqual(rib.Shape.Volume, automatic, delta=1e-5)
            self.assertEqual(rib.Direction, growth)
        floor = next(
            i + 1
            for i, f in enumerate(base.Shape.Faces)
            if f.normalAt(0, 0).z > 0.99 and abs(f.CenterOfMass.z) < 1e-6
        )
        rib.DraftPullMode = "ParallelToReference"
        rib.DraftPullMode = "ParallelToReference"
        rib.DraftPullDirection = (base, ["Face%d" % floor])
        self.doc.recompute()
        self.valid(rib)
        self.assertAlmostEqual(rib.Shape.Volume, automatic, delta=1e-5)
        rib.DraftPullMode = "TowardParent"
        self.doc.recompute()
        self.valid(rib)
        self.assertEqual(rib.Direction, growth)
        rib.DraftPullMode = "TowardReference"
        edge = next(
            i + 1
            for i, e in enumerate(base.Shape.Edges)
            if abs(e.CenterOfMass.z) < 1e-6 and abs(e.CenterOfMass.x - 3) < 1e-6 and e.Length > 23
        )
        rib.DraftPullDirection = (base, ["Edge%d" % edge])
        self.doc.recompute()
        self.valid(rib)
        self.assertEqual(rib.Direction, growth)
        # Local vectors follow the sketch/body; global vectors remain global.
        rib.DraftPullMode = "ProfileLocalVector"
        rib.DraftPullVector = App.Vector(0, 1, 0)
        self.doc.Body.Placement = App.Placement(
            App.Vector(15, -27, 36), App.Rotation(App.Vector(1, 2, 3), 47)
        )
        rib.touch()
        self.doc.recompute()
        self.valid(rib)
        self.assertAlmostEqual(rib.Shape.Volume, automatic, delta=1e-5)
        rib.DraftPullVector = App.Vector()
        self.doc.recompute()
        self.assertIn("Invalid", rib.State)
        self.assertIn("must not be zero", rib.getStatusString())
        rib.DraftPullVector = App.Vector(0, 1, 0)
        self.doc.recompute()
        with tempfile.TemporaryDirectory() as directory:
            name = os.path.join(directory, "rib-pull-modes.FCStd")
            self.doc.saveAs(name)
            App.closeDocument(self.doc.Name)
            self.doc = App.openDocument(name)
            self.doc.Rib.touch()
            self.doc.recompute()
            self.valid(self.doc.Rib)
            self.assertEqual(self.doc.Rib.DraftPullMode, "ProfileLocalVector")
            self.assertAlmostEqual(self.doc.Rib.Shape.Volume, automatic, delta=1e-5)

    def testIndependentDraftPull(self):
        self.testSideProfileRib()
        rib, base = self.doc.Rib, self.doc.Base
        rib.TaperAngle = 2
        floor = next(
            i + 1
            for i, face in enumerate(base.Shape.Faces)
            if face.normalAt(0, 0).z > 0.99 and abs(face.CenterOfMass.z) < 1e-6
        )
        rib.DraftPullMode = "ParallelToReference"
        rib.DraftPullDirection = (base, ["Face%d" % floor])
        direction = rib.Direction
        volumes = []
        for flip in (False, True):
            rib.FlipPullDirection = flip
            self.doc.recompute()
            self.valid(rib)
            self.assertEqual(rib.Direction, direction)
            self.assertFalse(rib.Reversed)
            volumes.append(rib.Shape.Volume)
        self.assertGreater(abs(volumes[0] - volumes[1]), 1e-3)
        self.doc.Body.Placement = App.Placement(
            App.Vector(15, -27, 36), App.Rotation(App.Vector(1, 2, 3), 47)
        )
        rib.touch()
        self.doc.recompute()
        self.valid(rib)
        self.assertAlmostEqual(rib.Shape.Volume, volumes[-1], delta=1e-5)
        with tempfile.TemporaryDirectory() as directory:
            name = os.path.join(directory, "draft-pull.FCStd")
            self.doc.saveAs(name)
            App.closeDocument(self.doc.Name)
            self.doc = App.openDocument(name)
            self.doc.Rib.touch()
            self.doc.recompute()
            self.valid(self.doc.Rib)
            self.assertTrue(self.doc.Rib.FlipPullDirection)
            self.assertEqual(self.doc.Rib.DraftPullDirection[1], ["Face%d" % floor])
            self.assertAlmostEqual(self.doc.Rib.Shape.Volume, volumes[-1], delta=1e-5)

    def testInactiveDirectionInputs(self):
        self.testSideProfileRib()
        rib = self.doc.Rib
        rib.AutoDirection = True
        rib.FillDirection = App.Vector()
        self.doc.recompute()
        self.valid(rib)
        automatic = rib.Shape.copy()
        rib.UseCustomVector = True
        self.doc.recompute()
        self.valid(rib)
        self.assertAlmostEqual(rib.Shape.Volume, automatic.Volume, delta=1e-5)
        rib.UseCustomVector = False
        rib.TaperAngle = 2
        self.doc.recompute()
        self.valid(rib)
        drafted = rib.Shape.copy()
        # A point cannot define a parallel axis, but an unused reference is harmless.
        rib.DraftPullDirection = (self.doc.Base, ["Vertex1"])
        rib.DraftPullVector = App.Vector()
        rib.DraftPullMode = "Automatic"
        self.doc.recompute()
        self.valid(rib)
        self.assertAlmostEqual(rib.Shape.Volume, drafted.Volume, delta=1e-5)
        rib.DraftPullMode = "ParallelToReference"
        self.doc.recompute()
        self.assertIn("Invalid", rib.State)
        rib.DraftPullMode = "Automatic"
        self.doc.recompute()
        self.valid(rib)

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
