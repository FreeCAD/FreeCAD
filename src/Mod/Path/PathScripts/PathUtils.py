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
from DraftGeomUtils import geomType
import PathScripts
from PathScripts import PathJob
import numpy
import PathLog
#from math import pi
from FreeCAD import Vector

LOG_MODULE = 'PathUtils'
PathLog.setLevel(PathLog.Level.INFO, LOG_MODULE)
#PathLog.trackModule('PathUtils')

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

        elif geomType(spline) == "LineSegment":
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

def isDrillable(obj, candidate, tooldiameter=None):
    PathLog.track('obj: {} candidate: {} tooldiameter {}'.format(obj, candidate, tooldiameter))
    drillable = False
    if candidate.ShapeType == 'Face':
        face = candidate
        # eliminate flat faces
        if (round(face.ParameterRange[0], 8) == 0.0) and (round(face.ParameterRange[1], 8) == round(math.pi * 2, 8)):
            for edge in face.Edges:  # Find seam edge and check if aligned to Z axis.
                if (isinstance(edge.Curve, Part.Line)):
                    PathLog.debug("candidate is a circle")
                    v0 = edge.Vertexes[0].Point
                    v1 = edge.Vertexes[1].Point
                    if (v1.sub(v0).x == 0) and (v1.sub(v0).y == 0):
                        # vector of top center
                        lsp = Vector(face.BoundBox.Center.x,face.BoundBox.Center.y, face.BoundBox.ZMax)
                        # vector of bottom center
                        lep = Vector(face.BoundBox.Center.x,face.BoundBox.Center.y, face.BoundBox.ZMin)
                        if obj.isInside(lsp, 0, False) or obj.isInside(lep, 0, False):
                            drillable = False
                        # eliminate elliptical holes
                        elif not hasattr(face.Surface, "Radius"): #abs(face.BoundBox.XLength - face.BoundBox.YLength) > 0.05:
                            drillable = False
                        else:
                            if tooldiameter is not None:
                                drillable = face.Surface.Radius >= tooldiameter/2
                            else:
                                drillable = True
    else:
        for edge in candidate.Edges:
            if isinstance(edge.Curve, Part.Circle) and edge.isClosed():
                PathLog.debug("candidate is a circle or ellipse")
                if not hasattr(edge.Curve, "Radius"): #bbdiff > 0.05:
                    PathLog.debug("No radius.  Ellipse.")
                    drillable = False
                else:
                    PathLog.debug("Has Radius, Circle")
                    if tooldiameter is not None:
                        drillable = edge.Curve.Radius >= tooldiameter/2
                    else:
                        drillable = True
    PathLog.debug("candidate is drillable: {}".format(drillable))
    return drillable

# fixme set at 4 decimal places for testing
def fmt(val): return format(val, '.4f')

def segments(poly):
    ''' A sequence of (x,y) numeric coordinates pairs '''
    return zip(poly, poly[1:] + [poly[0]])

def loopdetect(obj, edge1, edge2):
    '''
    Returns a loop wire that includes the two edges.
    Useful for detecting boundaries of negative space features ie 'holes'
    If a unique loop is not found, returns None
    edge1 = edge
    edge2 = edge
    '''

    PathLog.track()
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
    elif isinstance(s.Curve, Part.LineSegment):
        pass
    return splitlist


def reverseEdge(e):
    if geomType(e) == "Circle":
        arcstpt = e.valueAt(e.FirstParameter)
        arcmid = e.valueAt((e.LastParameter - e.FirstParameter) * 0.5 + e.FirstParameter)
        arcendpt = e.valueAt(e.LastParameter)
        arcofCirc = Part.ArcOfCircle(arcendpt, arcmid, arcstpt)
        newedge = arcofCirc.toShape()
    elif geomType(e) == "LineSegment" or geomType(e) == "Line":
        stpt = e.valueAt(e.FirstParameter)
        endpt = e.valueAt(e.LastParameter)
        newedge = Part.makeLine(endpt, stpt)

    return newedge

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

def getToolControllers(obj):
    '''returns all the tool controllers'''
    controllers = []
    try:
        parent = findParentJob(obj)
    except:
        parent = None

    if parent is not None and hasattr(parent, 'Group'):
        sibs = parent.Group
        for g in sibs:
            if isinstance(g.Proxy, PathScripts.PathLoadTool.LoadTool):
                controllers.append(g)
    return controllers

def findToolController(obj, name=None):
    '''returns a tool controller with a given name.
    If no name is specified, returns the first controller.
    if no controller is found, returns None'''

    PathLog.track('name: {}'.format(name))
    c = None
    #First check if a user has selected a tool controller in the tree. Return the first one and remove all from selection
    for sel in FreeCADGui.Selection.getSelectionEx():
        if hasattr(sel.Object, 'Proxy'):
            if isinstance(sel.Object.Proxy, PathScripts.PathLoadTool.LoadTool):
                if c is None:
                    c = sel.Object
                FreeCADGui.Selection.removeSelection(sel.Object)
    if c is not None:
        return c

    controllers = getToolControllers(obj)

    if len(controllers) == 0:
        return None

    #If there's only one in the job, use it.
    if len(controllers) == 1:
        if name is None or name == controllers[0].Label:
            tc = controllers[0]
        else:
            tc = None
    elif name is not None:  #More than one, make the user choose.
        tc = [i for i in controllers if i.Label == name][0]
    else:
        #form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Path/DlgTCChooser.ui")
        form = FreeCADGui.PySideUic.loadUi(":/panels/DlgTCChooser.ui")
        mylist = [i.Label for i in controllers]
        form.uiToolController.addItems(mylist)
        r = form.exec_()
        if r is False:
            tc = None
        else:
            tc = [i for i in controllers if i.Label == form.uiToolController.currentText()][0]
    return tc

def findParentJob(obj):
    '''retrieves a parent job object for an operation or other Path object'''
    PathLog.track()
    for i in obj.InList:
        if isinstance(i.Proxy, PathScripts.PathJob.ObjectPathJob):
            return i
        if i.TypeId == "Path::FeaturePython" or i.TypeId == "Path::FeatureCompoundPython":
            grandParent = findParentJob(i)
            if grandParent is not None:
                return grandParent
    return None

def GetJobs(jobname = None):
    '''returns all jobs in the current document.  If name is given, returns that job'''
    PathLog.track()
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
    '''adds a path object to a job
    obj = obj
    jobname = None'''
    PathLog.track()
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
                print(form.cboProject.currentText())
                job = [i for i in jobs if i.Name == form.cboProject.currentText()][0]

    g = job.Group
    g.append(obj)
    job.Group = g
    return job

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
        print("ERROR: Illegal arc: Start and end radii not equal")
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
    '''calculates the intermediate depth values for various operations given the starting, ending, and stepdown parameters
    (self, clearance_height, rapid_safety_space, start_depth, step_down, z_finish_depth, final_depth, [user_depths=None])

        Note: if user_depths are supplied, only user_depths will be used.

        clearance_height:   Height to clear all obstacles
        rapid_safety_space: Height to rapid between locations
        start_depth:        Top of Stock
        step_down:          Distance to step down between passes (always positive)
        z_finish_step:      Maximum amount of material to remove on the final pass
        final_depth:        Lowest point of the cutting operation
        user_depths:        List of specified depths
    '''

    def __init__(self, clearance_height, rapid_safety_space, start_depth, step_down, z_finish_step, final_depth, user_depths=None):
        '''self, clearance_height, rapid_safety_space, start_depth, step_down, z_finish_depth, final_depth, [user_depths=None]'''
        if z_finish_step > step_down:
            raise ValueError('z_finish_step must be less than step_down')

        self.clearance_height = clearance_height
        self.rapid_safety_space = math.fabs(rapid_safety_space)
        self.start_depth = start_depth
        self.step_down = math.fabs(step_down)
        self.z_finish_step = math.fabs(z_finish_step)
        self.final_depth = final_depth
        self.user_depths = user_depths

    def get_depths(self, equalstep=False):
        '''returns a list of depths to be used in order from first to last.
        equalstep=True: all steps down before the finish pass will be equalized.'''

        if self.user_depths is not None:
            return self.user_depths

        total_depth = self.start_depth - self.final_depth

        if total_depth < 0:
            return []

        depths = [self.final_depth]

        # apply finish step if necessary
        if self.z_finish_step > 0:
            if self.z_finish_step < total_depth:
                depths.append(self.z_finish_step + self.final_depth)
            else:
                return depths

        if equalstep:
            depths += self.__equal_steps(self.start_depth, depths[-1], self.step_down)[1:]
        else:
            depths += self.__fixed_steps(self.start_depth, depths[-1], self.step_down)[1:]

        depths.reverse()
        return depths

    def __equal_steps(self, start, stop, max_size):
        '''returns a list of depths beginning with the bottom (included), ending
        with the top (not included).
        all steps are of equal size, which is as big as possible but not bigger
        than max_size.'''

        steps_needed = math.ceil((start - stop) / max_size)
        depths = numpy.linspace(stop, start, steps_needed, endpoint=False)

        return depths.tolist()

    def __fixed_steps(self, start, stop, size):
        '''returns a list of depths beginning with the bottom (included), ending
        with the top (not included).
        all steps are of size 'size' except the one at the bottom which can be
        smaller.'''

        fullsteps = int((start - stop) / size)
        last_step = start - (fullsteps * size)
        depths = numpy.linspace(last_step, start, fullsteps, endpoint=False)

        if last_step == stop:
            return depths.tolist()
        else:
            return [stop] + depths.tolist()
