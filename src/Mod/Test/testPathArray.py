#!/usr/bin/env python
# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2013 WandererFan <wandererfan@gmail.com>                *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************/

# Tester for Draft makePathArray - shapes on a path - without subelements (see testPathArraySel.py)
# Usage: in FC gui, select a "shape" document object (sphere, box, etc) (!!select in
# tree, not document view!!), then a "wire" document object (Wire, Circle, Rectangle,
# DWire, etc) then run this macro.

import FreeCAD
import FreeCADGui
import Part
import Draft

print("testPathArray started")
items = 4  # count
centretrans = FreeCAD.Vector(0, 0, 0)  # no translation
# centretrans = FreeCAD.Vector(-5,-5,0)                     # translation
orient = True  # align to curve
# orient = False                                            # don't align to curve

s = FreeCADGui.Selection.getSelection()
print("testPathArray: Objects in selection: ", len(s))
print("First object in selection is a: ", s[0].Shape.ShapeType)
print("Second object in selection is a: ", s[1].Shape.ShapeType)
base = s[0]
path = s[1]
pathsubs = []

# o = Draft.makePathArray(base,path,items)                            # test with defaults
o = Draft.makePathArray(base, path, items, centretrans, orient, pathsubs)  # test with non-defaults

print("testPathArray ended")
