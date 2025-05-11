# -*- coding: utf-8 -*-
import Path
from ...shape import ToolBitShapeVBit
from ..mixins import RotaryToolBitMixin, ChiploadMixin
from .base import ToolBit


class ToolBitVBit(ToolBit, ChiploadMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeVBit

    def __init__(self, shape: ToolBitShapeVBit, id: str | None = None):
        Path.Log.track(f"ToolBitVBit __init__ called with shape: {shape}, id: {id}")
        super().__init__(shape, id=id)
