# -*- coding: utf-8 -*-

import FreeCAD
from typing import Any, Mapping, Tuple, Union, Optional, cast
import Path
from .base import ToolBit
from ..shape.chamfer import ToolBitShapeChamfer
import pathlib


class ToolBitChamfer(ToolBit):
    SHAPE_CLASS = ToolBitShapeChamfer

    def __init__(
        self,
        obj,
        shape: ToolBitShapeChamfer,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track(f"ToolBitChamfer __init__ called for {obj.Label}")
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
            **super(ToolBitChamfer, cls).schema(),
            "Chipload": (
                FreeCAD.Qt.translate("ToolBit", "Chipload"),
                "App::PropertyLength",
                0.0,  # Default value
            ),
        }
