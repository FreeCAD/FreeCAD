# ***************************************************************************
# *   Copyright (c) 2002,2003 Juergen Riegel <juergen.riegel@web.de>        *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Lesser General Public License for more details.                   *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************/

# Test gui init module


class TestWorkbench(Workbench):
    "Test workbench object"

    def __init__(self):
        self.__class__.Icon = (
            FreeCAD.getResourceDir() + "Mod/Test/Resources/icons/TestWorkbench.svg"
        )
        self.__class__.MenuText = "Test Framework"
        self.__class__.ToolTip = "Test Framework"

    def Initialize(self):
        import TestGui

        list = ["Test_Test", "Test_TestAll", "Test_TestDoc", "Test_TestBase"]
        self.appendToolbar("TestTools", list)

        menu = ["Test &Commands", "TestToolsGui"]
        list = [
            "Std_TestQM",
            "Std_TestReloadQM",
            "Test_Test",
            "Test_TestAll",
            "Test_TestDoc",
            "Test_TestBase",
        ]
        self.appendCommandbar("TestToolsGui", list)
        self.appendMenu(menu, list)

        menu = ["Test &Commands", "TestToolsText"]
        list = ["Test_TestAllText", "Test_TestDocText", "Test_TestBaseText"]
        self.appendCommandbar("TestToolsText", list)
        self.appendMenu(menu, list)

        menu = ["Test &Commands", "TestToolsMenu"]
        list = ["Test_TestCreateMenu", "Test_TestDeleteMenu", "Test_TestWork"]
        self.appendCommandbar("TestToolsMenu", list)
        self.appendMenu(menu, list)

        menu = ["Test &Commands", "TestFeatureMenu"]
        list = ["Test_InsertFeature"]
        self.appendCommandbar("TestFeature", list)
        self.appendMenu(menu, list)

        menu = ["Test &Commands", "Progress bar"]
        list = [
            "Std_TestProgress1",
            "Std_TestProgress2",
            "Std_TestProgress3",
            "Std_TestProgress4",
            "Std_TestProgress5",
        ]
        self.appendMenu(menu, list)

        menu = ["Test &Commands", "Console"]
        list = ["Std_TestConsoleOutput"]
        self.appendMenu(menu, list)

        menu = ["Test &Commands", "MDI"]
        list = ["Std_MDITest1", "Std_MDITest2", "Std_MDITest3"]
        self.appendMenu(menu, list)

        list = ["Std_ViewExample1", "Std_ViewExample2", "Std_ViewExample3"]
        self.appendMenu("Inventor View", list)


Gui.addWorkbench(TestWorkbench())

# Base system tests
FreeCAD.__unit_test__ += ["Workbench", "Menu", "Menu.MenuDeleteCases", "Menu.MenuCreateCases"]
