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
from PySide import QtCore, QtGui
from PySide.QtGui import QMenu, QAction, QKeySequence
import Path
import yaml
from ...assets import AssetManager, AssetUri
from ...toolbit import ToolBit
from ...toolbit.ui.browser import ToolBitBrowserWidget, ToolBitUriRole
from ...toolbit.serializers import YamlToolBitSerializer
from ..models.library import Library


Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())
Path.Log.trackModule(Path.Log.thisModule())


class LibraryBrowserWidget(ToolBitBrowserWidget):
    """
    A widget to browse, filter, and select Tool Library assets from the
    AssetManager, with sorting and batch insertion, using a current library.
    """

    current_library_changed = QtCore.Signal()

    def __init__(
        self,
        asset_manager: AssetManager,
        store: str = "local",
        parent=None,
        compact=True,
    ):
        super().__init__(
            asset_manager=asset_manager,
            store=store,
            parent=parent,
            tool_no_factory=self.get_tool_no_from_current_library,
            compact=compact,
        )
        self.current_library = None
        self.layout().setContentsMargins(0, 0, 0, 0)

    def _get_state(self):
        """Gets the current library URI, selected toolbit URI, and scroll
        position."""
        current_library_uri_str = (
            str(self.current_library.get_uri()) if self.current_library else None
        )

        selected_toolbit_uris = []
        selected_items = self._tool_list_widget.selectedItems()
        if selected_items:
            selected_toolbit_uris = [item.data(ToolBitUriRole) for item in selected_items]

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
            try:
                library_uri = AssetUri(library_uri_str)
                library = self._asset_manager.get(library_uri, store=self._store_name, depth=1)
                self.set_current_library(library)
            except FileNotFoundError:
                Path.Log.error(f"Library {library_uri_str} not found.")
                self.set_current_library(None)
        else:
            self.set_current_library(None)

        # Restore toolbit selection
        if toolbit_uris:
            for uri in toolbit_uris:
                for i in range(self._tool_list_widget.count()):
                    item = self._tool_list_widget.item(i)
                    if item.data(ToolBitUriRole) == uri:
                        item.setSelected(True)

        # Restore scroll position
        self._tool_list_widget.verticalScrollBar().setValue(scroll_pos)

    def refresh(self):
        """Refreshes the toolbits for the current library from disk."""
        Path.Log.debug("refresh(): Fetching and populating toolbits.")
        if self.current_library:
            library_uri = self.current_library.get_uri()
            try:
                self.current_library = self._asset_manager.get(
                    library_uri, store=self._store_name, depth=1
                )
            except FileNotFoundError:
                Path.Log.error(f"Library {library_uri} not found.")
                self.current_library = None
        self._update_tool_list()

    def get_tool_no_from_current_library(self, toolbit):
        """
        Retrieves the tool number for a toolbit based on the current library.
        """
        if self.current_library is None:
            return None
        tool_no = self.current_library.get_bit_no_from_bit(toolbit)
        return tool_no

    def set_current_library(self, library):
        """Sets the current library and updates the tool list."""
        Path.Log.track(f"called with {library}")
        self.current_library = library
        self._update_tool_list()
        self.current_library_changed.emit()

    def _update_tool_list(self):
        """Updates the tool list based on the current library."""
        if self.current_library:
            self._all_assets = [t for t in self.current_library]
            self._sort_assets()
            self._tool_list_widget.clear_list()
            self._update_list()
        else:
            self._all_assets = []
            self._tool_list_widget.clear_list()

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

        action = context_menu.addAction(
            "Remove from Library", self._on_remove_from_library_requested
        )
        action.setShortcut(QtGui.QKeySequence.Delete)

        action = context_menu.addAction("Delete from disk", self._on_delete_requested)
        action.setShortcut(QtGui.QKeySequence("Shift+Delete"))

        # Execute the menu
        context_menu.exec_(self._tool_list_widget.mapToGlobal(position))

    def get_current_library(self) -> Library | None:
        """Helper to get the current library."""
        return self.current_library

    def _on_cut_requested(self):
        """Handles cut request by copying and marking for removal from library."""
        uris = self.get_selected_bit_uris()
        library = self.get_current_library()
        if not library or not uris:
            return

        # Copy to clipboard (handled by base class _to_clipboard)
        extra_data = {"source_library_uri": str(library.get_uri())}
        self._to_clipboard(uris, mode="cut", extra_data=extra_data)

    def _on_duplicate_requested(self):
        """Handles duplicate request by duplicating and adding to library."""
        Path.Log.debug("LibraryBrowserWidget._on_duplicate_requested: Called.\n")
        uris = self.get_selected_bit_uris()
        library = self.get_current_library()
        if not library or not uris:
            Path.Log.debug(
                "LibraryBrowserWidget._on_duplicate_requested: No library or URIs selected. Returning."
            )
            return

        new_uris = set()
        for uri_string in uris:
            toolbit = cast(ToolBit, self._asset_manager.get(AssetUri(uri_string), depth=0))
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

        self._asset_manager.add(library)  # Save the modified library
        self.refresh()

        self.select_by_uri(list(new_uris))

    def _on_paste_requested(self):
        """Handles paste request by adding toolbits to the current library."""
        current_library = self.get_current_library()
        if not current_library:
            return

        clipboard = QtGui.QApplication.clipboard()
        mime_type = "application/x-freecad-toolbit-list-yaml"
        mime_data = clipboard.mimeData()

        if not mime_data.hasFormat(mime_type):
            return

        try:
            clipboard_content_yaml = mime_data.data(mime_type).data().decode("utf-8")
            clipboard_data_dict = yaml.safe_load(clipboard_content_yaml)

            if (
                not isinstance(clipboard_data_dict, dict)
                or "toolbits" not in clipboard_data_dict
                or not isinstance(clipboard_data_dict["toolbits"], list)
            ):
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
            Path.Log.warning(f"An unexpected error occurred during paste: {e}")

    def _on_copy_paste(self, current_library: Library, serialized_toolbits_data: list):
        """Handles pasting toolbits that were copied."""
        new_uris = set()
        for toolbit_yaml_str in serialized_toolbits_data:
            if not isinstance(toolbit_yaml_str, str) or not toolbit_yaml_str.strip():
                continue

            toolbit_data_bytes = toolbit_yaml_str.encode("utf-8")
            toolbit = YamlToolBitSerializer.deserialize(toolbit_data_bytes, dependencies=None)
            # Assign a new tool id and a label
            toolbit.set_id()
            self._asset_manager.add(toolbit)  # Save the new toolbit to disk

            # Add the bit to the current library
            added_toolbit = current_library.add_bit(toolbit)
            if added_toolbit:
                new_uris.add(str(toolbit.get_uri()))

        if new_uris:
            self._asset_manager.add(current_library)  # Save the modified library
            self.refresh()
            self.select_by_uri(list(new_uris))

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
            source_library = cast(
                Library,
                self._asset_manager.get(source_library_uri, store=self._store_name, depth=1),
            )
        except FileNotFoundError:
            Path.Log.warning(f"Source library {source_library_uri_str} not found.\n")
            return

        new_uris = set()
        for toolbit_yaml_str in serialized_toolbits_data:
            if not isinstance(toolbit_yaml_str, str) or not toolbit_yaml_str.strip():
                continue

            toolbit_data_bytes = toolbit_yaml_str.encode("utf-8")
            toolbit = YamlToolBitSerializer.deserialize(toolbit_data_bytes, dependencies=None)
            source_library.remove_bit(toolbit)

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
            self.select_by_uri(list(new_uris))

    def _on_remove_from_library_requested(self):
        """Handles request to remove selected toolbits from the current library."""
        Path.Log.debug("_on_remove_from_library_requested: Called.")
        uris = self.get_selected_bit_uris()
        library = self.get_current_library()
        if not library or not uris:
            return

        # Ask for confirmation
        reply = QtGui.QMessageBox.question(
            self,
            FreeCAD.Qt.translate("CAM", "Confirm Removal"),
            FreeCAD.Qt.translate(
                "CAM", "Are you sure you want to remove the selected toolbit(s) from the library?"
            ),
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
            self._asset_manager.add(library)
            self.refresh()


class LibraryBrowserWithCombo(LibraryBrowserWidget):
    """
    A widget extending LibraryBrowserWidget with a combo box for library selection.
    """

    def __init__(
        self,
        asset_manager: AssetManager,
        store: str = "local",
        parent=None,
        compact=True,
    ):
        super().__init__(
            asset_manager=asset_manager,
            store=store,
            parent=parent,
            compact=compact,
        )

        # Move search box into dedicated row to make space for the
        # library selection combo box
        layout = self.layout()
        self._top_layout.removeWidget(self._search_edit)
        layout.insertWidget(1, self._search_edit, 20)

        self._library_combo = QtGui.QComboBox()
        self._library_combo.setSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Preferred)
        self._top_layout.insertWidget(0, self._library_combo, 1)
        self._library_combo.currentIndexChanged.connect(self._on_library_combo_changed)
        self.current_library_changed.connect(self._on_current_library_changed)
        self.refresh()

    def _on_library_combo_changed(self, index):
        """Handles library selection change from the combo box."""
        Path.Log.track(f"called with index {index}")
        selected_library = cast(Library, self._library_combo.itemData(index))
        if not selected_library:
            return

        # Have to refetch the non-shallow library.
        uri = selected_library.get_uri()
        library = self._asset_manager.get(uri, store=self._store_name, depth=1)
        self.set_current_library(library)

    def _on_current_library_changed(self):
        """Updates the combo box when the current library changes externally."""
        if self.current_library:
            for i in range(self._library_combo.count()):
                lib = self._library_combo.itemData(i)
                if lib.get_uri() == self.current_library.get_uri():
                    self._library_combo.setCurrentIndex(i)
                    return
            Path.Log.warning(
                f"Current library {self.current_library.get_uri()} not found in combo box."
            )

    def refresh(self):
        """Reads available libraries and refreshes the combo box and toolbits."""
        Path.Log.debug("refresh(): Fetching and populating libraries and toolbits.")
        libraries = self._asset_manager.fetch("toolbitlibrary", store=self._store_name, depth=0)
        self._library_combo.clear()
        for library in sorted(libraries, key=lambda x: x.label):
            self._library_combo.addItem(library.label, userData=library)

        if not libraries:
            return
        if not self.current_library:
            self._library_combo.setCurrentIndex(0)

        for i in range(self._library_combo.count()):
            lib = self._library_combo.itemData(i)
            if lib.get_uri() == self.current_library.get_uri():
                self._library_combo.setCurrentIndex(i)
                break
        else:
            self._library_combo.setCurrentIndex(0)
