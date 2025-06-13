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
from ...spindle.ui.properties import SpindlePropertiesWidget
from ..models import Lathe
from .machine import MachinePropertiesDialog


translate = FreeCAD.Qt.translate


class LathePropertiesDialog(MachinePropertiesDialog):
    """Dialog for adding or editing a lathe machine."""

    def __init__(self, machine: Lathe, parent=None):
        super().__init__(machine, parent)
        self.setWindowTitle(
            translate("CAM_PreferencesLathe", "Add Lathe")
            if not machine
            else translate("CAM_PreferencesLathe", "Edit Lathe")
        )

        ui = FreeCADGui.UiLoader()

        # Add Lathe properties group
        lathe_properties_group = QtGui.QGroupBox(
            translate("CAM_PreferencesLathe", "Lathe properties")
        )
        lathe_properties_layout = QtGui.QFormLayout()
        self.max_diameter_edit = ui.createWidget("Gui::QuantitySpinBox")
        self.max_diameter_edit.setProperty(
            "value",
            machine.max_workpiece_diameter if machine else FreeCAD.Units.Quantity("300 mm"),
        )
        lathe_properties_layout.addRow(
            translate("CAM_PreferencesLathe", "Max Workpiece Diameter:"),
            self.max_diameter_edit,
        )
        lathe_properties_group.setLayout(lathe_properties_layout)
        self.layout.insertWidget(1, lathe_properties_group)  # Insert after general group

        # Add Axis Extents group
        extents_group = QtGui.QGroupBox(translate("CAM_PreferencesLathe", "Axis Extents"))
        extents_layout = QtGui.QFormLayout()
        self.x_min_edit = ui.createWidget("Gui::QuantitySpinBox")
        self.x_min_edit.setProperty("value", machine.x_min)
        self.x_max_edit = ui.createWidget("Gui::QuantitySpinBox")
        self.x_max_edit.setProperty("value", machine.x_max)
        self.z_min_edit = ui.createWidget("Gui::QuantitySpinBox")
        self.z_min_edit.setProperty("value", machine.z_min)
        self.z_max_edit = ui.createWidget("Gui::QuantitySpinBox")
        self.z_max_edit.setProperty("value", machine.z_max)
        x_layout = QtGui.QHBoxLayout()
        x_layout.addWidget(self.x_min_edit)
        x_layout.addWidget(self.x_max_edit)
        z_layout = QtGui.QHBoxLayout()
        z_layout.addWidget(self.z_min_edit)
        z_layout.addWidget(self.z_max_edit)
        extents_layout.addRow(translate("CAM_PreferencesLathe", "X-Axis (min, max):"), x_layout)
        extents_layout.addRow(translate("CAM_PreferencesLathe", "Z-Axis (min, max):"), z_layout)
        extents_group.setLayout(extents_layout)
        self.layout.insertWidget(2, extents_group)  # Insert after lathe properties group

        # Add Rigidity group
        rigidity_group = QtGui.QGroupBox(translate("CAM_PreferencesLathe", "Rigidity"))
        rigidity_layout = QtGui.QFormLayout()
        self.rigidity_x_edit = ui.createWidget("Gui::QuantitySpinBox")
        self.rigidity_x_edit.setProperty("value", machine.rigidity_x)
        self.rigidity_z_edit = ui.createWidget("Gui::QuantitySpinBox")
        self.rigidity_z_edit.setProperty("value", machine.rigidity_z)
        self.rigidity_rot_edit = ui.createWidget("Gui::QuantitySpinBox")
        self.rigidity_rot_edit.setProperty("value", machine.rigidity_rotational)
        rigidity_layout.addRow(translate("CAM_PreferencesLathe", "X (mm/N):"), self.rigidity_x_edit)
        rigidity_layout.addRow(translate("CAM_PreferencesLathe", "Z (mm/N):"), self.rigidity_z_edit)
        rigidity_layout.addRow(
            translate("CAM_PreferencesLathe", "Rotational (mm/rad):"), self.rigidity_rot_edit
        )
        rigidity_group.setLayout(rigidity_layout)
        self.layout.insertWidget(3, rigidity_group)  # Insert after extents group

        # Add Spindle editor
        self.spindles_group.hide()
        self.spindle_editor = SpindlePropertiesWidget(machine.spindles[0], parent=self)
        spindle_group = QtGui.QGroupBox(translate("CAM_PreferencesLathe", "Spindle"))
        spindle_layout = QtGui.QVBoxLayout()
        spindle_layout.setContentsMargins(0, 0, 0, 0)
        spindle_layout.addWidget(self.spindle_editor)
        spindle_group.setLayout(spindle_layout)
        self.layout.insertWidget(4, spindle_group)  # Insert after rigidity group

    def update_machine(self):
        x_min = self.x_min_edit.property("value")
        x_max = self.x_max_edit.property("value")
        z_min = self.z_min_edit.property("value")
        z_max = self.z_max_edit.property("value")
        rigidity_x = self.rigidity_x_edit.property("value")
        rigidity_z = self.rigidity_z_edit.property("value")
        rigidity_rot = self.rigidity_rot_edit.property("value")
        max_diameter = self.max_diameter_edit.property("value")

        if not all(
            isinstance(q, FreeCAD.Units.Quantity)
            for q in [
                x_min,
                x_max,
                z_min,
                z_max,
                rigidity_x,
                rigidity_z,
                rigidity_rot,
                max_diameter,
            ]
        ):
            QtGui.QMessageBox.warning(
                self,
                translate("CAM_PreferencesLathe", "Warning"),
                translate("CAM_PreferencesLathe", "Invalid quantity format in one or more fields."),
            )
            return None

        if x_min >= x_max or z_min >= z_max:
            QtGui.QMessageBox.warning(
                self,
                translate("CAM_PreferencesLathe", "Warning"),
                translate("CAM_PreferencesLathe", "Max axis extents must be greater than min."),
            )
            return None

        if any(r.Value < 0 for r in [rigidity_x, rigidity_z, rigidity_rot]):
            QtGui.QMessageBox.warning(
                self,
                translate("CAM_PreferencesLathe", "Warning"),
                translate("CAM_PreferencesLathe", "Rigidity values cannot be negative."),
            )
            return None

        if max_diameter.Value <= 0:
            QtGui.QMessageBox.warning(
                self,
                translate("CAM_PreferencesLathe", "Warning"),
                translate("CAM_PreferencesLathe", "Max workpiece diameter must be positive."),
            )
            return None

        # Update existing machine
        self.machine.x_min = x_min
        self.machine.x_max = x_max
        self.machine.z_min = z_min
        self.machine.z_max = z_max
        self.machine.rigidity_x = rigidity_x
        self.machine.rigidity_z = rigidity_z
        self.machine.rigidity_rotational = rigidity_rot
        self.machine.max_workpiece_diameter = max_diameter

        # Also update spindle values.
        self.spindle_editor.update_spindle()
