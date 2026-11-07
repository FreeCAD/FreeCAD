# SPDX-License-Identifier: LGPL-2.1-or-later
"""Small Rib geometry matrix and persistent-reference regressions."""

import math
import os
import tempfile
import unittest

import FreeCAD as App
import Part
import Sketcher


class TestRib(unittest.TestCase):
    def setUp(self):
        self.doc = App.newDocument("TestRib")

    def tearDown(self):
        App.closeDocument(self.doc.Name)

    def makeRib(self, bodyType="L", profileType="Spline"):
        """The same four open profiles exercise planar and cylindrical supports."""
        v = App.Vector
        self.body = self.doc.addObject("PartDesign::Body", "Body")
        self.base = self.body.newObject("PartDesign::Feature", "Base")
        floor = Part.makeBox(55, 30, 5)
        if bodyType == "L":
            support = floor.fuse(Part.makeBox(5, 30, 40))
        else:
            support = floor.fuse(
                [
                    Part.makeCylinder(8, 40, v(0, 15, 0)),
                    Part.makeCylinder(10, 25, v(42, 15, 0)),
                ]
            )
        self.base.Shape = support.removeSplitter()
        self.profile = self.body.newObject("Sketcher::SketchObject", "Profile")
        self.profile.Placement = App.Placement(v(0, 15, 0), App.Rotation(v(1, 0, 0), 90))
        if profileType == "Line":
            geometry = [Part.LineSegment(v(8, 30, 0), v(38, 9, 0))]
        elif profileType == "Arc":
            geometry = [Part.Arc(v(8, 30, 0), v(26, 24, 0), v(38, 9, 0))]
        elif profileType == "Spline":
            spline = Part.BSplineCurve()
            spline.interpolate([v(8, 30, 0), v(18, 28, 0), v(30, 20, 0), v(38, 9, 0)])
            geometry = [spline]
        else:
            # Tangent line-arc-line, rotated so no segment is parallel to the
            # fill direction (which would have zero swept area).
            angle = math.radians(20)

            def point(x, y):
                x, y = x - 8, y - 30
                return v(
                    8 + x * math.cos(angle) - y * math.sin(angle),
                    30 + x * math.sin(angle) + y * math.cos(angle),
                    0,
                )

            geometry = [
                Part.LineSegment(point(8, 30), point(26, 30)),
                Part.Arc(
                    point(26, 30),
                    point(26 + 12 / math.sqrt(2), 18 + 12 / math.sqrt(2)),
                    point(38, 18),
                ),
                Part.LineSegment(point(38, 18), point(38, 9)),
            ]
        self.profile.addGeometry(geometry, False)
        self.doc.recompute()
        self.rib = self.body.newObject("PartDesign::Rib", "Rib")
        self.rib.Profile = self.profile
        self.rib.Thickness = 4

    def assertRib(self):
        self.doc.recompute()
        self.assertNotIn("Invalid", self.rib.State, self.rib.getStatusString())
        shape = self.rib.Shape
        self.assertTrue(shape.isValid())
        self.assertEqual(len(shape.Solids), 1)
        self.assertGreater(self.rib.AddSubShape.Volume, 0)
        self.assertTrue(self.rib.AddSubShape.isValid())
        self.assertLess(self.base.Shape.cut(shape).Volume, 1e-6)

    def checkProfiles(self, bodyType):
        for profile in ("Spline", "Arc", "Line", "LineArcLine"):
            with self.subTest(body=bodyType, profile=profile):
                self.makeRib(bodyType, profile)
                self.assertRib()
                self.rib.ExtendType = "C2"
                self.rib.DraftAngle = 3
                self.rib.FilletRadius = 0.5
                self.assertRib()

    def testLProfiles(self):
        self.checkProfiles("L")

    def testCircularBossProfiles(self):
        self.checkProfiles("Boss")

    def testInPlaneSweepDirections(self):
        """A multi-edge rib can reach either flange or the back of a C bracket."""
        v = App.Vector
        self.makeRib("L", "Line")
        self.base.Shape = self.base.Shape.fuse(
            Part.makeBox(55, 30, 5, v(0, 0, 35))
        ).removeSplitter()
        self.profile.delGeometry(0)
        self.profile.addGeometry(
            [
                Part.LineSegment(v(12, 16, 0), v(24, 19, 0)),
                Part.LineSegment(v(24, 19, 0), v(36, 25, 0)),
            ],
            False,
        )
        self.rib.ExtendType = "Off"
        directions = (v(0, 0, -1), v(0, 0, 1), v(0.2, 0, 1), v(-1, 0, 0))
        sketchPlacement = self.profile.Placement
        # Rotate the entire fixture as well: no world-axis assumption is valid.
        for rotation in (App.Rotation(), App.Rotation(v(1, 2, 3), 37)):
            placement = App.Placement(v(), rotation)
            self.base.Placement = placement
            self.profile.Placement = placement.multiply(sketchPlacement)
            for direction in directions:
                with self.subTest(rotation=rotation, direction=direction):
                    # Rib inherits the base placement; Direction stays Rib-local.
                    self.rib.Direction = direction
                    self.rib.Reversed = False
                    self.assertRib()
                    forward = self.rib.Shape.copy()
                    # Reversed and the opposite input vector must describe the
                    # same solid, not merely two independently valid solids.
                    self.rib.Direction = -direction
                    self.rib.Reversed = True
                    self.assertRib()
                    self.assertLess(forward.cut(self.rib.Shape).Volume, 1e-6)
                    self.assertLess(self.rib.Shape.cut(forward).Volume, 1e-6)

    def testDistanceExtentAllowsFreeEnd(self):
        for profile in ("Line", "Spline"):
            with self.subTest(profile=profile):
                self.makeRib("L", profile)
                self.rib.ExtentType = "Distance"
                self.rib.Distance = 10
                self.assertRib()
                farProfile = self.profile.Shape.copy()
                farProfile.translate(App.Vector(0, 0, -10))
                # The retained rib reaches its intentional free end, unlike Shape extent.
                self.assertLess(self.rib.AddSubShape.distToShape(farProfile)[0], 1e-7)

    def testProfileMustBeSketch(self):
        self.makeRib("L", "Line")
        other = self.doc.addObject("Part::Feature", "NonSketchProfile")
        other.Shape = self.profile.Shape.copy()
        for subnames in ([""], ["Edge1"]):
            with self.subTest(subnames=subnames):
                self.rib.Profile = (other, subnames)
                self.doc.recompute()
                self.assertIn("Invalid", self.rib.State)
                self.assertIn("Rib profile must be a sketch", self.rib.getStatusString())

    def testSketchEdgeProfile(self):
        self.makeRib("L", "Line")
        self.rib.Profile = (self.profile, ["Edge1"])
        self.assertRib()

    def testSweepDirectionMagnitudeDoesNotChangeLength(self):
        self.makeRib("L", "Line")
        self.rib.Direction = App.Vector(0, 0, -1)
        self.assertRib()
        expected = self.rib.Shape.copy()
        self.rib.Direction = App.Vector(0, 0, -17)
        self.assertRib()
        self.assertLess(expected.cut(self.rib.Shape).Volume, 1e-6)
        self.assertLess(self.rib.Shape.cut(expected).Volume, 1e-6)

    def testSweepVectorRejectsInvalidInputs(self):
        self.makeRib("L", "Line")
        self.rib.ExtentType = "Distance"
        for direction, distance, message in (
            (App.Vector(), 10, "Rib direction must be nonzero and finite"),
            (App.Vector(0, 1, 0), 10, "Rib direction must lie in the sketch plane"),
            (App.Vector(0, 0, -1), 0, "Rib distance must be positive and finite"),
            (App.Vector(0, 0, -1), -1, "Rib distance must be positive and finite"),
        ):
            with self.subTest(direction=direction, distance=distance):
                self.rib.Direction = direction
                self.rib.Distance = distance
                self.doc.recompute()
                self.assertIn("Invalid", self.rib.State)
                self.assertIn(message, self.rib.getStatusString())

    def mappedElements(self, shape):
        if not shape.ElementMapVersion:
            self.skipTest("Element maps are disabled in this build")
        elements = [
            f"{kind}{index + 1}"
            for kind, shapes in (
                ("Face", shape.Faces),
                ("Edge", shape.Edges),
                ("Vertex", shape.Vertexes),
            )
            for index in range(len(shapes))
        ]
        # Every selectable element must have an identity which resolves back
        # to that element. Validity and element-map size alone do not prove this.
        result = {}
        for element in elements:
            name = shape.getElementMappedName(element)
            self.assertTrue(name, element)
            self.assertEqual(shape.getElementIndexedName(name), element)
            result[element] = name
        return result

    def testNamingAcrossDraftAndThickness(self):
        self.makeRib()
        self.assertRib()
        names = self.mappedElements(self.rib.Shape)
        normals = {
            names[f"Face{i + 1}"]: face.normalAt(0, 0)
            for i, face in enumerate(self.rib.Shape.Faces)
        }
        for angle, thickness, continuity in (
            (3, 4, "C1"),
            (4, 5, "C2"),
            (-2, 5, "C2"),
            (0, 4, "C1"),
        ):
            with self.subTest(angle=angle, thickness=thickness, continuity=continuity):
                self.rib.DraftAngle = angle
                self.rib.Thickness = thickness
                self.rib.ExtendType = continuity
                self.assertRib()
                self.mappedElements(self.rib.Shape)
                self.mappedElements(self.rib.AddSubShape)
                for oldElement, name in names.items():
                    self.assertTrue(
                        self.rib.Shape.getElementIndexedName(name),
                        f"Lost {oldElement} at draft {angle}: {name}",
                    )
                # Resolution must not silently swap the two broad sides.
                for name, normal in normals.items():
                    self.assertGreater(
                        self.rib.Shape.getElement(name).normalAt(0, 0).dot(normal), 0.9
                    )

    def testReferencesAcrossDraftToggle(self):
        """A lost face link must not silently become a link to the whole rib."""
        for profile in ("Line", "Spline"):
            for initialAngle in (0, 3):
                with self.subTest(profile=profile, initialAngle=initialAngle):
                    self.makeRib("L", profile)
                    self.rib.DraftAngle = initialAngle
                    self.assertRib()
                    names = self.mappedElements(self.rib.Shape)
                    references = []
                    for index, face in enumerate(self.rib.Shape.Faces, 1):
                        if abs(face.normalAt(0, 0).y) < 0.9:
                            continue
                        if face.common(self.base.Shape).Area > 1e-6:
                            continue
                        element = f"Face{index}"
                        reference = self.doc.addObject(
                            "PartDesign::SubShapeBinder", "DraftReference"
                        )
                        reference.Support = [(self.rib, [element])]
                        references.append((reference.Name, names[element], face.normalAt(0, 0)))
                    self.assertEqual(len(references), 2)
                    self.doc.recompute()

                    # Restore before toggling: identities must not depend on an
                    # in-memory cache of the previous draft operation.
                    ribName, baseName = self.rib.Name, self.base.Name
                    with tempfile.TemporaryDirectory() as directory:
                        path = os.path.join(directory, "DraftReferences.FCStd")
                        self.doc.saveAs(path)
                        App.closeDocument(self.doc.Name)
                        self.doc = App.openDocument(path)
                        self.rib = self.doc.getObject(ribName)
                        self.base = self.doc.getObject(baseName)
                        for angle, thickness, continuity in (
                            (3, 4, "C1"),
                            (0, 4, "C1"),
                            (0, 4, "C2"),
                            (-2, 5, "C2"),
                            (3, 5, "C2"),
                            (0, 4, "C1"),
                        ):
                            self.rib.DraftAngle = angle
                            self.rib.Thickness = thickness
                            self.rib.ExtendType = continuity
                            self.assertRib()
                            for referenceName, name, originalNormal in references:
                                reference = self.doc.getObject(referenceName)
                                self.assertNotIn("Invalid", reference.State)
                                self.assertEqual(len(reference.Shape.Faces), 1)
                                self.assertTrue(reference.Support[0][1][0])
                                expected = self.rib.Shape.getElement(name)
                                actual = reference.Shape.Faces[0]
                                self.assertAlmostEqual(actual.Area, expected.Area, places=6)
                                self.assertLess(
                                    (actual.CenterOfMass - expected.CenterOfMass).Length, 1e-6
                                )
                                self.assertGreater(
                                    actual.normalAt(0, 0).dot(expected.normalAt(0, 0)), 0.99
                                )
                                self.assertGreater(actual.normalAt(0, 0).dot(originalNormal), 0.99)

    def testNamingDownstreamReferenceAndRestore(self):
        self.makeRib("Boss", "Spline")
        self.rib.DraftAngle = 3
        self.rib.FilletRadius = 0.5
        self.assertRib()
        names = self.mappedElements(self.rib.Shape)
        # Pick a generated broad side geometrically, rather than assuming FaceN.
        candidates = [
            (face.Area, i + 1)
            for i, face in enumerate(self.rib.Shape.Faces)
            if abs(face.normalAt(0, 0).y) > 0.9 and face.common(self.base.Shape).Area < 1e-6
        ]
        index = max(candidates)[1]
        mapped = names[f"Face{index}"]
        reference = self.doc.addObject("PartDesign::SubShapeBinder", "Reference")
        reference.Support = [(self.rib, [f"Face{index}"])]
        self.doc.recompute()
        for thickness in (5, 4):
            self.rib.Thickness = thickness
            self.rib.FilletRadius += 0.1
            self.profile.Placement.Base.z += 0.2
            self.assertRib()
            self.assertNotIn("Invalid", reference.State)
            face = self.rib.Shape.getElement(mapped)
            self.assertAlmostEqual(reference.Shape.Area, face.Area, places=6)
            self.assertEqual(len(reference.Shape.Faces), 1)
            self.assertLess(
                (reference.Shape.Faces[0].CenterOfMass - face.CenterOfMass).Length, 1e-6
            )
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "Rib.FCStd")
            self.doc.saveAs(path)
            App.closeDocument(self.doc.Name)
            self.doc = App.openDocument(path)
            self.rib, self.base = self.doc.Rib, self.doc.Base
            self.rib.Thickness = 4.5
            self.assertRib()
            face = self.rib.Shape.getElement(mapped)
            self.assertAlmostEqual(self.doc.Reference.Shape.Area, face.Area, places=6)
            self.assertEqual(len(self.doc.Reference.Shape.Faces), 1)
            self.assertLess(
                (self.doc.Reference.Shape.Faces[0].CenterOfMass - face.CenterOfMass).Length, 1e-6
            )
