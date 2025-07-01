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
from typing import cast
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

        ui = FreeCADGui.UiLoader()

        # Add Lathe properties group
        lathe_properties_group = QtGui.QGroupBox(translate("CAM", "Lathe Properties"))
        lathe_properties_layout = QtGui.QFormLayout()
        self.max_diameter_edit = ui.createWidget("Gui::QuantitySpinBox")
        self.max_diameter_edit.setProperty("value", machine.max_workpiece_diameter)
        lathe_properties_layout.addRow(
            translate("CAM", "Maximum workpiece diameter"),
            self.max_diameter_edit,
        )
        lathe_properties_group.setLayout(lathe_properties_layout)
        self.layout.insertWidget(1, lathe_properties_group)  # Insert after general group

        # Add Spindle editor
        self.spindles_group.hide()
        self.spindle_editor = SpindlePropertiesWidget(machine.spindles[0], parent=self)
        spindle_group = QtGui.QGroupBox(translate("CAM", "Spindle"))
        spindle_layout = QtGui.QVBoxLayout()
        spindle_layout.setContentsMargins(0, 0, 0, 0)
        spindle_layout.addWidget(self.spindle_editor)
        spindle_group.setLayout(spindle_layout)
        self.layout.insertWidget(3, spindle_group)  # Insert after axis properties group

    def update_machine(self):
        machine = cast(Lathe, self.machine)
        max_diameter = self.max_diameter_edit.property("value")
        if max_diameter.Value <= 0:
            QtGui.QMessageBox.warning(
                self,
                translate("CAM", "Warning"),
                translate("CAM", "Maximum workpiece diameter must be positive."),
            )
            return None

        # Update existing machine
        machine.max_workpiece_diameter = max_diameter
        self.spindle_editor.update_spindle()
