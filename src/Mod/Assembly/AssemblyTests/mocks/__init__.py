# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

# /****************************************************************************
#                                                                           *
#    Copyright (c) 2025 Weston Schmidt <weston_schmidt@alumni.purdue.edu>   *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# ***************************************************************************/

"""Mock classes for FreeCAD GUI testing."""

import builtins
from .MockGui import (
    MockQIcon,
    MockQTreeWidgetItem,
    MockPySideUic,
    MockGetDocument,
    MockAddModule,
    MockDoCommandSkip,
)

# Set up all FreeCAD GUI mocks for testing
# Always create mocks for consistent testing
Gui = type("MockGui", (), {})()
QtCore = type("MockQtCore", (), {})()
QtGui = type("MockQtGui", (), {})()

# Patch QtGui with our mock classes
QtGui.QIcon = MockQIcon
QtGui.QTreeWidgetItem = MockQTreeWidgetItem

# Mock the PySideUic if it doesn't exist
if not hasattr(Gui, "PySideUic"):
    Gui.PySideUic = MockPySideUic

# Mock additional Gui methods that might be missing
if not hasattr(Gui, "getDocument"):
    Gui.getDocument = MockGetDocument

# Mock Selection module
if not hasattr(Gui, "Selection"):
    Gui.Selection = type(
        "MockSelection",
        (),
        {
            "clearSelection": lambda *args: None,
            "addSelection": lambda *args: None,
            "getSelection": lambda *args: [],
        },
    )()

# Mock addModule method
if not hasattr(Gui, "addModule"):
    Gui.addModule = MockAddModule

# Mock doCommandSkip method
if not hasattr(Gui, "doCommandSkip"):
    Gui.doCommandSkip = MockDoCommandSkip

# Make QtCore, QtGui and Gui available in the global namespace
builtins.QtCore = QtCore
builtins.QtGui = QtGui
builtins.Gui = Gui
builtins.QIcon = MockQIcon

__all__ = []
