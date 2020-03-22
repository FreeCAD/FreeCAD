# -*- coding: utf-8 -*-
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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
from PySide import QtCore
from copy import copy


__doc__ = """Path Custom object and FreeCAD command"""

movecommands = ['G0', 'G00', 'G1', 'G01', 'G2', 'G02', 'G3', 'G03']

# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class ObjectCustom:
    def __init__(self, obj):
        obj.addProperty("App::PropertyStringList", "Gcode", "Path",
                QtCore.QT_TRANSLATE_NOOP("PathCustom", "The gcode to be inserted"))
        obj.addProperty("App::PropertyLink", "ToolController", "Path",
                QtCore.QT_TRANSLATE_NOOP("PathCustom", "The tool controller that will be used to calculate the path"))
        obj.addProperty("App::PropertyPlacement", "Offset", "Path",
                "Placement Offset")

        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def execute(self, obj):
        newpath = Path.Path()
        if obj.Gcode:
            for l in obj.Gcode:
                newcommand = Path.Command(str(l))
                if newcommand.Name in movecommands:
                    if 'X' in newcommand.Parameters:
                        newcommand.x += obj.Offset.Base.x
                    if 'Y' in newcommand.Parameters:
                        newcommand.y += obj.Offset.Base.y
                    if 'Z' in newcommand.Parameters:
                        newcommand.z += obj.Offset.Base.z

                newpath.insertCommand(newcommand)

        obj.Path=newpath



class CommandPathCustom:

    def GetResources(self):
        return {'Pixmap': 'Path-Custom',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Custom", "Custom"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Custom", "Creates a path object based on custom G-code")}

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                    return True
        return False

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create Custom Path")
        FreeCADGui.addModule("PathScripts.PathCustom")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "Custom")')
        FreeCADGui.doCommand('PathScripts.PathCustom.ObjectCustom(obj)')
        FreeCADGui.doCommand('obj.ViewObject.Proxy = 0')
        FreeCADGui.doCommand('PathScripts.PathUtils.addToJob(obj)')
        FreeCADGui.doCommand('obj.ToolController = PathScripts.PathUtils.findToolController(obj)')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Custom', CommandPathCustom())