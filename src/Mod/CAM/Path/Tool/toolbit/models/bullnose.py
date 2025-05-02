# -*- coding: utf-8 -*-

import pathlib
from typing import Optional
import Path
from ...shape import ToolBitShapeBullnose
from ..mixins import RotaryToolBitMixin, ChiploadMixin
from .base import ToolBit

class ToolBitBullnose(ToolBit, ChiploadMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeBullnose

    def __init__(
        self,
        tool_bit_shape: ToolBitShapeBullnose,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track("ToolBitBullnose __init__ called")
        super().__init__(tool_bit_shape, path=path)

