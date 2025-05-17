# -*- coding: utf-8 -*-
import FreeCAD
import Path
from ...shape import ToolBitShapeTap
from ..mixins import RotaryToolBitMixin, ChiploadMixin
from .base import ToolBit


class ToolBitTap(ToolBit, ChiploadMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeTap

    def __init__(self, shape: ToolBitShapeTap, id: str | None = None):
        Path.Log.track(f"ToolBitTap __init__ called with shape: {shape}, id: {id}")
        super().__init__(shape, id=id)

    @property
    def summary(self) -> str:
        diameter = self.get_property_str("Diameter", "?")
        flutes = self.get_property("Flutes")
        cutting_edge_length = self.get_property_str("CuttingEdgeLength", "?")

        return FreeCAD.Qt.translate(
            "CAM",
            f"{diameter} tap, {flutes}-flute, {cutting_edge_length} cutting edge"
        )
