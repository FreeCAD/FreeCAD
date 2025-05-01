# -*- coding: utf-8 -*-

import pathlib
from typing import Optional
import Path
from ...shape import ToolBitShapeBullnose
from .base import ToolBit, ChiploadMixin

class ToolBitBullnose(ToolBit, ChiploadMixin):
    SHAPE_CLASS = ToolBitShapeBullnose

    def __init__(
        self,
        tool_bit_shape: ToolBitShapeBullnose,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track("ToolBitBullnose __init__ called")
        super().__init__(tool_bit_shape, path=path)

