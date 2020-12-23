# Workbench test module  
# (c) 2006 Werner Mayer
#

#***************************************************************************
#*   (c) Werner Mayer <werner.wm.mayer@gmx.de> 2006                        *
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
#*   Werner Mayer 2006                                                     *
#***************************************************************************/

import FreeCAD, FreeCADGui, os, unittest

class WorkbenchTestCase(unittest.TestCase):
    def setUp(self):
        self.Active = FreeCADGui.activeWorkbench()
        FreeCAD.Console.PrintLog(FreeCADGui.activeWorkbench().name())
        
    def testActivate(self):
        wbs=FreeCADGui.listWorkbenches()
        # this gives workbenches a possibility to detect that we're under test environment
        FreeCAD.TestEnvironment = True
        try:
            for i in wbs:
                success = FreeCADGui.activateWorkbench(i)
                FreeCAD.Console.PrintLog("Active: "+FreeCADGui.activeWorkbench().name()+ " Expected: "+i+"\n")
                self.assertTrue(success, "Test on activating workbench {0} failed".format(i))
        except Exception as e:
            del FreeCAD.TestEnvironment
            self.fail("Loading of workbench '{0}' failed: {1}".format(i, e))
        else:
            del FreeCAD.TestEnvironment
        
    def testHandler(self):
        import __main__
        class UnitWorkbench(__main__.Workbench):
            MenuText = "Unittest"
            ToolTip = "Unittest"
            def Initialize(self):
                cmds = ["Test_Test"]
                self.appendToolbar("My Unittest",cmds)
            def GetClassName(self):
                return "Gui::PythonWorkbench"

        FreeCADGui.addWorkbench(UnitWorkbench())
        wbs=FreeCADGui.listWorkbenches()
        self.failUnless("UnitWorkbench" in wbs, "Test on adding workbench handler failed")
        FreeCADGui.activateWorkbench("UnitWorkbench")
        FreeCADGui.updateGui()
        self.failUnless(FreeCADGui.activeWorkbench().name()=="UnitWorkbench", "Test on loading workbench 'Unittest' failed")
        FreeCADGui.removeWorkbench("UnitWorkbench")
        wbs=FreeCADGui.listWorkbenches()
        self.failUnless(not "UnitWorkbench" in wbs, "Test on removing workbench handler failed")

    def tearDown(self):
        FreeCADGui.activateWorkbench(self.Active.name())
        FreeCAD.Console.PrintLog(self.Active.name())
