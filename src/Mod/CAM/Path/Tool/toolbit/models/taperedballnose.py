# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   © 2026 Billy Huddleston <billy@ivdc.com>                                   #
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
from ...shape import ToolBitShapeTaperedBallNose
from ..mixins import RotaryToolBitMixin, CuttingToolMixin
from .base import ToolBit


class ToolBitTaperedBallNose(ToolBit, CuttingToolMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeTaperedBallNose

    def __init__(self, shape: ToolBitShapeTaperedBallNose, id: str | None = None):
        Path.Log.track(f"ToolBitTaperedBallNose __init__ called with shape: {shape}, id: {id}")
        super().__init__(shape, id=id)
        CuttingToolMixin.__init__(self, self.obj)

    @property
    def summary(self) -> str:
        diameter = self.get_property_str("Diameter", "?", precision=3)
        flutes = self.get_property("Flutes")
        cutting_edge_height = self.get_property_str("CuttingEdgeHeight", "?", precision=3)
        taper_angle = self.get_property_str("TaperAngle", "?", precision=1)

        return FreeCAD.Qt.translate(
            "CAM",
            f"{diameter} tip, {taper_angle} taper, {flutes}-flute tapered ball nose, {cutting_edge_height} cutting edge",
        )
