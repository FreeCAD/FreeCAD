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
from ..models.machine import Machine


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
        self.update_spindle_buttons_state()

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
            self.update_spindle_buttons_state()

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
            self.update_spindle_buttons_state()

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
            raise
            return

        super().accept()

    def update_machine(self):
        """Subclasses must implement this to update the specific machine."""
        raise NotImplementedError("Subclasses must implement update_machine()")

    def update_spindle_buttons_state(self):
        self.remove_spindle_btn.setEnabled(self.spindle_list.rowCount() > 1)
