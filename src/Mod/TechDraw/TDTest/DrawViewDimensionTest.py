import FreeCAD
import unittest
from .TechDrawTestUtilities import createPageWithSVGTemplate


class DrawViewDimensionTest(unittest.TestCase):
    def setUp(self):
        """Creates a page and 2 views"""
        FreeCAD.newDocument("TDDim")
        FreeCAD.setActiveDocument("TDDim")
        FreeCAD.ActiveDocument = FreeCAD.getDocument("TDDim")

        # make source feature
        FreeCAD.ActiveDocument.addObject("Part::Box", "Box")
        FreeCAD.ActiveDocument.addObject("Part::Sphere", "Sphere")

        # make a page
        self.page = createPageWithSVGTemplate()
        self.page.Scale = 5.0
        # page.ViewObject.show()  # unit tests run in console mode

        # make Views
        self.view1 = FreeCAD.ActiveDocument.addObject("TechDraw::DrawViewPart", "View")
        FreeCAD.ActiveDocument.View.Source = [FreeCAD.ActiveDocument.Box]
        self.page.addView(self.view1)
        self.view1.X = 30
        self.view1.Y = 150
        self.view2 = FreeCAD.activeDocument().addObject(
            "TechDraw::DrawViewPart", "View001"
        )
        FreeCAD.activeDocument().View001.Source = [FreeCAD.activeDocument().Sphere]
        self.page.addView(self.view2)
        self.view2.X = 220
        self.view2.Y = 150
        FreeCAD.ActiveDocument.recompute()

    def tearDown(self):
        FreeCAD.closeDocument("TDDim")

    def testLengthDimension(self):
        """Tests if a length dimension can be added to view1"""
        # make length dimension
        print("making length dimension")
        dimension = FreeCAD.ActiveDocument.addObject(
            "TechDraw::DrawViewDimension", "Dimension"
        )
        dimension.Type = "Distance"
        dimension.References2D = [(self.view1, "Edge1")]
        print("adding dim1 to page")
        self.page.addView(dimension)
        print("finished length dimension")
        FreeCAD.ActiveDocument.recompute()
        self.assertTrue("Up-to-date" in dimension.State)

    def testRadiusDimension(self):
        """Tests if a radius dimension can be added to view2"""
        print("making radius dimension")
        dimension = FreeCAD.ActiveDocument.addObject(
            "TechDraw::DrawViewDimension", "Dimension001"
        )
        dimension.Type = "Radius"
        dimension.MeasureType = "Projected"
        dimension.References2D = [(self.view2, "Edge0")]
        self.page.addView(dimension)
        FreeCAD.ActiveDocument.recompute()
        self.assertTrue("Up-to-date" in dimension.State)


if __name__ == "__main__":
    unittest.main()
