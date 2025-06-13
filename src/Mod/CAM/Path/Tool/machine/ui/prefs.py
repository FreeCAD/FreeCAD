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
from PySide import QtGui
from typing import List, cast
import FreeCAD
from ...camassets import cam_assets
from ..models import Machine, Lathe, Mill
from .lathe import LathePropertiesDialog
from .mill import MillPropertiesDialog


translate = FreeCAD.Qt.translate
MachineTypes = [m.get_type() for m in Machine.__subclasses__()]


class MachinePreferencesPage(QtGui.QWidget):
    """Preferences page for configuring multiple machine settings."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle(translate("CAM_PreferencesMachine", "Machine Settings"))
        self.layout = QtGui.QVBoxLayout(self)

        # Machine list
        self.machine_list = QtGui.QTableWidget()
        self.machine_list.setColumnCount(3)
        self.machine_list.setHorizontalHeaderLabels(
            [
                translate("CAM_PreferencesMachine", "Type"),
                translate("CAM_PreferencesMachine", "Label"),
                translate("CAM_PreferencesMachine", "ID"),
            ]
        )
        self.machine_list.horizontalHeader().setStretchLastSection(True)
        self.machine_list.setSelectionBehavior(QtGui.QAbstractItemView.SelectRows)
        self.machine_list.setSelectionMode(QtGui.QAbstractItemView.SingleSelection)
        self.machine_list.verticalHeader().setVisible(False)
        self.machine_list.setShowGrid(False)
        self.machine_list.setEditTriggers(QtGui.QAbstractItemView.NoEditTriggers)
        self.machine_list.cellDoubleClicked.connect(self.edit_machine)
        self.machine_list.itemSelectionChanged.connect(self._update_button_states)
        self.layout.addWidget(self.machine_list)

        # Buttons
        buttons_layout = QtGui.QHBoxLayout()
        self.add_btn = QtGui.QPushButton(translate("CAM_PreferencesMachine", "Add Machine"))
        self.edit_btn = QtGui.QPushButton(translate("CAM_PreferencesMachine", "Edit Machine"))
        self.remove_btn = QtGui.QPushButton(translate("CAM_PreferencesMachine", "Remove Machine"))
        self.add_btn.clicked.connect(self.add_machine)
        self.edit_btn.clicked.connect(self.edit_machine)
        self.remove_btn.clicked.connect(self.remove_machine)
        buttons_layout.addWidget(self.add_btn)
        buttons_layout.addWidget(self.edit_btn)
        buttons_layout.addWidget(self.remove_btn)
        self.layout.addLayout(buttons_layout)

        # Load all machines
        self.machines = cast(List[Machine], cam_assets.fetch("machine"))
        self.populate_machine_list()
        self._update_button_states()

        self.layout.addStretch()

    def _update_button_states(self):
        """Update the enabled state of the edit and remove buttons."""
        row_selected = self.machine_list.currentRow() >= 0
        self.edit_btn.setEnabled(row_selected)
        self.remove_btn.setEnabled(row_selected)

    def populate_machine_list(self):
        """Update the machine list display."""
        selected_row = self.machine_list.currentRow()

        self.machine_list.setRowCount(0)
        for row, machine in enumerate(self.machines):
            self.machine_list.insertRow(row)
            self.machine_list.setItem(row, 0, QtGui.QTableWidgetItem(machine.get_type()))
            self.machine_list.setItem(row, 1, QtGui.QTableWidgetItem(machine.label))
            self.machine_list.setItem(row, 2, QtGui.QTableWidgetItem(machine.get_id()))

        if selected_row <= self.machine_list.rowCount() - 1:
            self.machine_list.selectRow(selected_row)
        elif self.machine_list.rowCount():
            self.machine_list.selectRow(0)
        self._update_button_states()

    def add_machine(self):
        """Add a new machine by selecting its type and opening the appropriate dialog."""
        # Create a dialog that asks for the machine type.
        type_dialog = QtGui.QDialog(self)
        type_dialog.setWindowTitle(translate("CAM_PreferencesMachine", "Select Machine Type"))
        type_dialog.resize(350, type_dialog.sizeHint().height())
        layout = QtGui.QVBoxLayout(type_dialog)
        combo = QtGui.QComboBox()
        combo.addItems(MachineTypes)
        layout.addWidget(combo)
        buttons = QtGui.QDialogButtonBox(QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel)
        buttons.accepted.connect(type_dialog.accept)
        buttons.rejected.connect(type_dialog.reject)
        layout.addWidget(buttons)
        if type_dialog.exec_() != QtGui.QDialog.Accepted:
            return

        # Open the machine type-specific editor.
        machine_type = combo.currentText()
        if machine_type == "Lathe":
            machine = Lathe(translate("CAM", "My Lathe"))
            dialog = LathePropertiesDialog(machine, self)
        elif machine_type == "Mill":
            machine = Mill(translate("CAM", "My 3-Axis CNC"))
            dialog = MillPropertiesDialog(machine, self)
        else:
            raise AttributeError(f"invalid machine type {machine_type}. registered: {MachineTypes}")
        if not dialog.exec_():
            return

        # Save the new machine.
        cam_assets.add(dialog.machine)
        self.machines.append(dialog.machine)
        for spindle in dialog.machine.spindles:
            cam_assets.add(spindle)
        self.populate_machine_list()

    def edit_machine(self):
        """Edit the selected machine using its corresponding dialog."""
        current_row = self.machine_list.currentRow()
        if current_row < 0:
            return

        machine = self.machines[current_row]
        machine_type = machine.get_type()
        if machine_type == "Lathe":
            dialog = LathePropertiesDialog(machine, self)
        elif machine_type == "Mill":
            dialog = MillPropertiesDialog(machine, self)
        else:
            raise AttributeError(f"invalid machine type {machine_type}. registered: {MachineTypes}")
        if dialog.exec_():
            self.populate_machine_list()

    def remove_machine(self):
        """Remove the selected machine after confirmation."""
        current_row = self.machine_list.currentRow()
        if current_row < 0:
            return

        reply = QtGui.QMessageBox.question(
            self,
            translate("CAM_PreferencesMachine", "Confirm Removal"),
            translate("CAM_PreferencesMachine", "Are you sure you want to remove this machine?"),
            QtGui.QMessageBox.Yes | QtGui.QMessageBox.No,
            QtGui.QMessageBox.No,
        )
        if reply != QtGui.QMessageBox.Yes:
            return

        machine = self.machines.pop(current_row)
        cam_assets.delete(machine.get_uri())
        self.populate_machine_list()

    def saveSettings(self):
        """Save machine and spindle configurations to preferences."""
        for machine in self.machines:
            cam_assets.add(machine)
            for spindle in machine.spindles:
                cam_assets.add(spindle)
        return True
