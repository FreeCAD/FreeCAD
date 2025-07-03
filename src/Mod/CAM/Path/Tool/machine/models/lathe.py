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
from typing import Optional, cast, List
import FreeCAD
from .machine import Machine, MachineFeatureFlags
from .component import MachineComponent
from .axis import LinearAxis, AngularAxis
from .spindle import Spindle


class Lathe(Machine):
    """Represents a 2-axis lathe with rigidity and axis extents attributes."""

    _property_object_name = "Attributes"
    _property_object_group = "Lathe"

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
        Initializes a Lathe object.

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
        z_axis = LinearAxis("Z")
        a_axis = AngularAxis("A")
        main_spindle = Spindle("MainSpindle", FreeCAD.Qt.translate("CAM", "Main spindle"))
        tool_holder = MachineComponent("ToolHolder")

        self.add(main_spindle)
        main_spindle.add(a_axis)
        self.add(z_axis)
        z_axis.add(x_axis)
        x_axis.add(tool_holder)

    @staticmethod
    def get_type() -> str:
        """Returns a human readable machine type."""
        return FreeCAD.Qt.translate("CAM", "Lathe")

    @property
    def x_axis(self) -> LinearAxis:
        return cast(LinearAxis, self.find_child_by_name("X"))

    @property
    def z_axis(self) -> LinearAxis:
        return cast(LinearAxis, self.get_child_by_name("Z"))

    @property
    def a_axis(self) -> AngularAxis:
        return cast(AngularAxis, self.find_child_by_name("A"))

    def validate(self) -> None:
        """Validates lathe parameters."""
        super().validate()
        if not self.find_child_by_name("X"):
            raise AttributeError("Lathe must have an X axis")
        if not self.find_child_by_name("Z"):
            raise AttributeError("Lathe must have an Z axis")
        if not self.find_child_by_name("A"):
            raise AttributeError("Lathe must have an A axis")
        if not self.find_children_by_type(Spindle):
            raise AttributeError("Lathe must have a spindle")
