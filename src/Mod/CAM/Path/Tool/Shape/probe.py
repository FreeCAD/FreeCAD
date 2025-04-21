# -*- coding: utf-8 -*-
# Defines the Probe tool bit shape.

import FreeCAD
from .base import ToolBitShape


class ToolBitShapeProbe(ToolBitShape):
    name = "probe"
    _schema = {
        "Diameter": 'App::PropertyLength',
        "Length": 'App::PropertyLength',
        "ShaftDiameter": 'App::PropertyLength',
    }

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self._labels = {
            "Diameter": FreeCAD.Qt.translate(
                "ToolBitShapeProbe", "Ball diameter"),
            "Length": FreeCAD.Qt.translate(
                "ToolBitShapeProbe", "Length of probe"),
            "ShaftDiameter": FreeCAD.Qt.translate(
                "ToolBitShapeProbe", "Shaft diameter"),
        }

    @property
    def label(self) -> str:
        return FreeCAD.Qt.translate("ToolBitShapeProbe", "Probe")

    def can_rotate(self) -> bool:
        return False