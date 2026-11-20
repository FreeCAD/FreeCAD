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
from PySide.QtCore import QT_TRANSLATE_NOOP


class CuttingToolMixin:
    """
    This is a interface class to indicate that the ToolBit can chip, i.e.
    it has a Chipload property.
    It is used to determine if the tool bit can be used for chip removal.
    """

    def _init_cutting_properties(self, obj):
        """Initialize cutting tool properties. Must be called explicitly after obj is created."""
        obj.addProperty(
            "App::PropertyLength",
            "Chipload",
            "Attributes",
            QT_TRANSLATE_NOOP("App::Property", "Chipload per tooth"),
        )
        obj.Chipload = FreeCAD.Units.Quantity("0.0 mm")
