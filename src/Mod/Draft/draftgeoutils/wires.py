# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************
"""Provides various functions to work with wires."""
## @package wires
# \ingroup draftgeoutils
# \brief Provides various functions to work with wires.

import math
import lazy_loader.lazy_loader as lz

import FreeCAD as App
import DraftVecUtils
import WorkingPlane

from draftgeoutils.general import geomType, vec, precision
from draftgeoutils.geometry import get_normal
from draftgeoutils.edges import findMidpoint, isLine

# Delay import of module until first use because it is heavy
Part = lz.LazyLoader("Part", globals(), "Part")

## \addtogroup draftgeoutils
# @{


def findWires(edgeslist):
    """Find wires in a list of edges."""
    return [Part.Wire(e) for e in Part.sortEdges(edgeslist)]


def findWiresOld2(edgeslist):
    """Find connected wires in the given list of edges."""

    def touches(e1, e2):
        """Return True if two edges connect at the edges."""
        if len(e1.Vertexes) < 2:
            return False
        if len(e2.Vertexes) < 2:
            return False
        if DraftVecUtils.equals(e1.Vertexes[0].Point,
                                e2.Vertexes[0].Point):
            return True
        if DraftVecUtils.equals(e1.Vertexes[0].Point,
                                e2.Vertexes[-1].Point):
            return True
        if DraftVecUtils.equals(e1.Vertexes[-1].Point,
                                e2.Vertexes[0].Point):
            return True
        if DraftVecUtils.equals(e1.Vertexes[-1].Point,
                                e2.Vertexes[-1].Point):
            return True
        return False

    edges = edgeslist[:]
    wires = []
    lost = []
    while edges:
        e = edges[0]
        if not wires:
            # create first group
            edges.remove(e)
            wires.append([e])
        else:
            found = False
            for w in wires:
                if not found:
                    for we in w:
                        if touches(e, we):
                            edges.remove(e)
                            w.append(e)
                            found = True
                            break
            if not found:
                if e in lost:
                    # we already tried this edge, and still nothing
                    edges.remove(e)
                    wires.append([e])
                    lost = []
                else:
                    # put to the end of the list
                    edges.remove(e)
                    edges.append(e)
                    lost.append(e)
    nwires = []
    for w in wires:
        try:
            wi = Part.Wire(w)
        except Part.OCCError:
            print("couldn't join some edges")
        else:
            nwires.append(wi)
    return nwires


def findWiresOld(edges):
    """Return a list of lists containing edges that can be connected.

    Find connected edges in the list.
    """
    raise DeprecationWarning("This function shouldn't be called anymore. "
                             "Use findWires() instead")

    def verts(shape):
        return [shape.Vertexes[0].Point,
                shape.Vertexes[-1].Point]

    def group(shapes):
        shapesIn = shapes[:]
        shapesOut = [shapesIn.pop()]
        changed = False
        for s in shapesIn:
            if len(s.Vertexes) < 2:
                continue
            else:
                clean = True
                for v in verts(s):
                    for i in range(len(shapesOut)):
                        if clean and (v in verts(shapesOut[i])):
                            shapesOut[i] = Part.Wire(shapesOut[i].Edges
                                                     + s.Edges)
                            changed = True
                            clean = False
                if clean:
                    shapesOut.append(s)
        return changed, shapesOut

    working = True
    edgeSet = edges

    while working:
        result = group(edgeSet)
        working = result[0]
        edgeSet = result[1]

    return result[1]


def flattenWire(wire):
    """Force a wire to get completely flat along its normal."""
    n = get_normal(wire)
    # for backward compatibility with previous getNormal implementation
    if n == None:
        n = App.Vector(0, 0, 1)

    o = wire.Vertexes[0].Point
    plane = WorkingPlane.plane()
    plane.alignToPointAndAxis(o, n, 0)
    verts = [o]

    for v in wire.Vertexes[1:]:
        verts.append(plane.projectPoint(v.Point))

    if wire.isClosed():
        verts.append(o)
    w = Part.makePolygon(verts)

    return w


def superWire(edgeslist, closed=False):
    """Force a wire between edges that don't have coincident endpoints.

    Forces a wire between edges that don't necessarily
    have coincident endpoints. If closed=True, the wire will always be closed.
    """
    def median(v1, v2):
        vd = v2.sub(v1)
        vd.scale(0.5, 0.5, 0.5)
        return v1.add(vd)

    edges = Part.__sortEdges__(edgeslist)
    print(edges)
    newedges = []

    for i in range(len(edges)):
        curr = edges[i]
        if i == 0:
            if closed:
                prev = edges[-1]
            else:
                prev = None
        else:
            prev = edges[i - 1]

        if i == (len(edges) - 1):
            if closed:
                _next = edges[0]
            else:
                _next = None
        else:
            _next = edges[i+1]

        print(i, prev, curr, _next)

        if prev:
            if curr.Vertexes[0].Point == prev.Vertexes[-1].Point:
                p1 = curr.Vertexes[0].Point
            else:
                p1 = median(curr.Vertexes[0].Point, prev.Vertexes[-1].Point)
        else:
            p1 = curr.Vertexes[0].Point

        if _next:
            if curr.Vertexes[-1].Point == _next.Vertexes[0].Point:
                p2 = _next.Vertexes[0].Point
            else:
                p2 = median(curr.Vertexes[-1].Point, _next.Vertexes[0].Point)
        else:
            p2 = curr.Vertexes[-1].Point

        if geomType(curr) == "Line":
            print("line", p1, p2)
            newedges.append(Part.LineSegment(p1, p2).toShape())
        elif geomType(curr) == "Circle":
            p3 = findMidpoint(curr)
            print("arc", p1, p3, p2)
            newedges.append(Part.Arc(p1, p3, p2).toShape())
        else:
            print("Cannot superWire edges that are not lines or arcs")
            return None

    print(newedges)
    return Part.Wire(newedges)


def isReallyClosed(wire):
    """Check if a wire is really closed."""
    # TODO yet to find out why not use wire.isClosed() direct,
    # in isReallyClosed(wire)

    # Remark out below - Found not true if a vertex is used again
    # in a wire in sketch (e.g. wire with shape like 'd', 'b', 'g', ...)
    # if len(wire.Edges) == len(wire.Vertexes): return True

    # Found cases where Wire[-1] are not 'last' vertexes
    # e.g. Part.Wire( Part.__sortEdges__(<Rectangle Geometries>.toShape()))
    # aboveWire.isClosed() == True, but Wire[-1] are the 3rd vertex
    # for the rectangle - use Edges[i].Vertexes[0/1] instead
    length = len(wire.Edges)

    # Test if it is full circle / ellipse first
    if length == 1:
        if len(wire.Edges[0].Vertexes) == 1:
            return True  # This is a closed wire - full circle/ellipse
        else:
            # TODO Should be False if 1 edge but not single vertex, correct?
            # No need to test further below.
            return False

    # If more than 1 edge, further test below
    v1 = wire.Edges[0].Vertexes[0].Point   # v1 = wire.Vertexes[0].Point
    v2 = wire.Edges[length-1].Vertexes[1].Point  # v2 = wire.Vertexes[-1].Point
    if DraftVecUtils.equals(v1, v2):
        return True

    return False


def curvetowire(obj, steps):
    """Discretize the object and return a list of edges."""
    points = obj.copy().discretize(steps)
    p0 = points[0]
    edgelist = []
    for p in points[1:]:
        edge = Part.makeLine((p0.x, p0.y, p0.z), (p.x, p.y, p.z))
        edgelist.append(edge)
        p0 = p
    return edgelist


def curvetosegment(curve, seglen):
    """Discretize the curve and return a list of edges."""
    points = curve.discretize(seglen)
    p0 = points[0]
    edgelist = []
    for p in points[1:]:
        edge = Part.makeLine((p0.x, p0.y, p0.z), (p.x, p.y, p.z))
        edgelist.append(edge)
        p0 = p
    return edgelist


def rebaseWire(wire, vidx=0):
    """Return a copy of the wire with the first vertex indicated by the index.

    Return a new wire which is a copy of the current wire,
    but where the first vertex is the vertex indicated by the given
    index vidx, starting from 1.
    0 will return an exact copy of the wire.
    """
    if vidx < 1:
        return wire

    if vidx > len(wire.Vertexes):
        # print("Vertex index above maximum")
        return wire

    # This can be done in one step
    return Part.Wire(wire.Edges[vidx-1:] + wire.Edges[:vidx-1])


def removeInterVertices(wire):
    """Remove middle vertices from a straight wire and return a new wire.

    Remove unneeded vertices, those that are in the middle of a straight line,
    from a wire, return a new wire.
    """
    _pre = precision()
    edges = Part.__sortEdges__(wire.Edges)
    nverts = []

    def getvec(v1, v2):
        if not abs(round(v1.getAngle(v2), _pre) in [0, round(math.pi, _pre)]):
            nverts.append(edges[i].Vertexes[-1].Point)

    for i in range(len(edges) - 1):
        vA = vec(edges[i])
        vB = vec(edges[i + 1])
        getvec(vA, vB)

    vA = vec(edges[-1])
    vB = vec(edges[0])
    getvec(vA, vB)

    if nverts:
        if wire.isClosed():
            nverts.append(nverts[0])
        w = Part.makePolygon(nverts)
        return w
    else:
        return wire


def cleanProjection(shape, tessellate=True, seglength=0.05):
    """Return a valid compound of edges, by recreating them.

    This is because the projection algorithm somehow creates wrong shapes.
    They display fine, but on loading the file the shape is invalid.

    Now with tanderson's fix to `ProjectionAlgos`, that isn't the case,
    but this function can be used for tessellating ellipses and splines
    for DXF output-DF.
    """
    oldedges = shape.Edges
    newedges = []
    for e in oldedges:
        try:
            if geomType(e) == "Line":
                newedges.append(e.Curve.toShape())

            elif geomType(e) == "Circle":
                if len(e.Vertexes) > 1:
                    mp = findMidpoint(e)
                    a = Part.Arc(e.Vertexes[0].Point,
                                 mp,
                                 e.Vertexes[-1].Point).toShape()
                    newedges.append(a)
                else:
                    newedges.append(e.Curve.toShape())

            elif geomType(e) == "Ellipse":
                if tessellate:
                    newedges.append(Part.Wire(curvetowire(e, seglength)))
                else:
                    if len(e.Vertexes) > 1:
                        a = Part.Arc(e.Curve,
                                     e.FirstParameter,
                                     e.LastParameter).toShape()
                        newedges.append(a)
                    else:
                        newedges.append(e.Curve.toShape())

            elif (geomType(e) == "BSplineCurve"
                  or geomType(e) == "BezierCurve"):
                if tessellate:
                    newedges.append(Part.Wire(curvetowire(e, seglength)))
                else:
                    if isLine(e.Curve):
                        line = Part.LineSegment(e.Vertexes[0].Point,
                                                e.Vertexes[-1].Point).toShape()
                        newedges.append(line)
                    else:
                        newedges.append(e.Curve.toShape(e.FirstParameter,
                                                        e.LastParameter))
            else:
                newedges.append(e)
        except Part.OCCError:
            print("Debug: error cleaning edge ", e)

    return Part.makeCompound(newedges)


def tessellateProjection(shape, seglen):
    """Return projection with BSplines and Ellipses broken into line segments.

    Useful for exporting projected views to DXF files.
    """
    oldedges = shape.Edges
    newedges = []
    for e in oldedges:
        try:
            if geomType(e) == "Line":
                newedges.append(e.Curve.toShape())
            elif geomType(e) == "Circle":
                newedges.append(e.Curve.toShape())
            elif geomType(e) == "Ellipse":
                newedges.append(Part.Wire(curvetosegment(e, seglen)))
            elif geomType(e) == "BSplineCurve":
                newedges.append(Part.Wire(curvetosegment(e, seglen)))
            else:
                newedges.append(e)
        except Part.OCCError:
            print("Debug: error cleaning edge ", e)

    return Part.makeCompound(newedges)

## @}
