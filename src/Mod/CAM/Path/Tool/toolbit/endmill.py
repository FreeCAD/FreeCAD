# -*- coding: utf-8 -*-

import pathlib
from typing import Optional
import Path
from .base import ToolBit, ChiploadMixin
from ..shape.endmill import ToolBitShapeEndmill


class ToolBitEndmill(ToolBit, ChiploadMixin):
    SHAPE_CLASS = ToolBitShapeEndmill

    def __init__(
        self,
        obj,
        shape: ToolBitShapeEndmill,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track(f"ToolBitEndmill __init__ called for {obj.Label}")
        super().__init__(obj, shape, path)
