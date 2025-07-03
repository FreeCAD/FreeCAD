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
import itertools
from PySide import QtGui, QtCore
from PySide.QtGui import QComboBox, QPushButton, QHBoxLayout
import FreeCAD
import Path
from ..models.axis import LinearAxis, AngularAxis
from ..models.machine import Machine
from ..models.spindle import Spindle
from .spindle_edit import SpindleEditorDialog
from .post import PostProcessorSettingsDialog
from .rigidity import RigidityWizard
from .spinbox import VelocitySpinBox


translate = FreeCAD.Qt.translate


class MachinePropertiesDialog(QtGui.QDialog):
    """Base dialog for adding or editing a machine with common properties."""

    def __init__(self, machine: Machine, has_rigidity: bool = True, parent=None):
        super().__init__(parent)
        self.machine = machine
        self.has_rigidity = has_rigidity
        self.setWindowTitle(translate("CAM", "Edit Machine"))
        self.layout = QtGui.QVBoxLayout(self)

        # General group
        general_group = QtGui.QGroupBox(translate("CAM", "General"))
        general_layout = QtGui.QFormLayout()

        self.machine_id_label = QtGui.QLabel(machine.get_id())
        self.machine_id_label.setTextInteractionFlags(QtCore.Qt.TextSelectableByMouse)
        general_layout.addRow(translate("CAM", "ID"), self.machine_id_label)

        self.label_edit = QtGui.QLineEdit(machine.label)
        self.label_edit.textChanged.connect(self._update_window_title)
        general_layout.addRow(translate("CAM", "Label"), self.label_edit)
        self._update_window_title()

        self.post_processor_combo = QComboBox()
        available_post_processors = Path.Preferences.allAvailablePostProcessors()
        self.post_processor_combo.addItems(available_post_processors)
        if machine.post_processor in available_post_processors:
            self.post_processor_combo.setCurrentIndex(
                available_post_processors.index(machine.post_processor)
            )
        self.post_processor_combo.currentIndexChanged.connect(self._update_machine_post_processor)

        post_processor_layout = QHBoxLayout()
        post_processor_layout.addWidget(self.post_processor_combo)

        self.configure_post_processor_btn = QPushButton(translate("CAM", "Configure…"))
        self.configure_post_processor_btn.clicked.connect(self.launch_post_processor_settings)
        post_processor_layout.addWidget(self.configure_post_processor_btn)
        general_layout.addRow(translate("CAM", "Post processor"), post_processor_layout)

        general_group.setLayout(general_layout)
        self.layout.addWidget(general_group)

        # Linear Axis Properties group
        linear_axis_group = QtGui.QGroupBox(translate("CAM", "Linear Axis Settings"))
        linear_axis_layout = QtGui.QGridLayout()
        self.axis_edits = {}

        # Add column headers for the grid
        col = itertools.count(1)
        linear_axis_layout.addWidget(QtGui.QLabel(translate("CAM", "Maximum feed")), 0, next(col))

        # Linear Axes
        for i, axis in enumerate(machine.find_children_by_type(LinearAxis)):
            row = i + 1  # Start from row 1 after headers

            col = itertools.count(0)
            linear_axis_layout.addWidget(
                QtGui.QLabel(f"{axis.name.capitalize()}-axis"), row, next(col)
            )

            max_feed_edit = VelocitySpinBox()
            max_feed_edit.setValue(axis.max_feed)
            linear_axis_layout.addWidget(max_feed_edit, row, next(col))

            self.axis_edits[axis.name] = max_feed_edit
        linear_axis_layout.setColumnStretch(0, 1)
        linear_axis_layout.setColumnStretch(1, 4)

        linear_axis_group.setLayout(linear_axis_layout)
        self.layout.addWidget(linear_axis_group)

        if self.has_rigidity:
            # Rigidity group
            rigidity_group = QtGui.QGroupBox(translate("CAM", "Rigidity"))
            rigidity_layout = QtGui.QGridLayout()

            col = itertools.count(1)
            rigidity_layout.addWidget(
                QtGui.QLabel(translate("CAM", "Angular Rigidity")), 0, next(col)
            )
            rigidity_layout.addWidget(QtGui.QLabel(translate("CAM", "Rigidity X")), 0, next(col))
            rigidity_layout.addWidget(QtGui.QLabel(translate("CAM", "Rigidity Y")), 0, next(col))

            # Angular axis
            for row, axis in enumerate(machine.find_children_by_type(AngularAxis)):
                col = itertools.count(0)
                rigidity_layout.addWidget(
                    QtGui.QLabel(f"{axis.name.capitalize()}-axis"), row, next(col)
                )

                rigidity_x_edit = QtGui.QDoubleSpinBox()
                rigidity_x_edit.setDecimals(6)
                rigidity_x_edit.setMinimum(0.0)
                rigidity_x_edit.setValue(axis.rigidity_x.Value)
                rigidity_x_edit.setSuffix(" mm/N")
                rigidity_layout.addWidget(rigidity_x_edit, row, next(col))

                rigidity_y_edit = QtGui.QDoubleSpinBox()
                rigidity_y_edit.setDecimals(6)
                rigidity_y_edit.setMinimum(0.0)
                rigidity_y_edit.setValue(axis.rigidity_y.Value)
                rigidity_y_edit.setSuffix(" mm/N")
                rigidity_layout.addWidget(rigidity_y_edit, row, next(col))

                angular_rigidity_edit = QtGui.QDoubleSpinBox()
                angular_rigidity_edit.setDecimals(6)
                angular_rigidity_edit.setMinimum(0.0)
                angular_rigidity_edit.setValue(axis.angular_rigidity.Value)
                angular_rigidity_edit.setSuffix("°/N")
                rigidity_layout.addWidget(angular_rigidity_edit, row, next(col))

                self.axis_edits[axis.name] = angular_rigidity_edit, rigidity_x_edit, rigidity_y_edit

            rigidity_layout.setColumnStretch(0, 1)
            rigidity_layout.setColumnStretch(1, 4)
            rigidity_layout.setColumnStretch(2, 4)
            rigidity_layout.setColumnStretch(3, 4)

            # Add Configure Rigidities button
            self.configure_rigidities_btn = QtGui.QPushButton(translate("CAM", "Rigidity Wizard…"))
            self.configure_rigidities_btn.clicked.connect(self.launch_rigidity_wizard)
            rigidity_layout.addWidget(self.configure_rigidities_btn, row + 1, 0, 1, 4)

            rigidity_group.setLayout(rigidity_layout)
            self.layout.addWidget(rigidity_group)

        # Spindles group
        self.spindles_group = QtGui.QGroupBox(translate("CAM", "Spindles"))
        spindles_layout = QtGui.QVBoxLayout()
        self.spindle_list = QtGui.QTableWidget()
        self.spindle_list.setColumnCount(2)
        self.spindle_list.verticalHeader().setVisible(False)
        self.spindle_list.horizontalHeader().setVisible(False)
        self.spindle_list.horizontalHeader().setStretchLastSection(True)
        self.spindle_list.setSelectionBehavior(QtGui.QAbstractItemView.SelectRows)
        self.spindle_list.setSelectionMode(QtGui.QAbstractItemView.SingleSelection)
        self.spindle_list.setShowGrid(False)
        self.spindle_list.setEditTriggers(QtGui.QAbstractItemView.NoEditTriggers)
        self.spindle_list.doubleClicked.connect(self.edit_spindle)
        self.spindle_list.itemSelectionChanged.connect(self._update_button_states)

        for spindle in machine.find_children_by_type(Spindle):
            self._add_spindle_to_list(spindle)
        spindles_layout.addWidget(self.spindle_list)
        spindle_buttons = QtGui.QHBoxLayout()
        self.add_spindle_btn = QtGui.QPushButton(translate("CAM", "Add Spindle"))
        self.edit_spindle_btn = QtGui.QPushButton(translate("CAM", "Edit Spindle"))
        self.remove_spindle_btn = QtGui.QPushButton(translate("CAM", "Remove Spindle"))
        self.add_spindle_btn.clicked.connect(self.add_spindle)
        self.edit_spindle_btn.clicked.connect(self.edit_spindle)
        self.remove_spindle_btn.clicked.connect(self.remove_spindle)
        spindle_buttons.addWidget(self.add_spindle_btn)
        spindle_buttons.addWidget(self.edit_spindle_btn)
        spindle_buttons.addWidget(self.remove_spindle_btn)
        spindles_layout.addLayout(spindle_buttons)
        self._update_button_states()

        self.spindles_group.setLayout(spindles_layout)
        self.layout.addWidget(self.spindles_group)

        # Dialog buttons
        buttons = QtGui.QDialogButtonBox(
            QtGui.QDialogButtonBox.Ok | QtGui.QDialogButtonBox.Cancel, QtCore.Qt.Horizontal
        )
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        self.layout.addWidget(buttons)

        self.resize(750, self.sizeHint().height())

    def launch_post_processor_settings(self):
        dialog = PostProcessorSettingsDialog(self.machine, self)
        if dialog.exec_():
            # Update the post processor combo box
            current_post_processor = self.machine.post_processor
            index = self.post_processor_combo.findText(current_post_processor)
            if index != -1:
                self.post_processor_combo.setCurrentIndex(index)

    def _update_machine_post_processor(self, index):
        self.machine.post_processor = self.post_processor_combo.itemText(index)

    def _add_spindle_to_list(self, spindle: Spindle):
        row_position = self.spindle_list.rowCount()
        self.spindle_list.insertRow(row_position)

        label_item = QtGui.QTableWidgetItem(spindle.label)
        label_item.setData(QtCore.Qt.UserRole, spindle)
        self.spindle_list.setItem(row_position, 0, label_item)

        kw_item = QtGui.QTableWidgetItem(spindle.max_power.UserString)
        self.spindle_list.setItem(row_position, 1, kw_item)

    def add_spindle(self):
        label = translate("CAM", f"Spindle {self.spindle_list.rowCount() + 1}")
        dialog = SpindleEditorDialog(Spindle(label), parent=self)
        if dialog.exec_():
            new_spindle = dialog.spindle
            self.machine.add(new_spindle)
            self._add_spindle_to_list(new_spindle)
            self.spindle_list.setCurrentCell(self.spindle_list.rowCount() - 1, 0)
            self._update_button_states()

    def edit_spindle(self):
        current_row = self.spindle_list.currentRow()
        if current_row >= 0 and self.spindle_list.rowCount() > 0:
            spindle = self.spindle_list.item(current_row, 0).data(QtCore.Qt.UserRole)
            dialog = SpindleEditorDialog(spindle, parent=self)
            if dialog.exec_():
                updated_spindle = dialog.spindle
                power = updated_spindle.max_power.UserString
                # Find the old spindle and replace it with the updated one
                old_spindle = self.spindle_list.item(current_row, 0).data(QtCore.Qt.UserRole)
                self.machine.remove(old_spindle)
                self.machine.add(updated_spindle)
                self.spindle_list.item(current_row, 0).setText(updated_spindle.label)
                self.spindle_list.item(current_row, 0).setData(QtCore.Qt.UserRole, updated_spindle)
                self.spindle_list.item(current_row, 1).setText(power)

    def remove_spindle(self):
        current_row = self.spindle_list.currentRow()
        if current_row >= 0 and self.spindle_list.rowCount() > 0:
            spindle_to_remove = self.spindle_list.item(current_row, 0).data(QtCore.Qt.UserRole)
            self.machine.remove(spindle_to_remove)
            self.spindle_list.removeRow(current_row)
            self._update_button_states()

    def launch_rigidity_wizard(self):
        if self.has_rigidity:
            wizard = RigidityWizard(self.machine, self)
            if not wizard.exec_():
                return

            rigidities = wizard.get_rigidities()
            for axis in self.machine.find_children_by_type(AngularAxis):
                edits = self.axis_edits.get(axis.name)
                if edits:
                    angular_rigidity_edit, rigidity_x_edit, rigidity_y_edit = edits
                    angular_rigidity, rigidity_x, rigidity_y = rigidities.get(
                        axis.name, (None, None, None)
                    )
                    if angular_rigidity:
                        angular_rigidity_edit.setValue(angular_rigidity.Value)
                    if rigidity_x:
                        rigidity_x_edit.setValue(rigidity_x.Value)
                    if rigidity_y:
                        rigidity_y_edit.setValue(rigidity_y.Value)

    def accept(self):
        # Check the label
        label = self.label_edit.text()
        if not label:
            QtGui.QMessageBox.warning(
                self,
                translate("CAM", "Warning"),
                translate("CAM", "Machine label cannot be empty."),
            )
            return
        self.machine.label = label

        self.machine.post_processor = self.post_processor_combo.currentText()

        # Update axis properties
        for axis in self.machine.find_children_by_type(LinearAxis):
            max_feed_edit = self.axis_edits.get(axis.name)
            if max_feed_edit:
                max_feed_val = max_feed_edit.property("value")

                if max_feed_val.Value <= 0:
                    QtGui.QMessageBox.warning(
                        self,
                        translate("CAM", "Warning"),
                        translate("CAM", f"{axis.name}-axis max feed rate must be positive."),
                    )
                    return
                axis.max_feed = max_feed_val

        for axis in self.machine.find_children_by_type(AngularAxis):
            edits = self.axis_edits.get(axis.name)
            if edits:
                angular_rigidity_edit, rigidity_x_edit, rigidity_y_edit = edits
                if self.has_rigidity:
                    angular_rigidity_val = angular_rigidity_edit.value()
                    rigidity_x_val = rigidity_x_edit.value()
                    rigidity_y_val = rigidity_y_edit.value()

                    angular_rigidity_quantity = FreeCAD.Units.Quantity(
                        f"{angular_rigidity_val} °/N"
                    )
                    rigidity_x_quantity = FreeCAD.Units.Quantity(f"{rigidity_x_val} mm/N")
                    rigidity_y_quantity = FreeCAD.Units.Quantity(f"{rigidity_y_val} mm/N")

                    if angular_rigidity_val < 0:
                        QtGui.QMessageBox.warning(
                            self,
                            translate("CAM", "Warning"),
                            translate(
                                "CAM", f"{axis.name}-axis angular rigidity cannot be negative."
                            ),
                        )
                        return
                    if rigidity_x_val < 0:
                        QtGui.QMessageBox.warning(
                            self,
                            translate("CAM", "Warning"),
                            translate("CAM", f"{axis.name}-axis rigidity X cannot be negative."),
                        )
                        return
                    if rigidity_y_val < 0:
                        QtGui.QMessageBox.warning(
                            self,
                            translate("CAM", "Warning"),
                            translate("CAM", f"{axis.name}-axis rigidity Y cannot be negative."),
                        )
                        return
                    axis.angular_rigidity = angular_rigidity_quantity
                    axis.rigidity_x = rigidity_x_quantity
                    axis.rigidity_y = rigidity_y_quantity

        # Let child classes perform updates.
        self.update_machine()

        # Validate the updates.
        try:
            self.machine.validate()
        except AttributeError as e:
            QtGui.QMessageBox.warning(self, translate("CAM", "Warning"), str(e))
            return

        super().accept()

    def _update_window_title(self):
        title = translate("CAM", "Edit Machine")
        if self.label_edit.text():
            title = f"{self.label_edit.text()} - {title}"
        self.setWindowTitle(title)

    def update_machine(self):
        """Subclasses must implement this to update the specific machine."""
        raise NotImplementedError("Subclasses must implement update_machine()")

    def _update_button_states(self):
        spindle_selected = self.spindle_list.currentRow() >= 0
        self.edit_spindle_btn.setEnabled(spindle_selected)
        self.remove_spindle_btn.setEnabled(spindle_selected)
