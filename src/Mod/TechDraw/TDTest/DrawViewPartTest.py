# basic test script for TechDraw module
# creates a page and 1 view


import FreeCAD
import unittest
from .TechDrawTestUtilities import createPageWithSVGTemplate
from PySide import QtCore

class DrawViewPartTest(unittest.TestCase):
    def setUp(self):
        """Creates a page"""
        FreeCAD.newDocument("TDPart")
        FreeCAD.setActiveDocument("TDPart")
        FreeCAD.ActiveDocument = FreeCAD.getDocument("TDPart")

        FreeCAD.ActiveDocument.addObject("Part::Box", "Box")

        self.page = createPageWithSVGTemplate()
        self.page.Scale = 5.0
        # page.ViewObject.show()    # unit tests run in console mode
        print("DrawViewPart test: page created")

    def tearDown(self):
        print("DrawViewPart test finished")
        FreeCAD.closeDocument("TDPart")

    def testMakeDrawViewPart(self):
        """Tests if a view can be added to page"""
        print("testing DrawViewPart")
        view = FreeCAD.ActiveDocument.addObject("TechDraw::DrawViewPart", "View")
        self.page.addView(view)
        FreeCAD.ActiveDocument.View.Source = [FreeCAD.ActiveDocument.Box]
        FreeCAD.ActiveDocument.recompute()

        #wait for threads to complete before checking result
        loop = QtCore.QEventLoop()

        timer = QtCore.QTimer()
        timer.setSingleShot(True)
        timer.timeout.connect(loop.quit)

        timer.start(2000)   #2 second delay
        loop.exec_()

        edges = view.getVisibleEdges()
        self.assertEqual(len(edges), 4, "DrawViewPart has wrong number of edges")
        self.assertTrue("Up-to-date" in view.State, "DrawViewPart is not Up-to-date")

    def testDisplayStyleControlsHardHidden(self):
        """DisplayStyle controls hard hidden edges without changing other HLR options."""
        view = FreeCAD.ActiveDocument.addObject("TechDraw::DrawViewPart", "StyleView")

        self.assertEqual(view.DisplayStyle, "Visible Edges")
        self.assertFalse(view.HardHidden)
        hard_hidden_status = view.getPropertyStatus("HardHidden")
        self.assertIn(24, hard_hidden_status)  # App::Property::PropReadOnly
        self.assertIn(26, hard_hidden_status)  # App::Property::PropHidden

        view.DisplayStyle = "All Edges"
        self.assertTrue(view.HardHidden)

        view.DisplayStyle = "Hidden Edges"
        self.assertTrue(view.HardHidden)

        view.DisplayStyle = "Shaded with Edges"
        self.assertFalse(view.HardHidden)

        view.SmoothHidden = True
        view.SeamHidden = True
        view.DisplayStyle = "Shaded"
        self.assertTrue(view.SmoothHidden)
        self.assertTrue(view.SeamHidden)

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
        self.assertTrue(view.getVisibleEdges())

if __name__ == "__main__":
    unittest.main()
