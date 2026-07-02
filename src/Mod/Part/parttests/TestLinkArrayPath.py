# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

import FreeCAD as App
import Part


class TestLinkArrayPath(unittest.TestCase):
    def setUp(self):
        self.doc = App.newDocument("TestLinkArrayPath")
        source = self.doc.addObject("Part::Box", "Source")
        source.Length = 2
        source.Width = 2
        source.Height = 2
        self.path = self.doc.addObject("Part::Feature", "Path")
        self.path.Shape = Part.makePolygon(
            [App.Vector(0, 0, 0), App.Vector(10, 0, 0), App.Vector(10, 10, 0)]
        )
        self.array = self.doc.addObject("Part::LinkArrayPath", "Array")
        self.array.LinkedObject = source
        self.array.Path = (self.path, ["Edge1", "Edge2"])

    def tearDown(self):
        App.closeDocument(self.doc.Name)

    def testFixedCountFollowsConnectedEdges(self):
        self.assertEqual(self.array.getTypeIdOfProperty("StartOffset"), "App::PropertyLength")
        self.assertEqual(self.array.getTypeIdOfProperty("EndOffset"), "App::PropertyLength")
        self.array.Count = 5
        self.doc.recompute()

        self.assertEqual(self.array.getStatusString(), "Valid")
        self.assertEqual(self.array.ElementCount, 5)
        expected = [
            App.Vector(0, 0, 0),
            App.Vector(5, 0, 0),
            App.Vector(10, 0, 0),
            App.Vector(10, 5, 0),
            App.Vector(10, 10, 0),
        ]
        for placement, point in zip(self.array.PlacementList, expected):
            self.assertLess((placement.Base - point).Length, 1e-7)

    def testFixedSpacingAndOffsets(self):
        self.array.SpacingMode = "Fixed spacing"
        self.array.Spacing = 4
        self.array.StartOffset = 2
        self.array.EndOffset = 3
        self.doc.recompute()

        self.assertEqual(self.array.getStatusString(), "Valid")
        self.assertEqual(self.array.ElementCount, 4)
        expected = [
            App.Vector(2, 0, 0),
            App.Vector(6, 0, 0),
            App.Vector(10, 0, 0),
            App.Vector(10, 4, 0),
        ]
        for placement, point in zip(self.array.PlacementList, expected):
            self.assertLess((placement.Base - point).Length, 1e-7)

    def testReversePath(self):
        self.array.Count = 3
        self.array.ReversePath = True
        self.doc.recompute()

        self.assertEqual(self.array.getStatusString(), "Valid")
        expected = [
            App.Vector(10, 10, 0),
            App.Vector(10, 0, 0),
            App.Vector(0, 0, 0),
        ]
        for placement, point in zip(self.array.PlacementList, expected):
            self.assertLess((placement.Base - point).Length, 1e-7)

    def testAlignFollowsPathTangent(self):
        self.array.Count = 3
        self.array.Align = True
        self.doc.recompute()

        self.assertEqual(self.array.getStatusString(), "Valid")
        direction = self.array.PlacementList[-1].Rotation.multVec(App.Vector(1, 0, 0))
        self.assertLess((direction - App.Vector(0, 1, 0)).Length, 1e-7)

    def testReverseAndAlignFollowPathTogether(self):
        self.array.Count = 3
        self.array.ReversePath = True
        self.array.Align = True
        self.doc.recompute()

        self.assertEqual(self.array.getStatusString(), "Valid")
        expected = [
            App.Vector(10, 10, 0),
            App.Vector(10, 0, 0),
            App.Vector(0, 0, 0),
        ]
        for placement, point in zip(self.array.PlacementList, expected):
            self.assertLess((placement.Base - point).Length, 1e-7)

    def testUnsetPathIsSilent(self):
        self.array.Path = None
        self.doc.recompute()

        self.assertEqual(self.array.getStatusString(), "Valid")
