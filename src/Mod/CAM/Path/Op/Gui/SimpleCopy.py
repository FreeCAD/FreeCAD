# -*- coding: utf-8 -*-
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
import Path
import PathScripts
from PySide.QtCore import QT_TRANSLATE_NOOP

__doc__ = """CAM SimpleCopy command"""

translate = FreeCAD.Qt.translate


class CommandPathSimpleCopy:
    def GetResources(self):
        return {
            "Pixmap": "CAM_SimpleCopy",
            "MenuText": QT_TRANSLATE_NOOP("CAM_SimpleCopy", "Simple Copy"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_SimpleCopy", "Creates a non-parametric copy of another toolpath"
            ),
        }

    def IsActive(self):
        if bool(FreeCADGui.Selection.getSelection()) is False:
            return False
        try:
            obj = FreeCADGui.Selection.getSelectionEx()[0].Object
            return isinstance(obj.Proxy, Path.Op.Base.ObjectOp)
        except Exception:
            return False

    def Activated(self):
        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) != 1:
            FreeCAD.Console.PrintError(
                translate("CAM_SimpleCopy", "Please select exactly one toolpath object")
                + "\n"
            )
            return
        if not (selection[0].isDerivedFrom("Path::Feature")):
            FreeCAD.Console.PrintError(
                translate("CAM_SimpleCopy", "Please select exactly one toolpath object")
                + "\n"
            )
            return

        FreeCAD.ActiveDocument.openTransaction("Simple Copy")
        FreeCADGui.doCommand(
            "srcobj = FreeCADGui.Selection.getSelectionEx()[0].Object\n"
        )

        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand(
            "srcpath = PathScripts.PathUtils.getPathWithPlacement(srcobj)\n"
        )
        FreeCADGui.addModule("Path.Op.Custom")
        FreeCADGui.doCommand(
            'obj = Path.Op.Custom.Create("' + selection[0].Name + '_SimpleCopy")'
        )
        FreeCADGui.doCommand("obj.ViewObject.Proxy = 0")
        FreeCADGui.doCommand("obj.Gcode = [c.toGCode() for c in srcpath.Commands]")
        FreeCADGui.doCommand("PathScripts.PathUtils.addToJob(obj)")
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_SimpleCopy", CommandPathSimpleCopy())
