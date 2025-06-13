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
import FreeCADGui
from ...spindle import Spindle


class SpindlePropertiesWidget(QtGui.QWidget):
    """Widget for editing spindle properties."""

    def __init__(self, spindle: Spindle, parent=None):
        super().__init__(parent)
        self.spindle = spindle
        self.layout = QtGui.QFormLayout(self)
        ui = FreeCADGui.UiLoader()

        # Label
        self._label_edit = QtGui.QLineEdit(self.spindle.label)
        self.layout.addRow("Label:", self._label_edit)

        # Max Power
        self._max_power_edit = ui.createWidget("Gui::QuantitySpinBox")
        self._max_power_edit.setProperty("value", self.spindle.max_power)
        self.layout.addRow("Max Power:", self._max_power_edit)

        # Min RPM
        self._min_rpm_edit = QtGui.QDoubleSpinBox()
        self._min_rpm_edit.setRange(0.0, 1000000.0)
        self._min_rpm_edit.setDecimals(0)
        self._min_rpm_edit.setSuffix(" RPM")
        self._min_rpm_edit.setValue(self.spindle.min_rpm)
        self.layout.addRow("Min RPM:", self._min_rpm_edit)

        # Max RPM
        self._max_rpm_edit = QtGui.QDoubleSpinBox()
        self._max_rpm_edit.setRange(0.0, 1000000.0)
        self._max_rpm_edit.setDecimals(0)
        self._max_rpm_edit.setSuffix(" RPM")
        self._max_rpm_edit.setValue(self.spindle.max_rpm)
        self.layout.addRow("Max RPM:", self._max_rpm_edit)

        # Max Torque
        self._max_torque_edit = QtGui.QDoubleSpinBox()
        self._max_torque_edit.setRange(0.0, 1000.0)
        self._max_torque_edit.setDecimals(2)
        self._max_torque_edit.setSuffix(" Nm")
        self._max_torque_edit.setValue(self.spindle.max_torque)
        self.layout.addRow("Max Torque:", self._max_torque_edit)

        # Peak Torque RPM
        self._peak_torque_rpm_edit = QtGui.QDoubleSpinBox()
        self._peak_torque_rpm_edit.setRange(0.0, 1000000.0)
        self._peak_torque_rpm_edit.setDecimals(0)
        self._peak_torque_rpm_edit.setSuffix(" RPM")
        self._peak_torque_rpm_edit.setValue(self.spindle.peak_torque_rpm)
        self.layout.addRow("Peak Torque RPM:", self._peak_torque_rpm_edit)

    def update_spindle(self):
        """Update the spindle object with values from the form fields."""
        self.spindle.label = self._label_edit.text()
        self.spindle.max_power = self._max_power_edit.property("value")
        self.spindle.min_rpm = self._min_rpm_edit.value()
        self.spindle.max_rpm = self._max_rpm_edit.value()
        self.spindle.max_torque = self._max_torque_edit.value()
        self.spindle.peak_torque_rpm = self._peak_torque_rpm_edit.value()
