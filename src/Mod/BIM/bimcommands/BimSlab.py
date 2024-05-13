# -*- coding: utf8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 Yorik van Havre <yorik@uncreated.net>              *
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

"""The BIM Slab command"""

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate


class BIM_Slab:

    def __init__(self):
        self.callback = None
        self.view = None

    def GetResources(self):
        return {
            "Pixmap": "BIM_Slab",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Slab", "Slab"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Slab", "Creates a slab from a planar shape"
            ),
            "Accel": "S,B",
        }

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        import DraftTools

        self.removeCallback()
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            self.proceed()
        else:
            if hasattr(FreeCADGui, "draftToolBar"):
                FreeCADGui.draftToolBar.selectUi()
            FreeCAD.Console.PrintMessage(
                translate("BIM", "Select a planar object") + "\n"
            )
            FreeCAD.activeDraftCommand = self
            self.view = FreeCADGui.ActiveDocument.ActiveView
            self.callback = self.view.addEventCallback(
                "SoEvent", DraftTools.selectObject
            )

    def proceed(self):
        self.removeCallback()
        sel = FreeCADGui.Selection.getSelection()
        if len(sel) == 1:
            FreeCADGui.addModule("Arch")
            FreeCAD.ActiveDocument.openTransaction("Create Slab")
            FreeCADGui.doCommand(
                "s = Arch.makeStructure(FreeCAD.ActiveDocument."
                + sel[0].Name
                + ",height=200)"
            )
            FreeCADGui.doCommand('s.Label = "' + translate("BIM", "Slab") + '"')
            FreeCADGui.doCommand('s.IfcType = "Slab"')
            FreeCADGui.doCommand("s.Normal = FreeCAD.Vector(0,0,-1)")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
        self.finish()

    def removeCallback(self):
        if self.callback:
            try:
                self.view.removeEventCallback("SoEvent", self.callback)
            except RuntimeError:
                pass
            self.callback = None

    def finish(self):
        self.removeCallback()
        if hasattr(FreeCADGui, "draftToolBar"):
            FreeCADGui.draftToolBar.offUi()


FreeCADGui.addCommand('BIM_Slab', BIM_Slab())
