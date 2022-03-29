# -*- coding: utf-8 -*-
# ***************************************************************************
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
"""PathUtils -common functions used in PathScripts for filtering, sorting, and generating gcode toolpath data """

import FreeCAD
from FreeCAD import Vector
from PathScripts import PathLog
from PySide import QtCore
from PySide import QtGui
import Path
import PathScripts.PathGeom as PathGeom
import PathScripts.PathJob as PathJob
import math
from numpy import linspace

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

DraftGeomUtils = LazyLoader("DraftGeomUtils", globals(), "DraftGeomUtils")
Part = LazyLoader("Part", globals(), "Part")
TechDraw = LazyLoader("TechDraw", globals(), "TechDraw")

translate = FreeCAD.Qt.translate


if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())


UserInput = None


class PathNoTCExistsException(Exception):
    """PathNoECExistsException is raised when no TC exists at all, or when all
    existing TCs are rejected by a given op.
    This is typically an error because avery op requires a TC."""

    def __init__(self):
        super().__init__("No Tool Controllers exist")


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


# set at 4 decimal places for testing
def fmt(val):
    return format(val, ".4f")


def segments(poly):
    """A sequence of (x,y) numeric coordinates pairs"""
    return zip(poly, poly[1:] + [poly[0]])


def loopdetect(obj, edge1, edge2):
    """
    Returns a loop wire that includes the two edges.
    Useful for detecting boundaries of negative space features ie 'holes'
    If a unique loop is not found, returns None
    edge1 = edge
    edge2 = edge
    """

    PathLog.track()
    candidates = []
    for wire in obj.Shape.Wires:
        for e in wire.Edges:
            if e.hashCode() == edge1.hashCode():
                candidates.append((wire.hashCode(), wire))
            if e.hashCode() == edge2.hashCode():
                candidates.append((wire.hashCode(), wire))
    loop = set(
        [x for x in candidates if candidates.count(x) > 1]
    )  # return the duplicate item
    if len(loop) != 1:
        return None
    loopwire = next(x for x in loop)[1]
    return loopwire


def horizontalEdgeLoop(obj, edge):
    """horizontalEdgeLoop(obj, edge) ... returns a wire in the horizontal plane, if that is the only horizontal wire the given edge is a part of."""
    h = edge.hashCode()
    wires = [w for w in obj.Shape.Wires if any(e.hashCode() == h for e in w.Edges)]
    loops = [
        w
        for w in wires
        if all(PathGeom.isHorizontal(e) for e in w.Edges)
        and PathGeom.isHorizontal(Part.Face(w))
    ]
    if len(loops) == 1:
        return loops[0]
    return None


def horizontalFaceLoop(obj, face, faceList=None):
    """horizontalFaceLoop(obj, face, faceList=None) ... returns a list of face names which form the walls of a vertical hole face is a part of.
    All face names listed in faceList must be part of the hole for the solution to be returned."""

    wires = [horizontalEdgeLoop(obj, e) for e in face.Edges]
    # Not sure if sorting by Area is a premature optimization - but it seems
    # the loop we're looking for is typically the biggest of the them all.
    wires = sorted([w for w in wires if w], key=lambda w: Part.Face(w).Area)

    for wire in wires:
        hashes = [e.hashCode() for e in wire.Edges]

        # find all faces that share a an edge with the wire and are vertical
        faces = [
            "Face%d" % (i + 1)
            for i, f in enumerate(obj.Shape.Faces)
            if any(e.hashCode() in hashes for e in f.Edges) and PathGeom.isVertical(f)
        ]

        if faceList and not all(f in faces for f in faceList):
            continue

        # verify they form a valid hole by getting the outline and comparing
        # the resulting XY footprint with that of the faces
        comp = Part.makeCompound([obj.Shape.getElement(f) for f in faces])
        outline = TechDraw.findShapeOutline(comp, 1, Vector(0, 0, 1))

        # findShapeOutline always returns closed wires, by removing the
        # trace-backs single edge spikes don't contribute to the bound box
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
        if (
            w.isClosed()
            and PathGeom.isRoughly(bb1.XMin, bb2.XMin)
            and PathGeom.isRoughly(bb1.XMax, bb2.XMax)
            and PathGeom.isRoughly(bb1.YMin, bb2.YMin)
            and PathGeom.isRoughly(bb1.YMax, bb2.YMax)
        ):
            return faces
    return None


def filterArcs(arcEdge):
    """filterArcs(Edge) -used to split an arc that is over 180 degrees. Returns list"""
    PathLog.track()
    splitlist = []
    if isinstance(arcEdge.Curve, Part.Circle):
        angle = abs(arcEdge.LastParameter - arcEdge.FirstParameter)  # Angle in radians
        goodarc = angle <= math.pi

        if goodarc:
            splitlist.append(arcEdge)
        else:
            arcstpt = arcEdge.valueAt(arcEdge.FirstParameter)
            arcmid = arcEdge.valueAt(
                (arcEdge.LastParameter - arcEdge.FirstParameter) * 0.5
                + arcEdge.FirstParameter
            )
            arcquad1 = arcEdge.valueAt(
                (arcEdge.LastParameter - arcEdge.FirstParameter) * 0.25
                + arcEdge.FirstParameter
            )  # future midpt for arc1
            arcquad2 = arcEdge.valueAt(
                (arcEdge.LastParameter - arcEdge.FirstParameter) * 0.75
                + arcEdge.FirstParameter
            )  # future midpt for arc2
            arcendpt = arcEdge.valueAt(arcEdge.LastParameter)
            # reconstruct with 2 arcs
            arcseg1 = Part.ArcOfCircle(arcstpt, arcquad1, arcmid)
            arcseg2 = Part.ArcOfCircle(arcmid, arcquad2, arcendpt)

            eseg1 = arcseg1.toShape()
            eseg2 = arcseg2.toShape()
            splitlist.append(eseg1)
            splitlist.append(eseg2)

    elif isinstance(arcEdge.Curve, Part.LineSegment):
        pass
    return splitlist


def makeWorkplane(shape):
    """
    Creates a workplane circle at the ZMin level.
    """
    PathLog.track()
    loc = Vector(shape.BoundBox.Center.x, shape.BoundBox.Center.y, shape.BoundBox.ZMin)
    c = Part.makeCircle(10, loc)
    return c


def getEnvelope(partshape, subshape=None, depthparams=None):
    """
    getEnvelope(partshape, stockheight=None)
    returns a shape corresponding to the partshape silhouette extruded to height.
    if stockheight is given, the returned shape is extruded to that height otherwise the returned shape
    is the height of the original shape boundbox
    partshape = solid object
    stockheight = float - Absolute Z height of the top of material before cutting.
    """
    PathLog.track(partshape, subshape, depthparams)

    zShift = 0
    if subshape is not None:
        if isinstance(subshape, Part.Face):
            PathLog.debug("processing a face")
            sec = Part.makeCompound([subshape])
        else:
            area = Path.Area(Fill=2, Coplanar=0).add(subshape)
            area.setPlane(makeWorkplane(partshape))
            PathLog.debug("About to section with params: {}".format(area.getParams()))
            sec = area.makeSections(heights=[0.0], project=True)[0].getShape()

        PathLog.debug(
            "partshapeZmin: {}, subshapeZMin: {}, zShift: {}".format(
                partshape.BoundBox.ZMin, subshape.BoundBox.ZMin, zShift
            )
        )

    else:
        area = Path.Area(Fill=2, Coplanar=0).add(partshape)
        area.setPlane(makeWorkplane(partshape))
        sec = area.makeSections(heights=[0.0], project=True)[0].getShape()

    # If depthparams are passed, use it to calculate bottom and height of
    # envelope
    if depthparams is not None:
        eLength = depthparams.safe_height - depthparams.final_depth
        zShift = depthparams.final_depth - sec.BoundBox.ZMin
        PathLog.debug(
            "boundbox zMIN: {} elength: {} zShift {}".format(
                partshape.BoundBox.ZMin, eLength, zShift
            )
        )
    else:
        eLength = partshape.BoundBox.ZLength - sec.BoundBox.ZMin

    # Shift the section based on selection and depthparams.
    newPlace = FreeCAD.Placement(Vector(0, 0, zShift), sec.Placement.Rotation)
    sec.Placement = newPlace

    # Extrude the section to top of Boundbox or desired height
    envelopeshape = sec.extrude(Vector(0, 0, eLength))
    if PathLog.getLevel(PathLog.thisModule()) == PathLog.Level.DEBUG:
        removalshape = FreeCAD.ActiveDocument.addObject("Part::Feature", "Envelope")
        removalshape.Shape = envelopeshape
    return envelopeshape


# Function to extract offset face from shape
def getOffsetArea(
    fcShape,
    offset,
    removeHoles=False,
    # Default: XY plane
    plane=Part.makeCircle(10),
    tolerance=1e-4,
):
    """Make an offset area of a shape, projected onto a plane.
    Positive offsets expand the area, negative offsets shrink it.
    Inspired by _buildPathArea() from PathAreaOp.py module. Adjustments made
    based on notes by @sliptonic at this webpage:
    https://github.com/sliptonic/FreeCAD/wiki/PathArea-notes."""
    PathLog.debug("getOffsetArea()")

    areaParams = {}
    areaParams["Offset"] = offset
    areaParams["Fill"] = 1  # 1
    areaParams["Outline"] = removeHoles
    areaParams["Coplanar"] = 0
    areaParams["SectionCount"] = 1  # -1 = full(all per depthparams??) sections
    areaParams["Reorient"] = True
    areaParams["OpenMode"] = 0
    areaParams["MaxArcPoints"] = 400  # 400
    areaParams["Project"] = True
    areaParams["FitArcs"] = False  # Can be buggy & expensive
    areaParams["Deflection"] = tolerance
    areaParams["Accuracy"] = tolerance
    areaParams["Tolerance"] = 1e-5  # Equal point tolerance
    areaParams["Simplify"] = True
    areaParams["CleanDistance"] = tolerance / 5

    area = Path.Area()  # Create instance of Area() class object
    # Set working plane normal to Z=1
    area.setPlane(makeWorkplane(plane))
    area.add(fcShape)
    area.setParams(**areaParams)  # set parameters

    offsetShape = area.getShape()
    if not offsetShape.Faces:
        return False
    return offsetShape


def reverseEdge(e):
    if DraftGeomUtils.geomType(e) == "Circle":
        arcstpt = e.valueAt(e.FirstParameter)
        arcmid = e.valueAt(
            (e.LastParameter - e.FirstParameter) * 0.5 + e.FirstParameter
        )
        arcendpt = e.valueAt(e.LastParameter)
        arcofCirc = Part.ArcOfCircle(arcendpt, arcmid, arcstpt)
        newedge = arcofCirc.toShape()
    elif (
        DraftGeomUtils.geomType(e) == "LineSegment"
        or DraftGeomUtils.geomType(e) == "Line"
    ):
        stpt = e.valueAt(e.FirstParameter)
        endpt = e.valueAt(e.LastParameter)
        newedge = Part.makeLine(endpt, stpt)

    return newedge


def getToolControllers(obj, proxy=None):
    """returns all the tool controllers"""
    if proxy is None:
        proxy = obj.Proxy
    try:
        job = findParentJob(obj)
    except Exception:
        job = None

    PathLog.debug("op={} ({})".format(obj.Label, type(obj)))
    if job:
        return [tc for tc in job.Tools.Group if proxy.isToolSupported(obj, tc.Tool)]
    return []


def findToolController(obj, proxy, name=None):
    """returns a tool controller with a given name.
    If no name is specified, returns the first controller.
    if no controller is found, returns None"""

    PathLog.track("name: {}".format(name))
    c = None
    if UserInput:
        c = UserInput.selectedToolController()
    if c is not None:
        return c

    controllers = getToolControllers(obj, proxy)

    if len(controllers) == 0:
        raise PathNoTCExistsException()

    # If there's only one in the job, use it.
    if len(controllers) == 1:
        if name is None or name == controllers[0].Label:
            tc = controllers[0]
        else:
            tc = None
    elif name is not None:
        tc = [i for i in controllers if i.Label == name][0]
    elif UserInput:  # More than one, make the user choose.
        tc = UserInput.chooseToolController(controllers)
    return tc


def findParentJob(obj):
    """retrieves a parent job object for an operation or other Path object"""
    PathLog.track()
    for i in obj.InList:
        if hasattr(i, "Proxy") and isinstance(i.Proxy, PathJob.ObjectJob):
            return i
        if (
            i.TypeId == "Path::FeaturePython"
            or i.TypeId == "Path::FeatureCompoundPython"
            or i.TypeId == "App::DocumentObjectGroup"
        ):
            grandParent = findParentJob(i)
            if grandParent is not None:
                return grandParent
    return None


def GetJobs(jobname=None):
    """returns all jobs in the current document.  If name is given, returns that job"""
    if jobname:
        return [job for job in PathJob.Instances() if job.Name == jobname]
    return PathJob.Instances()


def addToJob(obj, jobname=None):
    """adds a path object to a job
    obj = obj
    jobname = None"""
    PathLog.track(jobname)

    job = None
    if jobname is not None:
        jobs = GetJobs(jobname)
        if len(jobs) == 1:
            job = jobs[0]
        else:
            PathLog.error(translate("Path", "Didn't find job {}".format(jobname)))
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


def sort_locations(locations, keys, attractors=None):
    """sort holes by the nearest neighbor method
    keys: two-element list of keys for X and Y coordinates. for example ['x','y']
    originally written by m0n5t3r for PathHelix
    """
    from queue import PriorityQueue
    from collections import defaultdict

    if attractors is None:
        attractors = []

    attractors = attractors or [keys[0]]

    def sqdist(a, b):
        """square Euclidean distance"""
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

    return depth_params(
        clearance, safe, start, 1.0, 0.0, final, user_depths=None, equalstep=False
    )


def drillTipLength(tool):
    """returns the length of the drillbit tip."""

    if isinstance(tool, Path.Tool):
        PathLog.error(translate("Path", "Legacy Tools not supported"))
        return 0.0

    if not hasattr(tool, "TipAngle"):
        PathLog.error(translate("Path", "Selected tool is not a drill"))
        return 0.0

    angle = tool.TipAngle

    if angle <= 0 or angle >= 180:
        PathLog.error(
            translate("Path", "Invalid Cutting Edge Angle %.2f, must be >0° and <=180°")
            % angle
        )
        return 0.0

    theta = math.radians(angle)
    length = (float(tool.Diameter) / 2) / math.tan(theta / 2)

    if length < 0:
        PathLog.error(
            translate(
                "Path", "Cutting Edge Angle (%.2f) results in negative tool tip length"
            )
            % angle
        )
        return 0.0

    return length


class depth_params(object):
    """calculates the intermediate depth values for various operations given the starting, ending, and stepdown parameters
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
    """

    def __init__(
        self,
        clearance_height,
        safe_height,
        start_depth,
        step_down,
        z_finish_step,
        final_depth,
        user_depths=None,
        equalstep=False,
    ):
        """self, clearance_height, safe_height, start_depth, step_down, z_finish_depth, final_depth, [user_depths=None], equalstep=False"""

        self.__clearance_height = clearance_height
        self.__safe_height = safe_height
        self.__start_depth = start_depth
        self.__step_down = math.fabs(step_down)
        self.__z_finish_step = math.fabs(z_finish_step)
        self.__final_depth = final_depth
        self.__user_depths = user_depths
        self.data = self.__get_depths(equalstep=equalstep)
        self.index = 0

        if self.__z_finish_step > self.__step_down:
            raise ValueError("z_finish_step must be less than step_down")

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
        """returns a list of depths to be used in order from first to last.
        equalstep=True: all steps down before the finish pass will be equalized."""

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
            depths += self.__equal_steps(
                self.__start_depth, depths[-1], self.__step_down
            )[1:]
        else:
            depths += self.__fixed_steps(
                self.__start_depth, depths[-1], self.__step_down
            )[1:]

        depths.reverse()

        if len(depths) < 2:
            return depths

        return self.__filter_roughly_equal_depths(depths)

    def __filter_roughly_equal_depths(self, depths):
        """Depths arrive sorted from largest to smallest, positive to negative.
        Return unique list of depths, using PathGeom.isRoughly() method to determine
        if the two values are equal.  Only one of two consecutive equals are removed.

        The assumption is that there are not enough consecutively roughly-equal depths
        to be removed, so as to eliminate an effective step-down depth with the removal
        of repetitive roughly-equal values."""

        depthcopy = sorted(depths)  # make a copy and sort low to high
        keep = [depthcopy[0]]
        for depth in depthcopy[1:]:
            if not PathGeom.isRoughly(depth, keep[-1]):
                keep.append(depth)
        keep.reverse()  # reverse results back high to low
        return keep

    def __equal_steps(self, start, stop, max_size):
        """returns a list of depths beginning with the bottom (included), ending
        with the top (not included).
        all steps are of equal size, which is as big as possible but not bigger
        than max_size."""

        steps_needed = math.ceil((start - stop) / max_size)
        depths = list(linspace(stop, start, steps_needed, endpoint=False))

        return depths

    def __fixed_steps(self, start, stop, size):
        """returns a list of depths beginning with the bottom (included), ending
        with the top (not included).
        all steps are of size 'size' except the one at the bottom which can be
        smaller."""

        fullsteps = int((start - stop) / size)
        last_step = start - (fullsteps * size)
        depths = list(linspace(last_step, start, fullsteps, endpoint=False))

        if last_step == stop:
            return depths
        else:
            return [stop] + depths


def extraKerf(tooldiameter, extrakerf=0.0):
    """
    Helper for converting Extrakerf property to PathArea params.
    ExtraKerf is a float % of tool diameter. Extrakerf must be larger than 0
    but may be more than 100% of the tool diameter.

    Returns a dict of ExtraPass (count of loops) and Stepover (distance between)
    {'ExtraPass': e, 'Stepover': s}
    """

    if extrakerf < 0:
        raise ValueError("extrakerf must be 0 or greater")

    if extrakerf == 0:
        return {"ExtraPass": 0, "Stepover": 0.0}

    passcount = math.ceil(extrakerf)
    passstep = (extrakerf * tooldiameter) / passcount
    return {"ExtraPass": passcount, "Stepover": passstep}


def simplify3dLine(line, tolerance=1e-4):
    """Simplify a line defined by a list of App.Vectors, while keeping the
    maximum deviation from the original line within the defined tolerance.
    Implementation of
    https://en.wikipedia.org/wiki/Ramer%E2%80%93Douglas%E2%80%93Peucker_algorithm"""
    stack = [(0, len(line) - 1)]
    results = []

    def processRange(start, end):
        """Internal worker. Process a range of Vector indices within the
        line."""
        if end - start < 2:
            results.extend(line[start:end])
            return
        # Find point with maximum distance
        maxIndex, maxDistance = 0, 0.0
        startPoint, endPoint = (line[start], line[end])
        for i in range(start + 1, end):
            v = line[i]
            distance = v.distanceToLineSegment(startPoint, endPoint).Length
            if distance > maxDistance:
                maxDistance = distance
                maxIndex = i
        if maxDistance > tolerance:
            # Push second branch first, to be executed last
            stack.append((maxIndex, end))
            stack.append((start, maxIndex))
        else:
            results.append(line[start])

    while len(stack):
        processRange(*stack.pop())
    # Each segment only appended its start point to the final result, so fill in
    # the last point.
    results.append(line[-1])
    return results


def RtoIJ(startpoint, command):
    """
    This function takes a startpoint and an arc command in radius mode and
    returns an arc command in IJ mode. Useful for preprocessor scripts
    """
    if "R" not in command.Parameters:
        raise ValueError("No R parameter in command")
    if command.Name not in ["G2", "G02", "G03", "G3"]:
        raise ValueError("Not an arc command")

    endpoint = command.Placement.Base
    radius = command.Parameters["R"]

    # calculate the IJ
    # we take a vector between the start and endpoints
    chord = endpoint.sub(startpoint)

    # Take its perpendicular (we assume the arc is in the XY plane)
    perp = chord.cross(Vector(0, 0, 1))

    # use pythagoras to get the perp length
    plength = math.sqrt(radius**2 - (chord.Length / 2) ** 2)
    perp.normalize()
    perp.scale(plength, plength, plength)

    # Calculate the relative center
    relativecenter = chord.scale(0.5, 0.5, 0.5).add(perp)

    # build new command
    params = {c: command.Parameters[c] for c in "XYZF" if c in command.Parameters}
    params["I"] = relativecenter.x
    params["J"] = relativecenter.y

    newcommand = Path.Command(command.Name)
    newcommand.Parameters = params

    return newcommand
