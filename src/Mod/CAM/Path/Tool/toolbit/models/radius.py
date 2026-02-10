# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

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
from ...shape import ToolBitShapeRadius
from ..mixins import RotaryToolBitMixin, CuttingToolMixin
from .base import ToolBit


class ToolBitRadius(ToolBit, CuttingToolMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeRadius

    def __init__(self, shape: ToolBitShapeRadius, id: str | None = None):
        Path.Log.track(f"ToolBitRadius __init__ called with shape: {shape}, id: {id}")
        super().__init__(shape, id=id)
        CuttingToolMixin.__init__(self, self.obj)

    @property
    def summary(self) -> str:
        radius = self.get_property_str("CuttingRadius", "?", precision=3)
        flutes = self.get_property("Flutes")
        diameter = self.get_property_str("ShankDiameter", "?", precision=3)

        return FreeCAD.Qt.translate(
            "CAM", f"R{radius} radius mill, {diameter} shank, {flutes}-flute"
        )
