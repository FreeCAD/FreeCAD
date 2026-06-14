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


class TestNavigationStyle(unittest.TestCase):
    def setUp(self):
        self.Doc = FreeCAD.newDocument("CreateTest")
        self.View = FreeCADGui.getDocument(self.Doc).ActiveView

    def testInvalidStyle(self):
        self.View.setNavigationType("App::Extension")
        self.assertNotEqual(self.View.getNavigationType(), "App::Extension")

    def testOrientationLockBlocksProgrammaticOrientationChanges(self):
        viewer = self.View.getViewer()
        navigation = viewer.getNavigationStyle()
        initial = self.View.getCameraOrientation()
        target = (0.0, 0.70710678, 0.0, 0.70710678)

        navigation.setOrientationLocked(True)
        self.View.setViewDirection((1.0, 0.0, 0.0))
        self.assertTrue(self.View.getCameraOrientation().isSame(initial, 1e-6))
        self.View.setCameraOrientation(target)
        self.assertTrue(self.View.getCameraOrientation().isSame(initial, 1e-6))

        navigation.setOrientationLocked(False)
        self.View.setCameraOrientation(target)
        self.assertFalse(self.View.getCameraOrientation().isSame(initial, 1e-6))

    def testOrientationLockAllowsTranslationOnlyViewPosition(self):
        navigation = self.View.getViewer().getNavigationStyle()
        initial = self.View.viewPosition()
        target = FreeCAD.Placement(
            FreeCAD.Vector(initial.Base.x + 10.0, initial.Base.y + 20.0, initial.Base.z + 30.0),
            initial.Rotation,
        )

        navigation.setOrientationLocked(True)
        self.View.viewPosition(target, 0, 0)
        moved = self.View.viewPosition()

        self.assertTrue(moved.Rotation.isSame(initial.Rotation, 1e-6))
        self.assertGreater((moved.Base - initial.Base).Length, 1e-6)

    def testOrientationLockDoesNotBlockCameraTypeChanges(self):
        navigation = self.View.getViewer().getNavigationStyle()
        current = self.View.getCameraType()
        target = "Orthographic" if current == "Perspective" else "Perspective"

        navigation.setOrientationLocked(True)
        self.View.setCameraType(target)

        self.assertEqual(self.View.getCameraType(), target)

    def testNavigationStyleSwitchPreservesRotationFlags(self):
        viewer = self.View.getViewer()
        navigation = viewer.getNavigationStyle()
        current = self.View.getNavigationType()
        styles = [style for style in self.View.listNavigationTypes() if style != current]

        self.assertTrue(styles)
        navigation.setRotationEnabled(False)
        navigation.setOrientationLocked(True)

        self.View.setNavigationType(styles[0])
        navigation = viewer.getNavigationStyle()

        self.assertFalse(navigation.isRotationEnabled())
        self.assertTrue(navigation.isOrientationLocked())

    def testRotationDisableStopsActiveSpinAnimation(self):
        viewer = self.View.getViewer()
        navigation = viewer.getNavigationStyle()

        self.View.startAnimating(0.0, 1.0, 0.0, 1.0)
        self.assertTrue(viewer.isSpinning())

        navigation.setRotationEnabled(False)

        self.assertFalse(viewer.isSpinning())

    def tearDown(self):
        FreeCAD.closeDocument("CreateTest")
