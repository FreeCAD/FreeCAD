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
from typing import Optional, Tuple
import FreeCAD
from ...spindle import Spindle
from .machine import Machine


class Mill(Machine):
    """Represents a 3 axis milling machine."""

    _property_object_name = "Attributes"
    _property_object_group = "Mill"

    def __init__(
        self,
        label: str,
        x_extent: Tuple[FreeCAD.Units.Quantity, FreeCAD.Units.Quantity] = (
            FreeCAD.Units.Quantity("0 mm"),
            FreeCAD.Units.Quantity("1000 mm"),
        ),
        y_extent: Tuple[FreeCAD.Units.Quantity, FreeCAD.Units.Quantity] = (
            FreeCAD.Units.Quantity("0 mm"),
            FreeCAD.Units.Quantity("1000 mm"),
        ),
        z_extent: Tuple[FreeCAD.Units.Quantity, FreeCAD.Units.Quantity] = (
            FreeCAD.Units.Quantity("0 mm"),
            FreeCAD.Units.Quantity("1000 mm"),
        ),
        max_feed: FreeCAD.Units.Quantity = FreeCAD.Units.Quantity("2000 mm/min"),
        rigidity: Tuple[
            FreeCAD.Units.Quantity,
            FreeCAD.Units.Quantity,
            FreeCAD.Units.Quantity,
            FreeCAD.Units.Quantity,
        ] = (
            FreeCAD.Units.Quantity("0.1 mm/N"),
            FreeCAD.Units.Quantity("0.1 mm/N"),
            FreeCAD.Units.Quantity("0.1 mm/N"),
            FreeCAD.Units.Quantity("0.1 mm/rad"),
        ),
        post_processor: str = "generic",
        post_processor_args: str = "",
        id: Optional[str] = None,
    ) -> None:
        """
        Initializes a Mill object.

        Args:
            label: Machine label.
            spindle: Associated spindle object.
            x_extent: Tuple of (min, max) X-axis extents.
            y_extent: Tuple of (min, max) Y-axis extents.
            z_extent: Tuple of (min, max) Z-axis extents.
            max_feed: Maximum feed rate (e.g., '2000 mm/min').
            rigidity: Tuple of (x, y, z, rotational) stiffness (mm/N for x,y,z; mm/rad for rotational).
            post_processor: Name of the FreeCAD post processor.
            post_processor_args: Arguments for the post processor.
            id: Unique identifier (optional).
        """
        super().__init__(
            label, max_feed, post_processor, post_processor_args, id or str(uuid.uuid1())
        )

        self._rigidity_x = rigidity[0]
        self._rigidity_y = rigidity[1]
        self._rigidity_z = rigidity[2]
        self._rigidity_rotational = rigidity[3]
        self._x_min = x_extent[0]
        self._x_max = x_extent[1]
        self._y_min = y_extent[0]
        self._y_max = y_extent[1]
        self._z_min = z_extent[0]
        self._z_max = z_extent[1]

        self.add_spindle(Spindle(FreeCAD.Qt.translate("CAM", "Spindle 1")))

    @staticmethod
    def get_type() -> str:
        """Returns a human readable machine type."""
        return FreeCAD.Qt.translate("CAM", "Mill")

    @property
    def rigidity(
        self,
    ) -> Tuple[
        FreeCAD.Units.Quantity,
        FreeCAD.Units.Quantity,
        FreeCAD.Units.Quantity,
        FreeCAD.Units.Quantity,
    ]:
        """Tuple of (x, y, z, rotational) rigidity (mm/N for x,y,z; mm/rad for rotational)."""
        return (
            self._rigidity_x,
            self._rigidity_y,
            self._rigidity_z,
            self._rigidity_rotational,
        )

    @property
    def x_min(self):
        return self._x_min

    @x_min.setter
    def x_min(self, value: FreeCAD.Units.Quantity):
        self._x_min = value

    @property
    def x_max(self):
        return self._x_max

    @x_max.setter
    def x_max(self, value: FreeCAD.Units.Quantity):
        self._x_max = value

    @property
    def x_extent(self) -> Tuple[FreeCAD.Units.Quantity, FreeCAD.Units.Quantity]:
        """Tuple of (min, max) X-axis extents."""
        return self._x_min, self._x_max

    @property
    def y_min(self):
        return self._y_min

    @y_min.setter
    def y_min(self, value: FreeCAD.Units.Quantity):
        self._y_min = value

    @property
    def y_max(self):
        return self._y_max

    @y_max.setter
    def y_max(self, value: FreeCAD.Units.Quantity):
        self._y_max = value

    @property
    def y_extent(self) -> Tuple[FreeCAD.Units.Quantity, FreeCAD.Units.Quantity]:
        """Tuple of (min, max) Y-axis extents."""
        return self._y_min, self._y_max

    @property
    def z_min(self):
        return self._z_min

    @z_min.setter
    def z_min(self, value: FreeCAD.Units.Quantity):
        self._z_min = value

    @property
    def z_max(self):
        return self._z_max

    @z_max.setter
    def z_max(self, value: FreeCAD.Units.Quantity):
        self._z_max = value

    @property
    def z_extent(self) -> Tuple[FreeCAD.Units.Quantity, FreeCAD.Units.Quantity]:
        """Tuple of (min, max) Z-axis extents."""
        return self._z_min, self._z_max

    @property
    def rigidity_x(self):
        return self._rigidity_x

    @rigidity_x.setter
    def rigidity_x(self, value: FreeCAD.Units.Quantity):
        self._rigidity_x = value

    @property
    def rigidity_y(self):
        return self._rigidity_y

    @rigidity_y.setter
    def rigidity_y(self, value: FreeCAD.Units.Quantity):
        self._rigidity_y = value

    @property
    def rigidity_z(self):
        return self._rigidity_z

    @rigidity_z.setter
    def rigidity_z(self, value: FreeCAD.Units.Quantity):
        self._rigidity_z = value

    @property
    def rigidity_rotational(self):
        return self._rigidity_rotational

    @rigidity_rotational.setter
    def rigidity_rotational(self, value: FreeCAD.Units.Quantity):
        self._rigidity_rotational = value

    def validate(self) -> None:
        """Validates mill parameters."""
        super().validate()
        if any(r.Value < 0 for r in self.rigidity):
            raise AttributeError("Rigidity values cannot be negative")
        if self._x_min >= self._x_max:
            raise AttributeError("X-axis max must be greater than min")
        if self._y_min >= self._y_max:
            raise AttributeError("Y-axis max must be greater than min")
        if self._z_min >= self._z_max:
            raise AttributeError("Z-axis max must be greater than min")

    def dump(self, do_print: bool = True) -> str:
        """
        Dumps mill info to console or as a string.

        Args:
            do_print: If True, prints; if False, returns string.

        Returns:
            Formatted string if do_print is False.
        """
        output = f"Mill {self.label}:\n"
        output += f"  Max Feed Rate: {self.max_feed.UserString}\n"
        rigidity_x, rigidity_y, rigidity_z, rigidity_rot = self.rigidity
        output += "  Rigidity (deflection):\n"
        output += f"    X={rigidity_x.UserString}/N\n"
        output += f"    Y={rigidity_y.UserString}/N\n"
        output += f"    Z={rigidity_z.UserString}/N\n"
        output += f"    Rotational={rigidity_rot.UserString}/rad\n"
        output += "  Axis Extents:\n"
        output += f"    X: {self._x_min.UserString} - {self._x_max.UserString}\n"
        output += f"    Y: {self._y_min.UserString} - {self._y_max.UserString}\n"
        output += f"    Z: {self._z_min.UserString} - {self._z_max.UserString}\n"
        output += f"  Post Processor: {self._post_processor or 'None'}\n"
        output += f"  Post Processor Args: {self._post_processor_args or 'None'}\n"
        for spindle in self.spindles:
            output += spindle.dump(do_print=False)
        if do_print:
            print(output)
        return output
