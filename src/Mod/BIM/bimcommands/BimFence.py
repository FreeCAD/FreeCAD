# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 Yorik van Havre <yorik@uncreated.net>              *
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

"""BIM fence command"""


import os
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
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        if len(FreeCADGui.Selection.getSelection()) != 3:
            FreeCAD.Console.PrintError(translate('Arch Fence selection','Select a section, post and path in exactly this order to build a fence.')+"\n")
            return
        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Fence"))
        FreeCADGui.addModule("Arch")
        FreeCADGui.doCommand("section = FreeCADGui.Selection.getSelection()[0]")
        FreeCADGui.doCommand("post = FreeCADGui.Selection.getSelection()[1]")
        FreeCADGui.doCommand("path = FreeCADGui.Selection.getSelection()[2]")
        FreeCADGui.doCommand("Arch.makeFence(section, post, path)")
        FreeCAD.ActiveDocument.commitTransaction()


FreeCADGui.addCommand('Arch_Fence', Arch_Fence())
