# -*- coding: utf-8 -*-
import FreeCAD
import Path
from ...shape import ToolBitShapeChamfer
from ..mixins import RotaryToolBitMixin, ChiploadMixin
from .base import ToolBit


class ToolBitChamfer(ToolBit, ChiploadMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeChamfer

    def __init__(self, shape: ToolBitShapeChamfer, id: str | None = None):
        Path.Log.track(f"ToolBitChamfer __init__ called with shape: {shape}, id: {id}")
        super().__init__(shape, id=id)

    @property
    def summary(self) -> str:
        diameter = self.get_property("Diameter").UserString
        flutes = self.get_property("Flutes")
        cutting_edge_angle = self.get_property("CuttingEdgeAngle").UserString

        return FreeCAD.Qt.translate(
            "CAM",
            f"{diameter} {cutting_edge_angle} chamfer bit, {flutes}-flute"
        )
