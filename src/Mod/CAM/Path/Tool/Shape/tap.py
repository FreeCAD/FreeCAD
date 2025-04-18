# -*- coding: utf-8 -*-
# Defines the Tap tool bit shape.

import FreeCAD
from typing import Tuple
from .base import ToolBitShape


class ToolBitShapeTap(ToolBitShape):
    aliases: Tuple[str, ...] = ("tap",)
    _LABELS = {
        "CuttingEdgeLength": "Cutting edge length",
        "Diameter": "Tap Diameter",
        "Length": "Overall Length of Tap",
        "ShankDiameter": "Shank Diameter",
        "TipAngle": "Tip Angle",
    }

    @property
    def label(self) -> str:
        return "Tap"  # TODO translatable

    def set_default_parameters(self):
        self._params = {
            "CuttingEdgeLength": (FreeCAD.Units.Quantity("15 mm"),
                                  'App::PropertyLength'),
            "Diameter": (FreeCAD.Units.Quantity("6 mm"),
                         'App::PropertyLength'),
            "Length": (FreeCAD.Units.Quantity("60 mm"),
                       'App::PropertyLength'),
            "ShankDiameter": (FreeCAD.Units.Quantity("6 mm"),
                               'App::PropertyLength'),
            "TipAngle": (FreeCAD.Units.Quantity("90 deg"),
                         'App::PropertyAngle'),
        }
