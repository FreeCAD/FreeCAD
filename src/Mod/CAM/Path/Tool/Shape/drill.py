# -*- coding: utf-8 -*-
# Defines the Drill tool bit shape.

import FreeCAD
from typing import Tuple
from .base import ToolBitShape


class ToolBitShapeDrill(ToolBitShape):
    name = "drill"
    _schema = {
        "Diameter": 'App::PropertyLength',
        "Length": 'App::PropertyLength',
        "TipAngle": 'App::PropertyAngle',
    }

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self._labels = {
            "Diameter": FreeCAD.Qt.translate(
                "ToolBitShapeDrill", "Diameter"),
            "Length": FreeCAD.Qt.translate(
                "ToolBitShapeDrill", "Overall Tool Length"),
            "TipAngle": FreeCAD.Qt.translate(
                "ToolBitShapeDrill", "Tip angle"),
        }

    @property
    def label(self) -> str:
        return FreeCAD.Qt.translate("ToolBitShapeDrill", "Drill")
