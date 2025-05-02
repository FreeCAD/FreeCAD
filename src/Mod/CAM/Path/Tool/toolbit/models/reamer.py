# -*- coding: utf-8 -*-

import pathlib
from typing import Optional
import Path
from ...shape import ToolBitShapeReamer
from ..mixins import RotaryToolBitMixin, ChiploadMixin
from .base import ToolBit


class ToolBitReamer(ToolBit, ChiploadMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeReamer

    def __init__(
        self,
        shape: ToolBitShapeReamer,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track(f"ToolBitReamer __init__ called with shape: {shape}")
        super().__init__(shape, path)
