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
from __future__ import print_function

import FreeCAD
import FreeCADGui
import Path
from PySide import QtCore, QtGui

"""Path Compound Extended object and FreeCAD command"""

# Qt tanslation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

class ObjectCompoundExtended:

    def __init__(self,obj):
        obj.addProperty("App::PropertyString","Description",  "Path",QtCore.QT_TRANSLATE_NOOP("App::Property","An optional description of this compounded operation"))
#        obj.addProperty("App::PropertySpeed", "FeedRate",     "Path",QtCore.QT_TRANSLATE_NOOP("App::Property","The feed rate of the paths in these compounded operations"))
#        obj.addProperty("App::PropertyFloat", "SpindleSpeed", "Path",QtCore.QT_TRANSLATE_NOOP("App::Property","The spindle speed, in revolutions per minute, of the tool used in these compounded operations"))
        obj.addProperty("App::PropertyLength","SafeHeight",   "Path",QtCore.QT_TRANSLATE_NOOP("App::Property","The safe height for this operation"))
        obj.addProperty("App::PropertyLength","RetractHeight","Path",QtCore.QT_TRANSLATE_NOOP("App::Property","The retract height, above top surface of part, between compounded operations inside clamping area"))
        obj.addProperty("App::PropertyLink", "ToolController", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "The tool controller that will be used to calculate the path"))
        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

    def onChanged(self, obj, prop):
        if prop == "Group":
            print('check order')
        for child in obj.Group:
            if child.isDerivedFrom("Path::Feature"):
                child.touch()

    def execute(self, obj):
        cmds = []
        for child in obj.Group:
            if child.isDerivedFrom("Path::Feature"):
                if obj.UsePlacements:
                    for c in child.Path.Commands:
                        cmds.append(c.transform(child.Placement))
                else:
                    cmds.extend(child.Path.Commands)
        if cmds:
            path = Path.Path(cmds)
            obj.Path = path


class ViewProviderCompoundExtended:

    def __init__(self, vobj):
        vobj.Proxy = self

    def attach(self, vobj):
        self.Object = vobj.Object
        return

    def getIcon(self):
        return ":/icons/Path-Compound.svg"

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None


class CommandCompoundExtended:

    def GetResources(self):
        return {'Pixmap': 'Path-Compound',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_CompoundExtended", "Compound"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_CompoundExtended", "Creates a Path Compound object")}

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                        return True
        return False

    def Activated(self):

        FreeCAD.ActiveDocument.openTransaction(translate("Path_CompoundExtended", "Create Compound"))
        FreeCADGui.addModule("PathScripts.PathCompoundExtended")
        snippet = '''
import Path
import PathScripts
from PathScripts import PathUtils
incl = []
prjexists = False
tc = None
sel = FreeCADGui.Selection.getSelection()
for s in sel:
    if s.isDerivedFrom("Path::Feature"):
        incl.append(s)

obj = FreeCAD.ActiveDocument.addObject("Path::FeatureCompoundPython","Compound")

PathScripts.PathCompoundExtended.ObjectCompoundExtended(obj)
PathScripts.PathCompoundExtended.ViewProviderCompoundExtended(obj.ViewObject)
project = PathUtils.addToJob(obj)

if incl:
    children = []
    p = project.Group
    g = obj.Group
    for child in incl:
        p.remove(child)
        childobj = FreeCAD.ActiveDocument.getObject(child.Name)
        if hasattr(childobj, 'ToolController'):
            tc = childobj.ToolController
        children.append(childobj)

    project.Group = p
    g.append(children)
    obj.Group = children
    obj.ToolController = tc
'''
        FreeCADGui.doCommand(snippet)
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_CompoundExtended', CommandCompoundExtended())

FreeCAD.Console.PrintLog("Loading PathCompoundExtended... done\n")
