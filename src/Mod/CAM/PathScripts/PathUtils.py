# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2014 Dan Falck <ddfalck@gmail.com>                      *
# *   Copyright (c) 2025 Billy Huddleston <billy@ivdc.com>                  *
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
"""PathUtils -common functions used in PathScripts for filtering, sorting, and generating gcode toolpath data"""

import FreeCAD
from FreeCAD import Vector
from PySide import QtCore
import Path
import Path.Main.Job as PathJob
import math
from numpy import linspace
import tsp_solver

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

DraftGeomUtils = LazyLoader("DraftGeomUtils", globals(), "DraftGeomUtils")
Part = LazyLoader("Part", globals(), "Part")
TechDraw = LazyLoader("TechDraw", globals(), "TechDraw")

translate = FreeCAD.Qt.translate


if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())


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
        from PySide import QtGui

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
    """Returns a loop of edges that includes the two edges.
    Useful for detecting boundaries of negative space features ie 'holes'
    If a unique loop is not found, returns None
    """

    Path.Log.track()
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
    return loopwire.Edges


def wiredetect(obj, edgeName):
    """Returns all edges from wire which includes the edge."""
    edge = obj.Shape.getElement(edgeName)
    for wire in obj.Shape.Wires:
        for e in wire.Edges:
            if e.hashCode() == edge.hashCode():
                return wire.Edges

    return None


def horizontalEdgeLoop(obj, edge, verbose=False):
    """Returns a loop of edges in the horizontal plane that includes one edge"""

    isHorizontal = Path.Geom.isHorizontal
    isRoughly = Path.Geom.isRoughly

    if not isHorizontal(edge) and verbose:
        # stop if selected edge is not horizontal
        return None

    # Trying to find edges in loop wires from shape
    ehash = edge.hashCode()
    wires = [w for w in obj.Shape.Wires if any(e.hashCode() == ehash for e in w.Edges)]
    loops = [
        w for w in wires if all(isHorizontal(e) for e in w.Edges) and isHorizontal(Part.Face(w))
    ]
    if len(loops) > 0:
        return loops[0].Edges

    # Trying to find edges in loop without wires from shape

    # get edges in horizontal plane with selected edge
    candidates = [
        e
        for e in obj.Shape.Edges
        if isHorizontal(e) and isRoughly(e.BoundBox.ZMin, edge.BoundBox.ZMin)
    ]

    # get cluster of edges from which closed wire can be created
    # this cluster should contain selected edge
    for cluster in Part.getSortedClusters(candidates):
        wire = Part.Wire(cluster)
        if wire.isClosed() and any(e.hashCode() == ehash for e in cluster):
            # cluster is found
            return cluster

    return None


def tangentEdgeLoop(obj, edge):
    """Returns a tangent loop of edges"""

    isCoincide = Path.Geom.pointsCoincide

    loop = [edge]
    hashes = [edge.hashCode()]
    startPoint = edge.Vertexes[0].Point
    lastEdge = edge
    lastIndex = -1
    repeatCount = 0
    while repeatCount < len(obj.Shape.Edges):
        repeatCount += 1

        lastPoint = lastEdge.Vertexes[lastIndex].Point
        lastTangent = lastEdge.tangentAt(lastEdge.ParameterRange[lastIndex])

        if isCoincide(lastEdge.Vertexes[lastIndex].Point, startPoint):
            # stop because return to start point and loop is closed
            break

        for e in obj.Shape.Edges:
            if e.hashCode() in hashes:
                # this edge is already in loop
                continue

            if isCoincide(lastPoint, e.Vertexes[0].Point):
                index = 0
            elif isCoincide(lastPoint, e.Vertexes[-1].Point):
                index = -1
            else:
                continue

            tangent = e.tangentAt(e.ParameterRange[index])
            if isCoincide(tangent, lastTangent, 0.05):
                # found next tangency edge
                loop.append(e)
                hashes.append(e.hashCode())
                lastEdge = e
                lastIndex = -1 if index == 0 else 0
                break

        else:
            # stop because next tangency edge was not found
            break

    if len(loop) > 1:
        # only if found tangent edges
        return loop

    return None


def horizontalFaceLoop(obj, face, faceList=None):
    """horizontalFaceLoop(obj, face, faceList=None) ... returns a list of face names which form the walls of a vertical hole face is a part of.
    All face names listed in faceList must be part of the hole for the solution to be returned."""

    isVertical = Path.Geom.isVertical
    isRoughly = Path.Geom.isRoughly

    if not all(isVertical(obj.Shape.getElement(f)) for f in faceList):
        # stop if selected faces is not vertical
        Path.Log.warning(
            translate(
                "CAM",
                "Selected faces should be vertical",
            )
        )
        return None

    cluster = [horizontalEdgeLoop(obj, e) for e in face.Edges]

    # use sorting by Area as simple optimization
    clusterSorted = sorted(
        [edges for edges in cluster if edges],
        key=lambda edges: Part.Face(Part.Wire(Part.sortEdges(edges)[0])).Area,
    )

    for edges in clusterSorted:
        hashes = [e.hashCode() for e in edges]

        # find all faces that share an edges and are vertical
        faces = [
            "Face%d" % (i + 1)
            for i, f in enumerate(obj.Shape.Faces)
            if any(e.hashCode() in hashes for e in f.Edges) and isVertical(f)
        ]

        if faceList and not all(f in faces for f in faceList):
            # not all selected faces in list of candidates faces
            continue

        # verify they form a valid hole by getting the outline and comparing
        # the resulting XY footprint with that of the faces
        comp = Part.makeCompound([obj.Shape.getElement(f) for f in faces])

        outline = TechDraw.findShapeOutline(comp, 1, Vector(0, 0, 1))

        # findShapeOutline always returns closed wires, by removing the
        # trace-backs single edge spikes don't contribute to the bound box
        uniqueEdges = []
        for edge in outline.Edges:
            if any(Path.Geom.edgesMatch(edge, e) for e in uniqueEdges):
                continue
            uniqueEdges.append(edge)

        w = Part.Wire(uniqueEdges)

        # if the faces really form the walls of a hole then the resulting
        # wire is still closed and it still has the same footprint
        bb1 = comp.BoundBox
        bb2 = w.BoundBox
        prec = 1  # used low precision because findShapeOutline() is dirty
        if (
            w.isClosed()
            and isRoughly(bb1.XMin, bb2.XMin, prec)
            and isRoughly(bb1.XMax, bb2.XMax, prec)
            and isRoughly(bb1.YMin, bb2.YMin, prec)
            and isRoughly(bb1.YMax, bb2.YMax, prec)
        ):
            return faces
    return None


def filterArcs(arcEdge):
    """filterArcs(Edge) -used to split an arc that is over 180 degrees. Returns list"""
    Path.Log.track()
    splitlist = []
    if isinstance(arcEdge.Curve, Part.Circle):
        angle = abs(arcEdge.LastParameter - arcEdge.FirstParameter)  # Angle in radians
        goodarc = angle <= math.pi

        if goodarc:
            splitlist.append(arcEdge)
        else:
            arcstpt = arcEdge.valueAt(arcEdge.FirstParameter)
            arcmid = arcEdge.valueAt(
                (arcEdge.LastParameter - arcEdge.FirstParameter) * 0.5 + arcEdge.FirstParameter
            )
            arcquad1 = arcEdge.valueAt(
                (arcEdge.LastParameter - arcEdge.FirstParameter) * 0.25 + arcEdge.FirstParameter
            )  # future midpt for arc1
            arcquad2 = arcEdge.valueAt(
                (arcEdge.LastParameter - arcEdge.FirstParameter) * 0.75 + arcEdge.FirstParameter
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
    Path.Log.track()
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
    Path.Log.track(partshape, subshape, depthparams)

    zShift = 0
    if subshape is not None:
        if isinstance(subshape, Part.Face):
            Path.Log.debug("processing a face")
            sec = Part.makeCompound([subshape])
        else:
            area = Path.Area(Fill=2, Coplanar=0).add(subshape)
            area.setPlane(makeWorkplane(partshape))
            Path.Log.debug("About to section with params: {}".format(area.getParams()))
            sec = area.makeSections(heights=[0.0], project=True)[0].getShape()

        Path.Log.debug(
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
        Path.Log.debug(
            "boundbox zMIN: {} elength: {} zShift {}".format(
                partshape.BoundBox.ZMin, eLength, zShift
            )
        )
    else:
        eLength = partshape.BoundBox.ZLength - sec.BoundBox.ZMin

    # Shift the section based on selection and depthparams.
    newPlace = FreeCAD.Placement(Vector(0, 0, zShift), sec.Placement.Rotation)
    sec.Placement = newPlace

    if Path.Geom.isRoughly(eLength, 0):
        # For 2D operations (e.g. laser cutting) use the section directly without extrusion
        envelopeshape = sec
    else:
        # Extrude the section to top of Boundbox or desired height
        envelopeshape = sec.extrude(Vector(0, 0, eLength))

    if Path.Log.getLevel(Path.Log.thisModule()) == Path.Log.Level.DEBUG:
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
    Inspired by _buildPathArea() from Path.Op.Area.py module. Adjustments made
    based on notes by @sliptonic at this webpage:
    https://github.com/sliptonic/FreeCAD/wiki/PathArea-notes."""
    Path.Log.debug("getOffsetArea()")

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
        arcmid = e.valueAt((e.LastParameter - e.FirstParameter) * 0.5 + e.FirstParameter)
        arcendpt = e.valueAt(e.LastParameter)
        arcofCirc = Part.ArcOfCircle(arcendpt, arcmid, arcstpt)
        newedge = arcofCirc.toShape()
    elif DraftGeomUtils.geomType(e) == "LineSegment" or DraftGeomUtils.geomType(e) == "Line":
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

    Path.Log.debug("op={} ({})".format(obj.Label, type(obj)))
    if job:
        return [tc for tc in job.Tools.Group if proxy.isToolSupported(obj, tc.Tool)]
    return []


def getToolShapeName(tool):
    if hasattr(tool, "ShapeName"):
        return tool.ShapeName.lower()
    if hasattr(tool, "ShapeType"):
        return tool.ShapeType.lower()
    return ""


def findToolController(obj, proxy, name=None):
    """returns a tool controller with a given name.
    If no name is specified, returns the first controller.
    if no controller is found, returns None"""

    Path.Log.track("name: {}".format(name))
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
    Path.Log.track()
    if hasattr(obj, "Proxy") and isinstance(obj.Proxy, PathJob.ObjectJob):
        return obj

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
    Path.Log.track(jobname)

    job = None
    if jobname is not None:
        jobs = GetJobs(jobname)
        if len(jobs) == 1:
            job = jobs[0]
        else:
            Path.Log.error(translate("Path", "Didn't find job {}".format(jobname)))
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


def sort_locations_tsp(locations, keys, attractors=None, startPoint=None, endPoint=None):
    """
    Python wrapper for the C++ TSP solver. Takes a list of dicts (locations),
    a list of keys (e.g. ['x', 'y']), and optional parameters.

    Parameters:
    - locations: List of dictionaries with point coordinates
    - keys: List of keys to use for coordinates (e.g. ['x', 'y'])
    - attractors: Optional parameter (not used, kept for compatibility)
    - startPoint: Optional starting point [x, y]
    - endPoint: Optional ending point [x, y]

    Returns the sorted list of locations in TSP order.
    If startPoint is None, the path is optimized to start near the first point in the original list,
    but may not start exactly at that point.
    """
    # Extract points from locations
    points = [(loc[keys[0]], loc[keys[1]]) for loc in locations]
    order = tsp_solver.solve(points=points, startPoint=startPoint, endPoint=endPoint)

    # Return the reordered locations
    return [locations[i] for i in order]


def sort_tunnels_tsp(tunnels, allowFlipping=False, routeStartPoint=None, routeEndPoint=None):
    """
    Python wrapper for the C++ TSP tunnel solver. Takes a list of dicts (tunnels),
    a list of keys for start/end coordinates, and optional parameters.

    Parameters:
    - tunnels: List of dictionaries with tunnel data. Each tunnel dictionary should contain:
        - startX: X-coordinate of the tunnel start point
        - startY: Y-coordinate of the tunnel start point
        - endX: X-coordinate of the tunnel end point
        - endY: Y-coordinate of the tunnel end point
        - isOpen: Boolean indicating if the tunnel is open (optional, defaults to True)
    - allowFlipping: Whether tunnels can be reversed (entry becomes exit)
    - routeStartPoint: Optional starting point [x, y] for the entire route
    - routeEndPoint: Optional ending point [x, y] for the entire route

    Returns the sorted list of tunnels in TSP order. Each returned tunnel dictionary
    will include the original keys plus:
    - flipped: Boolean indicating if the tunnel was reversed during optimization
    - index: Original index of the tunnel in the input list
    """
    # Call C++ TSP tunnel solver directly - it handles all the processing
    return tsp_solver.solveTunnels(
        tunnels=tunnels,
        allowFlipping=allowFlipping,
        routeStartPoint=routeStartPoint,
        routeEndPoint=routeEndPoint,
    )


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

    if not hasattr(tool, "TipAngle"):
        Path.Log.error(translate("Path", "Selected tool is not a drill"))
        return 0.0

    angle = tool.TipAngle

    if angle <= 0 or angle >= 180:
        Path.Log.error(
            translate("Path", "Invalid Cutting Edge Angle %.2f, must be >0° and <=180°") % angle
        )
        return 0.0

    theta = math.radians(angle)
    length = (float(tool.Diameter) / 2) / math.tan(theta / 2)

    if length < 0:
        Path.Log.error(
            translate("Path", "Cutting Edge Angle (%.2f) results in negative tool tip length")
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
            depths += self.__equal_steps(self.__start_depth, depths[-1], self.__step_down)[1:]
        else:
            depths += self.__fixed_steps(self.__start_depth, depths[-1], self.__step_down)[1:]

        depths.reverse()

        if len(depths) < 2:
            return depths

        return self.__filter_roughly_equal_depths(depths)

    def __filter_roughly_equal_depths(self, depths):
        """Depths arrive sorted from largest to smallest, positive to negative.
        Return unique list of depths, using Path.Geom.isRoughly() method to determine
        if the two values are equal.  Only one of two consecutive equals are removed.

        The assumption is that there are not enough consecutively roughly-equal depths
        to be removed, so as to eliminate an effective step-down depth with the removal
        of repetitive roughly-equal values."""

        depthcopy = sorted(depths)  # make a copy and sort low to high
        keep = [depthcopy[0]]
        for depth in depthcopy[1:]:
            if not Path.Geom.isRoughly(depth, keep[-1]):
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

        if Path.Geom.isRoughly(start, stop):
            return [stop]

        fullsteps = int((start - stop) / size)
        last_step = start - (fullsteps * size)
        depths = list(linspace(last_step, start, fullsteps, endpoint=False))

        if last_step == stop:
            return depths
        else:
            return [stop] + depths


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


def getPathWithPlacement(pathobj):
    """
    Applies the rotation, and then position of the obj's Placement
    to the obj's path
    """

    if pathobj.Path is None:
        return pathobj.Path

    # check for no placement or placement POS=(0,0,0), Yaw-Pitch-Roll=(0,0,0)
    # isIdentity() returns True if the placement has no displacement and no rotation
    if not hasattr(pathobj, "Placement") or pathobj.Placement.isIdentity():
        return pathobj.Path

    return applyPlacementToPath(pathobj.Placement, pathobj.Path)


def applyPlacementToPath(placement, path):
    """
    Applies the rotation, and then position of the placement to path
    """

    commands = []
    currX = 0
    currY = 0
    currZ = 0

    # Angles of rotation (on A, B or C) do not need translation but may need a correction on start position, get transformed angles of 0 deg.
    cmd = Path.Command("G0 A0 B0 C0")
    t = cmd.transform(placement)
    tparams = t.Parameters
    transA0 = tparams.get("A", 0)
    transB0 = tparams.get("B", 0)
    transC0 = tparams.get("C", 0)

    for cmd in path.Commands:
        if cmd.Name in Path.Geom.CmdMoveAll:
            params = cmd.Parameters
            currX = x = params.get("X", currX)
            currY = y = params.get("Y", currY)
            currZ = z = params.get("Z", currZ)

            x, y, z = placement.Rotation.multVec(FreeCAD.Vector(x, y, z))

            if x != currX:
                params.update({"X": x})
            if y != currY:
                params.update({"Y": y})
            if z != currZ:
                params.update({"Z": z})

            # Arcs need to have the I and J params rotated as well
            if cmd.Name in Path.Geom.CmdMoveArc:
                currI = i = params.get("I", 0)
                currJ = j = params.get("J", 0)

                i, j, k = placement.Rotation.multVec(FreeCAD.Vector(i, j, 0))

                if currI != i:
                    params.update({"I": i})
                if currJ != j:
                    params.update({"J": j})

            cmd.Parameters = params

        # Angles of rotation (on A, B or C) do not need translation, find values before translation.
        params = cmd.Parameters
        aVal = params.get("A", None)
        bVal = params.get("B", None)
        cVal = params.get("C", None)

        t = cmd.transform(placement)

        # Set angles of rotation on A, B or C corrected for the transformed angle of 0 deg..
        tparams = t.Parameters
        if aVal is not None:
            tparams.update({"A": transA0 + aVal})
        if bVal is not None:
            tparams.update({"B": transB0 + bVal})
        if cVal is not None:
            tparams.update({"C": transC0 + cVal})
        if aVal is not None or bVal is not None or cVal is not None:
            t.Parameters = tparams

        commands.append(t)
    newPath = Path.Path(commands)

    return newPath
