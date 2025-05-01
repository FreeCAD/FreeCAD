# -*- coding: utf-8 -*-

import pathlib
from typing import Optional
import Path
from ...shape import ToolBitShapeReamer
from .base import ToolBit, ChiploadMixin


class ToolBitReamer(ToolBit, ChiploadMixin):
    SHAPE_CLASS = ToolBitShapeReamer

    def __init__(
        self,
        obj,
        shape: ToolBitShapeReamer,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track(f"ToolBitReamer __init__ called for {obj.Label}")
        super().__init__(obj, shape, path)
