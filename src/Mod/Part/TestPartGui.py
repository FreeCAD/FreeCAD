#**************************************************************************
#   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
#                                                                         *
#   This file is part of the FreeCAD CAx development system.              *
#                                                                         *
#   This program is free software; you can redistribute it and/or modify  *
#   it under the terms of the GNU Lesser General Public License (LGPL)    *
#   as published by the Free Software Foundation; either version 2 of     *
#   the License, or (at your option) any later version.                   *
#   for detail see the LICENCE text file.                                 *
#                                                                         *
#   FreeCAD is distributed in the hope that it will be useful,            *
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#   GNU Library General Public License for more details.                  *
#                                                                         *
#   You should have received a copy of the GNU Library General Public     *
#   License along with FreeCAD; if not, write to the Free Software        *
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#   USA                                                                   *
#**************************************************************************

import os
import sys
import unittest
import FreeCAD
import FreeCADGui
import Part
import PartGui
from PySide import QtWidgets

def findDockWidget(name):
    """ Get a dock widget by name """
    mw = FreeCADGui.getMainWindow()
    dws = mw.findChildren(QtWidgets.QDockWidget)
    for dw in dws:
        if dw.objectName() == name:
            return dw
    return None

"""
#---------------------------------------------------------------------------
# define the test cases to test the FreeCAD Part module
#---------------------------------------------------------------------------
"""
from parttests.ColorPerFaceTest import ColorPerFaceTest


#class PartGuiTestCases(unittest.TestCase):
#    def setUp(self):
#        self.Doc = FreeCAD.newDocument("PartGuiTest")
#
#    def testBoxCase(self):
#        self.Box = self.Doc.addObject('Part::SketchObject','SketchBox')
#        self.Box.addGeometry(Part.LineSegment(App.Vector(-99.230339,36.960674,0),App.Vector(69.432587,36.960674,0)))
#        self.Box.addGeometry(Part.LineSegment(App.Vector(69.432587,36.960674,0),App.Vector(69.432587,-53.196629,0)))
#        self.Box.addGeometry(Part.LineSegment(App.Vector(69.432587,-53.196629,0),App.Vector(-99.230339,-53.196629,0)))
#        self.Box.addGeometry(Part.LineSegment(App.Vector(-99.230339,-53.196629,0),App.Vector(-99.230339,36.960674,0)))
#
#    def tearDown(self):
#        #closing doc
#        FreeCAD.closeDocument("PartGuiTest")
class PartGuiViewProviderTestCases(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartGuiTest")

    def testCanDropObject(self):
        # https://github.com/FreeCAD/FreeCAD/pull/6850
        box = self.Doc.addObject("Part::Box", "Box")
        with self.assertRaises(TypeError):
            box.ViewObject.canDragObject(0)
        with self.assertRaises(TypeError):
            box.ViewObject.canDropObject(0)
        box.ViewObject.canDropObject()
        with self.assertRaises(TypeError):
            box.ViewObject.dropObject(box, 0)

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartGuiTest")

class SectionCutTestCases(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("SectionCut")

    def testOpenDialog(self):
        box = self.Doc.addObject("Part::Box", "SectionCutBoxX")
        comp = self.Doc.addObject("Part::Compound", "SectionCutCompound")
        comp.Links = box
        grp = self.Doc.addObject("App::DocumentObjectGroup", "SectionCutX")
        grp.addObject(comp)
        self.Doc.recompute()

        FreeCADGui.runCommand("Part_SectionCut")
        dw = findDockWidget("Section Cutting")
        if dw:
            box = dw.findChild(QtWidgets.QDialogButtonBox)
            button = box.button(box.Close)
            button.click()
        else:
            print ("No Section Cutting panel found")

    def tearDown(self):
        FreeCAD.closeDocument("SectionCut")
