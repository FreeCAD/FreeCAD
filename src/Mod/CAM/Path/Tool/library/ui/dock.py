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

"""ToolBit Library Dock Widget."""

import os
import uuid as UUID
import FreeCAD
import FreeCADGui
import Path
import Path.Tool.Gui.Controller as PathToolControllerGui
import PathScripts.PathUtilsGui as PathUtilsGui
import PySide
from PySide.QtGui import QStandardItem, QStandardItemModel, QPixmap
from PySide.QtCore import Qt
from functools import partial
from typing import List, Tuple, cast
from ...assets import AssetUri
from ...camassets import cam_assets, ensure_assets_initialized
from ...toolbit import ToolBit
from .editor import LibraryEditor
from ..models.library import Library


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


class ToolBitLibraryDock(object):
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
        if selected and FreeCAD.ActiveDocument:
            jobs = len([1 for j in FreeCAD.ActiveDocument.Objects if j.Name[:3] == "Job"]) >= 1
            self.form.addToolController.setEnabled(selected and jobs)

    def libraryEditorOpen(self):
        library = LibraryEditor()
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
            tc = PathToolControllerGui.Create(f"TC: {toolbit.label}", toolbit.obj, toolNr)
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