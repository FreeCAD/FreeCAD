# -*- coding: utf-8 -*-
# Defines the Dovetail tool bit shape.

import FreeCAD
from typing import Tuple, Union, Mapping, Any
from .base import ToolBitShape


class ToolBitShapeDovetail(ToolBitShape):
    name = "dovetail"

    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    @classmethod
    def shape_schema(cls) -> Mapping[str, Tuple[str, str]]:
        return {
            "TipDiameter": (
                FreeCAD.Qt.translate("ToolBitShape", "Crest height"),
                "App::PropertyLength",
            ),
            "CuttingEdgeAngle": (
                FreeCAD.Qt.translate("ToolBitShape", "Cutting angle"),
                "App::PropertyAngle",
            ),
            "CuttingEdgeHeight": (
                FreeCAD.Qt.translate("ToolBitShape", "Dovetail height"),
                "App::PropertyLength",
            ),
            "Diameter": (
                FreeCAD.Qt.translate("ToolBitShape", "Major diameter"),
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
            "NeckDiameter": (
                FreeCAD.Qt.translate("ToolBitShape", "Neck diameter"),
                "App::PropertyLength",
            ),
            "NeckHeight": (
                FreeCAD.Qt.translate("ToolBitShape", "Neck length"),
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
            **super(ToolBitShapeDovetail, cls).feature_schema(),
            "Chipload": (
                FreeCAD.Qt.translate("ToolBitShape", "Chipload"),
                "App::PropertyLength",
                0.0,  # Default value
            ),
        }

    @property
    def label(self) -> str:
        return FreeCAD.Qt.translate("ToolBitShape", "Dovetail")
