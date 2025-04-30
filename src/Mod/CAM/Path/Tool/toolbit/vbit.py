# -*- coding: utf-8 -*-

import pathlib
from typing import Optional
import Path
from .base import ToolBit, ChiploadMixin
from ..shape.vbit import ToolBitShapeVBit


class ToolBitVBit(ToolBit, ChiploadMixin):
    SHAPE_CLASS = ToolBitShapeVBit

    def __init__(
        self,
        obj,
        shape: ToolBitShapeVBit,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track(f"ToolBitVBit __init__ called for {obj.Label}")
        super().__init__(obj, shape, path)
