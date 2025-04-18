# -*- coding: utf-8 -*-
# Defines the Chamfer tool bit shape.

import FreeCAD
from .base import ToolBitShape


class ToolBitShapeChamfer(ToolBitShape):
    _LABELS = {
        "CuttingEdgeHeight": "Cutting edge height",
        "Diameter": "Diameter",
        "Length": "Overall Tool Length",
        "Radius": "Radius",
        "ShankDiameter": "Shank diameter",
    }

    def set_default_parameters(self):
        self._params = {
            "CuttingEdgeHeight": FreeCAD.Units.Quantity("5 mm"),
            "Diameter": FreeCAD.Units.Quantity("6 mm"),
            "Length": FreeCAD.Units.Quantity("50 mm"),
            "Radius": FreeCAD.Units.Quantity("1 mm"),
            "ShankDiameter": FreeCAD.Units.Quantity("6 mm"),
        }
