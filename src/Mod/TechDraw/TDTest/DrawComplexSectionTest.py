import FreeCAD
import Part
import unittest
from .TechDrawTestUtilities import createPageWithSVGTemplate
from PySide import QtCore


class DrawComplexSectionTest(unittest.TestCase):
    def setUp(self):
        FreeCAD.newDocument("TDComplexSection")
        FreeCAD.setActiveDocument("TDComplexSection")
        FreeCAD.ActiveDocument = FreeCAD.getDocument("TDComplexSection")

        self.box = FreeCAD.ActiveDocument.addObject("Part::Box", "Box")
        self.box.Length = 10.0
        self.box.Width = 10.0
        self.box.Height = 10.0

        self.page = createPageWithSVGTemplate()
        self.page.Scale = 5.0

        self.view = FreeCAD.ActiveDocument.addObject("TechDraw::DrawViewPart", "View")
        self.page.addView(self.view)
        self.view.Source = [self.box]
        self.view.Direction = (0.0, 0.0, 1.0)
        self.view.XDirection = (1.0, 0.0, 0.0)
        self.view.Rotation = 0.0
        self.view.X = 30.0
        self.view.Y = 150.0
        FreeCAD.ActiveDocument.recompute()
        self._waitForThreads()

    def tearDown(self):
        FreeCAD.closeDocument("TDComplexSection")

    def _waitForThreads(self):
        loop = QtCore.QEventLoop()
        timer = QtCore.QTimer()
        timer.setSingleShot(True)
        timer.timeout.connect(loop.quit)
        timer.start(2000)
        loop.exec_()

    def testMakeBrokenOutComplexSection(self):
        profile = FreeCAD.ActiveDocument.addObject("Part::Feature", "BrokenOutProfile")
        profile.Shape = Part.makePolygon([
            FreeCAD.Vector(2.0, 2.0, 10.0),
            FreeCAD.Vector(8.0, 2.0, 10.0),
            FreeCAD.Vector(8.0, 8.0, 10.0),
            FreeCAD.Vector(2.0, 8.0, 10.0),
            FreeCAD.Vector(2.0, 2.0, 10.0),
        ])

        section = FreeCAD.ActiveDocument.addObject(
            "TechDraw::DrawComplexSection", "BrokenOutSection"
        )
        self.page.addView(section)
        section.Source = [self.box]
        section.BaseView = self.view
        section.CuttingToolWireObject = profile
        section.ProjectionStrategy = "BrokenOut"
        section.BrokenOutDepth = 3.0
        section.Direction = self.view.Direction
        section.SectionNormal = self.view.Direction
        section.XDirection = self.view.XDirection
        section.SectionOrigin = (0.0, 0.0, 0.0)
        section.SectionDirection = "Aligned"

        FreeCAD.ActiveDocument.recompute()
        self._waitForThreads()

        self.assertEqual(section.ProjectionStrategy, "BrokenOut")
        self.assertEqual(section.BrokenOutDepth, 3.0)
        self.assertGreater(len(section.getVisibleEdges()), 0)
        self.assertIn("Up-to-date", section.State)


if __name__ == "__main__":
    unittest.main()
