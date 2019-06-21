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
from PySide import QtCore, QtGui

"""Path Hop object and FreeCAD command"""

# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class ObjectHop:

    def __init__(self, obj):
        obj.addProperty("App::PropertyLink", "NextObject", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","The object to be reached by this hop"))
        obj.addProperty("App::PropertyDistance", "HopHeight", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property","The Z height of the hop"))
        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def execute(self, obj):
        nextpoint = FreeCAD.Vector()
        if obj.NextObject:
            if obj.NextObject.isDerivedFrom("Path::Feature"):
                # look for the first position of the next path
                for c in obj.NextObject.Path.Commands:
                    if c.Name in ["G0", "G00", "G1", "G01", "G2", "G02", "G3", "G03"]:
                        nextpoint = c.Placement.Base
                        break

        # absolute coords, millimeters, cancel offsets
        output = "G90\nG21\nG40\n"

        # go up to the given height
        output += "G0 Z" + str(obj.HopHeight.Value) + "\n"

        # go horizontally to the position of nextpoint
        output += "G0 X" + str(nextpoint.x) + " Y" + str(nextpoint.y) + "\n"

        # print output
        path = Path.Path(output)
        obj.Path = path


class ViewProviderPathHop:

    def __init__(self, vobj):
        vobj.Proxy = self

    def attach(self, vobj):
        self.Object = vobj.Object
        return

    def getIcon(self):
        return ":/icons/Path-Hop.svg"

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None


class CommandPathHop:

    def GetResources(self):
        return {'Pixmap': 'Path-Hop',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Hop", "Hop"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Hop", "Creates a Path Hop object")}

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                        return True
        return False

    def Activated(self):

        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) != 1:
            FreeCAD.Console.PrintError(
                translate("Path_Hop", "Please select one path object")+"\n")
            return
        if not selection[0].isDerivedFrom("Path::Feature"):
            FreeCAD.Console.PrintError(
                translate("Path_Hop", "The selected object is not a path")+"\n")
            return

        FreeCAD.ActiveDocument.openTransaction(
            translate("Path_Hop", "Create Hop"))
        FreeCADGui.addModule("PathScripts.PathHop")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand(
            'obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","Hop")')
        FreeCADGui.doCommand('PathScripts.PathHop.ObjectHop(obj)')
        FreeCADGui.doCommand(
            'PathScripts.PathHop.ViewProviderPathHop(obj.ViewObject)')
        FreeCADGui.doCommand(
            'obj.NextObject = FreeCAD.ActiveDocument.' + selection[0].Name)
        FreeCADGui.doCommand('PathScripts.PathUtils.addToJob(obj)')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Hop', CommandPathHop())

FreeCAD.Console.PrintLog("Loading PathHop... done\n")
