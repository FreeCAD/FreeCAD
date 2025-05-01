# -*- coding: utf-8 -*-

import pathlib
from typing import Optional
import Path
from ...shape import ToolBitShapeChamfer
from .base import ToolBit, ChiploadMixin


class ToolBitChamfer(ToolBit, ChiploadMixin):
    SHAPE_CLASS = ToolBitShapeChamfer

    def __init__(
        self,
        shape: ToolBitShapeChamfer,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track(f"ToolBitChamfer __init__ called with shape: {shape}")
        super().__init__(shape, path)
