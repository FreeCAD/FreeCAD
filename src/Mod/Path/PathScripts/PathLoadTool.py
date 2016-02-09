# -*- coding: utf-8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2015 Dan Falck <ddfalck@gmail.com>                      *
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
''' Used for CNC machine to load cutting Tool ie M6T3'''

import FreeCAD,FreeCADGui,Path,PathGui
import PathScripts, PathUtils
from PathScripts import PathProject
from PySide import QtCore,QtGui

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)

class LoadTool:
    def __init__(self,obj):
        obj.addProperty("App::PropertyIntegerConstraint", "ToolNumber","Tool", translate( "Tool Number",  "The active tool"))
        obj.ToolNumber = (0,0,10000,1)
        obj.addProperty("App::PropertyFloat", "SpindleSpeed", "Tool", translate("Spindle Speed","The speed of the cutting spindle in RPM"))
        obj.addProperty("App::PropertyEnumeration", "SpindleDir", "Tool", translate("Spindle Dir","Direction of spindle rotation"))
        obj.SpindleDir = ['Forward','Reverse']
        obj.Proxy = self
        mode = 2
        obj.setEditorMode('Placement',mode)



    def execute(self,obj):
        commands = ""
        commands = 'M6T'+str(obj.ToolNumber)+'\n'

        if obj.SpindleDir =='Forward':
            commands +='M3S'+str(obj.SpindleSpeed)+'\n'

        else:
            commands +='M4S'+str(obj.SpindleSpeed)+'\n'

        obj.Path = Path.Path(commands)
        obj.Label = "Tool"+str(obj.ToolNumber)


    def onChanged(self,obj,prop):
        mode = 2
        obj.setEditorMode('Placement',mode)
        if prop == "ToolNumber":
            proj = PathUtils.findProj()            
            for g in proj.Group:
                if not(isinstance(g.Proxy, PathScripts.PathLoadTool.LoadTool)):
                    g.touch()      
      
                
class _ViewProviderLoadTool:
    def __init__(self,vobj): #mandatory
        vobj.Proxy = self
        mode = 2
        vobj.setEditorMode('LineWidth',mode)
        vobj.setEditorMode('MarkerColor',mode)
        vobj.setEditorMode('NormalColor',mode)
        vobj.setEditorMode('ShowFirstRapid',mode)
        vobj.setEditorMode('DisplayMode',mode)
        vobj.setEditorMode('BoundingBox',mode)
        vobj.setEditorMode('Selectable',mode)
        vobj.setEditorMode('ShapeColor',mode)
        vobj.setEditorMode('Transparency',mode)
        vobj.setEditorMode('Visibility',mode)

    def __getstate__(self): #mandatory
        return None

    def __setstate__(self,state): #mandatory
        return None

    def getIcon(self): #optional
        return ":/icons/Path-LoadTool.svg"

    def onChanged(self,vobj,prop): #optional
        mode = 2
        vobj.setEditorMode('LineWidth',mode)
        vobj.setEditorMode('MarkerColor',mode)
        vobj.setEditorMode('NormalColor',mode)
        vobj.setEditorMode('ShowFirstRapid',mode)
        vobj.setEditorMode('DisplayMode',mode)
        vobj.setEditorMode('BoundingBox',mode)
        vobj.setEditorMode('Selectable',mode)
        vobj.setEditorMode('ShapeColor',mode)
        vobj.setEditorMode('Transparency',mode)
        vobj.setEditorMode('Visibility',mode)

    def updateData(self,vobj,prop): #optional
        # this is executed when a property of the APP OBJECT changes
        pass

    def setEdit(self,vobj,mode): #optional
        # this is executed when the object is double-clicked in the tree
        pass

    def unsetEdit(self,vobj,mode): #optional
        # this is executed when the user cancels or terminates edit mode
        pass

class CommandPathLoadTool:
    def GetResources(self):
        return {'Pixmap'  : 'Path-LoadTool',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_LoadTool","Tool Number to Load"),
                'Accel': "P, T",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_LoadTool","Tool Number to Load")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None
    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction(translate("Current Tool","Tool Number to Load"))
        FreeCADGui.addModule("PathScripts.PathLoadTool")
        snippet = '''
import Path
import PathScripts
from PathScripts import PathProject, PathUtils
prjexists = False
obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","Tool")
PathScripts.PathLoadTool.LoadTool(obj)

PathScripts.PathLoadTool._ViewProviderLoadTool(obj.ViewObject)

PathUtils.addToProject(obj)
'''
        FreeCADGui.doCommand(snippet)
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

if FreeCAD.GuiUp: 
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_LoadTool', CommandPathLoadTool())
    
FreeCAD.Console.PrintLog("Loading PathLoadTool... done\n")





