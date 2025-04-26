# -*- coding: utf-8 -*-
# Defines the Chamfer tool bit shape.

import FreeCAD
from typing import Tuple, Mapping
from .base import ToolBitShape


class ToolBitShapeChamfer(ToolBitShape):
    name = "Chamfer"
    aliases = ("chamfer",)

    @classmethod
    def schema(cls) -> Mapping[str, Tuple[str, str]]:
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

    @property
    def label(self) -> str:
        return FreeCAD.Qt.translate("ToolBitShape", "Chamfer")
