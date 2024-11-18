#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import FreeCAD
import os
import unittest


class DrawHatchTest(unittest.TestCase):
    def setUp(self):
        """Creates a page and view"""
        self.path = os.path.dirname(os.path.abspath(__file__))
        print("TDHatch path: " + self.path)
        templateFileSpec = self.path + "/TestTemplate.svg"

        FreeCAD.newDocument("TDHatch")
        FreeCAD.setActiveDocument("TDHatch")
        FreeCAD.ActiveDocument = FreeCAD.getDocument("TDHatch")

        # make source feature
        box = FreeCAD.ActiveDocument.addObject("Part::Box", "Box")

        # make a page
        self.page = FreeCAD.ActiveDocument.addObject("TechDraw::DrawPage", "Page")
        FreeCAD.ActiveDocument.addObject("TechDraw::DrawSVGTemplate", "Template")
        FreeCAD.ActiveDocument.Template.Template = templateFileSpec
        FreeCAD.ActiveDocument.Page.Template = FreeCAD.ActiveDocument.Template
        self.page.Scale = 5.0
        # page.ViewObject.show()  #unit tests run in console mode

        # make Views
        self.view = FreeCAD.ActiveDocument.addObject("TechDraw::DrawViewPart", "View")
        FreeCAD.ActiveDocument.View.Source = [box]
        self.page.addView(self.view)
        FreeCAD.ActiveDocument.recompute()

    def tearDown(self):
        FreeCAD.closeDocument("TDHatch")

    def testMakeHatchCase(self):
        """Tests if hatch area can be added to view"""
        # make hatch
        print("making hatch")
        hatch = FreeCAD.ActiveDocument.addObject("TechDraw::DrawHatch", "Hatch")
        hatch.Source = (self.view, ["Face0"])
        hatchFileSpec = self.path + "/TestHatch.svg"
        # comment out to use default from preferences
        hatch.HatchPattern = (
            hatchFileSpec
        )
        print("finished hatch")
        FreeCAD.ActiveDocument.recompute()

        self.assertTrue("Up-to-date" in hatch.State)


if __name__ == "__main__":
    unittest.main()
