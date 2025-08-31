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
import json
import FreeCAD
from FreeCAD import Base
from typing import Optional, Union, Mapping, List
from ...assets import Asset, AssetUri, AssetSerializer


class Machine(Asset):
    """Represents a machine with various operational parameters."""

    asset_type: str = "machine"
    API_VERSION = 1

    UNIT_CONVERSIONS = {
        "hp": 745.7,  # hp to W
        "in-lbf": 0.112985,  # in-lbf to N*m
        "inch/min": 25.4,  # inch/min to mm/min
        "rpm": 1.0 / 60.0,  # rpm to 1/s
        "kW": 1000.0,  # kW to W
        "Nm": 1.0,  # Nm to N*m
        "mm/min": 1.0,  # mm/min to mm/min
    }

    def __init__(
        self,
        label: str = "Machine",
        max_power: Union[int, float, FreeCAD.Units.Quantity] = 2,
        min_rpm: Union[int, float, FreeCAD.Units.Quantity] = 3000,
        max_rpm: Union[int, float, FreeCAD.Units.Quantity] = 60000,
        max_torque: Optional[Union[int, float, FreeCAD.Units.Quantity]] = None,
        peak_torque_rpm: Optional[Union[int, float, FreeCAD.Units.Quantity]] = None,
        min_feed: Union[int, float, FreeCAD.Units.Quantity] = 1,
        max_feed: Union[int, float, FreeCAD.Units.Quantity] = 2000,
        id: Optional[str] = None,
    ) -> None:
        """
        Initializes a Machine object.

        Args:
            label: The label of the machine.
            max_power: The maximum power of the machine (kW or Quantity).
            min_rpm: The minimum RPM of the machine (RPM or Quantity).
            max_rpm: The maximum RPM of the machine (RPM or Quantity).
            max_torque: The maximum torque of the machine (Nm or Quantity).
            peak_torque_rpm: The RPM at which peak torque is achieved
                             (RPM or Quantity).
            min_feed: The minimum feed rate of the machine
                      (mm/min or Quantity).
            max_feed: The maximum feed rate of the machine
                      (mm/min or Quantity).
            id: The unique identifier of the machine.
        """
        self.id = id or str(uuid.uuid1())
        self._label = label

        # Initialize max_power (W)
        if isinstance(max_power, FreeCAD.Units.Quantity):
            self._max_power = max_power.getValueAs("W").Value
        elif isinstance(max_power, (int, float)):
            self._max_power = max_power * self.UNIT_CONVERSIONS["kW"]
        else:
            self._max_power = 2000.0

        # Initialize min_rpm (1/s)
        if isinstance(min_rpm, FreeCAD.Units.Quantity):
            try:
                self._min_rpm = min_rpm.getValueAs("1/s").Value
            except (Base.ParserError, ValueError):
                self._min_rpm = min_rpm.Value * self.UNIT_CONVERSIONS["rpm"]
        elif isinstance(min_rpm, (int, float)):
            self._min_rpm = min_rpm * self.UNIT_CONVERSIONS["rpm"]
        else:
            self._min_rpm = 3000 * self.UNIT_CONVERSIONS["rpm"]

        # Initialize max_rpm (1/s)
        if isinstance(max_rpm, FreeCAD.Units.Quantity):
            try:
                self._max_rpm = max_rpm.getValueAs("1/s").Value
            except (Base.ParserError, ValueError):
                self._max_rpm = max_rpm.Value * self.UNIT_CONVERSIONS["rpm"]
        elif isinstance(max_rpm, (int, float)):
            self._max_rpm = max_rpm * self.UNIT_CONVERSIONS["rpm"]
        else:
            self._max_rpm = 60000 * self.UNIT_CONVERSIONS["rpm"]

        # Initialize min_feed (mm/min)
        if isinstance(min_feed, FreeCAD.Units.Quantity):
            self._min_feed = min_feed.getValueAs("mm/min").Value
        elif isinstance(min_feed, (int, float)):
            self._min_feed = min_feed
        else:
            self._min_feed = 1.0

        # Initialize max_feed (mm/min)
        if isinstance(max_feed, FreeCAD.Units.Quantity):
            self._max_feed = max_feed.getValueAs("mm/min").Value
        elif isinstance(max_feed, (int, float)):
            self._max_feed = max_feed
        else:
            self._max_feed = 2000.0

        # Initialize peak_torque_rpm (1/s)
        if isinstance(peak_torque_rpm, FreeCAD.Units.Quantity):
            try:
                self._peak_torque_rpm = peak_torque_rpm.getValueAs("1/s").Value
            except (Base.ParserError, ValueError):
                self._peak_torque_rpm = peak_torque_rpm.Value * self.UNIT_CONVERSIONS["rpm"]
        elif isinstance(peak_torque_rpm, (int, float)):
            self._peak_torque_rpm = peak_torque_rpm * self.UNIT_CONVERSIONS["rpm"]
        else:
            self._peak_torque_rpm = self._max_rpm / 3

        # Initialize max_torque (N*m)
        if isinstance(max_torque, FreeCAD.Units.Quantity):
            self._max_torque = max_torque.getValueAs("Nm").Value
        elif isinstance(max_torque, (int, float)):
            self._max_torque = max_torque
        else:
            # Convert 1/s to rpm
            peak_rpm_for_calc = self._peak_torque_rpm * 60
            self._max_torque = (
                self._max_power * 9.5488 / peak_rpm_for_calc if peak_rpm_for_calc else float("inf")
            )

    def get_id(self) -> str:
        """Returns the unique identifier for the Machine instance."""
        return self.id

    def to_dict(self) -> dict:
        """Returns a dictionary representation of the Machine."""
        return {
            "version": self.API_VERSION,
            "id": self.id,
            "label": self.label,
            "max_power": self._max_power,  # W
            "min_rpm": self._min_rpm,  # 1/s
            "max_rpm": self._max_rpm,  # 1/s
            "max_torque": self._max_torque,  # Nm
            "peak_torque_rpm": self._peak_torque_rpm,  # 1/s
            "min_feed": self._min_feed,  # mm/min
            "max_feed": self._max_feed,  # mm/min
        }

    def to_bytes(self, serializer: AssetSerializer) -> bytes:
        """Serializes the Machine object to bytes using to_dict."""
        data_dict = self.to_dict()
        json_str = json.dumps(data_dict)
        return json_str.encode("utf-8")

    @classmethod
    def from_dict(cls, data_dict: dict, id: str) -> "Machine":
        """Creates a Machine instance from a dictionary."""
        machine = cls(
            label=data_dict.get("label", "Machine"),
            max_power=data_dict.get("max_power", 2000.0),  # W
            min_rpm=data_dict.get("min_rpm", 3000 * cls.UNIT_CONVERSIONS["rpm"]),  # 1/s
            max_rpm=data_dict.get("max_rpm", 60000 * cls.UNIT_CONVERSIONS["rpm"]),  # 1/s
            max_torque=data_dict.get("max_torque", None),  # Nm
            peak_torque_rpm=data_dict.get("peak_torque_rpm", None),  # 1/s
            min_feed=data_dict.get("min_feed", 1.0),  # mm/min
            max_feed=data_dict.get("max_feed", 2000.0),  # mm/min
            id=id,
        )
        return machine

    @classmethod
    def from_bytes(
        cls,
        data: bytes,
        id: str,
        dependencies: Optional[Mapping[AssetUri, Asset]],
    ) -> "Machine":
        """
        Deserializes bytes into a Machine instance using from_dict.
        """
        # If dependencies is None, it's fine as Machine doesn't use it.
        data_dict = json.loads(data.decode("utf-8"))
        return cls.from_dict(data_dict, id)

    @classmethod
    def dependencies(cls, data: bytes) -> List[AssetUri]:
        """Returns a list of AssetUri dependencies parsed from the serialized data."""
        return []  # Machine has no dependencies

    @property
    def max_power(self) -> FreeCAD.Units.Quantity:
        return FreeCAD.Units.Quantity(self._max_power, "W")

    @property
    def min_rpm(self) -> FreeCAD.Units.Quantity:
        return FreeCAD.Units.Quantity(self._min_rpm, "1/s")

    @property
    def max_rpm(self) -> FreeCAD.Units.Quantity:
        return FreeCAD.Units.Quantity(self._max_rpm, "1/s")

    @property
    def max_torque(self) -> FreeCAD.Units.Quantity:
        return FreeCAD.Units.Quantity(self._max_torque, "Nm")

    @property
    def peak_torque_rpm(self) -> FreeCAD.Units.Quantity:
        return FreeCAD.Units.Quantity(self._peak_torque_rpm, "1/s")

    @property
    def min_feed(self) -> FreeCAD.Units.Quantity:
        return FreeCAD.Units.Quantity(self._min_feed, "mm/min")

    @property
    def max_feed(self) -> FreeCAD.Units.Quantity:
        return FreeCAD.Units.Quantity(self._max_feed, "mm/min")

    @property
    def label(self) -> str:
        return self._label

    @label.setter
    def label(self, label: str) -> None:
        self._label = label

    def get_min_rpm_value(self) -> float:
        """Helper method to get minimum RPM value for display/testing."""
        return self._min_rpm * 60

    def get_max_rpm_value(self) -> float:
        """Helper method to get maximum RPM value for display/testing."""
        return self._max_rpm * 60

    def get_peak_torque_rpm_value(self) -> float:
        """Helper method to get peak torque RPM value for display/testing."""
        return self._peak_torque_rpm * 60

    def validate(self) -> None:
        """Validates the machine parameters."""
        if not self.label:
            raise AttributeError("Machine name is required")
        if self._peak_torque_rpm > self._max_rpm:
            err = ("Peak Torque RPM {ptrpm:.2f} must be less than max RPM " "{max_rpm:.2f}").format(
                ptrpm=self._peak_torque_rpm * 60, max_rpm=self._max_rpm * 60
            )
            raise AttributeError(err)
        if self._max_rpm <= self._min_rpm:
            raise AttributeError("Max RPM must be larger than min RPM")
        if self._max_feed <= self._min_feed:
            raise AttributeError("Max feed must be larger than min feed")

    def get_torque_at_rpm(self, rpm: Union[int, float, FreeCAD.Units.Quantity]) -> float:
        """
        Calculates the torque at a given RPM.

        Args:
            rpm: The RPM value (int, float, or Quantity).

        Returns:
            The torque at the given RPM in Nm.
        """
        if isinstance(rpm, FreeCAD.Units.Quantity):
            try:
                rpm_hz = rpm.getValueAs("1/s").Value
            except (Base.ParserError, ValueError):
                rpm_hz = rpm.Value * self.UNIT_CONVERSIONS["rpm"]
        else:
            rpm_hz = rpm * self.UNIT_CONVERSIONS["rpm"]
        max_torque_nm = self._max_torque
        peak_torque_rpm_hz = self._peak_torque_rpm
        peak_rpm_for_calc = peak_torque_rpm_hz * 60
        rpm_for_calc = rpm_hz * 60
        torque_at_current_rpm = (
            self._max_power * 9.5488 / rpm_for_calc if rpm_for_calc else float("inf")
        )
        if rpm_for_calc <= peak_rpm_for_calc:
            torque_at_current_rpm = (
                max_torque_nm / peak_rpm_for_calc * rpm_for_calc
                if peak_rpm_for_calc
                else float("inf")
            )
        return min(max_torque_nm, torque_at_current_rpm)

    def set_max_power(self, power: Union[int, float], unit: Optional[str] = None) -> None:
        """Sets the maximum power of the machine."""
        unit = unit or "kW"
        if unit in self.UNIT_CONVERSIONS:
            power_value = power * self.UNIT_CONVERSIONS[unit]
        else:
            power_value = FreeCAD.Units.Quantity(power, unit).getValueAs("W").Value
        self._max_power = power_value
        if self._max_power <= 0:
            raise AttributeError("Max power must be positive")

    def set_min_rpm(self, min_rpm: Union[int, float, FreeCAD.Units.Quantity]) -> None:
        """Sets the minimum RPM of the machine."""
        if isinstance(min_rpm, FreeCAD.Units.Quantity):
            try:
                min_rpm_value = min_rpm.getValueAs("1/s").Value
            except (Base.ParserError, ValueError):
                min_rpm_value = min_rpm.Value * self.UNIT_CONVERSIONS["rpm"]
        else:
            min_rpm_value = min_rpm * self.UNIT_CONVERSIONS["rpm"]
        self._min_rpm = min_rpm_value
        if self._min_rpm < 0:
            raise AttributeError("Min RPM cannot be negative")
        if self._min_rpm >= self._max_rpm:
            self._max_rpm = min_rpm_value + 1.0 / 60.0

    def set_max_rpm(self, max_rpm: Union[int, float, FreeCAD.Units.Quantity]) -> None:
        """Sets the maximum RPM of the machine."""
        if isinstance(max_rpm, FreeCAD.Units.Quantity):
            try:
                max_rpm_value = max_rpm.getValueAs("1/s").Value
            except (Base.ParserError, ValueError):
                max_rpm_value = max_rpm.Value * self.UNIT_CONVERSIONS["rpm"]
        else:
            max_rpm_value = max_rpm * self.UNIT_CONVERSIONS["rpm"]
        self._max_rpm = max_rpm_value
        if self._max_rpm <= 0:
            raise AttributeError("Max RPM must be positive")
        if self._max_rpm <= self._min_rpm:
            self._min_rpm = max(0, max_rpm_value - 1.0 / 60.0)

    def set_min_feed(
        self,
        min_feed: Union[int, float, FreeCAD.Units.Quantity],
        unit: Optional[str] = None,
    ) -> None:
        """Sets the minimum feed rate of the machine."""
        unit = unit or "mm/min"
        if unit in self.UNIT_CONVERSIONS:
            min_feed_value = min_feed * self.UNIT_CONVERSIONS[unit]
        else:
            min_feed_value = FreeCAD.Units.Quantity(min_feed, unit).getValueAs("mm/min").Value
        self._min_feed = min_feed_value
        if self._min_feed < 0:
            raise AttributeError("Min feed cannot be negative")
        if self._min_feed >= self._max_feed:
            self._max_feed = min_feed_value + 1.0

    def set_max_feed(
        self,
        max_feed: Union[int, float, FreeCAD.Units.Quantity],
        unit: Optional[str] = None,
    ) -> None:
        """Sets the maximum feed rate of the machine."""
        unit = unit or "mm/min"
        if unit in self.UNIT_CONVERSIONS:
            max_feed_value = max_feed * self.UNIT_CONVERSIONS[unit]
        else:
            max_feed_value = FreeCAD.Units.Quantity(max_feed, unit).getValueAs("mm/min").Value
        self._max_feed = max_feed_value
        if self._max_feed <= 0:
            raise AttributeError("Max feed must be positive")
        if self._max_feed <= self._min_feed:
            self._min_feed = max(0, max_feed_value - 1.0)

    def set_peak_torque_rpm(
        self, peak_torque_rpm: Union[int, float, FreeCAD.Units.Quantity]
    ) -> None:
        """Sets the peak torque RPM of the machine."""
        if isinstance(peak_torque_rpm, FreeCAD.Units.Quantity):
            try:
                peak_torque_rpm_value = peak_torque_rpm.getValueAs("1/s").Value
            except (Base.ParserError, ValueError):
                peak_torque_rpm_value = peak_torque_rpm.Value * self.UNIT_CONVERSIONS["rpm"]
        else:
            peak_torque_rpm_value = peak_torque_rpm * self.UNIT_CONVERSIONS["rpm"]
        self._peak_torque_rpm = peak_torque_rpm_value
        if self._peak_torque_rpm < 0:
            raise AttributeError("Peak torque RPM cannot be negative")

    def set_max_torque(
        self,
        max_torque: Union[int, float, FreeCAD.Units.Quantity],
        unit: Optional[str] = None,
    ) -> None:
        """Sets the maximum torque of the machine."""
        unit = unit or "Nm"
        if unit in self.UNIT_CONVERSIONS:
            max_torque_value = max_torque * self.UNIT_CONVERSIONS[unit]
        else:
            max_torque_value = FreeCAD.Units.Quantity(max_torque, unit).getValueAs("Nm").Value
        self._max_torque = max_torque_value
        if self._max_torque <= 0:
            raise AttributeError("Max torque must be positive")

    def dump(self, do_print: bool = True) -> Optional[str]:
        """
        Dumps machine information to console or returns it as a string.

        Args:
            do_print: If True, prints the information to the console.
                      If False, returns the information as a string.

        Returns:
            A formatted string containing machine information if do_print is
            False, otherwise None.
        """
        min_rpm_value = self._min_rpm * 60
        max_rpm_value = self._max_rpm * 60
        peak_torque_rpm_value = self._peak_torque_rpm * 60

        output = ""
        output += f"Machine {self.label}:\n"
        output += f"  Max power: {self._max_power:.2f} W\n"
        output += f"  RPM: {min_rpm_value:.2f} RPM - {max_rpm_value:.2f} RPM\n"
        output += f"  Feed: {self.min_feed.UserString} - " f"{self.max_feed.UserString}\n"
        output += (
            f"  Peak torque: {self._max_torque:.2f} Nm at " f"{peak_torque_rpm_value:.2f} RPM\n"
        )
        output += f"  Max_torque: {self._max_torque} Nm\n"

        if do_print:
            print(output)
        return output
