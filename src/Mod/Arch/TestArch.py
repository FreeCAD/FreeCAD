# Unit test for the Arch module

#***************************************************************************
#*   (c) Yorik van Havre <yorik@uncreated.net> 2013                        *
#*                                                                         *
#*   This file is part of the FreeCAD CAx development system.              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   FreeCAD is distributed in the hope that it will be useful,            *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with FreeCAD; if not, write to the Free Software        *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************/

import FreeCAD, os, unittest, FreeCADGui, Arch, Draft

class ArchTest(unittest.TestCase):

    def setUp(self):
        # setting a new document to hold the tests
        if FreeCAD.ActiveDocument:
            if FreeCAD.ActiveDocument.Name != "ArchTest":
                FreeCAD.newDocument("ArchTest")
        else:
            FreeCAD.newDocument("ArchTest")
        FreeCAD.setActiveDocument("ArchTest")

    def testWall(self):
        FreeCAD.Console.PrintLog ('Checking Arch Wall...\n')
        l=Draft.makeLine(FreeCAD.Vector(0,0,0),FreeCAD.Vector(-2,0,0))
        w = Arch.makeWall(l)
        self.failUnless(w,"Arch Wall failed")

    def testStructure(self):
        FreeCAD.Console.PrintLog ('Checking Arch Structure...\n')
        s = Arch.makeStructure(length=2,width=3,hright=5)
        self.failUnless(s,"Arch Structure failed")

    def tearDown(self):
        FreeCAD.closeDocument("ArchTest")
        pass

