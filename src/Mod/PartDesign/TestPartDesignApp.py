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

import FreeCAD, FreeCADGui, os, sys, unittest, Sketcher, PartDesign, TestSketcherApp
App = FreeCAD
Gui = FreeCADGui
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
		self.Pad.Profile = self.PadSketch
		self.Doc.recompute()
		self.failUnless(len(self.Pad.Shape.Faces) == 6)
		
	def tearDown(self):
		#closing doc
		FreeCAD.closeDocument("PartDesignTest")
		#print ("omit clos document for debuging")

class PartDesignRevolveTestCases(unittest.TestCase):
	def setUp(self):
		self.Doc = FreeCAD.newDocument("PartDesignTest")

	def testRevolveFace(self):
		self.Body1 = self.Doc.addObject('PartDesign::Body','Body')
		self.Box1 = self.Doc.addObject('PartDesign::AdditiveBox','Box')
		self.Body1.addObject(self.Box1)
		self.Box1.Length=10.00
		self.Box1.Width=10.00
		self.Box1.Height=10.00
		self.Doc.recompute()
		self.Revolution = self.Doc.addObject("PartDesign::Revolution","Revolution")
		self.Revolution.Profile = (self.Box1, ["Face6"])
	 	self.Revolution.ReferenceAxis = (self.Doc.Y_Axis,[""])
		self.Revolution.Angle = 180.0
		self.Revolution.Reversed = 1
		self.Body1.addObject(self.Revolution)
		self.Doc.recompute()
		self.failUnless(len(self.Revolution.Shape.Faces) == 10)
		
	def testGrooveFace(self):
		self.Body2 = self.Doc.addObject('PartDesign::Body','Body')
		self.Box2 = self.Doc.addObject('PartDesign::AdditiveBox','Box')
		self.Body2.addObject(self.Box2)
		self.Box2.Length=10.00
		self.Box2.Width=10.00
		self.Box2.Height=10.00
		self.Doc.recompute()
		self.Groove = self.Doc.addObject("PartDesign::Groove","Groove")
		self.Groove.Profile = (self.Box2, ["Face6"])
	 	self.Groove.ReferenceAxis = (self.Doc.X_Axis,[""])
		self.Groove.Angle = 180.0
		self.Groove.Reversed = 1
		self.Body2.addObject(self.Groove)
		self.Doc.recompute()
		self.failUnless(len(self.Groove.Shape.Faces) == 5)
		
	def tearDown(self):
		#closing doc
		# FreeCAD.closeDocument("PartDesignTest")
		print ("omit closing document for debugging")