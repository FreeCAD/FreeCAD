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
'''PathKurve - Path Profile operation using libarea (created by Dan Heeks) for making simple CNC paths. 
libarea, originally from HeeksCNC project must be present for this to work.'''


import FreeCAD,FreeCADGui,Path,PathGui
from PathScripts import PathProject,PathUtils,PathKurveUtils,PathSelection
from PySide import QtCore,QtGui

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)

class PathProfile:
    def __init__(self,obj):
        obj.addProperty("App::PropertyLinkSub","Base","Path",translate("Parent Object","The base geometry of this toolpath"))
        obj.addProperty("App::PropertyLinkSub","StartPoint", "Path", translate("Start Point","Linked Start Point of Profile"))
        obj.addProperty("App::PropertyLinkSub","EndPoint", "Path", translate("End Point","Linked End Point of Profile"))
        obj.addProperty("App::PropertyBool","Active","Path",translate("Active","Make False, to prevent operation from generating code"))
        obj.addProperty("App::PropertyString","Comment","Path",translate("Comment","An optional comment for this profile"))

        obj.addProperty("App::PropertyIntegerConstraint","ToolNumber","Tool",translate("PathProfile","The tool number in use"))
        obj.ToolNumber = (0,0,1000,1)
        obj.setEditorMode('ToolNumber',1) #make this read only
        #Depth Properties
        obj.addProperty("App::PropertyDistance", "ClearanceHeight", "Depth", translate("Clearance Height","The height needed to clear clamps and obstructions"))
        obj.addProperty("App::PropertyLength", "StepDown", "Depth", translate("StepDown","Incremental Step Down of Tool"))
#        obj.addProperty("App::PropertyBool","UseStartDepth","Depth",translate("Use Start Depth","make True, if manually specifying a Start Start Depth"))
        obj.addProperty("App::PropertyDistance", "StartDepth", "Depth", translate("Start Depth","Starting Depth of Tool- first cut depth in Z"))
        obj.addProperty("App::PropertyDistance", "FinalDepth", "Depth", translate("Final Depth","Final Depth of Tool- lowest value in Z"))
        obj.addProperty("App::PropertyDistance", "RetractHeight", "Depth", translate("Retract Height","The height desired to retract tool when path is finished"))

        #Feed Properties
        obj.addProperty("App::PropertyLength", "VertFeed", "Feed",translate("Vert Feed","Feed  rate (in units per minute) for vertical moves in Z"))
        obj.addProperty("App::PropertyLength", "HorizFeed", "Feed",translate("Horiz Feed","Feed rate (in units per minute) for horizontal moves"))
       
        #Profile Properties
        obj.addProperty("App::PropertyEnumeration", "Side", "Profile", translate("Side","Side of edge that tool should cut"))
        obj.Side = ['left','right','on'] #side of profile that cutter is on in relation to direction of profile
        obj.addProperty("App::PropertyEnumeration", "Direction", "Profile",translate("Direction", "The direction that the toolpath should go around the part ClockWise CW or CounterClockWise CCW"))
        obj.Direction = ['CW','CCW'] #this is the direction that the profile runs
        obj.addProperty("App::PropertyBool","UseComp","Profile",translate("Use Cutter Comp","make True, if using Cutter Radius Compensation"))
        obj.addProperty("App::PropertyIntegerList","Edgelist","Profile",translate("Edge List", "List of edges selected"))
        obj.addProperty("App::PropertyDistance", "OffsetExtra", "Profile",translate("OffsetExtra","Extra value to stay away from final profile- good for roughing toolpath"))
#        obj.addProperty("App::PropertyLength", "SegLen", "Profile",translate("Seg Len","Tesselation  value for tool paths made from beziers, bsplines, and ellipses"))

#        #Start Point Properties
        obj.addProperty("App::PropertyString","StartPtName","Profile",translate("Start Point","The name of the start point of this path"))
        obj.addProperty("App::PropertyBool","UseStartPt","Profile",translate("Use Start Point","Make True, if specifying a Start Point"))
#        obj.addProperty("App::PropertyLength", "ExtendAtStart", "Profile", translate("extend at start", "extra length of tool path before start of part edge"))
#        obj.addProperty("App::PropertyLength", "LeadInLineLen", "Profile", translate("lead in length","length of straight segment of toolpath that comes in at angle to first part edge"))

#        #End Point Properties
        obj.addProperty("App::PropertyString","EndPtName","Profile",translate("End Point","The name of the end point of this path"))
        obj.addProperty("App::PropertyBool","UseEndPt","Profile",translate("Use End Point","Make True, if specifying an End Point"))
#        obj.addProperty("App::PropertyLength", "ExtendAtEnd", "Profile", translate("extend at end","extra length of tool path after end of part edge"))
#        obj.addProperty("App::PropertyLength", "LeadOutLineLen", "Profile", translate("lead_out_line_len","length of straight segment of toolpath that comes in at angle to last edge selected"))

#        obj.addProperty("App::PropertyDistance", "RollRadius", "Profile", translate("Roll Radius","Radius at start and end"))

        obj.Proxy = self




    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None

    def execute(self,obj):

        if obj.Base:
            
            # tie the toolnumber to the PathLoadTool object ToolNumber
            if len(obj.InList)>0: #check to see if obj is in the Project group yet
                project = obj.InList[0]
                tl = int(PathUtils.changeTool(obj,project))
                obj.ToolNumber= tl   
            
            tool = PathUtils.getTool(obj,obj.ToolNumber)
            if tool:
                self.radius = tool.Diameter/2
            else:
                # temporary value,in case we don't have any tools defined already
                self.radius = 0.25
#            self.radius = 0.25
            self.clearance = obj.ClearanceHeight.Value
            self.step_down=obj.StepDown.Value
            self.start_depth=obj.StartDepth.Value
            self.final_depth=obj.FinalDepth.Value
            self.rapid_safety_space=obj.RetractHeight.Value
            self.side=obj.Side
            self.offset_extra=obj.OffsetExtra.Value
            self.use_CRC=obj.UseComp
            self.vf=obj.VertFeed.Value
            self.hf=obj.HorizFeed.Value


            edgelist = []

            if obj.StartPtName and obj.UseStartPt:             
                self.startpt = FreeCAD.ActiveDocument.getObject(obj.StartPtName).Shape
            else:
                self.startpt = None

            if obj.EndPtName and obj.UseEndPt:
                self.endpt = FreeCAD.ActiveDocument.getObject(obj.EndPtName).Shape
            else:
                self.endpt = None
                
            for e in obj.Edgelist:
                edgelist.append(FreeCAD.ActiveDocument.getObject(obj.Base[0].Name).Shape.Edges[e-1])

            output=PathKurveUtils.makePath(edgelist,self.side,self.radius,self.vf,self.hf,self.offset_extra, \
                   self.rapid_safety_space,self.clearance,self.start_depth,self.step_down, \
                   self.final_depth,self.use_CRC,obj.Direction,self.startpt,self.endpt)

            if obj.Active:
                path = Path.Path(output)
                obj.Path = path
                obj.ViewObject.Visibility = True

            else:
                path = Path.Path("(inactive operation)")
                obj.Path = path
                obj.ViewObject.Visibility = False



class _ViewProviderKurve:

    def __init__(self,vobj): #mandatory
        vobj.Proxy = self
    def __getstate__(self): #mandatory
        return None

    def __setstate__(self,state): #mandatory
        return None

    def getIcon(self): #optional
        return ":/icons/Path-Kurve.svg"


class CommandPathKurve:
    def GetResources(self):
        return {'Pixmap'  : 'Path-Kurve',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Kurve","Profile"),
                'Accel': "P, P",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Kurve","Creates a Path Profile object from selected edges, using libarea for offset algorithm")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None
        
    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction(translate("Path_Kurve","Create a Profile operation using libarea"))
        FreeCADGui.addModule("PathScripts.PathKurve")
        snippet = '''
import Path
from PathScripts import PathSelection,PathProject,PathUtils
import area

def profileop():
    selection = PathSelection.multiSelect()

    if not selection:
        FreeCAD.Console.PrintError('please select some edges\\n')

    obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","Profile")
    PathScripts.PathKurve.PathProfile(obj)

    obj.Active = True
    PathScripts.PathKurve._ViewProviderKurve(obj.ViewObject)

    obj.Base = (FreeCAD.ActiveDocument.getObject(selection['objname']))

    elist = []
    for e in selection['edgenames']:
        elist.append(eval(e.lstrip('Edge')))

    obj.Edgelist = elist
    if selection['pointnames']:
        FreeCAD.Console.PrintMessage('There are points selected.\\n')
        if len(selection['pointnames'])>1:
            obj.StartPtName = selection['pointnames'][0]
            obj.StartPoint= FreeCAD.ActiveDocument.getObject(obj.StartPtName)
            
            obj.EndPtName = selection['pointnames'][-1]
            obj.EndPoint=FreeCAD.ActiveDocument.getObject(obj.EndPtName)
            
        else:            
            obj.StartPtName = selection['pointnames'][0]
            obj.StartPoint= FreeCAD.ActiveDocument.getObject(obj.StartPtName)
            
    obj.ClearanceHeight = 2.0
    obj.StepDown = 1.0
    obj.StartDepth=0.0
    obj.FinalDepth=-1.0
    obj.RetractHeight = 5.0
    obj.Side = 'left'
    obj.OffsetExtra = 0.0
    if selection['clockwise']:
        obj.Direction = 'CW'
    else:
        obj.Direction = 'CCW'
    obj.UseComp = False

    project = PathUtils.addToProject(obj)

    tl = PathUtils.changeTool(obj,project)

    if tl:
        obj.ToolNumber = tl


from PathScripts import PathProject,PathUtils,PathKurve, PathKurveUtils,PathSelection
try:
    import area
    
except:
    FreeCAD.Console.PrintError('libarea needs to be installed for this command to work\\n')
profileop()
'''

        FreeCADGui.doCommand(snippet)
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()

if FreeCAD.GuiUp: 
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Kurve',CommandPathKurve())

FreeCAD.Console.PrintLog("Loading PathKurve... done\n")





























