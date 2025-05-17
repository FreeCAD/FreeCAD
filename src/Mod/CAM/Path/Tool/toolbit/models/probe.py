# -*- coding: utf-8 -*-
import FreeCAD
import Path
from ...shape import ToolBitShapeProbe
from .base import ToolBit


class ToolBitProbe(ToolBit):
    SHAPE_CLASS = ToolBitShapeProbe

    def __init__(self, shape: ToolBitShapeProbe, id: str | None = None):
        Path.Log.track(f"ToolBitProbe __init__ called with shape: {shape}, id: {id}")
        super().__init__(shape, id=id)
        self.obj.SpindleDirection = "None"
        self.obj.setEditorMode("SpindleDirection", 2)  # Read-only

    @property
    def summary(self) -> str:
        diameter = self.get_property_str("Diameter", "?")
        length = self.get_property_str("Length", "?")
        shaft_diameter = self.get_property_str("ShaftDiameter", "?")

        return FreeCAD.Qt.translate(
            "CAM",
            f"{diameter} probe, {length} length, {shaft_diameter} shaft"
        )

    def can_rotate(self) -> bool:
        return False
