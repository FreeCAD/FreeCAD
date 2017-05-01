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

import FreeCAD, os, sys, unittest, Part
import copy 
App = FreeCAD

#---------------------------------------------------------------------------
# define the test cases to test the FreeCAD Part module
#---------------------------------------------------------------------------


class PartTestCases(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartTest")

    def testBoxCase(self):
        self.Box = App.ActiveDocument.addObject("Part::Box","Box")
        self.Doc.recompute()
        self.failUnless(len(self.Box.Shape.Faces)==6)
        
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

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartTest")
