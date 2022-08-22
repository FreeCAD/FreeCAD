#!/usr/bin/env python
# -*- coding: utf-8 -*-

# basic test script for TechDraw module
# creates a page and 1 view

import FreeCAD
import unittest
from .TechDrawTestUtilities import createPageWithSVGTemplate


class DrawViewPartTest(unittest.TestCase):
    def setUp(self):
        """Creates a page"""
        FreeCAD.newDocument("TDPart")
        FreeCAD.setActiveDocument("TDPart")
        FreeCAD.ActiveDocument = FreeCAD.getDocument("TDPart")

        FreeCAD.ActiveDocument.addObject("Part::Box", "Box")

        self.page = createPageWithSVGTemplate()
        self.page.Scale = 5.0
        # page.ViewObject.show()    # unit tests run in console mode
        print("page created")

    def tearDown(self):
        FreeCAD.closeDocument("TDPart")

    def testMakeDrawViewPart(self):
        """Tests if a view can be added to page"""
        view = FreeCAD.ActiveDocument.addObject("TechDraw::DrawViewPart", "View")
        self.page.addView(view)
        FreeCAD.ActiveDocument.View.Source = [FreeCAD.ActiveDocument.Box]
        FreeCAD.ActiveDocument.recompute()
        self.assertTrue("Up-to-date" in view.State)


if __name__ == "__main__":
    unittest.main()
