# -*- coding: utf-8 -*-
# Defines the SlittingSaw tool bit shape.

import FreeCAD
from typing import Tuple, Union, Mapping, Any
from .base import ToolBitShape


class ToolBitShapeSlittingSaw(ToolBitShape):
    name = "slittingsaw"
    aliases = ("slitting-saw",)

    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    @classmethod
    def shape_schema(cls) -> Mapping[str, Tuple[str, str]]:
        return {
            "BladeThickness": (
                FreeCAD.Qt.translate("ToolBitShape", "Blade thickness"),
                "App::PropertyLength",
            ),
            "CapDiameter": (
                FreeCAD.Qt.translate("ToolBitShape", "Cap diameter"),
                "App::PropertyLength",
            ),
            "CapHeight": (
                FreeCAD.Qt.translate("ToolBitShape", "Cap height"),
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
            **super(ToolBitShapeSlittingSaw, cls).feature_schema(),
            "Chipload": (
                FreeCAD.Qt.translate("ToolBitShape", "Chipload"),
                "App::PropertyLength",
                0.0,  # Default value
            ),
        }

    @property
    def label(self) -> str:
        return FreeCAD.Qt.translate("ToolBitShape", "Slitting Saw")
