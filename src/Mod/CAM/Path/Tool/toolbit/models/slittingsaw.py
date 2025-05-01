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
        obj,
        shape: ToolBitShapeSlittingSaw,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track(f"ToolBitSlittingSaw __init__ called for {obj.Label}")
        super().__init__(obj, shape, path)
