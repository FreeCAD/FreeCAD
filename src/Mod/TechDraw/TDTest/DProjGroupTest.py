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
import os


def DProjGroupTest():
    path = os.path.dirname(os.path.abspath(__file__))
    print("TDGroup path: " + path)
    templateFileSpec = path + "/TestTemplate.svg"

    FreeCAD.newDocument("TDGroup")
    FreeCAD.setActiveDocument("TDGroup")
    FreeCAD.ActiveDocument = FreeCAD.getDocument("TDGroup")
    doc = FreeCAD.ActiveDocument
    print("document created")

    # make Fusion feature
    box = FreeCAD.ActiveDocument.addObject("Part::Box", "Box")
    box.recompute()
    print("box created")
    sphere = FreeCAD.ActiveDocument.addObject("Part::Sphere", "Sphere")
    sphere.recompute()
    print("sphere created")
    fusion = FreeCAD.ActiveDocument.addObject("Part::MultiFuse", "Fusion")
    FreeCAD.ActiveDocument.Fusion.Shapes = [box, sphere]
    fusion.recompute()
    print("Fusion created")

    # make a page
    print("making a page")
    page = FreeCAD.ActiveDocument.addObject("TechDraw::DrawPage", "Page")
    FreeCAD.ActiveDocument.addObject("TechDraw::DrawSVGTemplate", "Template")
    FreeCAD.ActiveDocument.Template.Template = templateFileSpec
    FreeCAD.ActiveDocument.Page.Template = FreeCAD.ActiveDocument.Template
    # page.ViewObject.show()  # for debugging. unit tests run in console mode
    print("Page created")

    # make projection group
    print("making a projection group")
    doc.openTransaction("Create Proj Group")
    groupName = "ProjGroup"
    group = FreeCAD.ActiveDocument.addObject("TechDraw::DrawProjGroup", groupName)
    page.addView(group)
    print("Group created")
    group.Source = [fusion]

    print("adding views")
    group.addProjection("Front")  # need an Anchor
    print("added Front")

    # update group
    anchorDir = FreeCAD.Vector(0.0, 0.0, 1.0)
    anchorRot = FreeCAD.Vector(1.0, 0.0, 0.0)
    group.Anchor.Direction = anchorDir
    group.Anchor.RotationVector = anchorRot
    print("Anchor values set")
    group.Anchor.recompute()
    doc.commitTransaction()
    print("Front/Anchor recomputed")

    print("adding left")
    group.addProjection("Left")
    print("added Left")
    group.addProjection("Top")
    print("added Top")
    group.addProjection("Right")
    print("added Right")
    group.addProjection("Rear")
    print("added Rear")
    group.addProjection("Bottom")
    print("added Bottom")

    # remove a view from projection group
    group.removeProjection("Left")
    print("removed Left")

    # test getItemByLabel method
    print("testing getItemByLabel")
    label = "Top"
    item = group.getItemByLabel(label)
    print("Item Label: " + label + " Item Name: " + item.Name)

    print("recomputing document")
    FreeCAD.ActiveDocument.recompute()

    for v in group.Views:
        print("View: " + v.Label + " " + v.TypeId)
        v.autoPosition()

    rc = False
    if "Up-to-date" in group.State:
        rc = True

    FreeCAD.closeDocument("TDGroup")
    return rc


if __name__ == "__main__":
    DProjGroupTest()
