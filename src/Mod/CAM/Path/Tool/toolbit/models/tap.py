# -*- coding: utf-8 -*-
import Path
from ...shape import ToolBitShapeTap
from ..mixins import RotaryToolBitMixin, ChiploadMixin
from .base import ToolBit


class ToolBitTap(ToolBit, ChiploadMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeTap

    def __init__(self, shape: ToolBitShapeTap, id: str | None = None):
        Path.Log.track(f"ToolBitTap __init__ called with shape: {shape}, id: {id}")
        super().__init__(shape, id=id)
