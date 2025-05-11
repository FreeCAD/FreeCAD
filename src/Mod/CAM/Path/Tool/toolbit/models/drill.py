# -*- coding: utf-8 -*-
import Path
from ...shape import ToolBitShapeDrill
from ..mixins import RotaryToolBitMixin, ChiploadMixin
from .base import ToolBit


class ToolBitDrill(ToolBit, ChiploadMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeDrill

    def __init__(self, shape: ToolBitShapeDrill, id: str | None = None):
        Path.Log.track(f"ToolBitDrill __init__ called with shape: {shape}, id: {id}")
        super().__init__(shape, id=id)
