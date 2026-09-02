# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 Yorik van Havre <yorik@uncreated.net>              *
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

"""The BIM Slab command"""

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate


class BIM_Slab:

    def __init__(self):
        self.observing = False

    def GetResources(self):
        return {
            "Pixmap": "BIM_Slab",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Slab", "Slab"),
            "ToolTip": QT_TRANSLATE_NOOP("BIM_Slab", "Creates a slab from a planar shape"),
            "Accel": "S,B",
        }

    def IsActive(self):
        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):
        self.removeObserver()
        sel = FreeCADGui.Selection.getSelection()
        if sel:
            self.proceed()
        else:
            if hasattr(FreeCADGui, "draftToolBar"):
                FreeCADGui.draftToolBar.selectUi(on_close_call=self.finish)
            FreeCAD.Console.PrintMessage(translate("BIM", "Select a planar object") + "\n")
            FreeCADGui.HintManager.show(
                FreeCADGui.InputHint(
                    translate("BIM", "%1 select a planar object"),
                    FreeCADGui.UserInput.MouseLeft,
                )
            )
            FreeCADGui.Selection.addObserver(self)
            self.observing = True

    def addSelection(self, document, obj_name, _sub_name, _position):
        obj = FreeCAD.getDocument(document).getObject(obj_name)
        if obj:
            self.proceed(obj)

    def proceed(self, obj=None):
        self.removeObserver()
        if obj is None:
            sel = FreeCADGui.Selection.getSelection()
            obj = sel[0] if len(sel) == 1 else None
        if obj:
            FreeCADGui.addModule("Arch")
            FreeCAD.ActiveDocument.openTransaction("Create Slab")
            FreeCADGui.doCommand(
                "s = Arch.makeStructure(FreeCAD.ActiveDocument." + obj.Name + ",height=200)"
            )
            FreeCADGui.doCommand("s.Label = " + repr(translate("BIM", "Slab")))
            FreeCADGui.doCommand('s.IfcType = "Slab"')
            FreeCADGui.doCommand("s.Normal = FreeCAD.Vector(0,0,-1)")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
        self.finish()

    def removeObserver(self):
        if self.observing:
            FreeCADGui.Selection.removeObserver(self)
            self.observing = False

    def finish(self):
        self.removeObserver()
        FreeCADGui.HintManager.hide()
        if hasattr(FreeCADGui, "draftToolBar"):
            FreeCADGui.draftToolBar.offUi()


FreeCADGui.addCommand("BIM_Slab", BIM_Slab())
