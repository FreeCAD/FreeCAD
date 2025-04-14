# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 Yorik van Havre <yorik@uncreated.net>              *
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

"""The BIM Door command"""

import FreeCAD
import FreeCADGui
from bimcommands import BimWindow

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP


class BIM_Door(BimWindow.Arch_Window):

    def __init__(self):
        super().__init__()
        self.doormode = True

    def GetResources(self):
        return {
            "Pixmap": "BIM_Door",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Door", "Door"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Door", "Places a door at a given location"
            ),
            "Accel": "D,O",
        }


FreeCADGui.addCommand("BIM_Door", BIM_Door())
