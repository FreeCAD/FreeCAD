
import FreeCAD
import os
import unittest
from .TechDrawTestUtilities import createPageWithSVGTemplate


class DrawViewSymbolTest(unittest.TestCase):
    def setUp(self):
        """Creates a page"""
        FreeCAD.newDocument("TDAnno")
        FreeCAD.setActiveDocument("TDAnno")
        FreeCAD.ActiveDocument = FreeCAD.getDocument("TDAnno")
        self.page = createPageWithSVGTemplate()

    def tearDown(self):
        FreeCAD.closeDocument("TDAnno")

    def testMakeSymbol(self):
        """Tests if an symbol can be added to page"""
        sym = FreeCAD.ActiveDocument.addObject("TechDraw::DrawViewSymbol", "TestSymbol")
        path = os.path.dirname(os.path.abspath(__file__))
        symbolFileSpec = path + "/TestSymbol.svg"
        f = open(symbolFileSpec, "r")
        svg = f.read()
        f.close()
        sym.Symbol = svg
        self.page.addView(sym)
        sym.X = 220.0
        sym.Y = 150.0

        FreeCAD.ActiveDocument.recompute()

        self.assertTrue("Up-to-date" in sym.State)


if __name__ == "__main__":
    unittest.main()
