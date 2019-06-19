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
import Part
import Path
import PathScripts
import PathScripts.PathGeom as PathGeom
import TechDraw
import math
import numpy

from DraftGeomUtils import geomType
from FreeCAD import Vector
from PathScripts import PathJob
from PathScripts import PathLog
from PySide import QtCore
from PySide import QtGui

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)


UserInput = None


def waiting_effects(function):
    def new_function(*args, **kwargs):
        if not FreeCAD.GuiUp:
            return function(*args, **kwargs)
        QtGui.QApplication.setOverrideCursor(QtCore.Qt.WaitCursor)
        res = None
        try:
            res = function(*args, **kwargs)
        # don't catch exceptions - want to know where they are coming from ....
        # except Exception as e:
        #    raise e
        #    print("Error {}".format(e.args[0]))
        finally:
            QtGui.QApplication.restoreOverrideCursor()
        return res
    return new_function


def cleanedges(splines, precision):
    '''cleanedges([splines],precision). Convert BSpline curves, Beziers, to arcs that can be used for cnc paths.
    Returns Lines as is. Filters Circle and Arcs for over 180 degrees. Discretizes Ellipses. Ignores other geometry. '''
    PathLog.track()
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

    PathLog.track()
    points = obj.copy().discretize(Distance=eval('steps'))
    p0 = points[0]
    edgelist = []
    for p in points[1:]:
        edge = Part.makeLine((p0.x, p0.y, p0.z), (p.x, p.y, p.z))
        edgelist.append(edge)
        p0 = p
    return edgelist


def isDrillable(obj, candidate, tooldiameter=None, includePartials=False):
    """
    Checks candidates to see if they can be drilled.
    Candidates can be either faces - circular or cylindrical or circular edges.
    The tooldiameter can be optionally passed.  if passed, the check will return
    False for any holes smaller than the tooldiameter.
    obj=Shape
    candidate = Face or Edge
    tooldiameter=float
    """
    PathLog.track('obj: {} candidate: {} tooldiameter {}'.format(obj, candidate, tooldiameter))
    if list == type(obj):
        for shape in obj:
            if isDrillable(shape, candidate, tooldiameter, includePartials):
                return (True, shape)
        return (False, None)

    drillable = False
    try:
        if candidate.ShapeType == 'Face':
            face = candidate
            # eliminate flat faces
            if (round(face.ParameterRange[0], 8) == 0.0) and (round(face.ParameterRange[1], 8) == round(math.pi * 2, 8)):
                for edge in face.Edges:  # Find seam edge and check if aligned to Z axis.
                    if (isinstance(edge.Curve, Part.Line)):
                        PathLog.debug("candidate is a circle")
                        v0 = edge.Vertexes[0].Point
                        v1 = edge.Vertexes[1].Point
                        # check if the cylinder seam is vertically aligned.  Eliminate tilted holes
                        if (numpy.isclose(v1.sub(v0).x, 0, rtol=1e-05, atol=1e-06)) and \
                                (numpy.isclose(v1.sub(v0).y, 0, rtol=1e-05, atol=1e-06)):
                            drillable = True
                            # vector of top center
                            lsp = Vector(face.BoundBox.Center.x, face.BoundBox.Center.y, face.BoundBox.ZMax)
                            # vector of bottom center
                            lep = Vector(face.BoundBox.Center.x, face.BoundBox.Center.y, face.BoundBox.ZMin)
                            # check if the cylindrical 'lids' are inside the base
                            # object.  This eliminates extruded circles but allows
                            # actual holes.
                            if obj.isInside(lsp, 1e-6, False) or obj.isInside(lep, 1e-6, False):
                                PathLog.track("inside check failed. lsp: {}  lep: {}".format(lsp, lep))
                                drillable = False
                            # eliminate elliptical holes
                            elif not hasattr(face.Surface, "Radius"):
                                PathLog.debug("candidate face has no radius attribute")
                                drillable = False
                            else:
                                if tooldiameter is not None:
                                    drillable = face.Surface.Radius >= tooldiameter / 2
                                else:
                                    drillable = True
            elif type(face.Surface) == Part.Plane and PathGeom.pointsCoincide(face.Surface.Axis, FreeCAD.Vector(0, 0, 1)):
                if len(face.Edges) == 1 and type(face.Edges[0].Curve) == Part.Circle:
                    center = face.Edges[0].Curve.Center
                    if obj.isInside(center, 1e-6, False):
                        if tooldiameter is not None:
                            drillable = face.Edges[0].Curve.Radius >= tooldiameter / 2
                        else:
                            drillable = True
        else:
            for edge in candidate.Edges:
                if isinstance(edge.Curve, Part.Circle) and (includePartials or edge.isClosed()):
                    PathLog.debug("candidate is a circle or ellipse")
                    if not hasattr(edge.Curve, "Radius"):
                        PathLog.debug("No radius.  Ellipse.")
                        drillable = False
                    else:
                        PathLog.debug("Has Radius, Circle")
                        if tooldiameter is not None:
                            drillable = edge.Curve.Radius >= tooldiameter / 2
                            if not drillable:
                                FreeCAD.Console.PrintMessage(
                                    "Found a drillable hole with diameter: {}: "
                                    "too small for the current tool with "
                                    "diameter: {}".format(edge.Curve.Radius * 2, tooldiameter))
                        else:
                            drillable = True
        PathLog.debug("candidate is drillable: {}".format(drillable))
    except Exception as ex:
        PathLog.warning(translate("PathUtils", "Issue determine drillability: {}").format(ex))
    return drillable


# fixme set at 4 decimal places for testing
def fmt(val):
    return format(val, '.4f')


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
                candidates.append((wire.hashCode(), wire))
            if e.hashCode() == edge2.hashCode():
                candidates.append((wire.hashCode(), wire))
    loop = set([x for x in candidates if candidates.count(x) > 1])  # return the duplicate item
    if len(loop) != 1:
        return None
    loopwire = next(x for x in loop)[1]
    return loopwire


def horizontalEdgeLoop(obj, edge):
    '''horizontalEdgeLoop(obj, edge) ... returns a wire in the horizontal plane, if that is the only horizontal wire the given edge is a part of.'''
    h = edge.hashCode()
    wires = [w for w in obj.Shape.Wires if any(e.hashCode() == h for e in w.Edges)]
    loops = [w for w in wires if all(PathGeom.isHorizontal(e) for e in w.Edges) and PathGeom.isHorizontal(Part.Face(w))]
    if len(loops) == 1:
        return loops[0]
    return None


def horizontalFaceLoop(obj, face, faceList=None):
    '''horizontalFaceLoop(obj, face, faceList=None) ... returns a list of face names which form the walls of a vertical hole face is a part of.
    All face names listed in faceList must be part of the hole for the solution to be returned.'''

    wires = [horizontalEdgeLoop(obj, e) for e in face.Edges]
    # Not sure if sorting by Area is a premature optimization - but it seems
    # the loop we're looking for is typically the biggest of the them all.
    wires = sorted([w for w in wires if w], key=lambda w: Part.Face(w).Area)

    for wire in wires:
        hashes = [e.hashCode() for e in wire.Edges]

        # find all faces that share a an edge with the wire and are vertical
        faces = ["Face%d" % (i + 1) for i, f in enumerate(obj.Shape.Faces) if any(e.hashCode() in hashes for e in f.Edges) and PathGeom.isVertical(f)]

        if faceList and not all(f in faces for f in faceList):
            continue

        # verify they form a valid hole by getting the outline and comparing
        # the resulting XY footprint with that of the faces
        comp = Part.makeCompound([obj.Shape.getElement(f) for f in faces])
        outline = TechDraw.findShapeOutline(comp, 1, FreeCAD.Vector(0, 0, 1))

        # findShapeOutline always returns closed wires, by removing the
        # trace-backs single edge spikes don't contriubte to the bound box
        uniqueEdges = []
        for edge in outline.Edges:
            if any(PathGeom.edgesMatch(edge, e) for e in uniqueEdges):
                continue
            uniqueEdges.append(edge)
        w = Part.Wire(uniqueEdges)

        # if the faces really form the walls of a hole then the resulting
        # wire is still closed and it still has the same footprint
        bb1 = comp.BoundBox
        bb2 = w.BoundBox
        if w.isClosed() and PathGeom.isRoughly(bb1.XMin, bb2.XMin) and PathGeom.isRoughly(bb1.XMax, bb2.XMax) and PathGeom.isRoughly(bb1.YMin, bb2.YMin) and PathGeom.isRoughly(bb1.YMax, bb2.YMax):
            return faces
    return None


def filterArcs(arcEdge):
    '''filterArcs(Edge) -used to split arcs that over 180 degrees. Returns list '''
    PathLog.track()
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
            arcquad1 = s.valueAt((s.LastParameter - s.FirstParameter) * 0.25 + s.FirstParameter)  # future midpt for arc1
            arcquad2 = s.valueAt((s.LastParameter - s.FirstParameter) * 0.75 + s.FirstParameter)  # future midpt for arc2
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


def makeWorkplane(shape):
    """
    Creates a workplane circle at the ZMin level.
    """
    PathLog.track()
    loc = FreeCAD.Vector(shape.BoundBox.Center.x,
                         shape.BoundBox.Center.y,
                         shape.BoundBox.ZMin)
    c = Part.makeCircle(10, loc)
    return c


def getEnvelope(partshape, subshape=None, depthparams=None):
    '''
    getEnvelope(partshape, stockheight=None)
    returns a shape corresponding to the partshape silhouette extruded to height.
    if stockheight is given, the returned shape is extruded to that height otherwise the returned shape
    is the height of the original shape boundbox
    partshape = solid object
    stockheight = float - Absolute Z height of the top of material before cutting.
    '''
    PathLog.track(partshape, subshape, depthparams)

    zShift = 0
    if subshape is not None:
        if isinstance(subshape, Part.Face):
            PathLog.debug('processing a face')
            sec = Part.makeCompound([subshape])
        else:
            area = Path.Area(Fill=2, Coplanar=0).add(subshape)
            area.setPlane(makeWorkplane(partshape))
            PathLog.debug("About to section with params: {}".format(area.getParams()))
            sec = area.makeSections(heights=[0.0], project=True)[0].getShape()

        PathLog.debug('partshapeZmin: {}, subshapeZMin: {}, zShift: {}'.format(partshape.BoundBox.ZMin, subshape.BoundBox.ZMin, zShift))

    else:
        area = Path.Area(Fill=2, Coplanar=0).add(partshape)
        area.setPlane(makeWorkplane(partshape))
        sec = area.makeSections(heights=[0.0], project=True)[0].getShape()

    # If depthparams are passed, use it to calculate bottom and height of
    # envelope
    if depthparams is not None:
        eLength = depthparams.safe_height - depthparams.final_depth
        zShift = depthparams.final_depth - sec.BoundBox.ZMin
        PathLog.debug('boundbox zMIN: {} elength: {} zShift {}'.format(partshape.BoundBox.ZMin, eLength, zShift))
    else:
        eLength = partshape.BoundBox.ZLength - sec.BoundBox.ZMin

    # Shift the section based on selection and depthparams.
    newPlace = FreeCAD.Placement(FreeCAD.Vector(0, 0, zShift), sec.Placement.Rotation)
    sec.Placement = newPlace

    # Extrude the section to top of Boundbox or desired height
    envelopeshape = sec.extrude(FreeCAD.Vector(0, 0, eLength))
    if PathLog.getLevel(PathLog.thisModule()) == PathLog.Level.DEBUG:
        removalshape = FreeCAD.ActiveDocument.addObject("Part::Feature", "Envelope")
        removalshape.Shape = envelopeshape
    return envelopeshape


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


def getToolControllers(obj):
    '''returns all the tool controllers'''
    try:
        job = findParentJob(obj)
    except Exception:
        job = None

    if job:
        return job.ToolController
    return []


def findToolController(obj, name=None):
    '''returns a tool controller with a given name.
    If no name is specified, returns the first controller.
    if no controller is found, returns None'''

    PathLog.track('name: {}'.format(name))
    c = None
    if UserInput:
        c = UserInput.selectedToolController()
    if c is not None:
        return c

    controllers = getToolControllers(obj)

    if len(controllers) == 0:
        return None

    # If there's only one in the job, use it.
    if len(controllers) == 1:
        if name is None or name == controllers[0].Label:
            tc = controllers[0]
        else:
            tc = None
    elif name is not None:  # More than one, make the user choose.
        tc = [i for i in controllers if i.Label == name][0]
    elif UserInput:
        tc = UserInput.chooseToolController(controllers)
    return tc


def findParentJob(obj):
    '''retrieves a parent job object for an operation or other Path object'''
    PathLog.track()
    for i in obj.InList:
        if hasattr(i, 'Proxy') and isinstance(i.Proxy, PathScripts.PathJob.ObjectJob):
            return i
        if i.TypeId == "Path::FeaturePython" or i.TypeId == "Path::FeatureCompoundPython" or i.TypeId == "App::DocumentObjectGroup":
            grandParent = findParentJob(i)
            if grandParent is not None:
                return grandParent
    return None


def GetJobs(jobname=None):
    '''returns all jobs in the current document.  If name is given, returns that job'''
    if jobname:
        return [job for job in PathJob.Instances() if job.Name == jobname]
    return PathJob.Instances()


def addToJob(obj, jobname=None):
    '''adds a path object to a job
    obj = obj
    jobname = None'''
    PathLog.track(jobname)
    if jobname is not None:
        jobs = GetJobs(jobname)
        if len(jobs) == 1:
            job = jobs[0]
        else:
            PathLog.error(translate("Path", "Didn't find job %s") % jobname)
            return None
    else:
        jobs = GetJobs()
        if len(jobs) == 0 and UserInput:
            job = UserInput.createJob()
        elif len(jobs) == 1:
            job = jobs[0]
        elif UserInput:
            job = UserInput.chooseJob(jobs)

    if obj and job:
        job.Proxy.addOperation(obj)
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

    helixX = plungePos.x + toold / 2 * plungeR
    helixY = plungePos.y

    helixCirc = math.pi * toold * plungeR
    dzPerRev = math.sin(rampangle / 180. * math.pi) * helixCirc

    # Go to the start of the helix position
    helixCmds += rapid(helixX, helixY)
    helixCmds += rapid(z=startZ)

    # Helix as required to get to the requested depth
    lastZ = startZ
    curZ = max(startZ - dzPerRev, destZ)
    done = False
    while not done:
        done = (curZ == destZ)
        # NOTE: FreeCAD doesn't render this, but at least LinuxCNC considers it valid
        # helixCmds += arc(plungePos.x, plungePos.y, helixX, helixY, helixX, helixY, ez = curZ, ccw=True)

        # Use two half-helixes; FreeCAD renders that correctly,
        # and it fits with the other code breaking up 360-degree arcs
        helixCmds += arc(plungePos.x, plungePos.y, helixX, helixY, helixX - toold * plungeR, helixY, horizFeed, ez=(curZ + lastZ) / 2., ccw=True)
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
    rampDZ = math.sin(rampangle / 180. * math.pi) * rampDist

    rampCmds += rapid(sPoint.x, sPoint.y)
    rampCmds += rapid(z=startZ)

    # Ramp down to the requested depth
    # FIXME: This might be an arc, so handle that as well

    curZ = max(startZ - rampDZ, destZ)
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


def sort_jobs(locations, keys, attractors=[]):
    """ sort holes by the nearest neighbor method
        keys: two-element list of keys for X and Y coordinates. for example ['x','y']
        originally written by m0n5t3r for PathHelix
    """
    try:
        from queue import PriorityQueue
    except ImportError:
        from Queue import PriorityQueue
    from collections import defaultdict

    attractors = attractors or [keys[0]]

    def sqdist(a, b):
        """ square Euclidean distance """
        d = 0
        for k in keys:
            d += (a[k] - b[k]) ** 2

        return d

    def weight(location):
        w = 0

        for k in attractors:
            w += abs(location[k])

        return w

    def find_closest(location_list, location, dist):
        q = PriorityQueue()

        for i, j in enumerate(location_list):
            # prevent dictionary comparison by inserting the index
            q.put((dist(j, location) + weight(j), i, j))

        prio, i, result = q.get()

        return result

    out = []
    zero = defaultdict(lambda: 0)

    out.append(find_closest(locations, zero, sqdist))
    locations.remove(out[-1])

    while locations:
        closest = find_closest(locations, out[-1], sqdist)
        out.append(closest)
        locations.remove(closest)

    return out


def guessDepths(objshape, subs=None):
    """
    takes an object shape and optional list of subobjects and returns a depth_params
    object with suggested height/depth values.

    objshape = Part::Shape.
    subs = list of subobjects from objshape
    """

    bb = objshape.BoundBox  # parent boundbox
    clearance = bb.ZMax + 5.0
    safe = bb.ZMax
    start = bb.ZMax
    final = bb.ZMin

    if subs is not None:
        subobj = Part.makeCompound(subs)
        fbb = subobj.BoundBox  # feature boundbox
        start = fbb.ZMax

        if fbb.ZMax == fbb.ZMin and fbb.ZMax == bb.ZMax:  # top face
            final = fbb.ZMin
        elif fbb.ZMax > fbb.ZMin and fbb.ZMax == bb.ZMax:  # vertical face, full cut
            final = fbb.ZMin
        elif fbb.ZMax > fbb.ZMin and fbb.ZMin > bb.ZMin:  # internal vertical wall
            final = fbb.ZMin
        elif fbb.ZMax == fbb.ZMin and fbb.ZMax > bb.ZMin:  # face/shelf
            final = fbb.ZMin

    return depth_params(clearance, safe, start, 1.0, 0.0, final, user_depths=None, equalstep=False)


def drillTipLength(tool):
    """returns the length of the drillbit tip."""
    if tool.CuttingEdgeAngle == 180 or tool.CuttingEdgeAngle == 0.0 or tool.Diameter == 0.0:
        return 0.0
    else:
        if tool.CuttingEdgeAngle <= 0 or tool.CuttingEdgeAngle >= 180:
            PathLog.error(translate("Path", "Invalid Cutting Edge Angle %.2f, must be >0° and <=180°") % tool.CuttingEdgeAngle)
            return 0.0
        theta = math.radians(tool.CuttingEdgeAngle)
        length = (tool.Diameter / 2) / math.tan(theta / 2)
        if length < 0:
            PathLog.error(translate("Path", "Cutting Edge Angle (%.2f) results in negative tool tip length") % tool.CuttingEdgeAngle)
            return 0.0
        return length


class depth_params(object):
    '''calculates the intermediate depth values for various operations given the starting, ending, and stepdown parameters
    (self, clearance_height, safe_height, start_depth, step_down, z_finish_depth, final_depth, [user_depths=None], equalstep=False)

        Note: if user_depths are supplied, only user_depths will be used.

        clearance_height:   Height to clear all obstacles
        safe_height:        Height to clear raw stock material
        start_depth:        Top of Model
        step_down:          Distance to step down between passes (always positive)
        z_finish_step:      Maximum amount of material to remove on the final pass
        final_depth:        Lowest point of the cutting operation
        user_depths:        List of specified depths
        equalstep:          Boolean.  If True, steps down except Z_finish_depth will be balanced.
    '''

    def __init__(self, clearance_height, safe_height, start_depth, step_down, z_finish_step, final_depth, user_depths=None, equalstep=False):
        '''self, clearance_height, safe_height, start_depth, step_down, z_finish_depth, final_depth, [user_depths=None], equalstep=False'''
        if z_finish_step > step_down:
            raise ValueError('z_finish_step must be less than step_down')

        self.__clearance_height = clearance_height
        self.__safe_height = safe_height
        self.__start_depth = start_depth
        self.__step_down = math.fabs(step_down)
        self.__z_finish_step = math.fabs(z_finish_step)
        self.__final_depth = final_depth
        self.__user_depths = user_depths
        self.data = self.__get_depths(equalstep=equalstep)
        self.index = 0

    def __iter__(self):
        self.index = 0
        return self

    def __next__(self):
        if self.index == len(self.data):
            raise StopIteration
        self.index = self.index + 1
        return self.data[self.index - 1]

    def next(self):
        return self.__next__()

    @property
    def clearance_height(self):
        """
        Height of all vises, clamps, and other obstructions.  Rapid moves at clearance height
        are always assumed to be safe from collision.
        """
        return self.__clearance_height

    @property
    def safe_height(self):
        """
        Height of top of raw stock material.  Rapid moves above safe height are
        assumed to be safe within an operation.  May not be safe between
        operations or tool changes.
        All moves below safe height except retraction should be at feed rate.
        """
        return self.__safe_height

    @property
    def start_depth(self):
        """
        Start Depth is the top of the model.
        """
        return self.__start_depth

    @property
    def step_down(self):
        """
        Maximum step down value between passes.  Step-Down may be less than
        this value, especially if equalstep is True.
        """
        return self.__step_down

    @property
    def z_finish_depth(self):
        """
        The amount of material to remove on the finish pass.  If given, the
        final pass will remove exactly this amount.
        """
        return self.__z_finish_step

    @property
    def final_depth(self):
        """
        The height of the cutter during the last pass or finish pass if
        z_finish_pass is given.
        """
        return self.__final_depth

    @property
    def user_depths(self):
        """
        Returns a list of the user_specified depths.  If user_depths were given
        in __init__, these depths override all calculation and only these are
        used.
        """
        return self.__user_depths

    def __get_depths(self, equalstep=False):
        '''returns a list of depths to be used in order from first to last.
        equalstep=True: all steps down before the finish pass will be equalized.'''

        if self.user_depths is not None:
            return self.__user_depths

        total_depth = self.__start_depth - self.__final_depth

        if total_depth < 0:
            return []

        depths = [self.__final_depth]

        # apply finish step if necessary
        if self.__z_finish_step > 0:
            if self.__z_finish_step < total_depth:
                depths.append(self.__z_finish_step + self.__final_depth)
            else:
                return depths

        if equalstep:
            depths += self.__equal_steps(self.__start_depth, depths[-1], self.__step_down)[1:]
        else:
            depths += self.__fixed_steps(self.__start_depth, depths[-1], self.__step_down)[1:]

        depths.reverse()
        return depths

    def __equal_steps(self, start, stop, max_size):
        '''returns a list of depths beginning with the bottom (included), ending
        with the top (not included).
        all steps are of equal size, which is as big as possible but not bigger
        than max_size.'''

        steps_needed = math.ceil((start - stop) / max_size)
        depths = list(numpy.linspace(stop, start, steps_needed, endpoint=False))

        return depths

    def __fixed_steps(self, start, stop, size):
        '''returns a list of depths beginning with the bottom (included), ending
        with the top (not included).
        all steps are of size 'size' except the one at the bottom which can be
        smaller.'''

        fullsteps = int((start - stop) / size)
        last_step = start - (fullsteps * size)
        depths = list(numpy.linspace(last_step, start, fullsteps, endpoint=False))

        if last_step == stop:
            return depths
        else:
            return [stop] + depths
