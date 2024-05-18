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

"""BIM Panel-related Arch_"""


import os
import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate
PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")


class Arch_Pipe:

    "the Arch Pipe command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Pipe',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Pipe","Pipe"),
                'Accel': "P, I",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Pipe","Creates a pipe object from a given Wire or Line")}

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None

    def Activated(self):

        s = FreeCADGui.Selection.getSelection()
        if s:
            for obj in s:
                if hasattr(obj,'Shape'):
                    if len(obj.Shape.Wires) == 1:
                        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Pipe"))
                        FreeCADGui.addModule("Arch")
                        FreeCADGui.addModule("Draft")
                        FreeCADGui.doCommand("obj = Arch.makePipe(FreeCAD.ActiveDocument."+obj.Name+")")
                        FreeCADGui.doCommand("Draft.autogroup(obj)")
                        FreeCAD.ActiveDocument.commitTransaction()
        else:
            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Pipe"))
            FreeCADGui.addModule("Arch")
            FreeCADGui.addModule("Draft")
            FreeCADGui.doCommand("obj = Arch.makePipe()")
            FreeCADGui.doCommand("Draft.autogroup(obj)")
            FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


class Arch_PipeConnector:

    "the Arch Pipe command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_PipeConnector',
                'MenuText': QT_TRANSLATE_NOOP("Arch_PipeConnector","Connector"),
                'Accel': "P, C",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_PipeConnector","Creates a connector between 2 or 3 selected pipes")}

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None

    def Activated(self):

        import Draft
        s = FreeCADGui.Selection.getSelection()
        if not (len(s) in [2,3]):
            FreeCAD.Console.PrintError(translate("Arch","Please select exactly 2 or 3 Pipe objects")+"\n")
            return
        o = "["
        for obj in s:
            if Draft.getType(obj) != "Pipe":
                FreeCAD.Console.PrintError(translate("Arch","Please select only Pipe objects")+"\n")
                return
            o += "FreeCAD.ActiveDocument."+obj.Name+","
        o += "]"
        FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Connector"))
        FreeCADGui.addModule("Arch")
        FreeCADGui.addModule("Draft")
        FreeCADGui.doCommand("obj = Arch.makePipeConnector("+o+")")
        FreeCADGui.doCommand("Draft.autogroup(obj)")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()
    

class Arch_PipeGroupCommand:

    def GetCommands(self):
        return tuple(['Arch_Pipe','Arch_PipeConnector'])
    def GetResources(self):
        return { 'MenuText': QT_TRANSLATE_NOOP("Arch_PipeTools",'Pipe tools'),
                 'ToolTip': QT_TRANSLATE_NOOP("Arch_PipeTools",'Pipe tools')
               }
    def IsActive(self):
        return not FreeCAD.ActiveDocument is None


FreeCADGui.addCommand('Arch_Pipe',Arch_Pipe())
FreeCADGui.addCommand('Arch_PipeConnector',Arch_PipeConnector())
FreeCADGui.addCommand('Arch_PipeTools', Arch_PipeGroupCommand())
