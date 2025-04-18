# -*- coding: utf-8 -*-
# Defines the EndMill tool bit shape.

import FreeCAD
from typing import Tuple
from .base import ToolBitShape


class ToolBitShapeEndMill(ToolBitShape):
    aliases: Tuple[str, ...] = ("endmill",)
    _LABELS = {
        "CuttingEdgeHeight": "Cutting edge height",
        "Diameter": "Diameter",
        "Length": "Overall Tool Length",
        "ShankDiameter": "Shank diameter",
    }

    @property
    def label(self) -> str:
        return "Endmill"  # TODO translatable

    def set_default_parameters(self):
        self._params = {
            "CuttingEdgeHeight": (FreeCAD.Units.Quantity("20 mm"),
                                  'App::PropertyLength'),
            "Diameter": (FreeCAD.Units.Quantity("10 mm"),
                         'App::PropertyLength'),
            "Length": (FreeCAD.Units.Quantity("75 mm"),
                       'App::PropertyLength'),
            "ShankDiameter": (FreeCAD.Units.Quantity("10 mm"),
                              'App::PropertyLength'),
        }
