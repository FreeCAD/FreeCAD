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


def flattenWire(wire, origin=None, normal=None):
    """Force a wire to be flat on a plane defined by an origin and a normal.

    If origin or normal are None they are derived from the wire.
    """
    if normal is None:
        normal = get_normal(wire)
        # for backward compatibility with previous getNormal implementation
        if normal is None:
            normal = App.Vector(0, 0, 1)
    if origin is None:
        origin = wire.Vertexes[0].Point

    plane = WorkingPlane.plane()
    plane.alignToPointAndAxis(origin, normal, 0)
    points = [plane.projectPoint(vert.Point) for vert in wire.Vertexes]
    if wire.isClosed():
        points.append(points[0])
    new_wire = Part.makePolygon(points)

    return new_wire


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
    if isinstance(wire, (Part.Wire, Part.Edge)):
        return wire.isClosed()
    return isinstance(wire, Part.Face)


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
    """Return a compound of edges, optionally tessellate ellipses, splines
    and bezcurves.

    The function was formerly used to workaround bugs in the projection
    algorithm. These bugs have since been fixed. Now the function is only
    used when tessellation of ellipses, splines and bezcurves is required
    (DXF output and Draft_Shape2DView).
    """
    oldedges = shape.Edges
    newedges = []
    for e in oldedges:
        typ = geomType(e)
        try:
            if typ in ["Line", "Circle"]:
                newedges.append(e)
            elif typ == "Ellipse":
                if tessellate:
                    newedges.append(Part.Wire(curvetowire(e, seglength)))
                else:
                    newedges.append(e)
            elif typ in ["BSplineCurve", "BezierCurve"]:
                if isLine(e.Curve):
                    line = Part.LineSegment(e.Vertexes[0].Point,
                                            e.Vertexes[-1].Point)
                    newedges.append(line)
                elif tessellate:
                    newedges.append(Part.Wire(curvetowire(e, seglength)))
                else:
                    newedges.append(e)
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

def get_placement_perpendicular_to_wire(wire):
    """Return the placement whose base is the wire's first vertex and it's z axis aligned to the wire's tangent."""
    pl = App.Placement()
    if wire.Length > 0.0:
        pl.Base = wire.OrderedVertexes[0].Point
        first_edge = wire.OrderedEdges[0]
        if first_edge.Orientation == "Forward":
            zaxis = -first_edge.tangentAt(first_edge.FirstParameter)
        else:
            zaxis = first_edge.tangentAt(first_edge.LastParameter)
        pl.Rotation = App.Rotation(App.Vector(1, 0, 0), App.Vector(0, 0, 1), zaxis, "ZYX")
    else:
        App.Console.PrintError("debug: get_placement_perpendicular_to_wire called with a zero-length wire.\n")
    return pl


def get_extended_wire(wire, offset_start, offset_end):
    """Return a wire trimmed (negative offset) or extended (positive offset) at its first vertex, last vertex or both ends.

    get_extended_wire(wire, -100.0, 0.0) -> returns a copy of the wire with its first 100 mm removed
    get_extended_wire(wire, 0.0, 100.0) -> returns a copy of the wire extended by 100 mm after it's last vertex
    """
    if min(offset_start, offset_end, offset_start + offset_end) <= -wire.Length:
        App.Console.PrintError("debug: get_extended_wire error, wire's length insufficient for trimming.\n")
        return wire
    if offset_start < 0: # Trim the wire from the first vertex
        offset_start = -offset_start
        out_edges = []
        for edge in wire.OrderedEdges:
            if offset_start >= edge.Length: # Remove entire edge
                offset_start -= edge.Length
            elif round(offset_start, precision()) > 0: # Split edge, to remove the required length
                if edge.Orientation == "Forward":
                    new_edge = edge.split(edge.getParameterByLength(offset_start)).OrderedEdges[1]
                else:
                    new_edge = edge.split(edge.getParameterByLength(edge.Length - offset_start)).OrderedEdges[0]
                new_edge.Placement = edge.Placement # Strangely, edge.split discards the placement and orientation
                new_edge.Orientation = edge.Orientation
                out_edges.append(new_edge)
                offset_start = 0
            else: # Keep the remaining entire edges
                out_edges.append(edge)
        wire = Part.Wire(out_edges)
    elif offset_start > 0: # Extend the first edge along its normal
        first_edge = wire.OrderedEdges[0]
        if first_edge.Orientation == "Forward":
            start, end = first_edge.FirstParameter, first_edge.LastParameter
            vec = first_edge.tangentAt(start).multiply(offset_start)
        else:
            start, end = first_edge.LastParameter, first_edge.FirstParameter
            vec = -first_edge.tangentAt(start).multiply(offset_start)
        if geomType(first_edge) == "Line": # Replace first edge with the extended new edge
            new_edge = Part.LineSegment(first_edge.valueAt(start).sub(vec), first_edge.valueAt(end)).toShape()
            wire = Part.Wire([new_edge] + wire.OrderedEdges[1:])
        else: # Add a straight edge before the first vertex
            new_edge = Part.LineSegment(first_edge.valueAt(start).sub(vec), first_edge.valueAt(start)).toShape()
            wire = Part.Wire([new_edge] + wire.OrderedEdges)
    if offset_end < 0: # Trim the wire from the last vertex
        offset_end = -offset_end
        out_edges = []
        for edge in reversed(wire.OrderedEdges):
            if offset_end >= edge.Length: # Remove entire edge
                offset_end -= edge.Length
            elif round(offset_end, precision()) > 0: # Split edge, to remove the required length
                if edge.Orientation == "Forward":
                    new_edge = edge.split(edge.getParameterByLength(edge.Length - offset_end)).OrderedEdges[0]
                else:
                    new_edge = edge.split(edge.getParameterByLength(offset_end)).OrderedEdges[1]
                new_edge.Placement = edge.Placement # Strangely, edge.split discards the placement and orientation
                new_edge.Orientation = edge.Orientation
                out_edges.insert(0, new_edge)
                offset_end = 0
            else: # Keep the remaining entire edges
                out_edges.insert(0, edge)
        wire = Part.Wire(out_edges)
    elif offset_end > 0: # Extend the last edge along its normal
        last_edge = wire.OrderedEdges[-1]
        if last_edge.Orientation == "Forward":
            start, end = last_edge.FirstParameter, last_edge.LastParameter
            vec = last_edge.tangentAt(end).multiply(offset_end)
        else:
            start, end = last_edge.LastParameter, last_edge.FirstParameter
            vec = -last_edge.tangentAt(end).multiply(offset_end)
        if geomType(last_edge) == "Line": # Replace last edge with the extended new edge
            new_edge = Part.LineSegment(last_edge.valueAt(start), last_edge.valueAt(end).add(vec)).toShape()
            wire = Part.Wire(wire.OrderedEdges[:-1] + [new_edge])
        else: # Add a straight edge after the last vertex
            new_edge = Part.LineSegment(last_edge.valueAt(end), last_edge.valueAt(end).add(vec)).toShape()
            wire = Part.Wire(wire.OrderedEdges + [new_edge])
    return wire

## @}
