# -*- coding: utf-8 -*-
# Defines the Probe tool bit shape.

import FreeCAD
from typing import Tuple, Mapping
from .base import ToolBitShape


class ToolBitShapeProbe(ToolBitShape):
    name = "Probe"
    aliases = ("probe",)

    @classmethod
    def schema(cls) -> Mapping[str, Tuple[str, str]]:
        return {
            "Diameter": (
                FreeCAD.Qt.translate("ToolBitShape", "Ball diameter"),
                "App::PropertyLength",
            ),
            "Length": (
                FreeCAD.Qt.translate("ToolBitShape", "Length of probe"),
                "App::PropertyLength",
            ),
            "ShaftDiameter": (
                FreeCAD.Qt.translate("ToolBitShape", "Shaft diameter"),
                "App::PropertyLength",
            ),
        }

    @property
    def label(self) -> str:
        return FreeCAD.Qt.translate("ToolBitShape", "Probe")
