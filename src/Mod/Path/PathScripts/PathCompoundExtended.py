# -*- coding: utf-8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

import FreeCAD,FreeCADGui,Path,PathGui
from PySide import QtCore,QtGui

"""Path Compound Extended object and FreeCAD command"""

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)


class ObjectCompoundExtended:
    

    def __init__(self,obj):
        obj.addProperty("App::PropertyString","Description",  "Path",translate("PathCompoundExtended","An optional description of this compounded operation"))
#        obj.addProperty("App::PropertySpeed", "FeedRate",     "Path",translate("PathCompoundExtended","The feed rate of the paths in these compounded operations"))
#        obj.addProperty("App::PropertyFloat", "SpindleSpeed", "Path",translate("PathCompoundExtended","The spindle speed, in revolutions per minute, of the tool used in these compounded operations"))
        obj.addProperty("App::PropertyLength","SafeHeight",   "Path",translate("PathCompoundExtended","The safe height for this operation"))
        obj.addProperty("App::PropertyLength","RetractHeight","Path",translate("PathCompoundExtended","The retract height, above top surface of part, between compounded operations inside clamping area"))
        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None

    def execute(self,obj):
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

    def __init__(self,vobj):
        vobj.Proxy = self

    def attach(self,vobj):
        self.Object = vobj.Object
        return

    def getIcon(self):
        return ":/icons/Path-Compound.svg"

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None


class CommandCompoundExtended:


    def GetResources(self):
        return {'Pixmap'  : 'Path-Compound',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_CompoundExtended","Compound"),
                'Accel': "P, C",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_CompoundExtended","Creates a Path Compound object")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None
        
    def Activated(self):

        FreeCAD.ActiveDocument.openTransaction(translate("Path_CompoundExtended","Create Compound"))
        FreeCADGui.addModule("PathScripts.PathCompoundExtended")
        snippet = '''
import Path
import PathScripts
from PathScripts import PathUtils
incl = []
prjexists = False
sel = FreeCADGui.Selection.getSelection()
for s in sel:
    if s.isDerivedFrom("Path::Feature"):
        incl.append(s)

obj = FreeCAD.ActiveDocument.addObject("Path::FeatureCompoundPython","Compound")
PathScripts.PathCompoundExtended.ObjectCompoundExtended(obj)
PathScripts.PathCompoundExtended.ViewProviderCompoundExtended(obj.ViewObject)
project = PathUtils.addToProject(obj)

if incl:
    children = []
    p = project.Group
    
    g = obj.Group
    for child in incl:
        p.remove(child)
        children.append(FreeCAD.ActiveDocument.getObject(child.Name))
    project.Group = p
    g.append(children)
    obj.Group = children
'''
        FreeCADGui.doCommand(snippet)
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp: 
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_CompoundExtended',CommandCompoundExtended())

FreeCAD.Console.PrintLog("Loading PathCompoundExtended... done\n")
