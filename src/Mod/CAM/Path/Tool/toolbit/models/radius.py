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
from ...shape import ToolBitShapeRadius
from ..mixins import RotaryToolBitMixin, CuttingToolMixin
from .base import ToolBit


class ToolBitRadius(ToolBit, CuttingToolMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeRadius

    def __init__(
        self, shape: ToolBitShapeRadius, id: str | None = None, attrs: Optional[Mapping] = None
    ):
        Path.Log.track(f"ToolBitRadius __init__ called with shape: {shape}, id: {id}")
        super().__init__(shape, id=id, attrs=attrs)
        self._init_cutting_properties(self.obj)

    @property
    def summary(self) -> str:
        radius = self.get_property_str("CuttingRadius", "?", precision=3)
        flutes = self.get_property("Flutes")
        diameter = self.get_property_str("ShankDiameter", "?", precision=3)

        return FreeCAD.Qt.translate(
            "CAM", f"R{radius} radius mill, {diameter} shank, {flutes}-flute"
        )
