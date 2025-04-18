# -*- coding: utf-8 -*-
# Defines the Dovetail tool bit shape.

import FreeCAD
from .base import ToolBitShape


class ToolBitShapeDovetail(ToolBitShape):
    _LABELS = {
        "Crest": "Crest height",
        "CuttingAngle": "Cutting angle",
        "Diameter": "Major diameter",
        "DovetailHeight": "Dovetail height",
        "Length": "Overall Tool Length",
        "NeckDiameter": "Neck diameter",
        "NeckLength": "Neck length",
        "ShankDiameter": "Shank diameter",
    }

    def set_default_parameters(self):
        self._params = {
            "Crest": FreeCAD.Units.Quantity("1 mm"),
            "CuttingAngle": FreeCAD.Units.Quantity("45 deg"),
            "Diameter": FreeCAD.Units.Quantity("10 mm"),
            "DovetailHeight": FreeCAD.Units.Quantity("5 mm"),
            "Length": FreeCAD.Units.Quantity("60 mm"),
            "NeckDiameter": FreeCAD.Units.Quantity("6 mm"),
            "NeckLength": FreeCAD.Units.Quantity("10 mm"),
            "ShankDiameter": FreeCAD.Units.Quantity("10 mm"),
        }
