# -*- coding: utf-8 -*-
import Path
from ...shape import ToolBitShapeBallend
from ..mixins import RotaryToolBitMixin, ChiploadMixin
from .base import ToolBit


class ToolBitBallend(ToolBit, ChiploadMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeBallend

    def __init__(self, shape: ToolBitShapeBallend, id: str | None = None):
        Path.Log.track(f"ToolBitBallend __init__ called with shape: {shape}, id: {id}")
        super().__init__(shape, id=id)
