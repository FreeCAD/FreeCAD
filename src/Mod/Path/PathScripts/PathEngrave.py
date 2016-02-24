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

"""Path Engrave object and FreeCAD command"""

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)


class ObjectPathEngrave:
    

    def __init__(self,obj):
        obj.addProperty("App::PropertyLinkSub","Base","Path","The base geometry of this object")

        obj.addProperty("App::PropertyEnumeration", "Algorithm", "Algorithm",translate("Path", "The library or Algorithm used to generate the path"))
        obj.Algorithm = ['OCC Native']

        #Depth Properties
        obj.addProperty("App::PropertyDistance", "ClearanceHeight", "Depth", translate("Path","The height needed to clear clamps and obstructions"))
        obj.addProperty("App::PropertyDistance", "SafeHeight", "Depth", translate("Path","Rapid Safety Height between locations."))
        obj.addProperty("App::PropertyDistance", "StartDepth", "Depth", translate("Path","Starting Depth of Tool- first cut depth in Z"))
        obj.addProperty("App::PropertyDistance", "FinalDepth", "Depth", translate("Path","Final Depth of Tool- lowest value in Z"))
        obj.addProperty("App::PropertyInteger","StartVertex","Path","The vertex index to start the path from")
        #Feed Properties
        obj.addProperty("App::PropertySpeed", "VertFeed", "Feed",translate("Path","Feed rate for vertical moves in Z"))
        obj.addProperty("App::PropertySpeed", "HorizFeed", "Feed",translate("Path","Feed rate for horizontal moves"))


        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None
        
    def execute(self,obj):
        if obj.Base:  
            # we only consider the outer wire if this is a Face
            wires = obj.Base[0].Shape.Wires

            output = ""
            if obj.Algorithm == "OCC Native":
                output += self.buildpathocc(obj, wires)

            #print output
            path = Path.Path(output)
            obj.Path = path

    def buildpathocc(self, obj, wires):
        import Part,DraftGeomUtils
        output = "G90\nG21\nG40\n"
        output += "G0 Z" + str(obj.ClearanceHeight.Value)

        # absolute coords, millimeters, cancel offsets

        for wire in wires:
            offset = wire
        
            # reorder the wire
            offset = DraftGeomUtils.rebaseWire(offset,obj.StartVertex)
            
            # we create the path from the offset shape
            last = None
            for edge in offset.Edges:
                if not last:
                    # we set the first move to our first point
                    last = edge.Vertexes[0].Point
                    output += "G0" + " X" + str("%f" % last.x) + " Y" + str("%f" % last.y) #Rapid sto starting position
                    output += "G1" + " Z" + str("%f" % last.z) +"F " + str(obj.VertFeed.Value)+ "\n" #Vertical feed to depth
                if isinstance(edge.Curve,Part.Circle):
                    point = edge.Vertexes[-1].Point
                    if point == last: # edges can come flipped
                        point = edge.Vertexes[0].Point
                    center = edge.Curve.Center
                    relcenter = center.sub(last)
                    v1 = last.sub(center)
                    v2 = point.sub(center)
                    if v1.cross(v2).z < 0:
                        output += "G2"
                    else:
                        output += "G3"
                    output += " X" + str("%f" % point.x) + " Y" + str("%f" % point.y) + " Z" + str("%f" % point.z)
                    output += " I" + str("%f" % relcenter.x) + " J" + str("%f" % relcenter.y) + " K" + str("%f" % relcenter.z)
                    output +=  " F " + str(obj.HorizFeed.Value)
                    output += "\n"
                    last = point
                else:
                    point = edge.Vertexes[-1].Point
                    if point == last: # edges can come flipped
                        point = edge.Vertexes[0].Point
                    output += "G1 X" + str("%f" % point.x) + " Y" + str("%f" % point.y) + " Z" + str("%f" % point.z)
                    output +=  " F " + str(obj.HorizFeed.Value)
                    output +=  "\n"
                    last = point
            output += "G0 Z " + str(obj.SafeHeight.Value)
        return output

class CommandPathEngrave:


    def GetResources(self):
        return {'Pixmap'  : 'Path-Engrave',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Engrave","ShapeString Engrave"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Engrave","Creates an Engraving Path around a Draft ShapeString")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None
        
    def Activated(self):
        import Draft
        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelectionEx()
        if len(selection) != 1:
            FreeCAD.Console.PrintError(translate("Path_Engrave","Please select one ShapeString\n"))
            return
        if len(selection[0].SubObjects) != 0:
            FreeCAD.Console.PrintError(translate("Path_Engrave","Please select one ShapeString\n"))
            return
        if not Draft.getType(selection[0].Object) == "ShapeString":
            FreeCAD.Console.PrintError(translate("Path_Engrave","Please select one ShapeString\n"))
            return
        
        # if everything is ok, execute and register the transaction in the undo/redo stack
        FreeCAD.ActiveDocument.openTransaction("Create Engrave Path")
        FreeCADGui.addModule("PathScripts.PathFaceProfile")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","PathEngrave")')
        FreeCADGui.doCommand('PathScripts.PathEngrave.ObjectPathEngrave(obj)')
        FreeCADGui.doCommand('obj.Base = (FreeCAD.ActiveDocument.'+selection[0].ObjectName+',[])')
        FreeCADGui.doCommand('obj.ClearanceHeight = 10')
        FreeCADGui.doCommand('obj.StartDepth= 0')
        FreeCADGui.doCommand('obj.FinalDepth= -0.1' )
        FreeCADGui.doCommand('obj.SafeHeight= 5.0' )

        FreeCADGui.doCommand('obj.ViewObject.Proxy = 0')
        FreeCADGui.doCommand('PathScripts.PathUtils.addToProject(obj)')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp: 
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Engrave',CommandPathEngrave())
