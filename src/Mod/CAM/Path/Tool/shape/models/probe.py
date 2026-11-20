# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2025 Samuel Abels
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
from typing import Tuple, Mapping
from .base import ToolBitShape


class ToolBitShapeProbe(ToolBitShape):
    name = "Probe"

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
