#!/usr/bin/env python
# -*- coding: utf-8 -*-

# tester for Draft makePathArray - shapes on a path - without subelements (see testPathArraySel.py)
# Usage: in FC gui, select a "shape" document object (sphere, box, etc) (!!select in
# tree, not document view!!), then a "wire" document object (Wire, Circle, Rectangle, 
# DWire, etc) then run this macro.

import FreeCAD
import FreeCADGui
import Part
import Draft

print("testPathArray started")
items = 4                                                  # count
centretrans = FreeCAD.Vector(0,0,0)                        # no translation
#centretrans = FreeCAD.Vector(-5,-5,0)                     # translation
orient = True                                              # align to curve
#orient = False                                             # don't align to curve

s = FreeCADGui.Selection.getSelection()
print("testPathArray: Objects in selection: ", len(s))
print("First object in selection is a: ", s[0].Shape.ShapeType)
print("Second object in selection is a: ", s[1].Shape.ShapeType)
base = s[0]
path = s[1]
pathsubs = []    

#o = Draft.makePathArray(base,path,items)                                       # test with defaults
o = Draft.makePathArray(base,path,items,centretrans,orient,pathsubs)            # test with non-defaults

print("testPathArray ended")
    
    
    
