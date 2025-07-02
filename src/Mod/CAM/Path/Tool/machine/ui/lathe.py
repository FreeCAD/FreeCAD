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
from ...spindle.ui.properties import SpindlePropertiesWidget
from ..models import Lathe
from .machine import MachinePropertiesDialog


translate = FreeCAD.Qt.translate


class LathePropertiesDialog(MachinePropertiesDialog):
    """Dialog for adding or editing a lathe machine."""

    def __init__(self, machine: Lathe, parent=None):
        super().__init__(machine, parent)

        # Add Spindle editor
        self.spindles_group.hide()
        self.spindle_editor = SpindlePropertiesWidget(machine.spindles[0], parent=self)
        spindle_group = QtGui.QGroupBox(translate("CAM", "Spindle"))
        spindle_layout = QtGui.QVBoxLayout()
        spindle_layout.setContentsMargins(0, 0, 0, 0)
        spindle_layout.addWidget(self.spindle_editor)
        spindle_group.setLayout(spindle_layout)
        self.layout.insertWidget(2, spindle_group)  # Insert after axis properties group

    def update_machine(self):
        self.spindle_editor.update_spindle()
