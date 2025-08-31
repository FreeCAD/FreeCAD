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

"""The BIM Ungroup command"""

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP


class BIM_Ungroup:

    def GetResources(self):
        return {
            "Pixmap": "Draft_AddToGroup",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Convert", "Remove From Group"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Convert", "Removes this object from its parent group"
            ),
        }

    def Activated(self):
        sel = FreeCADGui.Selection.getSelection()
        first = True
        if sel:
            for obj in sel:
                for parent in obj.InList:
                    if parent.isDerivedFrom(
                        "App::DocumentObjectGroup"
                    ) or parent.hasExtension("App::GroupExtension"):
                        if obj in parent.Group:
                            if first:
                                FreeCAD.ActiveDocument.openTransaction("Ungroup")
                                first = False
                            if hasattr(parent, "removeObject"):
                                parent.removeObject(obj)
                            else:
                                g = parent.Group
                                g.remove(obj)
                                parent.Group = g
        if not first:
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()


FreeCADGui.addCommand("BIM_Ungroup", BIM_Ungroup())
