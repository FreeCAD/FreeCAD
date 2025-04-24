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
    def schema(cls) -> Mapping[str, Tuple[str, str]]:
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

    @property
    def label(self) -> str:
        return FreeCAD.Qt.translate("ToolBitShape", "Slitting Saw")
