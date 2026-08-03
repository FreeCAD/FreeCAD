# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 Yorik van Havre <yorik@uncreated.net>              *
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

"""BIM equipment commands"""

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate

PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")


class Arch_Equipment:
    "the Arch Equipment command definition"

    def GetResources(self):

        return {
            "Pixmap": "Arch_Equipment",
            "MenuText": QT_TRANSLATE_NOOP("Arch_Equipment", "Equipment"),
            "Accel": "E, Q",
            "ToolTip": QT_TRANSLATE_NOOP(
                "Arch_Equipment", "Creates an equipment from a selected object (Part or Mesh)"
            ),
        }

    def IsActive(self):

        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):

        s = FreeCADGui.Selection.getSelection()
        if not s:
            FreeCAD.Console.PrintError(
                translate("Arch", "Select a base shape object and optionally a mesh object")
            )
        else:
            base = ""
            mesh = ""
            if len(s) == 2:
                if hasattr(s[0], "Shape"):
                    base = s[0].Name
                elif s[0].isDerivedFrom("Mesh::Feature"):
                    mesh = s[0].Name
                if hasattr(s[1], "Shape"):
                    if mesh:
                        base = s[1].Name
                elif s[1].isDerivedFrom("Mesh::Feature"):
                    if base:
                        mesh = s[1].Name
            else:
                if hasattr(s[0], "Shape"):
                    base = s[0].Name
                elif s[0].isDerivedFrom("Mesh::Feature"):
                    mesh = s[0].Name
            FreeCAD.ActiveDocument.openTransaction(str(translate("Arch", "Create Equipment")))
            FreeCADGui.addModule("Arch")
            if base:
                base = "FreeCAD.ActiveDocument." + base
            FreeCADGui.doCommand("obj = Arch.makeEquipment(" + base + ")")
            if mesh:
                FreeCADGui.doCommand("obj.HiRes = FreeCAD.ActiveDocument." + mesh)
            FreeCADGui.addModule("Draft")
            FreeCADGui.doCommand("Draft.autogroup(obj)")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
            # get diffuse color info from base object
            if base and hasattr(s[0].ViewObject, "DiffuseColor"):
                FreeCADGui.doCommand(
                    "FreeCAD.ActiveDocument.Objects[-1].ViewObject.DiffuseColor = "
                    + base
                    + ".ViewObject.DiffuseColor"
                )
        return


FreeCADGui.addCommand("Arch_Equipment", Arch_Equipment())
