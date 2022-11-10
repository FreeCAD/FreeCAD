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

App = FreeCAD

class TestBoolean(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestBoolean")

    def testBooleanFuseCase(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.Box = self.Doc.addObject('PartDesign::AdditiveBox','Box')
        self.Box.Length=10
        self.Box.Width=10
        self.Box.Height=10
        self.Body.addObject(self.Box)
        self.Doc.recompute()
        self.Body001 = self.Doc.addObject('PartDesign::Body','Body001')
        self.Box001 = self.Doc.addObject('PartDesign::AdditiveBox','Box001')
        self.Box001.Length=10
        self.Box001.Width=10
        self.Box001.Height=10
        self.Box001.Placement.Base = App.Vector(-5,0,0)
        self.Body001.addObject(self.Box001)
        self.Doc.recompute()
        self.BooleanFuse = self.Doc.addObject('PartDesign::Boolean','BooleanFuse')
        self.Body001.addObject(self.BooleanFuse)
        self.Doc.recompute()
        self.BooleanFuse.setObjects([self.Body,])
        self.BooleanFuse.Type = 0
        self.Doc.recompute()
        self.assertAlmostEqual(self.BooleanFuse.Shape.Volume, 1500)

    def testBooleanCutCase(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.Box = self.Doc.addObject('PartDesign::AdditiveBox','Box')
        self.Box.Length=10
        self.Box.Width=10
        self.Box.Height=10
        self.Body.addObject(self.Box)
        self.Doc.recompute()
        self.Body001 = self.Doc.addObject('PartDesign::Body','Body001')
        self.Box001 = self.Doc.addObject('PartDesign::AdditiveBox','Box001')
        self.Box001.Length=10
        self.Box001.Width=10
        self.Box001.Height=10
        self.Box001.Placement.Base = App.Vector(-5,0,0)
        self.Body001.addObject(self.Box001)
        self.Doc.recompute()
        self.BooleanCut = self.Doc.addObject('PartDesign::Boolean','BooleanCut')
        self.Body001.addObject(self.BooleanCut)
        self.Doc.recompute()
        self.BooleanCut.setObjects([self.Body,])
        self.BooleanCut.Type = 1
        self.Doc.recompute()
        self.assertAlmostEqual(self.BooleanCut.Shape.Volume, 500)

    def testBooleanCommonCase(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.Box = self.Doc.addObject('PartDesign::AdditiveBox','Box')
        self.Box.Length=10
        self.Box.Width=10
        self.Box.Height=10
        self.Body.addObject(self.Box)
        self.Doc.recompute()
        self.Body001 = self.Doc.addObject('PartDesign::Body','Body001')
        self.Box001 = self.Doc.addObject('PartDesign::AdditiveBox','Box001')
        self.Box001.Length=10
        self.Box001.Width=10
        self.Box001.Height=10
        self.Box001.Placement.Base = App.Vector(-5,0,0)
        self.Body001.addObject(self.Box001)
        self.Doc.recompute()
        self.BooleanCommon = self.Doc.addObject('PartDesign::Boolean','BooleanCommon')
        self.Body001.addObject(self.BooleanCommon)
        self.Doc.recompute()
        self.BooleanCommon.setObjects([self.Body,])
        self.BooleanCommon.Type = 2
        self.Doc.recompute()
        self.assertAlmostEqual(self.BooleanCommon.Shape.Volume, 500)

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartDesignTestBoolean")
        #print ("omit closing document for debugging")

