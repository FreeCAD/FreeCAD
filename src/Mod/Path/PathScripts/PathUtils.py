# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2014 Dan Falck <ddfalck@gmail.com>                      *
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
'''PathUtils -common functions used in PathScripts for filterig, sorting, and generating gcode toolpath data '''
import FreeCAD
import FreeCADGui
import Part
import math
import Draft
import Path
import TechDraw
from DraftGeomUtils import geomType
from DraftGeomUtils import findWires
import DraftVecUtils
import PathScripts
from PathScripts import PathJob
import itertools

def cleanedges(splines, precision):
    '''cleanedges([splines],precision). Convert BSpline curves, Beziers, to arcs that can be used for cnc paths.
    Returns Lines as is. Filters Circle and Arcs for over 180 degrees. Discretizes Ellipses. Ignores other geometry. '''
    edges = []
    for spline in splines:
        if geomType(spline) == "BSplineCurve":
            arcs = spline.Curve.toBiArcs(precision)
            for i in arcs:
                edges.append(Part.Edge(i))

        elif geomType(spline) == "BezierCurve":
            newspline = spline.Curve.toBSpline()
            arcs = newspline.toBiArcs(precision)
            for i in arcs:
                edges.append(Part.Edge(i))

        elif geomType(spline) == "Ellipse":
            edges = curvetowire(spline, 1.0)  # fixme hardcoded value

        elif geomType(spline) == "Circle":
            arcs = filterArcs(spline)
            for i in arcs:
                edges.append(Part.Edge(i))

        elif geomType(spline) == "Line":
            edges.append(spline)

        else:
            pass

    return edges


def curvetowire(obj, steps):
    '''adapted from DraftGeomUtils, because the discretize function changed a bit '''
    points = obj.copy().discretize(Distance=eval('steps'))
    p0 = points[0]
    edgelist = []
    for p in points[1:]:
        edge = Part.makeLine((p0.x, p0.y, p0.z), (p.x, p.y, p.z))
        edgelist.append(edge)
        p0 = p
    return edgelist


# fixme set at 4 decimal places for testing
def fmt(val): return format(val, '.4f')


def getProjected(shape,direction):
    "returns projected edges from a shape and a direction"
    import Part,Drawing
    edges = []
    groups = Drawing.projectEx(shape,direction)
    for g in groups[0:5]:
        if g:
            edges.append(g)
    # if hasattr(obj,"Tessellation") and obj.Tessellation:
    #     return DraftGeomUtils.cleanProjection(Part.makeCompound(edges),obj.Tessellation,obj.SegmentLength)
    # else:
    return Part.makeCompound(edges)


def silhouette(obj):
    from FreeCAD import Vector
    s = getProjected(obj.Shape, Vector(0,0,1))
    print s
    w = TechDraw.findOuterWire(s.Edges)
    return w

def isSameEdge(e1, e2):
    """isSameEdge(e1,e2): return True if the 2 edges are both lines or arcs/circles and have the same
    points - inspired by Yorik's function isSameLine"""
    if not (isinstance(e1.Curve, Part.Line) or isinstance(e1.Curve, Part.Circle)):
        return False
    if not (isinstance(e2.Curve, Part.Line) or isinstance(e2.Curve, Part.Circle)):
        return False
    if type(e1.Curve) != type(e2.Curve):
        return False
    if isinstance(e1.Curve, Part.Line):
        if (DraftVecUtils.equals(e1.Vertexes[0].Point, e2.Vertexes[0].Point)) and \
           (DraftVecUtils.equals(e1.Vertexes[-1].Point, e2.Vertexes[-1].Point)):
            return True
        elif (DraftVecUtils.equals(e1.Vertexes[-1].Point, e2.Vertexes[0].Point)) and \
                (DraftVecUtils.equals(e1.Vertexes[0].Point, e2.Vertexes[-1].Point)):
            return True
    if isinstance(e1.Curve, Part.Circle):
        center = False
        radius = False
        endpts = False
        if e1.Curve.Center == e2.Curve.Center:
            center = True
        if e1.Curve.Radius == e2.Curve.Radius:
            radius = True
        if (DraftVecUtils.equals(e1.Vertexes[0].Point, e2.Vertexes[0].Point)) and \
           (DraftVecUtils.equals(e1.Vertexes[-1].Point, e2.Vertexes[-1].Point)):
            endpts = True
        elif (DraftVecUtils.equals(e1.Vertexes[-1].Point, e2.Vertexes[0].Point)) and \
                (DraftVecUtils.equals(e1.Vertexes[0].Point, e2.Vertexes[-1].Point)):
            endpts = True
        if (center and radius and endpts):
            return True
    return False


def segments(poly):
    ''' A sequence of (x,y) numeric coordinates pairs '''
    return zip(poly, poly[1:] + [poly[0]])

def is_clockwise(obj):
    '''tests if a wire or Path is clockwise'''
    sum = 0
    if isinstance(obj, Part.Wire):
        for first, second in itertools.izip(obj.Edges, obj.Edges[1:]):
	    sum = (second.Vertexes[0].X - first.Vertexes[0].X) * (second.Vertexes[0].Y + first.Vertexes[0].Y)
        sum += (obj.Edges[0].Vertexes[0].X - obj.Edges[-1].Vertexes[0].X) * (obj.Edges[0].Vertexes[0].Y + obj.Edges[-1].Vertexes[0].Y)
    elif isinstance(obj, Path.Path):
        movecommands = ['G1', 'G01', 'G2', 'G02', 'G3', 'G03']

        lastLocation = {'Y': 0, 'X': 0, 'Z': 0.0}
        currLocation = {'Y': 0, 'X': 0, 'Z': 0.0}
        sum = 0

        for curCommand in obj.Commands:

            if curCommand.Name in movecommands:
                lastLocation.update(currLocation)
                currLocation.update(curCommand.Parameters)
                sum += (currLocation["X"] - lastLocation["X"]) * (currLocation["Y"] + lastLocation["Y"])
        sum += (0 - lastLocation["X"]) * (0 + lastLocation["Y"])

    return sum >= 0

def loopdetect(obj, edge1, edge2):
    '''
    Returns a loop wire that includes the two edges.
    Useful for detecting boundaries of negative space features ie 'holes'
    If a unique loop is not found, returns None
    '''
    candidates = []
    for wire in obj.Shape.Wires:
        for e in wire.Edges:
            if e.hashCode() == edge1.hashCode():
                candidates.append((wire.hashCode(),wire))
            if e.hashCode() == edge2.hashCode():
                candidates.append((wire.hashCode(),wire))
    loop = set([x for x in candidates if candidates.count(x) > 1]) #return the duplicate item
    if len(loop) != 1:
        return None
    loopwire = next(x for x in loop)[1]
    return loopwire


def check_clockwise(poly):
    '''
     check_clockwise(poly) a function for returning a boolean if the selected wire is clockwise or counter clockwise
     based on point order. poly = [(x1,y1),(x2,y2),(x3,y3)]
    '''
    clockwise = False
    if (sum(x0 * y1 - x1 * y0 for ((x0, y0), (x1, y1)) in segments(poly))) < 0:
        clockwise = not clockwise
    return clockwise


def filterArcs(arcEdge):
    '''filterArcs(Edge) -used to split arcs that over 180 degrees. Returns list '''
    s = arcEdge
    if isinstance(s.Curve, Part.Circle):
        splitlist = []
        angle = abs(s.LastParameter - s.FirstParameter)
        # overhalfcircle = False
        goodarc = False
        if (angle > math.pi):
            pass
            # overhalfcircle = True
        else:
            goodarc = True
        if not goodarc:
            arcstpt = s.valueAt(s.FirstParameter)
            arcmid = s.valueAt(
                (s.LastParameter - s.FirstParameter) * 0.5 + s.FirstParameter)
            arcquad1 = s.valueAt((s.LastParameter - s.FirstParameter)
                                 * 0.25 + s.FirstParameter)  # future midpt for arc1
            arcquad2 = s.valueAt((s.LastParameter - s.FirstParameter)
                                 * 0.75 + s.FirstParameter)  # future midpt for arc2
            arcendpt = s.valueAt(s.LastParameter)
            # reconstruct with 2 arcs
            arcseg1 = Part.ArcOfCircle(arcstpt, arcquad1, arcmid)
            arcseg2 = Part.ArcOfCircle(arcmid, arcquad2, arcendpt)

            eseg1 = arcseg1.toShape()
            eseg2 = arcseg2.toShape()
            splitlist.append(eseg1)
            splitlist.append(eseg2)
        else:
            splitlist.append(s)
    elif isinstance(s.Curve, Part.Line):
        pass
    return splitlist


def reverseEdge(e):
    if geomType(e) == "Circle":
        arcstpt = e.valueAt(e.FirstParameter)
        arcmid = e.valueAt((e.LastParameter - e.FirstParameter) * 0.5 + e.FirstParameter)
        arcendpt = e.valueAt(e.LastParameter)
        arcofCirc = Part.ArcOfCircle(arcendpt, arcmid, arcstpt)
        newedge = arcofCirc.toShape()
    elif geomType(e) == "Line":
        stpt = e.valueAt(e.FirstParameter)
        endpt = e.valueAt(e.LastParameter)
        newedge = Part.makeLine(endpt, stpt)

    return newedge

def edge_to_path(lastpt, edge, Z, hf=2.0):
    if isinstance(edge.Curve, Part.Circle):
        # FreeCAD.Console.PrintMessage("arc\n")
        arcstartpt = edge.valueAt(edge.FirstParameter)
        midpt = edge.valueAt(
            (edge.FirstParameter + edge.LastParameter) * 0.5)
        arcendpt = edge.valueAt(edge.LastParameter)
        # arcchkpt = edge.valueAt(edge.LastParameter * .99)

        if DraftVecUtils.equals(lastpt, arcstartpt):
            startpt = arcstartpt
            endpt = arcendpt
        else:
            startpt = arcendpt
            endpt = arcstartpt
        center = edge.Curve.Center
        relcenter = center.sub(lastpt)
        # FreeCAD.Console.PrintMessage("arc  startpt= " + str(startpt)+ "\n")
        # FreeCAD.Console.PrintMessage("arc  midpt= " + str(midpt)+ "\n")
        # FreeCAD.Console.PrintMessage("arc  endpt= " + str(endpt)+ "\n")
        arc_cw = check_clockwise(
            [(startpt.x, startpt.y), (midpt.x, midpt.y), (endpt.x, endpt.y)])
        # FreeCAD.Console.PrintMessage("arc_cw="+ str(arc_cw)+"\n")
        if arc_cw:
            output = "G2"
        else:
            output = "G3"
        output += " X" + str(fmt(endpt.x)) + " Y" + \
            str(fmt(endpt.y)) + " Z" + str(fmt(Z)) + " F" + str(hf)
        output += " I" + str(fmt(relcenter.x)) + " J" + \
            str(fmt(relcenter.y)) + " K" + str(fmt(relcenter.z))
        output += "\n"
        lastpt = endpt
        # FreeCAD.Console.PrintMessage("last pt arc= " + str(lastpt)+ "\n")
    else:
        point = edge.Vertexes[-1].Point
        if DraftVecUtils.equals(point, lastpt):  # edges can come flipped
            point = edge.Vertexes[0].Point
        output = "G1 X" + str(fmt(point.x)) + " Y" + str(fmt(point.y)) + \
            " Z" + str(fmt(Z)) + " F" + str(hf) + "\n"
        lastpt = point
        # FreeCAD.Console.PrintMessage("line\n")
        # FreeCAD.Console.PrintMessage("last pt line= " + str(lastpt)+ "\n")
    return lastpt, output


def convert(toolpath, Z=0.0, PlungeAngle=90.0, Zprevious=None, StopLength=None, vf=1.0, hf=2.0) :
    '''convert(toolpath,Z=0.0,vf=1.0,hf=2.0,PlungeAngle=90.0,Zprevious=None,StopLength=None) Converts lines and arcs to G1,G2,G3 moves. Returns a string.'''

    if PlungeAngle != 90.0:
        if Zprevious is None:
            raise Exception("Cannot use PlungeAngle != 90.0 degrees without parameter Zprevious")
        tanA = math.tan(math.pi * PlungeAngle / 180.0)
        minA = (Zprevious - Z) / sum(edge.Length for edge in toolpath)
        if tanA < minA:
            tanA = minA
            #FreeCAD.Console.PrintMessage('Increasing ramp angle to {0} degrees, to be able to make a full round\n'.format(math.atan(tanA) * 180.0 / math.pi))
    else:
        Zprevious = Z

    lastpt = None
    output = ""
    path_length = 0.0
    Z_cur = Zprevious

    # create the path from the offset shape
    for edge in toolpath:
        if not lastpt:
            # set the first point
            lastpt = edge.Vertexes[0].Point
            # FreeCAD.Console.PrintMessage("last pt= " + str(lastpt)+ "\n")
            output += "G1 X" + str(fmt(lastpt.x)) + " Y" + str(fmt(lastpt.y)) + \
                " Z" + str(fmt(Z_cur)) + " F" + str(vf) + "\n"

        if StopLength:
            if path_length + edge.Length > StopLength:
                # have to split current edge in two
                t0 = edge.FirstParameter
                t1 = edge.LastParameter
                dL = StopLength - path_length
                t = t0 + (t1 - t0) * dL / edge.Length
                assert(t0 < t < t1)
                edge = edge.split(t).Edges[0]
                path_length = StopLength
            else:
                path_length += edge.Length
        else:
            path_length += edge.Length

        if Z_cur > Z:
            Z_next = Zprevious - path_length * tanA
            if Z_next < Z:
                # have to split current edge in two
                t0 = edge.FirstParameter
                t1 = edge.LastParameter
                dZ = Z_cur - Z
                t = t0 + (t1 - t0) * (dZ / tanA) / edge.Length
                assert(t0 < t < t1)
                subwire = edge.split(t)
                assert(len(subwire.Edges) == 2)
                Z_cur = Z
                lastpt, codes = edge_to_path(lastpt, subwire.Edges[0], Z_cur, hf)
                output += codes
                edge = subwire.Edges[1]
            else:
                Z_cur = Z_next

        lastpt, codes = edge_to_path(lastpt, edge, Z_cur, hf)
        output += codes

        if StopLength:
            if path_length >= StopLength:
                break

    return output

def SortPath(wire, Side, radius, clockwise, firstedge=None, SegLen=0.5):
    '''SortPath(wire,Side,radius,clockwise,firstedge=None,SegLen =0.5) Sorts the wire and reverses it, if needed. Splits arcs over 180 degrees in two. Returns the reordered offset of the wire. '''
    if firstedge:
        edgelist = wire.Edges[:]
        if wire.isClosed():
            elindex = None
            n = 0
            for e in edgelist:
                if isSameEdge(e, firstedge):
                    #                    FreeCAD.Console.PrintMessage('found first edge\n')
                    elindex = n
                n = n + 1
            l1 = edgelist[:elindex]
            l2 = edgelist[elindex:]
            newedgelist = l2 + l1

            if clockwise:
                newedgelist.reverse()
                last = newedgelist.pop(-1)
                newedgelist.insert(0, last)

            preoffset = []
            for e in newedgelist:
                if clockwise:
                    r = reverseEdge(e)
                    preoffset.append(r)
                else:
                    preoffset.append(e)

            sortedpreoff = Part.__sortEdges__(preoffset)
            wire = Part.Wire(sortedpreoff)
            #wire = findWires(sortedpreoff)[0]
        else:
            sortedpreoff = Part.__sortEdges__(edgelist)
            wire = Part.Wire(sortedpreoff)
            #wire = findWires(sortedpreoff)[0]

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
            edgelist.append(Part.Wire(curvetowire(e, (SegLen))))
    #newwire = Part.Wire(edgelist)
    sortededges = Part.__sortEdges__(edgelist)
    newwire = findWires(sortededges)[0]

    if is_clockwise(newwire) is not clockwise:
        newwire.reverse()

    if Side == 'Left':
        # we use the OCC offset feature
        offset = newwire.makeOffset(radius)  # tool is outside line
    elif Side == 'Right':
        offset = newwire.makeOffset(-radius)  # tool is inside line
    else:
        if wire.isClosed():
            offset = newwire.makeOffset(0.0)
        else:
            offset = newwire
    offset.reverse()

    return offset


def MakePath(wire, Side, radius, clockwise, ZClearance, StepDown, ZStart,
                ZFinalDepth, firstedge=None, PathClosed=True, SegLen=0.5,
                VertFeed=1.0, HorizFeed=2.0, VertJog=1.0, HorizJog = 2.0, PlungeAngle=90.0):
    ''' makes the path - just a simple profile for now '''
    offset = SortPath(wire, Side, radius, clockwise, firstedge, SegLen=SegLen)
    if len(offset.Edges) == 0:
        return ""

    toolpath = offset.Edges[:]
    paths = ""
    paths += "G0 Z" + str(ZClearance) + "F " + fmt(VertJog) + "\n"
    first = toolpath[0].Vertexes[0].Point
    paths += "G0 X" + str(fmt(first.x)) + "Y" + str(fmt(first.y)) + "F " + fmt(HorizJog) + "\n"
    Zprevious = ZStart
    ZCurrent = ZStart - StepDown

    while ZCurrent > ZFinalDepth:
        paths += convert(toolpath, Z=ZCurrent, Zprevious=Zprevious, PlungeAngle=PlungeAngle,
                         vf=VertFeed, hf=HorizFeed)
        if not PathClosed:
            paths += "G0 Z" + str(ZClearance) + "F " + fmt(VertJog)
            paths += "G0 X" + str(fmt(first.x)) + "Y" + \
                str(fmt(first.y)) + "F " + fmt(HorizJog) + "\n"
        Zprevious = ZCurrent
        ZCurrent = ZCurrent - abs(StepDown)

    # do the final Z value
    paths += convert(toolpath, Z=ZFinalDepth, Zprevious=Zprevious, PlungeAngle=PlungeAngle,
                     vf=VertFeed, hf=HorizFeed)

    # when plunging with != 90 degree we have to do one last pass to clear the remaining ramp
    if PlungeAngle != 90.0:
        tanA = math.tan(math.pi * PlungeAngle / 180.0)
        if tanA <= 0.0:
            StopLength=None
        else:
            StopLength=abs(StepDown/tanA)
        paths += convert(toolpath, Z=ZFinalDepth, Zprevious=Zprevious, StopLength=StopLength,
                         vf=VertFeed, hf=HorizFeed)

    paths += "G0 Z" + str(ZClearance) + "F " + fmt(VertJog) + "\n"
    return paths

# the next two functions are for automatically populating tool
# numbers/height offset numbers based on previously active toolnumbers


def changeTool(obj, job):
    tlnum = 0
    for p in job.Group:
        if not hasattr(p, "Group"):
            if isinstance(p.Proxy, PathScripts.PathLoadTool.LoadTool) and p.ToolNumber > 0:
                tlnum = p.ToolNumber
            if p == obj:
                return tlnum
        elif hasattr(p, "Group"):
            for g in p.Group:
                if isinstance(g.Proxy, PathScripts.PathLoadTool.LoadTool):
                    tlnum = g.ToolNumber
                if g == obj:
                    return tlnum


def getLastToolLoad(obj):
    # This walks up the hierarchy and tries to find the closest preceding
    # toolchange.

    import PathScripts
    tc = None
    lastfound = None

    try:
        child = obj
        parent = obj.InList[0]
    except:
        parent = None

    while parent is not None:
        if hasattr(parent, 'Group'):
            sibs = parent.Group
            for g in sibs:
                if hasattr(g, 'Proxy'):
                    if isinstance(g.Proxy, PathScripts.PathLoadTool.LoadTool):
                        lastfound = g
                    if g == child:
                        tc = lastfound

        if tc is None:
            try:
                child = parent
                parent = parent.InList[0]
            except:
                parent = None
        else:
            return tc

    if tc is None:
        for g in FreeCAD.ActiveDocument.Objects:  # top level object
            try:
                if isinstance(g.Proxy, PathScripts.PathLoadTool.LoadTool):
                    lastfound = g
                if g == obj:
                    tc = lastfound
            except:
                continue
    return tc

def getToolControllers(obj):
    controllers = []
    try:
        parent = obj.InList[0]
    except:
        parent = None

    if parent is not None and hasattr(parent, 'Group'):
        sibs = parent.Group
        for g in sibs:
            if isinstance(g.Proxy, PathScripts.PathLoadTool.LoadTool):
                controllers.append(g.Name)
    return controllers



def findParentJob(obj):
    '''retrieves a parent job object for an operation or other Path object'''
    for i in obj.InList:
        if isinstance(i.Proxy, PathScripts.PathJob.ObjectPathJob):
            return i
        if i.TypeId == "Path::FeaturePython":
            grandParent = findParentJob(i)
            if grandParent is not None:
                return grandParent
    return None

def getTool(obj, number=0):
    "retrieves a tool from a hosting object with a tooltable, if any"
    job = findParentJob(obj)
    if job is not None:
        return job.Tooltable.getTool(number)
    return None

def GetJobs(jobname = None):
    '''returns all jobs in the current document.  If name is given, returns that job'''
    jobs = []
    for o in FreeCAD.ActiveDocument.Objects:
        if "Proxy" in o.PropertiesList:
            if isinstance(o.Proxy, PathJob.ObjectPathJob):
                if jobname is not None:
                    if o.Name == jobname:
                        jobs.append(o)
                else:
                    jobs.append(o)
    return jobs

def addToJob(obj, jobname = None):
    if jobname is not None:
        jobs = GetJobs(jobname)
        if len(jobs) == 1:
            job = jobs[0]
        else:
            FreeCAD.Console.PrintError("Didn't find the job")
            return None

    else:
        jobs = GetJobs()
        if len(jobs) == 0:
            job = PathJob.CommandJob.Create()

        elif len(jobs) == 1:
            job = jobs[0]
        else:
            #form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Path/DlgJobChooser.ui")
            form = FreeCADGui.PySideUic.loadUi(":/panels/DlgJobChooser.ui")
            
            mylist = [i.Name for i in jobs]
            form.cboProject.addItems(mylist)
            r = form.exec_()
            if r is False:
                return None
            else:
                print form.cboProject.currentText()
                job = [i for i in jobs if i.Name == form.cboProject.currentText()][0]

    g = job.Group
    g.append(obj)
    job.Group = g
    return job

def getLastZ(obj):
    ''' find the last z value in the job '''
    lastZ = ""
    for g in obj.Group:
        for c in g.Path.Commands:
            for n in c.Parameters:
                if n == 'Z':
                    lastZ = c.Parameters['Z']
    return lastZ

def rapid(x=None, y=None, z=None):
    """ Returns gcode string to perform a rapid move."""
    retstr = "G00"
    if (x is not None) or (y is not None) or (z is not None):
        if (x is not None):
            retstr += " X" + str("%.4f" % x)
        if (y is not None):
            retstr += " Y" + str("%.4f" % y)
        if (z is not None):
            retstr += " Z" + str("%.4f" % z)
    else:
        return ""
    return retstr + "\n"

def feed(x=None, y=None, z=None, horizFeed=0, vertFeed=0):
    """ Return gcode string to perform a linear feed."""
    global feedxy
    retstr = "G01 F"
    if(x is None) and (y is None):
        retstr += str("%.4f" % horizFeed)
    else:
        retstr += str("%.4f" % vertFeed)

    if (x is not None) or (y is not None) or (z is not None):
        if (x is not None):
            retstr += " X" + str("%.4f" % x)
        if (y is not None):
            retstr += " Y" + str("%.4f" % y)
        if (z is not None):
            retstr += " Z" + str("%.4f" % z)
    else:
        return ""
    return retstr + "\n"

def arc(cx, cy, sx, sy, ex, ey, horizFeed=0, ez=None, ccw=False):
    """
    Return gcode string to perform an arc.

    Assumes XY plane or helix around Z
    Don't worry about starting Z- assume that's dealt with elsewhere
    If start/end radii aren't within eps, abort.

    cx, cy -- arc center coordinates
    sx, sy -- arc start coordinates
    ex, ey -- arc end coordinates
    ez -- ending Z coordinate.  None unless helix.
    horizFeed -- horiz feed speed
    ccw -- arc direction
    """

    eps = 0.01
    if (math.sqrt((cx - sx)**2 + (cy - sy)**2) - math.sqrt((cx - ex)**2 + (cy - ey)**2)) >= eps:
        print "ERROR: Illegal arc: Start and end radii not equal"
        return ""

    retstr = ""
    if ccw:
        retstr += "G03 F" + str(horizFeed)
    else:
        retstr += "G02 F" + str(horizFeed)

    retstr += " X" + str("%.4f" % ex) + " Y" + str("%.4f" % ey)

    if ez is not None:
        retstr += " Z" + str("%.4f" % ez)

    retstr += " I" + str("%.4f" % (cx - sx)) + " J" + str("%.4f" % (cy - sy))

    return retstr + "\n"

def helicalPlunge(plungePos, rampangle, destZ, startZ, toold, plungeR, horizFeed):
    """
    Return gcode string to perform helical entry move.

    plungePos -- vector of the helical entry location
    destZ -- the lowest Z position or milling level
    startZ -- Starting Z position for helical move
    rampangle -- entry angle
    toold -- tool diameter
    plungeR -- the radius of the entry helix
    """
    # toold = self.radius * 2

    helixCmds = "(START HELICAL PLUNGE)\n"
    if(plungePos is None):
        raise Exception("Helical plunging requires a position!")
        return None

    helixX = plungePos.x + toold/2 * plungeR
    helixY = plungePos.y

    helixCirc = math.pi * toold * plungeR
    dzPerRev = math.sin(rampangle/180. * math.pi) * helixCirc

    # Go to the start of the helix position
    helixCmds += rapid(helixX, helixY)
    helixCmds += rapid(z=startZ)

    # Helix as required to get to the requested depth
    lastZ = startZ
    curZ = max(startZ-dzPerRev, destZ)
    done = False
    while not done:
        done = (curZ == destZ)
        # NOTE: FreeCAD doesn't render this, but at least LinuxCNC considers it valid
        # helixCmds += arc(plungePos.x, plungePos.y, helixX, helixY, helixX, helixY, ez = curZ, ccw=True)

        # Use two half-helixes; FreeCAD renders that correctly,
        # and it fits with the other code breaking up 360-degree arcs
        helixCmds += arc(plungePos.x, plungePos.y, helixX, helixY, helixX - toold * plungeR, helixY, horizFeed, ez=(curZ + lastZ)/2., ccw=True)
        helixCmds += arc(plungePos.x, plungePos.y, helixX - toold * plungeR, helixY, helixX, helixY, horizFeed, ez=curZ, ccw=True)
        lastZ = curZ
        curZ = max(curZ - dzPerRev, destZ)

    return helixCmds

def rampPlunge(edge, rampangle, destZ, startZ):
    """
    Return gcode string to linearly ramp down to milling level.

    edge -- edge to follow
    rampangle -- entry angle
    destZ -- Final Z depth
    startZ -- Starting Z depth

    FIXME: This ramps along the first edge, assuming it's long
    enough, NOT just wiggling back and forth by ~0.75 * toolD.
    Not sure if that's any worse, but it's simpler
    I think this should be changed to be limited to a maximum ramp size.  Otherwise machine time will get longer than it needs to be.
    """

    rampCmds = "(START RAMP PLUNGE)\n"
    if(edge is None):
        raise Exception("Ramp plunging requires an edge!")
        return None

    sPoint = edge.Vertexes[0].Point
    ePoint = edge.Vertexes[1].Point
    # Evidently edges can get flipped- pick the right one in this case
    # FIXME: This is iffy code, based on what already existed in the "for vpos ..." loop below
    if ePoint == sPoint:
        # print "FLIP"
        ePoint = edge.Vertexes[-1].Point

    rampDist = edge.Length
    rampDZ = math.sin(rampangle/180. * math.pi) * rampDist

    rampCmds += rapid(sPoint.x, sPoint.y)
    rampCmds += rapid(z=startZ)

    # Ramp down to the requested depth
    # FIXME: This might be an arc, so handle that as well

    curZ = max(startZ-rampDZ, destZ)
    done = False
    while not done:
        done = (curZ == destZ)

        # If it's an arc, handle it!
        if isinstance(edge.Curve, Part.Circle):
            raise Exception("rampPlunge: Screw it, not handling an arc.")
        # Straight feed! Easy!
        else:
            rampCmds += feed(ePoint.x, ePoint.y, curZ)
            rampCmds += feed(sPoint.x, sPoint.y)

        curZ = max(curZ - rampDZ, destZ)

    return rampCmds


class depth_params:
    '''calculates the intermediate depth values for various operations given the starting, ending, and stepdown parameters'''

    def __init__(self, clearance_height, rapid_safety_space, start_depth, step_down, z_finish_depth, final_depth, user_depths=None):
        self.clearance_height = clearance_height
        self.rapid_safety_space = math.fabs(rapid_safety_space)
        self.start_depth = start_depth
        self.step_down = math.fabs(step_down)
        self.z_finish_depth = math.fabs(z_finish_depth)
        self.final_depth = final_depth
        self.user_depths = user_depths

    def get_depths(self):
        depths = []
        if self.user_depths is not None:
            depths = self.user_depths
        else:
            depth = self.final_depth
            depths = [depth]
            depth += self.z_finish_depth
            if depth + 0.0000001 < self.start_depth:
                if self.z_finish_depth > 0.0000001:
                    depths.insert(0, depth)
                layer_count = int((self.start_depth - depth) /
                                  self.step_down - 0.0000001) + 1
                if layer_count > 0:
                    layer_depth = (self.start_depth - depth) / layer_count
                    for i in range(1, layer_count):
                        depth += layer_depth
                        depths.append(depth)
        depths.reverse()
        return depths
