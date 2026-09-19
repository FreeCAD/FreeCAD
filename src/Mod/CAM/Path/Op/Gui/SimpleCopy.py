# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2015 Yorik van Havre <yorik@uncreated.net>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

import FreeCAD
import FreeCADGui
import Path
from Path.Base.Util import coolantModeForOp
from Path.Base.Util import toolControllerForOp
import Path.Dressup.Utils as PathDressup
from PySide.QtCore import QT_TRANSLATE_NOOP

__doc__ = """CAM SimpleCopy command"""

translate = FreeCAD.Qt.translate


class ViewProvider(Path.Op.Gui.Base.ViewProvider):
    """This class exists to replace ViewProvider of SimpleCopy objects in old documents
    Should be removed in release 27.3"""

    def loads(self, state):
        """Overwrites base class because 'state' is None"""
        self.OpName = "Custom"
        self.OpIcon = ":/icons/CAM_Custom.svg"
        self.OpPageModule = "Path.Op.Gui.Custom"
        self.OpPageClass = "TaskPanelOpPage"

    def dumps(self):
        """Overwrites base class
        Changes ViewProvider and dumps correct 'state'"""
        res = Path.Op.Gui.Custom.Command.res
        self.Object.ViewObject.Proxy = Path.Op.Gui.Base.ViewProvider(self.Object.ViewObject, res)
        return super().dumps()


class CommandPathSimpleCopy:
    def GetResources(self):
        return {
            "Pixmap": "CAM_SimpleCopy",
            "MenuText": QT_TRANSLATE_NOOP("CAM_SimpleCopy", "Simple Copy"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_SimpleCopy",
                "Creates a non-parametric copy of another toolpath\n"
                "Several operations can be used with identical tool controller and coolant mode",
            ),
        }

    def IsActive(self):
        selection = FreeCADGui.Selection.getSelection()
        if not selection:
            return False

        if any(not PathDressup.isOp(sel) for sel in selection):
            return False

        coolant = coolantModeForOp(selection[0])
        if any(coolant != coolantModeForOp(op) for op in selection):
            return False

        tc = toolControllerForOp(selection[0])
        return all(tc == toolControllerForOp(op) for op in selection)

    def Activated(self):
        # check that the selection contains exactly what we want
        FreeCAD.ActiveDocument.openTransaction("Simple Copy")
        FreeCADGui.doCommand("sel = FreeCADGui.Selection.getSelection()")
        FreeCADGui.doCommand("name = sel[0].Name+'_SimpleCopy' if len(sel) == 1 else 'SimpleCopy'")
        FreeCADGui.addModule("PathScripts.PathUtils as PathUtils")
        FreeCADGui.doCommand("job = PathUtils.findParentJob(sel[0])")
        FreeCADGui.doCommand("obj = Path.Op.Custom.Create(name, parentJob=job)")
        FreeCADGui.doCommand("res = Path.Op.Gui.Custom.Command.res")
        FreeCADGui.doCommand(
            "obj.ViewObject.Proxy = Path.Op.Gui.Base.ViewProvider(obj.ViewObject, res)"
        )
        FreeCADGui.doCommand("obj.ToolController = Path.Base.Util.toolControllerForOp(sel[0])")
        FreeCADGui.doCommand("obj.CoolantMode = Path.Base.Util.coolantModeForOp(sel[0])")
        FreeCADGui.doCommand("paths = [PathUtils.getPathWithPlacement(s) for s in sel]")
        FreeCADGui.doCommand("obj.Gcode = [c.toGCode() for path in paths for c in path.Commands]")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_SimpleCopy", CommandPathSimpleCopy())
