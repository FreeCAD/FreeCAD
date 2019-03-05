#!/usr/bin/env python
# -*- coding: utf-8 -*-

# test script for TechDraw module
# creates a Box and a Sphere and makes a Fusions from them
# creates a page
# creates a Projection Group
# adds Front,Left,Top projections to the group
# a template in the source folder
from __future__ import print_function

import FreeCAD
import Part
import Measure
import TechDraw
import os

def DProjGroupTest():
    path = os.path.dirname(os.path.abspath(__file__))
    print ('TDGroup path: ' + path)
    templateFileSpec = path + '/TestTemplate.svg'

    FreeCAD.newDocument("TDGroup")
    FreeCAD.setActiveDocument("TDGroup")
    FreeCAD.ActiveDocument=FreeCAD.getDocument("TDGroup")

    #make Fusion feature
    box = FreeCAD.ActiveDocument.addObject("Part::Box","Box")
    sphere = FreeCAD.ActiveDocument.addObject("Part::Sphere","Sphere")
    fusion = FreeCAD.ActiveDocument.addObject("Part::MultiFuse","Fusion")
    FreeCAD.ActiveDocument.Fusion.Shapes = [box,sphere]

    #make a page
    print("making a page")
    page = FreeCAD.ActiveDocument.addObject('TechDraw::DrawPage','Page')
    FreeCAD.ActiveDocument.addObject('TechDraw::DrawSVGTemplate','Template')
    FreeCAD.ActiveDocument.Template.Template = templateFileSpec
    FreeCAD.ActiveDocument.Page.Template = FreeCAD.ActiveDocument.Template
#    page.ViewObject.show()     #unit tests run in console mode

    #make projection group
    print("making a projection group")
    group = FreeCAD.ActiveDocument.addObject('TechDraw::DrawProjGroup','ProjGroup')
    rc = page.addView(group)
    print("Group created")
    group.Source = [fusion]

    print("adding views")
    frontView = group.addProjection("Front")               ##need an Anchor
    print("added Front")

    #update group
    group.Anchor.Direction = FreeCAD.Vector(0,0,1)
    group.Anchor.RotationVector = FreeCAD.Vector(1,0,0)

    leftView = group.addProjection("Left")
    print("added Left")
    topView = group.addProjection("Top")
    print("added Top")
    rightView = group.addProjection("Right")
    print("added Right")
    rearView = group.addProjection("Rear")
    print("added Rear")
    BottomView = group.addProjection("Bottom")
    print("added Bottom")

    #remove a view from projection group
    #iv = group.removeProjection("Left")
    #print("removed Left")

    ##test getItemByLabel method
    print("testing getItemByLabel")
    label = "Top"
    item = group.getItemByLabel(label)
    print("Item Label: " + label + " Item Name: " + item.Name)

    print("recomputing document")
    FreeCAD.ActiveDocument.recompute()

    for v in group.Views:
        print ("View: " + v.Label + " " + v.TypeId)
        v.autoPosition()

    rc = False
    if ("Up-to-date" in group.State):
        rc = True
    FreeCAD.closeDocument("TDGroup")
    return rc
    
if __name__ == '__main__':
    DProjGroupTest()
