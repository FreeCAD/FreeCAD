# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
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
from PySide import QtCore, QtGui

"""Path Array object and FreeCAD command"""

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)


class ObjectArray:

    def __init__(self, obj):
        obj.addProperty("App::PropertyLink", "Base",
                        "Path", "The path to array")
        obj.addProperty("App::PropertyVectorDistance", "Offset",
                        "Path", "The spacing between the array copies")
        obj.addProperty("App::PropertyInteger", "Copies",
                        "Path", "The number of copies")
        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def execute(self, obj):
        if obj.Base:
            if not obj.Base.isDerivedFrom("Path::Feature"):
                return
            if not obj.Base.Path:
                return

            # build copies
            basepath = obj.Base.Path
            output = ""
            pl = FreeCAD.Placement()
            if obj.Offset != FreeCAD.Vector():
                for i in range(obj.Copies):
                    pl.move(obj.Offset)
                    np = Path.Path([cm.transform(pl)
                                    for cm in basepath.Commands])
                    output += np.toGCode()

            # print output
            path = Path.Path(output)
            obj.Path = path


class ViewProviderArray:

    def __init__(self, vobj):
        vobj.Proxy = self

    def attach(self, vobj):
        self.Object = vobj.Object
        return

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def claimChildren(self):
        if hasattr(self, "Object"):
            if hasattr(self.Object, "Base"):
                if self.Object.Base:
                    return self.Object.Base
        return []


class CommandPathArray:

    def GetResources(self):
        return {'Pixmap': 'Path-Array',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Array", "Array"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Array", "Creates an array from a selected path")}

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
                translate("Path_Array", "Please select exactly one path object\n"))
            return
        if not(selection[0].isDerivedFrom("Path::Feature")):
            FreeCAD.Console.PrintError(
                translate("Path_Array", "Please select exactly one path object\n"))
            return

        # if everything is ok, execute and register the transaction in the
        # undo/redo stack
        FreeCAD.ActiveDocument.openTransaction("Create Array")
        FreeCADGui.addModule("PathScripts.PathArray")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand(
            'obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","Array")')
        FreeCADGui.doCommand('PathScripts.PathArray.ObjectArray(obj)')
        FreeCADGui.doCommand(
            'obj.Base = (FreeCAD.ActiveDocument.' + selection[0].Name + ')')
        # FreeCADGui.doCommand('PathScripts.PathArray.ViewProviderArray(obj.ViewObject)')
        FreeCADGui.doCommand('obj.ViewObject.Proxy = 0')
        FreeCADGui.doCommand('PathScripts.PathUtils.addToJob(obj)')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Array', CommandPathArray())
