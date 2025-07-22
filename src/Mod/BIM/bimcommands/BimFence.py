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

"""BIM fence command"""

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate

PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")


class Arch_Fence:
    "the Arch Fence command definition"

    def GetResources(self):
        return {'Pixmap': 'Arch_Fence',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Fence", "Fence"),
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Fence", "Creates a fence object from a selected section, post and path")}

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):
        if len(FreeCADGui.Selection.getSelection()) != 3:
            FreeCAD.Console.PrintError(translate('Arch Fence selection','Select a section, post and path in exactly this order to build a fence.')+"\n")
            return
        doc = FreeCAD.ActiveDocument
        doc.openTransaction(translate("Arch","Create Fence"))
        FreeCADGui.addModule("Arch")
        FreeCADGui.doCommand("section = FreeCADGui.Selection.getSelection()[0]")
        FreeCADGui.doCommand("post = FreeCADGui.Selection.getSelection()[1]")
        FreeCADGui.doCommand("path = FreeCADGui.Selection.getSelection()[2]")
        FreeCADGui.doCommand("Arch.makeFence(section, post, path)")
        doc.commitTransaction()
        doc.recompute()

FreeCADGui.addCommand('Arch_Fence', Arch_Fence())
