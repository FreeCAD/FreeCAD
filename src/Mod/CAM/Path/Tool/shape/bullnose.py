# -*- coding: utf-8 -*-
# Defines the Torus (Bullnose) tool bit shape.

import FreeCAD
from typing import Dict, Tuple, Union, Mapping, Any
from .base import ToolBitShape


class ToolBitShapeBullnose(ToolBitShape):
    name = "bullnose"
    aliases = ("torus",)

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
            "FlatRadius": (
                FreeCAD.Qt.translate("ToolBitShape", "Torus radius"),
                "App::PropertyLength",
            ),
        }

    @property
    def label(self) -> str:
        return FreeCAD.Qt.translate("ToolBitShape", "Torus")
