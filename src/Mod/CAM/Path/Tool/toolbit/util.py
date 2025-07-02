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


def to_json(value):
    """Convert a value to JSON format."""
    if isinstance(value, FreeCAD.Units.Quantity):
        return str(value)
    return value


def format_value(value: FreeCAD.Units.Quantity | int | float | None, precision: int | None = None):
    if value is None:
        return None
    elif isinstance(value, FreeCAD.Units.Quantity):
        if precision is not None:
            # Format the value with the specified number of precision and strip trailing zeros
            formatted_value = f"{value.Value:.{precision}f}".rstrip("0").rstrip(".")
            unit = value.getUserPreferred()[2]
            return f"{formatted_value} {unit}"
        return value.UserString
    return str(value)
