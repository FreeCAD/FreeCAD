# -*- coding: utf-8 -*-
import Path
from ...shape import ToolBitShapeThreadMill
from ..mixins import RotaryToolBitMixin, ChiploadMixin
from .base import ToolBit


class ToolBitThreadMill(ToolBit, ChiploadMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeThreadMill

    def __init__(self, shape: ToolBitShapeThreadMill, id: str | None = None):
        Path.Log.track(
            f"ToolBitThreadMill __init__ called with shape: {shape}, id: {id}"
        )
        super().__init__(shape, id=id)
