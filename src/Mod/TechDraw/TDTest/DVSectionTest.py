#!/usr/bin/env python
# -*- coding: utf-8 -*-

# test script for TechDraw module
# creates a page, 1 view and 1 section view
from __future__ import print_function

import FreeCAD
import os


def DVSectionTest():
    path = os.path.dirname(os.path.abspath(__file__))
    print("TDSection path: " + path)
    templateFileSpec = path + "/TestTemplate.svg"

    FreeCAD.newDocument("TDSection")
    FreeCAD.setActiveDocument("TDSection")
    FreeCAD.ActiveDocument = FreeCAD.getDocument("TDSection")

    box = FreeCAD.ActiveDocument.addObject("Part::Box", "Box")

    page = FreeCAD.ActiveDocument.addObject("TechDraw::DrawPage", "Page")
    FreeCAD.ActiveDocument.addObject("TechDraw::DrawSVGTemplate", "Template")
    FreeCAD.ActiveDocument.Template.Template = templateFileSpec
    FreeCAD.ActiveDocument.Page.Template = FreeCAD.ActiveDocument.Template
    page.Scale = 5.0
    # page.ViewObject.show()    # unit tests run in console mode
    print("page created")

    view = FreeCAD.ActiveDocument.addObject("TechDraw::DrawViewPart", "View")
    rc = page.addView(view)
    view.Source = [box]
    view.Direction = (0.0, 0.0, 1.0)
    view.Rotation = 0.0
    view.X = 30.0
    view.Y = 150.0
    print("view created")

    section = FreeCAD.ActiveDocument.addObject("TechDraw::DrawViewSection", "Section")
    rc = page.addView(section)
    section.Source = [box]
    section.BaseView = view
    section.Direction = (0.0, 1.0, 0.0)
    section.SectionNormal = (0.0, 1.0, 0.0)
    section.SectionOrigin = (5.0, 5.0, 5.0)
    view.touch()
    print("section created")

    FreeCAD.ActiveDocument.recompute()
    rc = False
    if ("Up-to-date" in view.State) and ("Up-to-date" in section.State):
        rc = True

    FreeCAD.closeDocument("TDSection")
    return rc


if __name__ == "__main__":
    DVSectionTest()
