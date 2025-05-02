# -*- coding: utf-8 -*-

import pathlib
from typing import Optional
import Path
from ...shape import ToolBitShapeThreadMill
from ..mixins import RotaryToolBitMixin, ChiploadMixin
from .base import ToolBit


class ToolBitThreadMill(ToolBit, ChiploadMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeThreadMill

    def __init__(
        self,
        shape: ToolBitShapeThreadMill,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track(f"ToolBitThreadMill __init__ called with shape: {shape}")
        super().__init__(shape, path)
