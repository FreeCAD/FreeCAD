# -*- coding: utf-8 -*-

import pathlib
from typing import Optional
import Path
from ...shape import ToolBitShapeDovetail
from .base import ToolBit, ChiploadMixin


class ToolBitDovetail(ToolBit, ChiploadMixin):
    SHAPE_CLASS = ToolBitShapeDovetail

    def __init__(
        self,
        shape: ToolBitShapeDovetail,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track(f"ToolBitDovetail __init__ called with shape: {shape}")
        super().__init__(shape, path)
