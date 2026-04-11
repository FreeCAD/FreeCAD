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

"""The BIM Clone command"""

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP


class BIM_ResetCloneColors:

    def GetResources(self):
        return {
            "Pixmap": "BIM_ResetCloneColors",
            "MenuText": QT_TRANSLATE_NOOP("BIM_ResetCloneColors", "Reset Colors"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_ResetCloneColors",
                "Resets the colors of this object from its cloned original",
            ),
            "Accel": "D,O",
        }

    def Activated(self):
        for obj in FreeCADGui.Selection.getSelection():
            if hasattr(obj, "CloneOf") and obj.CloneOf:
                obj.ViewObject.DiffuseColor = obj.CloneOf.ViewObject.DiffuseColor


FreeCADGui.addCommand("BIM_ResetCloneColors", BIM_ResetCloneColors())
