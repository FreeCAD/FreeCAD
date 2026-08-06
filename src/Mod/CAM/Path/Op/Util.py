# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2018 sliptonic <shopinthewoods@gmail.com>               *
# *   Copyright (c) 2021 Schildkroet                                        *
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
import Path
import Path.Dressup.Utils as PathDressup
import math
import time

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Part = LazyLoader("Part", globals(), "Part")

__title__ = "Util - Utility functions for CAM operations."
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Collection of functions used by various operations. The functions are specific to CAM and the algorithms employed by CAM's operations."


PrintWireDebug = False

if False:
    Path.Log.setLevel(Path.Log.Level.DEBUG, Path.Log.thisModule())
    Path.Log.trackModule(Path.Log.thisModule())
else:
    Path.Log.setLevel(Path.Log.Level.INFO, Path.Log.thisModule())

translate = FreeCAD.Qt.translate


def debugEdge(label, e):
    """debugEdge(label, e) ... prints a python statement to create e
    Currently lines and arcs are supported."""
    if not PrintWireDebug:
        return
    p0 = e.valueAt(e.FirstParameter)
    p1 = e.valueAt(e.LastParameter)
    if isinstance(e.Curve, Part.Line):
        print(
            "%s Part.makeLine((%.2f, %.2f, %.2f), (%.2f, %.2f, %.2f))"
            % (label, p0.x, p0.y, p0.z, p1.x, p1.y, p1.z)
        )
    elif isinstance(e.Curve, Part.Circle):
        r = e.Curve.Radius
        c = e.Curve.Center
        a = e.Curve.Axis
        xu = e.Curve.AngleXU
        if a.z < 0:
            first = math.degrees(xu - e.FirstParameter)
        else:
            first = math.degrees(xu + e.FirstParameter)
        last = first + math.degrees(e.LastParameter - e.FirstParameter)
        print(
            "%s Part.makeCircle(%.2f, App.Vector(%.2f, %.2f, %.2f), App.Vector(%.2f, %.2f, %.2f), %.2f, %.2f)"
            % (label, r, c.x, c.y, c.z, a.x, a.y, a.z, first, last)
        )
    else:
        print(
            "%s %s (%.2f, %.2f, %.2f) -> (%.2f, %.2f, %.2f)"
            % (label, type(e.Curve).__name__, p0.x, p0.y, p0.z, p1.x, p1.y, p1.z)
        )


def makeWires(inEdges):
    """makeWires ... function to make non-forking wires from a collection of edges"""
    edgelists = Part.sortEdges(inEdges)
    result = [Part.Wire(e) for e in edgelists]
    return result


def debugWire(label, w):
    """debugWire(label, w) ... prints python statements for all edges of w to be added to the object tree in a group."""
    if not PrintWireDebug:
        return
    print("#%s wire >>>>>>>>>>>>>>>>>>>>>>>>" % label)
    print("grp = FreeCAD.ActiveDocument.addObject('App::DocumentObjectGroup', '%s')" % label)
    for i, e in enumerate(w.Edges):
        edge = "%s_e%d" % (label, i)
        debugEdge("%s = " % edge, e)
        print("Part.show(%s, '%s')" % (edge, edge))
        print("grp.addObject(FreeCAD.ActiveDocument.ActiveObject)")
    print("#%s wire <<<<<<<<<<<<<<<<<<<<<<<<" % label)


def getCoincideTolerance(edges):
    """getCoincideTolerance(edges) ... Returns actual tolerance of edges connection in wire
    Assumes the edges are in an order so they can be connected.
    Return None if any edge length less than defined tolerance

    Note:
    Default tolerance of methods Part.sortEdges() and Part.__sortEdges__()
    is Base.Precision.confusion() => 1e-7
    """
    tolerance = FreeCAD.Base.Precision.confusion()
    for i in range(len(edges) - 1):
        e1 = edges[i]
        e1f = e1.valueAt(e1.FirstParameter)
        e1l = e1.valueAt(e1.LastParameter)
        e2 = edges[i + 1]
        e2f = e2.valueAt(e2.FirstParameter)
        e2l = e2.valueAt(e2.LastParameter)
        pairs = ((e1l, e2f), (e1l, e2l), (e1f, e2f), (e1f, e2l))
        dist = min((p2 - p1).Length for p1, p2 in pairs)
        tolerance = max(dist, tolerance)

    if any(e.Length < 2 * tolerance for e in edges):
        Path.Log.error(
            "Can not define tolerance edge connection. "
            "One of the edge has length less than defined tolerance or edges not sorted."
        )
        return None

    return tolerance


def _orientEdges(inEdges):
    """_orientEdges(inEdges) ... internal worker function to orient edges so the last vertex of one edge connects to the first vertex of the next edge.
    Assumes the edges are in an order so they can be connected."""
    Path.Log.track()

    tol = getCoincideTolerance(inEdges)
    if not tol:
        return None

    # orient all edges of the wire so each edge's last value connects to the next edge's first value
    e0 = inEdges[0]
    # well, even the very first edge could be misoriented, so let's try and connect it to the second
    if 1 < len(inEdges):
        last = e0.valueAt(e0.LastParameter)
        e1 = inEdges[1]
        if not Path.Geom.pointsCoincide(
            last, e1.valueAt(e1.FirstParameter), tol
        ) and not Path.Geom.pointsCoincide(last, e1.valueAt(e1.LastParameter), tol):
            debugEdge("#  _orientEdges - flip first", e0)
            e0 = Path.Geom.flipEdge(e0)

    edges = [e0]
    last = e0.valueAt(e0.LastParameter)
    for e in inEdges[1:]:
        ef = e.valueAt(e.FirstParameter)
        edge = e if Path.Geom.pointsCoincide(last, ef, tol) else Path.Geom.flipEdge(e)
        edges.append(edge)
        last = edge.valueAt(edge.LastParameter)
    return edges


def _isWireClockwise(w):
    """_isWireClockwise(w) ... return True if wire is oriented clockwise.
    Assumes the edges of w are already properly oriented - for generic access use isWireClockwise(w).
    """
    # handle wires consisting of a single circle or 2 edges where one is an arc.
    # in both cases, because the edges are expected to be oriented correctly, the orientation can be
    # determined by looking at (one of) the circle curves.
    if len(w.Edges) <= 2 and isinstance(w.Edges[0].Curve, Part.Circle):
        return 0 > w.Edges[0].Curve.Axis.z
    if len(w.Edges) == 2 and isinstance(w.Edges[1].Curve, Part.Circle):
        return 0 > w.Edges[1].Curve.Axis.z

    # for all other wires we presume they are polygonial and refer to Gauss
    # https://en.wikipedia.org/wiki/Shoelace_formula
    area = 0
    for e in w.Edges:
        v0 = e.valueAt(e.FirstParameter)
        v1 = e.valueAt(e.LastParameter)
        area = area + (v0.x * v1.y - v1.x * v0.y)
    Path.Log.track(area)
    return area < 0


def isWireClockwise(w):
    """isWireClockwise(w) ... returns True if the wire winds clockwise."""
    return _isWireClockwise(Part.Wire(_orientEdges(w.Edges)))


def orientWire(w, forward=True):
    """orientWire(w, forward=True) ... orients given wire in a specific direction.
    If forward = True (the default) the wire is oriented clockwise, looking down the negative Z axis.
    If forward = False the wire is oriented counter clockwise.
    If forward = None the orientation is determined by the order in which the edges appear in the wire.
    """
    Path.Log.debug("orienting forward: {}: {} edges".format(forward, len(w.Edges)))
    wire = Part.Wire(_orientEdges(w.Edges))
    if forward is not None:
        if forward != _isWireClockwise(wire):
            Path.Log.track("orientWire - needs flipping")
            return Path.Geom.flipWire(wire)
        Path.Log.track("orientWire - ok")
    return wire


def approximateWire(wire, tolerance=0.01):
    """approximateWire approximates any non-line/arc edges with lines or arcs.
    Edges that are lines or circular arcs are kept as-is.
    tolerance: Deflection tolerance for approximation. Must be positive if wire contains non-line/arc edges.
    Returns the wire with non-line/arc edges replaced by arcs and line segments.
    """
    processed_edges = []
    modified = False
    for edge in wire.Edges:
        curve = edge.Curve
        if isinstance(curve, (Part.Line, Part.LineSegment, Part.Circle, Part.ArcOfCircle)):
            # Keep lines and arcs as-is
            processed_edges.append(edge)
        else:
            # Approximate with lines and arcs
            if tolerance <= 0:
                raise ValueError(
                    "tolerance parameter is required to be a positive value to approximate non-line/arc edges"
                )
            modified = True

            # Convert to BSpline first if appropriate, to enable arc fitting
            if isinstance(curve, (Part.Ellipse, Part.Hyperbola, Part.Parabola)):
                # Convert edge to NURBS (BSpline)
                shape = edge.toNurbs()
                edge = shape.Edges[0]
            elif isinstance(curve, Part.BezierCurve):
                # Convert BezierCurve to BSpline
                curve = edge.Curve.toBSpline()
                edge = curve.toShape()

            if isinstance(edge.Curve, Part.BSplineCurve):
                # Convert BSpline to arcs
                curve = edge.Curve
                trimmed_curve = curve.trim(*edge.ParameterRange)
                curves = trimmed_curve.toBiArcs(tolerance)
                for curve in curves:
                    processed_edges.append(curve.toShape())
            else:
                # For other curve types, fall back to discretization to line segments
                vertices = edge.discretize(Deflection=tolerance)
                line_edges = [
                    Part.makeLine(vertices[i], vertices[i + 1]) for i in range(len(vertices) - 1)
                ]
                processed_edges.extend(line_edges)

    # Reassemble the wire if any edges were replaced
    if modified:
        return Part.Wire(Part.__sortEdges__(processed_edges))
    return wire


def offsetWire(wire, base, offset, forward, Side=None, tolerance=0.01):
    """offsetWire ... offsets the wire away from base and orients the wire accordingly.
    The function tries to avoid most of the pitfalls of Part.makeOffset2D which is possible because all offsetting
    happens in the XY plane.
    tolerance: Deflection tolerance for discretization. Must be positive if wire contains non-line/arc edges.
    """
    Path.Log.track("offsetWire")

    # Pre-process the wire: approximate any non-line/arc edges with arcs and lines
    wire = approximateWire(wire, tolerance)

    if len(wire.Edges) == 1:
        edge = wire.Edges[0]
        curve = edge.Curve
        if isinstance(curve, Part.Circle) and wire.isClosed():
            # it's a full circle and there are some problems with that, see
            # https://www.freecad.org/wiki/Part%20Offset2D
            # it's easy to construct them manually though
            center = curve.Center
            radius = curve.Radius
            axis = FreeCAD.Vector(0, 0, -1) if forward else FreeCAD.Vector(0, 0, 1)
            checkSidePoint = FreeCAD.Vector(center.x + radius + tolerance * 2, center.y, center.z)
            if base.isInside(checkSidePoint, tolerance, True):
                if offset > radius or Path.Geom.isRoughly(offset, radius):
                    # offsetting a hole by its own radius (or more) makes the hole vanish
                    return None
                if Side:
                    Side[0] = "Inside"
                new_edge = Part.makeCircle(radius - offset, center, axis)  # inside
            else:
                new_edge = Part.makeCircle(radius + offset, center, axis)  # outside

            return Part.Wire([new_edge])

        if isinstance(curve, Part.Circle) and not wire.isClosed():
            # Process arc segment
            center = curve.Center
            radius = curve.Radius
            point1 = edge.firstVertex().Point
            point2 = edge.lastVertex().Point

            l1 = math.hypot((point1.x - center.x), (point1.y - center.y))
            l2 = math.hypot((point2.x - center.x), (point2.y - center.y))

            # Calculate angles based on x-axis (0 - PI/2)
            start_angle = math.acos((point1.x - center.x) / l1)
            end_angle = math.acos((point2.x - center.x) / l2)

            # Angles are based on x-axis (Mirrored on x-axis) -> negative y value means negative angle
            if point1.y < center.y:
                start_angle *= -1
            if point2.y < center.y:
                end_angle *= -1

            if curve.Axis.z < 0:
                start_angle, end_angle = end_angle, start_angle

            # check if arc should be created on other side
            vec1 = point1 - center
            len1 = math.hypot(vec1.x, vec1.y)
            len2 = len1 + tolerance * 2
            vec2 = vec1 * len2 / len1
            checkSidePoint = center + vec2

            axis = FreeCAD.Vector(0, 0, 1)

            if base.isInside(checkSidePoint, tolerance, True):
                if offset > radius or Path.Geom.isRoughly(offset, radius):
                    # inner offset should not be equal or greater than arc radius
                    return None
                if Side:
                    Side[0] = "Inside"
                circle = Part.Circle(center, axis, radius - offset)  # inside
            else:
                circle = Part.Circle(center, axis, radius + offset)  # outside

            arc = Part.ArcOfCircle(circle, start_angle, end_angle)
            edge = arc.toShape()
            if forward:
                # default arc is CCW, so edge should be flipped to get forward direction
                edge = Path.Geom.flipEdge(edge)

            return Part.Wire([edge])

        if isinstance(curve, (Part.Line, Part.LineSegment)):
            # offsetting a single edge doesn't work because there is an infinite
            # possible planes into which the edge could be offset
            # luckily, the plane here must be the XY-plane ...
            p0 = edge.Vertexes[0].Point
            v0 = edge.Vertexes[1].Point - p0
            n = v0.cross(FreeCAD.Vector(0, 0, 1))
            o = n.normalize() * offset
            edge.translate(o)

            # offset edde the other way if the result is inside
            if base.isInside(
                edge.valueAt((edge.FirstParameter + edge.LastParameter) / 2),
                offset / 2,
                True,
            ):
                edge.translate(-2 * o)

            # flip the edge if it's not on the right side of the original edge
            if forward is not None:
                v1 = edge.Vertexes[1].Point - p0
                left = Path.Geom.Side.Left == Path.Geom.Side.of(v0, v1)
                if left != forward:
                    edge = Path.Geom.flipEdge(edge)
            return Part.Wire([edge])

        # if we get to this point the assumption is that makeOffset2D can deal with the edge

    owire = orientWire(wire.makeOffset2D(offset), True)
    debugWire("makeOffset2D_%d" % len(wire.Edges), owire)

    if wire.isClosed():
        if not base.isInside(owire.Edges[0].Vertexes[0].Point, offset / 2, True):
            Path.Log.track("closed - outside")
            if Side:
                Side[0] = "Outside"
            return orientWire(owire, forward)
        Path.Log.track("closed - inside")
        if Side:
            Side[0] = "Inside"
        try:
            owire = wire.makeOffset2D(-offset)
        except Exception:
            # most likely offsetting didn't work because the wire is a hole
            # and the offset is too big - making the hole vanish
            return None
        # For negative offsets (holes) 'forward' is the other way
        if forward is None:
            return orientWire(owire, None)
        return orientWire(owire, not forward)

    # An edge is considered to be inside of shape if the mid point is inside
    # Of the remaining edges we take the longest wire to be the engraving side
    # Looking for a circle with the start vertex as center marks and end
    #  starting from there follow the edges until a circle with the end vertex as center is found
    #  if the traversed edges include any of the remaining from above, all those edges are remaining
    #  this is to also include edges which might partially be inside shape
    #  if they need to be discarded, split, that should happen in a post process
    # Depending on the Axis of the circle, and which side remains we know if the wire needs to be flipped

    # first, let's make sure all edges are oriented the proper way
    edges = _orientEdges(wire.Edges)

    # determine the start and end point
    start = edges[0].firstVertex().Point
    end = edges[-1].lastVertex().Point
    debugWire("wire", wire)
    debugWire("wedges", Part.Wire(edges))

    # find edges that are not inside the shape
    common = base.common(owire)
    insideEndpoints = [e.lastVertex().Point for e in common.Edges]
    insideEndpoints.append(common.Edges[0].firstVertex().Point)

    def isInside(edge):
        p0 = edge.firstVertex().Point
        p1 = edge.lastVertex().Point
        for p in insideEndpoints:
            if Path.Geom.pointsCoincide(p, p0, 0.01) or Path.Geom.pointsCoincide(p, p1, 0.01):
                return True
        return False

    outside = [e for e in owire.Edges if not isInside(e)]
    # discard all edges that are not part of the longest wire
    longestWire = None
    for w in [Part.Wire(el) for el in Part.sortEdges(outside)]:
        if not longestWire or longestWire.Length < w.Length:
            longestWire = w

    if len(outside) >= 2:
        debugWire("outside", Part.Wire(outside))
    debugWire("longest", longestWire)

    def isCircleAt(edge, center):
        """isCircleAt(edge, center) ... helper function returns True if edge is a circle at the given center."""
        if isinstance(edge.Curve, (Part.Circle, Part.ArcOfCircle)):
            return Path.Geom.pointsCoincide(edge.Curve.Center, center)
        return False

    # split offset wire into edges to the left side and edges to the right side
    collectLeft = False
    collectRight = False
    leftSideEdges = []
    rightSideEdges = []

    # traverse through all edges in order and start collecting them when we encounter
    # an end point (circle centered at one of the end points of the original wire).
    # should we come to an end point and determine that we've already collected the
    # next side, we're done
    for e in owire.Edges + owire.Edges:
        if isCircleAt(e, start):
            if Path.Geom.pointsCoincide(e.Curve.Axis, FreeCAD.Vector(0, 0, 1)):
                if not collectLeft and leftSideEdges:
                    break
                collectLeft = True
                collectRight = False
            else:
                if not collectRight and rightSideEdges:
                    break
                collectLeft = False
                collectRight = True
        elif isCircleAt(e, end):
            if Path.Geom.pointsCoincide(e.Curve.Axis, FreeCAD.Vector(0, 0, 1)):
                if not collectRight and rightSideEdges:
                    break
                collectLeft = False
                collectRight = True
            else:
                if not collectLeft and leftSideEdges:
                    break
                collectLeft = True
                collectRight = False
        elif collectLeft:
            leftSideEdges.append(e)
        elif collectRight:
            rightSideEdges.append(e)

    debugWire("left", Part.Wire(leftSideEdges))
    debugWire("right", Part.Wire(rightSideEdges))

    # figure out if all the left sided edges or the right sided edges are the ones
    # that are 'outside'. However, we return the full side.
    edges = leftSideEdges
    if longestWire:
        for e in longestWire.Edges:
            for e0 in rightSideEdges:
                if Path.Geom.edgesMatch(e, e0):
                    edges = rightSideEdges
                    Path.Log.debug("#use right side edges")
                    if not forward:
                        Path.Log.debug("#reverse")
                        edges.reverse()
                    return orientWire(Part.Wire(edges), None)

    # at this point we have the correct edges and they are in the order for forward
    # traversal (climb milling). If that's not what we want just reverse the order,
    # orientWire takes care of orienting the edges appropriately.
    Path.Log.debug("#use left side edges")
    if not forward:
        Path.Log.debug("#reverse")
        edges.reverse()

    return orientWire(Part.Wire(edges), None)


_ROTARY_AXES = ("A", "B", "C", "U", "V", "W")


def _stripRotaryAxes(path):
    """Return a copy of path with rotary-axis parameters removed.

    PathSegmentWalker accumulates A/B/C state and applies compensateRotation()
    to every subsequent move, mapping rotated-frame X/Y/Z back to world coords.
    For 3+2 ops the X/Y/Z stored in the gcode are already in the rotated
    workplane frame, so that compensation produces the wrong positions when we
    just want to read the toolpath geometry as-emitted (e.g. to compute a
    cleared area to compare against another op in the same rotated frame).
    Stripping the rotary parameters keeps the walker's internal A/B/C at zero
    so positions are passed through unrotated.
    """
    stripped = []
    for cmd in path.Commands:
        params = {k: v for k, v in cmd.Parameters.items() if k not in _ROTARY_AXES}
        if not params and any(k in cmd.Parameters for k in _ROTARY_AXES):
            # Pure rotary command (e.g. the leading G0 A45) — drop entirely.
            continue
        stripped.append(Path.Command(cmd.Name, params))
    return Path.Path(stripped)


def getClearedAreas(currentOp, bbox):
    """
    Returns the cleared area relevant to the operation
    - currentOp: the operation we are checking for. Only operations performed
      before this operation will be considered
    - bbox: the cleared region is only generated where it is close enough to
      impact the bbox region

    Operations whose Workplane differs from the current op's are skipped:
    each op's Path stores X/Y/Z in the rotated workplane frame used at
    generation time, and projecting cleared area between non-coplanar
    workplanes has no meaningful 2D interpretation. For ops that share a
    non-Z-up Workplane the path's leading rotary G0 is stripped before
    walking so positions are read in the same rotated frame as bbox.
    """
    clearedAreas = []
    job = currentOp.Proxy.job
    z = bbox.ZMin + job.GeometryTolerance.getValueAs("mm")
    z_up = FreeCAD.Vector(0, 0, 1)
    currentWp = getattr(currentOp, "Workplane", z_up)
    rotated = not currentWp.isEqual(z_up, 1e-6)
    for op in job.Operations.Group:
        baseOp = PathDressup.baseOp(op)
        if baseOp.Name == currentOp.Name:
            break
        if getattr(op, "RestMachiningPass", None):
            op = baseOp
        if not (getattr(baseOp, "Active", False) and op.Path):
            continue
        opWp = getattr(baseOp, "Workplane", z_up)
        if not opWp.isEqual(currentWp, 1e-6):
            continue
        tool = baseOp.ToolController.Tool
        diameter = tool.Diameter.getValueAs("mm")
        # for drills, dz translates to the full width part of the tool
        dz = 0 if not hasattr(tool, "TipAngle") else -drillTipLength(tool)
        opPath = _stripRotaryAxes(op.Path) if rotated else op.Path
        clearedAreas.append(opPath.getClearedArea(diameter, z + dz, bbox))
    return clearedAreas


def getOpSide(obj, default="Outside"):
    """getOpSide(obj) ...  offer side for op base"""

    def getVerticalFaces(edges, shape):
        """Returns vertical faces (wall around) that contains given edges
        Excludes faces which is longer than common edges"""
        vFaces = []
        for f in shape.Faces:
            if len(vFaces) == len(edges):
                break
            if isHorizontal(f):
                continue
            for hEdge in edges:
                hsh = hEdge.hashCode()
                if any(hsh == e.hashCode() for e in f.Edges) and all(
                    e.Length < hEdge.Length or isRoughly(hEdge.Length, e.Length)
                    for e in f.Edges
                    if isHorizontal(e)
                ):
                    vFaces.append(f)
                    edges.remove(hEdge)
                    break
        return vFaces

    if not obj.Base:
        return default
    isRoughly = Path.Geom.isRoughly
    isHorizontal = Path.Geom.isHorizontal
    base, subNames = obj.Base[0]
    if "Face" in subNames[0]:
        faces = [base.Shape.getElement(sub) for sub in subNames if sub.startswith("Face")]
        vFaces = []
        hFaces = []
        for face in faces:
            if isHorizontal(face):
                hFaces.append(face)
            else:
                vFaces.append(face)
        if vFaces:
            volume = Part.Compound(vFaces).Volume
            if volume > 0 or isRoughly(volume, 0):
                return "Outside"
            # check if vertical faces creates a closed area
            fzMin = min(e.BoundBox.ZMin for f in vFaces for e in f.Edges)
            bEdges = [e for f in vFaces for e in f.Edges if isRoughly(e.BoundBox.ZMax, fzMin)]
            wire = Part.Wire(Part.__sortEdges__(bEdges))
            if not wire.isClosed():  # for open area always offer 'Outside'
                return "Outside"
            if volume < 0 and not isRoughly(volume, 0):  # negative volume forms inner area
                return "Inside"
        if hFaces:
            vFaces = getVerticalFaces(hFaces[0].OuterWire.Edges, base.Shape)
            volume = Part.Compound(vFaces).Volume
            if volume < 0 and not isRoughly(volume, 0):  # negative volume forms inner area
                return "Inside"
            else:
                return "Outside"
    elif "Edge" in subNames[0]:
        edges = [base.Shape.getElement(sub) for sub in subNames if sub.startswith("Edge")]
        cluster = Part.getSortedClusters(edges)[0]
        wire = Part.Wire(Part.__sortEdges__(cluster))
        if not wire.isClosed():  # for open wire always offer 'Outside'
            return "Outside"
        vFaces = getVerticalFaces(edges, base.Shape)
        volume = Part.Compound(vFaces).Volume
        if volume < 0 and not isRoughly(volume, 0):  # negative volume forms inner area
            return "Inside"
        else:
            return "Outside"

    return default


def getCycleTimeEstimate(obj, formatted=True):
    """getCycleTimeEstimate(obj, formated=True) ... Returns operation cycle time estimation
    If formatted=True returns string which describes time in format 'hh:mm:ss'
    If formatted=False returns seconds as a float value"""
    baseOp = PathDressup.baseOp(obj)
    tc = baseOp.ToolController

    if tc is None or tc.ToolNumber == 0:
        Path.Log.error(translate("CAM", "No Tool Controller selected."))
        return translate("CAM", "Tool Error")

    hFeedrate = tc.HorizFeed.Value
    vFeedrate = tc.VertFeed.Value
    hRapidrate = tc.HorizRapid.Value
    vRapidrate = tc.VertRapid.Value

    if hFeedrate == 0 or vFeedrate == 0:
        if not Path.Preferences.suppressAllSpeedsWarning():
            Path.Log.warning(
                translate(
                    "CAM",
                    "Tool Controller feedrates required to calculate the cycle time.",
                )
            )
        return translate("CAM", "Tool Feedrate Error")

    if (hRapidrate == 0 or vRapidrate == 0) and not Path.Preferences.suppressRapidSpeedsWarning():
        Path.Log.warning(
            translate(
                "CAM",
                "Add Tool Controller Rapid Speeds on the SetupSheet for more accurate cycle times.",
            )
        )

    # Get the cycle time in seconds
    seconds = obj.Path.getCycleTime(hFeedrate, vFeedrate, hRapidrate, vRapidrate)

    if math.isnan(seconds):
        return translate("CAM", "Cycletime Error")

    if formatted:  # Convert the cycle time to a HH:MM:SS format
        return time.strftime("%H:%M:%S", time.gmtime(seconds))
    else:
        return seconds


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
