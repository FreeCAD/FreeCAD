# -*- coding: utf-8 -*-

import pathlib
from typing import Optional
import Path
from .base import ToolBit, ChiploadMixin
from ..shape.ballend import ToolBitShapeBallend


class ToolBitBallend(ToolBit, ChiploadMixin):
    SHAPE_CLASS = ToolBitShapeBallend

    def __init__(
        self,
        obj,
        shape: ToolBitShapeBallend,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track(f"ToolBitBallend __init__ called for {obj.Label}")
        super().__init__(obj, shape, path)
