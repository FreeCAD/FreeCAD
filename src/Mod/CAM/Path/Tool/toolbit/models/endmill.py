# -*- coding: utf-8 -*-

import pathlib
from typing import Optional
import Path
from ...shape import ToolBitShapeEndmill
from ..mixins import RotaryToolBitMixin, ChiploadMixin
from .base import ToolBit


class ToolBitEndmill(ToolBit, ChiploadMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeEndmill

    def __init__(
        self,
        shape: ToolBitShapeEndmill,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track(f"ToolBitEndmill __init__ called with shape: {shape}")
        super().__init__(shape, path)
