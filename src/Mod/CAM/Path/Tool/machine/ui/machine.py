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
import FreeCADGui
import Path
from ...spindle import Spindle
from ...spindle.ui.editor import SpindleEditorDialog
from ..models.machine import Machine, LinearAxis, AngularAxis
from .post import PostProcessorSettingsDialog
from .rigidity import RigidityWizard


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

        ui = FreeCADGui.UiLoader()

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

        # Axis Properties group
        axis_properties_group = QtGui.QGroupBox(translate("CAM", "Axis Properties"))
        axis_properties_layout = QtGui.QGridLayout()
        self.axis_edits = {}

        # Add column headers for the grid
        col = itertools.count(1)
        axis_properties_layout.addWidget(QtGui.QLabel(translate("CAM", "Minimum")), 0, next(col))
        axis_properties_layout.addWidget(QtGui.QLabel(translate("CAM", "Maximum")), 0, next(col))
        if self.has_rigidity:
            axis_properties_layout.addWidget(
                QtGui.QLabel(translate("CAM", "Rigidity")), 0, next(col)
            )
        axis_properties_layout.addWidget(
            QtGui.QLabel(translate("CAM", "Maximum feed")), 0, next(col)
        )

        # Linear Axes
        for i, (axis_name, axis) in enumerate(sorted(machine.axes.items())):
            if not isinstance(axis, LinearAxis):
                continue
            row = i + 1  # Start from row 1 after headers

            col = itertools.count(0)
            axis_properties_layout.addWidget(
                QtGui.QLabel(f"{axis_name.capitalize()}-axis"), row, next(col)
            )

            start_edit = ui.createWidget("Gui::QuantitySpinBox")
            start_edit.setProperty("value", axis.start)
            axis_properties_layout.addWidget(start_edit, row, next(col))

            end_edit = ui.createWidget("Gui::QuantitySpinBox")
            end_edit.setProperty("unit", "mm")
            end_edit.setProperty("value", axis.end or FreeCAD.Units.Quantity("0 mm"))
            axis_properties_layout.addWidget(end_edit, row, next(col))

            rigidity_edit = None
            if self.has_rigidity:
                # Use FreeCAD.Units.Quantity does not support distance/N, so
                # we use QDoubleSpinBox for rigidity to avoid unit issues
                rigidity_edit = QtGui.QDoubleSpinBox()
                rigidity_edit.setDecimals(6)
                rigidity_edit.setMinimum(0.0)
                rigidity_edit.setValue(axis.rigidity.Value)
                rigidity_edit.setSuffix(" mm/N")
                axis_properties_layout.addWidget(rigidity_edit, row, next(col))

            max_feed_edit = ui.createWidget("Gui::QuantitySpinBox")
            max_feed_edit.setProperty("value", axis.max_feed)
            axis_properties_layout.addWidget(max_feed_edit, row, next(col))

            self.axis_edits[axis_name] = (
                start_edit,
                end_edit,
                rigidity_edit,
                max_feed_edit,
            )

        # Angular axis
        # Determine the starting row for angular axes based on the number of linear axes
        angular_axis_start_row = (
            len([a for a in machine.axes.values() if isinstance(a, LinearAxis)]) + 1
        )
        row = angular_axis_start_row
        if self.has_rigidity:
            for i, (axis_name, axis) in enumerate(sorted(machine.axes.items())):
                if not isinstance(axis, AngularAxis):
                    continue
                row = angular_axis_start_row + i

                col = itertools.count(0)
                axis_properties_layout.addWidget(
                    QtGui.QLabel(f"{axis_name.capitalize()}-axis"), row, next(col)
                )

                # Add empty widgets for "Minimum" and "Maximum" columns for angular axes
                axis_properties_layout.addWidget(QtGui.QWidget(), row, next(col))
                axis_properties_layout.addWidget(QtGui.QWidget(), row, next(col))

                rigidity_edit = QtGui.QDoubleSpinBox()
                rigidity_edit.setDecimals(6)
                rigidity_edit.setMinimum(0.0)
                rigidity_edit.setValue(axis.rigidity.Value)
                rigidity_edit.setSuffix("°/N")
                axis_properties_layout.addWidget(rigidity_edit, row, next(col))

                # Add an empty widget for the last column
                axis_properties_layout.addWidget(QtGui.QWidget(), row, next(col))

                self.axis_edits[axis_name] = rigidity_edit

        axis_properties_group.setLayout(axis_properties_layout)

        # Add Configure Rigidities button
        if self.has_rigidity:
            self.configure_rigidities_btn = QtGui.QPushButton(translate("CAM", "Rigidity Wizard…"))
            self.configure_rigidities_btn.clicked.connect(self.launch_rigidity_wizard)
            axis_properties_layout.addWidget(self.configure_rigidities_btn, row + 1, 0, 1, 5)

        self.layout.addWidget(axis_properties_group)

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

        for spindle in machine.spindles:
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
            self.machine.add_spindle(new_spindle)
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
                self.machine.spindles[current_row] = updated_spindle
                self.spindle_list.item(current_row, 0).setText(updated_spindle.label)
                self.spindle_list.item(current_row, 0).setData(QtCore.Qt.UserRole, updated_spindle)
                self.spindle_list.item(current_row, 1).setText(power)

    def remove_spindle(self):
        current_row = self.spindle_list.currentRow()
        if current_row >= 0 and self.spindle_list.rowCount() > 0:
            self.spindle_list.removeRow(current_row)
            del self.machine.spindles[current_row]
            self._update_button_states()

    def launch_rigidity_wizard(self):
        if self.has_rigidity:
            wizard = RigidityWizard(self.machine, self)
            if not wizard.exec_():
                return

            rigidities = wizard.get_rigidities()
            for axis_name, edits in self.axis_edits.items():
                axis = self.machine.axes[axis_name]
                if isinstance(axis, LinearAxis):
                    rigidity, unit = rigidities.get(axis_name, (None, None))
                    if rigidity:
                        rigidity_edit = edits[2]  # Third item is rigidity_edit
                        rigidity_edit.setValue(rigidity.Value)
                elif isinstance(axis, AngularAxis):
                    rigidity, unit = rigidities.get(axis_name, (None, None))
                    if rigidity:
                        rigidity_edit = edits[0]
                        rigidity_edit.setValue(rigidity.Value)

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
        for axis_name, edits in self.axis_edits.items():
            axis = self.machine.axes[axis_name]
            if isinstance(axis, LinearAxis):
                start_edit, end_edit, rigidity_edit, max_feed_edit = edits
                start_val = start_edit.property("value")
                end_val = end_edit.property("value")
                max_feed_val = max_feed_edit.property("value")

                if start_val.Value < 0:
                    QtGui.QMessageBox.warning(
                        self,
                        translate("CAM", "Warning"),
                        translate("CAM", f"{axis_name}-axis min cannot be negative."),
                    )
                    return
                if end_val.Value <= start_val.Value:
                    QtGui.QMessageBox.warning(
                        self,
                        translate("CAM", "Warning"),
                        translate("CAM", f"{axis_name}-axis max must be greater than min."),
                    )
                    return
                if max_feed_val.Value <= 0:
                    QtGui.QMessageBox.warning(
                        self,
                        translate("CAM", "Warning"),
                        translate("CAM", f"{axis_name}-axis max feed rate must be positive."),
                    )
                    return

                axis.start = start_val
                axis.end = end_val
                axis.max_feed = max_feed_val

                if self.has_rigidity and rigidity_edit:
                    rigidity_val = rigidity_edit.value()
                    rigidity_quantity = FreeCAD.Units.Quantity(f"{rigidity_val} mm")
                    if rigidity_val < 0:
                        QtGui.QMessageBox.warning(
                            self,
                            translate("CAM", "Warning"),
                            translate("CAM", f"{axis_name}-axis rigidity cannot be negative."),
                        )
                        return
                    axis.rigidity = rigidity_quantity
            elif isinstance(axis, AngularAxis):
                rigidity_edit = edits
                if self.has_rigidity and rigidity_edit:
                    rigidity_val = rigidity_edit.value()
                    rigidity_quantity = FreeCAD.Units.Quantity(f"{rigidity_val} °")
                    if rigidity_val < 0:
                        QtGui.QMessageBox.warning(
                            self,
                            translate("CAM", "Warning"),
                            translate("CAM", f"{axis_name}-axis rigidity cannot be negative."),
                        )
                        return
                    axis.rigidity = rigidity_quantity

        # Get spindles from list
        spindles = []
        for i in range(self.spindle_list.rowCount()):
            spindles.append(self.spindle_list.item(i, 0).data(QtCore.Qt.UserRole))
        self.machine.spindles = spindles

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
