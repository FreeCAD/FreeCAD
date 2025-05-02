# -*- coding: utf-8 -*-

import pathlib
from typing import Optional
import Path
from ...shape import ToolBitShapeVBit
from ..mixins import RotaryToolBitMixin, ChiploadMixin
from .base import ToolBit


class ToolBitVBit(ToolBit, ChiploadMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeVBit

    def __init__(
        self,
        shape: ToolBitShapeVBit,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track(f"ToolBitVBit __init__ called with shape: {shape}")
        super().__init__(shape, path)
