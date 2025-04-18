# -*- coding: utf-8 -*-
# Defines the Probe tool bit shape.

import FreeCAD
from .base import ToolBitShape


class ToolBitShapeProbe(ToolBitShape):
    _LABELS = {
        "Diameter": "Ball diameter",
        "Length": "Length of probe",
        "ShankDiameter": "Shaft diameter", # Using ShankDiameter internally
    }

    def set_default_parameters(self):
        self._params = {
            "Diameter": FreeCAD.Units.Quantity("3 mm"),
            "Length": FreeCAD.Units.Quantity("40 mm"),
            "ShankDiameter": FreeCAD.Units.Quantity("6 mm"),
        }
