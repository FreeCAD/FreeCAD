# -*- coding: utf-8 -*-
# Defines the Vbit tool bit shape.

import FreeCAD
from typing import Any, Tuple, Union, Mapping
from .base import ToolBitShape


class ToolBitShapeVBit(ToolBitShape):
    name = "v-bit"
    aliases: Tuple[str, ...] = ("vbit",)

    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    @classmethod
    def shape_schema(cls) -> Mapping[str, Tuple[str, str]]:
        return {
            "CuttingEdgeAngle": (
                FreeCAD.Qt.translate("ToolBitShape", "Cutting edge angle"),
                "App::PropertyAngle",
            ),
            "CuttingEdgeHeight": (
                FreeCAD.Qt.translate("ToolBitShape", "Cutting edge height"),
                "App::PropertyLength",
            ),
            "Diameter": (
                FreeCAD.Qt.translate("ToolBitShape", "Diameter"),
                "App::PropertyLength",
            ),
            "Flutes": (
                FreeCAD.Qt.translate("ToolBitShape", "Flutes"),
                "App::PropertyInteger",
            ),
            "Length": (
                FreeCAD.Qt.translate("ToolBitShape", "Overall tool length"),
                "App::PropertyLength",
            ),
            "ShankDiameter": (
                FreeCAD.Qt.translate("ToolBitShape", "Shank diameter"),
                "App::PropertyLength",
            ),
            "TipDiameter": (
                FreeCAD.Qt.translate("ToolBitShape", "Tip diameter"),
                "App::PropertyLength",
            ),
        }

    @classmethod
    def feature_schema(
        cls,
    ) -> Mapping[str, Union[Tuple[str, str, Any], Tuple[str, str, Any, Tuple[str, ...]]]]:
        return {
            **super(ToolBitShapeVBit, cls).feature_schema(),
            "Chipload": (
                FreeCAD.Qt.translate("ToolBitShape", "Chipload"),
                "App::PropertyLength",
                0.0,  # Default value
            ),
        }

    @property
    def label(self) -> str:
        return FreeCAD.Qt.translate("ToolBitShape", "V-Bit")
