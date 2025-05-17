# -*- coding: utf-8 -*-
import FreeCAD
import Path
from ...shape import ToolBitShapeBallend
from ..mixins import RotaryToolBitMixin, ChiploadMixin
from .base import ToolBit


class ToolBitBallend(ToolBit, ChiploadMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeBallend

    def __init__(self, shape: ToolBitShapeBallend, id: str | None = None):
        Path.Log.track(f"ToolBitBallend __init__ called with shape: {shape}, id: {id}")
        super().__init__(shape, id=id)

    @property
    def summary(self) -> str:
        diameter = self.get_property_str("Diameter", "?")
        flutes = self.get_property("Flutes")
        cutting_edge_height = self.get_property_str("CuttingEdgeHeight", "?")

        return FreeCAD.Qt.translate(
            "CAM", f"{diameter} {flutes}-flute ballend, {cutting_edge_height} cutting edge"
        )
