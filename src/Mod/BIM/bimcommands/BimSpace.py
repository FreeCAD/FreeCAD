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

"""BIM Schedule command"""

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate

PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")


class Arch_Space:

    "the Arch Space command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Space',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Space","Space"),
                'Accel': "S, P",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Space","Creates a space object from selected boundary objects")}

    def IsActive(self):

        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):

        import ArchComponent
        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Space"))
        FreeCADGui.addModule("Arch")
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            FreeCADGui.Control.closeDialog()
            FreeCADGui.doCommand("obj = Arch.makeSpace(FreeCADGui.Selection.getSelectionEx())")
            FreeCADGui.addModule("Draft")
            FreeCADGui.doCommand("Draft.autogroup(obj)")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
        else:
            FreeCAD.Console.PrintMessage(translate("Arch","Please select a base object")+"\n")
            FreeCADGui.Control.showDialog(ArchComponent.SelectionTaskPanel())
            FreeCAD.ArchObserver = ArchComponent.ArchSelectionObserver(nextCommand="Arch_Space")
            FreeCADGui.Selection.addObserver(FreeCAD.ArchObserver)


FreeCADGui.addCommand('Arch_Space', Arch_Space())
