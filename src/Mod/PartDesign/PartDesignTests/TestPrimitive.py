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

from math import pi, sqrt
import unittest

import FreeCAD

class TestPrimitive(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestPrimitive")

    def testPrimitiveBox(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.Box = self.Doc.addObject('PartDesign::AdditiveBox','Box')
        self.Box.Length = 11
        self.Box.Width = 11
        self.Box.Height = 11
        self.Body.addObject(self.Box)
        self.Doc.recompute()
        self.Box001 = self.Doc.addObject('PartDesign::SubtractiveBox','Box001')
        self.Box001.Length = 10
        self.Box001.Width = 10
        self.Box001.Height = 10
        self.Body.addObject(self.Box001)
        self.Doc.recompute()
        self.assertAlmostEqual(self.Box001.Shape.Volume, 11**3-10**3)

    def testPrimitiveCylinder(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.Cylinder = self.Doc.addObject('PartDesign::AdditiveCylinder','Cylinder')
        self.Cylinder.Radius = 11
        self.Cylinder.Height = 10
        self.Cylinder.Angle = 360
        self.Body.addObject(self.Cylinder)
        self.Doc.recompute()
        self.Cylinder001 = self.Doc.addObject('PartDesign::SubtractiveCylinder','Cylinder001')
        self.Cylinder001.Radius = 10
        self.Cylinder001.Height = 10
        self.Cylinder001.Angle = 360
        self.Body.addObject(self.Cylinder001)
        self.Doc.recompute()
        self.assertAlmostEqual(self.Cylinder001.Shape.Volume, pi * 10 * (11**2 - 10**2))

    def testPrimitiveSphere(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.Sphere = self.Doc.addObject('PartDesign::AdditiveSphere','Sphere')
        self.Sphere.Radius = 6
        self.Body.addObject(self.Sphere)
        self.Doc.recompute()
        self.Sphere001 = self.Doc.addObject('PartDesign::SubtractiveSphere','Sphere001')
        self.Sphere001.Radius = 5
        self.Body.addObject(self.Sphere001)
        self.Doc.recompute()
        self.assertAlmostEqual(self.Sphere001.Shape.Volume, 4/3.0 * pi * (6**3 - 5**3))

    def testPrimitiveCone(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.Cone = self.Doc.addObject('PartDesign::AdditiveCone','Cone')
        self.Cone.Radius1 = 0
        self.Cone.Radius2 = 4
        self.Cone.Height = 10
        self.Body.addObject(self.Cone)
        self.Doc.recompute()
        self.Cone001 = self.Doc.addObject('PartDesign::SubtractiveCone','Cone')
        self.Cone001.Radius1 = 0
        self.Cone001.Radius2 = 3
        self.Cone001.Height = 10
        self.Body.addObject(self.Cone001)
        self.Doc.recompute()
        self.assertAlmostEqual(self.Cone001.Shape.Volume, 1/3.0 * pi * 10 * (4**2 - 3**2))

    def testPrimitiveEllipsoid(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.Ellipsoid = self.Doc.addObject('PartDesign::AdditiveEllipsoid','Ellipsoid')
        self.Ellipsoid.Radius1 = 2
        self.Ellipsoid.Radius2 = 4
        self.Body.addObject(self.Ellipsoid)
        self.Doc.recompute()
        self.Ellipsoid001 = self.Doc.addObject('PartDesign::SubtractiveEllipsoid','Ellipsoid001')
        self.Ellipsoid001.Radius1 = 1.5
        self.Ellipsoid001.Radius2 = 3
        self.Body.addObject(self.Ellipsoid001)
        self.Doc.recompute()
        self.assertAlmostEqual(self.Ellipsoid001.Shape.Volume, 4/3.0 * pi * (2*4**2 - 1.5*3**2), places=1)

    def testPrimitiveTorus(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.Torus = self.Doc.addObject('PartDesign::AdditiveTorus','Torus')
        self.Torus.Radius1 = 10
        self.Torus.Radius2 = 4
        self.Body.addObject(self.Torus)
        self.Doc.recompute()
        self.Torus001 = self.Doc.addObject('PartDesign::SubtractiveTorus','Torus001')
        self.Torus001.Radius1 = 10
        self.Torus001.Radius2 = 3
        self.Body.addObject(self.Torus001)
        self.Doc.recompute()
        self.assertAlmostEqual(self.Torus001.Shape.Volume, 2 * pi**2 * 10 * (4**2 - 3**2))

    def testPrimitivePrism(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.Prism = self.Doc.addObject('PartDesign::AdditivePrism','Prism')
        self.Prism.Polygon = 6
        self.Prism.Circumradius = 4
        self.Prism.Height = 10
        self.Body.addObject(self.Prism)
        self.Doc.recompute()
        self.Prism001 = self.Doc.addObject('PartDesign::SubtractivePrism','Prism001')
        self.Prism001.Polygon = 6
        self.Prism001.Circumradius = 3
        self.Prism001.Height = 10
        self.Body.addObject(self.Prism001)
        self.Doc.recompute()
        self.assertAlmostEqual(self.Prism001.Shape.Volume, 3*sqrt(3)/2.0 * 10 * (4**2 - 3**2))

    def testPrimitiveWedge(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.Wedge = self.Doc.addObject('PartDesign::AdditiveWedge','Wedge')
        self.Wedge.X2min = 5
        self.Wedge.X2max = 5
        self.Wedge.Z2min = 0
        self.Wedge.Z2max = 10
        self.Body.addObject(self.Wedge)
        self.Doc.recompute()
        self.Wedge001 = self.Doc.addObject('PartDesign::SubtractiveWedge','Wedge001')
        self.Wedge001.Xmin = 1
        self.Wedge001.Xmax = 9
        self.Wedge001.Ymax = 9
        self.Wedge001.X2min = 5
        self.Wedge001.X2max = 5
        self.Wedge001.Z2min = 0
        self.Wedge001.Z2max = 10
        self.Body.addObject(self.Wedge001)
        self.Doc.recompute()
        self.assertAlmostEqual(self.Wedge001.Shape.Volume, 1/2.0 * (10*10 - 9*8) * 10)

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartDesignTestPrimitive")
        #print ("omit closing document for debugging")

