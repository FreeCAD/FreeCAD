# -*- coding: utf-8 -*-
# Defines the Reamer tool bit shape.

import FreeCAD
from .base import ToolBitShape


class ToolBitShapeReamer(ToolBitShape):
    _LABELS = {
        "CuttingEdgeHeight": "Cutting edge height",
        "Diameter": "Diameter",
        "Length": "Overall Tool Length",
        "ShankDiameter": "Shank diameter",
    }

    def set_default_parameters(self):
        self._params = {
            "CuttingEdgeHeight": FreeCAD.Units.Quantity("25 mm"),
            "Diameter": FreeCAD.Units.Quantity("8 mm"),
            "Length": FreeCAD.Units.Quantity("80 mm"),
            "ShankDiameter": FreeCAD.Units.Quantity("8 mm"),
        }
