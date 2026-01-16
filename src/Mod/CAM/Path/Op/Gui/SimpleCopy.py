# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2015 Yorik van Havre <yorik@uncreated.net>              *
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

import FreeCAD
import FreeCADGui
from Path.Base.Util import coolantModeForOp
from Path.Base.Util import toolControllerForOp
from PySide.QtCore import QT_TRANSLATE_NOOP

__doc__ = """CAM SimpleCopy command"""

translate = FreeCAD.Qt.translate


class ViewProvider:

    def __init__(self, vobj):
        self.attach(vobj)
        vobj.Proxy = self

    def attach(self, vobj):
        self.vobj = vobj
        self.obj = vobj.Object

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def onChanged(self, vobj, prop):
        return

    def getIcon(self):
        if self.obj.Active:
            return ":/icons/CAM_SimpleCopy.svg"
        else:
            return ":/icons/CAM_OpActive.svg"


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
        if any([not hasattr(sel, "Path") for sel in selection]):
            return False
        if any([not op.Path.Commands for op in selection]):
            return False

        coolant = coolantModeForOp(selection[0])
        if not all([coolant == coolantModeForOp(op) for op in selection]):
            return False

        toolController = toolControllerForOp(selection[0])
        if not all([toolController == toolControllerForOp(op) for op in selection]):
            return False

        return True

    def Activated(self):
        # check that the selection contains exactly what we want
        FreeCAD.ActiveDocument.openTransaction("Simple Copy")
        FreeCADGui.doCommand("selection = FreeCADGui.Selection.getSelection()")
        FreeCADGui.doCommand(
            "name = selection[0].Name+'_SimpleCopy' if len(selection) == 1 else 'SimpleCopy'"
        )
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand("job = PathScripts.PathUtils.findParentJob(selection[0])")
        FreeCADGui.doCommand(
            "paths = [PathScripts.PathUtils.getPathWithPlacement(sel) for sel in selection]"
        )
        FreeCADGui.addModule("Path.Op.Custom")
        FreeCADGui.doCommand("obj = Path.Op.Custom.Create(name, parentJob=job)")
        FreeCADGui.doCommand(
            "obj.ToolController = Path.Base.Util.toolControllerForOp(selection[0])"
        )
        FreeCADGui.doCommand("obj.CoolantMode = Path.Base.Util.coolantModeForOp(selection[0])")
        FreeCADGui.doCommand(
            "obj.ViewObject.Proxy = Path.Op.Gui.SimpleCopy.ViewProvider(obj.ViewObject)"
        )
        FreeCADGui.doCommand("obj.Gcode = [c.toGCode() for path in paths for c in path.Commands]")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_SimpleCopy", CommandPathSimpleCopy())
