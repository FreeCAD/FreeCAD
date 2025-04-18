# -*- coding: utf-8 -*-
# Defines the Chamfer tool bit shape.

import FreeCAD
from typing import Tuple
from .base import ToolBitShape


class ToolBitShapeChamfer(ToolBitShape):
    aliases: Tuple[str, ...] = ("chamfer",)
    _LABELS = {
        "CuttingEdgeHeight": "Cutting edge height",
        "Diameter": "Diameter",
        "Length": "Overall Tool Length",
        "Radius": "Radius",
        "ShankDiameter": "Shank diameter",
    }

    @property
    def label(self) -> str:
        return "Chamfer"  # TODO translatable

    def set_default_parameters(self):
        self._params = {
            "CuttingEdgeHeight": (FreeCAD.Units.Quantity("5 mm"),
                                  'App::PropertyLength'),
            "Diameter": (FreeCAD.Units.Quantity("6 mm"),
                         'App::PropertyLength'),
            "Length": (FreeCAD.Units.Quantity("50 mm"),
                       'App::PropertyLength'),
            "Radius": (FreeCAD.Units.Quantity("1 mm"),
                       'App::PropertyLength'),
            "ShankDiameter": (FreeCAD.Units.Quantity("6 mm"),
                              'App::PropertyLength'),
        }
