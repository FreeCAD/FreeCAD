# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

# ***************************************************************************
# *   Copyright (c) 2025 Samuel Abels <knipknap@gmail.com>                  *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""Widget for displaying a list of ToolBits using TwoLineTableCell."""

import yaml
import Path
from typing import Callable, List
from PySide import QtGui, QtCore
from PySide.QtGui import QDrag
from PySide.QtCore import QMimeData
from ..models.base import ToolBit
from .tablecell import TwoLineTableCell, CompactTwoLineTableCell


# Role for storing the ToolBit URI string
ToolBitUriRole = QtCore.Qt.UserRole + 1
ToolBitUriListMimeType = "application/x-freecad-toolbit-uri-list-yaml"


class ToolBitListWidget(QtGui.QListWidget):
    """
    A QListWidget specialized for displaying ToolBit items using
    TwoLineTableCell widgets.
    """

    def __init__(self, parent=None, tool_no_factory: Callable | None = None):
        super().__init__(parent)
        self._tool_no_factory = tool_no_factory
        self.setAutoScroll(True)
        self.setSelectionMode(QtGui.QAbstractItemView.ExtendedSelection)

    def setDragEnabled(self, enabled: bool = True):
        """Enable or disable drag-and-drop support for toolbits."""
        super().setDragEnabled(enabled)

    def startDrag(self, supportedActions):
        """Initiate drag with selected toolbits serialized as mime data if drag is enabled."""
        Path.Log.debug("startDrag: Drag initiated.")
        selected_items = self.selectedItems()
        if not selected_items:
            Path.Log.debug("startDrag: No items selected for drag.")
            return

        uris = [item.data(ToolBitUriRole) for item in selected_items]
        if not uris:
            Path.Log.debug("startDrag: No valid URIs found for selected items.")
            return

        # Create clipboard data
        clipboard_data = {
            "toolbits": uris,
        }
        yaml_data = yaml.safe_dump(clipboard_data).encode("utf-8")

        # Set mime data
        mime_data = QMimeData()
        mime_data.setData(ToolBitUriListMimeType, yaml_data)

        # Start drag
        drag = QDrag(self)
        drag.setMimeData(mime_data)
        drag.exec_(QtCore.Qt.CopyAction | QtCore.Qt.MoveAction)
        Path.Log.debug("startDrag: Drag executed.")

    def _create_toolbit_item(self, toolbit: ToolBit, tool_no: int | None = None):
        """
        Creates a QListWidgetItem and populates it with ToolBit data.
        """
        # Use the factory function if provided, otherwise use the passed tool_no
        final_tool_no = None
        if self._tool_no_factory:
            final_tool_no = self._tool_no_factory(toolbit)
        elif tool_no is not None:
            final_tool_no = tool_no

        # Add item to this widget
        item = QtGui.QListWidgetItem(self)
        cell = TwoLineTableCell(self)

        # Populate the cell widget
        cell.set_tool_no(final_tool_no)
        cell.set_upper_text(toolbit.label)
        cell.set_lower_text(toolbit.summary)
        cell.set_icon_from_shape(toolbit._tool_bit_shape)

        # Set the custom widget for the list item
        item.setSizeHint(cell.sizeHint())
        self.setItemWidget(item, cell)

        # Store the ToolBit URI for later retrieval
        item.setData(ToolBitUriRole, str(toolbit.get_uri()))

        return item

    def add_toolbit(self, toolbit: ToolBit, tool_no: int | None = None):
        """
        Adds a ToolBit to the list.

        Args:
            toolbit (ToolBit): The ToolBit object to add.
            tool_no (int | None): The tool number associated with the ToolBit,
                                  or None if not applicable.
        """
        item = self._create_toolbit_item(toolbit, tool_no)
        self.addItem(item)

    def insert_toolbit(self, row: int, toolbit: ToolBit, tool_no: int | None = None):
        """
        Inserts a ToolBit to the list at the specified row.

        Args:
            row (int): The row index where the item should be inserted.
            toolbit (ToolBit): The ToolBit object to add.
            tool_no (int | None): The tool number associated with the ToolBit,
                                  or None if not applicable.
        """
        item = self._create_toolbit_item(toolbit, tool_no)
        self.insertItem(row, item)

    def clear_list(self):
        """Removes all items from the list."""
        self.clear()

    def apply_filter(self, search_text: str):
        """
        Filters the list items based on the search text.
        Items are hidden if they don't contain the text in their
        tool number, upper text, or lower text.
        Also applies highlighting to the visible matching text.
        """
        search_text_lower = search_text.lower()
        for i in range(self.count()):
            item = self.item(i)
            cell = self.itemWidget(item)

            if isinstance(cell, TwoLineTableCell):
                cell.highlight(search_text)  # Apply highlighting
                # Determine visibility based on content
                contains = cell.contains_text(search_text_lower)
                item.setHidden(not contains)
            else:
                # Fallback for items without the expected widget (shouldn't happen)
                item_text = item.text().lower()  # Basic text search
                item.setHidden(search_text_lower not in item_text)

    def count_visible_items(self) -> int:
        """
        Counts and returns the number of visible items in the list.
        """
        visible_count = 0
        for i in range(self.count()):
            item = self.item(i)
            if not item.isHidden():
                visible_count += 1
        return visible_count

    def get_selected_toolbit_uri(self) -> str | None:
        """
        Returns the URI string of the currently selected ToolBit item.
        Returns None if no item is selected.
        """
        currentItem = self.currentItem()
        if currentItem:
            return currentItem.data(ToolBitUriRole)
        return None

    def get_selected_toolbit_uris(self) -> List[str]:
        """
        Returns a list of URI strings for the currently selected ToolBit items.
        Returns an empty list if no item is selected.
        """
        selected_uris = []
        selected_items = self.selectedItems()
        for item in selected_items:
            uri = item.data(ToolBitUriRole)
            if uri:
                selected_uris.append(uri)
        return selected_uris


class CompactToolBitListWidget(ToolBitListWidget):
    """
    A QListWidget specialized for displaying ToolBit items using
    CompactTwoLineTableCell widgets.
    """

    def _create_toolbit_item(self, toolbit: ToolBit, tool_no: int | None = None):
        """
        Creates a QListWidgetItem and populates it with ToolBit data
        using CompactTwoLineTableCell.
        """
        # Use the factory function if provided, otherwise use the passed tool_no
        final_tool_no = None
        if self._tool_no_factory:
            final_tool_no = self._tool_no_factory(toolbit)
        elif tool_no is not None:
            final_tool_no = tool_no

        item = QtGui.QListWidgetItem(self)
        cell = CompactTwoLineTableCell(self)

        # Populate the cell widget
        cell.set_tool_no(final_tool_no)
        cell.set_upper_text(toolbit.label)
        cell.set_lower_text(toolbit.summary)
        cell.set_icon_from_shape(toolbit._tool_bit_shape)

        # Set the custom widget for the list item
        item.setSizeHint(cell.sizeHint())
        self.setItemWidget(item, cell)

        # Store the ToolBit URI for later retrieval
        item.setData(ToolBitUriRole, str(toolbit.get_uri()))

        return item
