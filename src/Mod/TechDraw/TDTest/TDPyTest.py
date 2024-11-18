#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# basic test script for TechDraw Py functions migrated from Drawing


import FreeCAD
import Part
import Measure
import TechDraw
import os

def TDPyTest():
    path = os.path.dirname(os.path.abspath(__file__))
    print ('TDPy path: ' + path)

    FreeCAD.newDocument("TDPy")
    FreeCAD.setActiveDocument("TDPy")
    FreeCAD.ActiveDocument=FreeCAD.getDocument("TDPy")

    direction = FreeCAD.Vector(0.0, 1.0, 0.0)
    box = FreeCAD.ActiveDocument.addObject("Part::Box","Box")

    result = TechDraw.project(box.Shape, direction)   #visible hard & smooth, hidden hard & smooth
    print("project result: {0}".format(result))
#    Part.show(result[0])

    result = TechDraw.projectEx(box.Shape, direction)   #visible & hidden hard, smooth, seam, outline, iso
    print("projectEx result: {0}".format(result))
#    Part.show(result[0])

    SVGResult = TechDraw.projectToSVG(box.Shape, direction, "ShowHiddenLines", 0.10)   #SVG string
    print("SVG result: {0}".format(SVGResult))

    result = TechDraw.projectToDXF(box.Shape, direction)   #DXF string
    print("DXF result: {0}".format(result))

    result = TechDraw.removeSvgTags(SVGResult)
    print("remove tags result: {0}".format(result))

if __name__ == '__main__':
    TDPyTest()
