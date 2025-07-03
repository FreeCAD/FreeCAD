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
import FreeCAD
from abc import ABC
from typing import Dict, List, Optional
from .component import MachineComponent, AttributeConfig


translate = FreeCAD.Qt.translate


class Axis(MachineComponent, ABC):
    """
    Abstract base class for all machine axes.
    """

    pass


class LinearAxis(Axis):
    """
    Represents a linear axis of a machine.
    """

    def __init__(
        self,
        name: str,
        label: Optional[str] = None,
        max_feed: FreeCAD.Units.Quantity = FreeCAD.Units.Quantity("1000 mm/min"),
        icon: Optional[str] = None,
    ):
        super().__init__(name, label=label, icon=icon)
        self.max_feed = max_feed

    @property
    def type(self):
        return translate("CAM", "Linear Axis")

    def validate(self) -> None:
        """Validate parameters."""
        if self.max_feed.Value <= 0:
            raise AttributeError("Linear axis feed rate must be positive")

    def _dump_self(self, indent_str="") -> str:
        output = f"{indent_str}LinearAxis: {self.name}\n"
        output += f"{indent_str}  Max Feed: {self.max_feed.UserString}\n"
        return output

    def to_dict(self) -> Dict:
        data = super().to_dict()
        data.update(
            {
                "max_feed": self.max_feed.UserString,
            }
        )
        return data

    @classmethod
    def _from_dict_self(cls, data: Dict) -> "LinearAxis":
        return cls(
            name=data["name"],
            icon=data.get("icon"),
            max_feed=FreeCAD.Units.Quantity(data["max_feed"]),
        )

    def get_attribute_configs(self) -> List[AttributeConfig]:
        return super().get_attribute_configs() + [
            AttributeConfig(
                name="max_feed",
                label="Max Feed",
                property_type="App::PropertyVelocity",
                min_value=0.0,
            )
        ]


class AngularAxis(Axis):
    """
    Represents an angular axis of a machine.
    """

    def __init__(
        self,
        name: str,
        label: Optional[str] = None,
        angular_rigidity: FreeCAD.Units.Quantity = FreeCAD.Units.Quantity("0.5 °"),
        rigidity_x: FreeCAD.Units.Quantity = FreeCAD.Units.Quantity("0.001 mm/N"),
        rigidity_y: FreeCAD.Units.Quantity = FreeCAD.Units.Quantity("0.001 mm/N"),
        icon: Optional[str] = None,
    ):
        """
        Rigidity is specified as "deg/Newton applied to the spindle nose" and
        specifies the *worst-case rigidity at the child-side end of the axis*.
        It should be determined by measuring the deflection of the spindle in
        every direction and choosing the highest value.
        FreeCAD quantities do not support deg/N, so we store it as deg internally.
        """
        super().__init__(name, label=label, icon=icon)
        self.angular_rigidity = angular_rigidity
        self.rigidity_x = rigidity_x
        self.rigidity_y = rigidity_y

    @property
    def type(self):
        return translate("CAM", "Angular Axis")

    def validate(self) -> None:
        """Validate parameters."""
        if self.angular_rigidity.Value < 0:
            raise AttributeError("Angular axis rigidity cannot be negative")
        if self.rigidity_x.Value < 0:
            raise AttributeError("Rigidity X cannot be negative")
        if self.rigidity_y.Value < 0:
            raise AttributeError("Rigidity Y cannot be negative")

    def _dump_self(self, indent_str="") -> str:
        output = f"{indent_str}AngularAxis: {self.name}\n"
        output += f"{indent_str}  Angular Rigidity: {self.angular_rigidity.UserString}/N\n"
        output += f"{indent_str}  Rigidity X: {self.rigidity_x.getValueAs('mm/N')} mm/N\n"
        output += f"{indent_str}  Rigidity Y: {self.rigidity_y.getValueAs('mm/N')} mm/N\n"
        return output

    def to_dict(self) -> Dict:
        data = super().to_dict()
        data.update(
            {
                "angular_rigidity": f"{self.angular_rigidity.UserString}/N",
                "rigidity_x": f"{self.rigidity_x.getValueAs('mm/N')} mm/N",
                "rigidity_y": f"{self.rigidity_y.getValueAs('mm/N')} mm/N",
            }
        )
        return data

    @classmethod
    def _from_dict_self(cls, data: Dict) -> "AngularAxis":
        return cls(
            name=data["name"],
            icon=data.get("icon"),
            angular_rigidity=FreeCAD.Units.Quantity(data["angular_rigidity"].strip("/N")),
            rigidity_x=FreeCAD.Units.Quantity(data["rigidity_x"]),
            rigidity_y=FreeCAD.Units.Quantity(data["rigidity_y"]),
        )

    def get_attribute_configs(self) -> List[AttributeConfig]:
        return super().get_attribute_configs() + [
            AttributeConfig(
                name="angular_rigidity",
                label=translate("CAM", "Angular rigidity"),
                property_type="App::PropertyAngularRigidity",
                min_value=0.0,
            ),
            AttributeConfig(
                name="rigidity_x",
                label=translate("CAM", "Rigidity X"),
                property_type="App::PropertyRigidity",
                min_value=0.0,
            ),
            AttributeConfig(
                name="rigidity_y",
                label=translate("CAM", "Rigidity Y"),
                property_type="App::PropertyRigidity",
                min_value=0.0,
            ),
        ]
