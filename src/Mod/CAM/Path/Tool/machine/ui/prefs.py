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
from .machine import MachinePropertiesDialog
from .selector import MachineSelector
from ...toolbit.ui.tablecell import CompactTwoLineTableCell


translate = FreeCAD.Qt.translate


class MachinePreferencesPage(QtGui.QWidget):
    """Preferences page for configuring multiple machine settings."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle(translate("CAM_PreferencesMachine", "Machine Settings"))
        self.layout = QtGui.QVBoxLayout(self)

        # Machine list
        self.machine_list = QtGui.QListWidget()
        self.machine_list.setSelectionMode(QtGui.QAbstractItemView.SingleSelection)
        self.machine_list.itemDoubleClicked.connect(self.edit_machine)
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
        item_selected = self.machine_list.currentItem() is not None
        self.edit_btn.setEnabled(item_selected)
        self.remove_btn.setEnabled(item_selected)

    def populate_machine_list(self):
        """Update the machine list display."""
        selected_item = self.machine_list.currentItem()

        self.machine_list.clear()
        for machine in self.machines:
            item = QtGui.QListWidgetItem(self.machine_list)
            cell = CompactTwoLineTableCell()
            cell.set_upper_text(machine.label)
            cell.set_lower_text(machine.summary)
            item.setSizeHint(cell.sizeHint())
            self.machine_list.addItem(item)
            self.machine_list.setItemWidget(item, cell)

        if selected_item and selected_item in [
            self.machine_list.item(i) for i in range(self.machine_list.count())
        ]:
            self.machine_list.setCurrentItem(selected_item)
        elif self.machine_list.count():
            self.machine_list.setCurrentRow(0)
        self._update_button_states()

    def add_machine(self):
        """Add a new machine by selecting its type and opening the appropriate dialog."""
        selector = MachineSelector(parent=self)
        machine = selector.show()
        if not machine:
            return

        # Open the machine editor.
        dialog = MachinePropertiesDialog(machine, self)
        if not dialog.exec_():
            return

        # Save the new machine.
        cam_assets.add(dialog.machine)
        self.machines.append(dialog.machine)
        self.populate_machine_list()

    def edit_machine(self):
        """Edit the selected machine using its corresponding dialog."""
        current_item = self.machine_list.currentItem()
        if not current_item:
            return

        current_row = self.machine_list.row(current_item)
        machine = self.machines[current_row]
        dialog = MachinePropertiesDialog(machine, self)
        if dialog.exec_():
            self.populate_machine_list()

    def remove_machine(self):
        """Remove the selected machine after confirmation."""
        current_item = self.machine_list.currentItem()
        if not current_item:
            return

        current_row = self.machine_list.row(current_item)

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
        return True
