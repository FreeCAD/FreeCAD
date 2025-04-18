# -*- coding: utf-8 -*-
# Defines the BallEnd tool bit shape.

import FreeCAD
from .base import ToolBitShape


class ToolBitShapeBallEnd(ToolBitShape):
    _LABELS = {
        "CuttingEdgeHeight": "Cutting edge height",
        "Diameter": "Diameter",
        "Length": "Overall Tool Length",
        "ShankDiameter": "Shank diameter",
    }

    def set_default_parameters(self):
        self._params = {
            "CuttingEdgeHeight": FreeCAD.Units.Quantity("10 mm"),
            "Diameter": FreeCAD.Units.Quantity("6 mm"),
            "Length": FreeCAD.Units.Quantity("50 mm"),
            "ShankDiameter": FreeCAD.Units.Quantity("6 mm"),
        }
