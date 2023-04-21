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

class TestDatumPoint(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestDatumPoint")

    def testOriginDatumPoint(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.DatumPoint = self.Doc.addObject('PartDesign::Point','DatumPoint')
        self.DatumPoint.Support = [(self.Doc.XY_Plane,'')]
        self.DatumPoint.MapMode = 'ObjectOrigin'
        self.Body.addObject(self.DatumPoint)
        self.Doc.recompute()
        self.assertEqual(self.DatumPoint.AttachmentOffset.Base, App.Vector(0))

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartDesignTestDatumPoint")
        #print ("omit closing document for debugging")

class TestDatumLine(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestDatumLine")

    def testXAxisDatumLine(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.DatumLine = self.Doc.addObject('PartDesign::Line','DatumLine')
        self.DatumLine.Support = [(self.Doc.XY_Plane,'')]
        self.DatumLine.MapMode = 'ObjectX'
        self.Body.addObject(self.DatumLine)
        self.Doc.recompute()
        self.assertNotIn('Invalid', self.DatumLine.State)

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartDesignTestDatumLine")
        #print ("omit closing document for debugging")

class TestDatumPlane(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("PartDesignTestDatumPlane")

    def testXYDatumPlane(self):
        self.Body = self.Doc.addObject('PartDesign::Body','Body')
        self.DatumPlane = self.Doc.addObject('PartDesign::Plane','DatumPlane')
        self.DatumPlane.Support = [(self.Doc.XY_Plane,'')]
        self.DatumPlane.MapMode = 'FlatFace'
        self.Body.addObject(self.DatumPlane)
        self.Doc.recompute()
        self.DatumPlaneNormal = self.DatumPlane.Shape.Surface.Axis
        self.assertEqual(abs(self.DatumPlaneNormal.dot(App.Vector(0,0,1))), 1)

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartDesignTestDatumPlane")
        #print ("omit closing document for debugging")

