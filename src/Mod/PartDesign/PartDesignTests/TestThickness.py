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

class TestThickness(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestThickness")

    def testReversedThickness(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.Box = self.Doc.addObject('PartDesign::AdditiveBox','Box')
        self.Body.addObject(self.Box)
        self.Box.Length=10.00
        self.Box.Width=10.00
        self.Box.Height=10.00
        self.Doc.recompute()
        self.Thickness = self.Doc.addObject("PartDesign::Thickness", "Thickness")
        self.Thickness.Base = (self.Box, ["Face1"])
        self.Body.addObject(self.Thickness)
        self.Doc.recompute()
        self.Thickness.Value = 1.0
        self.Thickness.Reversed = 1
        self.Thickness.Mode = 0
        self.Thickness.Join = 0
        self.Thickness.Base = (self.Box, ["Face1"])
        self.Doc.recompute()
        self.assertEqual(len(self.Thickness.Shape.Faces), 11)

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartDesignTestThickness")
        # print ("omit closing document for debugging")

