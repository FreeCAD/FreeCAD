# -*- coding: utf-8 -*-

import FreeCAD
from typing import Any, Mapping, Tuple, Union, Optional, cast
import Path
from .base import ToolBit
from ..shape.ballend import ToolBitShapeBallEnd
from ..shape.util import get_shape_from_name
import pathlib


class ToolBitBallEnd(ToolBit):
    SHAPE_CLASS = ToolBitShapeBallEnd

    def __init__(
        self,
        obj,
        shape: ToolBitShapeBallEnd,
        path: Optional[pathlib.Path] = None,
    ):
        Path.Log.track(f"ToolBitBallEnd __init__ called for {obj.Label}")
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
            **super(ToolBitBallEnd, cls).schema(),
            "Chipload": (
                FreeCAD.Qt.translate("ToolBit", "Chipload"),
                "App::PropertyLength",
                0.0,  # Default value
            ),
        }
