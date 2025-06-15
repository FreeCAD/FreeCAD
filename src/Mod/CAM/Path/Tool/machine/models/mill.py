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
from typing import Dict, Optional, cast
import FreeCAD
from ...spindle import Spindle
from .machine import Machine, Axis, AngularAxis, LinearAxis


class Mill(Machine):
    """Represents a 3 axis milling machine."""

    _property_object_name = "Attributes"
    _property_object_group = "Mill"

    def __init__(
        self,
        label: str,
        axes: Optional[Dict[str, Axis]] = None,
        max_feed: FreeCAD.Units.Quantity = FreeCAD.Units.Quantity("2000 mm/min"),
        post_processor: str = "generic",
        post_processor_args: str = "",
        id: Optional[str] = None,
    ) -> None:
        """
        Initializes a Mill object.

        Args:
            label: Machine label.
            axes: E.g. {"x": Axis(), "y": Axis(), "z": Axis()}
            max_feed: Maximum feed rate (e.g., '2000 mm/min').
            post_processor: Name of the FreeCAD post processor.
            post_processor_args: Arguments for the post processor.
            id: Unique identifier (optional).
        """
        if axes is None:
            axes = {
                "X": LinearAxis(),
                "Y": LinearAxis(),
                "Z": LinearAxis(),
                "spindle": AngularAxis(),
            }

        super().__init__(
            label=label,
            axes=axes,
            max_feed=max_feed,
            post_processor=post_processor,
            post_processor_args=post_processor_args,
            id=id or str(uuid.uuid1()),
        )

        self.add_spindle(Spindle(FreeCAD.Qt.translate("CAM", "Spindle 1")))

    @staticmethod
    def get_type() -> str:
        """Returns a human readable machine type."""
        return FreeCAD.Qt.translate("CAM", "Mill")

    @property
    def x_axis(self) -> LinearAxis:
        return cast(LinearAxis, self._axes["X"])

    @property
    def y_axis(self) -> LinearAxis:
        return cast(LinearAxis, self._axes["Y"])

    @property
    def z_axis(self) -> LinearAxis:
        return cast(LinearAxis, self._axes["Z"])

    @property
    def spindle_axis(self) -> AngularAxis:
        return cast(AngularAxis, self._axes["spindle"])

    def validate(self) -> None:
        """Validates mill parameters."""
        super().validate()
        if "X" not in self._axes:
            raise AttributeError("Mill must have an X axis")
        if "Y" not in self._axes:
            raise AttributeError("Mill must have an Y axis")
        if "Z" not in self._axes:
            raise AttributeError("Mill must have an Z axis")
        if "spindle" not in self._axes:
            raise AttributeError("Mill must have a spindle axis")

    def dump(self, do_print: bool = True) -> str:
        """
        Dumps mill info to console or as a string.

        Args:
            do_print: If True, prints; if False, returns string.

        Returns:
            Formatted string if do_print is False.
        """
        output = f"Mill {self.label}:\n"
        for name, axis in sorted(self.axes.items()):
            output += f"  {name}-Axis:\n"
            output += axis.dump(do_print=False, indent=2)
        output += f"  Max Feed Rate: {self.max_feed.UserString}\n"
        output += f"  Post Processor: {self._post_processor or 'None'}\n"
        output += f"  Post Processor Args: {self._post_processor_args or 'None'}\n"
        for spindle in self.spindles:
            output += spindle.dump(do_print=False)
        if do_print:
            print(output)
        return output
