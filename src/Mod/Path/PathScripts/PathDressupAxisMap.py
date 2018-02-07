# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
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
import math
import PathScripts.PathUtils as P
from PySide import QtCore, QtGui

"""Axis remapping Dressup object and FreeCAD command.  This dressup remaps one axis of motion to another.
For example, you can re-map the Y axis to A to control a 4th axis rotary."""

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8

    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)

except AttributeError:

    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)


class ObjectDressup:

    def __init__(self, obj):
        maplist = ["X->A", "Y->A", "X->B", "Y->B", "X->C", "Y->C"]
        obj.addProperty("App::PropertyLink", "Base","Path", "The base path to modify")
        obj.addProperty("App::PropertyEnumeration", "axisMap", "Path", "The input mapping axis")
        obj.addProperty("App::PropertyDistance", "radius", "Path", "The radius of the wrapped axis")
        obj.axisMap = maplist
        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def _linear2angular(self, radius, length):
        '''returns an angular distance in degrees to achieve a linear move of a given lenth'''
        circum = 2 * math.pi * float(radius)
        return 360 * (float(length) / circum)

    def execute(self, obj):

        inAxis = obj.axisMap[0]
        outAxis = obj.axisMap[3]

        if obj.Base:
            if obj.Base.isDerivedFrom("Path::Feature"):
                if obj.Base.Path:
                    if obj.Base.Path.Commands:
                        newcommandlist = []
                        for c in obj.Base.Path.Commands:
                            newparams = dict(c.Parameters)
                            remapvar = newparams.pop(inAxis, None)
                            if remapvar is not None:
                                newparams[outAxis] = self._linear2angular(obj.radius, remapvar)
                                newcommand = Path.Command(c.Name, newparams)
                                newcommandlist.append(newcommand)
                            else:
                                newcommandlist.append(c)

                        path = Path.Path(newcommandlist)
                        obj.Path = path

#                        obj.Path.Commands = newcommandlist

class ViewProviderDressup:

    def __init__(self, vobj):
        vobj.Proxy = self

    def attach(self, vobj):
        self.Object = vobj.Object
        return

    def claimChildren(self):
        for i in self.Object.Base.InList:
            if hasattr(i, "Group"):
                group = i.Group
                for g in group:
                    if g.Name == self.Object.Base.Name:
                        group.remove(g)
                i.Group = group
                print i.Group
        return [self.Object.Base]

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def onDelete(self, arg1=None, arg2=None):
        '''this makes sure that the base operation is added back to the project and visible'''
        FreeCADGui.ActiveDocument.getObject(arg1.Object.Base.Name).Visibility = True
        P.addToProject(arg1.Object.Base)
        return True

class CommandPathDressup:

    def GetResources(self):
        return {'Pixmap': 'Path-Dressup',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_DressupAxisMap", "Axis Map Dress-up"),
                'Accel': "",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_DressupAxisMap", "Remap one axis to another.")}

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
            FreeCAD.Console.PrintError(translate("Path_Dressup", "Please select one path object\n"))
            return
        if not selection[0].isDerivedFrom("Path::Feature"):
            FreeCAD.Console.PrintError(translate("Path_Dressup", "The selected object is not a path\n"))
            return
        if selection[0].isDerivedFrom("Path::FeatureCompoundPython"):
            FreeCAD.Console.PrintError(translate("Path_Dressup", "Please select a Path object"))
            return

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction(translate("Path_Dressup", "Create Dress-up"))
        FreeCADGui.addModule("PathScripts.PathDressupAxisMap")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "AxisMapDressup")')
        FreeCADGui.doCommand('PathScripts.PathDressupAxisMap.ObjectDressup(obj)')
        FreeCADGui.doCommand('obj.Base = FreeCAD.ActiveDocument.' + selection[0].Name)
        FreeCADGui.doCommand('obj.radius = 45')
        FreeCADGui.doCommand('PathScripts.PathDressupAxisMap.ViewProviderDressup(obj.ViewObject)')
        FreeCADGui.doCommand('PathScripts.PathUtils.addToJob(obj)')
        FreeCADGui.doCommand('Gui.ActiveDocument.getObject(obj.Base.Name).Visibility = False')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_DressupAxisMap', CommandPathDressup())

FreeCAD.Console.PrintLog("Loading PathDressup... done\n")
