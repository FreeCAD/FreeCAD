# -*- coding: utf-8 -*-

import pathlib
from typing import Optional
import Path
from ...shape import ToolBitShapeDrill
from .base import ToolBit, ChiploadMixin


class ToolBitDrill(ToolBit, ChiploadMixin):
    SHAPE_CLASS = ToolBitShapeDrill

    def __init__(
        self,
        shape: ToolBitShapeDrill,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track(f"ToolBitDrill __init__ called with shape: {shape}")
        super().__init__(shape, path)
