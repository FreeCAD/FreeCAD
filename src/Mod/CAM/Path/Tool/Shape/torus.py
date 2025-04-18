# -*- coding: utf-8 -*-
# Defines the Torus tool bit shape.

import FreeCAD
from typing import Tuple
from .base import ToolBitShape


class ToolBitShapeTorus(ToolBitShape):
    aliases: Tuple[str, ...] = ("torus",)
    _LABELS = {
        "CuttingEdgeHeight": "Cutting edge height",
        "Diameter": "Diameter",
        "Length": "Overall Tool Length",
        "ShankDiameter": "Shank diameter",
        "TorusRadius": "Torus radius",
    }

    @property
    def label(self) -> str:
        return "Torus"  # TODO translatable

    def set_default_parameters(self):
        self._params = {
            "CuttingEdgeHeight": (FreeCAD.Units.Quantity("15 mm"),
                                  'App::PropertyLength'),
            "Diameter": (FreeCAD.Units.Quantity("10 mm"),
                         'App::PropertyLength'),
            "Length": (FreeCAD.Units.Quantity("70 mm"),
                       'App::PropertyLength'),
            "ShankDiameter": (FreeCAD.Units.Quantity("10 mm"),
                               'App::PropertyLength'),
            "TorusRadius": (FreeCAD.Units.Quantity("1 mm"),
                            'App::PropertyLength'),
        }
