#   Copyright (c) 2015 WandererFan <wandererfan@gmail.com>                *
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
import Measure
import TechDraw
import time
App = FreeCAD

#---------------------------------------------------------------------------
# define the test cases to test the FreeCAD TechDraw module
#---------------------------------------------------------------------------


class TechDrawTestCases(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("TechDrawTest")

    def testPageCase(self):
        self.templateFileSpec = App.getResourceDir() + 'Mod/TechDraw/Templates/A4_LandscapeTD.svg'
        self.Box = self.Doc.addObject("Part::Box","Box")
        self.Page = self.Doc.addObject('TechDraw::DrawPage','Page')
        self.Template = self.Doc.addObject('TechDraw::DrawSVGTemplate','Template')
        self.Template.Template = self.templateFileSpec
        self.Page.Template = self.Template
        self.View = self.Doc.addObject('TechDraw::DrawViewPart','View')
        rc = self.Page.addView(self.View)
        self.View.Source = self.Box
        self.Anno = self.Doc.addObject('TechDraw::DrawViewAnnotation','TestAnno')
        rc = self.Page.addView(self.Anno)
        self.Sect = self.Doc.addObject('TechDraw::DrawViewSection','Section')
        self.Sect.Source = self.Box
        self.Sect.Direction = FreeCAD.Vector(-1.0,0.0,0.0)
        self.Sect.BaseView = self.View
        self.Sect.SectionDirection = "Right"
        self.Sect.SectionOrigin = FreeCAD.Vector(1.0,1.0,1.0)
        self.Sect.SectionNormal = FreeCAD.Vector(-1.0,0.0,0.0)
        rc = self.Page.addView(self.Sect)
        self.Doc.recompute()
        self.failUnless(len(self.Page.Views) == 3)

    def tearDown(self):
        #closing doc
        FreeCAD.closeDocument("TechDrawTest")
        #print ("omit closing document for debugging")
