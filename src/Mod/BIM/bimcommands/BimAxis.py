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

"""The BIM Axis-related commands"""


import os
import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate
PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")


class Arch_Axis:

    "the Arch Axis command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Axis',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Axis","Axis"),
                'Accel': "A, X",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Axis","Creates a set of axes")}

    def Activated(self):

        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Axis"))
        FreeCADGui.addModule("Arch")

        FreeCADGui.doCommand("Arch.makeAxis()")
        FreeCAD.ActiveDocument.commitTransaction()

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None



class Arch_AxisSystem:

    "the Arch Axis System command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Axis_System',
                'MenuText': QT_TRANSLATE_NOOP("Arch_AxisSystem","Axis System"),
                'Accel': "X, S",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_AxisSystem","Creates an axis system from a set of axes")}

    def Activated(self):

        import Draft
        if FreeCADGui.Selection.getSelection():
            s = "["
            for o in FreeCADGui.Selection.getSelection():
                if Draft.getType(o) != "Axis":
                    FreeCAD.Console.PrintError(translate("Arch","Only axes must be selected")+"\n")
                    return
                s += "FreeCAD.ActiveDocument."+o.Name+","
            s += "]"
            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Axis System"))
            FreeCADGui.addModule("Arch")
            FreeCADGui.doCommand("Arch.makeAxisSystem("+s+")")
            FreeCAD.ActiveDocument.commitTransaction()
        else:
            FreeCAD.Console.PrintError(translate("Arch","Please select at least one axis")+"\n")

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None


class Arch_Grid:

    "the Arch Grid command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Grid',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Grid","Grid"),
                'Accel': "A, X",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Grid","Creates a customizable grid object")}

    def Activated(self):

        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Grid"))
        FreeCADGui.addModule("Arch")

        FreeCADGui.doCommand("Arch.makeGrid()")
        FreeCAD.ActiveDocument.commitTransaction()

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None


class Arch_AxisTools:

    """The Axis tools group command"""

    def GetCommands(self):
        return tuple(['Arch_Axis','Arch_AxisSystem','Arch_Grid'])

    def GetResources(self):
        return { 'MenuText': QT_TRANSLATE_NOOP("Arch_AxisTools",'Axis tools'),
                 'ToolTip': QT_TRANSLATE_NOOP("Arch_AxisTools",'Axis tools')
               }

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None



FreeCADGui.addCommand('Arch_Axis', Arch_Axis())
FreeCADGui.addCommand('Arch_AxisSystem', Arch_AxisSystem())
FreeCADGui.addCommand('Arch_Grid', Arch_Grid())
FreeCADGui.addCommand('Arch_AxisTools', Arch_AxisTools())
