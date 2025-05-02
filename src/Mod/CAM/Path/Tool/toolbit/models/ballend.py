# -*- coding: utf-8 -*-

import pathlib
from typing import Optional
import Path
from ...shape import ToolBitShapeBallend
from ..mixins import RotaryToolBitMixin, ChiploadMixin
from .base import ToolBit


class ToolBitBallend(ToolBit, ChiploadMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeBallend

    def __init__(
        self,
        shape: ToolBitShapeBallend,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track(f"ToolBitBallend __init__ called with shape: {shape}")
        super().__init__(shape, path)
