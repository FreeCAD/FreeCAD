# -*- coding: utf-8 -*-
# Defines the Dovetail tool bit shape.

import FreeCAD
from typing import Tuple
from .base import ToolBitShape


class ToolBitShapeDovetail(ToolBitShape):
    name = "dovetail"
    _schema = {
        "TipDiameter": 'App::PropertyLength',
        "CuttingEdgeAngle": 'App::PropertyAngle',
        "CuttingEdgeHeight": 'App::PropertyLength',
        "Diameter": 'App::PropertyLength',
        "Length": 'App::PropertyLength',
        "NeckDiameter": 'App::PropertyLength',
        "NeckHeight": 'App::PropertyLength',
        "ShankDiameter": 'App::PropertyLength',
    }

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self._labels = {
            "Crest": FreeCAD.Qt.translate(
                "ToolBitShapeDovetail", "Crest height"),
            "CuttingEdgeAngle": FreeCAD.Qt.translate(
                "ToolBitShapeDovetail", "Cutting angle"),
            "Diameter": FreeCAD.Qt.translate(
                "ToolBitShapeDovetail", "Major diameter"),
            "DovetailHeight": FreeCAD.Qt.translate(
                "ToolBitShapeDovetail", "Dovetail height"),
            "Length": FreeCAD.Qt.translate(
                "ToolBitShapeDovetail", "Overall Tool Length"),
            "NeckDiameter": FreeCAD.Qt.translate(
                "ToolBitShapeDovetail", "Neck diameter"),
            "NeckHeight": FreeCAD.Qt.translate(
                "ToolBitShapeDovetail", "Neck length"),
            "ShankDiameter": FreeCAD.Qt.translate(
                "ToolBitShapeDovetail", "Shank diameter"),
        }

    @property
    def label(self) -> str:
        return FreeCAD.Qt.translate("ToolBitShapeDovetail", "Dovetail")
