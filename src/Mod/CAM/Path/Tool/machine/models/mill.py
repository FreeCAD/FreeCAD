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
from typing import List, Optional, cast
import FreeCAD
from .machine import Machine, MachineFeatureFlags
from .axis import LinearAxis, AngularAxis
from .spindle import Spindle


class Mill(Machine):
    """Represents a 3 axis milling machine."""

    _property_object_name = "Attributes"
    _property_object_group = "Mill"

    def __init__(
        self,
        name: str,
        label: Optional[str] = None,
        post_processor: str = "generic",
        post_processor_args: str = "",
        icon: Optional[str] = None,
        feature_flags: Optional[List[MachineFeatureFlags]] = None,
        id: Optional[str] = None,
    ) -> None:
        """
        Initializes a Mill object.

        Args:
            name: Machine name.
            label: Machine label (optional).
            post_processor: Name of the FreeCAD post processor.
            post_processor_args: Arguments for the post processor.
            id: Unique identifier (optional).
        """
        super().__init__(
            name=name,
            label=label,
            post_processor=post_processor,
            post_processor_args=post_processor_args,
            icon=icon,
            feature_flags=feature_flags,
            id=id,
        )

        x_axis = LinearAxis("X")
        y_axis = LinearAxis("Y")
        z_axis = LinearAxis("Z")
        spindle_axis = AngularAxis("A")
        main_spindle = Spindle("MainSpindle", FreeCAD.Qt.translate("CAM", "Spindle 1"))

        self.add(x_axis)
        x_axis.add(y_axis)
        y_axis.add(z_axis)
        z_axis.add(spindle_axis)
        spindle_axis.add(main_spindle)

    @staticmethod
    def get_type() -> str:
        """Returns a human readable machine type."""
        return FreeCAD.Qt.translate("CAM", "Mill")

    @property
    def x_axis(self) -> LinearAxis:
        return cast(LinearAxis, self.find_child_by_name("X"))

    @property
    def y_axis(self) -> LinearAxis:
        return cast(LinearAxis, self.find_child_by_name("Y"))

    @property
    def z_axis(self) -> LinearAxis:
        return cast(LinearAxis, self.find_child_by_name("Z"))

    @property
    def spindle_axis(self) -> AngularAxis:
        return cast(AngularAxis, self.find_child_by_name("A"))

    def validate(self) -> None:
        """Validates mill parameters."""
        super().validate()
        if not self.find_child_by_name("X"):
            raise AttributeError("Mill must have an X axis")
        if not self.find_child_by_name("Y"):
            raise AttributeError("Mill must have an Y axis")
        if not self.find_child_by_name("Z"):
            raise AttributeError("Mill must have an Z axis")
        if not self.find_child_by_name("A"):
            raise AttributeError("Mill must have a spindle axis")
