# -*- coding: utf-8 -*-
# Defines the Vbit tool bit shape.

import FreeCAD
from typing import Tuple
from .base import ToolBitShape


class ToolBitShapeVBit(ToolBitShape):
    name = "v-bit"
    aliases: Tuple[str, ...] = ("vbit",)
    _schema = {
        "CuttingEdgeAngle": "App::PropertyAngle",
        "CuttingEdgeHeight": "App::PropertyLength",
        "Diameter": "App::PropertyLength",
        "Length": "App::PropertyLength",
        "ShankDiameter": "App::PropertyLength",
        "TipDiameter": "App::PropertyLength",
    }

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self._labels = {
            "CuttingEdgeAngle": FreeCAD.Qt.translate(
                "ToolBitShapeVBit", "Cutting edge angle"
            ),
            "CuttingEdgeHeight": FreeCAD.Qt.translate(
                "ToolBitShapeVBit", "Cutting edge height"
            ),
            "Diameter": FreeCAD.Qt.translate("ToolBitShapeVBit", "Diameter"),
            "Length": FreeCAD.Qt.translate(
                "ToolBitShapeVBit", "Overall Tool Length"
            ),
            "ShankDiameter": FreeCAD.Qt.translate(
                "ToolBitShapeVBit", "Shank diameter"
            ),
            "TipDiameter": FreeCAD.Qt.translate(
                "ToolBitShapeVBit", "Tip diameter"
            ),
        }

    @property
    def label(self) -> str:
        return FreeCAD.Qt.translate("ToolBitShapeVBit", "V-Bit")
