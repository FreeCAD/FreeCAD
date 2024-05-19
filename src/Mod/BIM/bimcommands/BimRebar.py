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

"""BIM Rebar command"""


import os
import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate
PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")


class Arch_Rebar:

    "the Arch Rebar command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Rebar',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Rebar","Custom Rebar"),
                'Accel': "R, B",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Rebar","Creates a Reinforcement bar from the selected face of solid object and/or a sketch")}

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None

    def Activated(self):

        import ArchComponent
        sel = FreeCADGui.Selection.getSelectionEx()
        if sel:
            obj = sel[0].Object
            if hasattr(obj,"Shape") and obj.Shape.Solids:
                # this is our host object
                if len(sel) > 1:
                    sk = sel[1].Object
                    if hasattr(sk,'Shape'):
                        if len(sk.Shape.Wires) == 1:
                            # we have a structure and a wire: create the rebar now
                            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Rebar"))
                            FreeCADGui.addModule("Arch")
                            FreeCADGui.doCommand("Arch.makeRebar(FreeCAD.ActiveDocument."+obj.Name+",FreeCAD.ActiveDocument."+sk.Name+")")
                            FreeCAD.ActiveDocument.commitTransaction()
                            FreeCAD.ActiveDocument.recompute()
                            return
                else:
                    # we have only a structure: open the sketcher
                    FreeCADGui.activateWorkbench("SketcherWorkbench")
                    FreeCADGui.runCommand("Sketcher_NewSketch")
                    FreeCAD.ArchObserver = ArchComponent.ArchSelectionObserver(obj,FreeCAD.ActiveDocument.Objects[-1],hide=False,nextCommand="Arch_Rebar")
                    FreeCADGui.Selection.addObserver(FreeCAD.ArchObserver)
                    return
            elif hasattr(obj,'Shape'):
                if len(obj.Shape.Wires) == 1:
                    # we have only a wire: extract its support object, if available, and make the rebar
                    support = "None"
                    if hasattr(obj,"AttachmentSupport"):
                        if obj.AttachmentSupport:
                            if len(obj.AttachmentSupport) != 0:
                                support = "FreeCAD.ActiveDocument."+obj.AttachmentSupport[0][0].Name
                    FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Rebar"))
                    FreeCADGui.addModule("Arch")
                    FreeCADGui.doCommand("Arch.makeRebar("+support+",FreeCAD.ActiveDocument."+obj.Name+")")
                    FreeCAD.ActiveDocument.commitTransaction()
                    FreeCAD.ActiveDocument.recompute()
                    return

        FreeCAD.Console.PrintMessage(translate("Arch","Please select a base face on a structural object")+"\n")
        FreeCADGui.Control.closeDialog()
        FreeCADGui.Control.showDialog(ArchComponent.SelectionTaskPanel())
        FreeCAD.ArchObserver = ArchComponent.ArchSelectionObserver(nextCommand="Arch_Rebar")
        FreeCADGui.Selection.addObserver(FreeCAD.ArchObserver)
        

FreeCADGui.addCommand('Arch_Rebar',Arch_Rebar())
