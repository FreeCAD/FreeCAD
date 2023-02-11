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
import Part
import Sketcher
import TestSketcherApp

App = FreeCAD

class TestPipe(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestPipe")

    def testSimpleAdditivePipeCase(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.ProfileSketch = self.Doc.addObject('Sketcher::SketchObject', 'ProfileSketch')
        self.Body.addObject(self.ProfileSketch)
        TestSketcherApp.CreateCircleSketch(self.ProfileSketch, (0, 0), 1)
        self.Doc.recompute()
        self.SpineSketch = self.Doc.addObject('Sketcher::SketchObject', 'SpineSketch')
        self.Body.addObject(self.SpineSketch)
        self.SpineSketch.MapMode = 'FlatFace'
        self.SpineSketch.Support = (self.Doc.XZ_Plane, [''])
        self.Doc.recompute()
        self.SpineSketch.addGeometry(Part.LineSegment(App.Vector(0.0,0.0,0),App.Vector(0,1,0)),False)
        self.SpineSketch.addConstraint(Sketcher.Constraint('Coincident',0,1,-1,1))
        self.SpineSketch.addConstraint(Sketcher.Constraint('PointOnObject',0,2,-2))
        self.SpineSketch.addConstraint(Sketcher.Constraint('DistanceY',0,1,0,2,1))
        self.Doc.recompute()
        self.AdditivePipe = self.Doc.addObject("PartDesign::AdditivePipe","AdditivePipe")
        self.Body.addObject(self.AdditivePipe)
        self.AdditivePipe.Profile = self.ProfileSketch
        self.AdditivePipe.Spine = self.SpineSketch
        self.Doc.recompute()
        self.assertAlmostEqual(self.AdditivePipe.Shape.Volume, 3.14159265)

    def testSimpleSubtractivePipeCase(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.ProfileSketch = self.Doc.addObject('Sketcher::SketchObject', 'ProfileSketch')
        self.Body.addObject(self.ProfileSketch)
        TestSketcherApp.CreateCircleSketch(self.ProfileSketch, (0, 0), 1)
        self.Doc.recompute()
        self.SpineSketch = self.Doc.addObject('Sketcher::SketchObject', 'SpineSketch')
        self.Body.addObject(self.SpineSketch)
        self.SpineSketch.MapMode = 'FlatFace'
        self.SpineSketch.Support = (self.Doc.XZ_Plane, [''])
        self.Doc.recompute()
        self.SpineSketch.addGeometry(Part.LineSegment(App.Vector(0.0,0.0,0),App.Vector(0,1,0)),False)
        self.SpineSketch.addConstraint(Sketcher.Constraint('Coincident',0,1,-1,1))
        self.SpineSketch.addConstraint(Sketcher.Constraint('PointOnObject',0,2,-2))
        self.SpineSketch.addConstraint(Sketcher.Constraint('DistanceY',0,1,0,2,1))
        self.Doc.recompute()
        self.PadSketch = self.Doc.addObject('Sketcher::SketchObject', 'PadSketch')
        self.Body.addObject(self.PadSketch)
        TestSketcherApp.CreateRectangleSketch(self.PadSketch, (-5, -5), (10, 10))
        self.Doc.recompute()
        self.Pad = self.Doc.addObject("PartDesign::Pad", "Pad")
        self.Body.addObject(self.Pad)
        self.Pad.Profile = self.PadSketch
        self.Pad.Length = 1.0
        self.Doc.recompute()
        self.SubtractivePipe = self.Doc.addObject("PartDesign::SubtractivePipe","SubtractivePipe")
        self.Body.addObject(self.SubtractivePipe)
        self.SubtractivePipe.Profile = self.ProfileSketch
        self.SubtractivePipe.Spine = self.SpineSketch
        self.Doc.recompute()
        self.assertAlmostEqual(self.SubtractivePipe.Shape.Volume, 100 - 3.14159265)

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartDesignTestPipe")
        #print ("omit closing document for debugging")

