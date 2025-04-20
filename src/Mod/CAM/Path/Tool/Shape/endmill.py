# -*- coding: utf-8 -*-
# Defines the EndMill tool bit shape.

import FreeCAD
from typing import Tuple
from .base import ToolBitShape


class ToolBitShapeEndMill(ToolBitShape):
    name = "endmill"
    _schema = {
        "CuttingEdgeHeight": 'App::PropertyLength',
        "Diameter": 'App::PropertyLength',
        "Length": 'App::PropertyLength',
        "ShankDiameter": 'App::PropertyLength',
    }

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self._labels = {
            "CuttingEdgeHeight": FreeCAD.Qt.translate(
                "ToolBitShapeEndMill", "Cutting edge height"),
            "Diameter": FreeCAD.Qt.translate(
                "ToolBitShapeEndMill", "Diameter"),
            "Length": FreeCAD.Qt.translate(
                "ToolBitShapeEndMill", "Overall Tool Length"),
            "ShankDiameter": FreeCAD.Qt.translate(
                "ToolBitShapeEndMill", "Shank diameter"),
        }

    @property
    def label(self) -> str:
        return FreeCAD.Qt.translate("ToolBitShapeEndMill", "Endmill")
