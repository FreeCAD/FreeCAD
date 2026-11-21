# SPDX-License-Identifier: LGPL-2.1-or-later
"""Projected distance dimensions across the breaks of a view.

A view break removes material from the drawing, but a distance dimension
across it must still give the model length, as it does for a broken view
built from break objects. With a view break the dimension gave the shortened
drawing length instead: 70 instead of 100 for a 100 long box with 40 removed
and a gap of 10.
"""

import math
import time
import unittest

import FreeCAD as App
from PySide import QtCore

from .TechDrawTestUtilities import createPageWithSVGTemplate


class DrawDimensionViewBreakTest(unittest.TestCase):
    def setUp(self):
        self.doc = App.newDocument("TDDimensionViewBreak")
        self.addCleanup(lambda: App.closeDocument(self.doc.Name))
        self.page = createPageWithSVGTemplate(self.doc)
        self.box = self.doc.addObject("Part::Box", "Box")
        self.box.Length = 100
        self.box.Width = 20
        self.box.Height = 10

    def brokenView(self, rotation):
        view = self.doc.addObject("TechDraw::DrawViewPart", "View")
        self.page.addView(view)
        view.Source = [self.box]
        view.Direction = App.Vector(0, 0, 1)
        view.XDirection = App.Vector(1, 0, 0)
        view.ScaleType = "Custom"
        view.Scale = 2
        view.Rotation = rotation
        viewBreak = self.doc.addObject("TechDraw::DrawViewBreak", "Break")
        viewBreak.StartPoint = App.Vector(30, 0, 0)
        viewBreak.EndPoint = App.Vector(70, 0, 0)
        viewBreak.Direction = App.Vector(1, 0, 0)
        viewBreak.Gap = 10
        view.Breaks = [viewBreak]
        # both pieces are drawn: the break is applied
        self.recomputeView(view, 8)
        return view

    def recomputeView(self, view, vertexCount):
        """Recompute and wait for the projection to give vertexCount visible vertices."""
        self.doc.recompute()
        deadline = time.monotonic() + 10
        while len(view.getVisibleVertexes()) != vertexCount:
            self.assertLess(time.monotonic(), deadline, "projection did not finish")
            loop = QtCore.QEventLoop()
            QtCore.QTimer.singleShot(20, loop.quit)
            loop.exec_()

    def farthestVertices(self, view):
        """Opposite corners of the box, one on each side of the break."""
        vertices = view.getVisibleVertexes()
        pairs = [(i, j) for i in range(len(vertices)) for j in range(i + 1, len(vertices))]
        return max(pairs, key=lambda pair: (vertices[pair[0]] - vertices[pair[1]]).Length)

    def dimension(self, view, kind, vertices):
        dim = self.doc.addObject("TechDraw::DrawViewDimension", "Dimension")
        self.page.addView(dim)
        dim.Type = kind
        dim.MeasureType = "Projected"
        dim.References2D = [(view, "Vertex%d" % vertices[0]), (view, "Vertex%d" % vertices[1])]
        self.doc.recompute()
        self.assertIn("Up-to-date", dim.State)
        return dim.getRawValue()

    def testDistanceAcrossBreak(self):
        diagonal = math.hypot(100, 20)
        for rotation in (0, 30):
            with self.subTest(rotation=rotation):
                view = self.brokenView(rotation)
                vertices = self.farthestVertices(view)
                self.assertAlmostEqual(self.dimension(view, "Distance", vertices), diagonal, places=6)

    def testDistanceXAcrossBreak(self):
        view = self.brokenView(0)
        vertices = self.farthestVertices(view)
        self.assertAlmostEqual(self.dimension(view, "DistanceX", vertices), 100, places=6)
        self.assertAlmostEqual(self.dimension(view, "DistanceY", vertices), 20, places=6)

    def testSuppressedBreak(self):
        """Without an applied break the drawing length is the model length."""
        view = self.brokenView(0)
        view.Breaks[0].Suppressed = True
        self.recomputeView(view, 4)
        vertices = self.farthestVertices(view)
        self.assertAlmostEqual(self.dimension(view, "DistanceX", vertices), 100, places=6)


if __name__ == "__main__":
    unittest.main()
