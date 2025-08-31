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

"""ToolBit Library Dock Widget."""
import FreeCAD
import FreeCADGui
import Path
import Path.Tool.Gui.Controller as PathToolControllerGui
import PathScripts.PathUtilsGui as PathUtilsGui
from PySide import QtGui, QtCore
from functools import partial
from typing import List, Tuple
from ...camassets import cam_assets, ensure_assets_initialized
from ...toolbit import ToolBit
from .editor import LibraryEditor
from .browser import LibraryBrowserWidget


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


translate = FreeCAD.Qt.translate


class ToolBitLibraryDock(object):
    """Controller for displaying a library and creating ToolControllers"""

    def __init__(self):
        ensure_assets_initialized(cam_assets)
        # Create the main form widget directly
        self.form = QtGui.QDockWidget()
        self.form.setObjectName("ToolSelector")
        self.form.setWindowTitle(translate("CAM_ToolBit", "Tool Selector"))

        # Create the browser widget
        self.browser_widget = LibraryBrowserWidget(asset_manager=cam_assets)

        self._setup_ui()

    def _setup_ui(self):
        """Setup the form and load the tooltable data"""
        Path.Log.track()

        # Create a main widget and layout for the dock
        main_widget = QtGui.QWidget()
        main_layout = QtGui.QVBoxLayout(main_widget)

        # Add the browser widget to the layout
        main_layout.addWidget(self.browser_widget)

        # Create buttons
        self.libraryEditorOpenButton = QtGui.QPushButton(
            translate("CAM_ToolBit", "Open Library Editor")
        )
        self.addToolControllerButton = QtGui.QPushButton(translate("CAM_ToolBit", "Add to Job"))

        # Add buttons to a horizontal layout
        button_layout = QtGui.QHBoxLayout()
        button_layout.addWidget(self.libraryEditorOpenButton)
        button_layout.addWidget(self.addToolControllerButton)

        # Add the button layout to the main layout
        main_layout.addLayout(button_layout)

        # Set the main widget as the dock's widget
        self.form.setWidget(main_widget)

        # Connect signals from the browser widget and buttons
        self.browser_widget.toolSelected.connect(self._update_state)
        self.browser_widget.itemDoubleClicked.connect(partial(self._add_tool_controller_to_doc))
        self.libraryEditorOpenButton.clicked.connect(self._open_editor)
        self.addToolControllerButton.clicked.connect(partial(self._add_tool_controller_to_doc))

        # Initial state of buttons
        self._update_state()

    def _update_state(self):
        """Enable button to add tool controller when a tool is selected"""
        # Set buttons inactive
        self.addToolControllerButton.setEnabled(False)
        # Check if any tool is selected in the browser widget
        selected = self.browser_widget._tool_list_widget.selectedItems()
        if selected and FreeCAD.ActiveDocument:
            jobs = len([1 for j in FreeCAD.ActiveDocument.Objects if j.Name[:3] == "Job"]) >= 1
            self.addToolControllerButton.setEnabled(len(selected) >= 1 and jobs)

    def _open_editor(self):
        library = LibraryEditor()
        library.open()
        # After editing, we might need to refresh the libraries in the browser widget
        # Assuming _populate_libraries is the correct method to call
        self.browser_widget.refresh()

    def _add_tool_to_doc(self) -> List[Tuple[int, ToolBit]]:
        """
        Get the selected toolbit assets from the browser widget.
        """
        Path.Log.track()
        tools = []
        selected_toolbits = self.browser_widget.get_selected_bits()

        for toolbit in selected_toolbits:
            # Need to get the tool number for this toolbit from the currently
            # selected library in the browser widget.
            toolNr = self.browser_widget.get_tool_no_from_current_library(toolbit)
            if toolNr is not None:
                toolbit.attach_to_doc(FreeCAD.ActiveDocument)
                tools.append((toolNr, toolbit))
            else:
                Path.Log.warning(
                    f"Could not get tool number for toolbit {toolbit.get_uri()} in selected library."
                )

        return tools

    def _add_tool_controller_to_doc(self, index=None):
        """
        if no jobs, don't do anything, otherwise all TCs for all
        selected toolbit assets
        """
        Path.Log.track()
        jobs = PathUtilsGui.PathUtils.GetJobs()
        if len(jobs) == 0:
            QtGui.QMessageBox.information(
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
        selected_tools = self._add_tool_to_doc()

        for toolNr, toolbit in selected_tools:
            tc = PathToolControllerGui.Create(f"TC: {toolbit.label}", toolbit.obj, toolNr)
            job.Proxy.addToolController(tc)
            FreeCAD.ActiveDocument.recompute()

    def open(self, path=None):
        """load library stored in path and bring up ui"""
        docs = FreeCADGui.getMainWindow().findChildren(QtGui.QDockWidget)
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
            QtCore.Qt.RightDockWidgetArea,
            self.form,
            QtCore.Qt.Orientation.Vertical,
        )
