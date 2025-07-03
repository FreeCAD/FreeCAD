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


class VelocitySpinBox(QtGui.QDoubleSpinBox):
    def __init__(self):
        super().__init__()

        # Get user-preferred unit for feed rate
        _, self.unit_factor, self.preferred_unit = FreeCAD.Units.Quantity(
            1, FreeCAD.Units.Velocity
        ).getUserPreferred()
        self.display_unit = "mm/min" if "mm/" in self.preferred_unit else "ft/min"

        self.setDecimals(2)
        self.setMinimum(0)
        self.setMaximum(100000 * self.unit_factor)
        self.setSingleStep(1.0)
        self.setSuffix(f" {self.display_unit}")

        # Internal storage (Quantity in mm/min)
        self._value = FreeCAD.Units.Quantity("0 mm/min")

        # Connections
        self.valueChanged.connect(self._on_input_changed)

    def _on_input_changed(self, value):
        # Convert input value (in display_unit) to mm/min
        self._value = FreeCAD.Units.Quantity(f"{value} {self.display_unit}")
        self.blockSignals(True)
        self.setValue(self._value)
        self.blockSignals(False)

    def value(self):
        """Return value as Quantity in mm/min"""
        return self._value

    def setValue(self, value):
        """Set value as Quantity in mm/min"""
        if isinstance(value, FreeCAD.Units.Quantity):
            self._value = value
        else:
            self._value = FreeCAD.Units.Quantity(f"{value} mm/min")
        super().setValue(self._value.getValueAs(self.display_unit))


class AngularRigiditySpinBox(QtGui.QDoubleSpinBox):
    def __init__(self):
        super().__init__()
        self.setDecimals(2)
        self.setMinimum(0)
        self.setMaximum(100000)
        self.setSingleStep(1.0)
        self.setSuffix(" °/N")
        self._value = FreeCAD.Units.Quantity("0 deg")
        self.valueChanged.connect(self._on_input_changed)

    def _on_input_changed(self, value):
        self._value = FreeCAD.Units.Quantity(f"{value} deg")
        self.blockSignals(True)
        self.setValue(self._value)
        self.blockSignals(False)

    def value(self):
        return self._value

    def setValue(self, value):
        # FreeCAD.Units.Quantity does not support deg/N directly, so we use deg
        if isinstance(value, FreeCAD.Units.Quantity):
            self._value = value
        else:
            self._value = FreeCAD.Units.Quantity(f"{value} deg")
        super().setValue(self._value)


class MmPerNewtonSpinBox(QtGui.QDoubleSpinBox):
    def __init__(self):
        super().__init__()
        self.setDecimals(2)
        self.setMinimum(0)
        self.setMaximum(100000)
        self.setSingleStep(1.0)
        self.setSuffix(" mm/N")
        self._value = FreeCAD.Units.Quantity("0 mm/N")
        self.valueChanged.connect(self._on_input_changed)

    def _on_input_changed(self, value):
        self._value = FreeCAD.Units.Quantity(f"{value} mm/N")
        self.blockSignals(True)
        self.setValue(self._value)
        self.blockSignals(False)

    def value(self):
        return self._value

    def setValue(self, value):
        if isinstance(value, FreeCAD.Units.Quantity):
            self._value = value
        else:
            self._value = FreeCAD.Units.Quantity(f"{value} mm/N")
        super().setValue(self._value.getValueAs("mm/N"))


class PowerSpinBox(QtGui.QDoubleSpinBox):
    def __init__(self):
        super().__init__()
        self.setDecimals(2)
        self.setMinimum(0)
        self.setMaximum(100000)
        self.setSingleStep(1.0)
        self.setSuffix(" W")
        self._value = FreeCAD.Units.Quantity("0 W")
        self.valueChanged.connect(self._on_input_changed)

    def _on_input_changed(self, value):
        self._value = FreeCAD.Units.Quantity(f"{value} W")
        self.blockSignals(True)
        self.setValue(self._value)
        self.blockSignals(False)

    def value(self):
        return self._value

    def setValue(self, value):
        if isinstance(value, FreeCAD.Units.Quantity):
            self._value = value
        else:
            self._value = FreeCAD.Units.Quantity(f"{value} W")
        super().setValue(self._value.getValueAs("W"))


class RPMSpinBox(QtGui.QSpinBox):
    def __init__(self):
        super().__init__()
        self.setMinimum(0)
        self.setMaximum(200000)
        self.setSingleStep(1)
        self.setSuffix(" rpm")


class TorqueSpinBox(QtGui.QDoubleSpinBox):
    def __init__(self):
        super().__init__()
        self.setDecimals(2)
        self.setMinimum(0)
        self.setMaximum(100000)
        self.setSingleStep(1.0)
        self.setSuffix(" Nm")
