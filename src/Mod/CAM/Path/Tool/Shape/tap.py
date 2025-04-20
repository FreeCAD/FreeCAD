# -*- coding: utf-8 -*-
# Defines the Tap tool bit shape.

import FreeCAD
from typing import Tuple
from .base import ToolBitShape


class ToolBitShapeTap(ToolBitShape):
    name = "tap"
    _schema = {
        "CuttingEdgeLength": 'App::PropertyLength',
        "Diameter": 'App::PropertyLength',
        "Length": 'App::PropertyLength',
        "ShankDiameter": 'App::PropertyLength',
        "TipAngle": 'App::PropertyAngle',
    }

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self._labels = {
            "CuttingEdgeLength": FreeCAD.Qt.translate(
                "ToolBitShapeTap", "Cutting edge length"),
            "Diameter": FreeCAD.Qt.translate(
                "ToolBitShapeTap", "Tap Diameter"),
            "Length": FreeCAD.Qt.translate(
                "ToolBitShapeTap", "Overall Length of Tap"),
            "ShankDiameter": FreeCAD.Qt.translate(
                "ToolBitShapeTap", "Shank Diameter"),
            "TipAngle": FreeCAD.Qt.translate(
                "ToolBitShapeTap", "Tip Angle"),
        }

    @property
    def label(self) -> str:
        return FreeCAD.Qt.translate("ToolBitShapeTap", "Tap")
