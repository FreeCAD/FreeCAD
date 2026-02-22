# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 Furgo                                              *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

import unittest
import FreeCAD
import FreeCADGui
from bimtests.TestArchBase import TestArchBase


class TestArchBaseGui(TestArchBase):
    """
    The base class for all Arch/BIM GUI unit tests.
    It inherits from TestArchBase to handle document setup and adds
    GUI-specific initialization by activating the BIM workbench.
    """

    @classmethod
    def setUpClass(cls):
        """
        Ensure the GUI is available and activate the BIM workbench once
        before any tests in the inheriting class are run.
        """
        if not FreeCAD.GuiUp:
            raise unittest.SkipTest("Cannot run GUI tests in a CLI environment.")

        # Activating the workbench ensures all GUI commands are loaded and ready.
        # TODO: commenting out this line for now as it causes a timeout without further logging in
        # CI
        # FreeCADGui.activateWorkbench("BIMWorkbench")

    def setUp(self):
        """
        Run the parent's setup to create the uniquely named document.
        The workbench is already activated by setUpClass.
        """
        super().setUp()

    def tearDown(self):
        """
        Ensure GUI events are processed and dialogs closed before the document is destroyed.
        This prevents race conditions where pending GUI tasks try to access a closed document.
        """
        # Process any pending Qt events (like todo.delay calls) while the doc is still open
        self.pump_gui_events()

        # Close any open task panels
        if FreeCAD.GuiUp:
            FreeCADGui.Control.closeDialog()

        super().tearDown()

    def pump_gui_events(self, timeout_ms=200):
        """Run the Qt event loop briefly so queued GUI callbacks execute.

        This helper starts a QEventLoop and quits it after `timeout_ms` milliseconds using
        QTimer.singleShot. Any exception (e.g. missing Qt in the environment) is silently ignored so
        tests can still run in pure-CLI environments where the GUI isn't available.
        """
        try:
            from PySide import QtCore

            loop = QtCore.QEventLoop()
            QtCore.QTimer.singleShot(int(timeout_ms), loop.quit)
            loop.exec_()
        except Exception:
            # Best-effort: if Qt isn't present or event pumping fails, continue.
            pass
