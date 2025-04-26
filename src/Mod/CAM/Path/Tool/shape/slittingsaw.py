# -*- coding: utf-8 -*-
# Defines the SlittingSaw tool bit shape.

import FreeCAD
from typing import Tuple, Mapping
from .base import ToolBitShape


class ToolBitShapeSlittingSaw(ToolBitShape):
    name = "SlittingSaw"
    aliases = "slittingsaw", "slitting-saw"

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
