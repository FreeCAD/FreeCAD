#***************************************************************************
#*   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

import unittest

import FreeCAD
from FreeCAD import Base
from FreeCAD import Units
import Part
import Sketcher
import TestSketcherApp

class TestLoft(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestLoft")

    def testSimpleAdditiveLoftCase(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.ProfileSketch = self.Doc.addObject('Sketcher::SketchObject', 'ProfileSketch')
        self.Body.addObject(self.ProfileSketch)
        TestSketcherApp.CreateRectangleSketch(self.ProfileSketch, (0, 0), (1, 1))
        self.Doc.recompute()
        self.LoftSketch = self.Doc.addObject('Sketcher::SketchObject', 'LoftSketch')
        self.Body.addObject(self.LoftSketch)
        self.LoftSketch.MapMode = 'FlatFace'
        self.LoftSketch.Support = (self.Doc.XZ_Plane, [''])
        self.Doc.recompute()
        TestSketcherApp.CreateRectangleSketch(self.LoftSketch, (0, 1), (1, 1))
        self.Doc.recompute()
        self.AdditiveLoft = self.Doc.addObject("PartDesign::AdditiveLoft","AdditiveLoft")
        self.Body.addObject(self.AdditiveLoft)
        self.AdditiveLoft.Profile = self.ProfileSketch
        self.AdditiveLoft.Sections = [self.LoftSketch]
        self.Doc.recompute()
        self.assertAlmostEqual(self.AdditiveLoft.Shape.Volume, 1)

    def testSimpleSubtractiveLoftCase(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject', 'SketchPad')
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (0, 0), (1, 1))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 2
        self.Doc.recompute()
        self.ProfileSketch = self.Doc.addObject('Sketcher::SketchObject', 'ProfileSketch')
        self.Body.addObject(self.ProfileSketch)
        TestSketcherApp.CreateRectangleSketch(self.ProfileSketch, (0, 0), (1, 1))
        self.Doc.recompute()
        self.LoftSketch = self.Doc.addObject('Sketcher::SketchObject', 'LoftSketch')
        self.Body.addObject(self.LoftSketch)
        self.LoftSketch.MapMode = 'FlatFace'
        self.LoftSketch.Support = (self.Doc.XZ_Plane, [''])
        self.Doc.recompute()
        TestSketcherApp.CreateRectangleSketch(self.LoftSketch, (0, 1), (1, 1))
        self.Doc.recompute()
        self.SubtractiveLoft = self.Doc.addObject("PartDesign::SubtractiveLoft","SubtractiveLoft")
        self.Body.addObject(self.SubtractiveLoft)
        self.SubtractiveLoft.Profile = self.ProfileSketch
        self.SubtractiveLoft.Sections = [self.LoftSketch]
        self.Doc.recompute()
        self.assertAlmostEqual(self.SubtractiveLoft.Shape.Volume, 1)

    def testClosedAdditiveLoftCase(self):
        """ Test issue #6156: Loft tool "Closed" option not working """
        body = self.Doc.addObject('PartDesign::Body','Body')

        sketch1 = body.newObject('Sketcher::SketchObject','Sketch')
        sketch1.Support = (self.Doc.XZ_Plane,[''])
        sketch1.MapMode = 'FlatFace'
        sketch1.addGeometry(Part.Circle(Base.Vector(-40.0,0.0,0.0),Base.Vector(0,0,1),10.0), False)
        sketch1.addConstraint(Sketcher.Constraint('PointOnObject',0,3,-1))
        sketch1.addConstraint(Sketcher.Constraint('Diameter',0,20.0))
        sketch1.setDatum(1,Units.Quantity('20.000000 mm'))
        sketch1.addConstraint(Sketcher.Constraint('Distance',-1,1,0,3,40.0))
        sketch1.setDatum(2,Units.Quantity('40.000000 mm'))

        sketch2 = body.newObject('Sketcher::SketchObject','Sketch001')
        sketch2.Support = (self.Doc.YZ_Plane,'')
        sketch2.MapMode = 'FlatFace'
        sketch2.addGeometry(Part.Circle(Base.Vector(-10.0,0.0,0.0),Base.Vector(0,0,1),10.0),False)
        sketch2.addConstraint(Sketcher.Constraint('PointOnObject',0,3,-1))
        sketch2.addConstraint(Sketcher.Constraint('Diameter',0,20.0))
        sketch2.setDatum(1,Units.Quantity('20.000000 mm'))
        sketch2.addConstraint(Sketcher.Constraint('Distance',-1,1,0,3,40.0))
        sketch2.setDatum(2,Units.Quantity('40.000000 mm'))

        sketch3 = body.newObject('Sketcher::SketchObject','Sketch002')
        sketch3.Support = (self.Doc.getObject('YZ_Plane'),'')
        sketch3.MapMode = 'FlatFace'
        sketch3.addGeometry(Part.Circle(Base.Vector(40.0,0.0,0.0),Base.Vector(0,0,1),10.0),False)
        sketch3.addConstraint(Sketcher.Constraint('PointOnObject',0,3,-1))
        sketch3.addConstraint(Sketcher.Constraint('Distance',-1,1,0,3,40.0))
        sketch3.setDatum(1,Units.Quantity('40.000000 mm'))
        sketch3.addConstraint(Sketcher.Constraint('Diameter',0,20.0))
        sketch3.setDatum(2,Units.Quantity('20.000000 mm'))

        sketch4 = body.newObject('Sketcher::SketchObject','Sketch003')
        sketch4.Support = (self.Doc.XZ_Plane,'')
        sketch4.MapMode = 'FlatFace'
        sketch4.addGeometry(Part.Circle(Base.Vector(40.0,0.0,0.0),Base.Vector(0,0,1),10.0),False)
        sketch4.addConstraint(Sketcher.Constraint('PointOnObject',0,3,-1))
        sketch4.addConstraint(Sketcher.Constraint('Distance',-1,1,0,3,40.0))
        sketch4.setDatum(1,Units.Quantity('40.000000 mm'))
        sketch4.addConstraint(Sketcher.Constraint('Diameter',0,20.0))
        sketch4.setDatum(2,Units.Quantity('20.000000 mm'))

        self.Doc.recompute()

        loft = body.newObject('PartDesign::AdditiveLoft','AdditiveLoft')
        loft.Profile = sketch1
        loft.Sections = [sketch2, sketch4, sketch3]
        loft.Closed = True

        sketch1.Visibility = False
        sketch2.Visibility = False
        sketch3.Visibility = False
        sketch4.Visibility = False

        self.Doc.recompute()

        self.assertGreater(loft.Shape.Volume, 80000.0) # 85105.5788704151

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartDesignTestLoft")
        #print ("omit closing document for debugging")

