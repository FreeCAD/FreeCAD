#!/usr/bin/env python
# -*- coding: utf-8 -*-

# test script for TechDraw module
# creates a page and 1 views
# adds a hatch area to view1
from __future__ import print_function

import FreeCAD
import os


def DHatchTest():
    path = os.path.dirname(os.path.abspath(__file__))
    print("TDHatch path: " + path)
    templateFileSpec = path + "/TestTemplate.svg"
    hatchFileSpec = path + "/TestHatch.svg"

    FreeCAD.newDocument("TDHatch")
    FreeCAD.setActiveDocument("TDHatch")
    FreeCAD.ActiveDocument = FreeCAD.getDocument("TDHatch")

    # make source feature
    box = FreeCAD.ActiveDocument.addObject("Part::Box", "Box")

    # make a page
    page = FreeCAD.ActiveDocument.addObject("TechDraw::DrawPage", "Page")
    FreeCAD.ActiveDocument.addObject("TechDraw::DrawSVGTemplate", "Template")
    FreeCAD.ActiveDocument.Template.Template = templateFileSpec
    FreeCAD.ActiveDocument.Page.Template = FreeCAD.ActiveDocument.Template
    page.Scale = 5.0
    # page.ViewObject.show()  #unit tests run in console mode

    # make Views
    view1 = FreeCAD.ActiveDocument.addObject("TechDraw::DrawViewPart", "View")
    FreeCAD.ActiveDocument.View.Source = [box]
    page.addView(view1)
    FreeCAD.ActiveDocument.recompute()

    # make hatch
    print("making hatch")
    hatch = FreeCAD.ActiveDocument.addObject("TechDraw::DrawHatch", "Hatch")
    hatch.Source = (view1, ["Face0"])
    hatch.HatchPattern = hatchFileSpec  # comment out to use default from preferences
    print("adding hatch to page")
    page.addView(hatch)
    print("finished hatch")

    FreeCAD.ActiveDocument.recompute()

    rc = False
    if "Up-to-date" in hatch.State:
        rc = True

    FreeCAD.closeDocument("TDHatch")
    return rc


if __name__ == "__main__":
    DHatchTest()
