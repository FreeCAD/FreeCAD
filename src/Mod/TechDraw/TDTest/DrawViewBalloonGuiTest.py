import os
import re
import tempfile
import unittest

import FreeCAD
import TechDrawGui

from .TechDrawTestUtilities import createPageWithSVGTemplate


class DrawViewBalloonGuiTest(unittest.TestCase):
    def setUp(self):
        FreeCAD.newDocument("TDBalloonGui")
        FreeCAD.setActiveDocument("TDBalloonGui")
        FreeCAD.ActiveDocument = FreeCAD.getDocument("TDBalloonGui")
        self.document = FreeCAD.ActiveDocument

        self.document.addObject("Part::Box", "Box")

        self.page = createPageWithSVGTemplate(self.document)
        self.page.Scale = 5.0

        self.view = self.document.addObject("TechDraw::DrawViewPart", "View")
        self.page.addView(self.view)
        self.view.Source = [self.document.Box]
        self.view.X = 80
        self.view.Y = 120

        self.balloon = self.document.addObject("TechDraw::DrawViewBalloon", "Balloon")
        self.page.addView(self.balloon)
        self.balloon.SourceView = self.view
        self.balloon.BubbleShape = "Rectangle"
        self.balloon.Text = "⌖|0.05|A\n⌯|0.10|B"
        self.balloon.OriginX = 5
        self.balloon.OriginY = 5
        self.balloon.X = 45
        self.balloon.Y = 45

        self.document.recompute()

    def tearDown(self):
        FreeCAD.closeDocument("TDBalloonGui")

    def _export_svg(self):
        handle, path = tempfile.mkstemp(suffix=".svg")
        os.close(handle)
        TechDrawGui.exportPageAsSvg(self.page, path)
        with open(path, "r", encoding="utf-8") as svg_file:
            svg = svg_file.read()
        os.remove(path)
        return svg

    def testMultilineRectangleBalloonSvgExport(self):
        svg = self._export_svg()

        self.assertNotIn("nan", svg.lower())
        self.assertNotIn("inf", svg.lower())
        self.assertNotIn("|", svg)
        for token in ("⌖", "0.05", "A", "⌯", "0.10", "B"):
            self.assertIn(token, svg)

        paths = re.findall(r'<path d="([^"]+)"', svg)
        frame_path = max(paths, key=lambda path: path.count("M"))
        self.assertGreaterEqual(frame_path.count("M"), 4)

        coordinates = [
            (float(x), float(y))
            for x, y in re.findall(r"(-?\d+(?:\.\d+)?),(-?\d+(?:\.\d+)?)", frame_path)
        ]
        xs = sorted({round(x, 3) for x, _y in coordinates})
        ys = sorted({round(y, 3) for _x, y in coordinates})
        self.assertGreaterEqual(len(xs), 4)
        self.assertGreaterEqual(len(ys), 3)

        left = xs[0]
        first_separator = xs[1]
        top = ys[0]
        row_separator = ys[1]

        first_cell_width = first_separator - left
        row_height = row_separator - top
        self.assertLess(abs(first_cell_width - row_height), max(2.0, row_height * 0.20))


if __name__ == "__main__":
    unittest.main()
