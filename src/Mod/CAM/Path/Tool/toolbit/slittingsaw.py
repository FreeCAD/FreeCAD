# -*- coding: utf-8 -*-

import FreeCAD
from typing import Any, Mapping, Tuple, Union, Optional
import Path
from .base import ToolBit
from ..shape.slittingsaw import ToolBitShapeSlittingSaw
import pathlib


class ToolBitSlittingSaw(ToolBit):
    SHAPE_CLASS = ToolBitShapeSlittingSaw

    def __init__(
        self,
        obj,
        shape: ToolBitShapeSlittingSaw,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track(f"ToolBitSlittingSaw __init__ called for {obj.Label}")
        super().__init__(obj, shape, path)

    @classmethod
    def schema(
        cls,
    ) -> Mapping[
        str, Union[Tuple[str, str, Any], Tuple[str, str, Any, Tuple[str, ...]]]
    ]:
        """
        This schema defines any properties that the tool supports and
        that are not part of the shape file.
        """
        return {
            **super(ToolBitSlittingSaw, cls).schema(),
            "Chipload": (
                FreeCAD.Qt.translate("ToolBit", "Chipload"),
                "App::PropertyLength",
                0.0,  # Default value
            ),
        }
