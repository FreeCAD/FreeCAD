# -*- coding: utf-8 -*-
# Defines the Tap tool bit shape.

import FreeCAD
from typing import Tuple, Mapping, Any, Union
from .base import ToolBitShape


class ToolBitShapeTap(ToolBitShape):
    name = "tap"

    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    @classmethod
    def schema(cls) -> Mapping[str, Tuple[str, str]]:
        return {
            "CuttingEdgeLength": (
                FreeCAD.Qt.translate("ToolBitShape", "Cutting edge length"),
                "App::PropertyLength",
            ),
            "Diameter": (
                FreeCAD.Qt.translate("ToolBitShape", "Tap diameter"),
                "App::PropertyLength",
            ),
            "Flutes": (
                FreeCAD.Qt.translate("ToolBitShape", "Flutes"),
                "App::PropertyInteger",
            ),
            "Length": (
                FreeCAD.Qt.translate("ToolBitShape", "Overall length of tap"),
                "App::PropertyLength",
            ),
            "ShankDiameter": (
                FreeCAD.Qt.translate("ToolBitShape", "Shank diameter"),
                "App::PropertyLength",
            ),
            "TipAngle": (
                FreeCAD.Qt.translate("ToolBitShape", "Tip angle"),
                "App::PropertyAngle",
            ),
        }

    @property
    def label(self) -> str:
        return FreeCAD.Qt.translate("ToolBitShape", "Tap")
