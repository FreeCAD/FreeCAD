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


class RotaryToolBitMixin:
    """
    Mixin class for rotary tool bits.
    Provides methods for accessing diameter and length from the shape.
    """

    def can_rotate(self) -> bool:
        return True

    def get_diameter(self) -> FreeCAD.Units.Quantity:
        """
        Get the diameter of the rotary tool bit from the shape.
        """
        return self.obj.Diameter

    def set_diameter(self, diameter: FreeCAD.Units.Quantity):
        """
        Set the diameter of the rotary tool bit on the shape.
        """
        if not isinstance(diameter, FreeCAD.Units.Quantity):
            raise ValueError("Diameter must be a FreeCAD Units.Quantity")
        self.obj.Diameter = diameter

    def get_length(self) -> FreeCAD.Units.Quantity:
        """
        Get the length of the rotary tool bit from the shape.
        """
        return self.obj.Length

    def set_length(self, length: FreeCAD.Units.Quantity):
        """
        Set the length of the rotary tool bit on the shape.
        """
        if not isinstance(length, FreeCAD.Units.Quantity):
            raise ValueError("Length must be a FreeCAD Units.Quantity")
        self.obj.Length = length
