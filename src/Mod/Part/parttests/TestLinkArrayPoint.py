# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

import FreeCAD as App
import Part


class TestLinkArrayPoint(unittest.TestCase):
    def setUp(self):
        self.doc = App.newDocument("TestLinkArrayPoint")
        source = self.doc.addObject("Part::Box", "Source")
        self.points = self.doc.addObject("Part::Feature", "Points")
        self.points.Shape = Part.makeCompound(
            [
                Part.Vertex(App.Vector(2, 3, 4)),
                Part.Vertex(App.Vector(12, 3, 4)),
                Part.Vertex(App.Vector(12, 13, 4)),
            ]
        )
        self.array = self.doc.addObject("Part::LinkArrayPoint", "Array")
        self.array.LinkedObject = source
        self.array.PointObject = self.points

    def tearDown(self):
        App.closeDocument(self.doc.Name)

    def testCopiesArePlacedAtPointCoordinates(self):
        self.doc.recompute()

        self.assertEqual(self.array.getStatusString(), "Valid")
        self.assertEqual(self.array.ElementCount, 3)
        expected = [
            App.Vector(2, 3, 4),
            App.Vector(12, 3, 4),
            App.Vector(12, 13, 4),
        ]
        for placement, point in zip(self.array.PlacementList, expected):
            self.assertLess((placement.Base - point).Length, 1e-7)

    def testDuplicateVerticesAreIgnored(self):
        self.points.Shape = Part.makeCompound(
            [
                Part.Vertex(App.Vector(1, 2, 3)),
                Part.Vertex(App.Vector(1, 2, 3)),
                Part.Vertex(App.Vector(4, 5, 6)),
            ]
        )
        self.doc.recompute()

        self.assertEqual(self.array.getStatusString(), "Valid")
        self.assertEqual(self.array.ElementCount, 2)

    def testUnsetPointObjectIsSilent(self):
        self.array.PointObject = None
        self.doc.recompute()

        self.assertEqual(self.array.getStatusString(), "Valid")
