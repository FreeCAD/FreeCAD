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

"""ToolBit Library Dock Widget."""

import FreeCAD
import FreeCADGui
import Path
import Path.Tool.Gui.Controller as PathToolControllerGui
import PathScripts.PathUtilsGui as PathUtilsGui
from PySide import QtGui, QtWidgets
from ...camassets import cam_assets, ensure_assets_initialized
from ...toolbit import ToolBit
from .editor import LibraryEditor
from .browser import LibraryBrowserWithCombo

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


translate = FreeCAD.Qt.translate


class ToolBitLibraryDock(object):
    """Controller for displaying a library and creating ToolControllers"""

    def __init__(
        self, defaultJob=None, autoClose=False, askToolNumber=False, singleSelection=False
    ):
        ensure_assets_initialized(cam_assets)
        # Create the main form widget directly
        self.defaultJob = defaultJob
        self.autoClose = autoClose
        self.askToolNumber = askToolNumber
        self.form = QtWidgets.QDialog()
        self.form.setObjectName("ToolSelector")
        self.form.setWindowTitle(translate("CAM_ToolBit", "Toolbit Selector"))
        self.form.setMinimumSize(600, 400)
        self.form.resize(800, 600)
        self.form.adjustSize()
        self.form_layout = QtGui.QVBoxLayout(self.form)
        self.form_layout.setContentsMargins(4, 4, 4, 4)
        self.form_layout.setSpacing(4)

        # Create the browser widget
        self.browser_widget = LibraryBrowserWithCombo(asset_manager=cam_assets, show_all_tools=True)
        if singleSelection:
            self.browser_widget.setSingleSelection()

        self._setup_ui()

    def _setup_ui(self):
        """Setup the form and load the tooltable data"""
        Path.Log.track()

        # Create a main widget and layout for the dock
        main_widget = QtGui.QWidget()
        main_layout = QtGui.QVBoxLayout(main_widget)
        main_layout.setContentsMargins(4, 4, 4, 4)
        main_layout.setSpacing(4)

        main_layout.addWidget(self.browser_widget)

        # Create buttons
        self.libraryEditorOpenButton = QtGui.QPushButton(
            translate("CAM_ToolBit", "Open Library Editor")
        )
        self.addToolControllerButton = QtGui.QPushButton(translate("CAM_ToolBit", "Add to Job"))
        self.closeButton = QtGui.QPushButton(translate("CAM_ToolBit", "Close"))

        button_width = 120
        self.libraryEditorOpenButton.setMinimumWidth(button_width)
        self.addToolControllerButton.setMinimumWidth(button_width)
        self.closeButton.setMinimumWidth(button_width)

        # Add buttons to a horizontal layout, right-align Close
        button_layout = QtGui.QHBoxLayout()
        button_layout.addWidget(self.libraryEditorOpenButton)
        button_layout.addWidget(self.addToolControllerButton)
        button_layout.addStretch(1)
        button_layout.addWidget(self.closeButton)

        # Add the button layout to the main layout
        main_layout.addLayout(button_layout)

        # Set the main widget as the dock's widget
        self.form.layout().addWidget(main_widget)

        # Connect signals from the browser widget and buttons
        self.browser_widget.toolSelected.connect(lambda x: self._update_state())
        self.browser_widget.itemDoubleClicked.connect(self._on_doubleclick)
        self.libraryEditorOpenButton.clicked.connect(self._open_editor)
        self.addToolControllerButton.clicked.connect(self._add_tool_controller_to_doc)
        self.closeButton.clicked.connect(self.form.reject)

        # Update the initial state of the UI
        self._update_state()

    def _count_jobs(self):
        if not FreeCAD.ActiveDocument:
            return 0
        return len([1 for j in FreeCAD.ActiveDocument.Objects if j.Name[:3] == "Job"]) >= 1

    def _update_state(self):
        """Enable button to add tool controller when a tool is selected"""
        selected = bool(self.browser_widget.get_selected_bit_uris())
        has_job = selected and self._count_jobs() > 0
        self.addToolControllerButton.setEnabled(selected and has_job)

    def _on_doubleclick(self, toolbit: ToolBit):
        """Opens the ToolBitEditor for the selected toolbit."""
        self._add_tool_controller_to_doc()

    def _open_editor(self):
        library = LibraryEditor(parent=FreeCADGui.getMainWindow())
        library.open()
        # After editing, we might need to refresh the libraries in the browser widget
        # Assuming _populate_libraries is the correct method to call
        self.browser_widget.refresh()

    def _conflicting_tool_controller(self, job, toolNr, toolbit: ToolBit):
        """Returns a controller of the job using a tool number for another tool."""
        return PathToolControllerGui.findConflictingToolController(
            job.Tools.Group, toolNr, toolbit.get_id()
        )

    def _needs_prompt(self, job, toolbits) -> bool:
        """
        True if adding these toolbits will ask the user for a tool number.

        The job is examined as it stands, before anything is added, so a
        conflict that only a preceding toolbit of the same batch would cause is
        not seen. That is sound as long as every caller closing the dialog up
        front (autoClose) adds one toolbit at a time.
        """
        for toolbit in toolbits:
            toolNr = self.browser_widget.get_tool_no_from_current_library(toolbit)
            if toolNr is None:
                if self.askToolNumber:
                    return True
            elif self._conflicting_tool_controller(job, toolNr, toolbit) is not None:
                return True
        return False

    def _ask_tool_number(self, toolbit: ToolBit, job, taken=None) -> int:
        """
        Asks the user for a tool number, defaulting to the next one free in the
        job. `taken` is the controller holding the number the toolbit wanted,
        when that is the reason for asking. Keeps asking as long as the number
        belongs to another tool. Returns None if the user cancelled.
        """
        toolNr = job.Proxy.nextToolNumber()
        while True:
            if taken is None:
                label = translate("CAM_ToolBit", "Tool number for {}:").format(toolbit.label)
            else:
                label = translate(
                    "CAM_ToolBit", "Tool number {} is already used by {}.\nTool number for {}:"
                ).format(taken.ToolNumber, taken.Label, toolbit.label)

            toolNr, ok = QtGui.QInputDialog.getInt(
                self.form,
                translate("CAM_ToolBit", "Tool Number"),
                label,
                toolNr,
                1,
            )
            if not ok:
                return None

            taken = self._conflicting_tool_controller(job, toolNr, toolbit)
            if taken is None:
                return toolNr

    def _add_tool_controller(self, job, toolbit: ToolBit, toolNr: int):
        """
        Attaches a toolbit to the document and gives the job a tool controller
        for it. Attaching and adding happen together, so the toolbit is never
        left sitting unparented in the tree.
        """
        toolbit.attach_to_doc(FreeCAD.ActiveDocument)
        # Toolbits are visible when attached; hide them right away, or they
        # show up in the 3D view until their tool controller is created.
        if toolbit.obj.ViewObject:
            toolbit.obj.ViewObject.Visibility = False

        tc = PathToolControllerGui.Create(f"TC: {toolbit.label}", toolbit.obj, toolNr)
        job.Proxy.addToolController(tc)

    def _add_tool_controller_to_doc(self):
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
                translate("CAM_ToolBit", "Create a Job first."),
            )
            return
        elif self.defaultJob or len(jobs) == 1:
            job = self.defaultJob or jobs[0]
        else:
            userinput = PathUtilsGui.PathUtilsUserInput()
            job = userinput.chooseJob(jobs)
            self.defaultJob = job

        if job is None:  # user may have canceled
            return

        toolbits = self.browser_widget.get_selected_bits()
        if not toolbits:
            return

        added = False

        # Attaching a toolbit loads its shape, which takes long enough to be
        # noticeable, so get the dialog off the screen before that rather than
        # leaving it up while the tool loads. If a tool number is going to be
        # asked for, it has to stay: the prompts are parented to it.
        closeFirst = self.autoClose and not self._needs_prompt(job, toolbits)
        if closeFirst:
            self.form.accept()
            FreeCADGui.updateGui()

        for toolbit in toolbits:
            toolNr = self.browser_widget.get_tool_no_from_current_library(toolbit)

            # A library's tool number is only a suggestion. Another tool in the
            # job may already hold it, typically one from a different library,
            # so let the user pick a free number instead of silently colliding.
            taken = None
            if toolNr is not None:
                taken = self._conflicting_tool_controller(job, toolNr, toolbit)
                if taken is not None:
                    toolNr = None

            if toolNr is None and (self.askToolNumber or taken is not None):
                toolNr = self._ask_tool_number(toolbit, job, taken)
                if toolNr is None:  # user cancelled
                    continue
            if toolNr is None:
                toolNr = job.Proxy.nextToolNumber()

            self._add_tool_controller(job, toolbit, toolNr)
            added = True

            if self.askToolNumber:
                FreeCAD.ActiveDocument.recompute()
                FreeCADGui.updateGui()

        if added and not self.askToolNumber:
            FreeCAD.ActiveDocument.recompute()

        # Nothing was added only if every tool number prompt was cancelled, so
        # keep the dialog up for another pick rather than closing on a cancel.
        if added and self.autoClose and not closeFirst:
            self.form.accept()

    def open(self, path=None):
        """load library stored in path and bring up ui"""
        self.form.exec_()
