# SPDX-License-Identifier: LGPL-2.1-or-later

# flake8: noqa E731
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

import yaml
from typing import List, Optional, cast, Sequence
from PySide import QtGui, QtCore
from PySide.QtGui import QApplication, QMessageBox, QMenu, QAction, QKeySequence, QDialog
from PySide.QtCore import QMimeData
import FreeCAD
import Path
from ...assets import AssetManager, AssetUri
from ..models.base import ToolBit
from ..serializers.yaml import YamlToolBitSerializer
from .toollist import ToolBitListWidget, CompactToolBitListWidget, ToolBitUriRole
from .editor import ToolBitEditor
from .util import natural_sort_key


Path.Log.setLevel(Path.Log.Level.ERROR, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


class ToolBitBrowserWidget(QtGui.QWidget):
    """
    A widget to browse, filter, and select ToolBit assets from the
    AssetManager, with sorting and batch insertion.
    """

    # Signal emitted when a tool is selected in the list
    toolSelected = QtCore.Signal(str)  # Emits ToolBit URI string
    # Signal emitted when a tool is requested for editing (e.g., double-click)
    itemDoubleClicked = QtCore.Signal(ToolBit)  # Emits ToolBit URI string

    # Debounce timer for search input
    _search_timer_interval = 300  # milliseconds

    def __init__(
        self,
        asset_manager: AssetManager,
        store: str = "local",
        parent=None,
        tool_no_factory=None,
        tool_fetcher=None,
        compact=False,
    ):
        super().__init__(parent)
        self._asset_manager = asset_manager
        self._tool_no_factory = tool_no_factory
        self._compact_mode = compact

        self._is_fetching = False
        self._store_name = store
        self._all_assets: Sequence[ToolBit] = []  # Store all fetched assets
        self._current_search = ""  # Track current search term
        self._sort_key = "tool_no" if tool_no_factory else "label"
        self._selected_uris: List[str] = []  # Track selected toolbit URIs

        # UI Elements
        self._search_edit = QtGui.QLineEdit()
        self._search_edit.setPlaceholderText("Search toolbits...")

        # Sorting dropdown
        self._sort_combo = QtGui.QComboBox()
        if self._tool_no_factory:
            self._sort_combo.addItem("Sort by Toolbit Number", "tool_no")
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
        self._search_timer.timeout.connect(self._update_list)
        self._search_edit.textChanged.connect(self._search_timer.start)
        self._sort_combo.currentIndexChanged.connect(self._on_sort_changed)

        # Connect signals from the list widget
        self._tool_list_widget.itemDoubleClicked.connect(self._on_item_double_clicked)
        self._tool_list_widget.itemSelectionChanged.connect(self._on_item_selection_changed)

        # Connect list widget context menu request to browser handler
        self._tool_list_widget.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self._tool_list_widget.customContextMenuRequested.connect(self._show_context_menu)

        # Add keyboard shortcuts
        self._add_shortcuts()

        # Note that fetching of assets is done at showEvent(),
        # because we need to know the widget size to calculate the number
        # of items that need to be fetched.
        self.tool_fetcher = tool_fetcher or self._tool_fetcher

    def showEvent(self, event):
        """Handles the widget show event to trigger initial data fetch."""
        super().showEvent(event)
        # Fetch all assets the first time the widget is shown
        if not self._all_assets and not self._is_fetching:
            self.refresh()
        # Set focus to the search field
        self._search_edit.setFocus()

    def _tool_fetcher(self) -> Sequence[ToolBit]:
        return cast(
            List[ToolBit],
            self._asset_manager.fetch(
                asset_type="toolbit",
                depth=0,  # do not fetch dependencies (e.g. shape, icon)
                store=self._store_name,
            ),
        )

    def select_by_uri(self, uris: List[str]):
        if not uris:
            return

        # Select and scroll to the first toolbit
        is_first = True
        for i in range(self._tool_list_widget.count()):
            item = self._tool_list_widget.item(i)
            if item.data(ToolBitUriRole) in uris:
                self._tool_list_widget.setCurrentItem(item)
                if is_first:
                    # Scroll to the first selected item
                    is_first = False
                    self._tool_list_widget.scrollToItem(item)

    def refresh(self):
        """Fetches all ToolBit assets and stores them in memory, then updates the UI."""
        if self._is_fetching:
            return
        self._is_fetching = True
        try:
            self._all_assets = self.tool_fetcher()
        finally:
            self._is_fetching = False
        Path.Log.debug(f"Loaded {len(self._all_assets)} ToolBits.")

        self._sort_assets()
        self._update_list()

    def _sort_assets(self):
        """Sorts the in-memory assets based on the current sort key."""
        if self._sort_key == "label":
            self._all_assets.sort(key=lambda x: natural_sort_key(x.label))
        elif self._sort_key == "tool_no" and self._tool_no_factory:
            self._all_assets.sort(
                key=lambda x: int(self._tool_no_factory(x) or 0) if self._tool_no_factory else 0
            )

    def _matches_search(self, toolbit, search_term):
        """Checks if a ToolBit matches the search term."""
        search_term = search_term.lower()
        return search_term in toolbit.label.lower() or search_term in toolbit.summary.lower()

    def _update_list(self):
        """Updates the list widget based on current search and sort."""
        if self._is_fetching:
            return

        self._current_search = self._search_edit.text()
        filtered_assets = [
            asset
            for asset in self._all_assets
            if not self._current_search or self._matches_search(asset, self._current_search)
        ]

        # Collect current items in the list widget
        current_items = {}
        for i in range(self._tool_list_widget.count()):
            item = self._tool_list_widget.item(i)
            uri = item.data(ToolBitUriRole)
            if uri:
                current_items[uri] = item

        # Iterate through filtered assets and update the list widget
        for i, asset in enumerate(filtered_assets):
            uri = str(asset.get_uri())
            if uri in current_items:
                # Item exists, remove the old one and insert the new one
                item = current_items[uri]
                row = self._tool_list_widget.row(item)
                self._tool_list_widget.takeItem(row)
                self._tool_list_widget.insert_toolbit(i, asset)
                del current_items[uri]
            else:
                # Insert new item
                self._tool_list_widget.insert_toolbit(i, asset)

        # Remove items that are no longer in filtered_assets
        for uri, item in current_items.items():
            row = self._tool_list_widget.row(item)
            self._tool_list_widget.takeItem(row)

        # Restore selection and scroll to the selected item
        if self._selected_uris:
            first_selected_item = None
            for i in range(self._tool_list_widget.count()):
                item = self._tool_list_widget.item(i)
                uri = item.data(ToolBitUriRole)
                if uri in self._selected_uris:
                    item.setSelected(True)
                    if first_selected_item is None:
                        first_selected_item = item
            if first_selected_item:
                self._tool_list_widget.scrollToItem(first_selected_item)

        # Apply the filter to trigger highlighting in the list widget
        self._tool_list_widget.apply_filter(self._current_search)

    def set_sort_order(self, key: str):
        for i in range(self._sort_combo.count()):
            if self._sort_combo.itemData(i) == key:
                if self._sort_combo.currentIndex() != i:
                    self._sort_combo.setCurrentIndex(i)
                break
        else:
            return
        self._sort_key = key
        self._sort_assets()
        self._update_list()

    def _on_sort_changed(self):
        """Handles sort order change from the dropdown."""
        key = self._sort_combo.itemData(self._sort_combo.currentIndex())
        self.set_sort_order(key)

    def _on_item_double_clicked(self, item):
        """Handles double-click on a list item to request editing."""
        uri_string = item.data(ToolBitUriRole)
        if not uri_string:
            return
        try:
            toolbit = self._asset_manager.get(AssetUri(uri_string))
            if toolbit:
                self.itemDoubleClicked.emit(toolbit)
        except FileNotFoundError:
            # Handle missing/placeholder toolbits gracefully
            QMessageBox.warning(
                self,
                FreeCAD.Qt.translate("CAM", "Missing Toolbit"),
                FreeCAD.Qt.translate(
                    "CAM",
                    "This toolbit is missing from your local store. It may be a placeholder for a toolbit that was not found during library import.",
                ),
            )
        except Exception as e:
            # Handle other errors
            QMessageBox.critical(
                self,
                FreeCAD.Qt.translate("CAM", "Error"),
                FreeCAD.Qt.translate("CAM", f"Failed to load toolbit: {e}"),
            )

    def _on_item_selection_changed(self):
        """Emits toolSelected signal and tracks selected URIs."""
        selected_uris = self._tool_list_widget.get_selected_toolbit_uris()
        self._selected_uris = selected_uris
        if not selected_uris:
            return
        self.toolSelected.emit(selected_uris[0])

    def _get_first_selected_bit(self) -> Optional[ToolBit]:
        uris = self.get_selected_bit_uris()
        if not uris:
            return None
        uri_string = uris[0]
        return cast(ToolBit, self._asset_manager.get(AssetUri(uri_string)))

    def _on_edit_requested(self):
        """Opens the ToolBitEditor for the selected toolbit."""
        toolbit = self._get_first_selected_bit()
        if not toolbit:
            return

        # Open the editor for the selected toolbit
        editor = ToolBitEditor(toolbit)
        result = editor.show()
        if result != QDialog.Accepted:
            return

        # If the editor was closed with "OK", save the changes
        self._asset_manager.add(toolbit)
        Path.Log.info(f"Toolbit {toolbit.get_id()} saved.")
        self.refresh()
        self._update_list()

    def _add_shortcuts(self):
        """Adds keyboard shortcuts for common actions."""
        copy_action = QAction(self)
        copy_action.setShortcut(QKeySequence.Copy)
        copy_action.triggered.connect(self._on_copy_requested)
        self.addAction(copy_action)

        delete_action = QAction(self)
        delete_action.setShortcut(QKeySequence("Shift+Delete"))
        delete_action.triggered.connect(self._on_delete_requested)
        self.addAction(delete_action)

        edit_action = QAction(self)
        edit_action.setShortcut(QKeySequence("F2"))
        edit_action.triggered.connect(self._on_edit_requested)
        self.addAction(edit_action)

    def _create_base_context_menu(self):
        """Creates the base context menu with Edit, Copy, and Delete actions."""
        selected_items = self._tool_list_widget.selectedItems()
        has_selection = bool(selected_items)

        context_menu = QMenu(self)

        edit_action = context_menu.addAction("Edit", self._on_edit_requested)
        edit_action.setEnabled(has_selection)
        context_menu.addSeparator()
        action = context_menu.addAction("Copy", self._on_copy_requested)
        action.setShortcut(QKeySequence.Copy)
        action = context_menu.addAction("Delete from disk", self._on_delete_requested)
        action.setShortcut(QKeySequence("Shift+Delete"))

        return context_menu

    def _show_context_menu(self, position):
        """Shows the context menu at the given position."""
        context_menu = self._create_base_context_menu()
        context_menu.exec_(self._tool_list_widget.mapToGlobal(position))

    def _to_clipboard(
        self,
        uris: List[str],
        mode: str = "copy",
        extra_data: Optional[dict] = None,
    ):
        """Copies selected toolbits to the clipboard as YAML."""
        if not uris:
            return

        selected_bits = [cast(ToolBit, self._asset_manager.get(AssetUri(uri))) for uri in uris]
        selected_bits = [bit for bit in selected_bits if bit]  # Filter out None
        if not selected_bits:
            return

        # Serialize selected toolbits individually
        serialized_toolbits_data = []
        for toolbit in selected_bits:
            yaml_data = YamlToolBitSerializer.serialize(toolbit)
            serialized_toolbits_data.append(yaml_data.decode("utf-8"))

        # Create a dictionary to hold the operation type and serialized data
        clipboard_data_dict = {
            "operation": mode,
            "toolbits": serialized_toolbits_data,
        }

        # Include extra data if provided
        if extra_data:
            clipboard_data_dict.update(extra_data)

        # Serialize the dictionary to YAML
        clipboard_content_yaml = yaml.dump(clipboard_data_dict, default_flow_style=False)

        # Put the YAML data on the clipboard with a custom MIME type
        mime_data = QMimeData()
        mime_type = "application/x-freecad-toolbit-list-yaml"
        mime_data.setData(mime_type, clipboard_content_yaml.encode("utf-8"))

        # Put it in text format for pasting to text editors
        toolbit_list = [yaml.safe_load(d) for d in serialized_toolbits_data]
        mime_data.setText(yaml.dump(toolbit_list, default_flow_style=False))

        clipboard = QApplication.clipboard()
        clipboard.setMimeData(mime_data)

    def _on_copy_requested(self):
        """Copies selected toolbits to the clipboard as YAML."""
        uris = self.get_selected_bit_uris()
        self._to_clipboard(uris, mode="copy")

    def _on_delete_requested(self):
        """Deletes selected toolbits and removes them from all libraries."""
        Path.Log.debug("ToolBitBrowserWidget._on_delete_requested: Function entered.")
        uris = self.get_selected_bit_uris()
        if not uris:
            Path.Log.debug("_on_delete_requested: No URIs selected. Returning.")
            return

        # Ask for confirmation
        reply = QMessageBox.question(
            self,
            FreeCAD.Qt.translate("CAM", "Confirm Deletion"),
            FreeCAD.Qt.translate(
                "CAM",
                "Are you sure you want to delete the selected toolbit(s)? This is not reversible. The toolbits will be removed from disk and from all libraries that contain them.",
            ),
            QMessageBox.Yes | QMessageBox.No,
            QMessageBox.No,
        )

        if reply != QMessageBox.Yes:
            return

        deleted_count = 0
        libraries_modified = []  # Use list instead of set since Library objects aren't hashable

        for uri_string in uris:
            try:
                toolbit_uri = AssetUri(uri_string)

                # First, remove the toolbit from all libraries that contain it
                libraries_to_update = self._find_libraries_containing_toolbit(toolbit_uri)
                for library in libraries_to_update:
                    library.remove_bit_by_uri(uri_string)
                    if library not in libraries_modified:  # Avoid duplicates
                        libraries_modified.append(library)
                    Path.Log.info(
                        f"Removed toolbit {toolbit_uri.asset_id} from library {library.label}"
                    )

                # Then delete the toolbit file from disk
                self._asset_manager.delete(toolbit_uri)
                deleted_count += 1
                Path.Log.info(f"Deleted toolbit file {uri_string}")

            except Exception as e:
                Path.Log.error(f"Failed to delete toolbit {uri_string}: {e}")

        # Save all modified libraries
        for library in libraries_modified:
            try:
                self._asset_manager.add(library)
                Path.Log.info(f"Saved updated library {library.label}")
            except Exception as e:
                Path.Log.error(f"Failed to save library {library.label}: {e}")

        if deleted_count > 0:
            Path.Log.info(
                f"Deleted {deleted_count} toolbit(s) and updated {len(libraries_modified)} libraries."
            )
            self.refresh()

    def _find_libraries_containing_toolbit(self, toolbit_uri: AssetUri) -> List:
        """Find all libraries that contain the specified toolbit."""
        from ...library.models.library import Library

        libraries_with_toolbit = []
        try:
            # Get all libraries from the asset manager
            all_libraries = self._asset_manager.fetch("toolbitlibrary", store="local", depth=1)

            for library in all_libraries:
                if isinstance(library, Library):
                    # Check if this library contains the toolbit
                    for toolbit in library:
                        if toolbit.get_uri() == toolbit_uri:
                            libraries_with_toolbit.append(library)
                            break

        except Exception as e:
            Path.Log.error(f"Error finding libraries containing toolbit {toolbit_uri}: {e}")

        return libraries_with_toolbit

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
