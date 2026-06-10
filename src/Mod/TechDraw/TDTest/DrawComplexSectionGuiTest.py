import FreeCAD
import Part
import unittest
from .TechDrawTestUtilities import createPageWithSVGTemplate
from PySide import QtCore, QtWidgets

try:
    import shiboken6 as shiboken
except ImportError:
    import shiboken2 as shiboken


class DrawComplexSectionGuiTest(unittest.TestCase):
    def setUp(self):
        FreeCAD.newDocument("TDComplexSectionGui")
        FreeCAD.setActiveDocument("TDComplexSectionGui")
        FreeCAD.ActiveDocument = FreeCAD.getDocument("TDComplexSectionGui")

        self.box = FreeCAD.ActiveDocument.addObject("Part::Box", "Box")
        self.box.Length = 10.0
        self.box.Width = 10.0
        self.box.Height = 10.0

        self.closedProfile = FreeCAD.ActiveDocument.addObject(
            "Part::Feature", "ClosedProfile"
        )
        self.closedProfile.Shape = Part.makePolygon(
            [
                FreeCAD.Vector(1.0, 1.0, 10.0),
                FreeCAD.Vector(4.0, 1.0, 10.0),
                FreeCAD.Vector(4.0, 4.0, 10.0),
                FreeCAD.Vector(1.0, 4.0, 10.0),
                FreeCAD.Vector(1.0, 1.0, 10.0),
            ]
        )

        self.page = createPageWithSVGTemplate()
        self.page.Scale = 5.0

        self.view = FreeCAD.ActiveDocument.addObject("TechDraw::DrawViewPart", "View")
        self.page.addView(self.view)
        self.view.Source = [self.box]
        self.view.Direction = (0.0, 0.0, 1.0)
        self.view.Rotation = 0.0
        self.view.X = 30.0
        self.view.Y = 150.0

    def tearDown(self):
        FreeCAD.closeDocument("TDComplexSectionGui")

    def waitForGui(self, milliseconds=2000):
        loop = QtCore.QEventLoop()
        timer = QtCore.QTimer()
        timer.setSingleShot(True)
        timer.timeout.connect(loop.quit)
        timer.start(milliseconds)
        loop.exec_()

    def testBrokenOutSectionDrawsSectionFaceAndPatternHatch(self):
        import TechDrawGui

        section = FreeCAD.ActiveDocument.addObject(
            "TechDraw::DrawComplexSection", "BrokenOutSection"
        )
        self.page.addView(section)
        section.Source = [self.box]
        section.BaseView = self.view
        section.CuttingToolWireObject = self.closedProfile
        section.Direction = (0.0, 0.0, 1.0)
        section.SectionNormal = (0.0, 0.0, 1.0)
        section.SectionOrigin = (5.0, 5.0, 10.0)
        section.ProjectionStrategy = "BrokenOut"
        section.BrokenOutDepth = 4.0
        section.CutSurfaceDisplay = "PatHatch"
        section.HatchScale = 1.0

        FreeCAD.ActiveDocument.recompute()
        self.page.ViewObject.show()
        self.page.requestPaint()
        self.waitForGui()

        sceneObject = TechDrawGui.getSceneForPage(self.page)
        sceneAddress = shiboken.getCppPointer(sceneObject)[0]
        scene = shiboken.wrapInstance(sceneAddress, QtWidgets.QGraphicsScene)
        sectionFaces = [
            item
            for item in scene.items()
            if item.type() == 65556 and abs(item.zValue() - 40.0) < 0.1
        ]

        self.assertGreaterEqual(
            len(sectionFaces),
            1,
            "Broken-out section should draw at least one section-face item",
        )
        self.assertTrue(
            any(face.path().boundingRect().width() > 0 for face in sectionFaces),
            "Broken-out section faces should have a non-empty drawn path",
        )
        self.assertTrue(
            any(
                child.type() == 2 and not child.path().isEmpty()
                for face in sectionFaces
                for child in face.childItems()
            ),
            "Broken-out section faces should draw PAT hatch child paths",
        )


if __name__ == "__main__":
    unittest.main()
