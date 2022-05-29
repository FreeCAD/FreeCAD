#   (c) Juergen Riegel (FreeCAD@juergen-riegel.net) 2011      LGPL        *
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

import FreeCAD, FreeCADGui, os, sys, unittest, Part, PartGui


#---------------------------------------------------------------------------
# define the test cases to test the FreeCAD Part module
#---------------------------------------------------------------------------


#class PartGuiTestCases(unittest.TestCase):
#	def setUp(self):
#		self.Doc = FreeCAD.newDocument("PartGuiTest")
#
#	def testBoxCase(self):
#		self.Box = self.Doc.addObject('Part::SketchObject','SketchBox')
#		self.Box.addGeometry(Part.LineSegment(App.Vector(-99.230339,36.960674,0),App.Vector(69.432587,36.960674,0)))
#		self.Box.addGeometry(Part.LineSegment(App.Vector(69.432587,36.960674,0),App.Vector(69.432587,-53.196629,0)))
#		self.Box.addGeometry(Part.LineSegment(App.Vector(69.432587,-53.196629,0),App.Vector(-99.230339,-53.196629,0)))
#		self.Box.addGeometry(Part.LineSegment(App.Vector(-99.230339,-53.196629,0),App.Vector(-99.230339,36.960674,0)))
#		
#	def tearDown(self):
#		#closing doc
#		FreeCAD.closeDocument("PartGuiTest")
class PartGuiViewProviderTestCases(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartGuiTest")

    def testCanDropObject(self):
        # https://github.com/FreeCAD/FreeCAD/pull/6850
        box = self.Doc.addObject("Part::Box", "Box")
        with self.assertRaises(TypeError):
            box.ViewObject.canDragObject(None)
        with self.assertRaises(TypeError):
            box.ViewObject.canDropObject(None)
        box.ViewObject.canDropObject()
        with self.assertRaises(TypeError):
            box.ViewObject.dropObject(box, None)

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartGuiTest")
