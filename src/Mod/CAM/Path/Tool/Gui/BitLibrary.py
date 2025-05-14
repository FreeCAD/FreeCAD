# -*- coding: utf-8 -*-
# ***************************************************************************
# *   Copyright (c) 2019 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2020 Schildkroet                                        *
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
import Path.Tool.Gui.BitEdit as PathToolBitEdit
import Path.Tool.Gui.Controller as PathToolControllerGui
import PathScripts.PathUtilsGui as PathUtilsGui
import PySide
from PySide.QtGui import QStandardItem, QStandardItemModel, QPixmap
from PySide.QtCore import Qt
import os
import uuid as UUID
from functools import partial
from typing import List, Tuple
from ..assets import AssetUri
from ..camassets import cam_assets, ensure_assets_initialized
from ..shape.ui.shapeselector import ShapeSelector
from ..toolbit import ToolBit
from ..toolbit.ui.dialog import ToolBitOpenDialog
from ..library import Library


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
            libraries = cam_assets.fetch(asset_type="toolbitlibrary", depth=0)
        except Exception as e:
            Path.Log.error(f"Failed to fetch toolbit libraries: {e}")
            return model # Return empty model on error

        # Sort by label for consistent ordering, falling back to asset_id if label is missing
        def get_sort_key(library):
            label = getattr(library, 'label', None)
            return label if label else library.get_id()

        for library in sorted(libraries, key=get_sort_key):
            lib_uri_str = str(library.get_uri())
            libItem = QStandardItem(library.label or library.get_id())
            libItem.setToolTip(f"ID: {library.get_id()}\nURI: {lib_uri_str}")
            libItem.setData(lib_uri_str, _LibraryRole) # Store the URI string
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
                ModelFactory._tool_add(
                    tool_no,
                    tool_bit.to_dict(),
                    str(tool_bit.get_uri())
                )
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


class ToolBitSelector(object):
    """Controller for displaying a library and creating ToolControllers"""

    def __init__(self):
        ensure_assets_initialized(cam_assets)
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/ToolBitSelector.ui")
        self.factory = ModelFactory()
        self.toolModel = PySide.QtGui.QStandardItemModel(0, len(self.columnNames()))
        self.libraryModel = PySide.QtGui.QStandardItemModel(0, len(self.columnNames()))

        self.setupUI()
        self.title = self.form.windowTitle()

    def columnNames(self):
        """Define the column names to display"""
        return ["#", "Tool"]

    def loadData(self):
        """Load the toolbits for the selected tool library"""
        Path.Log.track()
        self.toolModel.clear()
        self.toolModel.setHorizontalHeaderLabels(self.columnNames())

        # Get the currently selected index in the combobox
        currentIndex = self.form.cboLibraries.currentIndex()

        if currentIndex != -1:
            # Get the data for the selected index
            library = self.libraryModel.item(currentIndex).data(_LibraryRole)
            self.factory.library_open(self.toolModel, library)

        self.toolModel.takeColumn(3)
        self.toolModel.takeColumn(2)

    def loadToolLibraries(self):
        """
        Load the tool libraries in to self.libraryModel
        and populate the tooldock form combobox with the
        libraries names
        """
        Path.Log.track()

        # Get the current library so we can try and maintain any previous selection
        current_lib = Path.Preferences.getLastToolLibrary()

        # load the tool libraries
        self.factory.find_libraries(self.libraryModel)

        # Set the library model to the combobox
        self.form.cboLibraries.setModel(self.libraryModel)

        # Set the current library as the selected item in the combobox
        if self.libraryModel.rowCount() == 0:
            return
        currentIndex = self.form.cboLibraries.findText(current_lib)
        currentIndex = max(0, currentIndex)  # select first item by default
        self.form.cboLibraries.setCurrentIndex(currentIndex)

    def setupUI(self):
        """Setup the form and load the tooltable data"""
        Path.Log.track()

        # Connect the library change to reload data and update tooltip
        self.form.cboLibraries.currentIndexChanged.connect(self.loadData)
        self.form.cboLibraries.currentIndexChanged.connect(self.updateLibraryTooltip)

        # Load the tool libraries.
        # This will trigger a change in current index of the cboLibraries combobox
        self.loadToolLibraries()

        self.form.tools.setModel(self.toolModel)
        self.form.tools.selectionModel().selectionChanged.connect(self.enableButtons)
        self.form.tools.doubleClicked.connect(partial(self.selectedOrAllToolControllers))

        self.form.libraryEditorOpen.clicked.connect(self.libraryEditorOpen)
        self.form.addToolController.clicked.connect(self.selectedOrAllToolControllers)

    def updateLibraryTooltip(self, index):
        """Add a tooltip to the combobox"""
        if index != -1:
            item = self.libraryModel.item(index)
            if item:
                library = item.data(_LibraryRole)
                self.form.cboLibraries.setToolTip(f"{library}")
            else:
                self.form.cboLibraries.setToolTip(translate("CAM_Toolbit", "Select a library"))
        else:
            self.form.cboLibraries.setToolTip(translate("CAM_Toolbit", "No library selected"))

    def enableButtons(self):
        """Enable button to add tool controller when a tool is selected"""
        # Set buttons inactive
        self.form.addToolController.setEnabled(False)
        selected = len(self.form.tools.selectedIndexes()) >= 1
        if selected:
            jobs = len([1 for j in FreeCAD.ActiveDocument.Objects if j.Name[:3] == "Job"]) >= 1
            self.form.addToolController.setEnabled(selected and jobs)

    def libraryEditorOpen(self):
        library = ToolBitLibrary()
        library.open()
        self.loadToolLibraries()

    def selectedOrAllTools(self) -> List[Tuple[int, ToolBit]]:
        """
        Iterate the selection and get individual toolbit assets
        If a group is selected, iterate and get children
        """
        Path.Log.track()
        itemsToProcess = []
        for index in self.form.tools.selectedIndexes():
            item = index.model().itemFromIndex(index)

            if item.hasChildren():
                for i in range(item.rowCount()): # Iterate through all children
                    child_item = item.child(i, 0) # Assuming tool number is in column 0
                    if child_item:
                         itemsToProcess.append(child_item)

            elif item.column() == 0: # Ensure it's the tool number column
                itemsToProcess.append(item)

        tools = []
        for item in itemsToProcess:
            toolNr = int(item.data(PySide.QtCore.Qt.EditRole))
            tool_uri_string = item.data(_PathRole)
            if tool_uri_string:
                try:
                    toolbit = cam_assets.get(AssetUri(tool_uri_string))
                    toolbit.attach_to_doc(FreeCAD.ActiveDocument)
                    tools.append((toolNr, toolbit))
                except Exception as e:
                    Path.Log.error(f"Failed to load toolbit asset {tool_uri_string}: {e}")
                    # Optionally inform the user with a QMessageBox
        return tools

    def selectedOrAllToolControllers(self, index=None):
        """
        if no jobs, don't do anything, otherwise all TCs for all
        selected toolbit assets
        """
        Path.Log.track()
        jobs = PathUtilsGui.PathUtils.GetJobs()
        if len(jobs) == 0:
            PySide.QtGui.QMessageBox.information(
                self.form,
                translate("CAM_ToolBit", "No Job Found"),
                translate("CAM_ToolBit", "Please create a Job first."),
            )
            return
        elif len(jobs) == 1:
            job = jobs[0]
        else:
            userinput = PathUtilsGui.PathUtilsUserInput()
            job = userinput.chooseJob(jobs)

        if job is None:  # user may have canceled
            return

        # Get the selected toolbit assets
        selected_tools = self.selectedOrAllTools()

        for toolNr, toolbit in selected_tools:
            tc = PathToolControllerGui.Create(f"TC: {toolbit.get_label()}", toolbit.obj, toolNr)
            job.Proxy.addToolController(tc)
            FreeCAD.ActiveDocument.recompute()

    def open(self, path=None):
        """load library stored in path and bring up ui"""
        docs = FreeCADGui.getMainWindow().findChildren(PySide.QtGui.QDockWidget)
        for doc in docs:
            if doc.objectName() == "ToolSelector":
                if doc.isVisible():
                    doc.deleteLater()
                    return
                else:
                    doc.setVisible(True)
                    return

        mw = FreeCADGui.getMainWindow()
        mw.addDockWidget(
            PySide.QtCore.Qt.RightDockWidgetArea,
            self.form,
            PySide.QtCore.Qt.Orientation.Vertical,
        )


class ToolBitLibrary(object):
    """ToolBitLibrary is the controller for
    displaying/selecting/creating/editing a collection of ToolBits."""

    def __init__(self):
        Path.Log.track()
        ensure_assets_initialized(cam_assets)
        self.factory = ModelFactory()
        self.editing_toolbit = None # Store the ToolBit asset being edited
        self.toolModel = PySide.QtGui.QStandardItemModel(0, len(self.columnNames()))
        self.listModel = PySide.QtGui.QStandardItemModel()
        self.form = FreeCADGui.PySideUic.loadUi(":/panels/ToolBitLibraryEdit.ui")
        self.toolTableView = _TableView(self.form.toolTableGroup)
        self.form.toolTableGroup.layout().replaceWidget(self.form.toolTable, self.toolTableView)
        self.form.toolTable.hide()
        self.setupUI()
        self.title = self.form.windowTitle()

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
                tool_no,
                toolbit.to_dict(),
                str(toolbit.get_uri()) # URI of the persisted toolbit
            )
            self.toolModel.appendRow(new_row_items)
            
            # 4. Save the library (which now references the saved toolbit)
            self.librarySave()
            Path.Log.info(f"toolBitNew: Library saved. Tool ID: {toolbit.get_id()}")

        except Exception as e:
            Path.Log.error(f"Failed to create or add new toolbit: {e}")
            PySide.QtGui.QMessageBox.critical(
                self.form,
                translate("CAM_ToolBit", "Error Creating Toolbit"),
                str(e),
            )

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
        dialog = ToolBitOpenDialog(self.form)
        dialog_result = dialog.exec()
        if not dialog_result:
            return  # User canceled or error
        file_path, toolbit = dialog_result

        try:
            # Add the existing toolbit to the current library's model
            # The add_bit method handles assigning a tool number and returns it.
            cam_assets.add(toolbit)
            tool_no = self.current_library.add_bit(toolbit)

            # Add the new tool directly to the UI model
            new_row_items = ModelFactory._tool_add(
                tool_no,
                toolbit.to_dict(),
                str(toolbit.get_uri()) # URI of the persisted toolbit
            )
            self.toolModel.appendRow(new_row_items)

            # Save the library (which now references the added toolbit)
            self.librarySave()

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
            item_tool_nr_or_uri = self.toolModel.item(row, 0) # Column 0 stores _PathRole
            tool_uri_string = item_tool_nr_or_uri.data(_PathRole)
            tool_uri = AssetUri(tool_uri_string)
            bit = self.current_library.get_tool_by_uri(tool_uri)
            self.current_library.remove_bit(bit)
            self.toolModel.removeRows(row, 1)

        Path.Log.info(f"toolDelete: Removed {len(selected_rows)} rows from UI model.")

        self.librarySave()

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

    def cleanupDocument(self):
        """Clean up the tool editing state"""
        Path.Log.track()
        # Remove the editor from the dialog
        widget = self.form.toolTableGroup.children()[-1]
        widget.setParent(None)
        self.editor = None
        self.editing_toolbit = None
        self.lockoff()

    def accept(self):
        """Handle accept signal for tool editing"""
        Path.Log.track()
        if not self.editor or not self.editing_toolbit:
            self.cleanupDocument()
            return

        try:
            # Assuming editor.accept() updates the editing_toolbit
            self.editor.accept()
            # Save the modified toolbit asset
            cam_assets.add(self.editing_toolbit)
            Path.Log.info(f"Toolbit {self.editing_toolbit.get_id()} saved.")
        except Exception as e:
            Path.Log.error(f"Failed to save toolbit {self.editing_toolbit.get_id()}: {e}")
            PySide.QtGui.QMessageBox.critical(
                self.form,
                translate("CAM_ToolBit", "Error Saving Toolbit"),
                str(e),
            )
            raise

        # Refresh the display and save the library (which contains the updated toolbit reference)
        self._loadSelectedLibraryTools(self.current_library.get_uri() if self.current_library else None)
        self.librarySave()
        self.cleanupDocument()

    def reject(self):
        """Handle reject signal"""
        self.cleanupDocument()

    def lockon(self):
        """Set the state of the form widgets: inactive"""
        self.toolTableView.setEnabled(False)
        self.form.toolCreate.setEnabled(False)
        self.form.toolDelete.setEnabled(False)
        self.form.toolAdd.setEnabled(False)
        self.form.TableList.setEnabled(False)
        self.form.libraryExport.setEnabled(False)
        self.form.addToolTable.setEnabled(False)
        self.form.librarySave.setEnabled(False)

    def lockoff(self):
        """Set the state of the form widgets: active"""
        self.toolTableView.setEnabled(True)
        self.form.toolCreate.setEnabled(True)
        self.form.toolDelete.setEnabled(True)
        self.form.toolAdd.setEnabled(True)
        self.form.toolTable.setEnabled(True)
        self.form.TableList.setEnabled(True)
        self.form.libraryExport.setEnabled(True)
        self.form.addToolTable.setEnabled(True)
        self.form.librarySave.setEnabled(True)

    def toolEdit(self, selected):
        """Edit the selected tool bit asset"""
        Path.Log.track()
        item = self.toolModel.item(selected.row(), 0)

        if self.editing_toolbit is not None:
            Path.Log.warning("Already editing a toolbit. Please finish or cancel the current edit.")
            return # Prevent multiple editors

        if selected.column() == 0:
            return  # Assuming tool number editing is handled directly in the table model

        toolbit_uri_string = item.data(_PathRole)
        if not toolbit_uri_string:
            Path.Log.error("No toolbit URI found for selected item.")
            return
        toolbit_uri = AssetUri(toolbit_uri_string)

        # Load the toolbit asset for editing
        try:
            self.editing_toolbit = cam_assets.get(toolbit_uri)

            # Initialize the editor with the toolbit asset
            # Assuming PathToolBitEdit.ToolBitEditor can accept a ToolBit asset instance
            self.editor = PathToolBitEdit.ToolBitEditor(
                self.editing_toolbit, self.form.toolTableGroup, loadBitBody=False
            )
        except Exception as e:
            Path.Log.error(f"Failed to load or edit toolbit asset {toolbit_uri_string}: {e}")
            PySide.QtGui.QMessageBox.critical(
                self.form,
                translate("CAM_ToolBit", "Error Editing Toolbit"),
                str(e),
            )
            self.cleanupDocument() # Clean up if editor initialization fails
            raise

        QBtn = PySide.QtGui.QDialogButtonBox.Ok | PySide.QtGui.QDialogButtonBox.Cancel
        buttonBox = PySide.QtGui.QDialogButtonBox(QBtn)
        buttonBox.accepted.connect(self.accept)
        buttonBox.rejected.connect(self.reject)

        layout = self.editor.form.layout()
        layout.addWidget(buttonBox)
        self.lockon()
        self.editor.setupUI()

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

    def librarySave(self):
        """Save the current tool library asset"""
        Path.Log.track()
        if not self.current_library:
            Path.Log.warning("No library asset loaded to save.")
            return
        
        try:
            cam_assets.add(self.current_library)
        except Exception as e:
            Path.Log.error(f"Failed to save library {self.current_library.get_uri()}: {e}")
            raise

    def libraryOk(self):
        self.librarySave()
        self.form.close()

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

        self.current_library = None # Reset current_library before loading

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
        self.form.setWindowTitle(
            f"Tool Library Editor - {self.current_library.label}"
        )
        for tool_no, tool_bit in sorted(self.current_library._bit_nos.items()):
            self.toolModel.appendRow(
                ModelFactory._tool_add(
                    tool_no,
                    tool_bit.to_dict(),
                    str(tool_bit.get_uri())
                )
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
        Path.Log.debug(f"setupUI: Last used library identifier from prefs: '{last_used_lib_identifier}'")
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
        # No libraries in list, or something went wrong. Load with None.
        #Path.Log.debug("setupUI: No libraries in list or no initial selection. Loading empty tool model.")
        #self._loadSelectedLibraryTools(None)

        self.toolTableView.resizeColumnsToContents()
        self.toolTableView.selectionModel().selectionChanged.connect(self.toolSelect)
        self.toolTableView.doubleClicked.connect(self.toolEdit)

        self.form.TableList.clicked.connect(self.tableSelected)

        self.form.toolAdd.clicked.connect(self.toolBitExisting)
        self.form.toolDelete.clicked.connect(self.toolDelete)
        self.form.toolCreate.clicked.connect(self.toolBitNew)

        self.form.addToolTable.clicked.connect(self.libraryNew)

        self.form.librarySave.clicked.connect(self.libraryOk)

        self.toolSelect([], [])
