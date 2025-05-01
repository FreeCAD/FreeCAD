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
        obj,
        shape: ToolBitShapeThreadMill,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track(f"ToolBitThreadMill __init__ called for {obj.Label}")
        super().__init__(obj, shape, path)
