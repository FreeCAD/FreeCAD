# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2025 Samuel Abels <knipknap@gmail.com>                  *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

import FreeCAD
import math
from typing import Any, Tuple, Mapping
from .base import ToolBitShape


def _mm(value) -> float:
    """A parameter's magnitude, whether it arrives as a Quantity or a number."""
    return float(value.Value if hasattr(value, "Value") else value)


class ToolBitShapeChamfer(ToolBitShape):
    name = "Chamfer"

    @classmethod
    def schema(cls) -> Mapping[str, Tuple[str, str]]:
        return {
            "CuttingEdgeAngle": (
                FreeCAD.Qt.translate("ToolBitShape", "Cutting edge angle"),
                "App::PropertyAngle",
            ),
            "CuttingEdgeHeight": (
                FreeCAD.Qt.translate("ToolBitShape", "Cutting edge height"),
                "App::PropertyLength",
            ),
            "Diameter": (
                FreeCAD.Qt.translate("ToolBitShape", "Diameter"),
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
            "ShankDiameter": (
                FreeCAD.Qt.translate("ToolBitShape", "Shank diameter"),
                "App::PropertyLength",
            ),
            "TipDiameter": (
                FreeCAD.Qt.translate("ToolBitShape", "Tip diameter"),
                "App::PropertyLength",
            ),
        }

    @classmethod
    def derived_parameters(cls) -> Mapping[str, Any]:
        """
        The cutting diameter is where the cone reaches the top of the cutting
        edge, so it follows from the tip, the angle and the height rather than
        being something to type in. The sketch is constrained by those three and
        never by Diameter, so a hand-entered value silently disagrees with the
        tool it claims to describe.
        """
        return {
            "Diameter": lambda p: _mm(p["TipDiameter"])
            + 2
            * _mm(p["CuttingEdgeHeight"])
            * math.tan(math.radians(_mm(p["CuttingEdgeAngle"]) / 2))
        }

    @property
    def label(self) -> str:
        return FreeCAD.Qt.translate("ToolBitShape", "Chamfer")
