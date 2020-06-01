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
"""Provides various functions for circle operations."""
## @package circles
# \ingroup DRAFTGEOUTILS
# \brief Provides various functions for circle operations.

import math
import lazy_loader.lazy_loader as lz

import FreeCAD as App
import DraftVecUtils

from draftgeoutils.general import geomType, edg, vec, NORM
from draftgeoutils.geometry import mirror
from draftgeoutils.edges import findMidpoint
from draftgeoutils.intersections import findIntersection, angleBisection


# Delay import of module until first use because it is heavy
Part = lz.LazyLoader("Part", globals(), "Part")


def findClosestCircle(point, circles):
    """Return the circle with closest center to a given point."""
    dist = 1000000
    closest = None
    for c in circles:
        if c.Center.sub(point).Length < dist:
            dist = c.Center.sub(point).Length
            closest = c
    return closest


def getCircleFromSpline(edge):
    """Return a circle-based edge from a bspline-based edge.

    Return None if the edge is not a BSplineCurve.
    """
    if geomType(edge) != "BSplineCurve" or len(edge.Vertexes) != 1:
        return None

    # get 2 points
    p1 = edge.Curve.value(0)
    p2 = edge.Curve.value(math.pi/2)
    # get 2 tangents
    t1 = edge.Curve.tangent(0)[0]
    t2 = edge.Curve.tangent(math.pi/2)[0]
    # get normal
    n = p1.cross(p2)
    if DraftVecUtils.isNull(n):
        return None

    # get rays
    r1 = DraftVecUtils.rotate(t1, math.pi/2, n)
    r2 = DraftVecUtils.rotate(t2, math.pi/2, n)
    # get center (intersection of rays)
    i = findIntersection(p1, p1.add(r1), p2, p2.add(r2), True, True)
    if not i:
        return None

    c = i[0]
    r = (p1.sub(c)).Length
    circle = Part.makeCircle(r, c, n)
    # print(circle.Curve)
    return circle


def circlefrom1Line2Points(edge, p1, p2):
    """Return a list of circles created from an edge and two points.

    It calculates up to 2 possible centers.
    """
    p1_p2 = edg(p1, p2)
    s = findIntersection(edge, p1_p2, True, True)
    if not s:
        return None

    s = s[0]
    v1 = p1.sub(s)
    v2 = p2.sub(s)
    projectedDist = math.sqrt(abs(v1.dot(v2)))
    edgeDir = vec(edge)
    edgeDir.normalize()

    projectedCen1 = App.Vector.add(s, App.Vector(edgeDir).multiply(projectedDist))
    projectedCen2 = App.Vector.add(s, App.Vector(edgeDir).multiply(-projectedDist))
    perpEdgeDir = edgeDir.cross(App.Vector(0, 0, 1))
    perpCen1 = App.Vector.add(projectedCen1, perpEdgeDir)
    perpCen2 = App.Vector.add(projectedCen2, perpEdgeDir)

    mid = findMidpoint(p1_p2)
    x = DraftVecUtils.crossproduct(vec(p1_p2))
    x.normalize()
    perp_mid = App.Vector.add(mid, x)
    cen1 = findIntersection(edg(projectedCen1, perpCen1),
                            edg(mid, perp_mid), True, True)
    cen2 = findIntersection(edg(projectedCen2, perpCen2),
                            edg(mid, perp_mid), True, True)

    circles = []
    if cen1:
        radius = DraftVecUtils.dist(projectedCen1, cen1[0])
        circles.append(Part.Circle(cen1[0], NORM, radius))

    if cen2:
        radius = DraftVecUtils.dist(projectedCen2, cen2[0])
        circles.append(Part.Circle(cen2[0], NORM, radius))

    if circles:
        return circles
    else:
        return None


def circlefrom2Lines1Point(edge1, edge2, point):
    """Return a list of circles from two edges and one point."""
    bis = angleBisection(edge1, edge2)
    if not bis:
        return None

    mirrPoint = mirror(point, bis)
    return circlefrom1Line2Points(edge1, point, mirrPoint)


def circleFrom2LinesRadius(edge1, edge2, radius):
    """Retun a list of circles from two edges and one radius.

    It calculates 4 centers.
    """
    intsec = findIntersection(edge1, edge2, True, True)
    if not intsec:
        return None

    intsec = intsec[0]
    bis12 = angleBisection(edge1, edge2)
    bis21 = Part.LineSegment(bis12.Vertexes[0].Point,
                             DraftVecUtils.rotate(vec(bis12), math.pi/2.0))
    ang12 = abs(DraftVecUtils.angle(vec(edge1), vec(edge2)))
    ang21 = math.pi - ang12
    dist12 = radius / math.sin(ang12 * 0.5)
    dist21 = radius / math.sin(ang21 * 0.5)

    circles = []
    cen = App.Vector.add(intsec, vec(bis12).multiply(dist12))
    circles.append(Part.Circle(cen, NORM, radius))

    cen = App.Vector.add(intsec, vec(bis12).multiply(-dist12))
    circles.append(Part.Circle(cen, NORM, radius))

    cen = App.Vector.add(intsec, vec(bis21).multiply(dist21))
    circles.append(Part.Circle(cen, NORM, radius))

    cen = App.Vector.add(intsec, vec(bis21).multiply(-dist21))
    circles.append(Part.Circle(cen, NORM, radius))

    return circles
