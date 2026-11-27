# SPDX-License-Identifier: LGPL-2.1-or-later
"""The cosmetic thread of a reversed hole is clipped on the right side (issue #33321)."""

import unittest

import FreeCAD as App
import FreeCADGui as Gui
import TestSketcherApp
from pivy import coin


class TestHoleThreadClipGui(unittest.TestCase):
    def setUp(self):
        self.doc = App.newDocument("PartDesignTestHoleThreadClipGui")
        self.body = self.doc.addObject("PartDesign::Body", "Body")
        box = self.body.newObject("PartDesign::AdditiveBox", "Box")
        box.Length = box.Width = box.Height = 20
        sketch = self.body.newObject("Sketcher::SketchObject", "SketchHole")
        sketch.AttachmentSupport = (self.doc.getObject("XY_Plane"), [""])
        sketch.MapMode = "FlatFace"
        TestSketcherApp.CreateCircleSketch(sketch, (10, 10), 1)
        self.doc.recompute()
        # The sketch normal points out of the box: only a reversed hole goes into it
        self.hole = self.body.newObject("PartDesign::Hole", "Hole")
        self.hole.Profile = sketch
        self.hole.ThreadType = "ISOMetricProfile"
        self.hole.ThreadSize = "M6x1.0"
        self.hole.Threaded = True
        self.hole.CosmeticThread = True
        self.hole.Depth = 10
        self.hole.Reversed = True
        self.doc.recompute()
        Gui.updateGui()

    def tearDown(self):
        App.closeDocument(self.doc.Name)
        Gui.updateGui()

    def threadOverlay(self):
        search = coin.SoSearchAction()
        search.setType(coin.SoClipPlane.getClassTypeId())
        search.setInterest(coin.SoSearchAction.ALL)
        search.apply(self.body.ViewObject.RootNode)
        self.assertEqual(search.getPaths().getLength(), 1)
        path = search.getPaths().get(0)
        clipper = path.getTail()
        points = []
        separator = path.getNodeFromTail(1)
        for i in range(separator.getNumChildren()):
            child = separator.getChild(i)
            if child.isOfType(coin.SoCoordinate3.getClassTypeId()):
                points = [child.point[k].getValue() for k in range(child.point.getNum())]
        self.assertTrue(points)
        return clipper, points

    def testHoleDepthIsNotClipped(self):
        self.assertEqual(self.hole.ThreadDepthType, "Hole Depth")
        clipper, _ = self.threadOverlay()
        self.assertFalse(clipper.on.getValue())

    def testReversedThreadDepthClipsTheEnd(self):
        self.hole.ThreadDepthType = "Dimension"
        self.hole.ThreadDepth = 4
        self.doc.recompute()
        Gui.updateGui()
        self.assertAlmostEqual(self.hole.ThreadDepth.Value, 4)
        clipper, points = self.threadOverlay()
        self.assertTrue(clipper.on.getValue())
        plane = clipper.plane.getValue()
        normal = plane.getNormal()
        distance = plane.getDistanceFromOrigin()

        def visible(p):
            return normal[0] * p[0] + normal[1] * p[1] + normal[2] * p[2] - distance >= -1e-6

        # The hole goes from z=0 to z=10; the thread must stop at z=4
        inside = [p for p in points if p[2] < 4 - 1e-3]
        beyond = [p for p in points if p[2] > 4 + 1e-3]
        self.assertTrue(inside)
        self.assertTrue(beyond)
        self.assertTrue(all(visible(p) for p in inside))
        self.assertFalse(any(visible(p) for p in beyond))


if __name__ == "__main__":
    unittest.main()
