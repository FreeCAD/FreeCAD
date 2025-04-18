# -*- coding: utf-8 -*-
# Defines the BallEnd tool bit shape.

import FreeCAD
from typing import Tuple
from .base import ToolBitShape


class ToolBitShapeBallEnd(ToolBitShape):
    aliases: Tuple[str, ...] = ("ballend",)
    _LABELS = {
        "CuttingEdgeHeight": "Cutting edge height",
        "Diameter": "Diameter",
        "Length": "Overall Tool Length",
        "ShankDiameter": "Shank diameter",
    }

    @property
    def label(self) -> str:
        return "Ballend"  # TODO translatable

    def set_default_parameters(self):
        self._params = {
            "CuttingEdgeHeight": (FreeCAD.Units.Quantity("10 mm"),
                                  'App::PropertyLength'),
            "Diameter": (FreeCAD.Units.Quantity("6 mm"),
                         'App::PropertyLength'),
            "Length": (FreeCAD.Units.Quantity("50 mm"),
                       'App::PropertyLength'),
            "ShankDiameter": (FreeCAD.Units.Quantity("6 mm"),
                              'App::PropertyLength'),
        }
