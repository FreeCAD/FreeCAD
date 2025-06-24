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
from abc import ABC
from typing import Dict, Optional, List
import FreeCAD
import Path
from ...assets import Asset
from ...spindle import Spindle


class Axis(ABC):
    def validate(self) -> None:
        """Validate parameters."""
        pass

    def dump(self, do_print: bool = True, indent: int = 0) -> str:
        """
        Dumps axis info to console or as a string.

        Args:
            do_print: If True, prints; if False, returns string.

        Returns:
            Formatted string if do_print is False.
        """
        raise NotImplementedError


class LinearAxis(Axis):
    def __init__(
        self,
        start: FreeCAD.Units.Quantity = FreeCAD.Units.Quantity("0 mm"),
        end: Optional[FreeCAD.Units.Quantity] = None,
        rigidity: FreeCAD.Units.Quantity = FreeCAD.Units.Quantity("0.1 mm"),
        max_feed: FreeCAD.Units.Quantity = FreeCAD.Units.Quantity("2000 mm/min"),
    ):
        """
        Rigidity is specified in mm/Newton. FreeCAD quantities do not support mm/N,
        so we store it as mm internally.
        """
        super().__init__()
        self.start = start
        self.end = end
        self.rigidity = rigidity
        self.max_feed = max_feed

    def validate(self) -> None:
        super().validate()
        if self.start is None:
            raise AttributeError("Linear axis start cannot be None")
        if self.start.Value < 0:
            raise AttributeError("Linear axis start cannot be negative")
        if self.end and self.end.Value <= self.start.Value:
            raise AttributeError("Linear axis end must be larger than axis start")
        if self.rigidity.Value < 0:
            raise AttributeError("Linear axis rigidity cannot be negative")
        if self.max_feed.Value <= 0:
            raise AttributeError("Linear axis feed rate must be positive")

    def dump(self, do_print: bool = True, indent: int = 0) -> str:
        prefix = "  " * indent
        output = ""
        if self.start is not None:
            output += f"{prefix}Start={self.start.UserString}\n"
        if self.end is not None:
            output += f"{prefix}End={self.end.UserString}\n"
        output += f"{prefix}Rigidity={self.rigidity.UserString}/N\n"
        output += f"{prefix}Feed Rate={self.max_feed.UserString}\n"
        if do_print:
            print(output)
        return output


class AngularAxis(Axis):
    def __init__(
        self,
        rigidity_x: FreeCAD.Units.Quantity = FreeCAD.Units.Quantity("0.5 °"),
        rigidity_y: FreeCAD.Units.Quantity = FreeCAD.Units.Quantity("0.5 °"),
    ):
        """
        Rigidity is specified as "deg/Newton applied to the spindle nose".
        FreeCAD quantities do not support deg/N, so we store it as deg internally.
        """
        super().__init__()
        self.rigidity_x = rigidity_x
        self.rigidity_y = rigidity_y

    def validate(self) -> None:
        """Validate parameters."""
        super().validate()
        if self.rigidity_x.Value < 0:
            raise AttributeError("rigidity_x cannot be negative")
        if self.rigidity_y.Value < 0:
            raise AttributeError("rigidity_y cannot be negative")

    def dump(self, do_print: bool = True, indent: int = 0) -> str:
        prefix = "  " * indent
        output = ""
        output += f"{prefix}Rigidity X={self.rigidity_x.UserString}/N\n"
        output += f"{prefix}Rigidity Y={self.rigidity_y.UserString}/N\n"
        if do_print:
            print(output)
        return output


class Machine(Asset, ABC):
    """Represents a machine (e.g. a 3 axis CNC or a lathe)."""

    asset_type = "machine"
    _property_object_name = "Attributes"
    _property_object_group = "Machine"

    def __init__(
        self,
        label: str,
        axes: Dict[str, Axis],
        post_processor: str = "generic",
        post_processor_args: str = "",
        id: Optional[str] = None,
    ) -> None:
        """
        Initializes a Machine object.

        Args:
            label: Machine label.
            axes: E.g. {"x": Axis(0, 1000, 0.01)}
            post_processor: Name of the FreeCAD post processor.
            post_processor_args: Arguments for the post processor.
            id: Unique identifier (optional).
        """
        super().__init__()
        self._id = id or str(uuid.uuid1())
        self._label = label
        self._axes = axes
        self._spindles = []

        # Set property values
        self.post_processor = post_processor  # property setter below checks validity
        self._post_processor_args = post_processor_args

    def get_id(self):
        return self._id

    @staticmethod
    def get_type() -> str:
        """Returns a human readable machine type."""
        raise NotImplementedError

    @property
    def label(self) -> str:
        """Returns the machine's label."""
        return self._label

    @label.setter
    def label(self, value: str):
        self._label = value

    @property
    def summary(self) -> str:
        """
        Returns a summary of the machine parameters for display in the UI.

        Returns:
            Summary string.
        """
        return FreeCAD.Qt.translate(
            "CAM",
            f"{len(self._axes)}-axis {self.get_type()} with " f"{len(self._spindles)} spindles",
        )

    @property
    def axes(self) -> Dict[str, Axis]:
        """Returns the machine's axes."""
        return self._axes

    @axes.setter
    def axes(self, value: Dict[str, Axis]):
        self._axes = value

    def set_axis(self, name: str, value: Axis):
        self._axes[name] = value

    @property
    def spindles(self) -> List[Spindle]:
        """Gets a list of spindles of the machine."""
        return self._spindles

    @spindles.setter
    def spindles(self, value: List[Spindle]):
        self._spindles = value

    def add_spindle(self, spindle: Spindle) -> None:
        """Adds a spindle to the machine."""
        assert spindle is not None
        self._spindles.append(spindle)

    def remove_spindle(self, spindle: Spindle) -> None:
        """Removes a spindle from the machine."""
        self._spindles.remove(spindle)

    @property
    def post_processor(self) -> str:
        """Name of the FreeCAD post processor."""
        return self._post_processor

    @post_processor.setter
    def post_processor(self, value: str):
        available_post_processors = Path.Preferences.allAvailablePostProcessors()
        if value not in available_post_processors:
            raise AttributeError(f"Post processor '{value}' does not exist.")
        self._post_processor = value

    @property
    def post_processor_args(self) -> str:
        """Arguments for the post processor."""
        return self._post_processor_args

    @post_processor_args.setter
    def post_processor_args(self, value: str):
        self._post_processor_args = value

    def validate(self) -> None:
        """Validates machine parameters."""
        if not self.label:
            raise AttributeError("Machine name is required")
        for name, axis in self._axes.items():
            try:
                axis.validate()
            except AttributeError as e:
                raise AttributeError(f"Axis {name}: {e}")
        for spindle in self._spindles:
            spindle.validate()

    def dump(self, do_print: bool = True) -> str:
        """
        Dumps machine info to console or as a string.

        Args:
            do_print: If True, prints; if False, returns string.

        Returns:
            Formatted string if do_print is False.
        """
        output = f"Machine {self.label}:\n"
        for name, axis in sorted(self._axes.items()):
            output += f"  {name}-Axis:\n"
            output += axis.dump(do_print, 2)
        output += f"  Post Processor: {self._post_processor or 'None'}\n"
        output += f"  Post Processor Args: {self._post_processor_args or 'None'}\n"
        for spindle in self.spindles:
            output += spindle.dump(do_print=False)
        if do_print:
            print(output)
        return output
