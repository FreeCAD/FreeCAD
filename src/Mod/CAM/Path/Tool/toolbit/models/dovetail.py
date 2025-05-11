# -*- coding: utf-8 -*-
import Path
from ...shape import ToolBitShapeDovetail
from ..mixins import RotaryToolBitMixin, ChiploadMixin
from .base import ToolBit


class ToolBitDovetail(ToolBit, ChiploadMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeDovetail

    def __init__(self, shape: ToolBitShapeDovetail, id: str | None = None):
        Path.Log.track(f"ToolBitDovetail __init__ called with shape: {shape}, id: {id}")
        super().__init__(shape, id=id)
