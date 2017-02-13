# ***************************************************************************
# *   (c) Sliptonic (shopinthewoods@gmail.com)  2016                        *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Lesser General Public License for more details.                   *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************/

'''This file has utilities for checking and catching common errors in FreeCAD
Path projects.  Ideally, the user could execute these utilities from an icon
to make sure tools are selected and configured and defaults have been revised'''

from __future__ import print_function
from PySide import QtCore, QtGui
import FreeCAD
import FreeCADGui
import PathScripts.PathUtils as PU

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)


def review(obj):
    "checks the selected job for common errors"
    toolcontrolcount = 0

    if len(obj.Tooltable.Tools) == 0:
        FreeCAD.Console.PrintWarning(translate("Path_Sanity",  "Machine: " + str(obj.Label) + " has no tools defined in the tool table\n"))
    if obj.X_Max == obj.X_Min or obj.Y_Max == obj.Y_Min:
        FreeCAD.Console.PrintWarning(translate("Path_Sanity", "It appears the machine limits haven't been set.  Not able to check path extents.\n"))

    for item in obj.Group:
        print("Checking: " + item.Label)
        if item.Name[:2] == "TC":
            toolcontrolcount += 1
            if item.ToolNumber == 0:
                FreeCAD.Console.PrintWarning(translate("Path_Sanity", "Tool Controller: " + str(item.Label) + " is using ID 0 which the undefined default. Please set a real tool.\n"))
            else:
                tool = PU.getTool(item, item.ToolNumber)
                if tool is None:
                    FreeCAD.Console.PrintError(translate("Path_Sanity", "Tool Controller: " + str(item.Label) + " is using tool: " + str(item.ToolNumber) + " which is invalid\n"))
                elif tool.Diameter == 0:
                    FreeCAD.Console.PrintError(translate("Path_Sanity", "Tool Controller: " +  str(item.Label) + " is using tool: " + str(item.ToolNumber) + " which has a zero diameter\n"))
            if item.HorizFeed == 0:
                FreeCAD.Console.PrintWarning(translate("Path_Sanity",  "Tool Controller: " + str(item.Label) + " has a 0 value for the Horizontal feed rate\n"))
            if item.VertFeed == 0:
                FreeCAD.Console.PrintWarning(translate("Path_Sanity",  "Tool Controller: " + str(item.Label) + " has a 0 value for the Vertical feed rate\n"))
            if item.SpindleSpeed == 0:
               FreeCAD.Console.PrintWarning(translate("Path_Sanity", "Tool Controller: " + str(item.Label) + " has a 0 value for the spindle speed\n"))

    if toolcontrolcount == 0:
        FreeCAD.Console.PrintWarning(translate("Path_Sanity", "A Tool Controller was not found. Default values are used which is dangerous.  Please add a Tool Controller.\n"))


class CommandPathSanity:

    def GetResources(self):
        return {'Pixmap'  : 'Path-Sanity',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Sanity","Check the Path project for common errors"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Sanity","Check the Path Project for common errors")}

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
            FreeCAD.Console.PrintError(translate("Path_Sanity","Please select a path Project to check\n"))
            return
        if not(selection[0].TypeId == "Path::FeatureCompoundPython"):
            FreeCAD.Console.PrintError(translate("Path_Sanity","Please select a path project to check\n"))
            return

        # if everything is ok, execute
        FreeCADGui.addModule("PathScripts.PathSanity")
        FreeCADGui.doCommand('PathScripts.PathSanity.review(FreeCAD.ActiveDocument.' + selection[0].Name + ')')


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Sanity',CommandPathSanity())

