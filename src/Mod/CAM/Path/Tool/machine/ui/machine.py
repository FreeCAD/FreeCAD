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
from PySide import QtGui, QtCore
from PySide.QtGui import QComboBox, QTextEdit
import FreeCAD
import FreeCADGui
import Path
from ...spindle import Spindle
from ...spindle.ui.editor import SpindleEditorDialog
from ..models.machine import Machine, LinearAxis, AngularAxis


translate = FreeCAD.Qt.translate


class MachinePropertiesDialog(QtGui.QDialog):
    """Base dialog for adding or editing a machine with common properties."""

    def __init__(self, machine: Machine, parent=None):
        super().__init__(parent)
        self.machine = machine
        self.setWindowTitle(translate("CAM", "Edit Machine"))
        self.layout = QtGui.QVBoxLayout(self)

        # General group
        general_group = QtGui.QGroupBox(translate("CAM", "General"))
        general_layout = QtGui.QFormLayout()

        self.label_edit = QtGui.QLineEdit(machine.label)
        general_layout.addRow(translate("CAM", "Label:"), self.label_edit)

        ui = FreeCADGui.UiLoader()
        self.max_feed_edit = ui.createWidget("Gui::QuantitySpinBox")
        self.max_feed_edit.setProperty("value", machine.max_feed)
        general_layout.addRow(translate("CAM", "Max Feed Rate:"), self.max_feed_edit)

        self.post_processor_combo = QComboBox()
        available_post_processors = Path.Preferences.allAvailablePostProcessors()
        self.post_processor_combo.addItems(available_post_processors)
        if machine.post_processor in available_post_processors:
            self.post_processor_combo.setCurrentIndex(
                available_post_processors.index(machine.post_processor)
            )
        general_layout.addRow(translate("CAM", "Post Processor:"), self.post_processor_combo)

        self.post_processor_args = QtGui.QLineEdit(machine.post_processor_args)
        general_layout.addRow(translate("CAM", "Post Processor Args:"), self.post_processor_args)

        self.supported_args = QTextEdit()
        self.supported_args.setReadOnly(True)
        font = QtGui.QFont("Courier New")
        self.supported_args.setFont(font)
        self.supported_args.setMinimumHeight(200)
        general_layout.addRow(translate("CAM", "Supported Args:"), self.supported_args)

        self.post_processor_combo.currentIndexChanged.connect(self.update_post_processor_args)
        self.update_post_processor_args(self.post_processor_combo.currentIndex())

        general_group.setLayout(general_layout)
        self.layout.addWidget(general_group)

        # Axis Properties group
        axis_properties_group = QtGui.QGroupBox(translate("CAM", "Axis Properties"))
        axis_properties_layout = QtGui.QFormLayout()
        self.axis_edits = {}

        # Linear Axes
        for axis_name, axis in sorted(machine.axes.items()):
            if not isinstance(axis, LinearAxis):
                continue
            start_edit = ui.createWidget("Gui::QuantitySpinBox")
            start_edit.setProperty("value", axis.start)
            end_edit = ui.createWidget("Gui::QuantitySpinBox")
            end_edit.setProperty("unit", "mm")
            end_edit.setProperty("value", axis.end or FreeCAD.Units.Quantity("0 mm"))

            # Use FreeCAD.Units.Quantity does not support distance/N, so
            # we use QDoubleSpinBox for rigidity to avoid unit issues
            rigidity_edit = QtGui.QDoubleSpinBox()
            rigidity_edit.setDecimals(6)
            rigidity_edit.setMinimum(0.0)
            rigidity_edit.setValue(axis.rigidity.Value)
            rigidity_edit.setSuffix(" mm/N")

            axis_layout = QtGui.QHBoxLayout()
            axis_layout.addWidget(start_edit)
            axis_layout.addWidget(end_edit)
            axis_layout.addWidget(rigidity_edit)
            axis_properties_layout.addRow(
                f"{axis_name.capitalize()}-Axis (min, max, rigidity):", axis_layout
            )
            self.axis_edits[axis_name] = start_edit, end_edit, rigidity_edit

        # Angular axis
        for axis_name, axis in sorted(machine.axes.items()):
            if not isinstance(axis, AngularAxis):
                continue

            # Use FreeCAD.Units.Quantity does not support rad/N, so
            # we use QDoubleSpinBox for rigidity to avoid unit issues
            rigidity_x_edit = QtGui.QDoubleSpinBox()
            rigidity_x_edit.setDecimals(6)
            rigidity_x_edit.setMinimum(0.0)
            rigidity_x_edit.setValue(axis.rigidity_x.Value)
            rigidity_x_edit.setSuffix(" rad/N")
            rigidity_y_edit = QtGui.QDoubleSpinBox()
            rigidity_y_edit.setDecimals(6)
            rigidity_y_edit.setMinimum(0.0)
            rigidity_y_edit.setValue(axis.rigidity_y.Value)
            rigidity_y_edit.setSuffix(" rad/N")

            axis_layout = QtGui.QHBoxLayout()
            axis_layout.addWidget(rigidity_x_edit)
            axis_layout.addWidget(rigidity_y_edit)
            axis_properties_layout.addRow(
                f"{axis_name.capitalize()}-Axis (rigidity X/Y):", axis_layout
            )
            self.axis_edits[axis_name] = rigidity_x_edit, rigidity_y_edit

        axis_properties_group.setLayout(axis_properties_layout)
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

    def update_post_processor_args(self, index):
        post_processor_name = self.post_processor_combo.itemText(index)
        post = Path.Post.Processor.PostProcessorFactory.get_post_processor(
            None, post_processor_name
        )
        if not post:
            return
        args = post.tooltipArgs or translate("CAM", "No arguments found")
        self.supported_args.setText(args)

    def _add_spindle_to_list(self, spindle: Spindle):
        row_position = self.spindle_list.rowCount()
        self.spindle_list.insertRow(row_position)

        label_item = QtGui.QTableWidgetItem(spindle.label)
        label_item.setData(QtCore.Qt.UserRole, spindle)
        self.spindle_list.setItem(row_position, 0, label_item)

        kw_item = QtGui.QTableWidgetItem(spindle.max_power.UserString)
        self.spindle_list.setItem(row_position, 1, kw_item)

    def add_spindle(self):
        label = translate("CAM", f"Spindle {self.spindle_list.rowCount()+1}")
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

        # Check max feed
        max_feed = self.max_feed_edit.property("value")
        if max_feed.Value <= 0:
            QtGui.QMessageBox.warning(
                self,
                translate("CAM", "Warning"),
                translate("CAM", "Max feed rate must be positive."),
            )
            return
        self.machine.max_feed = max_feed

        self.machine.post_processor = self.post_processor_combo.currentText()
        self.machine.post_processor_args = self.post_processor_args.text()

        # Update axis properties
        for axis_name, edits in self.axis_edits.items():
            axis = self.machine.axes[axis_name]
            if isinstance(axis, LinearAxis):
                start_edit, end_edit, rigidity_edit = edits
                start_val = start_edit.property("value")
                end_val = end_edit.property("value")
                rigidity_val = rigidity_edit.value()
                rigidity_quantity = FreeCAD.Units.Quantity(f"{rigidity_val} mm/N")
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
                if rigidity_val < 0:
                    QtGui.QMessageBox.warning(
                        self,
                        translate("CAM", "Warning"),
                        translate("CAM", f"{axis_name}-axis rigidity cannot be negative."),
                    )
                    return
                axis.start = start_val
                axis.end = end_val
                axis.rigidity = rigidity_quantity
            elif isinstance(axis, AngularAxis):
                rigidity_x_edit, rigidity_y_edit = edits
                rigidity_x_val = rigidity_x_edit.value()
                rigidity_y_val = rigidity_y_edit.value()
                rigidity_x_quantity = FreeCAD.Units.Quantity(f"{rigidity_x_val} rad/N")
                rigidity_y_quantity = FreeCAD.Units.Quantity(f"{rigidity_y_val} rad/N")
                if rigidity_x_val < 0:
                    QtGui.QMessageBox.warning(
                        self,
                        translate("CAM", "Warning"),
                        translate("CAM", f"{axis_name}-axis rigidity X cannot be negative."),
                    )
                    return
                if rigidity_y_val < 0:
                    QtGui.QMessageBox.warning(
                        self,
                        translate("CAM", "Warning"),
                        translate("CAM", f"{axis_name}-axis rigidity Y cannot be negative."),
                    )
                    return
                axis.rigidity_x = rigidity_x_quantity
                axis.rigidity_y = rigidity_y_quantity

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

    def update_machine(self):
        """Subclasses must implement this to update the specific machine."""
        raise NotImplementedError("Subclasses must implement update_machine()")

    def _update_button_states(self):
        spindle_selected = self.spindle_list.currentRow() >= 0
        self.edit_spindle_btn.setEnabled(spindle_selected)
        self.remove_spindle_btn.setEnabled(spindle_selected)
