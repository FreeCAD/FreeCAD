# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import os
import tempfile
import unittest
import xml.etree.ElementTree as ET
import zipfile

import FreeCAD
import TestSketcherApp


class TestLinearPattern(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestLinearPattern")

    def makeSuppressionPattern(self):
        body = self.Doc.addObject("PartDesign::Body", "Body")
        box = body.newObject("PartDesign::AdditiveBox", "Box")
        box.Length = box.Width = box.Height = 10
        self.Doc.recompute()
        pattern = body.newObject("PartDesign::LinearPattern", "LinearPattern")
        pattern.Originals = [box]
        pattern.Direction = (self.Doc.X_Axis, [""])
        pattern.Direction2 = (self.Doc.Y_Axis, [""])
        pattern.Mode = pattern.Mode2 = "Spacing"
        pattern.Offset = pattern.Offset2 = 10
        pattern.Occurrences = pattern.Occurrences2 = 3
        pattern.Spacings = pattern.Spacings2 = [-1, -1]
        self.Doc.recompute()
        return pattern

    def testSuppressionKeepsGridPosition(self):
        pattern = self.makeSuppressionPattern()
        pattern.SuppressedIndices = [5]  # (1, 2)
        pattern.Occurrences2 = 4
        self.Doc.recompute()
        self.assertEqual(pattern.SuppressedIndices, [6])
        self.assertAlmostEqual(pattern.Shape.Volume, 11000)
        self.assertFalse(pattern.Shape.isInside(FreeCAD.Vector(15, 25, 5), 1e-7, True))
        self.assertTrue(pattern.Shape.isInside(FreeCAD.Vector(15, 15, 5), 1e-7, True))

        pattern.Occurrences = 1
        pattern.Occurrences2 = 2
        self.Doc.recompute()
        self.assertEqual(pattern.SuppressedIndices, [])
        self.assertAlmostEqual(pattern.Shape.Volume, 2000)
        pattern.Occurrences = pattern.Occurrences2 = 3
        self.Doc.recompute()
        self.assertEqual(pattern.SuppressedIndices, [5])
        self.assertAlmostEqual(pattern.Shape.Volume, 8000)

    def testCoordinateSuppressionUndoRedo(self):
        pattern = self.makeSuppressionPattern()
        self.Doc.UndoMode = 1
        self.Doc.openTransaction("Suppress instance")
        pattern.SuppressedPositions = [FreeCAD.Vector(1, 2, 0)]
        self.Doc.recompute()
        self.Doc.commitTransaction()
        self.assertAlmostEqual(pattern.Shape.Volume, 8000)
        self.Doc.undo()
        self.Doc.recompute()
        self.assertEqual(pattern.SuppressedPositions, [])
        self.assertAlmostEqual(pattern.Shape.Volume, 9000)
        self.Doc.redo()
        self.Doc.recompute()
        self.assertEqual(pattern.SuppressedIndices, [5])
        self.assertAlmostEqual(pattern.Shape.Volume, 8000)

        self.Doc.openTransaction("Resize pattern")
        pattern.Occurrences2 = 4
        self.Doc.recompute()
        self.Doc.commitTransaction()
        self.Doc.undo()
        self.Doc.recompute()
        self.assertEqual(pattern.SuppressedIndices, [5])
        self.Doc.redo()
        self.Doc.recompute()
        self.assertEqual(pattern.SuppressedIndices, [6])

    def testSuppressionInMultiTransform(self):
        pattern = self.makeSuppressionPattern()
        multi = self.Doc.Body.newObject("PartDesign::MultiTransform", "MultiTransform")
        multi.Originals = [self.Doc.Box]
        multi.Transformations = [pattern]
        pattern.SuppressedIndices = [5]
        self.Doc.recompute()
        self.assertAlmostEqual(multi.Shape.Volume, 8000)
        pattern.Occurrences2 = 4
        self.Doc.recompute()
        self.assertEqual(pattern.SuppressedIndices, [6])
        self.assertAlmostEqual(multi.Shape.Volume, 11000)
        self.assertFalse(multi.Shape.isInside(FreeCAD.Vector(15, 25, 5), 1e-7, True))

    def testSuppressionPersistenceAndMigration(self):
        pattern = self.makeSuppressionPattern()
        for legacy in (False, True):
            with self.subTest(legacy=legacy):
                pattern.Occurrences2 = 3
                pattern.SuppressedIndices = [5]
                if not legacy:
                    pattern.Occurrences2 = 1  # Save an out-of-range suppression.
                self.Doc.recompute()
                with tempfile.TemporaryDirectory(prefix="freecad_linear_pattern_") as directory:
                    path = os.path.join(directory, "pattern.FCStd")
                    self.Doc.saveAs(path)
                    FreeCAD.closeDocument(self.Doc.Name)
                    if legacy:
                        with zipfile.ZipFile(path) as archive:
                            entries = {name: archive.read(name) for name in archive.namelist()}
                        root = ET.fromstring(entries["Document.xml"])
                        for properties in root.iter("Properties"):
                            for prop in list(properties):
                                if prop.get("name") == "SuppressedPositions":
                                    properties.remove(prop)
                            properties.set("Count", str(len(properties.findall("Property"))))
                        entries["Document.xml"] = ET.tostring(root)
                        with zipfile.ZipFile(path, "w") as archive:
                            for name, data in entries.items():
                                archive.writestr(name, data)
                    self.Doc = FreeCAD.openDocument(path)
                    pattern = self.Doc.getObject("LinearPattern")
                pattern.Occurrences2 = 4
                pattern.Spacings2 = [-1, -1, -1]
                self.Doc.recompute()
                self.assertEqual(pattern.SuppressedIndices, [6])
                self.assertAlmostEqual(pattern.Shape.Volume, 11000)

    def testXAxisLinearPattern(self):
        self.Body = self.Doc.addObject("PartDesign::Body", "Body")
        self.Box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        self.Body.addObject(self.Box)
        self.Box.Length = 10.00
        self.Box.Width = 10.00
        self.Box.Height = 10.00
        self.Doc.recompute()
        self.LinearPattern = self.Doc.addObject("PartDesign::LinearPattern", "LinearPattern")
        self.LinearPattern.Originals = [self.Box]
        self.LinearPattern.Direction = (self.Doc.X_Axis, [""])
        self.LinearPattern.Length = 90.0
        self.LinearPattern.Occurrences = 10
        self.LinearPattern.Refine = True
        self.Body.addObject(self.LinearPattern)
        self.Doc.recompute()
        self.assertAlmostEqual(self.LinearPattern.Shape.Volume, 1e4)
        self.LinearPattern.SuppressedIndices = [0]
        self.Doc.recompute()
        self.assertAlmostEqual(self.LinearPattern.Shape.Volume, 9e3)
        self.assertAlmostEqual(self.LinearPattern.Shape.BoundBox.XMin, 10.0)
        self.LinearPattern.SuppressedIndices = []
        self.Doc.recompute()
        self.assertAlmostEqual(self.LinearPattern.Shape.Volume, 1e4)
        # 44 + 84 + 42 = 170.  44 - 8 = 36 / 9 = 4.  84-12 = 72 / 9 = 8.  42 - 6 = 36 / 9 = 4
        # We have the original 26 from the first shape, plus 4 more vertices, 8 more edges and
        # 4 more faces for each additional copy.  Since they have to touch ( single shape rule ),
        # We're adding 4 points to define each additional prism's new points, 8 edges makes sense,
        # and 4 faces makes sense since we're defining essentially a tube, not a box for each copy.
        # self.assertNotEqual(self.LinearPattern.Shape.ElementReverseMap["Vertex44"], "")
        # self.assertNotEqual(self.LinearPattern.Shape.ElementReverseMap["Edge84"], "")
        # self.assertNotEqual(self.LinearPattern.Shape.ElementReverseMap["Face42"], "")
        #
        # self.assertEqual(self.LinearPattern.Shape.ElementMapSize, 170)    # TODO
        self.assertEqual(self.LinearPattern.Shape.ElementMapSize, 26)

    def testYAxisLinearPattern(self):
        self.Body = self.Doc.addObject("PartDesign::Body", "Body")
        self.Box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        self.Body.addObject(self.Box)
        self.Box.Length = 10.00
        self.Box.Width = 10.00
        self.Box.Height = 10.00
        self.Doc.recompute()
        self.LinearPattern = self.Doc.addObject("PartDesign::LinearPattern", "LinearPattern")
        self.LinearPattern.Originals = [self.Box]
        self.LinearPattern.Direction = (self.Doc.Y_Axis, [""])
        self.LinearPattern.Length = 90.0
        self.LinearPattern.Occurrences = 10
        self.LinearPattern.Refine = True
        self.Body.addObject(self.LinearPattern)
        self.Doc.recompute()
        self.assertAlmostEqual(self.LinearPattern.Shape.Volume, 1e4)
        # self.assertEqual(self.LinearPattern.Shape.ElementMapSize, 170)    # TODO
        self.assertEqual(self.LinearPattern.Shape.ElementMapSize, 26)

    def testSuppressOriginalFeatureOccurrence(self):
        self.Body = self.Doc.addObject("PartDesign::Body", "Body")
        self.BaseBox = self.Doc.addObject("PartDesign::AdditiveBox", "BaseBox")
        self.Body.addObject(self.BaseBox)
        self.BaseBox.Length = 40.0
        self.BaseBox.Width = 10.0
        self.BaseBox.Height = 10.0

        self.Box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        self.Body.addObject(self.Box)
        self.Box.Length = 10.0
        self.Box.Width = 10.0
        self.Box.Height = 10.0
        self.Box.Placement.Base.z = 10.0
        self.Doc.recompute()

        self.LinearPattern = self.Doc.addObject("PartDesign::LinearPattern", "LinearPattern")
        self.Body.addObject(self.LinearPattern)
        self.LinearPattern.Originals = [self.Box]
        self.LinearPattern.Direction = (self.Doc.X_Axis, [""])
        self.LinearPattern.Length = 20.0
        self.LinearPattern.Occurrences = 3
        self.Doc.recompute()
        self.assertAlmostEqual(self.LinearPattern.Shape.Volume, 7e3)

        self.LinearPattern.SuppressedIndices = [0]
        self.Doc.recompute()
        self.assertAlmostEqual(self.LinearPattern.Shape.Volume, 6e3)

    def testSuppressOriginalWholeShapeOccurrence(self):
        self.Body = self.Doc.addObject("PartDesign::Body", "Body")
        self.Box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        self.Body.addObject(self.Box)
        self.Box.Length = 10.0
        self.Box.Width = 10.0
        self.Box.Height = 10.0
        self.Doc.recompute()

        self.LinearPattern = self.Doc.addObject("PartDesign::LinearPattern", "LinearPattern")
        self.Body.addObject(self.LinearPattern)
        self.LinearPattern.TransformMode = "Whole shape"
        self.LinearPattern.Direction = (self.Doc.X_Axis, [""])
        self.LinearPattern.Length = 20.0
        self.LinearPattern.Occurrences = 3
        self.LinearPattern.SuppressedIndices = [0]
        self.Doc.recompute()
        self.assertAlmostEqual(self.LinearPattern.Shape.Volume, 2e3)
        self.assertAlmostEqual(self.LinearPattern.Shape.BoundBox.XMin, 10.0)

    def testSuppressOriginalSubtractiveOccurrence(self):
        self.Body = self.Doc.addObject("PartDesign::Body", "Body")
        self.BaseBox = self.Doc.addObject("PartDesign::AdditiveBox", "BaseBox")
        self.Body.addObject(self.BaseBox)
        self.BaseBox.Length = 40.0
        self.BaseBox.Width = 10.0
        self.BaseBox.Height = 10.0

        self.Box = self.Doc.addObject("PartDesign::SubtractiveBox", "Box")
        self.Body.addObject(self.Box)
        self.Box.Length = 10.0
        self.Box.Width = 10.0
        self.Box.Height = 5.0
        self.Box.Placement.Base.z = 5.0
        self.Doc.recompute()

        self.LinearPattern = self.Doc.addObject("PartDesign::LinearPattern", "LinearPattern")
        self.Body.addObject(self.LinearPattern)
        self.LinearPattern.Originals = [self.Box]
        self.LinearPattern.Direction = (self.Doc.X_Axis, [""])
        self.LinearPattern.Length = 20.0
        self.LinearPattern.Occurrences = 3
        self.Doc.recompute()
        self.assertAlmostEqual(self.LinearPattern.Shape.Volume, 2.5e3)

        self.LinearPattern.SuppressedIndices = [0]
        self.Doc.recompute()
        self.assertAlmostEqual(self.LinearPattern.Shape.Volume, 3e3)

    def testZAxisLinearPattern(self):
        self.Body = self.Doc.addObject("PartDesign::Body", "Body")
        self.Box = self.Doc.addObject("PartDesign::AdditiveBox", "Box")
        self.Body.addObject(self.Box)
        self.Box.Length = 10.00
        self.Box.Width = 10.00
        self.Box.Height = 10.00
        self.Doc.recompute()
        self.LinearPattern = self.Doc.addObject("PartDesign::LinearPattern", "LinearPattern")
        self.LinearPattern.Originals = [self.Box]
        self.LinearPattern.Direction = (self.Doc.Z_Axis, [""])
        self.LinearPattern.Length = 90.0
        self.LinearPattern.Occurrences = 10
        self.LinearPattern.Refine = True
        self.Body.addObject(self.LinearPattern)
        self.Doc.recompute()
        self.assertAlmostEqual(self.LinearPattern.Shape.Volume, 1e4)
        # self.assertEqual(self.LinearPattern.Shape.ElementMapSize, 170)    # TODO
        self.assertEqual(self.LinearPattern.Shape.ElementMapSize, 26)

    def testNormalSketchAxisLinearPattern(self):
        self.Body = self.Doc.addObject("PartDesign::Body", "Body")
        self.PadSketch = self.Doc.addObject("Sketcher::SketchObject", "SketchPad")
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0, 0), (10, 10))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 10
        self.Doc.recompute()
        self.LinearPattern = self.Doc.addObject("PartDesign::LinearPattern", "LinearPattern")
        self.LinearPattern.Originals = [self.Pad]
        self.LinearPattern.Direction = (self.PadSketch, ["N_Axis"])
        self.LinearPattern.Length = 90.0
        self.LinearPattern.Occurrences = 10
        self.LinearPattern.Refine = True
        self.Body.addObject(self.LinearPattern)
        self.Doc.recompute()
        self.assertAlmostEqual(self.LinearPattern.Shape.Volume, 1e4)
        # self.assertEqual(self.LinearPattern.Shape.ElementMapSize, 170)    # TODO
        self.assertEqual(self.LinearPattern.Shape.ElementMapSize, 26)

    def testVerticalSketchAxisLinearPattern(self):
        self.Body = self.Doc.addObject("PartDesign::Body", "Body")
        self.PadSketch = self.Doc.addObject("Sketcher::SketchObject", "SketchPad")
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0, 0), (10, 10))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 10
        self.Doc.recompute()
        self.LinearPattern = self.Doc.addObject("PartDesign::LinearPattern", "LinearPattern")
        self.LinearPattern.Originals = [self.Pad]
        self.LinearPattern.Direction = (self.PadSketch, ["V_Axis"])
        self.LinearPattern.Length = 90.0
        self.LinearPattern.Occurrences = 10
        self.LinearPattern.Refine = True
        self.Body.addObject(self.LinearPattern)
        self.Doc.recompute()
        self.assertAlmostEqual(self.LinearPattern.Shape.Volume, 1e4)
        # self.assertEqual(self.LinearPattern.Shape.ElementMapSize, 170)    # TODO
        self.assertEqual(self.LinearPattern.Shape.ElementMapSize, 26)

    def testHorizontalSketchAxisLinearPattern(self):
        self.Body = self.Doc.addObject("PartDesign::Body", "Body")
        self.PadSketch = self.Doc.addObject("Sketcher::SketchObject", "SketchPad")
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0, 0), (10, 10))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 10
        self.Doc.recompute()
        self.LinearPattern = self.Doc.addObject("PartDesign::LinearPattern", "LinearPattern")
        self.LinearPattern.Originals = [self.Pad]
        self.LinearPattern.Direction = (self.PadSketch, ["H_Axis"])
        self.LinearPattern.Length = 90.0
        self.LinearPattern.Occurrences = 10
        self.LinearPattern.Refine = True
        self.Body.addObject(self.LinearPattern)
        self.Doc.recompute()
        self.assertAlmostEqual(self.LinearPattern.Shape.Volume, 1e4)
        # self.assertEqual(self.LinearPattern.Shape.ElementMapSize, 170)    # TODO
        # self.assertEqual(len(self.LinearPattern.Shape.ElementReverseMap), 170)
        self.assertEqual(self.LinearPattern.Shape.ElementMapSize, 26)

    def tearDown(self):
        # closing doc
        FreeCAD.closeDocument(self.Doc.Name)
        # print ("omit closing document for debugging")
