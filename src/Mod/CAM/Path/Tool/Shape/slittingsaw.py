# -*- coding: utf-8 -*-
# Defines the SlittingSaw tool bit shape.

import FreeCAD
from typing import Tuple
from .base import ToolBitShape


class ToolBitShapeSlittingSaw(ToolBitShape):
    aliases: Tuple[str, ...] = "slitting-saw", "slittingsaw"
    _LABELS = {
        "BladeThickness": "Blade thickness",
        "CapDiameter": "Cap diameter",
        "CapHeight": "Cap height",
        "Diameter": "Diameter",
        "Length": "Overall Tool Length",
        "ShankDiameter": "Shank diameter", # Arbor diameter?
    }

    @property
    def label(self) -> str:
        return "Slitting Saw"  # TODO translatable

    def set_default_parameters(self):
        self._params = {
            "BladeThickness": (FreeCAD.Units.Quantity("1 mm"),
                               'App::PropertyLength'),
            "CapDiameter": (FreeCAD.Units.Quantity("20 mm"),
                            'App::PropertyLength'),
            "CapHeight": (FreeCAD.Units.Quantity("5 mm"),
                          'App::PropertyLength'),
            "Diameter": (FreeCAD.Units.Quantity("50 mm"),
                         'App::PropertyLength'),
            "Length": (FreeCAD.Units.Quantity("30 mm"),
                       'App::PropertyLength'),
            "ShankDiameter": (FreeCAD.Units.Quantity("16 mm"),
                               'App::PropertyLength'),
        }
