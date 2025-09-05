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
import FreeCAD
import Path
from ...shape import ToolBitShapeTap
from ..mixins import RotaryToolBitMixin, CuttingToolMixin
from .base import ToolBit


class ToolBitTap(ToolBit, CuttingToolMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeTap

    def __init__(self, shape: ToolBitShapeTap, id: str | None = None):
        Path.Log.track(f"ToolBitTap __init__ called with shape: {shape}, id: {id}")
        super().__init__(shape, id=id)
        CuttingToolMixin.__init__(self, self.obj)

    @property
    def summary(self) -> str:
        diameter = self.get_property_str("Diameter", "?")
        flutes = self.get_property("Flutes")
        cutting_edge_length = self.get_property_str("CuttingEdgeLength", "?")

        return FreeCAD.Qt.translate(
            "CAM", f"{diameter} tap, {flutes}-flute, {cutting_edge_length} cutting edge"
        )
