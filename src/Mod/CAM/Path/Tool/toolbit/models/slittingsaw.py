# -*- coding: utf-8 -*-
import Path
from ...shape import ToolBitShapeSlittingSaw
from ..mixins import RotaryToolBitMixin, ChiploadMixin
from .base import ToolBit


class ToolBitSlittingSaw(ToolBit, ChiploadMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeSlittingSaw

    def __init__(self, shape: ToolBitShapeSlittingSaw, id: str | None = None):
        Path.Log.track(
            f"ToolBitSlittingSaw __init__ called with shape: {shape}, id: {id}"
        )
        super().__init__(shape, id=id)
