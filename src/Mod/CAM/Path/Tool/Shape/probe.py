# -*- coding: utf-8 -*-
# Defines the Probe tool bit shape.

import FreeCAD
from typing import Tuple
from .base import ToolBitShape


class ToolBitShapeProbe(ToolBitShape):
    aliases: Tuple[str, ...] = ("probe",)
    _LABELS = {
        "Diameter": "Ball diameter",
        "Length": "Length of probe",
        "ShankDiameter": "Shaft diameter", # Using ShankDiameter internally
    }

    @property
    def label(self) -> str:
        return "Probe"  # TODO translatable

    def set_default_parameters(self):
        self._params = {
            "Diameter": (FreeCAD.Units.Quantity("3 mm"),
                         'App::PropertyLength'),
            "Length": (FreeCAD.Units.Quantity("40 mm"),
                       'App::PropertyLength'),
            "ShankDiameter": (FreeCAD.Units.Quantity("6 mm"),
                               'App::PropertyLength'),
        }
