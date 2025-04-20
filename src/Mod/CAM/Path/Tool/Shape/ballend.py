# -*- coding: utf-8 -*-
# Defines the BallEnd tool bit shape.

import FreeCAD
from typing import Tuple
from .base import ToolBitShape


class ToolBitShapeBallEnd(ToolBitShape):
    name: str = "ballend"
    _schema = {
        "CuttingEdgeHeight": "App::PropertyLength",
        "Diameter": "App::PropertyLength",
        "Length": "App::PropertyLength",
        "ShankDiameter": "App::PropertyLength",
    }

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self._labels = {
            "CuttingEdgeHeight": FreeCAD.Qt.translate(
                "ToolBitShapeBallEnd", "Cutting edge height"
            ),
            "Diameter": FreeCAD.Qt.translate("ToolBitShapeBallEnd", "Diameter"),
            "Length": FreeCAD.Qt.translate(
                "ToolBitShapeBallEnd", "Overall Tool Length"
            ),
            "ShankDiameter": FreeCAD.Qt.translate(
                "ToolBitShapeBallEnd", "Shank diameter"
            ),
        }

    @property
    def label(self) -> str:
        return FreeCAD.Qt.translate("ToolBitShapeBallEnd", "Ballend")
