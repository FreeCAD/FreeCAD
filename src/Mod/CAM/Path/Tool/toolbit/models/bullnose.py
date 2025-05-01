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
        obj,
        shape: ToolBitShapeBullnose,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track(f"ToolBitBullnose __init__ called for {obj.Label}")
        super().__init__(obj, shape, path)
