# -*- coding: utf-8 -*-
# Defines the ThreadMill tool bit shape.

import FreeCAD
from typing import Tuple
from .base import ToolBitShape


class ToolBitShapeThreadMill(ToolBitShape):
    name = "thread-mill"
    aliases: Tuple[str, ...] = ("threadmill",)
    _schema = {
        "Crest": 'App::PropertyLength',
        "Diameter": 'App::PropertyLength',
        "Length": 'App::PropertyLength',
        "NeckDiameter": 'App::PropertyLength',
        "NeckLength": 'App::PropertyLength',
        "ShankDiameter": 'App::PropertyLength',
        "cuttingAngle": 'App::PropertyAngle',  # TODO rename to CuttingAngle
    }

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self._labels = {
            "Crest": FreeCAD.Qt.translate(
                "ToolBitShapeThreadMill", "Crest height"),
            "Diameter": FreeCAD.Qt.translate(
                "ToolBitShapeThreadMill", "Major diameter"),
            "Length": FreeCAD.Qt.translate(
                "ToolBitShapeThreadMill", "Overall Tool Length"),
            "NeckDiameter": FreeCAD.Qt.translate(
                "ToolBitShapeThreadMill", "Neck diameter"),
            "NeckLength": FreeCAD.Qt.translate(
                "ToolBitShapeThreadMill", "Neck length"),
            "ShankDiameter": FreeCAD.Qt.translate(
                "ToolBitShapeThreadMill", "Shank diameter"),
            "cuttingAngle": FreeCAD.Qt.translate(
                "ToolBitShapeThreadMill", "Cutting angle"),
        }

    @property
    def label(self) -> str:
        return FreeCAD.Qt.translate("ToolBitShapeThreadMill", "Thread Mill")
