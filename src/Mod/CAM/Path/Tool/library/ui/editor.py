# -*- coding: utf-8 -*-
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


import FreeCAD
import FreeCADGui
import Path
import PySide
from PySide.QtGui import QStandardItem, QStandardItemModel, QPixmap
from PySide.QtCore import Qt
import os
import uuid as UUID
from typing import List, cast
from ...assets import AssetUri
from ...assets.ui import AssetOpenDialog, AssetSaveDialog
from ...camassets import cam_assets, ensure_assets_initialized
from ...shape.ui.shapeselector import ShapeSelector
from ...toolbit import ToolBit
from ...toolbit.serializers import all_serializers as toolbit_serializers
from ...toolbit.ui import ToolBitEditor
from ...library import Library
from ...library.serializers import all_serializers as library_serializers


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


_UuidRole = PySide.QtCore.Qt.UserRole + 1
_PathRole = PySide.QtCore.Qt.UserRole + 2
_LibraryRole = PySide.QtCore.Qt.UserRole + 3


translate = FreeCAD.Qt.translate


class _TableView(PySide.QtGui.QTableView):
    """Subclass of QTableView to support rearrange and copying of ToolBits"""

    def __init__(self, parent):
        PySide.QtGui.QTableView.__init__(self, parent)
        self.setDragEnabled(False)
        self.setAcceptDrops(False)
        self.setDropIndicatorShown(False)
        self.setDragDropMode(PySide.QtGui.QAbstractItemView.DragOnly)
        self.setDefaultDropAction(PySide.QtCore.Qt.IgnoreAction)
        self.setSortingEnabled(True)
        self.setSelectionBehavior(PySide.QtGui.QAbstractItemView.SelectRows)
        self.verticalHeader().hide()

    def supportedDropActions(self):
        return [PySide.QtCore.Qt.CopyAction, PySide.QtCore.Qt.MoveAction]

    def _uuidOfRow(self, row):
        model = self.toolModel()
        return model.data(model.index(row, 0), _UuidRole)

    def _rowWithUuid(self, uuid):
        model = self.toolModel()
        for row in range(model.rowCount()):
            if self._uuidOfRow(row) == uuid:
                return row
        return None

    def _copyTool(self, uuid_, dstRow):
        model = self.toolModel()
        model.insertRow(dstRow)
        srcRow = self._rowWithUuid(uuid_)
        for col in range(model.columnCount()):
            srcItem = model.item(srcRow, col)

            model.setData(
                model.index(dstRow, col),
                srcItem.data(PySide.QtCore.Qt.EditRole),
                PySide.QtCore.Qt.EditRole,
            )
            if col == 0:
                model.setData(model.index(dstRow, col), srcItem.data(_PathRole), _PathRole)
                # Even a clone of a tool gets its own uuid so it can be identified when
                # rearranging the order or inserting/deleting rows
                model.setData(model.index(dstRow, col), UUID.uuid4(), _UuidRole)
            else:
                model.item(dstRow, col).setEditable(False)

    def _copyTools(self, uuids, dst):
        for i, uuid in enumerate(uuids):
            self._copyTool(uuid, dst + i)

    def dropEvent(self, event):
        """Handle drop events on the tool table"""
        Path.Log.track()
        mime = event.mimeData()
        data = mime.data("application/x-qstandarditemmodeldatalist")
        stream = PySide.QtCore.QDataStream(data)
        srcRows = []
        while not stream.atEnd():
            row = stream.readInt32()
            srcRows.append(row)

        # get the uuids of all srcRows
        model = self.toolModel()
        srcUuids = [self._uuidOfRow(row) for row in set(srcRows)]
        destRow = self.rowAt(event.pos().y())

        self._copyTools(srcUuids, destRow)
        if PySide.QtCore.Qt.DropAction.MoveAction == event.proposedAction():
            for uuid in srcUuids:
                model.removeRow(self._rowWithUuid(uuid))


class ModelFactory:
    """Helper class to generate qtdata models for toolbit libraries"""

    @staticmethod
    def find_libraries(model) -> QStandardItemModel:
        """
        Finds all the fctl files in a location.
        Returns a QStandardItemModel.
        """
        Path.Log.track()
        model.clear()

        # Use AssetManager to fetch library assets (depth=0 for shallow fetch)
        try:
            # Fetch library assets themselves, not their deep dependencies (toolbits).
            # depth=0 means "fetch this asset, but not its dependencies"
            # The 'fetch' method returns actual Asset objects.
            libraries = cast(List[Library], cam_assets.fetch(asset_type="toolbitlibrary", depth=0))
        except Exception as e:
            Path.Log.error(f"Failed to fetch toolbit libraries: {e}")
            return model  # Return empty model on error

        # Sort by label for consistent ordering, falling back to asset_id if label is missing
        def get_sort_key(library):
            label = getattr(library, "label", None)
            return label if label else library.get_id()

        for library in sorted(libraries, key=get_sort_key):
            lib_uri_str = str(library.get_uri())
            libItem = QStandardItem(library.label or library.get_id())
            libItem.setToolTip(f"ID: {library.get_id()}\nURI: {lib_uri_str}")
            libItem.setData(lib_uri_str, _LibraryRole)  # Store the URI string
            libItem.setIcon(QPixmap(":/icons/CAM_ToolTable.svg"))
            model.appendRow(libItem)

        Path.Log.debug("model rows: {}".format(model.rowCount()))
        return model

    @staticmethod
    def __library_load(library_uri: str, data_model: QStandardItemModel):
        Path.Log.track(library_uri)

        if library_uri:
            # Store the AssetUri string, not just the name
            Path.Preferences.setLastToolLibrary(library_uri)

        try:
            # Load the library asset using AssetManager
            loaded_library = cam_assets.get(AssetUri(library_uri), depth=1)
        except Exception as e:
            Path.Log.error(f"Failed to load library from {library_uri}: {e}")
            raise

        # Iterate over the loaded ToolBit asset instances
        for tool_no, tool_bit in sorted(loaded_library._bit_nos.items()):
            data_model.appendRow(
                ModelFactory._tool_add(tool_no, tool_bit.to_dict(), str(tool_bit.get_uri()))
            )

    @staticmethod
    def _generate_tooltip(toolbit: dict) -> str:
        """
        Generate an HTML tooltip for a given toolbit dictionary.

        Args:
        toolbit (dict): A dictionary containing toolbit information.

        Returns:
        str: An HTML string representing the tooltip.
        """
        tooltip = f"<b>Name:</b> {toolbit['name']}<br>"
        tooltip += f"<b>Shape File:</b> {toolbit['shape']}<br>"
        tooltip += "<b>Parameters:</b><br>"
        parameters = toolbit.get("parameter", {})
        if parameters:
            for key, value in parameters.items():
                tooltip += f"  <b>{key}:</b> {value}<br>"
        else:
            tooltip += "  No parameters provided.<br>"

        attributes = toolbit.get("attribute", {})
        if attributes:
            tooltip += "<b>Attributes:</b><br>"
            for key, value in attributes.items():
                tooltip += f"  <b>{key}:</b> {value}<br>"

        return tooltip

    @staticmethod
    def _tool_add(nr: int, tool: dict, path: str):
        str_shape = os.path.splitext(os.path.basename(tool["shape"]))[0]
        tooltip = ModelFactory._generate_tooltip(tool)

        tool_nr = QStandardItem()
        tool_nr.setData(nr, Qt.EditRole)
        tool_nr.setData(path, _PathRole)
        tool_nr.setData(UUID.uuid4(), _UuidRole)
        tool_nr.setToolTip(tooltip)

        tool_name = QStandardItem()
        tool_name.setData(tool["name"], Qt.EditRole)
        tool_name.setEditable(False)
        tool_name.setToolTip(tooltip)

        tool_shape = QStandardItem()
        tool_shape.setData(str_shape, Qt.EditRole)
        tool_shape.setEditable(False)

        return [tool_nr, tool_name, tool_shape]

    @staticmethod
    def library_open(model: QStandardItemModel, library_uri: str) -> QStandardItemModel:
        """
        Opens the tools in a library using its AssetUri.
        Returns a QStandardItemModel.
        """
        Path.Log.track(library_uri)
        ModelFactory.__library_load(library_uri, model)
        Path.Log.debug("model rows: {}".format(model.rowCount()))
        return model


class LibraryEditor(object):
    """LibraryEditor is the controller for
    displaying/selecting/creating/editing a collection of ToolBits."""

    def __init__(self):
        Path.Log.track()
        ensure_assets_initialized(cam_assets)
        self.factory = ModelFactory()
        self.toolModel = PySide.QtGui.QStandardItemModel(0, len(self.columnNames()))
        self.listModel = PySide.QtGui.QStandardItemModel()
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/ToolBitLibraryEdit.ui")
        self.toolTableView = _TableView(self.form.toolTableGroup)
        self.form.toolTableGroup.layout().replaceWidget(self.form.toolTable, self.toolTableView)
        self.form.toolTable.hide()

        self.setupUI()
        self.title = self.form.windowTitle()

        # Connect signals for tool editing
        self.toolTableView.doubleClicked.connect(self.toolEdit)

    def toolBitNew(self):
        """Create a new toolbit asset and add it to the current library"""
        Path.Log.track()

        if not self.current_library:
            PySide.QtGui.QMessageBox.warning(
                self.form,
                translate("CAM_ToolBit", "No Library Loaded"),
                translate("CAM_ToolBit", "Load or create a tool library first."),
            )
            return

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

            # Create a new ToolBit instance using the subclass constructor
            # The constructor will generate a UUID
            toolbit = tool_bit_class(shape)

            # 1. Save the individual toolbit asset first.
            tool_asset_uri = cam_assets.add(toolbit)
            Path.Log.debug(f"toolBitNew: Saved tool with URI: {tool_asset_uri}")

            # 2. Add the toolbit (which now has a persisted URI) to the current library's model
            tool_no = self.current_library.add_bit(toolbit)
            Path.Log.debug(
                f"toolBitNew: Added toolbit {toolbit.get_id()} (URI: {toolbit.get_uri()}) "
                f"to current_library with tool number {tool_no}."
            )

            # 3. Add the new tool directly to the UI model
            new_row_items = ModelFactory._tool_add(
                tool_no, toolbit.to_dict(), str(toolbit.get_uri())  # URI of the persisted toolbit
            )
            self.toolModel.appendRow(new_row_items)

            # 4. Save the library (which now references the saved toolbit)
            self.saveLibrary()

        except Exception as e:
            Path.Log.error(f"Failed to create or add new toolbit: {e}")
            PySide.QtGui.QMessageBox.critical(
                self.form,
                translate("CAM_ToolBit", "Error Creating Toolbit"),
                str(e),
            )
            raise

    def toolBitExisting(self):
        """Add an existing toolbit asset to the current library"""
        Path.Log.track()

        if not self.current_library:
            PySide.QtGui.QMessageBox.warning(
                self.form,
                translate("CAM_ToolBit", "No Library Loaded"),
                translate("CAM_ToolBit", "Load or create a tool library first."),
            )
            return

        # Open the file dialog
        dialog = AssetOpenDialog(ToolBit, toolbit_serializers, self.form)
        dialog_result = dialog.exec_()
        if not dialog_result:
            return  # User canceled or error
        file_path, toolbit = dialog_result
        toolbit = cast(ToolBit, toolbit)

        try:
            # Add the existing toolbit to the current library's model
            # The add_bit method handles assigning a tool number and returns it.
            cam_assets.add(toolbit)
            tool_no = self.current_library.add_bit(toolbit)

            # Add the new tool directly to the UI model
            new_row_items = ModelFactory._tool_add(
                tool_no, toolbit.to_dict(), str(toolbit.get_uri())  # URI of the persisted toolbit
            )
            self.toolModel.appendRow(new_row_items)

            # Save the library (which now references the added toolbit)
            # Use cam_assets.add directly for internal save on existing toolbit
            self.saveLibrary()

        except Exception as e:
            Path.Log.error(
                f"Failed to add imported toolbit {toolbit.get_id()} "
                f"from {file_path} to library: {e}"
            )
            PySide.QtGui.QMessageBox.critical(
                self.form,
                translate("CAM_ToolBit", "Error Adding Imported Toolbit"),
                str(e),
            )
            raise

    def toolDelete(self):
        """Delete a tool"""
        Path.Log.track()
        selected_indices = self.toolTableView.selectedIndexes()
        if not selected_indices:
            return

        if not self.current_library:
            Path.Log.error("toolDelete: No current_library loaded. Cannot delete tools.")
            return

        # Collect unique rows to process, as selectedIndexes can return multiple indices per row
        selected_rows = sorted(list(set(index.row() for index in selected_indices)), reverse=True)

        # Remove the rows from the library model.
        for row in selected_rows:
            item_tool_nr_or_uri = self.toolModel.item(row, 0)  # Column 0 stores _PathRole
            tool_uri_string = item_tool_nr_or_uri.data(_PathRole)
            tool_uri = AssetUri(tool_uri_string)
            bit = self.current_library.get_tool_by_uri(tool_uri)
            self.current_library.remove_bit(bit)
            self.toolModel.removeRows(row, 1)

        Path.Log.info(f"toolDelete: Removed {len(selected_rows)} rows from UI model.")

        # Save the library after deleting a tool
        self.saveLibrary()

    def toolSelect(self, selected, deselected):
        sel = len(self.toolTableView.selectedIndexes()) > 0
        self.form.toolDelete.setEnabled(sel)

    def tableSelected(self, index):
        """loads the tools for the selected tool table"""
        Path.Log.track()
        item = index.model().itemFromIndex(index)
        library_uri_string = item.data(_LibraryRole)
        self._loadSelectedLibraryTools(library_uri_string)

    def open(self):
        Path.Log.track()
        return self.form.exec_()

    def toolEdit(self, selected):
        """Edit the selected tool bit asset"""
        Path.Log.track()
        item = self.toolModel.item(selected.row(), 0)

        if selected.column() == 0:
            return  # Assuming tool number editing is handled directly in the table model

        toolbit_uri_string = item.data(_PathRole)
        if not toolbit_uri_string:
            Path.Log.error("No toolbit URI found for selected item.")
            return
        toolbit_uri = AssetUri(toolbit_uri_string)

        # Load the toolbit asset for editing
        try:
            bit = cast(ToolBit, cam_assets.get(toolbit_uri))
            editor_dialog = ToolBitEditor(bit, self.form)  # Create dialog instance
            result = editor_dialog.show()  # Show as modal dialog

            if result == PySide.QtGui.QDialog.Accepted:
                # The editor updates the toolbit directly, so we just need to save
                cam_assets.add(bit)
                Path.Log.info(f"Toolbit {bit.get_id()} saved.")
                # Refresh the display and save the library
                self._loadSelectedLibraryTools(
                    self.current_library.get_uri() if self.current_library else None
                )
                # Save the library after editing a toolbit
                self.saveLibrary()

        except Exception as e:
            Path.Log.error(f"Failed to load or edit toolbit asset {toolbit_uri_string}: {e}")
            PySide.QtGui.QMessageBox.critical(
                self.form,
                translate("CAM_ToolBit", "Error Editing Toolbit"),
                str(e),
            )
            raise

    def libraryNew(self):
        """Create a new tool library asset"""
        Path.Log.track()

        # Get the desired library name (label) from the user
        library_label, ok = PySide.QtGui.QInputDialog.getText(
            self.form,
            translate("CAM_ToolBit", "New Tool Library"),
            translate("CAM_ToolBit", "Enter a name for the new library:"),
        )
        if not ok or not library_label:
            return

        # Create a new Library asset instance, UUID will be auto-generated
        new_library = Library(library_label)
        uri = cam_assets.add(new_library)
        Path.Log.info(f"New library created: {uri}")

        # Refresh the list of libraries in the UI
        self._refreshLibraryListModel()
        self._loadSelectedLibraryTools(uri)

        # Attempt to select the newly added library in the list
        for i in range(self.listModel.rowCount()):
            item = self.listModel.item(i)
            if item and item.data(_LibraryRole) == str(uri):
                curIndex = self.listModel.indexFromItem(item)
                self.form.TableList.setCurrentIndex(curIndex)
                Path.Log.debug(f"libraryNew: Selected new library '{str(uri)}' in TableList.")
                break

    def _refreshLibraryListModel(self):
        """Clears and repopulates the self.listModel with available libraries."""
        Path.Log.track()
        self.listModel.clear()
        self.factory.find_libraries(self.listModel)
        self.listModel.setHorizontalHeaderLabels(["Library"])

    def saveLibrary(self):
        """Internal method to save the current tool library asset"""
        Path.Log.track()
        if not self.current_library:
            Path.Log.warning("saveLibrary: No library asset loaded to save.")
            return

        # Create a new dictionary to hold the updated tool numbers and bits
        for row in range(self.toolModel.rowCount()):
            tool_nr_item = self.toolModel.item(row, 0)
            tool_uri_item = self.toolModel.item(
                row, 0
            )  # Tool URI is stored in column 0 with _PathRole

            tool_nr = tool_nr_item.data(Qt.EditRole)
            tool_uri_string = tool_uri_item.data(_PathRole)

            if tool_nr is not None and tool_uri_string:
                try:
                    tool_uri = AssetUri(tool_uri_string)
                    # Retrieve the toolbit using the public method
                    found_bit = self.current_library.get_tool_by_uri(tool_uri)

                    if found_bit:
                        # Use assign_new_bit_no to update the tool number
                        # This method modifies the library in place
                        self.current_library.assign_new_bit_no(found_bit, int(tool_nr))
                        Path.Log.debug(f"Assigned tool number {tool_nr} to {tool_uri_string}")
                    else:
                        Path.Log.warning(
                            f"Toolbit with URI {tool_uri_string} not found in current library."
                        )
                except Exception as e:
                    Path.Log.error(
                        f"Error processing row {row} (tool_nr: {tool_nr}, uri: {tool_uri_string}): {e}"
                    )
                    # Continue processing other rows even if one fails
                    continue
            else:
                Path.Log.warning(f"Skipping row {row}: Invalid tool number or URI.")

        # The current_library object has been modified in the loop by assign_new_bit_no
        # Now save the modified library asset
        try:
            cam_assets.add(self.current_library)
            Path.Log.debug(f"saveLibrary: Library " f"{self.current_library.get_uri()} saved.")
        except Exception as e:
            Path.Log.error(
                f"saveLibrary: Failed to save library " f"{self.current_library.get_uri()}: {e}"
            )
            PySide.QtGui.QMessageBox.critical(
                self.form,
                translate("CAM_ToolBit", "Error Saving Library"),
                str(e),
            )
            raise

    def exportLibrary(self):
        """Export the current tool library asset to a file"""
        Path.Log.track()
        if not self.current_library:
            PySide.QtGui.QMessageBox.warning(
                self.form,
                translate("CAM_ToolBit", "No Library Loaded"),
                translate("CAM_ToolBit", "Load or create a tool library first."),
            )
            return

        dialog = AssetSaveDialog(Library, library_serializers, self.form)
        dialog_result = dialog.exec_(self.current_library)
        if not dialog_result:
            return  # User canceled or error

        file_path, serializer_class = dialog_result

        Path.Log.info(
            f"Exported library {self.current_library.label} "
            f"to {file_path} using serializer {serializer_class.__name__}"
        )

    def columnNames(self):
        return [
            "Tn",
            translate("CAM_ToolBit", "Tool"),
            translate("CAM_ToolBit", "Shape"),
        ]

    def _loadSelectedLibraryTools(self, library_uri: AssetUri | str | None = None):
        """Loads tools for the given library_uri into self.toolModel and selects it in the list."""
        Path.Log.track(library_uri)
        self.toolModel.clear()
        # library_uri is now expected to be a string URI or None when called from setupUI/tableSelected.
        # AssetUri object conversion is handled by cam_assets.get() if needed.

        self.current_library = None  # Reset current_library before loading

        if not library_uri:
            self.form.setWindowTitle("Tool Library Editor - No Library Selected")
            return

        # Fetch the library from the asset manager
        try:
            self.current_library = cam_assets.get(library_uri, depth=1)
        except Exception as e:
            Path.Log.error(f"Failed to load library asset {library_uri}: {e}")
            self.form.setWindowTitle("Tool Library Editor - Error")
            return

        # Success! Add the tools to the toolModel.
        self.toolTableView.setUpdatesEnabled(False)
        self.form.setWindowTitle(f"Tool Library Editor - {self.current_library.label}")
        for tool_no, tool_bit in sorted(self.current_library._bit_nos.items()):
            self.toolModel.appendRow(
                ModelFactory._tool_add(tool_no, tool_bit.to_dict(), str(tool_bit.get_uri()))
            )

        self.toolModel.setHorizontalHeaderLabels(self.columnNames())
        self.toolTableView.setUpdatesEnabled(True)

    def setupUI(self):
        """Setup the form and load the tool library data"""
        Path.Log.track()

        self.form.TableList.setModel(self.listModel)
        self._refreshLibraryListModel()

        self.toolTableView.setModel(self.toolModel)

        # Find the last used library.
        last_used_lib_identifier = Path.Preferences.getLastToolLibrary()
        Path.Log.debug(
            f"setupUI: Last used library identifier from prefs: '{last_used_lib_identifier}'"
        )
        last_used_lib_uri = None
        if last_used_lib_identifier:
            last_used_lib_uri = Library.resolve_name(last_used_lib_identifier)

        # Find it in the list.
        index = 0
        for i in range(self.listModel.rowCount()):
            item = self.listModel.item(i)
            if item and item.data(_LibraryRole) == str(last_used_lib_uri):
                index = i
                break

        # Select it.
        if index <= self.listModel.rowCount():
            item = self.listModel.item(index)
            if item:  # Should always be true, but...
                library_uri_str = item.data(_LibraryRole)
                self.form.TableList.setCurrentIndex(self.listModel.index(index, 0))

                # Load tools for the selected library.
                self._loadSelectedLibraryTools(library_uri_str)

        self.toolTableView.resizeColumnsToContents()
        self.toolTableView.selectionModel().selectionChanged.connect(self.toolSelect)

        self.form.TableList.clicked.connect(self.tableSelected)

        self.form.toolAdd.clicked.connect(self.toolBitExisting)
        self.form.toolDelete.clicked.connect(self.toolDelete)
        self.form.toolCreate.clicked.connect(self.toolBitNew)

        self.form.addLibrary.clicked.connect(self.libraryNew)
        self.form.exportLibrary.clicked.connect(self.exportLibrary)
        self.form.saveLibrary.clicked.connect(self.saveLibrary)

        self.form.okButton.clicked.connect(self.form.close)

        self.toolSelect([], [])
