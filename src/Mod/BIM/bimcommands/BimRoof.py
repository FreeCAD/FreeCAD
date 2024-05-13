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

"""BIM Roof command"""


import os
import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate
PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")


class Arch_Roof:
    '''the Arch Roof command definition'''

    def GetResources(self):
        return {"Pixmap"  : "Arch_Roof",
                "MenuText": QT_TRANSLATE_NOOP("Arch_Roof", "Roof"),
                "Accel"   : "R, F",
                "ToolTip" : QT_TRANSLATE_NOOP("Arch_Roof", "Creates a roof object from the selected wire.")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        import ArchComponent
        sel = FreeCADGui.Selection.getSelectionEx()
        if sel:
            sel = sel[0]
            obj = sel.Object
            FreeCADGui.Control.closeDialog()
            if sel.HasSubObjects:
                if "Face" in sel.SubElementNames[0]:
                    i = int(sel.SubElementNames[0][4:])
                    FreeCAD.ActiveDocument.openTransaction(translate("Arch", "Create Roof"))
                    FreeCADGui.addModule("Arch")
                    FreeCADGui.doCommand("obj = Arch.makeRoof(FreeCAD.ActiveDocument." + obj.Name + "," + str(i) + ")")
                    FreeCADGui.addModule("Draft")
                    FreeCADGui.doCommand("Draft.autogroup(obj)")
                    FreeCAD.ActiveDocument.commitTransaction()
                    FreeCAD.ActiveDocument.recompute()
                    return
            if hasattr(obj, "Shape"):
                if obj.Shape.Wires:
                    FreeCAD.ActiveDocument.openTransaction(translate("Arch", "Create Roof"))
                    FreeCADGui.addModule("Arch")
                    FreeCADGui.doCommand("obj = Arch.makeRoof(FreeCAD.ActiveDocument." + obj.Name + ")")
                    FreeCADGui.addModule("Draft")
                    FreeCADGui.doCommand("Draft.autogroup(obj)")
                    FreeCAD.ActiveDocument.commitTransaction()
                    FreeCAD.ActiveDocument.recompute()
                    return
            else:
                FreeCAD.Console.PrintMessage(translate("Arch", "Unable to create a roof"))
        else:
            FreeCAD.Console.PrintMessage(translate("Arch", "Please select a base object") + "\n")
            FreeCADGui.Control.showDialog(ArchComponent.SelectionTaskPanel())
            FreeCAD.ArchObserver = ArchComponent.ArchSelectionObserver(nextCommand = "Arch_Roof")
            FreeCADGui.Selection.addObserver(FreeCAD.ArchObserver)


FreeCADGui.addCommand("Arch_Roof", Arch_Roof())

