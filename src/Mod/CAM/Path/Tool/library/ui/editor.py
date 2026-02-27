# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
# *                 2020 Schildkroet                                        *
# *                 2025 Samuel Abels <knipknap@gmail.com>                  *
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
import yaml
import pathlib
import FreeCAD
import FreeCADGui
import Path
from PySide.QtGui import (
    QStandardItem,
    QStandardItemModel,
    QPixmap,
    QDialog,
    QMessageBox,
    QWidget,
)
from PySide.QtCore import Qt, QEvent
from typing import List, cast, Tuple, Optional
from ...assets import AssetUri
from ...assets.ui import AssetOpenDialog, AssetSaveDialog
from ...camassets import cam_assets, ensure_assets_initialized
from ...shape.ui.shapeselector import ShapeSelector
from ...toolbit import ToolBit
from ...toolbit.serializers import all_serializers as toolbit_serializers
from ...toolbit.ui import ToolBitEditor
from ...toolbit.ui.toollist import ToolBitUriListMimeType
from ...toolbit.ui.util import natural_sort_key
from ...toolbit.util import setToolBitSchema
from ..serializers import all_serializers as library_serializers
from ..models import Library
from .browser import LibraryBrowserWidget
from .properties import LibraryPropertyDialog


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.ERROR, Path.Log.thisModule())


_LibraryRole = Qt.UserRole + 1
translate = FreeCAD.Qt.translate


class LibraryEditor(QWidget):
    """LibraryEditor is the controller for
    displaying/selecting/creating/editing a collection of ToolBits."""

    def __init__(self, parent=None):
        super().__init__(parent=parent)
        Path.Log.track()
        ensure_assets_initialized(cam_assets)
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/ToolBitLibraryEdit.ui")
        self.form.installEventFilter(self)  # to forward keypress events
        self._base_title = self.form.windowTitle()

        # Create the library list.
        self.listModel = QStandardItemModel()
        self.form.TableList.setModel(self.listModel)
        self.form.TableList.clicked.connect(self._on_library_selected)

        # Enable drop support for the library list
        self.form.TableList.viewport().installEventFilter(self)  # Also on viewport

        # Create the LibraryBrowserWidget
        self.browser = LibraryBrowserWidget(
            asset_manager=cam_assets,
            parent=self,
        )
        self.browser.setDragEnabled(True)
        self.form.verticalLayout_2.layout().replaceWidget(self.form.toolTable, self.browser)
        self.form.toolTable.hide()

        # Connect signals.
        self.browser.itemDoubleClicked.connect(self.browser._on_edit_requested)

        self.form.addLibraryButton.clicked.connect(self._on_add_library_requested)
        self.form.removeLibraryButton.clicked.connect(self._on_remove_library_requested)
        self.form.renameLibraryButton.clicked.connect(self._on_rename_library_requested)
        self.form.importLibraryButton.clicked.connect(self._on_import_library_requested)
        self.form.exportLibraryButton.clicked.connect(self._on_export_library_requested)

        self.form.addToolBitButton.clicked.connect(self._on_add_toolbit_requested)
        self.form.importToolBitButton.clicked.connect(self._on_import_toolbit_requested)
        self.form.exportToolBitButton.clicked.connect(self._on_export_toolbit_requested)

        # Populate the UI.
        self._refresh_library_list()
        self._select_last_library()
        self._update_button_states()

    def _highlight_row(self, index):
        """Highlights the row at the given index using the selection model."""
        if not index.isValid():
            return
        self.form.TableList.setCurrentIndex(index)

    def _clear_highlight(self):
        """Clears the highlighting from the previously highlighted row."""
        self.form.TableList.selectionModel().clear()

    def eventFilter(self, obj, event):
        if event.type() == QEvent.KeyPress and self.form.TableList.hasFocus():
            if event.key() == Qt.Key_F2:
                Path.Log.debug("F2 pressed on library list.")
                self._on_rename_library_requested()
                return True
            elif event.key() == Qt.Key_Delete:
                Path.Log.debug("Del pressed on library list.")
                self._on_remove_library_requested()
                return True
        if obj == self.form.TableList.viewport():
            if event.type() == QEvent.DragEnter or event.type() == QEvent.DragMove:
                return self._handle_drag_enter(event)
            elif event.type() == QEvent.DragLeave:
                self._handle_drag_leave(event)
                return True
            elif event.type() == QEvent.Drop:
                return self._handle_drop(event)
        return super().eventFilter(obj, event)

    def _handle_drag_enter(self, event):
        """Handle drag enter and move events for the library list."""
        mime_data = event.mimeData()
        Path.Log.debug(f"_handle_drag_enter: MIME formats: {mime_data.formats()}")
        if not mime_data.hasFormat(ToolBitUriListMimeType):
            Path.Log.debug("_handle_drag_enter: Invalid MIME type, ignoring")
            return True

        # Get the row being hovered.
        pos = event.pos()
        event.acceptProposedAction()
        index = self.form.TableList.indexAt(pos)
        if not index.isValid():
            self._clear_highlight()
            return True

        # Prevent drop into "All Toolbits"
        item = self.listModel.itemFromIndex(index)
        if not item or item.data(_LibraryRole) == "all_tools":
            self._clear_highlight()
            return True

        self._highlight_row(index)
        return True

    def _handle_drag_leave(self, event):
        """Handle drag leave event for the library list."""
        self._clear_highlight()

    def _handle_drop(self, event):
        """Handle drop events to move or copy toolbits to the target library."""
        mime_data = event.mimeData()
        if not (mime_data.hasFormat(ToolBitUriListMimeType)):
            event.ignore()
            return True

        self._clear_highlight()
        pos = event.pos()
        index = self.form.TableList.indexAt(pos)
        if not index.isValid():
            event.ignore()
            return True

        item = self.listModel.itemFromIndex(index)
        if not item or item.data(_LibraryRole) == "all_tools":
            event.ignore()
            return True

        target_library_id = item.data(_LibraryRole)
        target_library_uri = f"toolbitlibrary://{target_library_id}"
        target_library = cast(Library, cam_assets.get(target_library_uri, depth=1))

        try:
            clipboard_content_yaml = mime_data.data(ToolBitUriListMimeType).data().decode("utf-8")
            clipboard_data_dict = yaml.safe_load(clipboard_content_yaml)

            if not isinstance(clipboard_data_dict, dict) or "toolbits" not in clipboard_data_dict:
                event.ignore()
                return True

            uris = clipboard_data_dict["toolbits"]
            new_uris = set()

            # Get the current library from the browser
            current_library = self.browser.get_current_library()

            for uri in uris:
                try:
                    toolbit = cast(ToolBit, cam_assets.get(AssetUri(uri), depth=0))
                    if toolbit:
                        added_toolbit = target_library.add_bit(toolbit)
                        if added_toolbit:
                            new_uris.add(str(toolbit.get_uri()))

                            # Remove the toolbit from the current library if it exists and
                            # it's not "all_tools"
                            if current_library and current_library.get_id() != "all_tools":
                                current_library.remove_bit(toolbit)
                except Exception as e:
                    Path.Log.error(f"Failed to load toolbit from URI {uri}: {e}")
                    continue

            if new_uris:
                cam_assets.add(target_library)
                # Save the current library if it was modified
                if current_library and current_library.get_id() != "all_tools":
                    cam_assets.add(current_library)
                self.browser.refresh()
                self.browser.select_by_uri(list(new_uris))
                self._update_button_states()

            event.acceptProposedAction()
        except Exception as e:
            Path.Log.error(f"Failed to process drop event: {e}")
            event.ignore()
        return True

    def get_selected_library_id(self) -> Optional[str]:
        index = self.form.TableList.currentIndex()
        if not index.isValid():
            return None
        item = self.listModel.itemFromIndex(index)
        if not item:
            return None
        return item.data(_LibraryRole)

    def get_selected_library(self, depth=1) -> Optional[Library]:
        library_id = self.get_selected_library_id()
        if not library_id:
            return None
        uri = f"toolbitlibrary://{library_id}"
        return cast(Library, cam_assets.get(uri, depth=depth))

    def select_library_by_uri(self, uri: AssetUri):
        # Find it in the list.
        index = 0
        for i in range(self.listModel.rowCount()):
            item = self.listModel.item(i)
            if item and item.data(_LibraryRole) == uri.asset_id:
                index = i
                break
        else:
            return

        # Select it.
        if index <= self.listModel.rowCount():
            item = self.listModel.item(index)
            if item:  # Should always be true, but...
                self.form.TableList.setCurrentIndex(self.listModel.index(index, 0))
                self._on_library_selected()

    def _select_last_library(self):
        # Find the last used library.
        last_used_lib_identifier = Path.Preferences.getLastToolLibrary()
        if last_used_lib_identifier:
            uri = Library.resolve_name(last_used_lib_identifier)
            self.select_library_by_uri(uri)

    def open(self):
        Path.Log.track()
        return self.form.exec_()

    def _refresh_library_list(self):
        """Clears and repopulates the self.listModel with available libraries."""
        Path.Log.track()
        self.listModel.clear()

        # Add "All Toolbits" item
        all_tools_item = QStandardItem(translate("CAM", "All Toolbits"))
        all_tools_item.setData("all_tools", _LibraryRole)
        # all_tools_item.setIcon(QPixmap(":/icons/CAM_ToolTable.svg"))
        # Make the "All Toolbits" item bold and italic
        font = all_tools_item.font()
        font.setBold(True)
        font.setItalic(True)
        all_tools_item.setFont(font)
        self.listModel.appendRow(all_tools_item)

        # Use AssetManager to fetch library assets (depth=0 for shallow fetch)
        try:
            # Fetch library assets themselves, not their deep dependencies (toolbits).
            # depth=0 means "fetch this asset, but not its dependencies"
            # The 'fetch' method returns actual Asset objects.
            libraries = cast(List[Library], cam_assets.fetch(asset_type="toolbitlibrary", depth=0))
        except Exception as e:
            Path.Log.error(f"Failed to fetch toolbit libraries: {e}")
            return

        # Sort by label for consistent ordering, falling back to asset_id if label is missing
        for library in sorted(
            libraries,
            key=lambda library: natural_sort_key(library.label or library.get_id()),
        ):
            lib_uri_str = str(library.get_uri())
            libItem = QStandardItem(library.label or library.get_id())
            libItem.setToolTip(f"ID: {library.get_id()}\nURI: {lib_uri_str}")
            libItem.setData(library.get_id(), _LibraryRole)  # Store the library ID
            libItem.setIcon(QPixmap(":/icons/CAM_ToolTable.svg"))
            self.listModel.appendRow(libItem)

        Path.Log.debug("model rows: {}".format(self.listModel.rowCount()))

        self.listModel.setHorizontalHeaderLabels(["Library"])

    def _on_library_selected(self):
        """Sets the current library in the browser when a library is selected."""
        Path.Log.debug("_on_library_selected: Called.")
        index = self.form.TableList.currentIndex()
        item = self.listModel.itemFromIndex(index)
        if not item:
            return
        if item.data(_LibraryRole) == "all_tools":
            selected_library = None
        else:
            selected_library = self.get_selected_library()
        self.browser.set_current_library(selected_library)
        self._update_window_title()
        self._update_button_states()

    def _update_window_title(self):
        """Updates the window title with the current library name."""
        current_library = self.browser.get_current_library()
        if current_library:
            title = f"{self._base_title} - {current_library.label}"
        else:
            title = self._base_title
        self.form.setWindowTitle(title)

    def _update_button_states(self):
        """Updates the enabled state of library management buttons."""
        library_selected = self.browser.get_current_library() is not None
        self.form.addLibraryButton.setEnabled(True)
        self.form.removeLibraryButton.setEnabled(library_selected)
        self.form.renameLibraryButton.setEnabled(library_selected)
        self.form.exportLibraryButton.setEnabled(library_selected)
        self.form.importLibraryButton.setEnabled(True)
        self.form.addToolBitButton.setEnabled(
            True
        )  # Always enabled - can create standalone toolbits
        # TODO: self.form.exportToolBitButton.setEnabled(toolbit_selected)

    def _save_library(self):
        """Internal method to save the current tool library asset"""
        Path.Log.track()
        library = self.browser.get_current_library()
        if not library:
            return

        # Save the modified library asset.
        try:
            cam_assets.add(library)
            Path.Log.debug(f"Library {library.get_uri()} saved")
        except Exception as e:
            Path.Log.error(f"Failed to save library {library.get_uri()}: {e}")
            QMessageBox.critical(
                self.form,
                translate("CAM_ToolBit", "Error Saving Library"),
                str(e),
            )
            raise

    def _on_add_library_requested(self):
        Path.Log.debug("_on_add_library_requested: Called.")
        new_library = Library(FreeCAD.Qt.translate("CAM", "New Library"))
        dialog = LibraryPropertyDialog(new_library, new=True, parent=self)
        if dialog.exec_() != QDialog.Accepted:
            return

        uri = cam_assets.add(new_library)
        Path.Log.debug(f"_on_add_library_requested: New library URI = {uri}")
        self._refresh_library_list()
        self.select_library_by_uri(uri)
        self._update_button_states()

    def _on_remove_library_requested(self):
        """Handles request to remove the selected library."""
        Path.Log.debug("_on_remove_library_requested: Called.")
        current_library = self.browser.get_current_library()
        if not current_library:
            return

        reply = QMessageBox.question(
            self,
            FreeCAD.Qt.translate("CAM", "Confirm Library Removal"),
            FreeCAD.Qt.translate(
                "CAM",
                "Are you sure you want to remove the library '{0}'?\n"
                "This will not delete the toolbits contained within it.",
            ).format(current_library.label),
            QMessageBox.Yes | QMessageBox.No,
            QMessageBox.No,
        )

        if reply != QMessageBox.Yes:
            return

        try:
            library_uri = current_library.get_uri()
            cam_assets.delete(library_uri)
            Path.Log.info(f"Library {current_library.label} deleted.")
            self._refresh_library_list()
            self.browser.refresh()
            self._update_button_states()
        except FileNotFoundError as e:
            Path.Log.error(f"Failed to delete library {current_library.label}: {e}")
            QMessageBox.critical(
                self,
                FreeCAD.Qt.translate("CAM", "Error"),
                FreeCAD.Qt.translate("CAM", "Failed to delete library '{0}': {1}").format(
                    current_library.label, str(e)
                ),
            )

    def _on_rename_library_requested(self):
        """Handles request to rename the selected library."""
        Path.Log.debug("_on_rename_library_requested: Called.")
        current_library = self.browser.get_current_library()
        if not current_library:
            return

        dialog = LibraryPropertyDialog(current_library, new=False, parent=self)
        if dialog.exec_() != QDialog.Accepted:
            return

        cam_assets.add(current_library)
        self._refresh_library_list()
        self._update_button_states()

    def _on_import_library_requested(self):
        """Handles request to import a library."""
        Path.Log.debug("_on_import_library_requested: Called.")
        dialog = AssetOpenDialog(
            cam_assets, asset_class=Library, serializers=library_serializers, parent=self
        )
        response = dialog.exec_()
        if not response:
            return
        file_path, library = cast(Tuple[pathlib.Path, Library], response)

        try:
            cam_assets.add(library)
            self._refresh_library_list()
            self._update_button_states()
        except Exception as e:
            Path.Log.error(f"Failed to import library: {file_path} {e}")
            QMessageBox.critical(
                self,
                FreeCAD.Qt.translate("CAM", "Error"),
                FreeCAD.Qt.translate("CAM", f"Failed to import library: {file_path} {e}"),
            )

    def _on_export_library_requested(self):
        """Handles request to export the selected library."""
        Path.Log.debug("_on_export_library_requested: Called.")
        current_library = self.browser.get_current_library()
        if not current_library:
            return

        dialog = AssetSaveDialog(asset_class=Library, serializers=library_serializers, parent=self)
        dialog.exec_(current_library)
        self._update_button_states()

    def _on_add_toolbit_requested(self):
        """Handles request to add a new toolbit to the current library or create standalone."""
        Path.Log.debug("_on_add_toolbit_requested: Called.")
        current_library = self.browser.get_current_library()

        # Select the shape for the new toolbit
        selector = ShapeSelector()
        shape = selector.show()
        if shape is None:  # user canceled
            return

        try:
            # Find the appropriate ToolBit subclass based on the shape name
            tool_bit_classes = {b.SHAPE_CLASS.name: b for b in ToolBit.__subclasses__()}
            tool_bit_class = tool_bit_classes.get(shape.name)
            if not tool_bit_class:
                raise ValueError(f"No ToolBit subclass found for shape '{shape.name}'")

            # Create a new ToolBit instance
            new_toolbit = tool_bit_class(shape)
            new_toolbit.label = FreeCAD.Qt.translate("CAM", "New Toolbit")

            # Save the individual toolbit asset first
            tool_asset_uri = cam_assets.add(new_toolbit)
            Path.Log.debug(f"_on_add_toolbit_requested: Saved tool with URI: {tool_asset_uri}")

            # Add the toolbit to the current library if one is selected
            if current_library:
                toolno = current_library.add_bit(new_toolbit)
                Path.Log.debug(
                    f"_on_add_toolbit_requested: Added toolbit {new_toolbit.get_id()} (URI: {new_toolbit.get_uri()}) "
                    f"to current_library with number {toolno}."
                )
                # Save the library
                cam_assets.add(current_library)
            else:
                Path.Log.debug(
                    f"_on_add_toolbit_requested: Created standalone toolbit {new_toolbit.get_id()} (URI: {new_toolbit.get_uri()})"
                )

        except Exception as e:
            Path.Log.error(f"Failed to create or add new toolbit: {e}")
            QMessageBox.critical(
                self,
                FreeCAD.Qt.translate("CAM", "Error Creating Toolbit"),
                str(e),
            )
            raise

        setToolBitSchema()  # Ensure correct schema is set for the new toolbit
        self.browser.refresh()
        self.browser.select_by_uri([str(new_toolbit.get_uri())])
        self._update_button_states()

    def _on_import_toolbit_requested(self):
        """Handles request to import a toolbit."""
        Path.Log.debug("_on_import_toolbit_requested: Called.")
        current_library = self.browser.get_current_library()
        if not current_library:
            Path.Log.warning("Cannot import toolbit: No library selected.")
            QMessageBox.warning(
                self,
                FreeCAD.Qt.translate("CAM", "Warning"),
                FreeCAD.Qt.translate("CAM", "Please select a library first."),
            )
            return

        dialog = AssetOpenDialog(
            cam_assets, asset_class=ToolBit, serializers=toolbit_serializers, parent=self
        )
        response = dialog.exec_()
        if not response:
            return
        file_path, toolbit = cast(Tuple[pathlib.Path, ToolBit], response)

        # Debug logging for imported toolbit
        Path.Log.info(
            f"IMPORT TOOLBIT: file_path={file_path}, toolbit.id={toolbit.id}, toolbit.label={toolbit.label}"
        )
        import traceback

        stack = traceback.format_stack()
        caller_info = "".join(stack[-3:-1])
        Path.Log.info(f"IMPORT TOOLBIT CALLER:\n{caller_info}")

        # Check if toolbit already exists in asset manager
        toolbit_uri = toolbit.get_uri()
        Path.Log.info(f"IMPORT CHECK: toolbit_uri={toolbit_uri}")
        existing_toolbit = None
        try:
            existing_toolbit = cam_assets.get(toolbit_uri, store=["local", "builtin"], depth=0)
            Path.Log.info(
                f"IMPORT CHECK: Toolbit {toolbit.id} already exists, using existing reference"
            )
            Path.Log.info(
                f"IMPORT CHECK: existing_toolbit.id={existing_toolbit.id}, existing_toolbit.label={existing_toolbit.label}"
            )
        except FileNotFoundError:
            # Toolbit doesn't exist, save it as new
            Path.Log.info(f"IMPORT CHECK: Toolbit {toolbit.id} is new, saving to disk")
            new_uri = cam_assets.add(toolbit)
            Path.Log.info(f"IMPORT CHECK: Toolbit saved with new URI: {new_uri}")
            existing_toolbit = toolbit

        # Add the toolbit (existing or new) to the current library
        Path.Log.info(
            f"IMPORT ADD: Adding toolbit {existing_toolbit.id} to library {current_library.label}"
        )
        added_toolbit = current_library.add_bit(existing_toolbit)
        if added_toolbit:
            Path.Log.info(f"IMPORT ADD: Successfully added toolbit to library")
            cam_assets.add(current_library)  # Save the modified library
            self.browser.refresh()
            self.browser.select_by_uri([str(existing_toolbit.get_uri())])
            self._update_button_states()
        else:
            Path.Log.warning(f"IMPORT ADD: Failed to add toolbit {existing_toolbit.id} to library")
            Path.Log.warning(
                f"IMPORT FAILED: Failed to import toolbit from {file_path} to library {current_library.label}."
            )
            QMessageBox.warning(
                self,
                FreeCAD.Qt.translate("CAM", "Warning"),
                FreeCAD.Qt.translate(
                    "CAM",
                    f"Failed to import toolbit from '{file_path}' to library '{current_library.label}'.",
                ),
            )

    def _on_export_toolbit_requested(self):
        """Handles request to export the selected toolbit."""
        Path.Log.debug("_on_export_toolbit_requested: Called.")
        selected_toolbits = self.browser.get_selected_bits()
        if not selected_toolbits:
            Path.Log.warning("Cannot export toolbit: No toolbit selected.")
            QMessageBox.warning(
                self,
                FreeCAD.Qt.translate("CAM", "Warning"),
                FreeCAD.Qt.translate("CAM", "Please select a toolbit to export."),
            )
            return

        if len(selected_toolbits) > 1:
            Path.Log.warning("Cannot export multiple toolbits: Please select only one.")
            QMessageBox.warning(
                self,
                FreeCAD.Qt.translate("CAM", "Warning"),
                FreeCAD.Qt.translate("CAM", "Please select only one toolbit to export."),
            )
            return

        toolbit_to_export = selected_toolbits[0]
        dialog = AssetSaveDialog(asset_class=ToolBit, serializers=toolbit_serializers, parent=self)
        dialog.exec_(toolbit_to_export)  # This will open the save dialog and handle the export
        self._update_button_states()
