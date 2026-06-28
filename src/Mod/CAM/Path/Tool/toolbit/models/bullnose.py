# SPDX-License-Identifier: LGPL-2.1-or-later

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
import Path
from typing import Optional, Mapping
from ...shape import ToolBitShapeBullnose
from ..mixins import RotaryToolBitMixin, CuttingToolMixin
from .base import ToolBit


class ToolBitBullnose(ToolBit, CuttingToolMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeBullnose

    def __init__(
        self,
        tool_bit_shape: ToolBitShapeBullnose,
        id: str | None = None,
        attrs: Optional[Mapping] = None,
    ):
        Path.Log.track(f"ToolBitBullnose __init__ called with id: {id}")
        super().__init__(tool_bit_shape, id=id, attrs=attrs)
        self._init_cutting_properties(self.obj)

    @property
    def summary(self) -> str:
        diameter = self.get_property_str("Diameter", "?", precision=3)
        flutes = self.get_property("Flutes")
        cutting_edge_height = self.get_property_str("CuttingEdgeHeight", "?", precision=3)
        # flat_radius = self.get_property_str("FlatRadius", "?", precision=3)
        corner_radius = self.get_property_str("CornerRadius", "?", precision=3)

        return FreeCAD.Qt.translate(
            "CAM",
            f"{diameter} {flutes}-flute bullnose, {cutting_edge_height} cutting edge, {corner_radius} corner radius",
        )
