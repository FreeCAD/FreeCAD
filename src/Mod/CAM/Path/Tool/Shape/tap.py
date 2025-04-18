# -*- coding: utf-8 -*-
# Defines the Tap tool bit shape.

import FreeCAD
from .base import ToolBitShape


class ToolBitShapeTap(ToolBitShape):
    _LABELS = {
        "CuttingEdgeLength": "Cutting edge length",
        "Diameter": "Tap Diameter",
        "Length": "Overall Length of Tap",
        "ShankDiameter": "Shank Diameter",
        "TipAngle": "Tip Angle",
    }

    def set_default_parameters(self):
        self._params = {
            "CuttingEdgeLength": FreeCAD.Units.Quantity("15 mm"),
            "Diameter": FreeCAD.Units.Quantity("6 mm"),
            "Length": FreeCAD.Units.Quantity("60 mm"),
            "ShankDiameter": FreeCAD.Units.Quantity("6 mm"),
            "TipAngle": FreeCAD.Units.Quantity("90 deg"),
        }
