# -*- coding: utf-8 -*-
import FreeCAD
import Path
from ...shape import ToolBitShapeDovetail
from ..mixins import RotaryToolBitMixin, ChiploadMixin
from .base import ToolBit


class ToolBitDovetail(ToolBit, ChiploadMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeDovetail

    def __init__(self, shape: ToolBitShapeDovetail, id: str | None = None):
        Path.Log.track(f"ToolBitDovetail __init__ called with shape: {shape}, id: {id}")
        super().__init__(shape, id=id)

    @property
    def summary(self) -> str:
        diameter = self.get_property_str("Diameter", "?")
        cutting_edge_angle = self.get_property_str("CuttingEdgeAngle", "?")
        flutes = self.get_property("Flutes")

        return FreeCAD.Qt.translate(
            "CAM", f"{diameter} {cutting_edge_angle} dovetail bit, {flutes}-flute"
        )
