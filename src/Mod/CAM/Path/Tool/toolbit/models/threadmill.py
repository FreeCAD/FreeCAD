# -*- coding: utf-8 -*-

import pathlib
from typing import Optional
import Path
from ...shape import ToolBitShapeThreadMill
from .base import ToolBit, ChiploadMixin


class ToolBitThreadMill(ToolBit, ChiploadMixin):
    SHAPE_CLASS = ToolBitShapeThreadMill

    def __init__(
        self,
        shape: ToolBitShapeThreadMill,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track(f"ToolBitThreadMill __init__ called with shape: {shape}")
        super().__init__(shape, path)
