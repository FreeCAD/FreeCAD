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

import FreeCAD, os, sys, unittest, Sketcher, PartDesign, TestSketcherApp
App = FreeCAD

#---------------------------------------------------------------------------
# define the test cases to test the FreeCAD Sketcher module
#---------------------------------------------------------------------------


class PartDesignPadTestCases(unittest.TestCase):
	def setUp(self):
		self.Doc = FreeCAD.newDocument("PartDesignTest")

	def testBoxCase(self):
		self.PadSketch = self.Doc.addObject('Sketcher::SketchObject','SketchPad')
		TestSketcherApp.CreateSlotPlateSet(self.PadSketch)
		self.Doc.recompute()
		self.Pad = self.Doc.addObject("PartDesign::Pad","Pad")
		self.Pad.Sketch = self.PadSketch
		self.Doc.recompute()
		self.failUnless(len(self.Pad.Shape.Faces) == 6)
		
	def tearDown(self):
		#closing doc
		FreeCAD.closeDocument("PartDesignTest")
		#print ("omit clos document for debuging")
