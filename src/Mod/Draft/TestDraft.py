# Unit test for the Draft module

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
#*   Werner Mayer 2005                                                     *
#***************************************************************************/

import FreeCAD, os, unittest, FreeCADGui, Draft

class DraftTest(unittest.TestCase):
    
    def setUp(self):
        # setting a new document to hold the tests
        if FreeCAD.ActiveDocument:
            if FreeCAD.ActiveDocument.Name != "DraftTest":
                FreeCAD.newDocument("DraftTest")
        else:
            FreeCAD.newDocument("DraftTest")
        FreeCAD.setActiveDocument("DraftTest")
            
    def testPivy(self):
        # first checking if pivy is working
        FreeCAD.Console.PrintLog ('Checking Pivy...\n')
        from pivy import coin
        c = coin.SoCube()
        FreeCADGui.ActiveDocument.ActiveView.getSceneGraph().addChild(c)
        self.failUnless(c,"Pivy is not working properly")

    def testWire(self):
        # testing the Wire tool
        FreeCAD.Console.PrintLog ('Checking Draft Wire...\n')
        Draft.makeWire([FreeCAD.Vector(0,0,0),FreeCAD.Vector(2,0,0),FreeCAD.Vector(2,2,0)])
        self.failUnless(FreeCAD.ActiveDocument.getObject("DWire"),"Draft Wire failed")
        
    def testArc(self):
        # testing the Arc tool
        FreeCAD.Console.PrintLog ('Checking Draft Arc...\n')
        Draft.makeCircle(2, startangle=0, endangle=90)
        self.failUnless(FreeCAD.ActiveDocument.getObject("Arc"),"Draft Arc failed")

    def testDimension(self):
        # testing the Arc tool
        FreeCAD.Console.PrintLog ('Checking Draft Dimension...\n')
        Draft.makeDimension(FreeCAD.Vector(0,0,0),FreeCAD.Vector(2,0,0),FreeCAD.Vector(1,-1,0))
        self.failUnless(FreeCAD.ActiveDocument.getObject("Dimension"),"Draft Dimension failed")

    def tearDown(self):
        #closing doc
        #FreeCAD.closeDocument("DraftTest")
        pass



