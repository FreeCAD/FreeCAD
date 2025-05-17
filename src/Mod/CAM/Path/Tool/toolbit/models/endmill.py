# -*- coding: utf-8 -*-
import FreeCAD
import Path
from ...shape import ToolBitShapeEndmill
from ..mixins import RotaryToolBitMixin, ChiploadMixin
from .base import ToolBit


class ToolBitEndmill(ToolBit, ChiploadMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeEndmill

    def __init__(self, shape: ToolBitShapeEndmill, id: str | None = None):
        Path.Log.track(f"ToolBitEndmill __init__ called with shape: {shape}, id: {id}")
        super().__init__(shape, id=id)

    @property
    def summary(self) -> str:
        diameter = self.get_property_str("Diameter", "?")
        flutes = self.get_property("Flutes")
        cutting_edge_height = self.get_property_str("CuttingEdgeHeight", "?")

        return FreeCAD.Qt.translate(
            "CAM",
            f"{diameter} {flutes}-flute endmill, {cutting_edge_height} cutting edge"
        )
