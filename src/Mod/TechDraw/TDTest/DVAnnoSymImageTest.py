#!/usr/bin/env python
# -*- coding: utf-8 -*-

# annotation & symbol test script for TechDraw module
# creates a page, 1 annotation and import 1 symbol
from __future__ import print_function

import FreeCAD
import os


def DVAnnoSymImageTest():
    path = os.path.dirname(os.path.abspath(__file__))
    print("TDTestAnno path: " + path)
    templateFileSpec = path + "/TestTemplate.svg"
    symbolFileSpec = path + "/TestSymbol.svg"
    imageFileSpec = path + "/TestImage.png"

    FreeCAD.newDocument("TDAnno")
    FreeCAD.setActiveDocument("TDAnno")
    FreeCAD.ActiveDocument = FreeCAD.getDocument("TDAnno")

    page = FreeCAD.ActiveDocument.addObject("TechDraw::DrawPage", "Page")
    FreeCAD.ActiveDocument.addObject("TechDraw::DrawSVGTemplate", "Template")
    FreeCAD.ActiveDocument.Template.Template = templateFileSpec
    FreeCAD.ActiveDocument.Page.Template = FreeCAD.ActiveDocument.Template
    # page.ViewObject.show()  # unit tests run in console mode

    # symbol
    sym = FreeCAD.ActiveDocument.addObject("TechDraw::DrawViewSymbol", "TestSymbol")
    f = open(symbolFileSpec, "r")
    svg = f.read()
    f.close()
    sym.Symbol = svg
    page.addView(sym)
    sym.X = 220.0
    sym.Y = 150.0

    # image
    img = FreeCAD.ActiveDocument.addObject("TechDraw::DrawViewImage", "TestImage")
    img.ImageFile = imageFileSpec
    page.addView(img)

    FreeCAD.ActiveDocument.recompute()
    rc = False
    if (
        ("Up-to-date" in sym.State)
        and ("Up-to-date" in img.State)
    ):
        rc = True

    FreeCAD.closeDocument("TDAnno")
    return rc


if __name__ == "__main__":
    DVAnnoSymImageTest()
