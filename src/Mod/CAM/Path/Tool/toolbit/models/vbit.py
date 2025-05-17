# -*- coding: utf-8 -*-
import FreeCAD
import Path
from ...shape import ToolBitShapeVBit
from ..mixins import RotaryToolBitMixin, ChiploadMixin
from .base import ToolBit


class ToolBitVBit(ToolBit, ChiploadMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeVBit

    def __init__(self, shape: ToolBitShapeVBit, id: str | None = None):
        Path.Log.track(f"ToolBitVBit __init__ called with shape: {shape}, id: {id}")
        super().__init__(shape, id=id)

    @property
    def summary(self) -> str:
        diameter = self.get_property("Diameter").UserString
        cutting_edge_angle = self.get_property("CuttingEdgeAngle").UserString
        flutes = self.get_property("Flutes")

        return FreeCAD.Qt.translate(
            "CAM",
            f"{diameter} {cutting_edge_angle} v-bit, {flutes}-flute"
        )
