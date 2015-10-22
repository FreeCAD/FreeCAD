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


import FreeCAD, os, sys, unittest, Part, Sketcher
App = FreeCAD

def CreateBoxSketchSet(SketchFeature):
	SketchFeature.addGeometry(Part.Line(FreeCAD.Vector(-99.230339,36.960674,0),FreeCAD.Vector(69.432587,36.960674,0)))
	SketchFeature.addGeometry(Part.Line(FreeCAD.Vector(69.432587,36.960674,0),FreeCAD.Vector(69.432587,-53.196629,0)))
	SketchFeature.addGeometry(Part.Line(FreeCAD.Vector(69.432587,-53.196629,0),FreeCAD.Vector(-99.230339,-53.196629,0)))
	SketchFeature.addGeometry(Part.Line(FreeCAD.Vector(-99.230339,-53.196629,0),FreeCAD.Vector(-99.230339,36.960674,0)))
	# add the constraints
	SketchFeature.addConstraint(Sketcher.Constraint('Coincident',0,2,1,1)) 
	SketchFeature.addConstraint(Sketcher.Constraint('Coincident',1,2,2,1)) 
	SketchFeature.addConstraint(Sketcher.Constraint('Coincident',2,2,3,1)) 
	SketchFeature.addConstraint(Sketcher.Constraint('Coincident',3,2,0,1)) 
	SketchFeature.addConstraint(Sketcher.Constraint('Horizontal',0)) 
	SketchFeature.addConstraint(Sketcher.Constraint('Horizontal',2)) 
	SketchFeature.addConstraint(Sketcher.Constraint('Vertical',1)) 
	SketchFeature.addConstraint(Sketcher.Constraint('Vertical',3)) 
	# add dimensions
	SketchFeature.addConstraint(Sketcher.Constraint('Distance',1,81.370787)) 
	SketchFeature.addConstraint(Sketcher.Constraint('Distance',0,187.573036)) 

def CreateSlotPlateSet(SketchFeature):
	SketchFeature.addGeometry(Part.Line(App.Vector(60.029362,-30.279360,0),App.Vector(-120.376335,-30.279360,0)))
	SketchFeature.addConstraint(Sketcher.Constraint('Horizontal',0)) 
	SketchFeature.addGeometry(Part.Line(App.Vector(-120.376335,-30.279360,0),App.Vector(-70.193062,38.113884,0)))
	SketchFeature.addConstraint(Sketcher.Constraint('Coincident',0,2,1,1)) 
	SketchFeature.addGeometry(Part.Line(App.Vector(-70.193062,38.113884,0),App.Vector(60.241116,37.478645,0)))
	SketchFeature.addConstraint(Sketcher.Constraint('Coincident',1,2,2,1)) 
	SketchFeature.addConstraint(Sketcher.Constraint('Horizontal',2)) 
	SketchFeature.addGeometry(Part.ArcOfCircle(Part.Circle(App.Vector(60.039921,3.811391,0),App.Vector(0,0,1),35.127132),-1.403763,1.419522))
	SketchFeature.addConstraint(Sketcher.Constraint('Coincident',3,2,2,2)) 
	SketchFeature.addConstraint(Sketcher.Constraint('Coincident',3,1,0,1)) 
	SketchFeature.addConstraint(Sketcher.Constraint('Tangent',3,2)) 
	SketchFeature.addConstraint(Sketcher.Constraint('Tangent',3,0)) 
	SketchFeature.addConstraint(Sketcher.Constraint('Angle',0,2,1,1,0.947837)) 
	SketchFeature.addConstraint(Sketcher.Constraint('Distance',0,184.127425)) 
	SketchFeature.setDatum(9,200.000000)
	SketchFeature.addConstraint(Sketcher.Constraint('Radius',3,38.424808)) 
	SketchFeature.setDatum(10,40.000000)
	SketchFeature.setDatum(8,0.872665)
	SketchFeature.addConstraint(Sketcher.Constraint('DistanceX',0,2,0.0)) 
	SketchFeature.setDatum(11,0.000000)
	SketchFeature.movePoint(0,2,App.Vector(-0.007829,-33.376450,0))
	SketchFeature.movePoint(0,2,App.Vector(-0.738149,-10.493386,0))
	SketchFeature.movePoint(0,2,App.Vector(-0.007829,2.165328,0))
	SketchFeature.addConstraint(Sketcher.Constraint('DistanceY',0,2,2.165328)) 
	SketchFeature.setDatum(12,0.000000)

def CreateSlotPlateInnerSet(SketchFeature):
	SketchFeature.addGeometry(Part.Circle(App.Vector(195.055893,39.562252,0),App.Vector(0,0,1),29.846098))
	SketchFeature.addGeometry(Part.Line(App.Vector(150.319031,13.449363,0),App.Vector(36.700474,13.139774,0)))
	SketchFeature.addConstraint(Sketcher.Constraint('Horizontal',5)) 
	SketchFeature.addGeometry(Part.Line(App.Vector(36.700474,13.139774,0),App.Vector(77.566010,63.292927,0)))
	SketchFeature.addConstraint(Sketcher.Constraint('Coincident',5,2,6,1)) 
	SketchFeature.addGeometry(Part.Line(App.Vector(77.566010,63.292927,0),App.Vector(148.151917,63.602505,0)))
	SketchFeature.addConstraint(Sketcher.Constraint('Coincident',6,2,7,1)) 
	SketchFeature.addConstraint(Sketcher.Constraint('Horizontal',7)) 
	SketchFeature.addConstraint(Sketcher.Constraint('Parallel',1,6)) 
	SketchFeature.addGeometry(Part.ArcOfCircle(Part.Circle(App.Vector(192.422913,38.216347,0),App.Vector(0,0,1),45.315174),2.635158,3.602228))
	SketchFeature.addConstraint(Sketcher.Constraint('Coincident',7,2,8,1)) 
	SketchFeature.addConstraint(Sketcher.Constraint('Coincident',8,2,5,1))
	


#---------------------------------------------------------------------------
# define the test cases to test the FreeCAD Sketcher module
#---------------------------------------------------------------------------


class SketcherSolverTestCases(unittest.TestCase):
	def setUp(self):
		self.Doc = FreeCAD.newDocument("SketchSolverTest")

	def testBoxCase(self):
		self.Box = self.Doc.addObject('Sketcher::SketchObject','SketchBox')
		CreateBoxSketchSet(self.Box)
		self.Doc.recompute()
		# moving a point of the sketch
		self.Box.movePoint(0,2,App.Vector(88.342697,28.174158,0))
		# fully constrain
		self.Box.addConstraint(Sketcher.Constraint('DistanceX',1,2,90.0)) 
		self.Box.addConstraint(Sketcher.Constraint('DistanceY',1,2,-50.0)) 
		self.Doc.recompute()
		
	def testSlotCase(self):
		self.Slot = self.Doc.addObject('Sketcher::SketchObject','SketchSlot')
		CreateSlotPlateSet(self.Slot)
		self.Doc.recompute()
		# test if all edges created 
		self.failUnless(len(self.Slot.Shape.Edges) == 4)
		CreateSlotPlateInnerSet(self.Slot)
		self.Doc.recompute()
		self.failUnless(len(self.Slot.Shape.Edges) == 9)
	
	
	def tearDown(self):
		#closing doc
		FreeCAD.closeDocument("SketchSolverTest")
		#print ("omit close document for debuging")
