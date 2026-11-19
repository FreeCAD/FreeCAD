# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2025 Samuel Abels
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

import FreeCAD
import Path
from typing import Optional, Mapping
from ...shape import ToolBitShapeProbe
from .base import ToolBit


class ToolBitProbe(ToolBit):
    SHAPE_CLASS = ToolBitShapeProbe

    def __init__(
        self, shape: ToolBitShapeProbe, id: str | None = None, attrs: Optional[Mapping] = None
    ):
        Path.Log.track(f"ToolBitProbe __init__ called with shape: {shape}, id: {id}")
        super().__init__(shape, id=id, attrs=attrs)
        self.obj.SpindleDirection = "None"
        self.obj.setEditorMode("SpindleDirection", 2)  # Read-only

    @property
    def summary(self) -> str:
        diameter = self.get_property_str("Diameter", "?", precision=3)
        length = self.get_property_str("Length", "?", precision=3)
        shaft_diameter = self.get_property_str("ShaftDiameter", "?", precision=3)

        return FreeCAD.Qt.translate(
            "CAM", f"{diameter} probe, {length} length, {shaft_diameter} shaft"
        )

    def can_rotate(self) -> bool:
        return False

    # Connor: Add getters and setters for Diameter and Length
    def get_diameter(self) -> FreeCAD.Units.Quantity:
        """
        Get the diameter of the rotary tool bit from the shape.
        """
        return self.obj.Diameter

    def set_diameter(self, diameter: FreeCAD.Units.Quantity):
        """
        Set the diameter of the rotary tool bit on the shape.
        """
        if not isinstance(diameter, FreeCAD.Units.Quantity):
            raise ValueError("Diameter must be a FreeCAD Units.Quantity")
        self.obj.Diameter = diameter

    def get_length(self) -> FreeCAD.Units.Quantity:
        """
        Get the length of the rotary tool bit from the shape.
        """
        return self.obj.Length

    def set_length(self, length: FreeCAD.Units.Quantity):
        """
        Set the length of the rotary tool bit on the shape.
        """
        if not isinstance(length, FreeCAD.Units.Quantity):
            raise ValueError("Length must be a FreeCAD Units.Quantity")
        self.obj.Length = length
