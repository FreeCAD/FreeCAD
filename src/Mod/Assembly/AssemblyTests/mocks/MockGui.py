# SPDX-License-Identifier: LGPL-2.1-or-later
# /****************************************************************************
#                                                                           *
#    Copyright (c) 2025 Weston Schmidt <weston_schmidt@alumni.purdue.edu>   *
#                                                                           *
#    This file is part of FreeCAD.                                          *
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

"""
Mock classes for FreeCAD GUI testing.

This module provides mock implementations of FreeCAD and Qt classes
to enable unit testing without requiring the full FreeCAD environment.
"""

# pylint: disable=too-few-public-methods

import builtins


class MockQIcon:
    """Mock QIcon class."""

    def __init__(self, *_args, **_kwargs):
        pass

    @staticmethod
    def fromTheme(_theme_name, _fallback=None):
        """Mock fromTheme method."""
        return MockQIcon()


class MockQTreeWidgetItem:
    """Mock QTreeWidgetItem class."""

    def __init__(self, *_args, **_kwargs):
        self.text_values = {}
        self.data_values = {}
        self._children = []

    def setText(self, column, text):
        """Mock setText method."""
        self.text_values[column] = text

    def text(self, column):
        """Mock text method."""
        return self.text_values.get(column, "")

    def setData(self, column, role, data):
        """Mock setData method."""
        self.data_values[(column, role)] = data

    def data(self, column, role):
        """Mock data method."""
        return self.data_values.get((column, role))

    def setIcon(self, _column, _icon):
        """Mock setIcon method."""

    def childCount(self):
        """Mock method for getting child count."""
        return len(self._children)

    def child(self, index):
        """Mock method for getting child by index."""
        if 0 <= index < len(self._children):
            return self._children[index]
        return None

    def addChild(self, child):
        """Mock method for adding a child item."""
        self._children.append(child)


class MockSignal:
    """Mock Signal class."""

    def connect(self, _slot):
        """Mock connect method."""


class MockCheckBox:
    """Mock CheckBox class."""

    def __init__(self):
        self.checked = False
        self.stateChanged = MockSignal()

    def setChecked(self, checked):
        """Mock setChecked method."""
        self.checked = checked

    def isChecked(self):
        """Mock isChecked method."""
        return self.checked


class MockButton:
    """Mock Button class."""

    def __init__(self):
        pass

    @property
    def clicked(self):
        """Mock clicked method."""
        return MockSignal()


class MockLineEdit:
    """Mock LineEdit class."""

    def __init__(self):
        self.textChanged = MockSignal()

    def text(self):
        """Mock text method."""
        return ""

    def setText(self, _text):
        """Mock setText method."""


class MockHeader:
    """Mock Header class."""

    def hide(self):
        """Mock hide method."""


class MockPartList:
    """Mock PartList class."""

    def __init__(self):
        self.itemClicked = MockSignal()
        self.itemDoubleClicked = MockSignal()
        self._items = []

    def header(self):
        """Mock header method."""
        return MockHeader()

    def clear(self):
        """Mock clear method."""
        self._items = []

    def addTopLevelItem(self, item):
        """Mock addTopLevelItem method."""
        self._items.append(item)

    def installEventFilter(self, _filter_obj):
        """Mock installEventFilter method."""

    def expandAll(self):
        """Mock expandAll method."""

    def topLevelItemCount(self):
        """Mock topLevelItemCount method."""
        return len(self._items)

    def topLevelItem(self, index):
        """Mock topLevelItem method."""
        if 0 <= index < len(self._items):
            return self._items[index]
        return None

    def sizeHintForRow(self, _row):
        """Mock sizeHintForRow method."""
        return 20

    def setMinimumHeight(self, _height):
        """Mock setMinimumHeight method."""


class MockForm:
    """Mock Form class."""

    def __init__(self):
        self.partList = MockPartList()
        self.CheckBox_ShowOnlyParts = MockCheckBox()
        self.CheckBox_RigidSubAsm = MockCheckBox()
        self.openFileButton = MockButton()
        self.filterPartList = MockLineEdit()

    def installEventFilter(self, _filter_obj):
        """Mock installEventFilter method."""

    def setWindowTitle(self, _title):
        """Mock setWindowTitle method."""

    def show(self):
        """Mock show method."""

    def hide(self):
        """Mock hide method."""


class MockPySideUic:
    """Mock PySideUic class."""

    @staticmethod
    def loadUi(_ui_file):
        """Mock loadUi method."""
        return MockForm()


def MockGetDocument(doc_name):
    """Mock getDocument function."""
    return type(
        "MockGuiDocument",
        (),
        {
            "Name": doc_name,
            "getObject": lambda _obj_name: None,
            "TreeRootObjects": [],
        },
    )()


def MockAddModule(_module_name):
    """Mock addModule function."""


def MockDoCommandSkip(_commands):
    """Mock doCommandSkip function."""


def SetupGuiMocks():
    """Set up all FreeCAD GUI mocks for testing."""
    import FreeCADGui as Gui  # pylint: disable=import-error,import-outside-toplevel
    from PySide import QtCore, QtGui  # pylint: disable=import-error,import-outside-toplevel

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

    return True
