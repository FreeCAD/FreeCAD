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

"""Path Pocket object and FreeCAD command"""

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)


class ObjectFacePocket:
    

    def __init__(self,obj):
        obj.addProperty("App::PropertyLinkSub","Base","Path","The base geometry of this object")
        obj.addProperty("App::PropertyDistance","Offset","Path","The distance between the face and the path")
        obj.addProperty("App::PropertyInteger","StartVertex","Path","The vertex index to start the path from")
        obj.addProperty("App::PropertyEnumeration","FirstMove","Path","The type of the first move")
        obj.addProperty("App::PropertyDistance","RetractHeight","Path","The height to travel at between loops")
        obj.addProperty("App::PropertyBool","Fill","Path","Perform only one loop or fill the whole shape")
        obj.FirstMove = ["G0","G1"]
        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None
        
    def execute(self,obj):
        if obj.Base and obj.Offset.Value:
            import Part, DraftGeomUtils
            if "Face" in obj.Base[1][0]:
                shape = getattr(obj.Base[0].Shape,obj.Base[1][0])
            else:
                edges = [getattr(obj.Base[0].Shape,sub) for sub in obj.Base[1]]
                shape = Part.Wire(edges)
                
            # absolute coords, millimeters, cancel offsets
            output = "G90\nG21\nG40\n"
            
            # build offsets
            offsets = []
            nextradius = obj.Offset.Value
            result = DraftGeomUtils.pocket2d(shape,nextradius)
            while result:
                offsets.extend(result)
                if obj.Fill:
                    nextradius += obj.Offset.Value
                    result = DraftGeomUtils.pocket2d(shape,nextradius)
                else:
                    result = []
            
            first = True
            point = None
            
            # revert the list so we start with the outer wires
            offsets.reverse()
            
            # loop over successive wires
            while offsets:
                currentWire = offsets.pop()
                if first:
                    currentWire = DraftGeomUtils.rebaseWire(currentWire,obj.StartVertex)
                last = None
                for edge in currentWire.Edges:
                    if not last:
                        # we set the base GO to our first point
                        if first:
                            output += obj.FirstMove
                            first = False
                        else:
                            if obj.RetractHeight.Value and point:
                                output += "G0 X" + str("%f" % point.x) + " Y" + str("%f" % point.y) + " Z" + str("%f" % obj.RetractHeight.Value) + "\n"
                                last = edge.Vertexes[0].Point
                                output += "G0 X" + str("%f" % last.x) + " Y" + str("%f" % last.y) + " Z" + str("%f" % obj.RetractHeight.Value) + "\n"
                            output += "G1"
                        last = edge.Vertexes[0].Point
                        output += " X" + str("%f" % last.x) + " Y" + str("%f" % last.y) + " Z" + str("%f" % last.z) + "\n"
                    if isinstance(edge.Curve,Part.Circle):
                        point = edge.Vertexes[-1].Point
                        if point == last: # edges can come flipped
                            point = edge.Vertexes[0].Point
                        center = edge.Curve.Center
                        relcenter = center.sub(last)
                        v1 = last.sub(center)
                        v2 = point.sub(center)
                        if edge.Curve.Axis.z < 0:
                            output += "G2"
                        else:
                            output += "G3"
                        output += " X" + str("%f" % point.x) + " Y" + str("%f" % point.y) + " Z" + str("%f" % point.z)
                        output += " I" + str("%f" % relcenter.x) + " J" + str("%f" % relcenter.y) + " K" + str("%f" % relcenter.z)
                        output += "\n"
                        last = point
                    else:
                        point = edge.Vertexes[-1].Point
                        if point == last: # edges can come flipped
                            point = edge.Vertexes[0].Point
                        output += "G1 X" + str("%f" % point.x) + " Y" + str("%f" % point.y) + " Z" + str("%f" % point.z) + "\n"
                        last = point
                    
            #print output
            path = Path.Path(output)
            obj.Path = path


class CommandPathFacePocket:


    def GetResources(self):
        return {'Pixmap'  : 'Path-FacePocket',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_FacePocket","Face Pocket"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_FacePocket","Creates a pocket inside a loop of edges or a face")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None
        
    def Activated(self):
        
        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelectionEx()
        if len(selection) != 1:
            FreeCAD.Console.PrintError(translate("Path_FacePocket","Please select an edges loop from one object, or a single face\n"))
            return
        if len(selection[0].SubObjects) == 0:
            FreeCAD.Console.PrintError(translate("Path_FacePocket","Please select an edges loop from one object, or a single face\n"))
            return
        for s in selection[0].SubObjects:
            if s.ShapeType != "Edge":
                if (s.ShapeType != "Face") or (len(selection[0].SubObjects) != 1):
                    FreeCAD.Console.PrintError(translate("Path_FacePocket","Please select only edges or a single face\n"))
                    return
        if selection[0].SubObjects[0].ShapeType == "Edge":
            try:
                import Part
                w = Part.Wire(selection[0].SubObjects)
            except:
                FreeCAD.Console.PrintError(translate("Path_FacePocket","The selected edges don't form a loop\n"))
                return
        
        # if everything is ok, execute and register the transaction in the undo/redo stack
        FreeCAD.ActiveDocument.openTransaction("Create Pocket")
        FreeCADGui.addModule("PathScripts.PathFacePocket")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","FacePocket")')
        FreeCADGui.doCommand('PathScripts.PathFacePocket.ObjectFacePocket(obj)')
        subs = "["
        for s in selection[0].SubElementNames:
            subs += '"' + s + '",'
        subs += "]"
        FreeCADGui.doCommand('obj.Base = (FreeCAD.ActiveDocument.' + selection[0].ObjectName + ',' + subs + ')')
        FreeCADGui.doCommand('obj.ViewObject.Proxy = 0')
        FreeCADGui.doCommand('PathScripts.PathUtils.addToProject(obj)')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp: 
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_FacePocket',CommandPathFacePocket())
