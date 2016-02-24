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
from PathScripts import PathUtils,PathSelection,PathProject

FreeCADGui = None
if FreeCAD.GuiUp:
    import FreeCADGui

"""Path Drilling object and FreeCAD command"""

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)


class ObjectDrilling:
    

    def __init__(self,obj):
        obj.addProperty("App::PropertyLinkSubList","Base","Path",translate("Parent Object(s)","The base geometry of this toolpath"))
        obj.addProperty("App::PropertyVectorList","locations","Path","The drilling locations")

        obj.addProperty("App::PropertyLength", "PeckDepth", "Depth", translate("PeckDepth","Incremental Drill depth before retracting to clear chips"))
        #obj.addProperty("App::PropertyFloat", "StartDepth", "Depth", translate("PathProject","Starting Depth of Tool- first cut depth in Z"))
        obj.addProperty("App::PropertyDistance", "ClearanceHeight", "Depth", translate("Clearance Height","The height needed to clear clamps and obstructions"))
        obj.addProperty("App::PropertyDistance", "FinalDepth", "Depth", translate("Final Depth","Final Depth of Tool- lowest value in Z"))
        obj.addProperty("App::PropertyDistance", "RetractHeight", "Depth", translate("Retract Height","The height where feed starts and height during retract tool when path is finished"))
        obj.addProperty("App::PropertyLength", "VertFeed", "Feed",translate("Vert Feed","Feed rate for vertical moves in Z"))
        obj.addProperty("App::PropertyString","Comment","Path",translate("PathProject","An optional comment for this profile"))
        obj.addProperty("App::PropertyBool","Active","Path",translate("Active","Make False, to prevent operation from generating code"))
        obj.addProperty("App::PropertyIntegerConstraint","ToolNumber","Tool",translate("PathProfile","The tool number in use"))
        obj.ToolNumber = (0,0,1000,1) 
        obj.setEditorMode('ToolNumber',1) #make this read only
        
        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None
        
    def execute(self,obj):
        locations = []
        for loc in obj.Base:

            if "Face" in loc[1] or "Edge" in loc[1]:    
                s = getattr(loc[0].Shape,loc[1])
            else:
                s = loc[0].Shape

            if s.ShapeType in ['Face', 'Wire', 'Edge' ]:
                    X = s.Edges[0].Curve.Center.x
                    Y = s.Edges[0].Curve.Center.y
                    Z = s.Edges[0].Curve.Center.z
            elif s.ShapeType == 'Vertex':
                    X = s.Point.x
                    Y = s.Point.y
                    Z = s.Point.z

            locations.append(FreeCAD.Vector(X,Y,Z))

        # tie the toolnumber to the PathLoadTool object ToolNumber
        if len(obj.InList)>0: #check to see if obj is in the Project group yet
            project = obj.InList[0]
            tl = int(PathUtils.changeTool(obj,project))
            obj.ToolNumber= tl   
        
        tool = PathUtils.getTool(obj,obj.ToolNumber)

        output = "G90 G98\n"
        # rapid to clearance height
        output += "G0 Z" + str(obj.ClearanceHeight.Value)
        # rapid to first hole location, with spindle still retracted:
        p0 = locations[0]
        output += "G0 X"+str(p0.x) + " Y" + str(p0.y)+ "\n"
        # move tool to clearance plane
        output += "G0 Z" + str(obj.ClearanceHeight.Value) + "\n"
        if obj.PeckDepth.Value > 0:
            cmd = "G83"
            qword = " Q"+ str(obj.PeckDepth.Value)
        else:
            cmd = "G81"
            qword = ""
        for p in locations:

            output += cmd + " X" + str(p.x) + " Y" + str(p.y) + " Z" + str(obj.FinalDepth.Value) + qword + " R" + str(obj.RetractHeight.Value) + " F" + str(obj.VertFeed.Value) + "\n"

        output += "G80\n"

        path = Path.Path(output)
        obj.Path = path

        # tie the toolnumber to the PathLoadTool object ToolNumber
        if len(obj.InList)>0: #check to see if obj is in the Project group yet
            project = obj.InList[0]
            tl = int(PathUtils.changeTool(obj,project))
            obj.ToolNumber= tl   

class ViewProviderDrill:
    def __init__(self,obj): #mandatory
#        obj.addProperty("App::PropertyFloat","SomePropertyName","PropertyGroup","Description of this property")
        obj.Proxy = self

    def __getstate__(self): #mandatory
        return None

    def __setstate__(self,state): #mandatory
        return None

    def getIcon(self): #optional
        return ":/icons/Path-Drilling.svg"

#    def attach(self): #optional
#        # this is executed on object creation and object load from file
#        pass

    def onChanged(self,obj,prop): #optional
        # this is executed when a property of the VIEW PROVIDER changes
        pass

    def updateData(self,obj,prop): #optional
        # this is executed when a property of the APP OBJECT changes
        pass

    def setEdit(self,vobj,mode): #optional
        # this is executed when the object is double-clicked in the tree
        pass

    def unsetEdit(self,vobj,mode): #optional
        # this is executed when the user cancels or terminates edit mode
        pass



class CommandPathDrilling:


    def GetResources(self):
        return {'Pixmap'  : 'Path-Drilling',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Drilling","Drilling"),
                'Accel': "P, D",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Drilling","Creates a Path Drilling object")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None

    def Drillable(self, obj):
        import Part
        drillable = ""
        if obj.ShapeType == 'Vertex': 
            drillable = 'Vertex'
        elif obj.ShapeType == 'Edge': 
            if isinstance(obj.Curve, Part.Circle):
                drillable = 'Circle'          
        elif obj.ShapeType == 'Face': 
            if isinstance(obj.Edges[0].Curve, Part.Circle):
                drillable = 'Circle'            
        elif obj.ShapeType == 'Wire': 
            if isinstance(obj.Edges[0].Curve, Part.Circle):
                drillable = 'Circle'
        else:
            drillable = None
        return drillable

    def Activated(self):
        import Path
        import Part

        from PathScripts import PathUtils,PathDrilling,PathProject
        prjexists = False

        selection = FreeCADGui.Selection.getSelectionEx()
        if not selection:
            FreeCAD.Console.PrintError(translate("PathDrilling","Please select points or cirlces for drilling.\n"))
            return
            
        diamcount = 0 #keep track of how many different hole sizes we're asked to deal with
        lastradius = 0.0
        locations = []
        vertexcount = 0 #keep track of how many vertices

        for s in selection:
            if s.HasSubObjects:
                for i in s.SubObjects:
                    d = self.Drillable (i)
                    if d == 'Circle':
                        if i.Edges[0].Curve.Radius != lastradius:
                            diamcount += 1
                            lastradius = i.Edges[0].Curve.Radius
                    elif d == 'Vertex':
                        vertexcount += 1
                    else:
                        FreeCAD.Console.PrintError(translate("PathDrilling","No drillable locations were selected.\n"))
                        return                    
                    
                    #subs = []
                    for n in s.SubElementNames:
                        # subs.append(n)
                        locations.append((s.ObjectName, s.Object, n))

            else:
                d = self.Drillable (s.Object.Shape)
                if d == 'Circle':
                    if not str(s.Object.Shape.Edges[0].Curve.Radius) == str(lastradius):
                        diamcount += 1
                        lastradius = s.Object.Shape.Edges[0].Curve.Radius

                elif d == 'Vertex':
                    vertexcount += 1
                else:
                    FreeCAD.Console.PrintError(translate("PathDrilling","No drillable locations were selected.\n"))
                    return

                locations.append((s.ObjectName, s.Object, []))

        if diamcount > 1:
            FreeCAD.Console.PrintError(translate("PathDrilling","Circles of different radii found. Select only one size circle at a time.\n"))
            return

        if diamcount >= 1 and vertexcount >= 1:
            FreeCAD.Console.PrintError(translate("PathDrilling","Please select either points or circles but not both.\n"))
            return

        # Take a guess at some reasonable values for Finish depth.
        if selection[0].HasSubObjects:
            bb = selection[0].Object.Shape.BoundBox  #parent boundbox
            fbb = selection[0].SubObjects[0].BoundBox #feature boundbox
            if fbb.ZMax < bb.ZMax:
                zbottom = fbb.ZMax
                ztop = bb.ZMax + 1
            else:
                zbottom = bb.ZMin
                ztop = 5
        else:
            zbottom = 0
            ztop = 5


        # if everything is ok, execute and register the transaction in the undo/redo stack
        FreeCAD.ActiveDocument.openTransaction(translate("Path_Drilling","Create Drilling"))
        FreeCADGui.addModule("PathScripts.PathDrilling")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","Drilling")')
        FreeCADGui.doCommand('PathScripts.PathDrilling.ObjectDrilling(obj)')
        FreeCADGui.doCommand('obj.Active = True')
        FreeCADGui.doCommand('PathScripts.PathDrilling.ViewProviderDrill(obj.ViewObject)')

        baselist = "["
        for loc in locations:
            baselist += "(FreeCAD.ActiveDocument." + str(loc[0]) + ',"' + str(loc[2]) + '"),'  
        baselist += "]"

        FreeCADGui.doCommand('obj.Base = (' + baselist  + ')')
        FreeCADGui.doCommand('from PathScripts import PathUtils')
        FreeCADGui.doCommand('obj.ClearanceHeight = ' + str(ztop))
        FreeCADGui.doCommand('obj.RetractHeight= ' + str(ztop))
        FreeCADGui.doCommand('obj.FinalDepth=' + str(zbottom))
        FreeCADGui.doCommand('PathScripts.PathUtils.addToProject(obj)')

        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp: 
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Drilling',CommandPathDrilling())

FreeCAD.Console.PrintLog("Loading PathDrilling... done\n")
