# -*- coding: utf-8 -*-

import pathlib
from typing import Optional
import Path
from ...shape import ToolBitShapeSlittingSaw
from .base import ToolBit, ChiploadMixin


class ToolBitSlittingSaw(ToolBit, ChiploadMixin):
    SHAPE_CLASS = ToolBitShapeSlittingSaw

    def __init__(
        self,
        shape: ToolBitShapeSlittingSaw,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track(f"ToolBitSlittingSaw __init__ called with shape: {shape}")
        super().__init__(shape, path)
