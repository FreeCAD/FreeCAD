# -*- coding: utf-8 -*-
# Defines the Probe tool bit shape.

import FreeCAD
from typing import Tuple, Mapping
from .base import ToolBitShape


class ToolBitShapeProbe(ToolBitShape):
    name = "probe"

    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    @classmethod
    def shape_schema(cls) -> Mapping[str, Tuple[str, str]]:
        return {
            "Diameter": (
                FreeCAD.Qt.translate("ToolBitShape", "Ball diameter"),
                "App::PropertyLength",
            ),
            "Length": (
                FreeCAD.Qt.translate("ToolBitShape", "Length of probe"),
                "App::PropertyLength",
            ),
            "ShaftDiameter": (
                FreeCAD.Qt.translate("ToolBitShape", "Shaft diameter"),
                "App::PropertyLength",
            ),
        }

    @property
    def label(self) -> str:
        return FreeCAD.Qt.translate("ToolBitShape", "Probe")

    def can_rotate(self) -> bool:
        return False
