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
        obj,
        shape: ToolBitShapeChamfer,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track(f"ToolBitChamfer __init__ called for {obj.Label}")
        super().__init__(obj, shape, path)
