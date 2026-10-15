# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Billy Huddleston <billy@ivdc.com>
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
from ...shape import ToolBitShapeLollipop
from ..mixins import RotaryToolBitMixin, CuttingToolMixin
from .base import ToolBit


class ToolBitLollipop(ToolBit, CuttingToolMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeLollipop

    def __init__(
        self, shape: ToolBitShapeLollipop, id: str | None = None, attrs: Optional[Mapping] = None
    ):
        Path.Log.track(f"ToolBitLollipop __init__ called with shape: {shape}, id: {id}")
        super().__init__(shape, id=id, attrs=attrs)
        self._init_cutting_properties(self.obj)

    @property
    def summary(self) -> str:
        diameter = self.get_property_str("Diameter", "?", precision=3)
        neck_diameter = self.get_property_str("NeckDiameter", "?", precision=3)
        flutes = self.get_property("Flutes")

        return FreeCAD.Qt.translate(
            "CAM", f"{diameter} lollipop on a {neck_diameter} neck, {flutes}-flute"
        )
