# -*- coding: utf-8 -*-
# Defines the Vbit tool bit shape.

import FreeCAD
from .base import ToolBitShape


class ToolBitShapeVBit(ToolBitShape):
    _LABELS = {
        "CuttingEdgeAngle": "Cutting edge angle",
        "CuttingEdgeHeight": "Cutting edge height",
        "Diameter": "Diameter",
        "Length": "Overall Tool Length",
        "ShankDiameter": "Shank diameter",
        "TipDiameter": "Tip diameter",
    }

    def set_default_parameters(self):
        self._params = {
            "CuttingEdgeAngle": FreeCAD.Units.Quantity("60 deg"),
            "CuttingEdgeHeight": FreeCAD.Units.Quantity("10 mm"),
            "Diameter": FreeCAD.Units.Quantity("12 mm"),
            "Length": FreeCAD.Units.Quantity("50 mm"),
            "ShankDiameter": FreeCAD.Units.Quantity("6 mm"),
            "TipDiameter": FreeCAD.Units.Quantity("0.2 mm"),
        }
