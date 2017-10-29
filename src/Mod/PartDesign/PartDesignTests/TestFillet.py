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
from __future__ import division
from math import pi
import unittest

import FreeCAD

class TestFillet(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestFillet")

    def testFilletCubeToSphere(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.Box = self.Doc.addObject('PartDesign::AdditiveBox','Box')
        self.Body.addObject(self.Box)
        self.Box.Length=10.00
        self.Box.Width=10.00
        self.Box.Height=10.00
        self.Doc.recompute()
        self.Fillet = self.Doc.addObject("PartDesign::Fillet","Fillet")
        self.Fillet.Base = (self.Box, ['Face'+str(i+1) for i in range(6)])
        self.Fillet.Radius = 4.999999
        self.Body.addObject(self.Fillet)
        self.Doc.recompute()
        self.assertAlmostEqual(self.Fillet.Shape.Volume, 4/3 * pi * 5**3, places=3)

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartDesignTestFillet")
        # print ("omit closing document for debugging")

