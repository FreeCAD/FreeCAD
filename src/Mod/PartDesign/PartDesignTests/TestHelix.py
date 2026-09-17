# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
# *   Copyright (c) 2023 <bgbsww@gmail.com>                                 *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

from math import pi, radians, tan
from pathlib import Path
import tempfile
import unittest

import FreeCAD
from FreeCAD import Base
import Part
import Sketcher
import TestSketcherApp

""" Test various helixes """


class TestHelix(unittest.TestCase):
    """Test various helixes"""

    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestHelix")

    def createRectangleHelix(self, bodyName):
        body = self.Doc.addObject("PartDesign::Body", bodyName)
        sketch = self.Doc.addObject("Sketcher::SketchObject", bodyName + "Sketch")
        body.addObject(sketch)
        TestSketcherApp.CreateRectangleSketch(sketch, (0, 0), (5, 5))
        self.Doc.recompute()

        helix = self.Doc.addObject("PartDesign::AdditiveHelix", bodyName + "Helix")
        body.addObject(helix)
        helix.Profile = sketch
        helix.ReferenceAxis = (sketch, "V_Axis")
        helix.Placement = FreeCAD.Placement(
            FreeCAD.Vector(0, 0, 0),
            FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 0),
            FreeCAD.Vector(0, 0, 0),
        )
        helix.Pitch = 50
        helix.Height = 150
        helix.Turns = 3
        helix.Angle = 0
        helix.Mode = 0
        return helix

    def assertBoundBoxesAlmostEqual(self, first, second):
        for attr in ("XMin", "XMax", "YMin", "YMax", "ZMin", "ZMax"):
            self.assertAlmostEqual(getattr(first, attr), getattr(second, attr), places=6)

    def testHelicalTubeCase(self):
        body = self.Doc.addObject("PartDesign::Body", "Body")
        sketch = body.newObject("Sketcher::SketchObject", "Sketch")
        sketch.AttachmentSupport = (self.Doc.getObject("XY_Plane"), [""])
        sketch.MapMode = "FlatFace"

        geoList = []
        geoList.append(Part.Circle(Base.Vector(-40.0, 0.0, 0.0), Base.Vector(0.0, 0.0, 1.0), 7.5))
        geoList.append(Part.Circle(Base.Vector(-40.0, 0.0, 0.0), Base.Vector(0.0, 0.0, 1.0), 10.0))
        sketch.addGeometry(geoList, False)
        del geoList

        sketch.addConstraint(Sketcher.Constraint("PointOnObject", 0, 3, -1))
        sketch.addConstraint(Sketcher.Constraint("Coincident", 1, 3, 0, 3))
        self.Doc.recompute()

        helix = body.newObject("PartDesign::AdditiveHelix", "AdditiveHelix")
        helix.Profile = sketch
        helix.ReferenceAxis = (sketch, ["V_Axis"])
        helix.Mode = 0
        helix.Pitch = 35
        helix.Height = 100
        helix.Turns = 3
        helix.Angle = 0
        helix.Growth = 0
        helix.LeftHanded = 0
        helix.Reversed = 0

        self.Doc.recompute()
        self.assertEqual(len(helix.Shape.Solids), 1)

    def testNegativeHeightActsAsReverse(self):
        """Test negative height produces the same shape as reversed"""
        negative = self.createRectangleHelix("NegativeHeightBody")
        negative.Height = -150
        negative.Reversed = False

        reversedHelix = self.createRectangleHelix("ReversedHeightBody")
        reversedHelix.Height = 150
        reversedHelix.Reversed = True

        self.Doc.recompute()
        self.assertAlmostEqual(negative.Shape.Volume, reversedHelix.Shape.Volume, places=6)
        self.assertBoundBoxesAlmostEqual(negative.Shape.BoundBox, reversedHelix.Shape.BoundBox)

    def testTwoSidedPitchHeightAngle(self):
        """Test two-sided helix in pitch-height-angle mode"""
        helix = self.createRectangleHelix("TwoSidedPitchHeightBody")
        helix.SideType = "Two sides"
        helix.Mode = 0
        helix.Pitch = 50
        helix.Height = 150
        helix.Height2 = 100
        self.Doc.recompute()

        self.assertAlmostEqual(helix.Turns, 3)
        self.assertAlmostEqual(helix.Turns2, 2)
        self.assertLess(helix.Shape.BoundBox.YMin, 0)
        self.assertGreater(helix.Shape.BoundBox.YMax, 0)
        # Helical sweeps are approximated; allow 0.01 mm^3 across OCCT versions.
        self.assertAlmostEqual(helix.Shape.Volume, pi * 25 * 5 * 5, delta=0.01)

    def testTwoSidedPitchTurnsAngle(self):
        """Test two-sided helix in pitch-turns-angle mode"""
        helix = self.createRectangleHelix("TwoSidedPitchTurnsBody")
        helix.SideType = "Two sides"
        helix.Mode = 1
        helix.Pitch = 50
        helix.Turns = 3
        helix.Turns2 = 2
        self.Doc.recompute()

        self.assertAlmostEqual(helix.Height, 150)
        self.assertAlmostEqual(helix.Height2, 100)
        self.assertLess(helix.Shape.BoundBox.YMin, 0)
        self.assertGreater(helix.Shape.BoundBox.YMax, 0)
        self.assertAlmostEqual(helix.Shape.Volume, pi * 25 * 5 * 5, delta=0.01)

    def testTwoSidedHeightTurnsAngleUsesTotalTurns(self):
        """Test two-sided height-turns-angle uses turns across both sides"""
        helix = self.createRectangleHelix("TwoSidedHeightTurnsBody")
        helix.SideType = "Two sides"
        helix.Mode = 2
        helix.Height = 150
        helix.Height2 = 100
        helix.Turns = 5
        self.Doc.recompute()

        self.assertAlmostEqual(helix.Pitch, 50)
        self.assertAlmostEqual(helix.Turns2, 2)
        self.assertAlmostEqual(helix.Shape.Volume, pi * 25 * 5 * 5, delta=0.01)

    def testTwoSidedAngleShrinksSecondSide(self):
        """Test two-sided cone angle uses opposite growth for the second side"""
        twoSided = self.createRectangleHelix("TwoSidedAngleBody")
        twoSided.SideType = "Two sides"
        twoSided.Mode = 0
        twoSided.Pitch = 50
        twoSided.Height = 100
        twoSided.Height2 = 100
        twoSided.Angle = 20

        symmetric = self.createRectangleHelix("SymmetricAngleBody")
        symmetric.SideType = "Symmetric"
        symmetric.Mode = 0
        symmetric.Pitch = 50
        symmetric.Height = 200
        symmetric.Angle = 20

        self.Doc.recompute()
        self.assertLess(twoSided.Shape.Volume, symmetric.Shape.Volume)

    def testSymmetricPitchHeightAngle(self):
        """Test symmetric helix in pitch-height-angle mode"""
        helix = self.createRectangleHelix("SymmetricPitchHeightBody")
        helix.SideType = "Symmetric"
        helix.Mode = 0
        helix.Pitch = 50
        helix.Height = 150
        self.Doc.recompute()

        self.assertLess(helix.Shape.BoundBox.YMin, 0)
        self.assertGreater(helix.Shape.BoundBox.YMax, 0)
        self.assertAlmostEqual(helix.Shape.Volume, pi * 25 * 5 * 3, places=2)

    def testFlatSpiralKeepsPerTurnFaces(self):
        """Preserve the per-turn sweep used by existing zero-angle spirals."""
        body = self.Doc.addObject("PartDesign::Body", "SpiralBody")
        sketch = body.newObject("Sketcher::SketchObject", "Sketch")
        TestSketcherApp.CreateRectangleSketch(sketch, (10, 0), (1, 1))
        sketch.AttachmentSupport = body.Origin.OriginFeatures[4]
        sketch.MapMode = "FlatFace"
        self.Doc.recompute()
        helix = body.newObject("PartDesign::AdditiveHelix", "Helix")
        helix.Profile = sketch
        helix.ReferenceAxis = (sketch, "V_Axis")
        helix.Mode = 3
        helix.Height = 0
        helix.Turns = 2.5
        helix.Growth = 2
        helix.Angle = 0
        self.Doc.recompute()

        self.assertNotIn("Invalid", helix.State)
        self.assertTrue(helix.Shape.isValid())
        self.assertEqual(len(helix.Shape.Solids), 1)
        # Three path segments sweep four profile edges each, plus the two end faces.
        # Removing the breaks changes existing faces and their downstream references.
        self.assertEqual(len(helix.AddSubShape.Faces), 14)

    def checkTwoSidedHelixAtScale(self, operation):
        """Extend AIRCAP's scale regressions to both directions and signed-height trimming."""
        occVersion = tuple(int(v) for v in Part.OCC_VERSION.split(".")[:2])
        # Retain the smaller boolean-test scale used below for OCCT 7.3 and older.
        largestExponent = 6 if occVersion > (7, 3) else 5
        for sideType, height2 in (("Two sides", 3), ("Symmetric", 3), ("Two sides", -1)):
            for exponent in (-1, 0, 3, largestExponent):
                with self.subTest(
                    operation=operation, sideType=sideType, height2=height2, exponent=exponent
                ):
                    scale = 10**exponent
                    body = self.Doc.addObject("PartDesign::Body", "ScaledBody")
                    sketch = body.newObject("Sketcher::SketchObject", "Sketch")
                    TestSketcherApp.CreateRectangleSketch(sketch, (10 * scale, 0), (scale, scale))
                    sketch.AttachmentSupport = body.Origin.OriginFeatures[4]
                    sketch.MapMode = "FlatFace"
                    if operation != "standalone":
                        cylinder = body.newObject("PartDesign::AdditiveCylinder", "Cylinder")
                        cylinder.Radius = (10 if operation == "add" else 11) * scale
                        cylinder.Height = 20 * scale
                        cylinder.Placement.Base.z = -10 * scale
                    self.Doc.recompute()

                    featureType = (
                        "PartDesign::SubtractiveHelix"
                        if operation == "cut"
                        else "PartDesign::AdditiveHelix"
                    )
                    helix = body.newObject(featureType, "Helix")
                    helix.Profile = sketch
                    helix.ReferenceAxis = (sketch, "V_Axis")
                    helix.SideType = sideType
                    helix.Pitch = 2 * scale
                    helix.Height = 5 * scale
                    helix.Height2 = height2 * scale
                    helix.Angle = 0
                    helix.Mode = 0
                    helix.Tolerance = 0.12 if operation == "add" else 0.1
                    self.Doc.recompute()

                    self.assertNotIn("Invalid", helix.State)
                    self.assertTrue(helix.Shape.isValid())
                    self.assertEqual(len(helix.Shape.Solids), 1)
                    # Annular area times profile height times the total turn count.
                    totalTurns = (5 + height2) / 2 if sideType == "Two sides" else 2.5
                    expected = pi * (11**2 - 10**2) * totalTurns
                    if operation == "add":
                        expected = pi * 10**2 * 20 + expected
                    elif operation == "cut":
                        expected = pi * 11**2 * 20 - expected
                    # Normalize to unit scale and allow for the approximated sweep/boolean.
                    self.assertAlmostEqual(
                        helix.Shape.Volume / scale**3, expected, delta=expected * 1e-5
                    )
                    if operation == "standalone":
                        zMin = -height2 if sideType == "Two sides" else -2.5
                        zMax = 6 if sideType == "Two sides" else 3.5
                    else:
                        zMin, zMax = -10, 10
                    self.assertAlmostEqual(helix.Shape.BoundBox.ZMin / scale, zMin, places=5)
                    self.assertAlmostEqual(helix.Shape.BoundBox.ZMax / scale, zMax, places=5)

    def testTwoSidedHelixAtScale(self):
        self.checkTwoSidedHelixAtScale("standalone")

    def testTwoSidedHelixAdditiveAtScale(self):
        self.checkTwoSidedHelixAtScale("add")

    def testTwoSidedHelixSubtractiveAtScale(self):
        self.checkTwoSidedHelixAtScale("cut")

    def testTwoSidedOverlappingHeights(self):
        """Opposite-signed heights bound the remaining segment, e.g. 50 - 10 = 40."""
        for mode in (0, 2, 3):
            for height, height2 in (
                (50, -10),
                (10, -50),
                (-50, 10),
                (-10, 50),
                (150, -100),
                (-150, 100),
            ):
                for reversed in (False, True):
                    for leftHanded in (False, True):
                        with self.subTest(
                            mode=mode,
                            height=height,
                            height2=height2,
                            reversed=reversed,
                            leftHanded=leftHanded,
                        ):
                            helix = self.createRectangleHelix("OverlappingBody")
                            helix.SideType = "Two sides"
                            helix.Mode = mode
                            helix.Height = height
                            helix.Height2 = height2
                            remainingTurns = abs(height + height2) / 50
                            helix.Turns = remainingTurns
                            helix.Growth = 0
                            helix.Reversed = reversed
                            helix.LeftHanded = leftHanded
                            self.Doc.recompute()

                            self.assertNotIn("Invalid", helix.State)
                            self.assertEqual(len(helix.Shape.Solids), 1)
                            self.assertAlmostEqual(helix.Pitch.Value, 50)
                            self.assertAlmostEqual(
                                helix.Shape.Volume, pi * 25 * 5 * remainingTurns, delta=0.01
                            )
                            direction = -1 if reversed else 1
                            endpoints = (direction * height, -direction * height2)
                            self.assertAlmostEqual(
                                helix.Shape.BoundBox.YMin, min(endpoints), places=5
                            )
                            # The profile adds 5 mm to the upper bound of the path.
                            self.assertAlmostEqual(
                                helix.Shape.BoundBox.YMax, max(endpoints) + 5, places=5
                            )

    def testTwoSidedOverlappingHeightsWithTaper(self):
        """Trim along the same cone or spiral instead of leaving a different taper."""
        for mode in (0, 2, 3):
            for taperSign in (-1, 1):
                for direction in (-1, 1):
                    with self.subTest(mode=mode, taperSign=taperSign, direction=direction):
                        body = self.Doc.addObject("PartDesign::Body", "TaperedBody")
                        sketch = body.newObject("Sketcher::SketchObject", "Sketch")
                        TestSketcherApp.CreateRectangleSketch(sketch, (10, 0), (1, 1))
                        sketch.AttachmentSupport = body.Origin.OriginFeatures[4]
                        sketch.MapMode = "FlatFace"
                        self.Doc.recompute()
                        helix = body.newObject("PartDesign::AdditiveHelix", "Helix")
                        helix.Profile = sketch
                        helix.ReferenceAxis = (sketch, "V_Axis")
                        helix.SideType = "Two sides"
                        helix.Mode = mode
                        helix.Pitch = 2
                        helix.Height = direction * 5
                        helix.Height2 = -direction
                        helix.Turns = 2
                        helix.Angle = taperSign * 15
                        helix.Growth = taperSign * 0.2
                        self.Doc.recompute()

                        self.assertNotIn("Invalid", helix.State)
                        self.assertTrue(helix.Shape.isValid())
                        self.assertEqual(len(helix.Shape.Solids), 1)
                        self.assertAlmostEqual(helix.Pitch.Value, 2)
                        growth = taperSign * (0.2 if mode == 3 else 2 * tan(radians(15)))
                        # Integrate the annular area between turn 0.5 and turn 2.5.
                        expected = pi * ((11**2 - 10**2) * 2 + growth * (2.5**2 - 0.5**2))
                        self.assertAlmostEqual(helix.Shape.Volume, expected, delta=0.01)
                        self.assertAlmostEqual(
                            helix.Shape.BoundBox.ZMin, 1 if direction == 1 else -5, places=5
                        )
                        self.assertAlmostEqual(
                            helix.Shape.BoundBox.ZMax, 6 if direction == 1 else 0, places=5
                        )

    def testTwoSidedHeightsCancel(self):
        """Equal opposite-signed heights must not leave the original full helix."""
        for mode in (0, 2, 3):
            with self.subTest(mode=mode):
                helix = self.createRectangleHelix("CancelledBody")
                helix.SideType = "Two sides"
                helix.Mode = mode
                helix.Height = 50
                helix.Height2 = -50
                self.Doc.recompute()
                self.assertIn("Invalid", helix.State)

    def testTwoSidedSaveRestore(self):
        """Retain direction, signed heights and recomputation after reopening."""
        helix = self.createRectangleHelix("SavedBody")
        helix.SideType = "Two sides"
        helix.Height = -150
        helix.Height2 = -100
        helix.Reversed = True
        self.Doc.recompute()
        self.assertNotIn("Invalid", helix.State)
        expectedVolume = helix.Shape.Volume
        helixName = helix.Name
        with tempfile.TemporaryDirectory() as directory:
            filename = str(Path(directory) / "TwoSidedHelix.FCStd")
            self.Doc.saveAs(filename)
            FreeCAD.closeDocument(self.Doc.Name)
            self.Doc = FreeCAD.openDocument(filename)
            restored = self.Doc.getObject(helixName)
            self.assertEqual(restored.SideType, "Two sides")
            self.assertEqual(restored.Height.Value, -150)
            self.assertEqual(restored.Height2.Value, -100)
            self.assertTrue(restored.Reversed)
            restored.touch()
            self.Doc.recompute()
            self.assertNotIn("Invalid", restored.State)
            self.assertEqual(len(restored.Shape.Solids), 1)
            self.assertAlmostEqual(restored.Shape.Volume, expectedVolume, delta=0.01)

    def testCircleQ1(self):
        """Test helix based on circle in Quadrant 1"""
        body = self.Doc.addObject("PartDesign::Body", "Body")
        profileSketch = self.Doc.addObject("Sketcher::SketchObject", "ProfileSketch")
        body.addObject(profileSketch)
        TestSketcherApp.CreateCircleSketch(profileSketch, (2, 0), 1)
        self.Doc.recompute()
        helix = self.Doc.addObject("PartDesign::AdditiveHelix", "AdditiveHelix")
        body.addObject(helix)
        helix.Profile = profileSketch
        helix.ReferenceAxis = (profileSketch, "V_Axis")
        helix.Placement = FreeCAD.Placement(
            FreeCAD.Vector(0, 0, 0),
            FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 0),
            FreeCAD.Vector(0, 0, 0),
        )
        helix.Pitch = 3
        helix.Height = 9
        helix.Turns = 2
        helix.Angle = 0
        helix.Mode = 1
        self.Doc.recompute()
        self.assertAlmostEqual(helix.Shape.Volume, 78.957, places=3)

        helix.Angle = 25
        self.Doc.recompute()
        self.assertAlmostEqual(helix.Shape.Volume, 134.17, places=2)

        profileSketch.addGeometry(
            Part.Circle(FreeCAD.Vector(2, 0, 0), FreeCAD.Vector(0, 0, 1), 0.5)
        )
        self.Doc.recompute()
        self.assertAlmostEqual(helix.Shape.Volume, 100.63, places=2)

    def testRectangle(self):
        """Test helix based on a rectangle"""
        body = self.Doc.addObject("PartDesign::Body", "GearBody")
        gearSketch = self.Doc.addObject("Sketcher::SketchObject", "GearSketch")
        body.addObject(gearSketch)
        TestSketcherApp.CreateRectangleSketch(gearSketch, (0, 0), (5, 5))
        self.Doc.recompute()

        # xz_plane = body.Origin.OriginFeatures[4]
        # coneSketch.AttachmentSupport = xz_plane
        # coneSketch.MapMode = 'FlatFace'
        helix = self.Doc.addObject("PartDesign::AdditiveHelix", "AdditiveHelix")
        body.addObject(helix)
        helix.Profile = gearSketch
        helix.ReferenceAxis = (gearSketch, "V_Axis")
        helix.Placement = FreeCAD.Placement(
            FreeCAD.Vector(0, 0, 0),
            FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 0),
            FreeCAD.Vector(0, 0, 0),
        )

        helix.Pitch = 50
        helix.Height = 150
        helix.Turns = 3
        helix.Angle = 0
        helix.Mode = 0
        self.Doc.recompute()
        bbox = helix.Shape.BoundBox
        self.assertAlmostEqual(bbox.YMin, 0)
        # Computed exact value
        # with r = radius, l = length of square, t = turns
        # pi * r**2 * l * t
        expected = pi * 25 * 5 * 3
        self.assertAlmostEqual(helix.Shape.Volume, expected, places=2)

    def testGiantHelix(self):
        """Test giant helix"""
        _OCC_VERSION = [int(v) for v in Part.OCC_VERSION.split(".") if v.isnumeric()]
        if _OCC_VERSION[0] > 7 or (_OCC_VERSION[0] == 7 and _OCC_VERSION[1] > 3):
            mine = -1
            maxe = 10
        else:
            mine = -1
            maxe = 9
        for iexponent in range(mine, maxe):
            exponent = float(iexponent)
            body = self.Doc.addObject("PartDesign::Body", "GearBody")
            gearSketch = self.Doc.addObject("Sketcher::SketchObject", "GearSketch")
            body.addObject(gearSketch)
            TestSketcherApp.CreateRectangleSketch(
                gearSketch, (10 * (10**exponent), 0), (1 * (10**exponent), 1 * (10**exponent))
            )
            xz_plane = body.Origin.OriginFeatures[4]
            gearSketch.AttachmentSupport = xz_plane
            gearSketch.MapMode = "FlatFace"
            self.Doc.recompute()

            helix = self.Doc.addObject("PartDesign::AdditiveHelix", "AdditiveHelix")
            body.addObject(helix)
            helix.Profile = gearSketch
            helix.ReferenceAxis = (gearSketch, "V_Axis")
            helix.Placement = FreeCAD.Placement(
                FreeCAD.Vector(0, 0, 0),
                FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 0),
                FreeCAD.Vector(0, 0, 0),
            )

            helix.Pitch = 2 * (10**exponent)
            helix.Turns = 2
            helix.Height = helix.Turns * helix.Pitch
            helix.Angle = 0
            helix.Mode = 0
            self.Doc.recompute()

            self.assertTrue(helix.Shape.isValid())
            bbox = helix.Shape.BoundBox
            self.assertAlmostEqual(bbox.ZMin / ((10**exponent) ** 3), 0, places=4)
            # Computed exact value
            # with r = radius, l = length of square, t = turns
            # pi * r**2 * l * t
            expected = (
                pi
                * (((11 * (10**exponent)) ** 2) - ((10 * (10**exponent)) ** 2))
                * 1
                * (10**exponent)
                * helix.Turns
            )
            self.assertAlmostEqual(
                helix.Shape.Volume / ((10**exponent) ** 3),
                expected / ((10**exponent) ** 3),
                places=2,
            )

    def testGiantHelixAdditive(self):
        """Test giant helix added to Cylinder"""
        _OCC_VERSION = [int(v) for v in Part.OCC_VERSION.split(".") if v.isnumeric()]
        if _OCC_VERSION[0] > 7 or (_OCC_VERSION[0] == 7 and _OCC_VERSION[1] > 3):
            mine = -1
            maxe = 8
        else:
            mine = -1
            maxe = 6
        for iexponent in range(mine, maxe):
            exponent = float(iexponent)
            body = self.Doc.addObject("PartDesign::Body", "GearBody")
            gearSketch = self.Doc.addObject("Sketcher::SketchObject", "GearSketch")
            body.addObject(gearSketch)
            TestSketcherApp.CreateRectangleSketch(
                gearSketch, (10 * (10**exponent), 0), (1 * (10**exponent), 1 * (10**exponent))
            )
            xz_plane = body.Origin.OriginFeatures[4]
            gearSketch.AttachmentSupport = xz_plane
            gearSketch.MapMode = "FlatFace"
            self.Doc.recompute()

            cylinder = self.Doc.addObject("PartDesign::AdditiveCylinder", "Cylinder")
            cylinder.Radius = 10 * (10**exponent)
            cylinder.Height = 8 * (10**exponent)
            cylinder.Angle = 360
            body.addObject(cylinder)
            self.Doc.recompute()

            helix = self.Doc.addObject("PartDesign::AdditiveHelix", "AdditiveHelix")
            body.addObject(helix)
            helix.Profile = gearSketch
            helix.ReferenceAxis = (gearSketch, "V_Axis")
            helix.Placement = FreeCAD.Placement(
                FreeCAD.Vector(0, 0, 0),
                FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 0),
                FreeCAD.Vector(0, 0, 0),
            )

            helix.Pitch = 2 * (10**exponent)
            helix.Turns = 2.5  # workaround for OCCT bug with very large helices - full turns truncate the cylinder due to seam alignment
            helix.Height = helix.Turns * helix.Pitch
            helix.Angle = 0
            helix.Mode = 0
            helix.Tolerance = 0.12
            self.Doc.recompute()

            self.assertTrue(helix.Shape.isValid())
            bbox = helix.Shape.BoundBox
            self.assertAlmostEqual(bbox.ZMin / ((10**exponent) ** 3), 0, places=4)
            # Computed exact value
            # with r = radius, l = length of square, t = turns
            # pi * r**2 * l * t
            cyl = pi * ((10 * (10**exponent)) ** 2) * 8 * (10**exponent)
            expected = cyl + (
                pi
                * (((11 * (10**exponent)) ** 2) - ((10 * (10**exponent)) ** 2))
                * 1
                * (10**exponent)
                * helix.Turns
            )
            self.assertAlmostEqual(
                helix.Shape.Volume / ((10**exponent) ** 3),
                expected / ((10**exponent) ** 3),
                places=2,
            )

    def testGiantHelixSubtractive(self):
        """Test giant helix subtracted from Cylinder"""
        _OCC_VERSION = [int(v) for v in Part.OCC_VERSION.split(".") if v.isnumeric()]
        if _OCC_VERSION[0] > 7 or (_OCC_VERSION[0] == 7 and _OCC_VERSION[1] > 3):
            mine = -1
            maxe = 8
        else:
            mine = -1
            maxe = 6
        for iexponent in range(mine, maxe):
            exponent = float(iexponent)
            body = self.Doc.addObject("PartDesign::Body", "GearBody")
            gearSketch = self.Doc.addObject("Sketcher::SketchObject", "GearSketch")
            body.addObject(gearSketch)
            TestSketcherApp.CreateRectangleSketch(
                gearSketch, (10 * (10**exponent), 0), (1 * (10**exponent), 1 * (10**exponent))
            )
            xz_plane = body.Origin.OriginFeatures[4]
            gearSketch.AttachmentSupport = xz_plane
            gearSketch.MapMode = "FlatFace"
            self.Doc.recompute()

            cylinder = self.Doc.addObject("PartDesign::AdditiveCylinder", "Cylinder")
            cylinder.Radius = 11 * (10**exponent)
            cylinder.Height = 8 * (10**exponent)
            cylinder.Angle = 360
            body.addObject(cylinder)
            self.Doc.recompute()

            helix = self.Doc.addObject("PartDesign::SubtractiveHelix", "SubtractiveHelix")
            body.addObject(helix)
            helix.Profile = gearSketch
            helix.ReferenceAxis = (gearSketch, "V_Axis")
            helix.Placement = FreeCAD.Placement(
                FreeCAD.Vector(0, 0, 0),
                FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 0),
                FreeCAD.Vector(0, 0, 0),
            )

            helix.Pitch = 2 * (10**exponent)
            helix.Turns = 2.5  # workaround for OCCT bug with very large helices - full turns truncate the cylinder due to seam alignment
            helix.Height = helix.Turns * helix.Pitch
            helix.Angle = 0
            helix.Mode = 0
            self.Doc.recompute()

            self.assertTrue(helix.Shape.isValid())
            bbox = helix.Shape.BoundBox
            self.assertAlmostEqual(bbox.ZMin / ((10**exponent) ** 3), 0, places=4)
            # Computed exact value
            # with r = radius, l = length of square, t = turns
            # pi * r**2 * l * t
            cyl = pi * ((11 * (10**exponent)) ** 2) * 8 * (10**exponent)
            expected = cyl - (
                pi
                * (((11 * (10**exponent)) ** 2) - ((10 * (10**exponent)) ** 2))
                * 1
                * (10**exponent)
                * helix.Turns
            )
            self.assertAlmostEqual(
                helix.Shape.Volume / ((10**exponent) ** 3),
                expected / ((10**exponent) ** 3),
                places=2,
            )

    def testCone(self):
        """Test helix following a cone"""
        body = self.Doc.addObject("PartDesign::Body", "ConeBody")
        coneSketch = self.Doc.addObject("Sketcher::SketchObject", "ConeSketch")
        body.addObject(coneSketch)

        geoList = []
        geoList.append(Part.LineSegment(FreeCAD.Vector(-5, -5, 0), FreeCAD.Vector(-3, 0, 0)))
        geoList.append(Part.LineSegment(FreeCAD.Vector(-3, 0, 0), FreeCAD.Vector(-2, 0, 0)))
        geoList.append(Part.LineSegment(FreeCAD.Vector(-2, 0, 0), FreeCAD.Vector(-4, -5, 0)))
        geoList.append(Part.LineSegment(FreeCAD.Vector(-4, -5, 0), FreeCAD.Vector(-5, -5, 0)))
        l1, l2, l3, l4 = coneSketch.addGeometry(geoList)

        conList = []
        conList.append(Sketcher.Constraint("Coincident", 0, 2, 1, 1))
        conList.append(Sketcher.Constraint("Coincident", 1, 2, 2, 1))
        conList.append(Sketcher.Constraint("Coincident", 2, 2, 3, 1))
        conList.append(Sketcher.Constraint("Coincident", 3, 2, 0, 1))
        conList.append(Sketcher.Constraint("Horizontal", 1))
        conList.append(
            Sketcher.Constraint("Angle", l3, 1, -2, 2, FreeCAD.Units.Quantity("30.000000 deg"))
        )
        conList.append(Sketcher.Constraint("DistanceX", 1, 2, -5))
        conList.append(Sketcher.Constraint("DistanceY", 1, 2, 0))
        conList.append(Sketcher.Constraint("Equal", 0, 2))
        conList.append(Sketcher.Constraint("Equal", 1, 3))
        conList.append(Sketcher.Constraint("DistanceY", 0, 50))
        conList.append(Sketcher.Constraint("DistanceX", 1, 10))
        coneSketch.addConstraint(conList)

        xz_plane = body.Origin.OriginFeatures[4]
        coneSketch.AttachmentSupport = xz_plane
        coneSketch.MapMode = "FlatFace"
        helix = self.Doc.addObject("PartDesign::AdditiveHelix", "AdditiveHelix")
        body.addObject(helix)
        helix.Profile = coneSketch
        helix.ReferenceAxis = (coneSketch, "V_Axis")
        helix.Placement = FreeCAD.Placement(
            FreeCAD.Vector(0, 0, 0),
            FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 0),
            FreeCAD.Vector(0, 0, 0),
        )

        helix.Pitch = 50
        helix.Height = 110
        helix.Turns = 2.2
        helix.Angle = 30
        helix.Mode = 0
        helix.Reversed = True
        self.Doc.recompute()
        self.assertAlmostEqual(helix.Shape.Volume / 1e5, 3.8828, places=4)

    def testNegativeCone(self):
        """Test helix following a cone with a negative angle"""
        body = self.Doc.addObject("PartDesign::Body", "ConeBody")
        coneSketch = self.Doc.addObject("Sketcher::SketchObject", "ConeSketch")
        body.addObject(coneSketch)

        geoList = []
        geoList.append(Part.LineSegment(FreeCAD.Vector(5, 5, 0), FreeCAD.Vector(3, 0, 0)))
        geoList.append(Part.LineSegment(FreeCAD.Vector(3, 0, 0), FreeCAD.Vector(2, 0, 0)))
        geoList.append(Part.LineSegment(FreeCAD.Vector(2, 0, 0), FreeCAD.Vector(4, 5, 0)))
        geoList.append(Part.LineSegment(FreeCAD.Vector(4, 5, 0), FreeCAD.Vector(5, 5, 0)))
        l1, l2, l3, l4 = coneSketch.addGeometry(geoList)

        conList = []
        conList.append(Sketcher.Constraint("Coincident", 0, 2, 1, 1))
        conList.append(Sketcher.Constraint("Coincident", 1, 2, 2, 1))
        conList.append(Sketcher.Constraint("Coincident", 2, 2, 3, 1))
        conList.append(Sketcher.Constraint("Coincident", 3, 2, 0, 1))
        conList.append(Sketcher.Constraint("Horizontal", 1))
        conList.append(Sketcher.Constraint("Angle", l3, 1, -2, 2, FreeCAD.Units.Quantity("30 deg")))
        conList.append(Sketcher.Constraint("DistanceX", 1, 2, -100))
        conList.append(Sketcher.Constraint("DistanceY", 1, 2, 0))
        conList.append(Sketcher.Constraint("Equal", 0, 2))
        conList.append(Sketcher.Constraint("Equal", 1, 3))
        conList.append(Sketcher.Constraint("DistanceY", 0, 50))
        conList.append(Sketcher.Constraint("DistanceX", 1, 10))
        coneSketch.addConstraint(conList)

        xz_plane = body.Origin.OriginFeatures[4]
        coneSketch.AttachmentSupport = xz_plane
        coneSketch.MapMode = "FlatFace"
        helix = self.Doc.addObject("PartDesign::AdditiveHelix", "AdditiveHelix")
        body.addObject(helix)
        helix.Profile = coneSketch
        helix.ReferenceAxis = (coneSketch, "V_Axis")

        helix.Pitch = 50
        helix.Height = 110
        helix.Angle = -30
        helix.Mode = 0
        helix.Reversed = False
        self.Doc.recompute()
        self.assertAlmostEqual(helix.Shape.Volume / 1e5, 6.0643, places=4)

    def tearDown(self):
        FreeCAD.closeDocument(self.Doc.Name)
