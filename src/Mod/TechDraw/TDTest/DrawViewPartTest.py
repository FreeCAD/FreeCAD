# SPDX-License-Identifier: LGPL-2.1-or-later

# basic test script for TechDraw module
# creates a page and 1 view


import FreeCAD
import time
import unittest
from .TechDrawTestUtilities import createPageWithSVGTemplate
from PySide import QtCore

class DrawViewPartTest(unittest.TestCase):
    def setUp(self):
        """Creates a page and a view"""
        FreeCAD.newDocument("TDPart")
        FreeCAD.setActiveDocument("TDPart")
        FreeCAD.ActiveDocument = FreeCAD.getDocument("TDPart")

        FreeCAD.ActiveDocument.addObject("Part::Box", "Box")

        self.page = createPageWithSVGTemplate()
        self.page.Scale = 5.0
        # page.ViewObject.show()    # unit tests run in console mode
        print("DrawViewPart test: page created")

        self.view = FreeCAD.ActiveDocument.addObject("TechDraw::DrawViewPart", "View")
        self.page.addView(self.view)
        self.view.Source = [FreeCAD.ActiveDocument.Box]
        FreeCAD.ActiveDocument.recompute()

        #wait for threads to complete before checking result
        loop = QtCore.QEventLoop()

        timer = QtCore.QTimer()
        timer.setSingleShot(True)
        timer.timeout.connect(loop.quit)

        timer.start(2000)   #2 second delay
        loop.exec_()

    def tearDown(self):
        print("DrawViewPart test finished")
        FreeCAD.closeDocument("TDPart")

    def testMakeDrawViewPart(self):
        """Tests if a view can be added to page"""
        print("testing DrawViewPart")
        view = self.view
        edges = view.getVisibleEdges()
        self.assertEqual(len(edges), 4, "DrawViewPart has wrong number of edges")
        self.assertTrue("Up-to-date" in view.State, "DrawViewPart is not Up-to-date")

    def testDecorateCosmeticLine(self):
        """Decorates a cosmetic edge and checks its format."""
        view = self.view
        edgeName = f"Edge{len(view.getVisibleEdges())}"
        tag = view.makeCosmeticLine(FreeCAD.Vector(0, 0, 0), FreeCAD.Vector(10, 10, 0))
        self.assertEqual(view.getCosmeticEdgeBySelection(edgeName).Tag, tag)

        lineNumber = 8
        weight = 0.75
        color = (0.25, 0.5, 0.75, 0.5)
        visible = False
        view.decorateLine(edgeName, lineNumber, weight, color, visible)
        lineFormat = view.getCosmeticEdgeBySelection(edgeName).Format
        self.assertEqual(lineFormat["lineNumber"], lineNumber)
        self.assertEqual(lineFormat["style"], 0)
        self.assertAlmostEqual(lineFormat["weight"], weight)
        self.assertEqual(lineFormat["color"], color)
        self.assertEqual(lineFormat["visible"], visible)
        self.assertEqual(len(view.GeomFormats), 0)

    def testBreakPropertiesOnDrawViewPart(self):
        """Every DrawViewPart stores multiple breaks without helper sketches."""
        view = FreeCAD.ActiveDocument.addObject("TechDraw::DrawViewPart", "BreakableView")
        self.page.addView(view)
        view.Source = [FreeCAD.ActiveDocument.Box]

        self.assertEqual(view.BreakPoints, [])
        self.assertEqual(view.BreakDirections, [])
        self.assertEqual(view.BreakGaps, [])
        self.assertEqual(view.BreakLineTypes, [])

        view.BreakPoints = [
            FreeCAD.Vector(3.0, 0.0, 0.0),
            FreeCAD.Vector(7.0, 0.0, 0.0),
        ]
        view.BreakDirections = [FreeCAD.Vector(1.0, 0.0, 0.0)]
        view.BreakGaps = [1.0]
        view.BreakLineTypes = [1]
        FreeCAD.ActiveDocument.recompute()

        self.assertEqual(len(view.BreakPoints), 2)
        self.assertEqual(len(view.BreakDirections), 1)
        self.assertEqual(view.BreakGaps, [1.0])
        self.assertEqual(view.BreakLineTypes, [1])
        deadline = time.monotonic() + 10.0
        while not view.getVisibleEdges() and time.monotonic() < deadline:
            QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.AllEvents, 50)
            time.sleep(0.01)
        self.assertTrue(view.getVisibleEdges())

if __name__ == "__main__":
    unittest.main()
