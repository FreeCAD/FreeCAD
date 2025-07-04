# -*- coding: utf-8 -*-
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
from typing import Tuple, Mapping
from .base import ToolBitShape


class ToolBitShapeIso(ToolBitShape):
    name = "IsoShape"
    aliases = "isoshape", "Isoshape", "IsoShape",

    @classmethod
    def schema(cls) -> Mapping[str, Tuple[str, str]]:
        return {
            "EdgeLength": (
                FreeCAD.Qt.translate("ToolBitShape", "Cutting edge length"),
                "App::PropertyLength",
            ),
            "Rotation": (
                FreeCAD.Qt.translate("ToolBitShape", "Rotation angle"),
                "App::PropertyAngle",
            ),
            "TipAngle": (
                FreeCAD.Qt.translate("ToolBitShape", "Tip angle"),
                "App::PropertyAngle",
            ),
            "TipRadius": (
                FreeCAD.Qt.translate("ToolBitShape", "Tip radius"),
                "App::PropertyLength",
            )}

    @property
    def label(self) -> str:
        return FreeCAD.Qt.translate("ToolBitShape", "IsoShape")
