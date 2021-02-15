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

import FreeCAD, unittest, Part
import copy 
from FreeCAD import Units
App = FreeCAD

from parttests.regression_tests import RegressionTests

#---------------------------------------------------------------------------
# define the test cases to test the FreeCAD Part module
#---------------------------------------------------------------------------


class PartTestCases(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartTest")

    def testBoxCase(self):
        self.Box = self.Doc.addObject("Part::Box","Box")
        self.Doc.recompute()
        self.failUnless(len(self.Box.Shape.Faces)==6)

    def testIssue2985(self):
        v1 = App.Vector(0.0,0.0,0.0)
        v2 = App.Vector(10.0,0.0,0.0)
        v3 = App.Vector(10.0,0.0,10.0)
        v4 = App.Vector(0.0,0.0,10.0)
        edge1 = Part.makeLine(v1, v2)
        edge2 = Part.makeLine(v2, v3)
        edge3 = Part.makeLine(v3, v4)
        edge4 = Part.makeLine(v4, v1)
        # Travis build confirms the crash under macOS
        #result = Part.makeFilledFace([edge1,edge2,edge3,edge4])
        #self.Doc.addObject("Part::Feature","Face").Shape = result
        #self.assertTrue(isinstance(result.Surface, Part.BSplineSurface))

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartTest")
        #print ("omit closing document for debugging")

class PartTestBSplineCurve(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartTest")

        poles = [[0, 0, 0], [1, 1, 0], [2, 0, 0]]
        self.spline = Part.BSplineCurve()
        self.spline.buildFromPoles(poles)

        poles = [[0, 0, 0], [1, 1, 0], [2, 0, 0], [1, -1, 0]]
        self.nurbs = Part.BSplineCurve()
        self.nurbs.buildFromPolesMultsKnots(poles, (3, 1, 3),(0, 0.5, 1), False, 2)

    def testProperties(self):
        self.assertEqual(self.spline.Continuity, 'CN')
        self.assertEqual(self.spline.Degree, 2)
        self.assertEqual(self.spline.EndPoint, App.Vector(2, 0, 0))
        self.assertEqual(self.spline.FirstParameter, 0.0)
        self.assertEqual(self.spline.FirstUKnotIndex, 1)
        self.assertEqual(self.spline.KnotSequence, [0.0, 0.0, 0.0, 1.0, 1.0, 1.0])
        self.assertEqual(self.spline.LastParameter, 1.0)
        self.assertEqual(self.spline.LastUKnotIndex, 2)
        max_degree = self.spline.MaxDegree
        self.assertEqual(self.spline.NbKnots, 2)
        self.assertEqual(self.spline.NbPoles, 3)
        self.assertEqual(self.spline.StartPoint, App.Vector(0.0, 0.0, 0.0))

    def testGetters(self):
        '''only check if the function doesn't crash'''
        self.spline.getKnot(1)
        self.spline.getKnots()
        self.spline.getMultiplicities()
        self.spline.getMultiplicity(1)
        self.spline.getPole(1)
        self.spline.getPoles()
        self.spline.getPolesAndWeights()
        self.spline.getResolution(0.5)
        self.spline.getWeight(1)
        self.spline.getWeights()

    def testSetters(self):
        spline = copy.copy(self.spline)
        spline.setKnot(1, 0.1)
        spline.setPeriodic()
        spline.setNotPeriodic()
        # spline.setKnots()
        # spline.setOrigin(2)   # not working?
        self.spline.setPole(1, App.Vector([1, 0, 0])) # first parameter 0 gives occ error

    def testIssue2671(self):
        self.Doc = App.newDocument("Issue2671")
        Box = self.Doc.addObject("Part::Box","Box")
        Mirroring = self.Doc.addObject("Part::Mirroring", 'Mirroring')
        Spreadsheet = self.Doc.addObject('Spreadsheet::Sheet', 'Spreadsheet')
        Mirroring.Source = Box
        Mirroring.Base = (8, 5, 25)
        Mirroring.Normal = (0.5, 0.2, 0.9)
        Spreadsheet.set('A1', '=Mirroring.Base.x')
        Spreadsheet.set('B1', '=Mirroring.Base.y')
        Spreadsheet.set('C1', '=Mirroring.Base.z')
        Spreadsheet.set('A2', '=Mirroring.Normal.x')
        Spreadsheet.set('B2', '=Mirroring.Normal.y')
        Spreadsheet.set('C2', '=Mirroring.Normal.z')
        self.Doc.recompute()
        self.assertEqual(Spreadsheet.A1, Units.Quantity('8 mm'))
        self.assertEqual(Spreadsheet.B1, Units.Quantity('5 mm'))
        self.assertEqual(Spreadsheet.C1, Units.Quantity('25 mm'))
        self.assertEqual(Spreadsheet.A2, Units.Quantity('0.5 mm'))
        self.assertEqual(Spreadsheet.B2, Units.Quantity('0.2 mm'))
        self.assertEqual(Spreadsheet.C2, Units.Quantity('0.9 mm'))
        App.closeDocument("Issue2671")

    def testIssue2876(self):
        self.Doc = App.newDocument("Issue2876")
        Cylinder = self.Doc.addObject("Part::Cylinder", "Cylinder")
        Cylinder.Radius = 5
        Pipe = self.Doc.addObject("Part::Thickness", "Pipe")
        Pipe.Faces = (Cylinder, ["Face2", "Face3"])
        Pipe.Mode = 1
        Pipe.Value = -1 # negative wall thickness
        Spreadsheet = self.Doc.addObject('Spreadsheet::Sheet', 'Spreadsheet')
        Spreadsheet.set('A1', 'Pipe OD')
        Spreadsheet.set('B1', 'Pipe WT')
        Spreadsheet.set('C1', 'Pipe ID')
        Spreadsheet.set('A2', '=2*Cylinder.Radius')
        Spreadsheet.set('B2', '=-Pipe.Value')
        Spreadsheet.set('C2', '=2*(Cylinder.Radius + Pipe.Value)')
        self.Doc.recompute()
        self.assertEqual(Spreadsheet.B2, Units.Quantity('1 mm'))
        self.assertEqual(Spreadsheet.C2, Units.Quantity('8 mm'))
        App.closeDocument("Issue2876")

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartTest")
