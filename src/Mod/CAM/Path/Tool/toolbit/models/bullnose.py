# -*- coding: utf-8 -*-
import FreeCAD
import Path
from ...shape import ToolBitShapeBullnose
from ..mixins import RotaryToolBitMixin, ChiploadMixin
from .base import ToolBit


class ToolBitBullnose(ToolBit, ChiploadMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeBullnose

    def __init__(self, tool_bit_shape: ToolBitShapeBullnose, id: str | None = None):
        Path.Log.track(f"ToolBitBullnose __init__ called with id: {id}")
        super().__init__(tool_bit_shape, id=id)

    @property
    def summary(self) -> str:
        diameter = self.get_property_str("Diameter", "?")
        flutes = self.get_property("Flutes")
        cutting_edge_height = self.get_property_str("CuttingEdgeHeight", "?")
        flat_radius = self.get_property_str("FlatRadius", "?")

        return FreeCAD.Qt.translate(
            "CAM",
            f"{diameter} {flutes}-flute bullnose, {cutting_edge_height} cutting edge, {flat_radius} flat radius"
        )
