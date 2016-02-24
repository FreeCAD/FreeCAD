# -*- coding: utf-8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2016 sliptonic <shopinthewoods@gmail.com>               *
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
from FreeCAD import Vector
from PySide import QtCore,QtGui
from PathScripts import PathUtils,PathSelection,PathProject

"""Path Surface object and FreeCAD command"""
'''Surface operation is used for 3D sculpting, roughing, and finishing
Possible algorithms include waterline, zigzag, and adaptive roughing.
Libraries to consider: opencamlib and libactp'''

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)


class ObjectProfile:

    def __init__(self,obj):
        obj.addProperty("App::PropertyLinkSub","Base","Path",translate("Parent Object","The base geometry of this toolpath"))
        obj.addProperty("App::PropertyBool","Active","Path",translate("Active","Make False, to prevent operation from generating code"))
        obj.addProperty("App::PropertyString","Comment","Path",translate("Comment","An optional comment for this profile"))

        obj.addProperty("App::PropertyEnumeration", "Algorithm", "Algorithm",translate("PathProject", "The library to use to generate the path"))
        obj.Algorithm = ['OCL Waterline']

        obj.addProperty("App::PropertyIntegerConstraint","ToolNumber","Tool",translate("PathProfile","The tool number in use"))
        obj.ToolNumber = (0,0,1000,1) 
        obj.setEditorMode('ToolNumber',1) #make this read only

        #Depth Properties
        obj.addProperty("App::PropertyDistance", "ClearanceHeight", "Depth", translate("Clearance Height","The height needed to clear clamps and obstructions"))
        obj.addProperty("App::PropertyDistance", "SafeHeight", "Depth", translate("PathProject","Rapid Safety Height between locations."))
        obj.addProperty("App::PropertyFloatConstraint", "StepDown", "Depth", translate("StepDown","Incremental Step Down of Tool"))
        obj.StepDown = (1,0.01,1000,0.5)
        obj.addProperty("App::PropertyDistance", "StartDepth", "Depth", translate("Start Depth","Starting Depth of Tool- first cut depth in Z"))
        obj.addProperty("App::PropertyDistance", "FinalDepth", "Depth", translate("Final Depth","Final Depth of Tool- lowest value in Z"))

        #Feed Properties
        obj.addProperty("App::PropertySpeed", "VertFeed", "Feed",translate("Vert Feed","Feed rate for vertical moves in Z"))
        obj.addProperty("App::PropertySpeed", "HorizFeed", "Feed",translate("Horiz Feed","Feed rate for horizontal moves"))
       
        #Start Point Properties
        obj.addProperty("App::PropertyVector","StartPoint","Start Point",translate("Start Point","The start point of this path"))
        obj.addProperty("App::PropertyBool","UseStartPoint","Start Point",translate("Use Start Point","make True, if specifying a Start Point"))
        obj.addProperty("App::PropertyLength", "ExtendAtStart", "Start Point", translate("extend at start", "extra length of tool path before start of part edge"))
        obj.addProperty("App::PropertyLength", "LeadInLineLen", "Start Point", translate("lead in length","length of straight segment of toolpath that comes in at angle to first part edge"))

        #End Point Properties
        obj.addProperty("App::PropertyBool","UseEndPoint","End Point",translate("Use End Point","make True, if specifying an End Point"))
        obj.addProperty("App::PropertyLength", "ExtendAtEnd", "End Point", translate("extend at end","extra length of tool path after end of part edge"))
        obj.addProperty("App::PropertyLength", "LeadOutLineLen", "End Point", translate("lead_out_line_len","length of straight segment of toolpath that comes in at angle to last part edge"))
        obj.addProperty("App::PropertyVector","EndPoint","End Point",translate("End Point","The end point of this path"))

        #Surface Properties

        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None

    def execute(self,obj):
        import Part, DraftGeomUtils
        from PathScripts.PathUtils import depth_params

        if obj.Base:
            # tie the toolnumber to the PathLoadTool object ToolNumber
            if len(obj.InList)>0: #check to see if obj is in the Project group yet
                project = obj.InList[0]
                tl = int(PathUtils.changeTool(obj,project))
                obj.ToolNumber= tl   
            
            tool = PathUtils.getTool(obj,obj.ToolNumber)
            if tool:
                radius = tool.Diameter/2
            else:
                # temporary value,in case we don't have any tools defined already
                radius = 0.25
            
            depthparams = depth_params (obj.ClearanceHeight.Value, obj.SafeHeight.Value, obj.StartDepth.Value, obj.StepDown, 0.0, obj.FinalDepth.Value, None)
            
            clearance = obj.ClearanceHeight.Value
            step_down=obj.StepDown
            start_depth=obj.StartDepth.Value
            final_depth=obj.FinalDepth.Value
            rapid_safety_space=obj.SafeHeight.Value

            vf=obj.VertFeed.Value
            hf=obj.HorizFeed.Value

            '''Parse the base and get the necessary geometry here'''

        
            if obj.Algorithm == "OCL Waterline":
                try:
                    import ocl
                except:
                    FreeCAD.Console.PrintError(translate("Path","OpenCAMLib needs to be installed for this command to work.\n"))
                    return

                output = ""
                output += '('+ str(obj.Comment)+')\n'

                output += self.buildoclwaterline()
           
            else:
                return

            
            
            if obj.Active:
                path = Path.Path(output)
                obj.Path = path
                obj.ViewObject.Visibility = True

            else:
                path = Path.Path("(inactive operation)")
                obj.Path = path
                obj.ViewObject.Visibility = False

    def buildoclwaterline(self):
        output = "Some text"
        return output

class ViewProviderProfile:

    def __init__(self,vobj):
        vobj.Proxy = self

    def attach(self,vobj):
        self.Object = vobj.Object
        return

    def getIcon(self):
        return ":/icons/Path-Surface.svg"

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None


class CommandPathProfile:
    def GetResources(self):
        return {'Pixmap'  : 'Path-Surface',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("PathSurface","Surface"),
                'Accel': "P, P",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("PathSurface","Creates a Path Surface object from selected solid")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None
        
    def Activated(self):
        import Path
        from PathScripts import PathProject, PathUtils

        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelectionEx()
        if len(selection) != 1:
            FreeCAD.Console.PrintError(translate("Path","Select a Solid.\n"))
            return
        if len(selection[0].SubObjects) == 0:
            FreeCAD.Console.PrintError(translate("Path","Select a Solid.\n"))
            return
        
        # if everything is ok, execute and register the transaction in the undo/redo stack

        # Take a guess at some reasonable values for Finish depth.
        bb = selection[0].Object.Shape.BoundBox  #parent boundbox
        fbb = selection[0].SubObjects[0].BoundBox #feature boundbox
        if fbb.ZMax < bb.ZMax:
            zbottom = fbb.ZMax
        else:
            zbottom = bb.ZMin

        FreeCAD.ActiveDocument.openTransaction(translate("Path","Create a Surfacing Operation"))
        FreeCADGui.addModule("PathScripts.PathSurface")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","Surface")')
        FreeCADGui.doCommand('PathScripts.PathSurface.ObjectProfile(obj)')

        FreeCADGui.doCommand('obj.Active = True')
        # subs ="["
        # for s in selection[0].SubElementNames:
        #     subs += '"' + s + '",'
        # subs += "]"
        # FreeCADGui.doCommand('obj.Base = (FreeCAD.ActiveDocument.' + selection[0].ObjectName + ',' + subs + ')')

        FreeCADGui.doCommand('obj.ViewObject.Proxy = 0')
        FreeCADGui.doCommand('obj.ClearanceHeight = ' + str(bb.ZMax + 10.0))
        FreeCADGui.doCommand('obj.StepDown = 1.0')
        FreeCADGui.doCommand('obj.StartDepth= ' + str(bb.ZMax))
        FreeCADGui.doCommand('obj.FinalDepth=' + str(zbottom))
        
        FreeCADGui.doCommand('obj.SafeHeight = '+ str(bb.ZMax + 2.0))
        FreeCADGui.doCommand('PathScripts.PathUtils.addToProject(obj)')

        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp: 
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Surface',CommandPathSurface())

FreeCAD.Console.PrintLog("Loading PathSurface... done\n")
