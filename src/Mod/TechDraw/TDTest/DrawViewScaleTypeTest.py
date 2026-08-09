#!/usr/bin/env python3

import os
import tempfile
import unittest

import FreeCAD

from .TechDrawTestUtilities import createPageWithSVGTemplate


class DrawViewScaleTypeTest(unittest.TestCase):
    """Regression test for issue #30186: a symbol-style view (DrawViewSymbol and
    its DraftView / ArchView subclasses) whose ScaleType is set to 'Page' must
    keep that ScaleType across a save/reload cycle instead of reverting to
    'Custom'."""

    def setUp(self):
        self.document = FreeCAD.newDocument("TDScaleType")
        self.page = createPageWithSVGTemplate(self.document)
        self.page.Scale = 0.05
        self.savedFile = None

    def tearDown(self):
        for name in list(FreeCAD.listDocuments().keys()):
            FreeCAD.closeDocument(name)
        if self.savedFile and os.path.exists(self.savedFile):
            os.remove(self.savedFile)

    def _addPageSymbol(self):
        sym = self.document.addObject("TechDraw::DrawViewSymbol", "ScaleSym")
        path = os.path.dirname(os.path.abspath(__file__))
        with open(path + "/TestSymbol.svg", "r", encoding="utf-8") as f:
            sym.Symbol = f.read()
        self.page.addView(sym)
        sym.ScaleType = "Page"
        return sym

    def testScaleTypePageSyncsStoredScale(self):
        """Setting ScaleType to 'Page' must write the page scale into the stored
        Scale property rather than leaving it at the default."""
        sym = self._addPageSymbol()
        self.assertEqual(sym.ScaleType, "Page")
        self.assertAlmostEqual(sym.Scale, self.page.Scale)

    def testScaleTypePagePersistsOnReload(self):
        """ScaleType 'Page' must survive a save / close / reopen cycle (#30186)."""
        self._addPageSymbol()
        self.document.recompute()

        self.savedFile = os.path.join(tempfile.gettempdir(), "td_scaletype_30186.FCStd")
        self.document.saveAs(self.savedFile)
        FreeCAD.closeDocument(self.document.Name)

        reloaded = FreeCAD.openDocument(self.savedFile)
        sym = reloaded.getObject("ScaleSym")
        self.assertEqual(sym.ScaleType, "Page")
        self.assertAlmostEqual(sym.Scale, reloaded.getObject("Page").Scale)


if __name__ == "__main__":
    unittest.main()
