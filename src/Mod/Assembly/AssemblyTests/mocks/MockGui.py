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

"""
Mock classes for FreeCAD GUI testing.

This module provides mock implementations of FreeCAD and Qt classes
to enable unit testing without requiring the full FreeCAD environment.
"""

# pylint: disable=too-few-public-methods

from unittest.mock import MagicMock


def create_mock_qicon():
    """Create a mock QIcon with fromTheme static method."""
    mock_qicon = MagicMock()
    mock_qicon.fromTheme = MagicMock(return_value=mock_qicon)
    return mock_qicon


def create_mock_qtreewidgetitem():
    """Create a mock QTreeWidgetItem with required methods and state tracking."""
    mock_item = MagicMock()

    # Add state tracking for setText/text functionality
    mock_item.text_values = {}
    mock_item.data_values = {}
    mock_item.children = []

    def mock_set_text(column, text):
        mock_item.text_values[column] = text

    def mock_get_text(column):
        return mock_item.text_values.get(column, "")

    def mock_set_data(column, role, data):
        mock_item.data_values[(column, role)] = data

    def mock_get_data(column, role):
        return mock_item.data_values.get((column, role))

    def mock_child_count():
        return len(mock_item.children)

    def mock_child(index):
        if 0 <= index < len(mock_item.children):
            return mock_item.children[index]
        return None

    def mock_add_child(child):
        mock_item.children.append(child)

    # Configure the mock with specific behaviors
    mock_item.setText = mock_set_text
    mock_item.text = mock_get_text
    mock_item.setData = mock_set_data
    mock_item.data = mock_get_data
    mock_item.childCount = mock_child_count
    mock_item.child = mock_child
    mock_item.addChild = mock_add_child

    return mock_item


def create_mock_checkbox():
    """Create a mock CheckBox with state tracking."""
    mock_checkbox = MagicMock()
    mock_checkbox.checked = False

    def mock_set_checked(checked):
        mock_checkbox.checked = checked

    def mock_is_checked():
        return mock_checkbox.checked

    mock_checkbox.setChecked = mock_set_checked
    mock_checkbox.isChecked = mock_is_checked
    mock_checkbox.stateChanged = MagicMock()

    return mock_checkbox


def create_mock_line_edit():
    """Create a mock LineEdit."""
    mock_line_edit = MagicMock()
    mock_line_edit.text.return_value = ""
    mock_line_edit.textChanged = MagicMock()
    return mock_line_edit


def create_mock_part_list():
    """Create a mock PartList with required functionality."""
    mock_part_list = MagicMock()
    mock_part_list.items = []

    def mock_clear():
        mock_part_list.items = []

    def mock_add_top_level_item(item):
        mock_part_list.items.append(item)

    def mock_top_level_item_count():
        return len(mock_part_list.items)

    def mock_top_level_item(index):
        if 0 <= index < len(mock_part_list.items):
            return mock_part_list.items[index]
        return None

    mock_part_list.clear = mock_clear
    mock_part_list.addTopLevelItem = mock_add_top_level_item
    mock_part_list.topLevelItemCount = mock_top_level_item_count
    mock_part_list.topLevelItem = mock_top_level_item
    mock_part_list.sizeHintForRow.return_value = 20

    # Add other required attributes/methods
    mock_part_list.itemClicked = MagicMock()
    mock_part_list.itemDoubleClicked = MagicMock()
    mock_part_list.header.return_value = MagicMock()

    return mock_part_list


def create_mock_form():
    """Create a mock Form with all required components."""
    mock_form = MagicMock()
    mock_form.partList = create_mock_part_list()
    mock_form.CheckBox_ShowOnlyParts = create_mock_checkbox()
    mock_form.CheckBox_RigidSubAsm = create_mock_checkbox()
    mock_form.openFileButton = MagicMock()
    mock_form.openFileButton.clicked = MagicMock()
    mock_form.filterPartList = create_mock_line_edit()
    return mock_form


def create_mock_pyside_uic():
    """Create a mock PySideUic with loadUi method."""
    mock_uic = MagicMock()
    mock_uic.loadUi.return_value = create_mock_form()
    return mock_uic


def create_mock_gui_document(doc_name):
    """Create a mock GUI document."""
    mock_doc = MagicMock()
    mock_doc.Name = doc_name
    mock_doc.getObject.return_value = None
    mock_doc.TreeRootObjects = []
    return mock_doc


# Factory functions for creating specific mock instances
MockQIcon = create_mock_qicon
MockQTreeWidgetItem = create_mock_qtreewidgetitem
MockPySideUic = create_mock_pyside_uic
MockGetDocument = create_mock_gui_document
MockAddModule = MagicMock()
MockDoCommandSkip = MagicMock()
