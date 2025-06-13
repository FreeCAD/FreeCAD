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
import FreeCAD
import FreeCADGui
from .machine import MachinePropertiesDialog
from ..models import Mill


translate = FreeCAD.Qt.translate


class MillPropertiesDialog(MachinePropertiesDialog):
    """Dialog for adding or editing a mill machine."""

    def __init__(self, machine: Mill, parent=None):
        super().__init__(machine, parent)
        self.setWindowTitle(translate("CAM", "Edit Mill"))

        # Axis Extents group
        extents_group = QtGui.QGroupBox(translate("CAM", "Axis Extents"))
        extents_layout = QtGui.QFormLayout()
        ui = FreeCADGui.UiLoader()
        self.x_min_edit = ui.createWidget("Gui::QuantitySpinBox")
        self.x_min_edit.setProperty("value", machine.x_min)
        self.x_max_edit = ui.createWidget("Gui::QuantitySpinBox")
        self.x_max_edit.setProperty("value", machine.x_max)
        self.y_min_edit = ui.createWidget("Gui::QuantitySpinBox")
        self.y_min_edit.setProperty("value", machine.y_min)
        self.y_max_edit = ui.createWidget("Gui::QuantitySpinBox")
        self.y_max_edit.setProperty("value", machine.y_max)
        self.z_min_edit = ui.createWidget("Gui::QuantitySpinBox")
        self.z_min_edit.setProperty("value", machine.z_min)
        self.z_max_edit = ui.createWidget("Gui::QuantitySpinBox")
        self.z_max_edit.setProperty("value", machine.z_max)
        x_layout = QtGui.QHBoxLayout()
        x_layout.addWidget(self.x_min_edit)
        x_layout.addWidget(self.x_max_edit)
        y_layout = QtGui.QHBoxLayout()
        y_layout.addWidget(self.y_min_edit)
        y_layout.addWidget(self.y_max_edit)
        z_layout = QtGui.QHBoxLayout()
        z_layout.addWidget(self.z_min_edit)
        z_layout.addWidget(self.z_max_edit)
        extents_layout.addRow(translate("CAM", "X-Axis (min, max):"), x_layout)
        extents_layout.addRow(translate("CAM", "Y-Axis (min, max):"), y_layout)
        extents_layout.addRow(translate("CAM", "Z-Axis (min, max):"), z_layout)
        extents_group.setLayout(extents_layout)
        self.layout.insertWidget(2, extents_group)  # Insert after spindles group

        # Rigidity group
        rigidity_group = QtGui.QGroupBox(translate("CAM", "Rigidity"))
        rigidity_layout = QtGui.QFormLayout()
        self.rigidity_x_edit = ui.createWidget("Gui::QuantitySpinBox")
        self.rigidity_x_edit.setProperty("value", machine.rigidity_x)
        self.rigidity_y_edit = ui.createWidget("Gui::QuantitySpinBox")
        self.rigidity_y_edit.setProperty("value", machine.rigidity_y)
        self.rigidity_z_edit = ui.createWidget("Gui::QuantitySpinBox")
        self.rigidity_z_edit.setProperty("value", machine.rigidity_z)
        self.rigidity_rot_edit = ui.createWidget("Gui::QuantitySpinBox")
        self.rigidity_rot_edit.setProperty("value", machine.rigidity_rotational)
        rigidity_layout.addRow(translate("CAM", "X (mm/N):"), self.rigidity_x_edit)
        rigidity_layout.addRow(translate("CAM", "Y (mm/N):"), self.rigidity_y_edit)
        rigidity_layout.addRow(translate("CAM", "Z (mm/N):"), self.rigidity_z_edit)
        rigidity_layout.addRow(translate("CAM", "Rotational (mm/rad):"), self.rigidity_rot_edit)
        rigidity_group.setLayout(rigidity_layout)
        self.layout.insertWidget(3, rigidity_group)  # Insert after extents group

    def update_machine(self):
        assert isinstance(self.machine, Mill)
        x_min = self.x_min_edit.property("value")
        x_max = self.x_max_edit.property("value")
        y_min = self.y_min_edit.property("value")
        y_max = self.y_max_edit.property("value")
        z_min = self.z_min_edit.property("value")
        z_max = self.z_max_edit.property("value")
        rigidity_x = self.rigidity_x_edit.property("value")
        rigidity_y = self.rigidity_y_edit.property("value")
        rigidity_z = self.rigidity_z_edit.property("value")
        rigidity_rot = self.rigidity_rot_edit.property("value")

        if not all(
            isinstance(q, FreeCAD.Units.Quantity)
            for q in [
                x_min,
                x_max,
                y_min,
                y_max,
                z_min,
                z_max,
                rigidity_x,
                rigidity_y,
                rigidity_z,
                rigidity_rot,
            ]
        ):
            QtGui.QMessageBox.warning(
                self,
                translate("CAM", "Warning"),
                translate("CAM", "Invalid quantity format in one or more fields."),
            )
            return None

        if x_min >= x_max or y_min >= y_max or z_min >= z_max:
            QtGui.QMessageBox.warning(
                self,
                translate("CAM", "Warning"),
                translate("CAM", "Max axis extents must be greater than min."),
            )
            return None

        if any(r.Value < 0 for r in [rigidity_x, rigidity_y, rigidity_z, rigidity_rot]):
            QtGui.QMessageBox.warning(
                self,
                translate("CAM", "Warning"),
                translate("CAM", "Rigidity values cannot be negative."),
            )
            return None

        # Update existing machine
        self.machine.x_min = x_min
        self.machine.x_max = x_max
        self.machine.y_min = y_min
        self.machine.y_max = y_max
        self.machine.z_min = z_min
        self.machine.z_max = z_max
        self.machine.rigidity_x = rigidity_x
        self.machine.rigidity_y = rigidity_y
        self.machine.rigidity_z = rigidity_z
        self.machine.rigidity_rotational = rigidity_rot
