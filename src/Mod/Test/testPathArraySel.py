#!/usr/bin/env python
# -*- coding: utf-8 -*-

# tester for Draft makePathArray - shapes on a path - with selected subobjects
# Usage: in FC gui, select a "shape" document object (sphere, box, etc) (!select in
# tree, not document view!!), then select edges from the "wire" object.

import FreeCAD
import FreeCADGui
import Part
import Draft

print("testPathArray started")
items = 4                                                  # count
centretrans = FreeCAD.Vector(0,0,0)                        # translation
#centretrans = FreeCAD.Vector(10,10,10)                     # translation
orient = True                                              # align to curve
#orient = False                                             # don't align to curve

# use this to test w/ path subelements
s = FreeCADGui.Selection.getSelectionEx()
for o in s:
   print("Selection: ", o.ObjectName)
   for name in o.SubElementNames:
      print("   name: ", name)
   for obj in o.SubObjects:
      print("   object: ",obj)

print("testPathArray: Objects in selection: ", len(s))
base = s[0].Object
path = s[1].Object
pathsubs = list(s[1].SubElementNames)
print("testPathArray: pathsubs: ", pathsubs)

#o = Draft.makePathArray(base,path,items)                                        # test with defaults
o = Draft.makePathArray(base,path,items,centretrans,orient,pathsubs)            # test w/o orienting shapes

print("testPathArray ended")
