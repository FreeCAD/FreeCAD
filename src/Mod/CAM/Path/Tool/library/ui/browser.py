# -*- coding: utf-8 -*-
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

"""Widget for browsing Tool Library assets with filtering and sorting."""

import FreeCAD
from typing import cast, List
from PySide import QtGui, QtCore
from PySide.QtGui import QMenu, QAction, QKeySequence
import Path
import yaml
from ...assets import AssetManager, AssetUri
from ...toolbit.ui.browser import ToolBitBrowserWidget, ToolBitUriRole
from ...toolbit.serializers import YamlToolBitSerializer
from ..models.library import Library


Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


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

    def _get_state(self):
        """Gets the current library URI, selected toolbit URI, and scroll
        position."""
        current_library = self._get_current_library()
        current_library_uri_str = (
            str(current_library.get_uri()) if current_library else None
        )

        selected_toolbit_uris = []
        selected_items = self._tool_list_widget.selectedItems()
        if selected_items:
            selected_toolbit_uris = [
                item.data(ToolBitUriRole) for item in selected_items
            ]

        scroll_pos = self._tool_list_widget.verticalScrollBar().value()

        return {
            "library_uri": current_library_uri_str,
            "toolbit_uris": selected_toolbit_uris,
            "scroll_pos": scroll_pos,
        }

    def _set_state(self, selection_data):
        """Restores the library selection, toolbit selection, and scroll
        position."""
        library_uri_str = selection_data.get("library_uri")
        toolbit_uris = selection_data.get("toolbit_uris", [])
        scroll_pos = selection_data.get("scroll_pos", 0)

        # Restore library selection
        if library_uri_str:
            for i in range(self._library_combo.count()):
                item_data = self._library_combo.itemData(i)
                if isinstance(item_data, Library):
                    if str(item_data.get_uri()) == library_uri_str:
                        self._library_combo.setCurrentIndex(i)
                        # The _on_library_changed signal will be emitted automatically
                        # and will repopulate the tool list.
                        break

        # Restore toolbit selection after the tool list has been repopulated
        # by _on_library_changed.
        if toolbit_uris:
            for uri in toolbit_uris:
                for i in range(self._tool_list_widget.count()):
                    item = self._tool_list_widget.item(i)
                    if item.data(ToolBitUriRole) == uri:
                        item.setSelected(True) # Use setSelected(True)

        # Always restore scroll position
        self._tool_list_widget.verticalScrollBar().setValue(scroll_pos)

    def _select_by_uri(self, uris: List[str]):
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
                break

    def refresh(self):
        """Reads available libraries and toolbits from disk."""
        Path.Log.debug("refresh(): Fetching and populating libraries and toolbits.")
        selection_data = self._get_state()
        self._library_combo.clear()

        # Fetch the library (shallow load). Updating the combo box also
        # triggers the _on_library_changed method, which populates the
        # tool list widget with the toolbits from the selected library.
        libraries = self._asset_manager.fetch("toolbitlibrary", store=self._store_name, depth=0)
        for library in sorted(libraries, key=lambda x: x.label):
            self._library_combo.addItem(library.label, userData=library)

        self._set_state(selection_data)

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
        self._update_list()  # Display data for the selected library

    def _add_shortcuts(self):
        """Adds keyboard shortcuts for common actions."""
        Path.Log.debug("LibraryBrowserWidget._add_shortcuts: Called.")
        super()._add_shortcuts()

        cut_action = QAction(self)
        cut_action.setShortcuts(QKeySequence.Cut)
        cut_action.triggered.connect(self._on_cut_requested)
        self.addAction(cut_action)

        duplicate_action = QAction(self)
        duplicate_action.setShortcut(QKeySequence("Ctrl+D"))
        duplicate_action.triggered.connect(self._on_duplicate_requested)
        self.addAction(duplicate_action)

        remove_action = QAction(self)
        remove_action.setShortcut(QKeySequence.Delete)
        remove_action.triggered.connect(self._on_remove_from_library_requested)
        self.addAction(remove_action)

        paste_action = QAction(self)
        paste_action.setShortcuts(QKeySequence.Paste)
        paste_action.triggered.connect(self._on_paste_requested)
        self.addAction(paste_action)

    def _show_context_menu(self, position):
        """Shows the context menu at the given position."""
        context_menu = QMenu(self)

        selected_items = self._tool_list_widget.selectedItems()
        has_selection = bool(selected_items)

        # Add actions in the desired order
        edit_action = context_menu.addAction("Edit", self._on_edit_requested)
        edit_action.setEnabled(has_selection)

        context_menu.addSeparator()

        action = context_menu.addAction("Copy", self._on_copy_requested)
        action.setShortcut(QtGui.QKeySequence("Ctrl+C"))

        action = context_menu.addAction("Cut", self._on_cut_requested)
        action.setShortcut(QtGui.QKeySequence("Ctrl+X"))

        action = context_menu.addAction("Paste", self._on_paste_requested)
        action.setShortcut(QtGui.QKeySequence("Ctrl+V"))

        # Paste is enabled if there is data in the clipboard
        clipboard = QtGui.QApplication.clipboard()
        mime_type = "application/x-freecad-toolbit-list-yaml"
        action.setEnabled(clipboard.mimeData().hasFormat(mime_type))

        action = context_menu.addAction("Duplicate", self._on_duplicate_requested)
        action.setShortcut(QtGui.QKeySequence("Ctrl+D"))

        context_menu.addSeparator()

        action = context_menu.addAction("Remove from Library",
                                       self._on_remove_from_library_requested)
        action.setShortcut(QtGui.QKeySequence.Delete)

        action = context_menu.addAction("Delete from disk",
                                       self._on_delete_requested)
        action.setShortcut(QtGui.QKeySequence("Shift+Delete"))

        # Execute the menu
        context_menu.exec_(self._tool_list_widget.mapToGlobal(position))

    def _get_current_library(self) -> Library | None:
        """Helper to get the currently selected library."""
        selected_library = self._library_combo.currentData()
        if isinstance(selected_library, Library):
            return selected_library
        return None

    def _on_cut_requested(self):
        """Handles cut request by copying and marking for removal from library."""
        mime_type = "application/x-freecad-toolbit-list-yaml"
        uris = self.get_selected_bit_uris()
        library = self._get_current_library()
        if not library or not uris:
            return

        # Copy to clipboard (handled by base class _to_clipboard)
        extra_data = {"source_library_uri": str(library.get_uri())}
        self._to_clipboard(uris, mode="cut", extra_data=extra_data)

    def _on_duplicate_requested(self):
        """Handles duplicate request by duplicating and adding to library."""
        Path.Log.debug("LibraryBrowserWidget._on_duplicate_requested: Called.\n")
        uris = self.get_selected_bit_uris()
        library = self._get_current_library()
        if not library or not uris:
            Path.Log.debug("LibraryBrowserWidget._on_duplicate_requested: No library or URIs selected. Returning.\n")
            return

        new_uris = set()
        for uri_string in uris:
            toolbit = self._asset_manager.get(AssetUri(uri_string), depth=0)
            if not toolbit:
                Path.Log.warning(f"Toolbit {uri_string} not found.\n")
                continue

            # Change the ID of the toolbit and save it to disk
            toolbit.set_id()  # Generate a new ID
            toolbit.label = toolbit.label + " (copy)"
            added_uri = self._asset_manager.add(toolbit)
            if added_uri:
                new_uris.add(str(toolbit.get_uri()))

            # Add the bit to the current library
            library.add_bit(toolbit)

        self._asset_manager.add(library) # Save the modified library
        self.refresh()

        # Select and scroll to the first duplicated toolbit
        self._select_by_uri(new_uris)

    def _on_paste_requested(self):
        """Handles paste request by adding toolbits to the current library."""
        current_library = self._get_current_library()
        if not current_library:
            return

        clipboard = QtGui.QApplication.clipboard()
        mime_type = "application/x-freecad-toolbit-list-yaml"
        mime_data = clipboard.mimeData()

        if not mime_data.hasFormat(mime_type):
            return

        try:
            clipboard_content_yaml = mime_data.data(mime_type).data().decode('utf-8')
            clipboard_data_dict = yaml.safe_load(clipboard_content_yaml)

            if not isinstance(clipboard_data_dict, dict) \
               or "toolbits" not in clipboard_data_dict \
               or not isinstance(clipboard_data_dict["toolbits"], list):
                return

            serialized_toolbits_data = clipboard_data_dict["toolbits"]
            mode = clipboard_data_dict.get("operation", "copy")
            source_library_uri_str = clipboard_data_dict.get("source_library_uri")

            if mode == "copy":
                self._on_copy_paste(current_library, serialized_toolbits_data)
            elif mode == "cut" and source_library_uri_str:
                self._on_cut_paste(
                    current_library, serialized_toolbits_data, source_library_uri_str
                )

        except Exception as e:
            Path.Log.warning(
                f"An unexpected error occurred during paste: {e}"
            )

    def _on_copy_paste(self, current_library: Library, serialized_toolbits_data: list):
        """Handles pasting toolbits that were copied."""
        new_uris = set()
        for toolbit_yaml_str in serialized_toolbits_data:
            if not isinstance(toolbit_yaml_str, str) or not toolbit_yaml_str.strip():
                continue

            toolbit_data_bytes = toolbit_yaml_str.encode('utf-8')
            toolbit = YamlToolBitSerializer.deserialize(
                toolbit_data_bytes, dependencies=None
            )
            # Assign a new tool id and a label
            toolbit.set_id()
            self._asset_manager.add(toolbit) # Save the new toolbit to disk

            # Save the bit to disk (handled by asset manager add)
            # Add the bit to the current library
            added_toolbit = current_library.add_bit(toolbit)
            if added_toolbit:
                new_uris.add(str(toolbit.get_uri()))

        if new_uris:
            self._asset_manager.add(current_library) # Save the modified library
            self.refresh()
            self._select_by_uri(new_uris)

    def _on_cut_paste(
        self,
        current_library: Library,
        serialized_toolbits_data: list,
        source_library_uri_str: str,
    ):
        """Handles pasting toolbits that were cut."""
        source_library_uri = AssetUri(source_library_uri_str)
        if source_library_uri == current_library.get_uri():
            # Cut from the same library, do nothing
            return

        try:
            source_library = cast(Library, self._asset_manager.get(
                source_library_uri, store=self._store_name, depth=1
            ))
        except FileNotFoundError:
            Path.Log.warning(
                f"Source library {source_library_uri_str} not found.\n"
            )
            return

        new_uris = set()
        for toolbit_yaml_str in serialized_toolbits_data:
            if not isinstance(toolbit_yaml_str, str) or not toolbit_yaml_str.strip():
                continue

            toolbit_data_bytes = toolbit_yaml_str.encode('utf-8')
            toolbit = YamlToolBitSerializer.deserialize(
                toolbit_data_bytes, dependencies=None
            )
            success = source_library.remove_bit(toolbit)

            # Remove it from the old library, add it to the new library
            source_library.remove_bit(toolbit)
            added_toolbit = current_library.add_bit(toolbit)
            if added_toolbit:
                new_uris.add(str(toolbit.get_uri()))

            # The toolbit itself does not change, so we don't need to save it.
            # It is only the reference in the library that changes.

        if new_uris:
            # Save the modified libraries
            self._asset_manager.add(current_library)
            self._asset_manager.add(source_library)
            self.refresh()
            self._select_by_uri(new_uris)

    def _on_remove_from_library_requested(self):
        """Handles request to remove selected toolbits from the current library."""
        Path.Log.debug("_on_remove_from_library_requested: Called.")
        uris = self.get_selected_bit_uris()
        library = self._get_current_library()
        if not library or not uris:
            return

        # Ask for confirmation
        reply = QtGui.QMessageBox.question(
            self,
            FreeCAD.Qt.translate("CAM", "Confirm Removal"),
            FreeCAD.Qt.translate("CAM", "Are you sure you want to remove the selected toolbit(s) from the library?"),
            QtGui.QMessageBox.Yes | QtGui.QMessageBox.No,
            QtGui.QMessageBox.No,
        )

        if reply == QtGui.QMessageBox.Yes:
            self._remove_toolbits_from_library(library, uris)

    def _remove_toolbits_from_library(self, library: Library, uris: List[str]):
        """Removes toolbits with the given URIs from the specified library."""
        removed_count = 0
        for uri_string in uris:
            try:
                # Remove the toolbit from the library
                library.remove_bit_by_uri(uri_string)
                removed_count += 1
            except Exception as e:
                Path.Log.error(f"Failed to remove toolbit {uri_string} from library: {e}\n")

        if removed_count > 0:
            self._asset_manager.add(library) # Save the modified library
            self.refresh()
