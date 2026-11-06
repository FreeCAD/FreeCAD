# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2025 Furgo
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""Base class for Import module GUI tests."""

import unittest
import FreeCAD
from importtests.TestImportBase import TestImportBase


class TestImportBaseGui(TestImportBase):
    """Base class for Import GUI tests.

    Adds to TestImportBase a skip when the GUI is not available, and a helper to run the Qt event
    loop.
    """

    @classmethod
    def setUpClass(cls):
        """Skip the tests of the class when the GUI is not available."""
        if not FreeCAD.GuiUp:
            raise unittest.SkipTest("Cannot run GUI tests in a CLI environment.")

    def pump_gui_events(self, timeout_ms=200):
        """Run the Qt event loop for `timeout_ms` milliseconds so queued GUI callbacks execute.

        Errors while running the event loop are ignored.
        """
        try:
            from PySide import QtCore

            loop = QtCore.QEventLoop()
            QtCore.QTimer.singleShot(int(timeout_ms), loop.quit)
            loop.exec_()
        except Exception:
            # Best effort: a failure to process events must not fail the test.
            pass
