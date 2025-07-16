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

"""Widget for browsing Tool Library assets with filtering and sorting."""

from typing import cast
from PySide import QtGui
import Path
from ...toolbit.ui.browser import ToolBitBrowserWidget
from ...assets import AssetManager
from ...library import Library


class LibraryBrowserWidget(ToolBitBrowserWidget):
    """
    A widget to browse, filter, and select Tool Library assets from the
    AssetManager, with sorting and batch insertion, including library selection.
    """

    def __init__(
        self,
        asset_manager: AssetManager,
        store: str = "local",
        parent=None,
        compact=True,
    ):
        self._library_combo = QtGui.QComboBox()

        super().__init__(
            asset_manager=asset_manager,
            store=store,
            parent=parent,
            tool_no_factory=self.get_tool_no_from_current_library,
            compact=compact,
        )

        # Create the library dropdown and insert it into the top layout
        self._top_layout.insertWidget(0, self._library_combo)
        self._library_combo.currentIndexChanged.connect(self._on_library_changed)

    def refresh(self):
        """Refreshes the library dropdown and fetches all assets."""
        self._library_combo.clear()
        self._fetch_all_assets()

    def _fetch_all_assets(self):
        """Populates the library dropdown with available libraries."""
        # Use list_assets("toolbitlibrary") to get URIs
        libraries = self._asset_manager.fetch("toolbitlibrary", store=self._store_name, depth=0)
        for library in sorted(libraries, key=lambda x: x.label):
            self._library_combo.addItem(library.label, userData=library)

        if not self._library_combo.count():
            return

        # Trigger initial load after populating libraries
        self._on_library_changed(0)

    def get_tool_no_from_current_library(self, toolbit):
        """
        Retrieves the tool number for a toolbit based on the currently
        selected library.
        """
        selected_library = self._library_combo.currentData()
        if selected_library is None:
            return None
        # Use the get_bit_no_from_bit method of the Library object
        # This method returns the tool number or None
        tool_no = selected_library.get_bit_no_from_bit(toolbit)
        return tool_no

    def _on_library_changed(self, index):
        """Handles library selection change."""
        # Get the selected library from the combo box
        selected_library = self._library_combo.currentData()
        if not isinstance(selected_library, Library):
            self._all_assets = []
            return

        # Fetch the library from the asset manager
        library_uri = selected_library.get_uri()
        try:
            library = self._asset_manager.get(library_uri, store=self._store_name, depth=1)
            # Update the combo box item's user data with the fully fetched library
            self._library_combo.setItemData(index, library)
        except FileNotFoundError:
            Path.Log.error(f"Library {library_uri} not found in store {self._store_name}.")
            self._all_assets = []
            return

        # Update _all_assets based on the selected library
        library = cast(Library, library)
        self._all_assets = [t for t in library]
        self._sort_assets()
        self._tool_list_widget.clear_list()
        self._scroll_position = 0
        self._trigger_fetch()  # Display data for the selected library
