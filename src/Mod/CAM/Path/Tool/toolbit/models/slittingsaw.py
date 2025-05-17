# -*- coding: utf-8 -*-
import FreeCAD
import Path
from ...shape import ToolBitShapeSlittingSaw
from ..mixins import RotaryToolBitMixin, ChiploadMixin
from .base import ToolBit


class ToolBitSlittingSaw(ToolBit, ChiploadMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeSlittingSaw

    def __init__(self, shape: ToolBitShapeSlittingSaw, id: str | None = None):
        Path.Log.track(f"ToolBitSlittingSaw __init__ called with shape: {shape}, id: {id}")
        super().__init__(shape, id=id)

    @property
    def summary(self) -> str:
        diameter = self.get_property_str("Diameter", "?")
        blade_thickness = self.get_property_str("BladeThickness", "?")
        flutes = self.get_property("Flutes")

        return FreeCAD.Qt.translate(
            "CAM", f"{diameter} slitting saw, {blade_thickness} blade, {flutes}-flute"
        )
