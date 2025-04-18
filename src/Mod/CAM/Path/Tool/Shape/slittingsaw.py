# -*- coding: utf-8 -*-
# Defines the SlittingSaw tool bit shape.

import FreeCAD
from .base import ToolBitShape


class ToolBitShapeSlittingSaw(ToolBitShape):
    _LABELS = {
        "BladeThickness": "Blade thickness",
        "CapDiameter": "Cap diameter",
        "CapHeight": "Cap height",
        "Diameter": "Diameter",
        "Length": "Overall Tool Length",
        "ShankDiameter": "Shank diameter", # Arbor diameter?
    }

    def set_default_parameters(self):
        self._params = {
            "BladeThickness": FreeCAD.Units.Quantity("1 mm"),
            "CapDiameter": FreeCAD.Units.Quantity("20 mm"),
            "CapHeight": FreeCAD.Units.Quantity("5 mm"),
            "Diameter": FreeCAD.Units.Quantity("50 mm"),
            "Length": FreeCAD.Units.Quantity("30 mm"),
            "ShankDiameter": FreeCAD.Units.Quantity("16 mm"),
        }
