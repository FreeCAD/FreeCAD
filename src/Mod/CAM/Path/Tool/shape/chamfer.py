# -*- coding: utf-8 -*-
# Defines the Chamfer tool bit shape.

import FreeCAD
from typing import Tuple, Union, Mapping, Any
from .base import ToolBitShape


class ToolBitShapeChamfer(ToolBitShape):
    name = "chamfer"

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
        }

    @classmethod
    def feature_schema(
        cls,
    ) -> Mapping[str, Union[Tuple[str, str, Any], Tuple[str, str, Any, Tuple[str, ...]]]]:
        return {
            **super(ToolBitShapeChamfer, cls).feature_schema(),
            "Chipload": (
                FreeCAD.Qt.translate("ToolBitShape", "Chipload"),
                "App::PropertyLength",
                0.0,  # Default value
            ),
        }

    @property
    def label(self) -> str:
        return FreeCAD.Qt.translate("ToolBitShape", "Chamfer")
