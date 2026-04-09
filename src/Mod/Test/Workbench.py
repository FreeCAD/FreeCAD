# ***************************************************************************
# *   Copyright (c) 2006 Werner Mayer <werner.wm.mayer@gmx.de>              *
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
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************/

# Workbench test module

import FreeCAD, FreeCADGui, os, unittest
import tempfile

from PySide import QtWidgets, QtCore
from PySide.QtWidgets import QApplication


class CallableCheckWarning:
    def __call__(self):
        diag = QApplication.activeModalWidget()
        if diag:
            QtCore.QTimer.singleShot(0, diag, QtCore.SLOT("accept()"))


class WorkbenchTestCase(unittest.TestCase):
    def setUp(self):
        self.Active = FreeCADGui.activeWorkbench()
        FreeCAD.Console.PrintLog(FreeCADGui.activeWorkbench().name())

    def testActivate(self):
        wbs = FreeCADGui.listWorkbenches()
        # this gives workbenches a possibility to detect that we're under test environment
        FreeCAD.TestEnvironment = True
        for i in wbs:
            try:
                print("Activate workbench '{}'".format(i))
                cobj = CallableCheckWarning()
                QtCore.QTimer.singleShot(500, cobj)
                if FreeCADGui.activeWorkbench().name() != i:
                    success = FreeCADGui.activateWorkbench(i)
                else:
                    # Cannot test activation of an already-active workbench
                    success = True
                FreeCAD.Console.PrintLog(
                    "Active: " + FreeCADGui.activeWorkbench().name() + " Expected: " + i + "\n"
                )
                self.assertTrue(success, "Test on activating workbench {0} failed".format(i))
            except Exception as e:
                self.fail("Loading of workbench '{0}' failed: {1}".format(i, e))
        del FreeCAD.TestEnvironment

    def testHandler(self):
        import __main__

        class UnitWorkbench(__main__.Workbench):
            MenuText = "Unittest"
            ToolTip = "Unittest"

            def Initialize(self):
                cmds = ["Test_Test"]
                self.appendToolbar("My Unittest", cmds)

            def GetClassName(self):
                return "Gui::PythonWorkbench"

        FreeCADGui.addWorkbench(UnitWorkbench())
        wbs = FreeCADGui.listWorkbenches()
        self.assertTrue("UnitWorkbench" in wbs, "Test on adding workbench handler failed")
        FreeCADGui.activateWorkbench("UnitWorkbench")
        FreeCADGui.updateGui()
        self.assertTrue(
            FreeCADGui.activeWorkbench().name() == "UnitWorkbench",
            "Test on loading workbench 'Unittest' failed",
        )
        FreeCADGui.removeWorkbench("UnitWorkbench")
        wbs = FreeCADGui.listWorkbenches()
        self.assertTrue(not "UnitWorkbench" in wbs, "Test on removing workbench handler failed")

    def testInvalidType(self):
        class MyExtWorkbench(FreeCADGui.Workbench):
            def Initialize(self):
                pass

            def GetClassName(self):
                return "App::Extension"

        FreeCADGui.addWorkbench(MyExtWorkbench())
        with self.assertRaises(TypeError):
            FreeCADGui.activateWorkbench("MyExtWorkbench")
        FreeCADGui.removeWorkbench("MyExtWorkbench")

    def testResetClearsStaleWorkbenchBinding(self):
        import __main__

        class UnitResetWorkbench(__main__.Workbench):
            MenuText = "Reset Unittest"
            ToolTip = "Reset Unittest"

            def Initialize(self):
                pass

            def GetClassName(self):
                return "Gui::PythonWorkbench"

        handler = UnitResetWorkbench()
        FreeCADGui.addWorkbench(handler)
        self.assertFalse(hasattr(handler, "__Workbench__"))
        self.assertTrue(FreeCADGui.activateWorkbench("UnitResetWorkbench"))
        self.assertTrue(hasattr(handler, "__Workbench__"))
        self.assertTrue(FreeCADGui.resetWorkbench("UnitResetWorkbench"))
        self.assertFalse(hasattr(handler, "__Workbench__"))
        self.assertNotIn("UnitResetWorkbench", FreeCADGui.listWorkbenches())

    def testResetAfterPythonCommandReplacementDoesNotCrash(self):
        import __main__

        class UnitReplaceResetWorkbench(__main__.Workbench):
            MenuText = "Replace Reset Unittest"
            ToolTip = "Replace Reset Unittest"

            def Initialize(self):
                pass

            def GetClassName(self):
                return "Gui::PythonWorkbench"

        class WorkingCommand:
            def __init__(self, label):
                self.label = label

            def GetResources(self):
                return {"MenuText": self.label}

            def Activated(self):
                pass

        command_name = "Test_CommandReplacementReset"
        handler = UnitReplaceResetWorkbench()
        FreeCADGui.addWorkbench(handler)
        self.assertTrue(FreeCADGui.activateWorkbench("UnitReplaceResetWorkbench"))
        FreeCADGui.addCommand(command_name, WorkingCommand("Initial label"))
        FreeCADGui.addCommand(command_name, WorkingCommand("Updated label"))
        self.assertTrue(FreeCADGui.resetWorkbench("UnitReplaceResetWorkbench"))
        self.assertFalse(hasattr(handler, "__Workbench__"))
        self.assertNotIn("UnitReplaceResetWorkbench", FreeCADGui.listWorkbenches())

    def testDisposedWorkbenchRuntimeRejectsMutation(self):
        import __main__

        class UnitRuntimeWorkbench(__main__.Workbench):
            MenuText = "Runtime Unittest"
            ToolTip = "Runtime Unittest"

            def Initialize(self):
                pass

            def GetClassName(self):
                return "Gui::PythonWorkbench"

        handler = UnitRuntimeWorkbench()
        session_name = "reload_test"
        FreeCADGui.addWorkbench(handler)
        self.assertTrue(FreeCADGui.activateWorkbench("UnitRuntimeWorkbench"))

        runtime = FreeCADGui.workbenchRuntime("UnitRuntimeWorkbench")
        session = FreeCADGui.sessionRuntime(session_name)
        self.assertEqual(session.workbench_name, "UnitRuntimeWorkbench")
        self.assertIs(session, FreeCADGui.findSessionRuntime(session_name))
        self.assertIs(
            session,
            FreeCADGui.findSessionRuntime("UnitRuntimeWorkbench:reload_test"),
        )

        self.assertTrue(FreeCADGui.disposeSessionRuntime(session_name))
        self.assertTrue(FreeCADGui.disposeWorkbenchRuntime("UnitRuntimeWorkbench"))

        with self.assertRaises(RuntimeError):
            session.own("session_key", object())
        with self.assertRaises(RuntimeError):
            runtime.own("workbench_key", object())

        self.assertTrue(FreeCADGui.resetWorkbench("UnitRuntimeWorkbench"))

    def testDisposedAppRuntimeRejectsMutation(self):
        runtime_name = "Test_DisposedAppRuntime"
        runtime = FreeCAD.appRuntime(runtime_name)
        self.assertTrue(FreeCAD.disposeAppRuntime(runtime_name))

        with self.assertRaises(RuntimeError):
            runtime.own("runtime_key", object())
        with self.assertRaises(RuntimeError):
            runtime.onDispose(lambda: None)

    def tearDown(self):
        FreeCADGui.activateWorkbench(self.Active.name())
        FreeCAD.Console.PrintLog(self.Active.name())


class CommandTestCase(unittest.TestCase):
    def testPR6889(self):
        # Fixes a crash
        TempPath = tempfile.gettempdir()
        macroName = TempPath + os.sep + "testmacro.py"
        macroFile = open(macroName, "w")
        macroFile.write("print ('Hello, World!')")
        macroFile.close()

        name = FreeCADGui.Command.createCustomCommand(macroName)
        cmd = FreeCADGui.Command.get(name)
        cmd.run()

    def testPythonCommandReplacementPreservesWorkingCommandOnFailure(self):
        command_name = "Test_CommandReplacementFailure"

        class WorkingCommand:
            def GetResources(self):
                return {"MenuText": "Stable label"}

            def Activated(self):
                pass

        class BrokenCommand:
            def GetResources(self):
                return []

            def Activated(self):
                pass

        FreeCADGui.addCommand(command_name, WorkingCommand())
        initial_info = FreeCADGui.Command.get(command_name).getInfo()
        self.assertEqual(initial_info["menuText"], "Stable label")

        with self.assertRaises(TypeError):
            FreeCADGui.addCommand(command_name, BrokenCommand())

        updated_info = FreeCADGui.Command.get(command_name).getInfo()
        self.assertEqual(updated_info["menuText"], "Stable label")


class TestNavigationStyle(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("CreateTest")

    def testInvalidStyle(self):
        FreeCADGui.getDocument(self.Doc).ActiveView.setNavigationType("App::Extension")
        self.assertNotEqual(
            FreeCADGui.getDocument(self.Doc).ActiveView.getNavigationType(), "App::Extension"
        )

    def tearDown(self):
        FreeCAD.closeDocument("CreateTest")
