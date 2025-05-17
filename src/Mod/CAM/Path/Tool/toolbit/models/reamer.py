# -*- coding: utf-8 -*-
import FreeCAD
import Path
from ...shape import ToolBitShapeReamer
from ..mixins import RotaryToolBitMixin, ChiploadMixin
from .base import ToolBit


class ToolBitReamer(ToolBit, ChiploadMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeReamer

    def __init__(self, shape: ToolBitShapeReamer, id: str | None = None):
        Path.Log.track(f"ToolBitReamer __init__ called with shape: {shape}, id: {id}")
        super().__init__(shape, id=id)

    @property
    def summary(self) -> str:
        diameter = self.get_property("Diameter").UserString
        cutting_edge_height = self.get_property("CuttingEdgeHeight").UserString

        return FreeCAD.Qt.translate(
            "CAM",
            f"{diameter} reamer, {cutting_edge_height} cutting edge"
        )
