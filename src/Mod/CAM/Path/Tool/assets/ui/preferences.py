# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

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
from ....Machine.ui.editor import MachineEditorDialog
from ....Machine.models import MachineFactory

translate = FreeCAD.Qt.translate


def _is_writable_dir(path: pathlib.Path) -> bool:
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


class AssetPreferencesPage:
    def __init__(self, parent=None):
        self.form = QtGui.QWidget()
        self.form.setWindowTitle(translate("CAM_PreferencesAssets", "Assets"))

        # Set up main layout directly on the form
        self.main_layout = QtGui.QVBoxLayout(self.form)

        # Create widgets
        self.assets_group = QtGui.QGroupBox(translate("CAM_PreferencesAssets", "Asset Location"))
        self.asset_path_label = QtGui.QLabel(translate("CAM_PreferencesAssets", "Default path"))
        self.asset_path_edit = QtGui.QLineEdit()
        self.asset_path_note_label = QtGui.QLabel(
            translate(
                "CAM_PreferencesAssets",
                "Note: Select the directory that will contain the "
                "Tools folder with Bit/, Shape/, and Library/ subfolders and the Machines/ folder.",
            )
        )
        self.asset_path_note_label.setWordWrap(True)
        self.select_path_button = QtGui.QToolButton()
        self.select_path_button.setIcon(QtGui.QIcon.fromTheme("folder-open"))
        self.select_path_button.clicked.connect(self.selectAssetPath)
        self.reset_path_button = QtGui.QPushButton(translate("CAM_PreferencesAssets", "Reset"))
        self.reset_path_button.clicked.connect(self.resetAssetPath)

        # Set note label font to italic
        font = self.asset_path_note_label.font()
        font.setItalic(True)
        self.asset_path_note_label.setFont(font)

        # Assets group box
        self.assets_layout = QtGui.QGridLayout(self.assets_group)
        self.assets_layout.addWidget(self.asset_path_label, 0, 0, QtCore.Qt.AlignVCenter)
        self.assets_layout.addWidget(self.asset_path_edit, 0, 1, QtCore.Qt.AlignVCenter)
        self.assets_layout.addWidget(self.select_path_button, 0, 2, QtCore.Qt.AlignVCenter)
        self.assets_layout.addWidget(self.reset_path_button, 0, 3, QtCore.Qt.AlignVCenter)
        self.assets_layout.addWidget(self.asset_path_note_label, 1, 1, 1, 1, QtCore.Qt.AlignTop)
        self.main_layout.addWidget(self.assets_group)

        # Machines group box
        self.machines_group = QtGui.QGroupBox(translate("CAM_PreferencesAssets", "Machines"))
        self.machines_layout = QtGui.QVBoxLayout(self.machines_group)

        self.warning_label = QtGui.QLabel(
            translate(
                "CAM_PreferencesAssets",
                "Warning: Machine definition is an experimental feature. Changes "
                "made here will not affect any CAM functionality",
            )
        )
        self.warning_label.setWordWrap(True)
        warning_font = self.warning_label.font()
        warning_font.setItalic(True)
        self.warning_label.setFont(warning_font)
        self.warning_label.setContentsMargins(0, 0, 0, 10)
        self.machines_layout.addWidget(self.warning_label)

        self.machines_label = QtGui.QLabel(translate("CAM_PreferencesAssets", "Machines"))
        self.machines_layout.addWidget(self.machines_label)

        self.machines_list = QtGui.QListWidget()
        self.machines_layout.addWidget(self.machines_list)

        # Buttons: Add / Edit / Delete
        self.btn_layout = QtGui.QHBoxLayout()
        self.add_machine_btn = QtGui.QPushButton(translate("CAM_PreferencesAssets", "Add"))
        self.edit_machine_btn = QtGui.QPushButton(translate("CAM_PreferencesAssets", "Edit"))
        self.delete_machine_btn = QtGui.QPushButton(translate("CAM_PreferencesAssets", "Delete"))
        self.btn_layout.addWidget(self.add_machine_btn)
        self.btn_layout.addWidget(self.edit_machine_btn)
        self.btn_layout.addWidget(self.delete_machine_btn)
        self.machines_layout.addLayout(self.btn_layout)

        self.machines_layout.addStretch()  # Prevent the list from stretching

        self.main_layout.addWidget(self.machines_group)

        # Wire up buttons
        self.add_machine_btn.clicked.connect(self.add_machine)
        self.edit_machine_btn.clicked.connect(self.edit_machine)
        self.delete_machine_btn.clicked.connect(self.delete_machine)

        # Connect double-click to edit
        self.machines_list.itemDoubleClicked.connect(self.edit_machine)

        for name, filename in MachineFactory.list_configuration_files():
            if name == "<any>" or filename is None:
                continue
            item = QtGui.QListWidgetItem(name)
            item.setData(QtCore.Qt.UserRole, filename)
            self.machines_list.addItem(item)

    def selectAssetPath(self):
        # Implement directory selection dialog
        path = QtGui.QFileDialog.getExistingDirectory(
            self.form,
            translate("CAM_PreferencesAssets", "Select Asset Directory"),
            self.asset_path_edit.text(),
        )
        if path:
            self.asset_path_edit.setText(str(path))

    def resetAssetPath(self):
        # Implement resetting path to default
        default_path = Path.Preferences.getDefaultAssetPath()
        self.asset_path_edit.setText(str(default_path))

    def saveSettings(self):
        # Check path is writable, then call Path.Preferences.setAssetPath()
        asset_path = pathlib.Path(self.asset_path_edit.text())
        param = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/CAM")
        if param.GetBool("CheckAssetPathWritable", True):
            if not _is_writable_dir(asset_path):
                QtGui.QMessageBox.warning(
                    self.form,
                    translate("CAM_PreferencesAssets", "Warning"),
                    translate("CAM_PreferencesAssets", "The selected asset path is not writable."),
                )
                return False
        Path.Preferences.setAssetPath(asset_path)
        Path.Preferences.setLastToolLibrary("")
        return True

    def loadSettings(self):
        # Get the raw preference value, not the versioned path
        pref = Path.Preferences.tool_preferences()
        asset_path = pref.GetString(Path.Preferences.ToolPath, "")
        if not asset_path:
            asset_path = str(Path.Preferences.getDefaultAssetPath())
        self.asset_path_edit.setText(asset_path)

    def add_machine(self):
        # Create a new machine JSON file in the user's machine asset folder
        try:
            # Open editor for new machine, filename will be generated on save
            editor = MachineEditorDialog()
            if editor.exec_() == QtGui.QDialog.Accepted:
                # add to list
                filename = editor.filename
                display_name = MachineFactory.get_machine_display_name(filename)
                item = QtGui.QListWidgetItem(display_name)
                item.setData(QtCore.Qt.UserRole, filename)  # Store filename only
                self.machines_list.addItem(item)
        except Exception as e:
            Path.Log.error(f"Failed to create machine file: {e}")

    def edit_machine(self):
        try:
            item = self.machines_list.currentItem()
            if not item:
                return
            filename = item.data(QtCore.Qt.UserRole)
            if not filename:
                return
            dlg = MachineEditorDialog(filename)
            if dlg.exec_() == QtGui.QDialog.Accepted:
                # Reload display name from file after save
                display = MachineFactory.get_machine_display_name(filename)
                if display:
                    item.setText(display)
        except Exception as e:
            Path.Log.error(f"Failed to open machine editor: {e}")

    def delete_machine(self):
        try:
            item = self.machines_list.currentItem()
            if not item:
                return
            filename = item.data(QtCore.Qt.UserRole)
            if not filename:
                return
            # Confirm delete
            resp = QtGui.QMessageBox.question(
                self.form,
                translate("CAM_PreferencesAssets", "Delete Machine"),
                translate(
                    "CAM_PreferencesAssets", "Are you sure you want to delete this machine file?"
                ),
                QtGui.QMessageBox.Yes | QtGui.QMessageBox.No,
            )
            if resp != QtGui.QMessageBox.Yes:
                return
            if MachineFactory.delete_configuration(filename):
                self.machines_list.takeItem(self.machines_list.currentRow())
            else:
                Path.Log.error("Failed to delete machine file.")
        except Exception as e:
            Path.Log.error(f"Failed to delete machine: {e}")
