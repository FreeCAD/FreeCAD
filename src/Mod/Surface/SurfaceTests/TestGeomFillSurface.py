# SPDX-License-Identifier: LGPL-2.1-or-later

import unittest

import FreeCAD
import Part

App = FreeCAD


class TestGeomFillSurface(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("SurfaceTestGeomFillSurface")

    def testBoundaryListUsesTransformedShape(self):
        line1 = self.Doc.addObject("Part::Feature", "Line1")
        line1.Shape = Part.makeLine(App.Vector(0, 0, 0), App.Vector(0, 10, 0))
        line1.Placement.Base = App.Vector(20, 0, 0)

        line2 = self.Doc.addObject("Part::Feature", "Line2")
        line2.Shape = Part.makeLine(App.Vector(10, 0, 0), App.Vector(10, 10, 0))
        line2.Placement.Base = App.Vector(20, 0, 0)

        surface = self.Doc.addObject("Surface::GeomFillSurface", "Surface")
        surface.BoundaryList = [(line1, "Edge1"), (line2, "Edge1")]
        self.Doc.recompute()

        self.assertAlmostEqual(surface.Shape.BoundBox.XMin, 20)
        self.assertAlmostEqual(surface.Shape.BoundBox.XMax, 30)

    def testBoundaryListUsesParentGroupPlacement(self):
        container = self.Doc.addObject("App::Part", "Part")
        container.Placement.Base = App.Vector(20, 0, 0)

        line1 = self.Doc.addObject("Part::Feature", "Line1")
        line1.Shape = Part.makeLine(App.Vector(0, 0, 0), App.Vector(0, 10, 0))
        container.addObject(line1)

        line2 = self.Doc.addObject("Part::Feature", "Line2")
        line2.Shape = Part.makeLine(App.Vector(10, 0, 0), App.Vector(10, 10, 0))
        container.addObject(line2)

        surface = self.Doc.addObject("Surface::GeomFillSurface", "Surface")
        surface.BoundaryList = [(line1, "Edge1"), (line2, "Edge1")]
        self.Doc.recompute()

        self.assertAlmostEqual(surface.Shape.BoundBox.XMin, 20)
        self.assertAlmostEqual(surface.Shape.BoundBox.XMax, 30)

    def tearDown(self):
        if hasattr(App, "KeepTestDoc") and App.KeepTestDoc:
            return
        FreeCAD.closeDocument("SurfaceTestGeomFillSurface")
