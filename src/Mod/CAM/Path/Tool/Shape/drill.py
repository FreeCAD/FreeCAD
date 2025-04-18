# -*- coding: utf-8 -*-
# Defines the Drill tool bit shape.

import FreeCAD
from typing import Tuple
from .base import ToolBitShape


class ToolBitShapeDrill(ToolBitShape):
    aliases: Tuple[str, ...] = ("drill",)
    _LABELS = {
        "Diameter": "Diameter",
        "Length": "Overall Tool Length",
        "TipAngle": "Tip angle",
    }

    @property
    def label(self) -> str:
        return "Drill"  # TODO translatable

    def set_default_parameters(self):
        self._params = {
            "Diameter": (FreeCAD.Units.Quantity("5 mm"),
                         'App::PropertyLength'),
            "Length": (FreeCAD.Units.Quantity("70 mm"),
                       'App::PropertyLength'),
            "TipAngle": (FreeCAD.Units.Quantity("118 deg"),
                         'App::PropertyAngle'),
        }
