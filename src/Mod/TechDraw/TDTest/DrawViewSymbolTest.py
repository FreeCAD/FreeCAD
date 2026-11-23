# SPDX-License-Identifier: LGPL-2.1-or-later

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
        f = open(symbolFileSpec, "r", encoding="utf-8")
        svg = f.read()
        f.close()
        sym.Symbol = svg
        self.page.addView(sym)
        sym.X = 220.0
        sym.Y = 150.0

        FreeCAD.ActiveDocument.recompute()

        self.assertTrue("Up-to-date" in sym.State)

    def testNonAsciiSymbol(self):
        """Tests if a Non-Ascii symbol can be added to page"""
        sym = FreeCAD.ActiveDocument.addObject(
            "TechDraw::DrawViewSymbol", "NonAsciiSymbol"
        )
        path = os.path.dirname(os.path.abspath(__file__))
        symbolFileSpec = path + "/TestNonAsciiSymbol.svg"
        f = open(symbolFileSpec, "r", encoding="utf-8")
        svg = f.read()
        f.close()
        sym.Symbol = svg
        self.page.addView(sym)
        sym.X = 220.0
        sym.Y = 150.0

        FreeCAD.ActiveDocument.recompute()

        self.assertTrue("Up-to-date" in sym.State)

    def testEditableTexts(self):
        """Tests if editable texts are extracted and can be updated/omitted"""
        sym = FreeCAD.ActiveDocument.addObject(
            "TechDraw::DrawViewSymbol", "EditableSymbol"
        )
        path = os.path.dirname(os.path.abspath(__file__))
        symbolFileSpec = path + "/TestNonAsciiSymbol.svg"
        with open(symbolFileSpec, "r", encoding="utf-8") as f:
            sym.Symbol = f.read()

        self.page.addView(sym)
        FreeCAD.ActiveDocument.recompute()

        self.assertEqual(len(sym.EditableTexts), 4)

        # Update editable texts with custom values
        sym.EditableTexts = ["custom1", "custom2", "custom3", "custom4"]
        self.assertIn("custom1", sym.Symbol)
        self.assertIn("custom2", sym.Symbol)

        # Omit/clear an editable text
        sym.EditableTexts = ["", "custom2", "custom3", "custom4"]
        self.assertNotIn("custom1", sym.Symbol)

        # Test symbol with freecad:editable attribute on text without tspan
        sym_no_tspan = FreeCAD.ActiveDocument.addObject(
            "TechDraw::DrawViewSymbol", "NoTspanSymbol"
        )
        sym_no_tspan.Symbol = (
            '<svg xmlns="http://www.w3.org/2000/svg" '
            'xmlns:freecad="https://www.freecad.org/wiki/index.php?title=Svg_Namespace">'
            '<text freecad:editable="Roughness">Ra50</text></svg>'
        )
        self.assertEqual(sym_no_tspan.EditableTexts, ["Ra50"])
        sym_no_tspan.EditableTexts = ["2.3"]
        self.assertIn("2.3", sym_no_tspan.Symbol)
        self.assertNotIn("Ra50", sym_no_tspan.Symbol)
        sym_no_tspan.EditableTexts = [""]
        self.assertNotIn("2.3", sym_no_tspan.Symbol)



if __name__ == "__main__":
    unittest.main()
