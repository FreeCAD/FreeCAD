# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2018 sliptonic <shopinthewoods@gmail.com>               *
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
import Part
import Path
import PathScripts.PathGeom as PathGeom
import PathScripts.PathLog as PathLog
import math

from PySide import QtCore

__title__ = "PathOpTools - Tools for Path operations."
__author__ = "sliptonic (Brad Collette)"
__url__ = "http://www.freecadweb.org"
__doc__ = "Collection of functions used by various Path operations. The functions are specific to Path and the algorithms employed by Path's operations."

if False:
    PathLog.setLevel(PathLog.Level.DEBUG, PathLog.thisModule())
    PathLog.trackModule(PathLog.thisModule())
else:
    PathLog.setLevel(PathLog.Level.INFO, PathLog.thisModule())

PrintWireDebug = False

# Qt translation handling
def translate(context, text, disambig=None):
    return QtCore.QCoreApplication.translate(context, text, disambig)

def debugEdge(label, e):
    '''debugEdge(label, e) ... prints a python statement to create e
    Currently lines and arcs are supported.'''
    if not PrintWireDebug:
        return
    p0 = e.valueAt(e.FirstParameter)
    p1 = e.valueAt(e.LastParameter)
    if Part.Line == type(e.Curve):
        print("%s Part.makeLine((%.2f, %.2f, %.2f), (%.2f, %.2f, %.2f))" % (label, p0.x, p0.y, p0.z, p1.x, p1.y, p1.z))
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
        print("%s Part.makeCircle(%.2f, App.Vector(%.2f, %.2f, %.2f), App.Vector(%.2f, %.2f, %.2f), %.2f, %.2f)" % (label, r, c.x, c.y, c.z, a.x, a.y, a.z, first, last))
    else:
        print("%s %s (%.2f, %.2f, %.2f) -> (%.2f, %.2f, %.2f)" % (label, type(e.Curve).__name__, p0.x, p0.y, p0.z, p1.x, p1.y, p1.z))

def debugWire(label, w):
    '''debugWire(label, w) ... prints python statements for all edges of w to be added to the object tree in a group.'''
    if not PrintWireDebug:
        return
    print("#%s wire >>>>>>>>>>>>>>>>>>>>>>>>" % label)
    print("grp = FreeCAD.ActiveDocument.addObject('App::DocumentObjectGroup', '%s')" % label)
    for i,e in enumerate(w.Edges):
        edge = "%s_e%d" % (label, i)
        debugEdge("%s = " % edge, e)
        print("Part.show(%s, '%s')" % (edge, edge))
        print("grp.addObject(FreeCAD.ActiveDocument.ActiveObject)")
    print("#%s wire <<<<<<<<<<<<<<<<<<<<<<<<" % label)

def _orientEdges(inEdges):
    '''_orientEdges(inEdges) ... internal worker function to orient edges so the last vertex of one edge connects to the first vertex of the next edge.
    Assumes the edges are in an order so they can be connected.'''
    PathLog.track()
    # orient all edges of the wire so each edge's last value connects to the next edge's first value
    e0 = inEdges[0]
    # well, even the very first edge could be misoriented, so let's try and connect it to the second
    if 1 < len(inEdges):
        last = e0.valueAt(e0.LastParameter)
        e1 = inEdges[1]
        if not PathGeom.pointsCoincide(last, e1.valueAt(e1.FirstParameter)) and not PathGeom.pointsCoincide(last, e1.valueAt(e1.LastParameter)):
            debugEdge('#  _orientEdges - flip first', e0)
            e0 = PathGeom.flipEdge(e0)

    edges = [e0]
    last = e0.valueAt(e0.LastParameter)
    for e in inEdges[1:]:
        edge = e if PathGeom.pointsCoincide(last, e.valueAt(e.FirstParameter)) else PathGeom.flipEdge(e)
        edges.append(edge)
        last = edge.valueAt(edge.LastParameter)
    return edges

def _isWireClockwise(w):
    '''_isWireClockwise(w) ... return True if wire is oriented clockwise.
    Assumes the edges of w are already properly oriented - for generic access use isWireClockwise(w).'''
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
    PathLog.track(area)
    return area < 0

def isWireClockwise(w):
    '''isWireClockwise(w) ... returns True if the wire winds clockwise. '''
    return _isWireClockwise(Part.Wire(_orientEdges(w.Edges)))


def orientWire(w, forward=True):
    '''orientWire(w, forward=True) ... orients given wire in a specific direction.
    If forward = True (the default) the wire is oriented clockwise, looking down the negative Z axis.
    If forward = False the wire is oriented counter clockwise.
    If forward = None the orientation is determined by the order in which the edges appear in the wire.'''
    wire = Part.Wire(_orientEdges(w.Edges))
    if forward is not None:
        if forward != _isWireClockwise(wire):
            PathLog.track('orientWire - needs flipping')
            return PathGeom.flipWire(wire)
        PathLog.track('orientWire - ok')
    return wire

def offsetWire(wire, base, offset, forward):
    '''offsetWire(wire, base, offset, forward) ... offsets the wire away from base and orients the wire accordingly.
    The function tries to avoid most of the pitfalls of Part.makeOffset2D which is possible because all offsetting
    happens in the XY plane.
    '''
    PathLog.track('offsetWire')

    if 1 == len(wire.Edges):
        edge = wire.Edges[0]
        curve = edge.Curve
        if Part.Circle == type(curve) and wire.isClosed():
            # it's a full circle and there are some problems with that, see
            # http://www.freecadweb.org/wiki/Part%20Offset2D
            # it's easy to construct them manually though
            z = -1 if forward else 1
            edge = Part.makeCircle(curve.Radius + offset, curve.Center, FreeCAD.Vector(0, 0, z))
            if base.isInside(edge.Vertexes[0].Point, offset/2, True):
                if offset > curve.Radius or PathGeom.isRoughly(offset, curve.Radius):
                    # offsetting a hole by its own radius (or more) makes the hole vanish
                    return None
                edge = Part.makeCircle(curve.Radius - offset, curve.Center, FreeCAD.Vector(0, 0, -z))
            w = Part.Wire([edge])
            return w
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
            if base.isInside(edge.valueAt((edge.FirstParameter + edge.LastParameter) / 2), offset / 2, True):
                edge.translate(-2 * o)

            # flip the edge if it's not on the right side of the original edge
            if forward is not None:
                v1 = edge.Vertexes[1].Point - p0
                left = PathGeom.Side.Left == PathGeom.Side.of(v0, v1)
                if left != forward:
                    edge = PathGeom.flipEdge(edge)
            return Part.Wire([edge])

        # if we get to this point the assumption is that makeOffset2D can deal with the edge
        pass

    owire = orientWire(wire.makeOffset2D(offset), True)
    debugWire('makeOffset2D_%d' % len(wire.Edges), owire)

    if wire.isClosed():
        if not base.isInside(owire.Edges[0].Vertexes[0].Point, offset/2, True):
            PathLog.track('closed - outside')
            return orientWire(owire, forward)
        PathLog.track('closed - inside')
        try:
            owire = wire.makeOffset2D(-offset)
        except:
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
    #  if the traversed edges include any oof the remainig from above, all those edges are remaining
    #  this is to also include edges which might partially be inside shape
    #  if they need to be discarded, split, that should happen in a post process
    # Depending on the Axis of the circle, and which side remains we know if the wire needs to be flipped

    # first, let's make sure all edges are oriented the proper way
    edges = _orientEdges(wire.Edges)

    # determine the start and end point
    start = edges[0].firstVertex().Point
    end = edges[-1].lastVertex().Point
    debugWire('wire', wire)
    debugWire('wedges', Part.Wire(edges))

    # find edges that are not inside the shape
    common = base.common(owire)
    insideEndpoints = [e.lastVertex().Point for e in common.Edges]
    insideEndpoints.append(common.Edges[0].firstVertex().Point)

    def isInside(edge):
        p0 = edge.firstVertex().Point
        p1 = edge.lastVertex().Point
        for p in insideEndpoints:
            if PathGeom.pointsCoincide(p, p0, 0.01) or PathGeom.pointsCoincide(p, p1, 0.01):
                return True
        return False

    outside = [e for e in owire.Edges if not isInside(e)]
    # discard all edges that are not part of the longest wire
    longestWire = None
    for w in [Part.Wire(el) for el in Part.sortEdges(outside)]:
        if not longestWire or longestWire.Length < w.Length:
            longestWire = w

    debugWire('outside', Part.Wire(outside))
    debugWire('longest', longestWire)

    def isCircleAt(edge, center):
        '''isCircleAt(edge, center) ... helper function returns True if edge is a circle at the given center.'''
        if Part.Circle == type(edge.Curve) or Part.ArcOfCircle == type(edge.Curve):
            return PathGeom.pointsCoincide(edge.Curve.Center, center)
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
    for e in (owire.Edges + owire.Edges):
        if isCircleAt(e, start):
            if PathGeom.pointsCoincide(e.Curve.Axis, FreeCAD.Vector(0, 0, 1)):
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
            if PathGeom.pointsCoincide(e.Curve.Axis, FreeCAD.Vector(0, 0, 1)):
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

    debugWire('left', Part.Wire(leftSideEdges))
    debugWire('right', Part.Wire(rightSideEdges))

    # figure out if all the left sided edges or the right sided edges are the ones
    # that are 'outside'. However, we return the full side.
    edges = leftSideEdges
    for e in longestWire.Edges:
        for e0 in rightSideEdges:
            if PathGeom.edgesMatch(e, e0):
                edges = rightSideEdges
                PathLog.debug("#use right side edges")
                if not forward:
                    PathLog.debug("#reverse")
                    edges.reverse()
                return orientWire(Part.Wire(edges), None)

    # at this point we have the correct edges and they are in the order for forward
    # traversal (climb milling). If that's not what we want just reverse the order,
    # orientWire takes care of orienting the edges appropriately.
    PathLog.debug("#use left side edges")
    if not forward:
        PathLog.debug("#reverse")
        edges.reverse()

    return orientWire(Part.Wire(edges), None)

