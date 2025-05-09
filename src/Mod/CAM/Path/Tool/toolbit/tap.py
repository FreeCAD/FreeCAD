# -*- coding: utf-8 -*-

import pathlib
from typing import Optional
import Path
from .base import ToolBit, ChiploadMixin
from ..shape.tap import ToolBitShapeTap


class ToolBitTap(ToolBit, ChiploadMixin):
    SHAPE_CLASS = ToolBitShapeTap

    def __init__(
        self,
        obj,
        shape: ToolBitShapeTap,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track(f"ToolBitTap __init__ called for {obj.Label}")
        super().__init__(obj, shape, path)
