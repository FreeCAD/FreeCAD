# -*- coding: utf-8 -*-
# Defines the Reamer tool bit shape.

import FreeCAD
from typing import Tuple, Mapping, Any, Union
from .base import ToolBitShape


class ToolBitShapeReamer(ToolBitShape):
    name = "reamer"

    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    @classmethod
    def schema(cls) -> Mapping[str, Tuple[str, str]]:
        return {
            "CuttingEdgeHeight": (
                FreeCAD.Qt.translate("ToolBitShape", "Cutting edge height"),
                "App::PropertyLength",
            ),
            "Diameter": (
                FreeCAD.Qt.translate("ToolBitShape", "Diameter"),
                "App::PropertyLength",
            ),
            "Length": (
                FreeCAD.Qt.translate("ToolBitShape", "Overall tool length"),
                "App::PropertyLength",
            ),
            "ShankDiameter": (
                FreeCAD.Qt.translate("ToolBitShape", "Shank diameter"),
                "App::PropertyLength",
            ),
        }

    @property
    def label(self) -> str:
        return FreeCAD.Qt.translate("ToolBitShape", "Reamer")
