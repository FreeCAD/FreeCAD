# -*- coding: utf-8 -*-
# Defines the EndMill tool bit shape.

import FreeCAD
from .base import ToolBitShape


class ToolBitShapeEndMill(ToolBitShape):
    _LABELS = {
        "CuttingEdgeHeight": "Cutting edge height",
        "Diameter": "Diameter",
        "Length": "Overall Tool Length",
        "ShankDiameter": "Shank diameter",
    }

    def set_default_parameters(self):
        self._params = {
            "CuttingEdgeHeight": FreeCAD.Units.Quantity("20 mm"),
            "Diameter": FreeCAD.Units.Quantity("10 mm"),
            "Length": FreeCAD.Units.Quantity("75 mm"),
            "ShankDiameter": FreeCAD.Units.Quantity("10 mm"),
        }
