# -*- coding: utf-8 -*-
# Defines the Reamer tool bit shape.

import FreeCAD
from typing import Tuple
from .base import ToolBitShape


class ToolBitShapeReamer(ToolBitShape):
    aliases: Tuple[str, ...] = ("reamer",)
    _LABELS = {
        "CuttingEdgeHeight": "Cutting edge height",
        "Diameter": "Diameter",
        "Length": "Overall Tool Length",
        "ShankDiameter": "Shank diameter",
    }

    @property
    def label(self) -> str:
        return "Reamer"  # TODO translatable

    def set_default_parameters(self):
        self._params = {
            "CuttingEdgeHeight": (FreeCAD.Units.Quantity("25 mm"),
                                  'App::PropertyLength'),
            "Diameter": (FreeCAD.Units.Quantity("8 mm"),
                         'App::PropertyLength'),
            "Length": (FreeCAD.Units.Quantity("80 mm"),
                       'App::PropertyLength'),
            "ShankDiameter": (FreeCAD.Units.Quantity("8 mm"),
                               'App::PropertyLength'),
        }
