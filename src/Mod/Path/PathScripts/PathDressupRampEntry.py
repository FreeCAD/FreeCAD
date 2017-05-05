# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 Pekka Roivainen <pekkaroi@gmail.com>               *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
import FreeCAD
import FreeCADGui
import Draft
import DraftGeomUtils
import Path
import Part
import PathScripts.PathLog as PathLog
import math

from PathScripts import PathUtils
from PathScripts.PathGeom import PathGeom
from PySide import QtCore

# Qt tanslation handling
def translate(text, context = "PathDressup_RampEntry", disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


LOG_MODULE = PathLog.thisModule()
PathLog.setLevel(PathLog.Level.DEBUG, LOG_MODULE)



class ObjectDressup:

    def __init__(self, obj):
        self.obj = obj
        obj.addProperty("App::PropertyLink", "ToolController", "Path", QtCore.QT_TRANSLATE_NOOP("App::Property", "The tool controller that will be used to calculate the path"))
        obj.addProperty("App::PropertyLink", "Base","Base", QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTags", "The base path to modify"))
        obj.addProperty("App::PropertyAngle", "Angle", "Tag", QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTags", "Angle of tag plunge and ascent."))
        obj.addProperty("App::PropertyIntegerList", "Disabled", "Tag", QtCore.QT_TRANSLATE_NOOP("PathDressup_HoldingTags", "Ids of disabled holding tags"))
        obj.Proxy = self
    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None
    def setup(self, obj):
        obj.Angle = 60
        toolLoad = obj.ToolController
        if toolLoad is None or toolLoad.ToolNumber == 0:
            PathLog.error(translate("No Tool Controller is selected. We need a tool to build a Path\n"))
            #return
        else:
            tool = toolLoad.Proxy.getTool(toolLoad)
            if not tool or tool.Diameter == 0:
                PathLog.error(translate("No Tool found or diameter is zero. We need a tool to build a Path.\n"))
                return
            else:
                self.toolRadius = tool.Diameter/2

    def execute(self, obj):

        if not obj.Base:
            return
        if not obj.Base.isDerivedFrom("Path::Feature"):
            return
        if not obj.Base.Path:
            return

        self.angle = obj.Angle
        self.wire, self.rapids = PathGeom.wireForPath(obj.Base.Path)
        self.outedges = self.generateRamps()
        obj.Path = self.createCommands(obj, self.outedges)

    def generateRamps(self):
        edges = self.wire.Edges
        outedges = []
        for edge in edges:
            israpid = False
            for redge in self.rapids:
                if PathGeom.edgesMatch(edge,redge):
                    israpid = True
            if not israpid:
                bb = edge.BoundBox
                p0 = edge.Vertexes[0].Point
                p1 = edge.Vertexes[1].Point
                rampangle = self.angle
                if bb.XLength < 1e-6 and bb.YLength < 1e-6 and bb.ZLength > 0 and p0.z > p1.z:
                    plungelen = abs(p0.z-p1.z)
                    projectionlen = plungelen * math.tan(math.radians(self.angle)) #length of the forthcoming ramp projected to XY plane
                    PathLog.debug("Found plunge move at X:{} Y:{} From Z:{} to Z{}, length of ramp: {}".format(p0.x,p0.y,p0.z,p1.z, projectionlen))
                    # next need to determine how many edges in the path after plunge are needed to cover the length:
                    covered = False
                    coveredlen = 0
                    rampedges = []
                    i = edges.index(edge)+1
                    while not covered:
                        candidate = edges[i]
                        cp0 = candidate.Vertexes[0].Point
                        cp1 = candidate.Vertexes[1].Point
                        if abs(cp0.z-cp1.z) > 1e-6:
                            #this edge is not parallel to XY plane, not qualified for ramping.
                            break
                        PathLog.debug("Next edge length {}".format(candidate.Length))
                        rampedges.append(candidate)
                        coveredlen = coveredlen + candidate.Length

                        if coveredlen > projectionlen:
                            covered = True
                        i=i+1
                        if i >= len(edges):
                            break
                    if len(rampedges) == 0:
                        PathLog.debug("No suitable edges for ramping, plunge will remain as such")
                        outedges.append(edge)
                    elif not covered:
                        l = 0
                        for redge in rampedges:
                            l = l + redge.Lentgh
                        rampangle = math.degrees(atan(l/plungelen))
                        PathLog.debug("Cannot cover with desired angle, tightening angle to: {}".format(rampangle))
                    else:
                        PathLog.debug("All good, doing ramp to edges: {}".format(rampedges))
                        rampremaining = projectionlen
                        curPoint = p0 # start from the upper point of plunge
                        for redge in rampedges:
                            if redge.Length >= rampremaining:
                                #this edge needs to be splitted
                                splitEdge = PathGeom.splitEdgeAt(redge, redge.valueAt(rampremaining))
                                PathLog.debug("Got split edges with lengths: {}, {}".format(splitEdge[0].Length, splitEdge[1].Length))
                                #ramp ends to the last point of first edge
                                p1 = splitEdge[0].valueAt(splitEdge[0].LastParameter)
                                outedges.append(self.createRampEdge(splitEdge[0], curPoint, p1))
                                #now we have reached the end of the ramp. Go back to plunge position with constant Z
                                #start that by going to the beginning of this splitEdge
                                outedges.append(self.createRampEdge(splitEdge[0], p1, redge.valueAt(redge.FirstParameter)))
                            else:
                                deltaZ = redge.Length / math.tan(math.pi*self.angle/180.0)
                                newPoint = FreeCAD.Base.Vector(redge.valueAt(redge.LastParameter).x, redge.valueAt(redge.LastParameter).y, curPoint.z - deltaZ)
                                outedges.append(self.createRampEdge(redge, curPoint, newPoint))
                                curPoint = newPoint
                                rampremaining = rampremaining - redge.Length
                        #the last edge got handled previously
                        rampedges.pop()
                        for redge in reversed(rampedges):
                            outedges.append(self.createRampEdge(redge, redge.valueAt(redge.LastParameter),redge.valueAt(redge.FirstParameter)))
                else:
                    outedges.append(edge)
            else:
                outedges.append(edge)
        return outedges

    def createRampEdge(self,originalEdge, startPoint, endPoint):
        #PathLog.debug("Create edge from [{},{},{}] to [{},{},{}]".format(startPoint.x,startPoint.y, startPoint.z, endPoint.x, endPoint.y, endPoint.z))
        if type(originalEdge.Curve) == Part.Line or type(originalEdge.Curve) == Part.LineSegment:
            return Part.makeLine(startPoint,endPoint)
        elif type(originalEdge.Curve) == Part.Circle:
            arcMid = originalEdge.valueAt((originalEdge.FirstParameter+originalEdge.LastParameter)/2)
            arcMid.z = (startPoint.z+endPoint.z)/2
            return Part.Arc(startPoint, arcMid, endPoint).toShape()
        else:
            PathLog.error("Edge should not be helix")
    def createCommands(self,obj,edges):
        commands = []
        for edge in edges:
            israpid=False
            for redge in self.rapids:
                if PathGeom.edgesMatch(edge,redge):
                    israpid = True
            if israpid:
                v = edge.valueAt(edge.LastParameter)
                commands.append(Path.Command('G0', {'X':v.x, 'Y': v.y, 'Z': v.z}))
            else:
                commands.extend(PathGeom.cmdsForEdge(edge))

        lastCmd = Path.Command('G0', {'X': 0.0, 'Y': 0.0, 'Z': 0.0});

        outCommands = []

        horizFeed = obj.ToolController.HorizFeed.Value
        vertFeed = obj.ToolController.VertFeed.Value
        horizRapid = obj.ToolController.HorizRapid.Value
        vertRapid = obj.ToolController.VertRapid.Value

        for cmd in commands:
            params = cmd.Parameters
            zVal = params.get('Z', None)
            zVal2 = lastCmd.Parameters.get('Z', None)

            zVal = zVal and round(zVal, 8)
            zVal2 = zVal2 and round(zVal2, 8)

            if cmd.Name in ['G1', 'G2', 'G3', 'G01', 'G02', 'G03']:
                if zVal is not None and zVal2 != zVal:
                    params['F'] = vertFeed
                else:
                    params['F'] = horizFeed
                lastCmd = cmd

            elif cmd.Name in ['G0', 'G00']:
                if zVal is not None and zVal2 != zVal:
                    params['F'] = vertRapid
                else:
                    params['F'] = horizRapid
                lastCmd = cmd

            outCommands.append(Path.Command(cmd.Name, params))

        return Path.Path(outCommands)



class ViewProviderDressup:

    def __init__(self, vobj):
        vobj.Proxy = self

    def attach(self, vobj):
        self.obj = vobj.Object


    def claimChildren(self):
        for i in self.obj.Base.InList:
            if hasattr(i, "Group"):
                group = i.Group
                for g in group:
                    if g.Name == self.obj.Base.Name:
                        group.remove(g)
                i.Group = group
                print(i.Group)
        #FreeCADGui.ActiveDocument.getObject(obj.Base.Name).Visibility = False
        return [self.obj.Base]


    def onDelete(self, arg1=None, arg2=None):
        '''this makes sure that the base operation is added back to the project and visible'''
        FreeCADGui.ActiveDocument.getObject(arg1.Object.Base.Name).Visibility = True
        PathUtils.addToJob(arg1.Object.Base)
        return True

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

class CommandPathDressupRampEntry:

    def GetResources(self):
        return {'Pixmap': 'Path-Dressup',
                'MenuText': QtCore.QT_TRANSLATE_NOOP("PathDressup_RampEntry", "RampEntry Dress-up"),
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("PathDressup_RampEntry", "Creates a Ramp Entry Dress-up object from a selected path")}

    def IsActive(self):
        if FreeCAD.ActiveDocument is not None:
            for o in FreeCAD.ActiveDocument.Objects:
                if o.Name[:3] == "Job":
                        return True
        return False

    def Activated(self):

        # check that the selection contains exactly what we want
        selection = FreeCADGui.Selection.getSelection()
        if len(selection) != 1:
            PathLog.error(translate("Please select one path object\n"))
            return
        baseObject = selection[0]
        if not baseObject.isDerivedFrom("Path::Feature"):
            PathLog.error(translate("The selected object is not a path\n"))
            return
        if baseObject.isDerivedFrom("Path::FeatureCompoundPython"):
            PathLog.error(translate("Please select a Profile object"))
            return

        # everything ok!
        FreeCAD.ActiveDocument.openTransaction(translate("Create RampEntry Dress-up"))
        FreeCADGui.addModule("PathScripts.PathDressUpRampEntry")
        FreeCADGui.addModule("PathScripts.PathUtils")
        FreeCADGui.doCommand('obj = FreeCAD.ActiveDocument.addObject("Path::FeaturePython", "RampEntryDressup")')
        FreeCADGui.doCommand('dbo = PathScripts.PathDressupRampEntry.ObjectDressup(obj)')
        FreeCADGui.doCommand('obj.Base = FreeCAD.ActiveDocument.' + selection[0].Name)
        FreeCADGui.doCommand('PathScripts.PathDressupRampEntry.ViewProviderDressup(obj.ViewObject)')
        FreeCADGui.doCommand('PathScripts.PathUtils.addToJob(obj)')
        FreeCADGui.doCommand('Gui.ActiveDocument.getObject(obj.Base.Name).Visibility = False')
        FreeCADGui.doCommand('obj.ToolController = PathScripts.PathUtils.findToolController(obj)')
        FreeCADGui.doCommand('dbo.setup(obj)')
        FreeCAD.ActiveDocument.commitTransaction()
        FreeCAD.ActiveDocument.recompute()


if FreeCAD.GuiUp:
    # register the FreeCAD command
    FreeCADGui.addCommand('PathDressup_RampEntry', CommandPathDressupRampEntry())

PathLog.notice("Loading PathDressupRampEntry... done\n")
