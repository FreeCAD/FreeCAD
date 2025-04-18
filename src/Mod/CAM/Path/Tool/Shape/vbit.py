# -*- coding: utf-8 -*-
# Defines the Vbit tool bit shape.

import FreeCAD
from typing import Tuple
from .base import ToolBitShape


class ToolBitShapeVBit(ToolBitShape):
    aliases: Tuple[str, ...] = "v-bit", "vbit"
    _LABELS = {
        "CuttingEdgeAngle": "Cutting edge angle",
        "CuttingEdgeHeight": "Cutting edge height",
        "Diameter": "Diameter",
        "Length": "Overall Tool Length",
        "ShankDiameter": "Shank diameter",
        "TipDiameter": "Tip diameter",
    }

    @property
    def label(self) -> str:
        return "V-Bit"  # TODO translatable

    def set_default_parameters(self):
        self._params = {
            "CuttingEdgeAngle": (FreeCAD.Units.Quantity("60 deg"),
                                 'App::PropertyAngle'),
            "CuttingEdgeHeight": (FreeCAD.Units.Quantity("10 mm"),
                                  'App::PropertyLength'),
            "Diameter": (FreeCAD.Units.Quantity("12 mm"),
                         'App::PropertyLength'),
            "Length": (FreeCAD.Units.Quantity("50 mm"),
                       'App::PropertyLength'),
            "ShankDiameter": (FreeCAD.Units.Quantity("6 mm"),
                               'App::PropertyLength'),
            "TipDiameter": (FreeCAD.Units.Quantity("0.2 mm"),
                           'App::PropertyLength'),
        }
