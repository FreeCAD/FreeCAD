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
from ...shape import ToolBitShapeTap
from ..mixins import RotaryToolBitMixin, CuttingToolMixin
from .base import ToolBit
from ..util import is_imperial_pitch


class ToolBitTap(ToolBit, CuttingToolMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeTap

    def __init__(
        self, shape: ToolBitShapeTap, id: str | None = None, attrs: Optional[Mapping] = None
    ):
        Path.Log.track(f"ToolBitTap __init__ called with shape: {shape}, id: {id}")
        super().__init__(shape, id=id, attrs=attrs)
        self._init_cutting_properties(self.obj)

    @property
    def summary(self) -> str:
        diameter = self.get_property_str("Diameter", "?", precision=3)
        flutes = self.get_property("Flutes")
        cutting_edge_length = self.get_property_str("CuttingEdgeLength", "?", precision=3)
        pitch_raw = self.get_property("Pitch")

        spindle_direction = self.get_property_str("SpindleDirection", "Forward")
        if spindle_direction == "Forward":
            rotation = "Right Hand"
        elif spindle_direction == "Reverse":
            rotation = "Left Hand"
        else:
            rotation = spindle_direction

        if isinstance(pitch_raw, FreeCAD.Units.Quantity):
            pitch_mm = pitch_raw.getValueAs("mm")
        else:
            pitch_mm = FreeCAD.Units.Quantity(str(pitch_raw)).getValueAs("mm")

        if pitch_raw:
            try:
                if is_imperial_pitch(pitch_raw):
                    tpi = round(25.4 / pitch_mm, 2)
                    pitch = f"{int(tpi) if tpi == int(tpi) else tpi} TPI"
                else:
                    pitch = f"{pitch_mm} mm"
            except Exception:
                pitch = str(pitch_raw)
        else:
            pitch = "?"

        return FreeCAD.Qt.translate(
            "CAM",
            f"{diameter} {pitch} {rotation} tap, {flutes}-flute, {cutting_edge_length} cutting edge",
        )
