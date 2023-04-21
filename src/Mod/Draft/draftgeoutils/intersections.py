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
"""Provides various functions to calculate intersections of shapes."""
## @package intersections
# \ingroup draftgeoutils
# \brief Provides various functions to calculate intersections of shapes.

import lazy_loader.lazy_loader as lz

import FreeCAD as App
import DraftVecUtils

from draftgeoutils.general import precision, vec, geomType, isPtOnEdge
from draftgeoutils.edges import findMidpoint

# Delay import of module until first use because it is heavy
Part = lz.LazyLoader("Part", globals(), "Part")

## \addtogroup draftgeoutils
# @{


def findIntersection(edge1, edge2,
                     infinite1=False, infinite2=False,
                     ex1=False, ex2=False,
                     dts=True, findAll=False):
    """Return a list containing the intersection points of 2 edges.

    You can also feed 4 points instead of `edge1` and `edge2`.
    If `dts` is used, `Shape.distToShape()` is used, which can be buggy.
    """

    def getLineIntersections(pt1, pt2, pt3, pt4, infinite1, infinite2):
        if pt1:
            # first check if we don't already have coincident endpoints
            if pt1 in [pt3, pt4]:
                return [pt1]
            elif (pt2 in [pt3, pt4]):
                return [pt2]
        norm1 = pt2.sub(pt1).cross(pt3.sub(pt1))
        norm2 = pt2.sub(pt4).cross(pt3.sub(pt4))

        if not DraftVecUtils.isNull(norm1):
            try:
                norm1.normalize()
            except Part.OCCError:
                return []

        if not DraftVecUtils.isNull(norm2):
            try:
                norm2.normalize()
            except Part.OCCError:
                return []

        if DraftVecUtils.isNull(norm1.cross(norm2)):
            vec1 = pt2.sub(pt1)
            vec2 = pt4.sub(pt3)
            if DraftVecUtils.isNull(vec1) or DraftVecUtils.isNull(vec2):
                return []  # One of the lines has zero-length
            try:
                vec1.normalize()
                vec2.normalize()
            except Part.OCCError:
                return []
            norm3 = vec1.cross(vec2)
            denom = norm3.x + norm3.y + norm3.z
            if not DraftVecUtils.isNull(norm3) and denom != 0:
                k = ((pt3.z - pt1.z) * (vec2.x - vec2.y)
                     + (pt3.y - pt1.y) * (vec2.z - vec2.x)
                     + (pt3.x - pt1.x) * (vec2.y - vec2.z))/denom
                vec1.scale(k, k, k)
                intp = pt1.add(vec1)

                if infinite1 is False and not isPtOnEdge(intp, edge1):
                    return []

                if infinite2 is False and not isPtOnEdge(intp, edge2):
                    return []

                return [intp]
            else:
                return []  # Lines have same direction
        else:
            return []  # Lines aren't on same plane

    # First, check bound boxes
    if (isinstance(edge1, Part.Edge) and isinstance(edge2, Part.Edge)
            and (not infinite1) and (not infinite2)):
        bb1 = edge1.BoundBox
        bb1.enlarge(pow(10, -precision()))  # enlarge one box to account for rounding errors
        if not bb1.intersect(edge2.BoundBox):
            return []  # bound boxes don't intersect

    # First, try to use distToShape if possible
    if (dts and isinstance(edge1, Part.Edge) and isinstance(edge2, Part.Edge)
            and (not infinite1) and (not infinite2)):
        dist, pts, geom = edge1.distToShape(edge2)
        sol = []
        if round(dist, precision()) == 0:
            for p in pts:
                if p[0] not in sol:
                    sol.append(p[0])
        return sol

    pt1 = None

    if isinstance(edge1, App.Vector) and isinstance(edge2, App.Vector):
        # we got points directly
        pt1 = edge1
        pt2 = edge2
        pt3 = infinite1
        pt4 = infinite2
        infinite1 = ex1
        infinite2 = ex2
        return getLineIntersections(pt1, pt2, pt3, pt4, infinite1, infinite2)

    elif (geomType(edge1) == "Line") and (geomType(edge2) == "Line"):
        # we have 2 straight lines
        pt1, pt2, pt3, pt4 = [edge1.Vertexes[0].Point,
                              edge1.Vertexes[1].Point,
                              edge2.Vertexes[0].Point,
                              edge2.Vertexes[1].Point]
        return getLineIntersections(pt1, pt2, pt3, pt4, infinite1, infinite2)

    elif ((geomType(edge1) == "Circle") and (geomType(edge2) == "Line")
          or (geomType(edge1) == "Line") and (geomType(edge2) == "Circle")):

        # deals with an arc or circle and a line
        edges = [edge1, edge2]
        for edge in edges:
            if geomType(edge) == "Line":
                line = edge
            else:
                arc = edge

        dirVec = vec(line)
        dirVec.normalize()
        pt1 = line.Vertexes[0].Point
        pt2 = line.Vertexes[1].Point
        pt3 = arc.Vertexes[0].Point
        pt4 = arc.Vertexes[-1].Point
        center = arc.Curve.Center

        int = []
        # first check for coincident endpoints
        if DraftVecUtils.equals(pt1, pt3) or DraftVecUtils.equals(pt1, pt4):
            if findAll:
                int.append(pt1)
            else:
                return [pt1]
        elif pt2 in [pt3, pt4]:
            if findAll:
                int.append(pt2)
            else:
                return [pt2]

        if DraftVecUtils.isNull(pt1.sub(center).cross(pt2.sub(center)).cross(arc.Curve.Axis)):
            # Line and Arc are on same plane

            dOnLine = center.sub(pt1).dot(dirVec)
            onLine = App.Vector(dirVec)
            onLine.scale(dOnLine, dOnLine, dOnLine)
            toLine = pt1.sub(center).add(onLine)

            if toLine.Length < arc.Curve.Radius:
                dOnLine = (arc.Curve.Radius**2 - toLine.Length**2)**(0.5)
                onLine = App.Vector(dirVec)
                onLine.scale(dOnLine, dOnLine, dOnLine)
                int += [center.add(toLine).add(onLine)]
                onLine = App.Vector(dirVec)
                onLine.scale(-dOnLine, -dOnLine, -dOnLine)
                int += [center.add(toLine).add(onLine)]
            elif round(toLine.Length - arc.Curve.Radius, precision()) == 0:
                int = [center.add(toLine)]
            else:
                return []

        else:
            # Line isn't on Arc's plane
            if dirVec.dot(arc.Curve.Axis) != 0:
                toPlane = App.Vector(arc.Curve.Axis)
                toPlane.normalize()
                d = pt1.dot(toPlane)
                if not d:
                    return []
                dToPlane = center.sub(pt1).dot(toPlane)
                toPlane = App.Vector(pt1)
                toPlane.scale(dToPlane/d, dToPlane/d, dToPlane/d)
                ptOnPlane = toPlane.add(pt1)
                if round(ptOnPlane.sub(center).Length - arc.Curve.Radius,
                         precision()) == 0:
                    int = [ptOnPlane]
                else:
                    return []
            else:
                return []

        if infinite1 is False:
            for i in range(len(int) - 1, -1, -1):
                if not isPtOnEdge(int[i], edge1):
                    del int[i]
        if infinite2 is False:
            for i in range(len(int) - 1, -1, -1):
                if not isPtOnEdge(int[i], edge2):
                    del int[i]
        return int

    elif (geomType(edge1) == "Circle") and (geomType(edge2) == "Circle"):
        # deals with 2 arcs or circles
        cent1, cent2 = edge1.Curve.Center, edge2.Curve.Center
        rad1, rad2 = edge1.Curve.Radius, edge2.Curve.Radius
        axis1, axis2 = edge1.Curve.Axis, edge2.Curve.Axis
        c2c = cent2.sub(cent1)

        if cent1.sub(cent2).Length == 0:
            # circles are concentric
            return []

        if DraftVecUtils.isNull(axis1.cross(axis2)):
            if round(c2c.dot(axis1), precision()) == 0:
                # circles are on same plane
                dc2c = c2c.Length
                if not DraftVecUtils.isNull(c2c):
                    c2c.normalize()
                if (round(rad1 + rad2 - dc2c, precision()) < 0
                        or round(rad1 - dc2c - rad2, precision()) > 0
                        or round(rad2 - dc2c - rad1, precision()) > 0):
                    return []
                else:
                    norm = c2c.cross(axis1)
                    if not DraftVecUtils.isNull(norm):
                        norm.normalize()
                    if DraftVecUtils.isNull(norm):
                        x = 0
                    else:
                        x = (dc2c**2 + rad1**2 - rad2**2) / (2*dc2c)
                    y = abs(rad1**2 - x**2)**(0.5)
                    c2c.scale(x, x, x)
                    if round(y, precision()) != 0:
                        norm.scale(y, y, y)
                        int = [cent1.add(c2c).add(norm)]
                        int += [cent1.add(c2c).sub(norm)]
                    else:
                        int = [cent1.add(c2c)]
            else:
                return []  # circles are on parallel planes
        else:
            # circles aren't on same plane
            axis1.normalize()
            axis2.normalize()
            U = axis1.cross(axis2)
            V = axis1.cross(U)
            dToPlane = c2c.dot(axis2)
            d = V.add(cent1).dot(axis2)
            V.scale(dToPlane/d, dToPlane/d, dToPlane/d)
            PtOn2Planes = V.add(cent1)
            planeIntersectionVector = U.add(PtOn2Planes)
            intTemp = findIntersection(planeIntersectionVector,
                                       edge1, True, True)
            int = []
            for pt in intTemp:
                if round(pt.sub(cent2).Length-rad2, precision()) == 0:
                    int += [pt]

        if infinite1 is False:
            for i in range(len(int) - 1, -1, -1):
                if not isPtOnEdge(int[i], edge1):
                    del int[i]
        if infinite2 is False:
            for i in range(len(int) - 1, -1, -1):
                if not isPtOnEdge(int[i], edge2):
                    del int[i]

        return int
    else:
        print("DraftGeomUtils: Unsupported curve type: "
              "(" + str(edge1.Curve) + ", " + str(edge2.Curve) + ")")
        return []


def wiresIntersect(wire1, wire2):
    """Return True if some of the edges of the wires are intersecting.

    Otherwise return `False`.
    """
    for e1 in wire1.Edges:
        for e2 in wire2.Edges:
            if findIntersection(e1, e2, dts=False):
                return True
    return False


def connect(edges, closed=False):
    """Connect the edges in the given list by their intersections."""

    inters_list = [] # List of intersections (with the previous edge).
    for i, curr in enumerate(edges):
        if i > 0:
            prev = edges[i - 1]
        elif closed:
            prev = edges[-1]
        else:
            inters_list.append(None)
            continue

        curr_inters_list = (findIntersection(prev, curr, True, True))
        if len(curr_inters_list) == 0:
            inters_list.append(None)
        elif len(curr_inters_list) == 1:
            inters_list.append(curr_inters_list[0])
        else:
            inters = curr_inters_list[DraftVecUtils.closest(curr.Vertexes[0].Point,
                                                            curr_inters_list)]
            inters_list.append(inters)

    new_edges = []
    for i, curr in enumerate(edges):
        curr_sta = inters_list[i]
        if i < (len(edges) - 1):
            curr_end = inters_list[i + 1]
        elif closed:
            curr_end = inters_list[0]
        else:
            curr_end = None

        if curr_sta is None:
            curr_sta = curr.Vertexes[0].Point
            if i > 0:
                prev = edges[i - 1]
            elif closed:
                prev = edges[-1]
            else:
                prev = None
            if prev is not None:
                prev_end = prev.Vertexes[-1].Point
                new_edges.append(Part.LineSegment(prev_end, curr_sta).toShape())

        if curr_end is None:
            curr_end = curr.Vertexes[-1].Point

        if curr_sta != curr_end:
            if geomType(curr) == "Line":
                new_edges.append(Part.LineSegment(curr_sta, curr_end).toShape())
            elif geomType(curr) == "Circle":
                new_edges.append(Part.Arc(curr_sta, findMidpoint(curr), curr_end).toShape())

    try:
        return Part.Wire(new_edges)
    except Part.OCCError:
        print("DraftGeomUtils.connect: unable to connect edges")
        for edge in new_edges:
            print(edge.Curve, " ",
                  edge.Vertexes[0].Point, " ",
                  edge.Vertexes[-1].Point)
        return None


def angleBisection(edge1, edge2):
    """Return an edge that bisects the angle between the 2 straight edges."""
    if geomType(edge1) != "Line" or geomType(edge2) != "Line":
        return None

    p1 = edge1.Vertexes[0].Point
    p2 = edge1.Vertexes[-1].Point
    p3 = edge2.Vertexes[0].Point
    p4 = edge2.Vertexes[-1].Point
    intersect = findIntersection(edge1, edge2, True, True)

    if intersect:
        line1Dir = p2.sub(p1)
        angleDiff = DraftVecUtils.angle(line1Dir, p4.sub(p3))
        ang = angleDiff * 0.5
        origin = intersect[0]
        line1Dir.normalize()
        direction = DraftVecUtils.rotate(line1Dir, ang)
    else:
        diff = p3.sub(p1)
        origin = p1.add(diff.multiply(0.5))
        direction = p2.sub(p1)
        direction.normalize()

    return Part.LineSegment(origin, origin.add(direction)).toShape()

## @}
