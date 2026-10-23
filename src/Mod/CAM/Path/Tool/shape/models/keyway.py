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
from typing import Any, Tuple, Mapping
from .base import ToolBitShape


def _mm(value) -> float:
    """A parameter's magnitude, whether it arrives as a Quantity or a number."""
    return float(value.Value if hasattr(value, "Value") else value)


class ToolBitShapeKeyway(ToolBitShape):
    """
    A keyway (keyseat, woodruff) cutter: a disc head on a neck, then the shank.
    The head cuts, and so may the part of the neck just above it that is fluted.
    """

    name = "Keyway"

    @classmethod
    def schema(cls) -> Mapping[str, Tuple[str, str]]:
        return {
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
            "LowerCuttingHeight": (
                FreeCAD.Qt.translate("ToolBitShape", "Lower cutting height"),
                "App::PropertyLength",
            ),
            "NeckCuttingHeight": (
                FreeCAD.Qt.translate("ToolBitShape", "Neck cutting height"),
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
        The tool cuts up through the head and on into the fluted part of the
        neck, so its cutting edge height is the two together.
        """
        return {
            "CuttingEdgeHeight": lambda p: _mm(p["LowerCuttingHeight"])
            + _mm(p["NeckCuttingHeight"])
        }

    @property
    def label(self) -> str:
        return FreeCAD.Qt.translate("ToolBitShape", "Keyway")
