# -*- coding: utf-8 -*-
# Defines the Torus tool bit shape.

import FreeCAD
from .base import ToolBitShape


class ToolBitShapeTorus(ToolBitShape):
    _LABELS = {
        "CuttingEdgeHeight": "Cutting edge height",
        "Diameter": "Diameter",
        "Length": "Overall Tool Length",
        "ShankDiameter": "Shank diameter",
        "TorusRadius": "Torus radius",
    }

    def set_default_parameters(self):
        self._params = {
            "CuttingEdgeHeight": FreeCAD.Units.Quantity("15 mm"),
            "Diameter": FreeCAD.Units.Quantity("10 mm"),
            "Length": FreeCAD.Units.Quantity("70 mm"),
            "ShankDiameter": FreeCAD.Units.Quantity("10 mm"),
            "TorusRadius": FreeCAD.Units.Quantity("1 mm"),
        }
