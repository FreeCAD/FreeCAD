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
from ..models.axis import AngularAxis
from ..models.machine import Machine, MachineFeature
from .component import MachineComponentWidget
from .rigidity import RigidityWizard


class AngularAxisWidget(MachineComponentWidget):
    """
    Widget for editing AngularAxis properties, including a rigidity wizard button.
    """

    def __init__(self, angular_axis: AngularAxis, parent=None):
        super().__init__(angular_axis, angular_axis.get_attribute_configs(), parent)

        self.angular_axis = angular_axis
        machine = cast(Machine, angular_axis.root)

        if MachineFeature.MILLING_3D in machine.feature_flags:
            # Add Rigidity Wizard Button
            rigidity_wizard_button = QtGui.QPushButton(
                FreeCAD.Qt.translate("CAM", "Rigidity Wizard…")
            )
            rigidity_wizard_button.clicked.connect(self._open_rigidity_wizard)
            self.layout().addRow("", rigidity_wizard_button)

    def _open_rigidity_wizard(self):
        """Opens the rigidity wizard dialog."""
        wizard = RigidityWizard(self.angular_axis, self)
        if wizard.exec_() == QtGui.QDialog.Accepted:
            rigidities = wizard.get_rigidities()
            if "angular_rigidity" in rigidities:
                self.angular_axis.angular_rigidity = rigidities["angular_rigidity"]
            if "rigidity_x" in rigidities:
                self.angular_axis.rigidity_x = rigidities["rigidity_x"]
            if "rigidity_y" in rigidities:
                self.angular_axis.rigidity_y = rigidities["rigidity_y"]
            self.update_ui_from_component()
