# -*- coding: utf-8 -*-
# Defines the SlittingSaw tool bit shape.

import FreeCAD
from typing import Tuple
from .base import ToolBitShape


class ToolBitShapeSlittingSaw(ToolBitShape):
    name = "slittingsaw"
    aliases = ("slitting-saw",)
    _schema = {
        "BladeThickness": 'App::PropertyLength',
        "CapDiameter": 'App::PropertyLength',
        "CapHeight": 'App::PropertyLength',
        "Diameter": 'App::PropertyLength',
        "Length": 'App::PropertyLength',
        "ShankDiameter": 'App::PropertyLength',
    }

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self._labels = {
            "BladeThickness": FreeCAD.Qt.translate(
                "ToolBitShapeSlittingSaw", "Blade thickness"),
            "CapDiameter": FreeCAD.Qt.translate(
                "ToolBitShapeSlittingSaw", "Cap diameter"),
            "CapHeight": FreeCAD.Qt.translate(
                "ToolBitShapeSlittingSaw", "Cap height"),
            "Diameter": FreeCAD.Qt.translate(
                "ToolBitShapeSlittingSaw", "Diameter"),
            "Length": FreeCAD.Qt.translate(
                "ToolBitShapeSlittingSaw", "Overall Tool Length"),
            "ShankDiameter": FreeCAD.Qt.translate(
                "ToolBitShapeSlittingSaw", "Shank diameter"),
        }

    @property
    def label(self) -> str:
        return FreeCAD.Qt.translate("ToolBitShapeSlittingSaw", "Slitting Saw")
