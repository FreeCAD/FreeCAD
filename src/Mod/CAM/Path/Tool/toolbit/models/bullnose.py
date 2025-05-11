# -*- coding: utf-8 -*-
import Path
from ...shape import ToolBitShapeBullnose
from ..mixins import RotaryToolBitMixin, ChiploadMixin
from .base import ToolBit


class ToolBitBullnose(ToolBit, ChiploadMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeBullnose

    def __init__(self, tool_bit_shape: ToolBitShapeBullnose, id: str | None = None):
        Path.Log.track(f"ToolBitBullnose __init__ called with id: {id}")
        super().__init__(tool_bit_shape, id=id)
