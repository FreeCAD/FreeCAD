import os
import re
import tempfile
import unittest

import FreeCAD
import TechDrawGui

from .TechDrawTestUtilities import createPageWithSVGTemplate


class DrawAuxiliaryViewGuiTest(unittest.TestCase):
    def setUp(self):
        FreeCAD.newDocument("TDAuxiliaryGui")
        FreeCAD.setActiveDocument("TDAuxiliaryGui")
        FreeCAD.ActiveDocument = FreeCAD.getDocument("TDAuxiliaryGui")

        self.box = FreeCAD.ActiveDocument.addObject("Part::Box", "Box")
        self.page = createPageWithSVGTemplate()

        self.base = FreeCAD.ActiveDocument.addObject("TechDraw::DrawViewPart", "BaseView")
        self.page.addView(self.base)
        self.base.Source = [self.box]
        self.base.Direction = (0.0, -1.0, 0.0)
        self.base.XDirection = (1.0, 0.0, 0.0)
        self.base.Scale = 1.0

    def tearDown(self):
        FreeCAD.closeDocument("TDAuxiliaryGui")

    def makeAuxiliaryView(self):
        auxiliary = FreeCAD.ActiveDocument.addObject("TechDraw::DrawAuxiliaryView", "Auxiliary")
        self.page.addView(auxiliary)
        auxiliary.BaseView = self.base
        auxiliary.ReferenceStart = FreeCAD.Vector(0.0, 0.0, 0.0)
        auxiliary.ReferenceEnd = FreeCAD.Vector(10.0, 0.0, 0.0)
        auxiliary.AuxiliaryDirection = FreeCAD.Vector(10.0, 0.0, 0.0)
        auxiliary.ReferenceLabel = "AUX"
        auxiliary.Scale = 1.0
        return auxiliary

    def assertAuxiliaryLabelsAreOnPage(self, svg):
        view_box_match = re.search(r"<svg[^>]*viewBox=\"([^\"]+)\"", svg)
        self.assertIsNotNone(view_box_match)
        min_x, min_y, width, height = [float(value) for value in view_box_match.group(1).split()]
        max_x = min_x + width
        max_y = min_y + height

        label_positions = re.findall(
            r"<g[^>]*transform=\"matrix\([^,]+,[^,]+,[^,]+,[^,]+,([-.0-9]+),([-.0-9]+)\)\"[^>]*>\s*"
            r"<text[^>]*>AUX</text>",
            svg,
        )
        self.assertEqual(len(label_positions), 2)
        for label_x, label_y in label_positions:
            self.assertGreaterEqual(float(label_x), min_x)
            self.assertLessEqual(float(label_x), max_x)
            self.assertGreaterEqual(float(label_y), min_y)
            self.assertLessEqual(float(label_y), max_y)

    def testAuxiliaryMarkerIsExportedWithBaseView(self):
        auxiliary = self.makeAuxiliaryView()
        FreeCAD.ActiveDocument.recompute()

        with tempfile.TemporaryDirectory() as directory:
            svg_path = os.path.join(directory, "auxiliary_marker.svg")
            TechDrawGui.exportPageAsSvg(self.page, svg_path)
            with open(svg_path, encoding="utf-8") as svg_file:
                svg = svg_file.read()

        self.assertEqual(svg.count(">AUX<"), 2)
        self.assertAuxiliaryLabelsAreOnPage(svg)

        FreeCAD.ActiveDocument.removeObject(auxiliary.Name)
        FreeCAD.ActiveDocument.recompute()

        with tempfile.TemporaryDirectory() as directory:
            svg_path = os.path.join(directory, "auxiliary_marker_removed.svg")
            TechDrawGui.exportPageAsSvg(self.page, svg_path)
            with open(svg_path, encoding="utf-8") as svg_file:
                svg = svg_file.read()

        self.assertEqual(svg.count(">AUX<"), 0)


if __name__ == "__main__":
    unittest.main()
