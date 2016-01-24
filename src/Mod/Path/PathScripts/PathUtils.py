# -*- coding: utf-8 -*-

#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2014 Dan Falck <ddfalck@gmail.com>                      *
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
'''PathUtils -common functions used in PathScripts for filterig, sorting, and generating gcode toolpath data '''
import FreeCAD
import Part
from FreeCAD import Vector
import FreeCADGui
import math
import DraftGeomUtils
from DraftGeomUtils import geomType
import DraftVecUtils
import PathScripts
from PathScripts import PathProject

def cleanedges(splines,precision):
    '''cleanedges([splines],precision). Convert BSpline curves, Beziers, to arcs that can be used for cnc paths.
    Returns Lines as is. Filters Circle and Arcs for over 180 degrees. Discretizes Ellipses. Ignores other geometry. '''
    edges = []
    for spline in splines:
        if geomType(spline)=="BSplineCurve":
            arcs = spline.Curve.toBiArcs(precision)
            for i in arcs:
                edges.append(Part.Edge(i))

        elif geomType(spline)=="BezierCurve":
            newspline=spline.Curve.toBSpline()
            arcs = newspline.toBiArcs(precision)
            for i in arcs:
                edges.append(Part.Edge(i))

        elif geomType(spline)=="Ellipse":
            edges = curvetowire(spline, 1.0) #fixme hardcoded value
                        
        elif geomType(spline)=="Circle":
            #arcs=filterArcs(spline)
            edges.append(spline)
                                     
        elif geomType(spline)=="Line":
            edges.append(spline)
 
        else:
            pass
               
    return edges

def curvetowire(obj,steps):
    '''adapted from DraftGeomUtils, because the discretize function changed a bit '''
    points = obj.copy().discretize(Distance = eval('steps'))
    p0 = points[0]
    edgelist = []
    for p in points[1:]:
        edge = Part.makeLine((p0.x,p0.y,p0.z),(p.x,p.y,p.z))
        edgelist.append(edge)
        p0 = p
    return edgelist

def fmt(val): return format(val, '.4f') #fixme set at 4 decimal places for testing

def isSameEdge(e1,e2):
    """isSameEdge(e1,e2): return True if the 2 edges are both lines or arcs/circles and have the same
    points - inspired by Yorik's function isSameLine"""
    if not (isinstance(e1.Curve,Part.Line) or isinstance(e1.Curve,Part.Circle)):
        return False
    if not (isinstance(e2.Curve,Part.Line) or isinstance(e2.Curve,Part.Circle)):
        return False
    if type(e1.Curve) <> type(e2.Curve):
        return False
    if isinstance(e1.Curve,Part.Line):
        if (DraftVecUtils.equals(e1.Vertexes[0].Point,e2.Vertexes[0].Point)) and \
           (DraftVecUtils.equals(e1.Vertexes[-1].Point,e2.Vertexes[-1].Point)):
            return True
        elif (DraftVecUtils.equals(e1.Vertexes[-1].Point,e2.Vertexes[0].Point)) and \
           (DraftVecUtils.equals(e1.Vertexes[0].Point,e2.Vertexes[-1].Point)):
            return True
    if isinstance(e1.Curve,Part.Circle):
        center = False; radius= False; endpts=False
        if e1.Curve.Center == e2.Curve.Center:
            center = True
        if e1.Curve.Radius == e2.Curve.Radius:
            radius = True
        if (DraftVecUtils.equals(e1.Vertexes[0].Point,e2.Vertexes[0].Point)) and \
           (DraftVecUtils.equals(e1.Vertexes[-1].Point,e2.Vertexes[-1].Point)):
            endpts = True
        elif (DraftVecUtils.equals(e1.Vertexes[-1].Point,e2.Vertexes[0].Point)) and \
           (DraftVecUtils.equals(e1.Vertexes[0].Point,e2.Vertexes[-1].Point)):
            endpts = True
        if (center and radius and endpts):
            return True
    return False

def segments(poly):
    ''' A sequence of (x,y) numeric coordinates pairs '''
    return zip(poly, poly[1:] + [poly[0]])

def check_clockwise(poly):
    '''
     check_clockwise(poly) a function for returning a boolean if the selected wire is clockwise or counter clockwise
     based on point order. poly = [(x1,y1),(x2,y2),(x3,y3)]
    '''
    clockwise = False
    if (sum(x0*y1 - x1*y0 for ((x0, y0), (x1, y1)) in segments(poly))) < 0:
        clockwise = not clockwise
    return clockwise

def filterArcs(arcEdge):
    '''filterArcs(Edge) -used to split arcs that over 180 degrees. Returns list '''
    s = arcEdge
    if isinstance(s.Curve,Part.Circle):
        splitlist =[]
        angle = abs(s.LastParameter-s.FirstParameter)
        overhalfcircle = False
        goodarc = False
        if (angle > math.pi):
            overhalfcircle = True
        else:
            goodarc = True
        if not goodarc:
            arcstpt  = s.valueAt(s.FirstParameter)
            arcmid   = s.valueAt((s.LastParameter-s.FirstParameter)*0.5+s.FirstParameter)
            arcquad1 = s.valueAt((s.LastParameter-s.FirstParameter)*0.25+s.FirstParameter)#future midpt for arc1
            arcquad2 = s.valueAt((s.LastParameter-s.FirstParameter)*0.75+s.FirstParameter) #future midpt for arc2
            arcendpt = s.valueAt(s.LastParameter)
            # reconstruct with 2 arcs
            arcseg1 = Part.ArcOfCircle(arcstpt,arcquad1,arcmid)
            arcseg2 = Part.ArcOfCircle(arcmid,arcquad2,arcendpt)

            eseg1 = arcseg1.toShape()
            eseg2 = arcseg2.toShape()
            splitlist.append(eseg1)
            splitlist.append(eseg2)
        else:
            splitlist.append(s)
    elif isinstance(s.Curve,Part.Line):
        pass
    return splitlist

def reverseEdge(e):
    if geomType(e) == "Circle":
        arcstpt  = e.valueAt(e.FirstParameter)
        arcmid   = e.valueAt((e.LastParameter-e.FirstParameter)*0.5+e.FirstParameter)
        arcendpt = e.valueAt(e.LastParameter)
        arcofCirc = Part.ArcOfCircle(arcendpt,arcmid,arcstpt)
        newedge = arcofCirc.toShape()
    elif geomType(e) == "Line":
        stpt = e.valueAt(e.FirstParameter)
        endpt = e.valueAt(e.LastParameter)
        newedge = Part.makeLine(endpt,stpt)

    return newedge

def convert(toolpath,Side,radius,clockwise=False,Z=0.0,firstedge=None,vf=1.0,hf=2.0):
    '''convert(toolpath,Side,radius,clockwise=False,Z=0.0,firstedge=None) Converts lines and arcs to G1,G2,G3 moves. Returns a string.'''
    last = None
    output = ""
    # create the path from the offset shape
    for edge in toolpath:
        if not last:
            #set the first point
            last = edge.Vertexes[0].Point
            #FreeCAD.Console.PrintMessage("last pt= " + str(last)+ "\n")
            output += "G1 X"+str(fmt(last.x))+" Y"+str(fmt(last.y))+" Z"+str(fmt(Z))+" F"+str(vf)+"\n"
        if isinstance(edge.Curve,Part.Circle):
            #FreeCAD.Console.PrintMessage("arc\n")
            arcstartpt = edge.valueAt(edge.FirstParameter)
            midpt = edge.valueAt((edge.FirstParameter+edge.LastParameter)*0.5)
            arcendpt = edge.valueAt(edge.LastParameter)
            arcchkpt=edge.valueAt(edge.LastParameter*.99)

            if DraftVecUtils.equals(last,arcstartpt):
                startpt = arcstartpt
                endpt = arcendpt
            else:
                startpt = arcendpt
                endpt = arcstartpt
            center = edge.Curve.Center
            relcenter = center.sub(last)
            #FreeCAD.Console.PrintMessage("arc  startpt= " + str(startpt)+ "\n")
            #FreeCAD.Console.PrintMessage("arc  midpt= " + str(midpt)+ "\n")
            #FreeCAD.Console.PrintMessage("arc  endpt= " + str(endpt)+ "\n")
            arc_cw = check_clockwise([(startpt.x,startpt.y),(midpt.x,midpt.y),(endpt.x,endpt.y)])
            #FreeCAD.Console.PrintMessage("arc_cw="+ str(arc_cw)+"\n")
            if arc_cw:
                output += "G2"
            else:
                output += "G3"
            output += " X"+str(fmt(endpt.x))+" Y"+str(fmt(endpt.y))+" Z"+str(fmt(Z))+" F"+str(hf)
            output += " I" + str(fmt(relcenter.x)) + " J" + str(fmt(relcenter.y)) + " K" + str(fmt(relcenter.z))
            output += "\n"
            last = endpt
            #FreeCAD.Console.PrintMessage("last pt arc= " + str(last)+ "\n")
        else:
            point = edge.Vertexes[-1].Point
            if DraftVecUtils.equals(point , last): # edges can come flipped
                point = edge.Vertexes[0].Point
            output += "G1 X"+str(fmt(point.x))+" Y"+str(fmt(point.y))+" Z"+str(fmt(Z))+" F"+str(hf)+"\n"
            last = point
            #FreeCAD.Console.PrintMessage("line\n")
            #FreeCAD.Console.PrintMessage("last pt line= " + str(last)+ "\n")
    return output

def SortPath(wire,Side,radius,clockwise,firstedge=None,SegLen =0.5):
    '''SortPath(wire,Side,radius,clockwise,firstedge=None,SegLen =0.5) Sorts the wire and reverses it, if needed. Splits arcs over 180 degrees in two. Returns the reordered offset of the wire. '''
    if firstedge:
        edgelist = wire.Edges[:]
        if wire.isClosed():
            elindex = None
            n=0
            for e in edgelist:
                if isSameEdge(e,firstedge):
#                    FreeCAD.Console.PrintMessage('found first edge\n')
                    elindex = n
                n=n+1
            l1 = edgelist[:elindex]
            l2 = edgelist[elindex:]
            newedgelist = l2+l1

            if clockwise:
                newedgelist.reverse()
                last = newedgelist.pop(-1)
                newedgelist.insert(0, last)

            preoffset= []
            for e in newedgelist:
                if  clockwise:
                    r = reverseEdge(e) 
                    preoffset.append(r)
                else:
                    preoffset.append(e)

            sortedpreoff = Part.__sortEdges__(preoffset)
            wire = Part.Wire(sortedpreoff)
        else:
            sortedpreoff = Part.__sortEdges__(edgelist)
            wire = Part.Wire(sortedpreoff)

    edgelist = []
    for e in wire.Edges:
        if geomType(e) == "Circle":
            arclist = filterArcs(e)
            for a in arclist:
                edgelist.append(a)
        elif geomType(e) == "Line":
            edgelist.append(e)
        elif geomType(e) == "BSplineCurve" or \
                 geomType(e) == "BezierCurve" or \
                 geomType(e) == "Ellipse":
                 edgelist.append(Part.Wire(curvetowire(e,(SegLen))))

    newwire = Part.Wire(edgelist)
    if Side == 'Left':
    # we use the OCC offset feature
        offset = newwire.makeOffset(radius)#tool is outside line
    elif Side == 'Right':
        offset = newwire.makeOffset(-radius)#tool is inside line
    else:
        if wire.isClosed():
            offset = newwire.makeOffset(0.0)
        else:
            offset = newwire

    return offset

def MakePath(wire,Side,radius,clockwise,ZClearance,StepDown,ZStart,ZFinalDepth,firstedge=None,PathClosed=True,SegLen =0.5,VertFeed=1.0,HorizFeed=2.0):
    ''' makes the path - just a simple profile for now '''
    offset = SortPath(wire,Side,radius,clockwise,firstedge,SegLen=0.5)
    toolpath = offset.Edges[:]
    paths = "" 
    first = toolpath[0].Vertexes[0].Point
    paths += "G0 X"+str(fmt(first.x))+"Y"+str(fmt(first.y))+"\n"
    ZCurrent = ZStart- StepDown
    if PathClosed:
        while ZCurrent > ZFinalDepth:
            paths += convert(toolpath,Side,radius,clockwise,ZCurrent,firstedge,VertFeed,HorizFeed)
            ZCurrent = ZCurrent-abs(StepDown)
        paths += convert(toolpath,Side,radius,clockwise,ZFinalDepth,firstedge,VertFeed,HorizFeed)
        paths += "G0 Z" + str(ZClearance)
    else:
        while ZCurrent > ZFinalDepth:
            paths += convert(toolpath,Side,radius,clockwise,ZCurrent,firstedge,VertFeed,HorizFeed)
            paths += "G0 Z" + str(ZClearance)
            paths += "G0 X"+str(fmt(first.x))+"Y"+str(fmt(first.y))+"\n"
            ZCurrent = ZCurrent-abs(StepDown)
        paths += convert(toolpath,Side,radius,clockwise,ZFinalDepth,firstedge,VertFeed,HorizFeed)
        paths += "G0 Z" + str(ZClearance)
    return paths

# the next two functions are for automatically populating tool numbers/height offset numbers based on previously active toolnumbers

def changeTool(obj,proj):
    tlnum = 0
    for p in proj.Group:
        if not hasattr(p,"Group"):
            if isinstance(p.Proxy,PathScripts.PathLoadTool.LoadTool) and p.ToolNumber > 0:
                tlnum = p.ToolNumber
            if p == obj:
                return tlnum
        elif hasattr(p,"Group"):
            for g in p.Group:
                if isinstance(g.Proxy,PathScripts.PathLoadTool.LoadTool):
                    tlnum = g.ToolNumber
                if g == obj:
                    return tlnum


def getLastTool(obj):
    toolNum = obj.ToolNumber
    if obj.ToolNumber == 0:
        # find tool from previous toolchange
        proj = findProj()
        toolNum = changeTool(obj, proj)
    return getTool(obj, toolNum)


def getTool(obj,number=0):
    "retrieves a tool from a hosting object with a tooltable, if any"
    for o in obj.InList:
        if o.TypeId == "Path::FeatureCompoundPython":
            for m in o.Group:
                if hasattr(m,"Tooltable"):
                    return m.Tooltable.getTool(number)
    # not found? search one level up
    for o in obj.InList:
        return getTool(o,number)
    return None


def findProj():
    for o in FreeCAD.ActiveDocument.Objects:
        if "Proxy" in o.PropertiesList:
            if isinstance(o.Proxy, PathProject.ObjectPathProject):
                return o

def findMachine():
    '''find machine object for the tooltable editor '''
    for o in FreeCAD.ActiveDocument.Objects:
        if "Proxy" in o.PropertiesList:
            if isinstance(o.Proxy, PathScripts.PathMachine.Machine):
                return o

def addToProject(obj):
    """Adds a path obj to this document, if no PathParoject exists it's created on the fly"""
    p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Path")
    if p.GetBool("pathAutoProject",True):
        project = findProj()
        if not project:
            project = PathProject.CommandProject.Create()
        g = project.Group
        g.append(obj)
        project.Group = g
        return project
    return None


def getLastZ(obj):
    ''' find the last z value in the project '''
    lastZ = ""
    for g in obj.Group:
        for c in g.Path.Commands:
            for n in c.Parameters:
                if n == 'Z':
                    lastZ= c.Parameters['Z']
    return lastZ

