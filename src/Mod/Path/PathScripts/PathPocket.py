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
from PathScripts import PathUtils

FreeCADGui = None
if FreeCAD.GuiUp:
    import FreeCADGui

"""Path Pocket object and FreeCAD command"""

# Qt tanslation handling
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def translate(context, text, disambig=None):
        return QtGui.QApplication.translate(context, text, disambig)

def frange(start, stop, step, finish):
    x = []
    curdepth = start
    if step == 0:
        return x
    # do the base cuts until finishing round
    while curdepth >= stop + step + finish:
        curdepth = curdepth - step
        if curdepth <= stop + finish:
            curdepth = stop + finish
        x.append(curdepth)

    # we might have to do a last pass or else finish round might be too far away
    if curdepth - stop > finish:
        x.append(stop + finish)

    # do the the finishing round
    if curdepth >= stop:
        curdepth = stop
        x.append(curdepth)

     # Why this?
#    if start >= stop:
#        start = stop
#        x.append (start)

    return x

class ObjectPocket:
    

    def __init__(self,obj):
        obj.addProperty("App::PropertyLinkSub","Base","Path",translate("PathProject","The base geometry of this object"))
        obj.addProperty("App::PropertyIntegerConstraint","ToolNumber","Tool",
                        translate("PathProfile","The tool number in use"))
        obj.ToolNumber = (0, 0, 1000, 0)


        obj.addProperty("App::PropertyFloat", "ClearanceHeight", "Pocket", translate("PathProject","The height needed to clear clamps and obstructions"))
        obj.addProperty("App::PropertyFloatConstraint", "StepDown", "Pocket", translate("PathProject","Incremental Step Down of Tool"))
        obj.StepDown = (0.0, 0.0, 100.0, 1.0)

        obj.addProperty("App::PropertyFloat", "StartDepth", "Pocket", translate("PathProject","Starting Depth of Tool- first cut depth in Z"))
        obj.addProperty("App::PropertyBool","UseStartDepth","Pocket",translate("PathProject","make True, if manually specifying a Start Start Depth"))
        obj.addProperty("App::PropertyFloat", "FinalDepth", "Pocket", translate("PathProject","Final Depth of Tool- lowest value in Z"))
        obj.addProperty("App::PropertyFloat", "RetractHeight", "Pocket", translate("PathProject","The height desired to retract tool when path is finished"))

        obj.addProperty("App::PropertyEnumeration", "CutMode", "Pocket",translate("PathProject", "The direction that the toolpath should go around the part ClockWise CW or CounterClockWise CCW"))
        obj.CutMode = ['Climb','Conventional']
        obj.addProperty("App::PropertyFloat", "MaterialAllowance", "Pocket", translate("PathProject","Amount of material to leave"))
        obj.addProperty("App::PropertyFloat", "FinishDepth", "Pocket", translate("PathProject","Maximum material removed on final pass."))

        obj.addProperty("App::PropertyEnumeration", "StartAt", "Pocket",translate("PathProject", "Start pocketing at center or boundary"))
        obj.StartAt = ['Center', 'Edge']

        obj.addProperty("App::PropertyFloatConstraint", "VertFeed", "Feed",translate("Vert Feed","Feed rate for vertical moves in Z"))
        obj.VertFeed = (0.0, 0.0, 100000.0, 1.0)

        obj.addProperty("App::PropertyFloatConstraint", "HorizFeed", "Feed",translate("Horiz Feed","Feed rate for horizontal moves"))
        obj.HorizFeed = (0.0, 0.0, 100000.0, 1.0)


        obj.addProperty("App::PropertyBool","Active","Path",translate("PathProject","Make False, to prevent operation from generating code"))
        obj.addProperty("App::PropertyString","Comment","Path",translate("PathProject","An optional comment for this profile"))

        obj.Proxy = self

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None
        

    def getStock(self,obj):
        "retrieves a stock object from hosting project if any"
        for o in obj.InList:
            if hasattr(o,"Group"):
                for g in o.Group:
                    if hasattr(g,"Height_Allowance"):
                        return o
        # not found? search one level up
        for o in obj.InList:
            return self.getStock(o)
        return None



    def execute(self,obj):
        if obj.Base:
            tool = PathUtils.getLastTool(obj)

            if tool:
                radius = tool.Diameter/2
                if radius < 0:# safe guard
                    radius -= radius
            else:
                # temporary value, to be taken from the properties later on
                radius = 1

            import Part, DraftGeomUtils
            if "Face" in obj.Base[1][0]:
                shape = getattr(obj.Base[0].Shape,obj.Base[1][0])
            else:
                edges = [getattr(obj.Base[0].Shape,sub) for sub in obj.Base[1]]
                shape = Part.Wire(edges)
                print len(edges)

            # absolute coords, millimeters, cancel offsets
            output = "G90\nG21\nG40\n"
            # save tool
            if obj.ToolNumber > 0 and tool.ToolNumber != obj.ToolNumber:
                output += "M06 T" + str(tool.ToolNumber) + "\n"

            # build offsets
            offsets = []
            nextradius = radius
            result = DraftGeomUtils.pocket2d(shape,nextradius)
            while result:
                offsets.extend(result)
                nextradius += radius
                result = DraftGeomUtils.pocket2d(shape,nextradius)
            
            # first move will be rapid, subsequent will be at feed rate
            first = True
            startPoint = None
            fastZPos = max(obj.StartDepth + 2, obj.RetractHeight)
            
            # revert the list so we start with the outer wires
            if obj.StartAt != 'Edge':
                offsets.reverse()

#            print "startDepth: " + str(obj.StartDepth)
#            print "finalDepth: " + str(obj.FinalDepth)
#            print "stepDown: " + str(obj.StepDown)
#            print "finishDepth" + str(obj.FinishDepth)
#            print "offsets:", len(offsets)

            def prnt(vlu): return str(round(vlu, 4))

            for vpos in frange(obj.StartDepth, obj.FinalDepth, obj.StepDown, obj.FinishDepth):
#                print "vpos: " + str(vpos)
                # loop over successive wires
                for currentWire in offsets:
#                    print "new line (offset)"
                    last = None
                    for edge in currentWire.Edges:
#                        print "new edge"
                        if not last:
                            # we set the base GO to our fast move to our starting pos
                            if first:
                                startPoint = edge.Vertexes[0].Point
                                output += "G0 X" + prnt(startPoint.x) + " Y" + prnt(startPoint.y) +\
                                          " Z" + prnt(fastZPos) + "\n"
                                first = False
                            #then move slow down to our starting point for our profile
                            last = edge.Vertexes[0].Point
                            output += "G1 X" + prnt(last.x) + " Y" + prnt(last.y) + " Z" + prnt(vpos) + "\n"
                        if isinstance(edge.Curve,Part.Circle):
                            point = edge.Vertexes[-1].Point
                            if point == last: # edges can come flipped
                                point = edge.Vertexes[0].Point
#                                print "flipped"
                            center = edge.Curve.Center
                            relcenter = center.sub(last)
                            v1 = last.sub(center)
                            v2 = point.sub(center)
                            if v1.cross(v2).z < 0:
                                output += "G2"
                            else:
                                output += "G3"
                            output += " X" + prnt(point.x) + " Y" + prnt(point.y) + " Z" + prnt(vpos)
                            output += " I" + prnt(relcenter.x) + " J" +prnt(relcenter.y) + " K" + prnt(relcenter.z)
                            output += "\n"
                            last = point
                        else:
                            point = edge.Vertexes[-1].Point
                            if point == last: # edges can come flipped
                                point = edge.Vertexes[0].Point
                            output += "G1 X" + prnt(point.x) + " Y" + prnt(point.y) + " Z" + prnt(vpos) + "\n"
                            last = point

            #move back up
            output += "G1 Z" + prnt(fastZPos) + "\n"
#            print output
#            path = Path.Path(output)
#            obj.Path = path
            if obj.Active:
                path = Path.Path(output)
                obj.Path = path
                obj.ViewObject.Visibility = True
            else:
                path = Path.Path("(inactive operation)")
                obj.Path = path
                obj.ViewObject.Visibility = False


class ViewProviderPocket:

    def __init__(self,vobj):
        vobj.Proxy = self

    def attach(self,vobj):
        self.Object = vobj.Object
        return

    def getIcon(self):
        return ":/icons/Path-Pocket.svg"

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None


class CommandPathPocket:


    def GetResources(self):
        return {'Pixmap'  : 'Path-Pocket',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Path_Pocket","Pocket"),
                'Accel': "P, O",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Path_Pocket","Creates a Path Pocket object from a loop of edges or a face")}

    def IsActive(self):
        return not FreeCAD.ActiveDocument is None
        
    def Activated(self):
        
        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelectionEx()
        if len(selection) != 1:
            FreeCAD.Console.PrintError(translate("Path_Pocket","Please select an edges loop from one object, or a single face\n"))
            return
        if len(selection[0].SubObjects) == 0:
            FreeCAD.Console.PrintError(translate("Path_Pocket","Please select an edges loop from one object, or a single face\n"))
            return
        for s in selection[0].SubObjects:
            if s.ShapeType != "Edge":
                if (s.ShapeType != "Face") or (len(selection[0].SubObjects) != 1):
                    FreeCAD.Console.PrintError(translate("Path_Pocket","Please select only edges or a single face\n"))
                    return
        if selection[0].SubObjects[0].ShapeType == "Edge":
            try:
                import Part
                w = Part.Wire(selection[0].SubObjects)
            except:
                FreeCAD.Console.PrintError(translate("Path_Pocket","The selected edges don't form a loop\n"))
                return
        
        # if everything is ok, execute and register the transaction in the undo/redo stack
        FreeCAD.ActiveDocument.openTransaction(translate("Path_Pocket","Create Pocket"))
        FreeCADGui.addModule("PathScripts.PathPocket")
        FreeCADGui.doCommand('prjexists = False')
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython","Pocket")')
        FreeCADGui.doCommand('PathScripts.PathPocket.ObjectPocket(obj)')
        FreeCADGui.doCommand('PathScripts.PathPocket.ViewProviderPocket(obj.ViewObject)')
        subs = "["
        for s in selection[0].SubElementNames:
            subs += '"' + s + '",'
        subs += "]"
        FreeCADGui.doCommand('obj.Base = (FreeCAD.ActiveDocument.' + selection[0].ObjectName + ',' + subs + ')')
        FreeCADGui.doCommand('obj.Active = True')
        snippet = '''
from PathScripts import PathUtils
PathUtils.addToProject(obj)

ZMax = obj.Base[0].Shape.BoundBox.ZMax
ZMin = obj.Base[0].Shape.BoundBox.ZMin
obj.StepDown = 1.0
obj.StartDepth = ZMax
obj.FinalDepth = ZMin
obj.ClearanceHeight =  ZMax + 5.0
    
'''
        FreeCADGui.doCommand(snippet)
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp: 
    # register the FreeCAD command
    FreeCADGui.addCommand('Path_Pocket',CommandPathPocket())

FreeCAD.Console.PrintLog("Loading PathPocket... done\n")
