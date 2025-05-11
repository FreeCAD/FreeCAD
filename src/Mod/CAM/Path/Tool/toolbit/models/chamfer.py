# -*- coding: utf-8 -*-
import Path
from ...shape import ToolBitShapeChamfer
from ..mixins import RotaryToolBitMixin, ChiploadMixin
from .base import ToolBit


class ToolBitChamfer(ToolBit, ChiploadMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeChamfer

    def __init__(self, shape: ToolBitShapeChamfer, id: str | None = None):
        Path.Log.track(f"ToolBitChamfer __init__ called with shape: {shape}, id: {id}")
        super().__init__(shape, id=id)
