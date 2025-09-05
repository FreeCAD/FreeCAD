# -*- coding: utf-8 -*-
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

from typing import Callable, List
from PySide import QtGui, QtCore
from .tablecell import TwoLineTableCell, CompactTwoLineTableCell
from ..models.base import ToolBit  # For type hinting

# Role for storing the ToolBit URI string
ToolBitUriRole = QtCore.Qt.UserRole + 1


class ToolBitListWidget(QtGui.QListWidget):
    """
    A QListWidget specialized for displaying ToolBit items using
    TwoLineTableCell widgets.
    """

    def __init__(self, parent=None, tool_no_factory: Callable | None = None):
        super().__init__(parent)
        self._tool_no_factory = tool_no_factory
        # Optimize view for custom widgets
        self.setUniformItemSizes(False)  # Allow different heights if needed
        self.setAutoScroll(True)
        self.setSelectionMode(QtGui.QAbstractItemView.ExtendedSelection)
        # Consider setting view mode if needed, default is ListMode
        # self.setViewMode(QtGui.QListView.ListMode)
        # self.setResizeMode(QtGui.QListView.Adjust) # Adjust items on resize

    def add_toolbit(self, toolbit: ToolBit, tool_no: int | None = None):
        """
        Adds a ToolBit to the list.

        Args:
            toolbit (ToolBit): The ToolBit object to add.
            tool_no (int | None): The tool number associated with the ToolBit,
                                  or None if not applicable.
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

        # Set the custom widget for the list item
        item.setSizeHint(cell.sizeHint())
        self.setItemWidget(item, cell)

        # Store the ToolBit URI for later retrieval
        item.setData(ToolBitUriRole, str(toolbit.get_uri()))

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

    def add_toolbit(self, toolbit: ToolBit, tool_no: int | None = None):
        """
        Adds a ToolBit to the list using CompactTwoLineTableCell.

        Args:
            toolbit (ToolBit): The ToolBit object to add.
            tool_no (int | None): The tool number associated with the ToolBit,
                                  or None if not applicable.
        """
        # Use the factory function if provided, otherwise use the passed tool_no
        final_tool_no = None
        if self._tool_no_factory:
            final_tool_no = self._tool_no_factory(toolbit)
        elif tool_no is not None:
            final_tool_no = tool_no

        item = QtGui.QListWidgetItem(self)  # Add item to this widget
        cell = CompactTwoLineTableCell(self)  # Parent the cell to this widget

        # Populate the cell widget
        cell.set_tool_no(final_tool_no)
        cell.set_upper_text(toolbit.label)
        lower_text = toolbit.summary
        cell.set_icon_from_shape(toolbit._tool_bit_shape)
        cell.set_lower_text(lower_text)

        # Set the custom widget for the list item
        item.setSizeHint(cell.sizeHint())
        self.setItemWidget(item, cell)

        # Store the ToolBit URI for later retrieval
        item.setData(ToolBitUriRole, str(toolbit.get_uri()))
