# -*- coding: utf-8 -*-

import pathlib
from typing import Optional
import Path
from ...shape import ToolBitShapeDovetail
from ..mixins import RotaryToolBitMixin, ChiploadMixin
from .base import ToolBit


class ToolBitDovetail(ToolBit, ChiploadMixin, RotaryToolBitMixin):
    SHAPE_CLASS = ToolBitShapeDovetail

    def __init__(
        self,
        shape: ToolBitShapeDovetail,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track(f"ToolBitDovetail __init__ called with shape: {shape}")
        super().__init__(shape, path)
