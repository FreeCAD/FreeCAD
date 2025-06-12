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

import uuid
import math
from typing import Optional
import FreeCAD
from ...assets import Asset
from FreeCAD import Units


class Spindle(Asset):
    """
    A class to represent a spindle with its operational parameters.
    """

    asset_type = "spindle"

    def __init__(
        self,
        label: str,
        max_power: FreeCAD.Units.Quantity = Units.Quantity("2 kW"),
        min_rpm: float = 3000.0,
        max_rpm: float = 60000.0,
        max_torque: float = 10.0,
        peak_torque_rpm: float = 20000.0,
        id: Optional[str] = None,
    ) -> None:
        """
        Initializes a Spindle object.

        Args:
            max_power: Maximum power (e.g., '2 kW').
            min_rpm: Minimum RPM (e.g., 3000 RPM).
            max_rpm: Maximum RPM (e.g., 60000 RPM).
            max_torque: Maximum torque in Nm (e.g., 10 Nm).
            peak_torque_rpm: RPM at peak torque (e.g., 20000 RPM).
            id: Unique identifier (optional).
            label: User-friendly label (optional).
        """
        super().__init__()
        self._id = id or str(uuid.uuid1())
        self._label = label or self._id
        self._max_power = max_power
        self._min_rpm = min_rpm
        self._max_rpm = max_rpm
        self._max_torque = max_torque
        self._peak_torque_rpm = peak_torque_rpm

    def get_id(self) -> str:
        """Returns the unique ID of the spindle."""
        return self._id

    @property
    def id(self) -> str:
        """Returns the unique ID of the spindle."""
        return self._id

    @property
    def label(self) -> str:
        """Returns the spindle's label."""
        return self._label

    @label.setter
    def label(self, value: str) -> None:
        self._label = value

    @property
    def max_power(self) -> FreeCAD.Units.Quantity:
        return self._max_power

    @max_power.setter
    def max_power(self, value: FreeCAD.Units.Quantity) -> None:
        self._max_power = value

    @property
    def min_rpm(self) -> float:
        return self._min_rpm

    @min_rpm.setter
    def min_rpm(self, value: float) -> None:
        self._min_rpm = value

    @property
    def max_rpm(self) -> float:
        return self._max_rpm

    @max_rpm.setter
    def max_rpm(self, value: float) -> None:
        self._max_rpm = value

    @property
    def max_torque(self) -> float:
        return self._max_torque

    @max_torque.setter
    def max_torque(self, value: float) -> None:
        self._max_torque = value

    @property
    def peak_torque_rpm(self) -> float:
        return self._peak_torque_rpm

    @peak_torque_rpm.setter
    def peak_torque_rpm(self, value: float) -> None:
        self._peak_torque_rpm = value

    def get_torque_at_rpm(self, rpm: float) -> float:
        """
        Calculates torque at a given RPM.

        Args:
            rpm: RPM value (e.g., 5000.0).

        Returns:
            Torque at the given RPM (e.g., 5.0).
        """
        max_torque = self.max_torque
        peak_torque_rpm = self.peak_torque_rpm
        max_power = self.max_power

        # Convert RPM to Hz for power calculation (1 RPM = 1/60 Hz)
        rpm_hz = rpm / 60.0
        peak_rpm_hz = peak_torque_rpm / 60.0

        if rpm_hz <= peak_rpm_hz:
            torque_nm = max_torque * (rpm_hz / peak_rpm_hz)
        else:
            # Power (W) = Torque (Nm) * Angular Velocity (rad/s)
            # Angular Velocity (rad/s) = 2 * pi * RPM / 60
            # Torque (Nm) = Power (W) / (2 * pi * RPM / 60)
            torque_nm = (
                max_power.getValueAs("W").Value / (2 * math.pi * rpm_hz)
                if rpm_hz > 0
                else float("inf")
            )
            torque_nm = min(max_torque, torque_nm)

        return torque_nm

    def validate(self) -> None:
        """Validates spindle parameters."""
        if self.min_rpm >= self.max_rpm:
            raise AttributeError("Max RPM must be larger than min RPM")
        if self.peak_torque_rpm > self.max_rpm:
            raise AttributeError("Peak Torque RPM must be less than max RPM")

    def dump(self, do_print: bool = True) -> str:
        """
        Dumps spindle info to console or as a string.

        Args:
            do_print: If True, prints; if False, returns string.

        Returns:
            Formatted string if do_print is False.
        """
        output = f"  Spindle: {self.label} ({self.id})\n"
        output += f"    Max power: {self.max_power.UserString}\n"
        output += f"    RPM: {self.min_rpm} - {self.max_rpm}\n"
        output += f"    Max torque: {self.max_torque} Nm\n"
        output += f"    Peak torque RPM: {self.peak_torque_rpm}\n"
        if do_print:
            print(output)
        return output
