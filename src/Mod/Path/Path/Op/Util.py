# -*- coding: utf-8 -*-
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

from PySide.QtCore import QT_TRANSLATE_NOOP
import FreeCAD
import Path
import math

# lazily loaded modules
from lazy_loader.lazy_loader import LazyLoader

Part = LazyLoader("Part", globals(), "Part")

__title__ = "Util - Utility functions for Path operations."
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Collection of functions used by various Path operations. The functions are specific to Path and the algorithms employed by Path's operations."


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
    if Part.Line == type(e.Curve):
        print(
            "%s Part.makeLine((%.2f, %.2f, %.2f), (%.2f, %.2f, %.2f))"
            % (label, p0.x, p0.y, p0.z, p1.x, p1.y, p1.z)
        )
    elif Part.Circle == type(e.Curve):
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
    print(
        "grp = FreeCAD.ActiveDocument.addObject('App::DocumentObjectGroup', '%s')"
        % label
    )
    for i, e in enumerate(w.Edges):
        edge = "%s_e%d" % (label, i)
        debugEdge("%s = " % edge, e)
        print("Part.show(%s, '%s')" % (edge, edge))
        print("grp.addObject(FreeCAD.ActiveDocument.ActiveObject)")
    print("#%s wire <<<<<<<<<<<<<<<<<<<<<<<<" % label)


def _orientEdges(inEdges):
    """_orientEdges(inEdges) ... internal worker function to orient edges so the last vertex of one edge connects to the first vertex of the next edge.
    Assumes the edges are in an order so they can be connected."""
    Path.Log.track()
    # orient all edges of the wire so each edge's last value connects to the next edge's first value
    e0 = inEdges[0]
    # well, even the very first edge could be misoriented, so let's try and connect it to the second
    if 1 < len(inEdges):
        last = e0.valueAt(e0.LastParameter)
        e1 = inEdges[1]
        if not Path.Geom.pointsCoincide(
            last, e1.valueAt(e1.FirstParameter)
        ) and not Path.Geom.pointsCoincide(last, e1.valueAt(e1.LastParameter)):
            debugEdge("#  _orientEdges - flip first", e0)
            e0 = Path.Geom.flipEdge(e0)

    edges = [e0]
    last = e0.valueAt(e0.LastParameter)
    for e in inEdges[1:]:
        edge = (
            e
            if Path.Geom.pointsCoincide(last, e.valueAt(e.FirstParameter))
            else Path.Geom.flipEdge(e)
        )
        edges.append(edge)
        last = edge.valueAt(edge.LastParameter)
    return edges


def _isWireClockwise(w):
    """_isWireClockwise(w) ... return True if wire is oriented clockwise.
    Assumes the edges of w are already properly oriented - for generic access use isWireClockwise(w)."""
    # handle wires consisting of a single circle or 2 edges where one is an arc.
    # in both cases, because the edges are expected to be oriented correctly, the orientation can be
    # determined by looking at (one of) the circle curves.
    if 2 >= len(w.Edges) and Part.Circle == type(w.Edges[0].Curve):
        return 0 > w.Edges[0].Curve.Axis.z
    if 2 == len(w.Edges) and Part.Circle == type(w.Edges[1].Curve):
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
    If forward = None the orientation is determined by the order in which the edges appear in the wire."""
    Path.Log.debug("orienting forward: {}: {} edges".format(forward, len(w.Edges)))
    wire = Part.Wire(_orientEdges(w.Edges))
    if forward is not None:
        if forward != _isWireClockwise(wire):
            Path.Log.track("orientWire - needs flipping")
            return Path.Geom.flipWire(wire)
        Path.Log.track("orientWire - ok")
    return wire


def offsetWire(wire, base, offset, forward, Side=None):
    """offsetWire(wire, base, offset, forward) ... offsets the wire away from base and orients the wire accordingly.
    The function tries to avoid most of the pitfalls of Part.makeOffset2D which is possible because all offsetting
    happens in the XY plane.
    """
    Path.Log.track("offsetWire")

    if 1 == len(wire.Edges):
        edge = wire.Edges[0]
        curve = edge.Curve
        if Part.Circle == type(curve) and wire.isClosed():
            # it's a full circle and there are some problems with that, see
            # https://www.freecad.org/wiki/Part%20Offset2D
            # it's easy to construct them manually though
            z = -1 if forward else 1
            new_edge = Part.makeCircle(
                curve.Radius + offset, curve.Center, FreeCAD.Vector(0, 0, z)
            )
            if base.isInside(new_edge.Vertexes[0].Point, offset / 2, True):
                if offset > curve.Radius or Path.Geom.isRoughly(offset, curve.Radius):
                    # offsetting a hole by its own radius (or more) makes the hole vanish
                    return None
                if Side:
                    Side[0] = "Inside"
                    print("inside")
                new_edge = Part.makeCircle(
                    curve.Radius - offset, curve.Center, FreeCAD.Vector(0, 0, -z)
                )

            return Part.Wire([new_edge])

        if Part.Circle == type(curve) and not wire.isClosed():
            # Process arc segment
            z = -1 if forward else 1
            l1 = math.sqrt(
                (edge.Vertexes[0].Point.x - curve.Center.x) ** 2
                + (edge.Vertexes[0].Point.y - curve.Center.y) ** 2
            )
            l2 = math.sqrt(
                (edge.Vertexes[1].Point.x - curve.Center.x) ** 2
                + (edge.Vertexes[1].Point.y - curve.Center.y) ** 2
            )

            # Calculate angles based on x-axis (0 - PI/2)
            start_angle = math.acos((edge.Vertexes[0].Point.x - curve.Center.x) / l1)
            end_angle = math.acos((edge.Vertexes[1].Point.x - curve.Center.x) / l2)

            # Angles are based on x-axis (Mirrored on x-axis) -> negative y value means negative angle
            if edge.Vertexes[0].Point.y < curve.Center.y:
                start_angle *= -1
            if edge.Vertexes[1].Point.y < curve.Center.y:
                end_angle *= -1

            if (
                edge.Vertexes[0].Point.x > curve.Center.x
                or edge.Vertexes[1].Point.x > curve.Center.x
            ) and curve.AngleXU < 0:
                tmp = start_angle
                start_angle = end_angle
                end_angle = tmp

            # Inside / Outside
            if base.isInside(edge.Vertexes[0].Point, offset / 2, True):
                offset *= -1
                if Side:
                    Side[0] = "Inside"

            # Create new arc
            if curve.AngleXU > 0:
                edge = Part.ArcOfCircle(
                    Part.Circle(
                        curve.Center, FreeCAD.Vector(0, 0, 1), curve.Radius + offset
                    ),
                    start_angle,
                    end_angle,
                ).toShape()
            else:
                edge = Part.ArcOfCircle(
                    Part.Circle(
                        curve.Center, FreeCAD.Vector(0, 0, 1), curve.Radius - offset
                    ),
                    start_angle,
                    end_angle,
                ).toShape()

            return Part.Wire([edge])

        if Part.Line == type(curve) or Part.LineSegment == type(curve):
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
            if Path.Geom.pointsCoincide(p, p0, 0.01) or Path.Geom.pointsCoincide(
                p, p1, 0.01
            ):
                return True
        return False

    outside = [e for e in owire.Edges if not isInside(e)]
    # discard all edges that are not part of the longest wire
    longestWire = None
    for w in [Part.Wire(el) for el in Part.sortEdges(outside)]:
        if not longestWire or longestWire.Length < w.Length:
            longestWire = w

    debugWire("outside", Part.Wire(outside))
    debugWire("longest", longestWire)

    def isCircleAt(edge, center):
        """isCircleAt(edge, center) ... helper function returns True if edge is a circle at the given center."""
        if Part.Circle == type(edge.Curve) or Part.ArcOfCircle == type(edge.Curve):
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
