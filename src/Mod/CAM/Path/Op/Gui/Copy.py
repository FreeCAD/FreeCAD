# -*- coding: utf-8 -*-
# ***************************************************************************
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
from PySide import QtCore

from PySide.QtCore import QT_TRANSLATE_NOOP

__doc__ = """CAM Copy object and FreeCAD command"""


translate = FreeCAD.Qt.translate


class ObjectPathCopy:
    def __init__(self, obj):
        obj.addProperty(
            "App::PropertyLink",
            "Base",
            "Path",
            QT_TRANSLATE_NOOP("App::Property", "The toolpath to be copied"),
        )
        obj.addProperty(
            "App::PropertyLink",
            "ToolController",
            "Path",
            QT_TRANSLATE_NOOP(
                "App::Property",
                "The tool controller that will be used to calculate the toolpath",
            ),
        )
        obj.Proxy = self

    def dumps(self):
        return None

    def loads(self, state):
        return None

    def execute(self, obj):
        if obj.Base:
            if hasattr(obj.Base, "ToolController"):
                obj.ToolController = obj.Base.ToolController
            if obj.Base.Path:
                obj.Path = obj.Base.Path.copy()
            if obj.Base.Placement:
                obj.Placement = obj.Base.Placement


class ViewProviderPathCopy:
    def __init__(self, vobj):
        self.Object = vobj.Object
        vobj.Proxy = self

    def attach(self, vobj):
        self.Object = vobj.Object
        return

    def getIcon(self):
        return ":/icons/CAM_Copy.svg"

    def dumps(self):
        return None

    def loads(self, state):
        return None


class CommandPathCopy:
    def GetResources(self):
        return {
            "Pixmap": "CAM_Copy",
            "MenuText": QT_TRANSLATE_NOOP("CAM_Copy", "Copy"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_Copy", "Creates a linked copy of another toolpath"
            ),
        }

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                    return True
        return False

    def Activated(self):

        FreeCAD.ActiveDocument.openTransaction("Create Copy")
        FreeCADGui.addModule("Path.Op.Gui.Copy")

        consolecode = """
import Path
import Path.Op.Gui.Copy
selGood = True
# check that the selection contains exactly what we want
selection = FreeCADGui.Selection.getSelection()
proj = selection[0].InList[0] #get the group that the selectied object is inside

if len(selection) != 1:
    FreeCAD.Console.PrintError(translate("CAM_Copy", "Please select one toolpath object")+"\n")
    selGood = False

if not selection[0].isDerivedFrom("Path::Feature"):
    FreeCAD.Console.PrintError(translate("CAM_Copy", "The selected object is not a toolpath")+"\n")
    selGood = False

if selGood:
    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", str(selection[0].Name)+'_Copy')
    Path.Op.Gui.Copy.ObjectPathCopy(obj)
    Path.Op.Gui.Copy.ViewProviderPathCopy(obj.ViewObject)
    obj.Base = FreeCAD.ActiveDocument.getObject(selection[0].Name)
    if hasattr(obj.Base, 'ToolController'):
        obj.ToolController = obj.Base.ToolController

g = proj.Group
g.append(obj)
proj.Group = g

FreeCAD.ActiveDocument.recompute()

"""

        FreeCADGui.doCommand(consolecode)
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand("CAM_Copy", CommandPathCopy())

FreeCAD.Console.PrintLog("Loading PathCopy... done\n")
