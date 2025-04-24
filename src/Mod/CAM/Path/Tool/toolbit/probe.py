# -*- coding: utf-8 -*-

from typing import Optional
import Path
import pathlib
from .base import ToolBit
from ..shape.probe import ToolBitShapeProbe


class ToolBitProbe(ToolBit):
    SHAPE_CLASS = ToolBitShapeProbe

    def __init__(
        self,
        obj,
        shape: ToolBitShapeProbe,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track(f"ToolBitProbe __init__ called for {obj.Label}")
        super().__init__(obj, shape, path)
