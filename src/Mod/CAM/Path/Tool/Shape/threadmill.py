# -*- coding: utf-8 -*-
# Defines the ThreadMill tool bit shape.

import FreeCAD
from .base import ToolBitShape


class ToolBitShapeThreadMill(ToolBitShape):
    _LABELS = {
        "Crest": "Crest height",
        "Diameter": "Major diameter",
        "Length": "Overall Tool Length",
        "NeckDiameter": "Neck diameter",
        "NeckLength": "Neck length",
        "ShankDiameter": "Shank diameter",
        "CuttingAngle": "Cutting angle",
    }

    def set_default_parameters(self):
        self._params = {
            "Crest": FreeCAD.Units.Quantity("0.5 mm"),
            "Diameter": FreeCAD.Units.Quantity("8 mm"),
            "Length": FreeCAD.Units.Quantity("60 mm"),
            "NeckDiameter": FreeCAD.Units.Quantity("6 mm"),
            "NeckLength": FreeCAD.Units.Quantity("10 mm"),
            "ShankDiameter": FreeCAD.Units.Quantity("8 mm"),
            "CuttingAngle": FreeCAD.Units.Quantity("60 deg"),
        }
