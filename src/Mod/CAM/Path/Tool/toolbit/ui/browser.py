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

"""Widget for browsing ToolBit assets with filtering and sorting."""

from typing import List, cast
from PySide import QtGui, QtCore
from typing import List, cast
from PySide import QtGui, QtCore
from ...assets import AssetManager, AssetUri
from ...toolbit import ToolBit
from .toollist import ToolBitListWidget, CompactToolBitListWidget, ToolBitUriRole


class ToolBitBrowserWidget(QtGui.QWidget):
    """
    A widget to browse, filter, and select ToolBit assets from the
    AssetManager, with sorting and batch insertion.
    """

    # Signal emitted when a tool is selected in the list
    toolSelected = QtCore.Signal(str)  # Emits ToolBit URI string
    # Signal emitted when a tool is requested for editing (e.g., double-click)
    itemDoubleClicked = QtCore.Signal(str)  # Emits ToolBit URI string

    # Debounce timer for search input
    _search_timer_interval = 300  # milliseconds
    _batch_size = 20  # Number of items to insert per batch

    def __init__(
        self,
        asset_manager: AssetManager,
        store: str = "local",
        parent=None,
        tool_no_factory=None,
        compact=False,
    ):
        super().__init__(parent)
        self._asset_manager = asset_manager
        self._tool_no_factory = tool_no_factory
        self._compact_mode = compact

        self._is_fetching = False
        self._store_name = store
        self._all_assets: List[ToolBit] = []  # Store all fetched assets
        self._current_search = ""  # Track current search term
        self._scroll_position = 0  # Track scroll position
        self._sort_key = "tool_no" if tool_no_factory else "label"

        # UI Elements
        self._search_edit = QtGui.QLineEdit()
        self._search_edit.setPlaceholderText("Search tools...")

        # Sorting dropdown
        self._sort_combo = QtGui.QComboBox()
        if self._tool_no_factory:
            self._sort_combo.addItem("Sort by Tool Number", "tool_no")
        self._sort_combo.addItem("Sort by Label", "label")
        self._sort_combo.setCurrentIndex(0)
        self._sort_combo.setVisible(self._tool_no_factory is not None)  # Hide if no tool_no_factory

        # Top layout for search and sort
        self._top_layout = QtGui.QHBoxLayout()
        self._top_layout.addWidget(self._search_edit, 3)  # Give search more space
        self._top_layout.addWidget(self._sort_combo, 1)

        if self._compact_mode:
            self._tool_list_widget = CompactToolBitListWidget(tool_no_factory=self._tool_no_factory)
        else:
            self._tool_list_widget = ToolBitListWidget(tool_no_factory=self._tool_no_factory)

        # Main layout
        layout = QtGui.QVBoxLayout(self)
        layout.addLayout(self._top_layout)
        layout.addWidget(self._tool_list_widget)

        # Connections
        self._search_timer = QtCore.QTimer(self)
        self._search_timer.setSingleShot(True)
        self._search_timer.setInterval(self._search_timer_interval)
        self._search_timer.timeout.connect(self._trigger_fetch)
        self._search_edit.textChanged.connect(self._search_timer.start)
        self._sort_combo.currentIndexChanged.connect(self._on_sort_changed)

        scrollbar = self._tool_list_widget.verticalScrollBar()
        scrollbar.valueChanged.connect(self._on_scroll)

        self._tool_list_widget.itemDoubleClicked.connect(self._on_item_double_clicked)
        self._tool_list_widget.currentItemChanged.connect(self._on_item_selection_changed)

        # Note that fetching of assets is done at showEvent(),
        # because we need to know the widget size to calculate the number
        # of items that need to be fetched.

    def showEvent(self, event):
        """Handles the widget show event to trigger initial data fetch."""
        super().showEvent(event)
        # Fetch all assets the first time the widget is shown
        if not self._all_assets and not self._is_fetching:
            self._fetch_all_assets()

    def _fetch_all_assets(self):
        """Fetches all ToolBit assets and stores them in memory."""
        if self._is_fetching:
            return
        self._is_fetching = True
        try:
            self._all_assets = cast(
                List[ToolBit],
                self._asset_manager.fetch(
                    asset_type="toolbit",
                    depth=0,  # do not fetch dependencies (e.g. shape, icon)
                    store=self._store_name,
                ),
            )
            self._sort_assets()
        finally:
            self._is_fetching = False
        self._trigger_fetch()

    def _sort_assets(self):
        """Sorts the in-memory assets based on the current sort key."""
        if self._sort_key == "label":
            self._all_assets.sort(key=lambda x: x.label.lower())
        elif self._sort_key == "tool_no" and self._tool_no_factory:
            self._all_assets.sort(
                key=lambda x: (int(self._tool_no_factory(x)) or 0) if self._tool_no_factory else 0
            )

    def _trigger_fetch(self):
        """Initiates a data fetch, clearing the list only if search term changes."""
        new_search = self._search_edit.text()
        if new_search != self._current_search:
            self._current_search = new_search
            self._tool_list_widget.clear_list()
            self._scroll_position = 0
        self._fetch_data()

    def _fetch_batch(self, offset):
        """Inserts a batch of filtered assets into the list widget."""
        filtered_assets = [
            asset
            for asset in self._all_assets
            if not self._current_search or self._matches_search(asset, self._current_search)
        ]
        end_idx = min(offset + self._batch_size, len(filtered_assets))
        for i in range(offset, end_idx):
            self._tool_list_widget.add_toolbit(filtered_assets[i])
        return end_idx < len(filtered_assets)  # Return True if more items remain

    def _matches_search(self, toolbit, search_term):
        """Checks if a ToolBit matches the search term."""
        search_term = search_term.lower()
        return search_term in toolbit.label.lower() or search_term in toolbit.summary.lower()

    def _fetch_data(self):
        """Inserts filtered and sorted ToolBit assets into the list widget."""
        if self._is_fetching:
            return
        self._is_fetching = True
        try:
            # Save current scroll position and selected item
            scrollbar = self._tool_list_widget.verticalScrollBar()
            self._scroll_position = scrollbar.value()
            selected_uri = self._tool_list_widget.get_selected_toolbit_uri()

            # Insert initial batches to fill the viewport
            offset = self._tool_list_widget.count()
            more_items = True
            while more_items:
                more_items = self._fetch_batch(offset)
                offset += self._batch_size
                if scrollbar.maximum() != 0:
                    break

            # Apply filter to ensure UI consistency
            self._tool_list_widget.apply_filter(self._current_search)

            # Restore scroll position and selection
            scrollbar.setValue(self._scroll_position)
            if selected_uri:
                for i in range(self._tool_list_widget.count()):
                    item = self._tool_list_widget.item(i)
                    if item.data(ToolBitUriRole) == selected_uri and not item.isHidden():
                        self._tool_list_widget.setCurrentItem(item)
                        break

        finally:
            self._is_fetching = False

    def _on_scroll(self, value):
        """Handles scroll events for lazy batch insertion."""
        scrollbar = self._tool_list_widget.verticalScrollBar()
        is_near_bottom = value >= scrollbar.maximum() - scrollbar.singleStep()
        filtered_count = sum(
            1
            for asset in self._all_assets
            if not self._current_search or self._matches_search(asset, self._current_search)
        )
        more_might_exist = self._tool_list_widget.count() < filtered_count

        if is_near_bottom and more_might_exist and not self._is_fetching:
            self._fetch_data()

    def _on_sort_changed(self):
        """Handles sort order change from the dropdown."""
        self._sort_key = self._sort_combo.itemData(self._sort_combo.currentIndex())
        self._sort_assets()
        self._tool_list_widget.clear_list()
        self._scroll_position = 0
        self._fetch_data()

    def _on_item_double_clicked(self, item):
        """Emits itemDoubleClicked signal when an item is double-clicked."""
        uri = item.data(ToolBitUriRole)
        if uri:
            self.itemDoubleClicked.emit(uri)

    def _on_item_selection_changed(self, current_item, previous_item):
        """Emits toolSelected signal when the selection changes."""
        uri = None
        if current_item:
            uri = current_item.data(ToolBitUriRole)
        self.toolSelected.emit(uri if current_item else None)

    def get_selected_bit_uris(self) -> List[str]:
        """
        Returns a list of URIs for the currently selected ToolBit items.
        Delegates to the underlying list widget.
        """
        return self._tool_list_widget.get_selected_toolbit_uris()

    def get_selected_bits(self) -> List[ToolBit]:
        """
        Returns a list of selected ToolBit objects.
        Retrieves the full ToolBit objects using the asset manager.
        """
        selected_bits = []
        selected_uris = self.get_selected_bit_uris()
        for uri_string in selected_uris:
            toolbit = self._asset_manager.get(AssetUri(uri_string))
            if toolbit:
                selected_bits.append(toolbit)
        return selected_bits
