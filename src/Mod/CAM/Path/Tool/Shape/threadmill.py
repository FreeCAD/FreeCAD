# -*- coding: utf-8 -*-
# Defines the ThreadMill tool bit shape.

import FreeCAD
from typing import Tuple
from .base import ToolBitShape


class ToolBitShapeThreadMill(ToolBitShape):
    aliases: Tuple[str, ...] = "thread-mill", "threadmill"
    _LABELS = {
        "Crest": "Crest height",
        "Diameter": "Major diameter",
        "Length": "Overall Tool Length",
        "NeckDiameter": "Neck diameter",
        "NeckLength": "Neck length",
        "ShankDiameter": "Shank diameter",
        "CuttingAngle": "Cutting angle",
    }

    @property
    def label(self) -> str:
        return "Thread Mill"  # TODO translatable

    def set_default_parameters(self):
        self._params = {
            "Crest": (FreeCAD.Units.Quantity("0.5 mm"),
                       'App::PropertyLength'),
            "Diameter": (FreeCAD.Units.Quantity("8 mm"),
                         'App::PropertyLength'),
            "Length": (FreeCAD.Units.Quantity("60 mm"),
                       'App::PropertyLength'),
            "NeckDiameter": (FreeCAD.Units.Quantity("6 mm"),
                              'App::PropertyLength'),
            "NeckLength": (FreeCAD.Units.Quantity("10 mm"),
                           'App::PropertyLength'),
            "ShankDiameter": (FreeCAD.Units.Quantity("8 mm"),
                               'App::PropertyLength'),
            "CuttingAngle": (FreeCAD.Units.Quantity("60 deg"),
                              'App::PropertyAngle'),
        }
