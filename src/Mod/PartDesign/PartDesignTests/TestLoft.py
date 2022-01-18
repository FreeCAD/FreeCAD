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

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("PartDesignTestLoft")
        #print ("omit closing document for debugging")

