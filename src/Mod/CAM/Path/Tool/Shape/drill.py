# -*- coding: utf-8 -*-
# Defines the Drill tool bit shape.

import FreeCAD
from .base import ToolBitShape


class ToolBitShapeDrill(ToolBitShape):
    _LABELS = {
        "Diameter": "Diameter",
        "Length": "Overall Tool Length",
        "TipAngle": "Tip angle",
    }

    def set_default_parameters(self):
        self._params = {
            "Diameter": FreeCAD.Units.Quantity("5 mm"),
            "Length": FreeCAD.Units.Quantity("70 mm"),
            "TipAngle": FreeCAD.Units.Quantity("118 deg"),
        }
