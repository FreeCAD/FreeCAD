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

import FreeCAD,Path
from PySide import QtCore,QtGui

FreeCADGui = None
if FreeCAD.GuiUp:
    import FreeCADGui

"""Path Project object and FreeCAD command"""

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)


class ObjectPathProject:
    

    def __init__(self,obj):
#        obj.addProperty("App::PropertyFile", "PostProcessor", "CodeOutput", translate("PostProcessor","Select the Post Processor file for this project"))
        obj.addProperty("App::PropertyFile", "OutputFile", "CodeOutput", translate("OutputFile","The NC output file for this project"))
        obj.setEditorMode("OutputFile",0) #set to default mode
#        obj.addProperty("App::PropertyBool","Editor","CodeOutput",translate("Show Editor","Show G-Code in simple editor after posting code"))
#        obj.addProperty("Path::PropertyTooltable","Tooltable",  "Path",translate("PathProject","The tooltable of this feature"))
        obj.addProperty("App::PropertyString",    "Description","Path",translate("PathProject","An optional description for this project"))
        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None

    def onChanged(self,obj,prop):
        pass

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

class ViewProviderProject:

    def __init__(self,vobj):
        vobj.Proxy = self
        mode = 2
#        vobj.setEditorMode('LineWidth',mode)
#        vobj.setEditorMode('MarkerColor',mode)
#        vobj.setEditorMode('NormalColor',mode)
#        vobj.setEditorMode('ShowFirstRapid',mode)
        vobj.setEditorMode('BoundingBox',mode)
        vobj.setEditorMode('DisplayMode',mode)
        vobj.setEditorMode('Selectable',mode)
        vobj.setEditorMode('ShapeColor',mode)
        vobj.setEditorMode('Transparency',mode)
#        vobj.setEditorMode('Visibility',mode)

    def __getstate__(self): #mandatory
        return None

    def __setstate__(self,state): #mandatory
        return None

    def getIcon(self):
        return ":/icons/Path-Project.svg"

    def onChanged(self,vobj,prop):
        mode = 2
#        vobj.setEditorMode('LineWidth',mode)
#        vobj.setEditorMode('MarkerColor',mode)
#        vobj.setEditorMode('NormalColor',mode)
#        vobj.setEditorMode('ShowFirstRapid',mode)
        vobj.setEditorMode('BoundingBox',mode)
        vobj.setEditorMode('DisplayMode',mode)
        vobj.setEditorMode('Selectable',mode)
        vobj.setEditorMode('ShapeColor',mode)
        vobj.setEditorMode('Transparency',mode)
#        vobj.setEditorMode('Visibility',mode)


class CommandProject:


    def GetResources(self):
        return {'Pixmap'  : 'Path-Project',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Project","Project"),
                'Accel': "P, P",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Project","Creates a Path Project object")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None
        
    def Activated(self):
        incl = []
        sel = FreeCADGui.Selection.getSelection()
        for obj in sel:
            if obj.isDerivedFrom("Path::Feature"):
                incl.append(obj)
        FreeCAD.ActiveDocument.openTransaction(translate("Path_Project","Create Project"))
        CommandProject.Create(incl)
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

    @staticmethod
    def Create(pathChildren = []):
        """Code to create a project"""
        #FreeCADGui.addModule("PathScripts.PathProject")
        obj = FreeCAD.ActiveDocument.addObject("Path::FeatureCompoundPython","Project")
        ObjectPathProject(obj)
        if pathChildren:
            for child in pathChildren:
                pathChildren.append(FreeCAD.ActiveDocument.getObject(obj.Name))
            obj.Group = pathChildren
        ViewProviderProject(obj.ViewObject)

        #create a machine obj
        import PathScripts
        PathScripts.PathMachine.CommandPathMachine.Create()

        return obj


if FreeCAD.GuiUp: 
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Project',CommandProject())

FreeCAD.Console.PrintLog("Loading PathProject... done\n")
