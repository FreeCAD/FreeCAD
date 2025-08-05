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

"""BIM Rebar command"""

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
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Rebar","Creates a reinforcement bar from the selected face of solid object and/or a sketch")}

    def IsActive(self):

        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

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

        FreeCAD.Console.PrintMessage(translate("Arch","Select a base face on a structural object")+"\n")
        FreeCADGui.Control.closeDialog()
        FreeCADGui.Control.showDialog(ArchComponent.SelectionTaskPanel())
        FreeCAD.ArchObserver = ArchComponent.ArchSelectionObserver(nextCommand="Arch_Rebar")
        FreeCADGui.Selection.addObserver(FreeCAD.ArchObserver)


FreeCADGui.addCommand('Arch_Rebar',Arch_Rebar())
