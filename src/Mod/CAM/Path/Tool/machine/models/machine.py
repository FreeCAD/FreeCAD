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
from typing import Optional, List
import FreeCAD
import Path
from ...assets import Asset
from ...spindle import Spindle


class Machine(Asset, ABC):
    """Represents a machine (e.g. a 3 axis CNC or a lathe)."""

    asset_type = "machine"
    _property_object_name = "Attributes"
    _property_object_group = "Machine"

    def __init__(
        self,
        label: str,
        max_feed: FreeCAD.Units.Quantity,
        post_processor: str = "generic",
        post_processor_args: str = "",
        id: Optional[str] = None,
    ) -> None:
        """
        Initializes a Machine object.

        Args:
            label: Machine label.
            max_feed: Maximum feed rate (e.g., '2000 mm/min').
            post_processor: Name of the FreeCAD post processor.
            post_processor_args: Arguments for the post processor.
            id: Unique identifier (optional).
        """
        super().__init__()
        self._id = id or str(uuid.uuid1())
        self._label = label
        self._spindles = []

        # Set property values
        self._max_feed = max_feed
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
    def max_feed(self) -> FreeCAD.Units.Quantity:
        """Maximum feed rate of the machine."""
        return self._max_feed

    @max_feed.setter
    def max_feed(self, value: FreeCAD.Units.Quantity):
        self._max_feed = value

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
        if self.max_feed.Value <= 0:
            raise AttributeError("Max feed rate must be positive")
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
        output += f"  Max Feed Rate: {self.max_feed.UserString}\n"
        output += f"  Post Processor: {self._post_processor or 'None'}\n"
        output += f"  Post Processor Args: {self._post_processor_args or 'None'}\n"
        for spindle in self.spindles:
            output += spindle.dump(do_print=False)
        if do_print:
            print(output)
        return output
