# -*- coding: utf-8 -*-
# Defines the Chamfer tool bit shape.

import FreeCAD
from typing import Tuple
from .base import ToolBitShape


class ToolBitShapeChamfer(ToolBitShape):
    name = "chamfer"
    _schema = {
        "CuttingEdgeAngle": 'App::PropertyAngle',
        "CuttingEdgeHeight": 'App::PropertyLength',
        "Diameter": 'App::PropertyLength',
        "Length": 'App::PropertyLength',
        "ShankDiameter": 'App::PropertyLength',
    }

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self._labels = {
            "CuttingEdgeAngle": FreeCAD.Qt.translate(
                "ToolBitShapeChamfer", "Cutting edge angle"),
            "CuttingEdgeHeight": FreeCAD.Qt.translate(
                "ToolBitShapeChamfer", "Cutting edge height"),
            "Diameter": FreeCAD.Qt.translate(
                "ToolBitShapeChamfer", "Diameter"),
            "Length": FreeCAD.Qt.translate(
                "ToolBitShapeChamfer", "Overall Tool Length"),
            "ShankDiameter": FreeCAD.Qt.translate(
                "ToolBitShapeChamfer", "Shank diameter"),
        }

    @property
    def label(self) -> str:
        return FreeCAD.Qt.translate("ToolBitShapeChamfer", "Chamfer")
