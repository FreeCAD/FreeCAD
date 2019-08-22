# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 Dan Falck <ddfalck@gmail.com>                      *
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
''' Used for CNC machine plane selection G17,G18,G19 '''

import FreeCAD
import FreeCADGui
import Path
from PySide import QtCore

# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


class Plane:
    def __init__(self,obj):
        obj.addProperty("App::PropertyEnumeration", "SelectionPlane","Path",QtCore.QT_TRANSLATE_NOOP("App::Property","Orientation plane of CNC path"))
        obj.SelectionPlane=['XY', 'XZ', 'YZ']
        obj.addProperty("App::PropertyBool","Active","Path",QtCore.QT_TRANSLATE_NOOP("App::Property","Make False, to prevent operation from generating code"))
        obj.Proxy = self

    def execute(self, obj):
        clonelist = ['XY', 'XZ', 'YZ']
        cindx = clonelist.index(str(obj.SelectionPlane))
        pathlist = ['G17', 'G18', 'G19']
        labelindx = clonelist.index(obj.SelectionPlane) + 1
        obj.Label = "Plane" + str(labelindx)
        if obj.Active:
            obj.Path = Path.Path(pathlist[cindx])
            obj.ViewObject.Visibility = True
        else:
            obj.Path = Path.Path("(inactive operation)")
            obj.ViewObject.Visibility = False


class _ViewProviderPlane:

    def __init__(self, vobj):  # mandatory
        vobj.Proxy = self
        mode = 2
        vobj.setEditorMode('LineWidth', mode)
        vobj.setEditorMode('MarkerColor', mode)
        vobj.setEditorMode('NormalColor', mode)
        vobj.setEditorMode('DisplayMode', mode)
        vobj.setEditorMode('BoundingBox', mode)
        vobj.setEditorMode('Selectable', mode)
        vobj.setEditorMode('ShapeColor', mode)
        vobj.setEditorMode('Transparency', mode)
        vobj.setEditorMode('Visibility', mode)

    def __getstate__(self):  # mandatory
        return None

    def __setstate__(self, state):  # mandatory
        return None

    def getIcon(self):  # optional
        return ":/icons/Path-Plane.svg"

    def onChanged(self, vobj, prop):  # optional
        # pylint: disable=unused-argument
        mode = 2
        vobj.setEditorMode('LineWidth', mode)
        vobj.setEditorMode('MarkerColor', mode)
        vobj.setEditorMode('NormalColor', mode)
        vobj.setEditorMode('DisplayMode', mode)
        vobj.setEditorMode('BoundingBox', mode)
        vobj.setEditorMode('Selectable', mode)
        vobj.setEditorMode('ShapeColor', mode)
        vobj.setEditorMode('Transparency', mode)
        vobj.setEditorMode('Visibility', mode)

    def updateData(self, vobj, prop):  # optional
        # this is executed when a property of the APP OBJECT changes
        pass

    def setEdit(self, vobj, mode):  # optional
        # this is executed when the object is double-clicked in the tree
        pass

    def unsetEdit(self, vobj, mode):  # optional
        # this is executed when the user cancels or terminates edit mode
        pass


class CommandPathPlane:

    def GetResources(self):
        return {'Pixmap': 'Path-Plane',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Plane", "Selection Plane"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Plane", "Create a Selection Plane object")}

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                    return True
        return False

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction(
            translate("Path_Plane", "Create a Selection Plane object"))
        FreeCADGui.addModule("PathScripts.PathPlane")
        snippet = '''
import Path
import PathScripts
from PathScripts import PathUtils
prjexists = False
obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","Plane")
PathScripts.PathPlane.Plane(obj)
obj.Active = True
PathScripts.PathPlane._ViewProviderPlane(obj.ViewObject)
PathUtils.addToJob(obj)

'''

        FreeCADGui.doCommand(snippet)
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Plane', CommandPathPlane())


FreeCAD.Console.PrintLog("Loading PathPlane... done\n")
