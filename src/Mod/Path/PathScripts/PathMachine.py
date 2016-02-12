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
''' A CNC machine object to define how code is posted '''

import FreeCAD,Path
import PathScripts
from PathScripts import PathProject, PathUtils
from PySide import QtCore,QtGui
import os, sys    

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)

class Machine:
    def __init__(self,obj):

        obj.addProperty("App::PropertyString", "MachineName","Base",translate("Path_Machine","Name of the Machine that will use the CNC program"))

        obj.addProperty("App::PropertyFile", "PostProcessor", "CodeOutput", translate("Path_Machine","Select the Post Processor file for this machine"))
        obj.setEditorMode("PostProcessor",1) #set to read only
        obj.addProperty("App::PropertyEnumeration", "MachineUnits","CodeOutput", translate("Path_Machine","Units that the machine works in, ie Metric or Inch"))
        obj.MachineUnits=['Metric', 'Inch']

        obj.addProperty("Path::PropertyTooltable","Tooltable", "Base",translate("Path_Machine","The tooltable used for this CNC program")) 

        obj.addProperty("App::PropertyDistance", "X_Max", "Limits", translate("Path_Machine","The Maximum distance in X the machine can travel"))
        obj.addProperty("App::PropertyDistance", "Y_Max", "Limits", translate("Path_Machine","The Maximum distance in X the machine can travel"))
        obj.addProperty("App::PropertyDistance", "Z_Max", "Limits", translate("Path_Machine","The Maximum distance in X the machine can travel"))

        obj.addProperty("App::PropertyDistance", "X_Min", "Limits", translate("Path_Machine","The Minimum distance in X the machine can travel"))
        obj.addProperty("App::PropertyDistance", "Y_Min", "Limits", translate("Path_Machine","The Minimum distance in X the machine can travel"))
        obj.addProperty("App::PropertyDistance", "Z_Min", "Limits", translate("Path_Machine","The Minimum distance in X the machine can travel"))

        obj.addProperty("App::PropertyDistance", "X", "HomePosition", translate("Path_Machine","Home position of machine, in X (mainly for visualization)"))
        obj.addProperty("App::PropertyDistance", "Y", "HomePosition", translate("Path_Machine","Home position of machine, in Y (mainly for visualization)"))
        obj.addProperty("App::PropertyDistance", "Z", "HomePosition", translate("Path_Machine","Home position of machine, in Z (mainly for visualization)"))

        obj.Proxy = self
        mode = 2
        obj.setEditorMode('Placement',mode)

    def execute(self,obj):
        obj.Label = "Machine_"+str(obj.MachineName)
        gcode = 'G0 X'+str(obj.X.Value)+' Y'+str(obj.Y.Value)+' Z'+str(obj.Z.Value) #need to filter this path out in post- only for visualization
        obj.Path = Path.Path(gcode)

    def onChanged(self,obj,prop):
        mode = 2
        obj.setEditorMode('Placement',mode)

        if prop == "PostProcessor":
            sys.path.append(os.path.split(obj.PostProcessor)[0])
            lessextn = os.path.splitext(obj.PostProcessor)[0]
            postname = os.path.split(lessextn)[1]

            exec "import %s as current_post" % postname
            if hasattr (current_post, "UNITS"): 
                if current_post.UNITS == "G21":
                    obj.MachineUnits = "Metric"
                else:
                    obj.MachineUnits = "Inch"
            if hasattr (current_post, "MACHINE_NAME"): obj.MachineName = current_post.MACHINE_NAME

            if hasattr (current_post, "CORNER_MAX"):
                obj.X_Max = current_post.CORNER_MAX['x']
                obj.Y_Max = current_post.CORNER_MAX['y']
                obj.Z_Max = current_post.CORNER_MAX['z']

            if hasattr (current_post, "CORNER_MIN"): 
                obj.X_Min = current_post.CORNER_MIN['x']
                obj.Y_Min = current_post.CORNER_MIN['y']
                obj.Z_Min = current_post.CORNER_MIN['z']

        if prop == "Tooltable":
            proj = PathUtils.findProj()            
            for g in proj.Group:
                if not(isinstance(g.Proxy, PathScripts.PathMachine.Machine)):
                    g.touch()
 


class _ViewProviderMachine:
    def __init__(self,vobj):
        vobj.Proxy = self
        vobj.addProperty("App::PropertyBool","ShowLimits","Path",translate("ShowMinMaxTravel","Switch the machine max and minimum travel bounding box on/off"))
        mode = 2
        vobj.setEditorMode('LineWidth',mode)
        vobj.setEditorMode('MarkerColor',mode)
        vobj.setEditorMode('NormalColor',mode)
        vobj.setEditorMode('ShowFirstRapid',0)
        vobj.setEditorMode('DisplayMode',mode)
        vobj.setEditorMode('BoundingBox',mode)
        vobj.setEditorMode('Selectable',mode)
        
        
    def __getstate__(self): #mandatory
        return None

    def __setstate__(self,state): #mandatory
        return None

    def getIcon(self): #optional
        return ":/icons/Path-Machine.svg"

    def attach(self,vobj):
        from pivy import coin
        self.extentsBox = coin.SoSeparator()
        vobj.RootNode.addChild(self.extentsBox)
        
    def onChanged(self,vobj,prop):

        if prop == "ShowLimits":
            self.extentsBox.removeAllChildren()
            if vobj.ShowLimits and hasattr(vobj,"Object"):
                from pivy import coin
                parent = coin.SoType.fromName("SoSkipBoundingGroup").createInstance()
                self.extentsBox.addChild(parent)
                # set pattern
                pattern = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Part").GetInt("GridLinePattern",0x0f0f)
                defStyle = coin.SoDrawStyle()
                defStyle.lineWidth = 1
                defStyle.linePattern = pattern
                parent.addChild(defStyle)
                # set color
                c = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Path").GetUnsigned("DefaultExtentsColor",3418866943)
                r = float((c>>24)&0xFF)/255.0
                g = float((c>>16)&0xFF)/255.0
                b = float((c>>8)&0xFF)/255.0
                color = coin.SoBaseColor()
                parent.addChild(color)
                # set boundbox
                extents = coin.SoType.fromName("SoFCBoundingBox").createInstance()
                extents.coordsOn.setValue(False)
                extents.dimensionsOn.setValue(False)

                XMax, YMax, ZMax =vobj.Object.X_Max.Value , vobj.Object.Y_Max.Value , vobj.Object.Z_Max.Value
                XMin, YMin, ZMin =vobj.Object.X_Min.Value , vobj.Object.Y_Min.Value , vobj.Object.Z_Min.Value
                UnitParams = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units")

                extents.minBounds.setValue(XMax, YMax, ZMax)
                extents.maxBounds.setValue(XMin, YMin, ZMin)

                parent.addChild(extents)
        mode = 2
        vobj.setEditorMode('LineWidth',mode)
        vobj.setEditorMode('MarkerColor',mode)
        vobj.setEditorMode('NormalColor',mode)
        vobj.setEditorMode('ShowFirstRapid',0)
        vobj.setEditorMode('DisplayMode',mode)
        vobj.setEditorMode('BoundingBox',mode)
        vobj.setEditorMode('Selectable',mode)




    def updateData(self,vobj,prop): #optional
        # this is executed when a property of the APP OBJECT changes
        pass

    def setEdit(self,vobj,mode=0): #optional
        # this is executed when the object is double-clicked in the tree
        pass

    def unsetEdit(self,vobj,mode=0): #optional
        # this is executed when the user cancels or terminates edit mode
        pass
        
    def doubleClicked(self,vobj):
        from PathScripts import TooltableEditor
        TooltableEditor.edit(vobj.Object.Name)

class CommandPathMachine:
    def GetResources(self):
        return {'Pixmap'  : 'Path-Machine',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Machine","Machine Object"),
                'Accel': "P, M",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Machine","Create a Machine object")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction(translate("Path_Machine","Create a Machine object"))
        CommandPathMachine.Create()
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

    @staticmethod
    def Create():
        obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","Machine")
        Machine(obj)
        _ViewProviderMachine(obj.ViewObject)

        PathUtils.addToProject(obj)

        UnitParams = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units")
        if UnitParams.GetInt('UserSchema') == 0:
            obj.MachineUnits = 'Metric'
            #metric mode
        else:
            obj.MachineUnits = 'Inch'

        obj.ViewObject.ShowFirstRapid = False
        return obj

if FreeCAD.GuiUp: 
    # register the FreeCAD command
    import FreeCADGui
    FreeCADGui.addCommand('Path_Machine',CommandPathMachine())


FreeCAD.Console.PrintLog("Loading PathMachine... done\n")


