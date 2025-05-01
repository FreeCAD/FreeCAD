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
        obj,
        shape: ToolBitShapeDrill,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track(f"ToolBitDrill __init__ called for {obj.Label}")
        super().__init__(obj, shape, path)
