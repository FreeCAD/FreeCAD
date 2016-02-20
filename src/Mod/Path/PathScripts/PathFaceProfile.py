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

"""Path Profile object and FreeCAD command"""

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)


class ObjectFaceProfile:
    

    def __init__(self,obj):
        obj.addProperty("App::PropertyLinkSub","Base","Path","The base geometry of this object")
        obj.addProperty("App::PropertyDistance","Offset","Path","The distance between the face and the path")
        obj.addProperty("App::PropertyInteger","StartVertex","Path","The vertex index to start the path from")
        obj.addProperty("App::PropertyEnumeration","FirstMove","Path","The type of the first move")
        obj.FirstMove = ["G0","G1"]
        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None
        
    def execute(self,obj):
        if obj.Base:  
            import Part,DraftGeomUtils
            # we only consider the outer wire if this is a Face
            shape = getattr(obj.Base[0].Shape,obj.Base[1][0])
            if shape.ShapeType == "Wire":
                wire = shape
            else:
                wire = shape.OuterWire
            # we use the OCC offset feature
            if obj.Offset.Value != 0:
                offset = wire.makeOffset(obj.Offset.Value)
            else:
                offset = wire
            # absolute coords, millimeters, cancel offsets
            output = "G90\nG21\nG40\n"
            # reorder the wire
            offset = DraftGeomUtils.rebaseWire(offset,obj.StartVertex)
            # we create the path from the offset shape
            last = None
            for edge in offset.Edges:
                if not last:
                    # we set the first move to our first point
                    last = edge.Vertexes[0].Point
                    output += obj.FirstMove + " X" + str("%f" % last.x) + " Y" + str("%f" % last.y) + " Z" + str("%f" % last.z) + "\n"
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


class CommandPathFaceProfile:


    def GetResources(self):
        return {'Pixmap'  : 'Path-FaceProfile',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_FaceProfile","Face Profile"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_FaceProfile","Creates a profile object around a selected face")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None
        
    def Activated(self):
        
        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelectionEx()
        if len(selection) != 1:
            FreeCAD.Console.PrintError(translate("Path_FaceProfile","Please select one face or wire\n"))
            return
        if len(selection[0].SubObjects) != 1:
            FreeCAD.Console.PrintError(translate("Path_FaceProfile","Please select only one face or wire\n"))
            return
        if not selection[0].SubObjects[0].ShapeType in ["Face","Wire"]:
            FreeCAD.Console.PrintError(translate("Path_FaceProfile","Please select only a face or a wire\n"))
            return
        
        # if everything is ok, execute and register the transaction in the undo/redo stack
        FreeCAD.ActiveDocument.openTransaction("Create Profile")
        FreeCADGui.addModule("PathScripts.PathFaceProfile")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","FaceProfile")')
        FreeCADGui.doCommand('PathScripts.PathFaceProfile.ObjectFaceProfile(obj)')
        FreeCADGui.doCommand('obj.Base = (FreeCAD.ActiveDocument.'+selection[0].ObjectName+',"'+selection[0].SubElementNames[0]+'")')
        FreeCADGui.doCommand('obj.ViewObject.Proxy = 0')
        FreeCADGui.doCommand('PathScripts.PathUtils.addToProject(obj)')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp: 
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_FaceProfile',CommandPathFaceProfile())
