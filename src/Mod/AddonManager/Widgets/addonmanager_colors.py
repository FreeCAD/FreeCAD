# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022-2024 FreeCAD Project Association                   *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

from enum import Enum, auto

import FreeCADGui
from PySide import QtGui


def is_darkmode() -> bool:
    """Heuristics to determine if we are in a darkmode stylesheet"""
    pl = FreeCADGui.getMainWindow().palette()
    return pl.color(QtGui.QPalette.Window).lightness() < 128


def warning_color_string() -> str:
    """A shade of red, adapted to darkmode if possible. Targets a minimum 7:1 contrast ratio."""
    return "rgb(255,105,97)" if is_darkmode() else "rgb(215,0,21)"


def bright_color_string() -> str:
    """A shade of green, adapted to darkmode if possible. Targets a minimum 7:1 contrast ratio."""
    return "rgb(48,219,91)" if is_darkmode() else "rgb(36,138,61)"


def attention_color_string() -> str:
    """A shade of orange, adapted to darkmode if possible. Targets a minimum 7:1 contrast ratio."""
    return "rgb(255,179,64)" if is_darkmode() else "rgb(255,149,0)"
