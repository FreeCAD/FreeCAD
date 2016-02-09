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
''' Used for CNC machine Tool Length Offsets ie G43H2'''

import FreeCAD,FreeCADGui,Path,PathGui
from PathScripts import PathProject,PathUtils
from PySide import QtCore,QtGui

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)

class ToolLenOffset:
    def __init__(self,obj):
        obj.addProperty("App::PropertyIntegerConstraint", "HeightNumber","HeightOffset", translate( "Height Offset Number",  "The Height offset number of the active tool"))
        obj.HeightNumber = (0,0,10000,1)
        obj.addProperty("App::PropertyLength", "Height", "HeightOffset", translate("Height","The first height value in Z, to rapid to, before making a feed move in Z"))
        obj.addProperty("App::PropertyBool","Active","HeightOffset",translate("Active","Make False, to prevent operation from generating code"))
        obj.Proxy = self
        mode = 2
        obj.setEditorMode('Placement',mode)

    def execute(self,obj):

        command = 'G43H'+str(obj.HeightNumber)+'G0Z'+str(obj.Height.Value)
        obj.Path = Path.Path(command)
        obj.Label = "Height"+str(obj.HeightNumber)
        if obj.Active:
            obj.Path = Path.Path(command)
            obj.ViewObject.Visibility = True
        else:
            obj.Path = Path.Path("(inactive operation)")
            obj.ViewObject.Visibility = False

        # tie the HeightNumber to the PathLoadTool object ToolNumber
        if len(obj.InList)>0: #check to see if obj is in the Project group yet
            project = obj.InList[0]
            tl = int(PathUtils.changeTool(obj,project))
            obj.HeightNumber= tl   

    def onChanged(self,obj,prop):
        if prop == "HeightNumber":
            obj.Label = "Height"+str(obj.HeightNumber)


class _ViewProviderTLO:
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
        return ":/icons/Path-LengthOffset.svg"

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


class CommandPathToolLenOffset:
    def GetResources(self):
        return {'Pixmap'  : 'Path-LengthOffset',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_ToolLenOffset","Tool Length Offset"),
                'Accel': "P, T",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_ToolLenOffset","Create a Tool Length Offset object")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction(translate("Path_ToolLenOffset","Create a Selection Plane object"))
        FreeCADGui.addModule("PathScripts.PathToolLenOffset")
        snippet = '''
import Path
import PathScripts
from PathScripts import PathProject,PathUtils
obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","HeightOffset")
PathScripts.PathToolLenOffset.ToolLenOffset(obj)
obj.Active = True
PathScripts.PathToolLenOffset._ViewProviderTLO(obj.ViewObject)
project = PathUtils.addToProject(obj)

tl = PathUtils.changeTool(obj,project)
if tl:
    obj.HeightNumber = tl
obj.ViewObject.ShowFirstRapid = False
FreeCAD.ActiveDocument.recompute()
'''
        FreeCADGui.doCommand(snippet)
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

if FreeCAD.GuiUp: 
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_ToolLenOffset', CommandPathToolLenOffset())
    
FreeCAD.Console.PrintLog("Loading PathToolLenOffset... done\n")




