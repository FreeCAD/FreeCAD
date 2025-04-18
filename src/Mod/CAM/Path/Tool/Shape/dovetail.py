# -*- coding: utf-8 -*-
# Defines the Dovetail tool bit shape.

import FreeCAD
from typing import Tuple
from .base import ToolBitShape


class ToolBitShapeDovetail(ToolBitShape):
    aliases: Tuple[str, ...] = ("dovetail",)
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

    @property
    def label(self) -> str:
        return "Dovetail"  # TODO translatable

    def set_default_parameters(self):
        self._params = {
            "Crest": (FreeCAD.Units.Quantity("1 mm"),
                       'App::PropertyLength'),
            "CuttingAngle": (FreeCAD.Units.Quantity("45 deg"),
                              'App::PropertyAngle'),
            "Diameter": (FreeCAD.Units.Quantity("10 mm"),
                         'App::PropertyLength'),
            "DovetailHeight": (FreeCAD.Units.Quantity("5 mm"),
                               'App::PropertyLength'),
            "Length": (FreeCAD.Units.Quantity("60 mm"),
                       'App::PropertyLength'),
            "NeckDiameter": (FreeCAD.Units.Quantity("6 mm"),
                              'App::PropertyLength'),
            "NeckLength": (FreeCAD.Units.Quantity("10 mm"),
                           'App::PropertyLength'),
            "ShankDiameter": (FreeCAD.Units.Quantity("10 mm"),
                               'App::PropertyLength'),
        }
