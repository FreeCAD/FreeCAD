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

import pathlib
import tempfile
import FreeCAD
import Path
from PySide import QtGui, QtCore

translate = FreeCAD.Qt.translate


def _is_writable_dir(path: Path) -> bool:
    """
    Check if a path is a writable directory.
    Returns True if writable, False otherwise.
    """
    if not path.is_dir():
         return False
    try:
        with tempfile.NamedTemporaryFile(dir=str(path), delete=True):
            return True
    except (OSError, PermissionError):
        return False


class ToolsPreferencesPage:
    def __init__(self, parent=None):
        self.form = QtGui.QToolBox()
        self.form.setWindowTitle(translate("CAM_PreferencesTools", "Tools"))

        # Tool Library Path UI
        tool_path_widget = QtGui.QWidget()
        main_layout = QtGui.QHBoxLayout(tool_path_widget)

        #main_layout.addWidget(self.tool_path_label, 0, QtCore.Qt.AlignTop)

        # Create widgets
        self.tool_path_label = QtGui.QLabel(translate("CAM_PreferencesTools", "Tool Directory:"))
        self.tool_path_edit = QtGui.QLineEdit()
        self.tool_path_note_label = QtGui.QLabel(
            translate("CAM_PreferencesTools",
                      "Note: Select the directory that will contain the "
                      "Bit/, Shape/, and Library/ subfolders for your "
                      "tool library."))
        self.tool_path_note_label.setWordWrap(True)
        self.select_path_button = QtGui.QToolButton()
        self.select_path_button.setIcon(QtGui.QIcon.fromTheme("folder-open"))
        self.select_path_button.clicked.connect(self.selectToolPath)
        self.reset_path_button = QtGui.QPushButton(translate("CAM_PreferencesTools", "Reset"))
        self.reset_path_button.clicked.connect(self.resetToolPath)

        # Set note label font to italic
        font = self.tool_path_note_label.font()
        font.setItalic(True)
        self.tool_path_note_label.setFont(font)

        # Layout for tool path section
        edit_button_layout = QtGui.QGridLayout()
        edit_button_layout.addWidget(self.tool_path_label, 0, 0, QtCore.Qt.AlignVCenter)
        edit_button_layout.addWidget(self.tool_path_edit, 0, 1, QtCore.Qt.AlignVCenter)
        edit_button_layout.addWidget(self.select_path_button, 0, 2, QtCore.Qt.AlignVCenter)
        edit_button_layout.addWidget(self.reset_path_button, 0, 3, QtCore.Qt.AlignVCenter)
        edit_button_layout.addWidget(self.tool_path_note_label, 1, 1, 1, 1, QtCore.Qt.AlignTop)
        edit_button_layout.addItem(QtGui.QSpacerItem(0, 0, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Expanding), 2, 0, 1, 4)

        main_layout.addLayout(edit_button_layout, QtCore.Qt.AlignTop)

        self.form.addItem(tool_path_widget, translate("CAM_PreferencesTools", "Tool Library"))

    def selectToolPath(self):
        # Implement directory selection dialog
        path = QtGui.QFileDialog.getExistingDirectory(
            self.form,
            translate("CAM_PreferencesTools", "Select Tool Library Directory"),
            self.tool_path_edit.text()
        )
        if path:
            self.tool_path_edit.setText(str(path))

    def resetToolPath(self):
        # Implement resetting path to default
        default_path = Path.Preferences.getDefaultToolPath()
        self.tool_path_edit.setText(str(default_path))

    def saveSettings(self):
        # Check path is writable, then call Path.Preferences.setToolPath()
        tool_path = pathlib.Path(self.tool_path_edit.text())
        if FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Path").GetBool("CheckToolPathWritable", True):
            if not _is_writable_dir(tool_path):
                QtGui.QMessageBox.warning(
                    self.form,
                    translate("CAM_PreferencesTools", "Warning"),
                    translate("CAM_PreferencesTools", "The selected tool library path is not writable.")
                )
                return False
        Path.Preferences.setToolPath(tool_path)
        return True

    def loadSettings(self):
        # use getToolPath() to initialize UI
        tool_path = Path.Preferences.getToolPath()
        self.tool_path_edit.setText(str(tool_path))
