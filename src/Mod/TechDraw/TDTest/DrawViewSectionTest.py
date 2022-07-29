import FreeCAD
import unittest
from .TechDrawTestUtilities import createPageWithSVGTemplate


class DrawViewSectionTest(unittest.TestCase):
    def setUp(self):
        """Creates a page and a view"""
        FreeCAD.newDocument("TDSection")
        FreeCAD.setActiveDocument("TDSection")
        FreeCAD.ActiveDocument = FreeCAD.getDocument("TDSection")

        self.box = FreeCAD.ActiveDocument.addObject("Part::Box", "Box")

        self.page = createPageWithSVGTemplate()
        self.page.Scale = 5.0
        # page.ViewObject.show()    # unit tests run in console mode
        print("page created")

        self.view = FreeCAD.ActiveDocument.addObject("TechDraw::DrawViewPart", "View")
        self.page.addView(self.view)
        self.view.Source = [self.box]
        self.view.Direction = (0.0, 0.0, 1.0)
        self.view.Rotation = 0.0
        self.view.X = 30.0
        self.view.Y = 150.0
        print("view created")

    def tearDown(self):
        FreeCAD.closeDocument("TDSection")

    def testMakeDrawViewSection(self):
        """Tests if a DrawViewSection can be added to page"""
        section = FreeCAD.ActiveDocument.addObject(
            "TechDraw::DrawViewSection", "Section"
        )
        self.page.addView(section)
        section.Source = [self.box]
        section.BaseView = self.view
        section.Direction = (0.0, 1.0, 0.0)
        section.SectionNormal = (0.0, 1.0, 0.0)
        section.SectionOrigin = (5.0, 5.0, 5.0)
        self.view.touch()
        print("section created")

        FreeCAD.ActiveDocument.recompute()
        self.assertTrue("Up-to-date" in section.State)


if __name__ == "__main__":
    unittest.main()
