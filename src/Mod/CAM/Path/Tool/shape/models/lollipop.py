# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Billy Huddleston <billy@ivdc.com>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

import FreeCAD
import math
from typing import Any, Tuple, Mapping
from .base import ToolBitShape


def _mm(value) -> float:
    """A parameter's magnitude, whether it arrives as a Quantity or a number."""
    return float(value.Value if hasattr(value, "Value") else value)


def _neck_junction(params) -> float:
    """Height above the tip where the neck meets the ball."""
    radius = _mm(params["Diameter"]) / 2
    neck_radius = _mm(params["NeckDiameter"]) / 2
    return radius + math.sqrt(max(radius * radius - neck_radius * neck_radius, 0.0))


class ToolBitShapeLollipop(ToolBitShape):
    """
    A lollipop (undercut) cutter: a ball on a narrower neck, then the shank.
    The ball cuts all the way round, up to where the neck meets it.
    """

    name = "Lollipop"

    @classmethod
    def schema(cls) -> Mapping[str, Tuple[str, str]]:
        return {
            "CuttingEdgeHeight": (
                FreeCAD.Qt.translate("ToolBitShape", "Cutting edge height"),
                "App::PropertyLength",
            ),
            "Diameter": (
                FreeCAD.Qt.translate("ToolBitShape", "Ball diameter"),
                "App::PropertyLength",
            ),
            "Flutes": (
                FreeCAD.Qt.translate("ToolBitShape", "Flutes"),
                "App::PropertyInteger",
            ),
            "Length": (
                FreeCAD.Qt.translate("ToolBitShape", "Overall tool length"),
                "App::PropertyLength",
            ),
            "NeckDiameter": (
                FreeCAD.Qt.translate("ToolBitShape", "Neck diameter"),
                "App::PropertyLength",
            ),
            "NeckHeight": (
                FreeCAD.Qt.translate("ToolBitShape", "Neck length"),
                "App::PropertyLength",
            ),
            "ShankDiameter": (
                FreeCAD.Qt.translate("ToolBitShape", "Shank diameter"),
                "App::PropertyLength",
            ),
        }

    @classmethod
    def derived_parameters(cls) -> Mapping[str, Any]:
        """
        The ball cuts up to where the neck leaves it, above its equator, so the
        cutting edge height follows from the ball and neck diameters.
        """
        return {"CuttingEdgeHeight": _neck_junction}

    @property
    def label(self) -> str:
        return FreeCAD.Qt.translate("ToolBitShape", "Lollipop")
