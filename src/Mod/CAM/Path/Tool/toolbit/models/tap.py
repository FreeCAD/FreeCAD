# -*- coding: utf-8 -*-

import pathlib
from typing import Optional
import Path
from ...shape import ToolBitShapeTap
from .base import ToolBit, ChiploadMixin


class ToolBitTap(ToolBit, ChiploadMixin):
    SHAPE_CLASS = ToolBitShapeTap

    def __init__(
        self,
        shape: ToolBitShapeTap,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track(f"ToolBitTap __init__ called with shape: {shape}")
        super().__init__(shape, path)
