# -*- coding: utf-8 -*-

import FreeCAD
from typing import Any, Mapping, Tuple, Union, Optional
import Path
from .base import ToolBit
from ..shape.vbit import ToolBitShapeVBit
import pathlib


class ToolBitVBit(ToolBit):
    SHAPE_CLASS = ToolBitShapeVBit

    def __init__(
        self,
        obj,
        shape: ToolBitShapeVBit,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track(f"ToolBitVBit __init__ called for {obj.Label}")
        super().__init__(obj, shape, path)

    @classmethod
    def schema(
        cls,
    ) -> Mapping[str, Union[Tuple[str, str, Any], Tuple[str, str, Any, Tuple[str, ...]]]]:
        """
        This schema defines any properties that the tool supports and
        that are not part of the shape file.
        """
        return {
            **super(ToolBitVBit, cls).schema(),
            "Chipload": (
                FreeCAD.Qt.translate("ToolBit", "Chipload"),
                "App::PropertyLength",
                0.0,  # Default value
            ),
        }
