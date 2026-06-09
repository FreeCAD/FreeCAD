#!/usr/bin/env python3

# SPDX-License-Identifier: LGPL-2.0-or-later

import FreeCAD
import FreeCADGui
import math
import Part
import Sketcher
import unittest
import _PartDesign

from .TechDrawTestUtilities import createPageWithSVGTemplate
from PySide import QtCore


class DrawThreadExtensionTest(unittest.TestCase):
    def setUp(self):
        FreeCAD.newDocument("TDThreadExtension")
        FreeCAD.setActiveDocument("TDThreadExtension")
        FreeCAD.ActiveDocument = FreeCAD.getDocument("TDThreadExtension")

        self.page = createPageWithSVGTemplate()

    def tearDown(self):
        FreeCAD.closeDocument("TDThreadExtension")

    def waitForGui(self):
        loop = QtCore.QEventLoop()
        timer = QtCore.QTimer()
        timer.setSingleShot(True)
        timer.timeout.connect(loop.quit)
        timer.start(1000)
        loop.exec_()
        FreeCADGui.updateGui()

    def selectViewEdges(self, view, subnames):
        FreeCADGui.Selection.clearSelection()
        for subname in subnames:
            FreeCADGui.Selection.addSelection(FreeCAD.ActiveDocument.Name, view.Name, subname)

    def makeThreadDepthView(self,
                            side0Start=FreeCAD.Vector(0, 0, 0),
                            side0End=FreeCAD.Vector(0, 20, 0),
                            side1Start=FreeCAD.Vector(10, 0, 0),
                            side1End=FreeCAD.Vector(10, 20, 0),
                            depthStart=FreeCAD.Vector(0, 8, 0),
                            depthEnd=FreeCAD.Vector(10, 8, 0)):
        fixture_shape = Part.makeCompound(
            [
                Part.makeLine(side0Start, side0End),
                Part.makeLine(side1Start, side1End),
                Part.makeLine(depthStart, depthEnd),
            ]
        )
        fixture = FreeCAD.ActiveDocument.addObject("Part::Feature", "ThreadDepthFixture")
        fixture.Shape = fixture_shape

        view = FreeCAD.ActiveDocument.addObject("TechDraw::DrawViewPart", "View")
        self.page.addView(view)
        view.Source = [fixture]
        view.Direction = (0, 0, 1)
        view.Scale = 1.0
        FreeCAD.ActiveDocument.recompute()
        self.waitForGui()
        self.assertEqual(len(view.getVisibleEdges()), 3)
        return view

    def makePartDesignThreadedHoleView(self, threaded=True, threadDepth=4):
        body = FreeCAD.ActiveDocument.addObject("PartDesign::Body", "Body")
        box = FreeCAD.ActiveDocument.addObject("PartDesign::AdditiveBox", "Box")
        box.Length = 10
        box.Width = 10
        box.Height = 10
        body.addObject(box)
        FreeCAD.ActiveDocument.recompute()

        sketch = FreeCAD.ActiveDocument.addObject("Sketcher::SketchObject", "SketchHole")
        sketch.AttachmentSupport = (FreeCAD.ActiveDocument.XY_Plane, [""])
        sketch.MapMode = "FlatFace"
        sketch.MapReversed = True
        body.addObject(sketch)
        sketch.addGeometry(Part.Circle(FreeCAD.Vector(-5, 5, 0), FreeCAD.Vector(0, 0, 1), 1), False)
        sketch.addConstraint(Sketcher.Constraint("Radius", 0, 1))
        sketch.addConstraint(Sketcher.Constraint("DistanceX", 0, 3, -5))
        sketch.addConstraint(Sketcher.Constraint("DistanceY", 0, 3, 5))
        FreeCAD.ActiveDocument.recompute()

        hole = FreeCAD.ActiveDocument.addObject("PartDesign::Hole", "Hole")
        hole.Profile = sketch
        body.addObject(hole)
        hole.Depth = 10
        hole.ThreadType = 1
        hole.ThreadSize = 0
        hole.Threaded = threaded
        hole.ThreadDepthType = 1
        hole.ThreadDepth = threadDepth
        hole.DepthType = 0
        hole.DrillPoint = 0
        FreeCAD.ActiveDocument.recompute()

        view = FreeCAD.ActiveDocument.addObject("TechDraw::DrawViewPart", "HoleView")
        self.page.addView(view)
        view.Source = [body]
        view.Direction = (0, 1, 0)
        view.Scale = 1.0
        view.HardHidden = True
        view.SmoothHidden = True
        FreeCAD.ActiveDocument.recompute()
        self.waitForGui()
        self.assertGreaterEqual(len(view.getHiddenEdges()), 2)
        return view

    def findHiddenHoleSideEdgeNames(self, view):
        visibleCount = len(view.getVisibleEdges())
        candidates = []
        for index, edge in enumerate(view.getHiddenEdges()):
            vertices = edge.Vertexes
            if len(vertices) != 2:
                continue

            start = vertices[0].Point
            end = vertices[1].Point
            if abs(start.x - end.x) > 1e-3:
                continue

            candidates.append((abs((start.x + end.x) / 2.0), visibleCount + index))

        self.assertGreaterEqual(len(candidates), 2)
        return [f"Edge{index}" for _, index in sorted(candidates)[:2]]

    def runThreadCommand(self, view, subnames, command):
        self.selectViewEdges(view, subnames)
        FreeCADGui.runCommand(command)
        self.waitForGui()

    def cosmeticLineLengths(self, view):
        return [
            math.hypot(edge.End.x - edge.Start.x, edge.End.y - edge.Start.y)
            for edge in view.CosmeticEdges[:2]
        ]

    def assertThreadLineLengths(self, view, expectedLength, expectedEdges):
        self.assertEqual(len(view.CosmeticEdges), expectedEdges)
        for length in self.cosmeticLineLengths(view):
            self.assertAlmostEqual(length, expectedLength, places=3)

    def pointsAlmostEqual(self, first, second):
        return (
            abs(first.x - second.x) < 1e-3
            and abs(first.y - second.y) < 1e-3
            and abs(first.z - second.z) < 1e-3
        )

    def assertHoleThreadEndLineConnectsThreadEdges(self, view):
        endLine = view.CosmeticEdges[2]
        firstThread = view.CosmeticEdges[0]
        secondThread = view.CosmeticEdges[1]
        endpointPairs = [
            (firstThread.Start, secondThread.Start),
            (firstThread.Start, secondThread.End),
            (firstThread.End, secondThread.Start),
            (firstThread.End, secondThread.End),
        ]
        for first, second in endpointPairs:
            if (self.pointsAlmostEqual(endLine.Start, first)
                    and self.pointsAlmostEqual(endLine.End, second)):
                return
            if (self.pointsAlmostEqual(endLine.Start, second)
                    and self.pointsAlmostEqual(endLine.End, first)):
                return
        self.fail("Thread end line is not connected to the generated thread line endpoints")

    def testThreadHoleSideUsesFullSelectedEdgeLengthByDefault(self):
        view = self.makeThreadDepthView()

        self.runThreadCommand(view, ["Edge0", "Edge1"], "TechDraw_ExtensionThreadHoleSide")

        self.assertThreadLineLengths(view, 20.0, 3)

    def testThreadHoleSideCanUseSelectedDepthReference(self):
        view = self.makeThreadDepthView()

        self.runThreadCommand(view, ["Edge0", "Edge1", "Edge2"], "TechDraw_ExtensionThreadHoleSide")

        self.assertThreadLineLengths(view, 8.0, 3)
        self.assertHoleThreadEndLineConnectsThreadEdges(view)

    def testThreadHoleSideUsesPartDesignHoleThreadDepth(self):
        view = self.makePartDesignThreadedHoleView()
        sideEdges = self.findHiddenHoleSideEdgeNames(view)

        self.runThreadCommand(view, sideEdges, "TechDraw_ExtensionThreadHoleSide")

        self.assertThreadLineLengths(view, 4.0, 3)
        self.assertEqual(len(view.CenterLines), 1)
        self.assertHoleThreadEndLineConnectsThreadEdges(view)

    def testThreadHoleSideUsesPartDesignHoleThreadDepthOverHalfDepth(self):
        view = self.makePartDesignThreadedHoleView(threadDepth=8)
        sideEdges = self.findHiddenHoleSideEdgeNames(view)

        self.runThreadCommand(view, sideEdges, "TechDraw_ExtensionThreadHoleSide")

        self.assertThreadLineLengths(view, 8.0, 3)
        self.assertEqual(len(view.CenterLines), 1)
        self.assertHoleThreadEndLineConnectsThreadEdges(view)

    def testThreadHoleSideIgnoresUnthreadedPartDesignHole(self):
        view = self.makePartDesignThreadedHoleView(threaded=False, threadDepth=4)
        sideEdges = self.findHiddenHoleSideEdgeNames(view)

        self.runThreadCommand(view, sideEdges, "TechDraw_ExtensionThreadHoleSide")

        self.assertThreadLineLengths(view, 10.0, 3)
        self.assertEqual(len(view.CenterLines), 1)

    def testThreadHoleSideDepthReferenceHandlesReversedSideEdges(self):
        view = self.makeThreadDepthView(
            side0Start=FreeCAD.Vector(0, 20, 0),
            side0End=FreeCAD.Vector(0, 0, 0),
            side1Start=FreeCAD.Vector(10, 20, 0),
            side1End=FreeCAD.Vector(10, 0, 0),
        )

        self.runThreadCommand(view, ["Edge0", "Edge1", "Edge2"], "TechDraw_ExtensionThreadHoleSide")

        self.assertThreadLineLengths(view, 8.0, 3)
        self.assertHoleThreadEndLineConnectsThreadEdges(view)

    def testThreadHoleSideDepthReferenceHandlesMixedSideEdgeOrientation(self):
        view = self.makeThreadDepthView(
            side0Start=FreeCAD.Vector(0, 0, 0),
            side0End=FreeCAD.Vector(0, 20, 0),
            side1Start=FreeCAD.Vector(10, 20, 0),
            side1End=FreeCAD.Vector(10, 0, 0),
        )

        self.runThreadCommand(view, ["Edge0", "Edge1", "Edge2"], "TechDraw_ExtensionThreadHoleSide")

        self.assertThreadLineLengths(view, 8.0, 3)
        self.assertHoleThreadEndLineConnectsThreadEdges(view)

    def testThreadHoleSideDepthReferenceHandlesSwappedSelectionOrder(self):
        view = self.makeThreadDepthView()

        self.runThreadCommand(view, ["Edge1", "Edge0", "Edge2"], "TechDraw_ExtensionThreadHoleSide")

        self.assertThreadLineLengths(view, 8.0, 3)
        self.assertHoleThreadEndLineConnectsThreadEdges(view)

    def testThreadBoltSideCanUseSelectedDepthReference(self):
        view = self.makeThreadDepthView()

        self.runThreadCommand(view, ["Edge0", "Edge1", "Edge2"], "TechDraw_ExtensionThreadBoltSide")

        self.assertThreadLineLengths(view, 8.0, 2)


if __name__ == "__main__":
    unittest.main()
