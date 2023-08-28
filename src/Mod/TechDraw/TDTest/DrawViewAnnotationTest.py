

import FreeCAD
import unittest
from .TechDrawTestUtilities import createPageWithSVGTemplate


class DrawViewAnnotationTest(unittest.TestCase):
    def setUp(self):
        """Creates a page"""
        FreeCAD.newDocument("TDAnno")
        FreeCAD.setActiveDocument("TDAnno")
        FreeCAD.ActiveDocument = FreeCAD.getDocument("TDAnno")
        self.page = createPageWithSVGTemplate()

    def tearDown(self):
        FreeCAD.closeDocument("TDAnno")

    def testMakeAnnotation(self):
        """Tests if an annotation can be added to page"""
        anno = FreeCAD.ActiveDocument.addObject(
            "TechDraw::DrawViewAnnotation", "TestAnno"
        )
        s = "Different Text"
        sl = list()
        sl.append(s)
        anno.Text = sl
        anno.TextStyle = "Bold"
        self.page.addView(anno)
        anno.X = 30.0
        anno.Y = 150.0
        FreeCAD.ActiveDocument.recompute()

        self.assertTrue("Up-to-date" in anno.State)


if __name__ == "__main__":
    unittest.main()
