# -*- coding: utf-8 -*-
# Defines the Torus (Bullnose) tool bit shape.

import FreeCAD
from typing import Tuple
from .base import ToolBitShape


class ToolBitShapeBullnose(ToolBitShape):
    name = "bullnose"
    _schema = {
        "CuttingEdgeHeight": 'App::PropertyLength',
        "Diameter": 'App::PropertyLength',
        "Length": 'App::PropertyLength',
        "ShankDiameter": 'App::PropertyLength',
        "FlatRadius": 'App::PropertyLength',
    }

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self._labels = {
            "CuttingEdgeHeight": FreeCAD.Qt.translate(
                "ToolBitShapeBullnose", "Cutting edge height"),
            "Diameter": FreeCAD.Qt.translate(
                "ToolBitShapeBullnose", "Diameter"),
            "Length": FreeCAD.Qt.translate(
                "ToolBitShapeBullnose", "Overall Tool Length"),
            "ShankDiameter": FreeCAD.Qt.translate(
                "ToolBitShapeBullnose", "Shank diameter"),
            "FlatRadius": FreeCAD.Qt.translate(
                "ToolBitShapeBullnose", "Torus radius"),
        }

    @property
    def label(self) -> str:
        return FreeCAD.Qt.translate("ToolBitShapeBullnose", "Torus")
