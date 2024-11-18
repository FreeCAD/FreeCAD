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

"""BIM Schedule command"""


import os
import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate
PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")


class Arch_Stairs:

    "the Arch Stairs command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Stairs',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Stairs","Stairs"),
                'Accel': "S, R",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Stairs","Creates a flight of stairs")}

    def IsActive(self):

        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):

        import Draft
        from draftutils import params
        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Stairs"))
        FreeCADGui.addModule("Arch")
        if FreeCADGui.Selection.getSelection():
            n = []
            nStr = ""
            for obj in FreeCADGui.Selection.getSelection():
                if nStr != "":
                    nStr += ","
                nStr += "FreeCAD.ActiveDocument." + obj.Name
            FreeCADGui.doCommand("obj = Arch.makeStairs(baseobj=["+nStr+"])")
        else:
            FreeCADGui.doCommand("obj = Arch.makeStairs(steps="+str(params.get_param_arch("StairsSteps"))+")")
        FreeCADGui.addModule("Draft")
        FreeCADGui.doCommand("Draft.autogroup(obj)")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


FreeCADGui.addCommand('Arch_Stairs', Arch_Stairs())
