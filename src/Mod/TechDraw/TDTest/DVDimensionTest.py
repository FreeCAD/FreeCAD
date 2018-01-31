#!/usr/bin/env python
# -*- coding: utf-8 -*-

# test script for TechDraw module
# creates a page and 2 views
# adds 1 length dimension to view1
# adds 1 radius dimension to view2
from __future__ import print_function

import FreeCAD
import Part
import Measure
import TechDraw
import os

def DVDimensionTest():
    path = os.path.dirname(os.path.abspath(__file__))
    print ('TDDim path: ' + path)
    templateFileSpec = path + '/TestTemplate.svg'

    FreeCAD.newDocument("TDDim")
    FreeCAD.setActiveDocument("TDDim")
    FreeCAD.ActiveDocument=FreeCAD.getDocument("TDDim")

    #make source feature
    box = FreeCAD.ActiveDocument.addObject("Part::Box","Box")
    sphere = FreeCAD.ActiveDocument.addObject("Part::Sphere","Sphere")

    #make a page
    page = FreeCAD.ActiveDocument.addObject('TechDraw::DrawPage','Page')
    FreeCAD.ActiveDocument.addObject('TechDraw::DrawSVGTemplate','Template')
    FreeCAD.ActiveDocument.Template.Template = templateFileSpec
    FreeCAD.ActiveDocument.Page.Template = FreeCAD.ActiveDocument.Template
    page.Scale = 5.0
#    page.ViewObject.show()   # unit tests run in console mode 

    #make Views
    view1 = FreeCAD.ActiveDocument.addObject('TechDraw::DrawViewPart','View')
    FreeCAD.ActiveDocument.View.Source = [FreeCAD.ActiveDocument.Box]
    rc = page.addView(view1)
    view1.X = 30
    view1.Y = 150
    view2 = FreeCAD.activeDocument().addObject('TechDraw::DrawViewPart','View001')
    FreeCAD.activeDocument().View001.Source = [FreeCAD.activeDocument().Sphere]
    rc = page.addView(view2)
    view2.X = 220
    view2.Y = 150
    FreeCAD.ActiveDocument.recompute()

    #make length dimension
    print("making length dimension")
    dim1 = FreeCAD.ActiveDocument.addObject('TechDraw::DrawViewDimension','Dimension')
    dim1.Type = "Distance"
    objs = list()
    objs.append(view1)
    subObjs = list()
    subObjs.append("Edge1")
    dim1.References2D=[(view1, 'Edge1')]
    print("adding dim1 to page")
    rc = page.addView(dim1)
    print("finished length dimension")

    #make radius dimension
    print("making radius dimension")
    dim2 = FreeCAD.ActiveDocument.addObject('TechDraw::DrawViewDimension','Dimension001')
    dim2.Type = "Radius"
    dim2.MeasureType = "Projected"
    dim2.References2D=[(view2, 'Edge0')]
    rc = page.addView(dim2)

    FreeCAD.ActiveDocument.recompute()

    rc = False
    if ("Up-to-date" in dim1.State) and ("Up-to-date" in dim2.State):
        rc = True
    FreeCAD.closeDocument("TDDim")
    return rc

if __name__ == '__main__':
    DVDimensionTest()
