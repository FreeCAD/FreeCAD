# -*- coding: utf-8 -*-
import FreeCAD
import Path
from ...shape import ToolBitShapeThreadMill
from ..mixins import RotaryToolBitMixin, ChiploadMixin
from .base import ToolBit


class ToolBitThreadMill(ToolBit, ChiploadMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeThreadMill

    def __init__(self, shape: ToolBitShapeThreadMill, id: str | None = None):
        Path.Log.track(f"ToolBitThreadMill __init__ called with shape: {shape}, id: {id}")
        super().__init__(shape, id=id)

    @property
    def summary(self) -> str:
        diameter = self.get_property_str("Diameter", "?")
        flutes = self.get_property("Flutes")
        cutting_angle = self.get_property_str("cuttingAngle", "?")

        return FreeCAD.Qt.translate(
            "CAM", f"{diameter} thread mill, {flutes}-flute, {cutting_angle} cutting angle"
        )
