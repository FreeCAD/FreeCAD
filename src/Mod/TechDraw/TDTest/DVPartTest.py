#!/usr/bin/env python
# -*- coding: utf-8 -*-

# basic test script for TechDraw module
# creates a page and 1 view
from __future__ import print_function

import FreeCAD
import Part
import Measure
import TechDraw
import os

def DVPartTest():
    path = os.path.dirname(os.path.abspath(__file__))
    print ('TDPart path: ' + path)
    templateFileSpec = path + '/TestTemplate.svg'

    FreeCAD.newDocument("TDPart")
    FreeCAD.setActiveDocument("TDPart")
    FreeCAD.ActiveDocument=FreeCAD.getDocument("TDPart")

    box = FreeCAD.ActiveDocument.addObject("Part::Box","Box")

    page = FreeCAD.ActiveDocument.addObject('TechDraw::DrawPage','Page')
    FreeCAD.ActiveDocument.addObject('TechDraw::DrawSVGTemplate','Template')
    FreeCAD.ActiveDocument.Template.Template = templateFileSpec
    FreeCAD.ActiveDocument.Page.Template = FreeCAD.ActiveDocument.Template
    page.Scale = 5.0
#    page.ViewObject.show()    # unit tests run in console mode
    print("page created")

    view = FreeCAD.ActiveDocument.addObject('TechDraw::DrawViewPart','View')
    rc = page.addView(view)

    FreeCAD.ActiveDocument.View.Source = [FreeCAD.ActiveDocument.Box]

    FreeCAD.ActiveDocument.recompute()

    rc = False
    if ("Up-to-date" in view.State):
        rc = True
    FreeCAD.closeDocument("TDPart")
    return rc

if __name__ == '__main__':
    DVPartTest()
