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
"""Provides various functions to work with circles."""
## @package circles
# \ingroup draftgeoutils
# \brief Provides various functions to work with circles.

import math
import lazy_loader.lazy_loader as lz

import FreeCAD as App
import DraftVecUtils

from draftgeoutils.general import geomType, edg, vec, NORM, v1
from draftgeoutils.geometry import mirror, findDistance
from draftgeoutils.edges import findMidpoint
from draftgeoutils.intersections import findIntersection, angleBisection

# Delay import of module until first use because it is heavy
Part = lz.LazyLoader("Part", globals(), "Part")

## \addtogroup draftgeoutils
# @{


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
    """Return a list of circles from two edges and one radius.

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


def circleFrom3LineTangents(edge1, edge2, edge3):
    """Return a list of circles from three edges.

    It calculates up to 6 possible centers.
    """
    def rot(ed):
        geo = Part.LineSegment(v1(ed),
                               v1(ed).add(DraftVecUtils.rotate(vec(ed),
                                                               math.pi/2)))
        return geo.toShape()

    bis12 = angleBisection(edge1, edge2)
    bis23 = angleBisection(edge2, edge3)
    bis31 = angleBisection(edge3, edge1)
    intersections = []

    intsec = findIntersection(bis12, bis23, True, True)
    if intsec:
        radius = findDistance(intsec[0], edge1).Length
        intersections.append(Part.Circle(intsec[0], NORM, radius))

    intsec = findIntersection(bis23, bis31, True, True)
    if intsec:
        radius = findDistance(intsec[0], edge1).Length
        intersections.append(Part.Circle(intsec[0], NORM, radius))

    intsec = findIntersection(bis31, bis12, True, True)
    if intsec:
        radius = findDistance(intsec[0], edge1).Length
        intersections.append(Part.Circle(intsec[0], NORM, radius))

    intsec = findIntersection(rot(bis12), rot(bis23), True, True)
    if intsec:
        radius = findDistance(intsec[0], edge1).Length
        intersections.append(Part.Circle(intsec[0], NORM, radius))

    intsec = findIntersection(rot(bis23), rot(bis31), True, True)
    if intsec:
        radius = findDistance(intsec[0], edge1).Length
        intersections.append(Part.Circle(intsec[0], NORM, radius))

    intsec = findIntersection(rot(bis31), rot(bis12), True, True)
    if intsec:
        radius = findDistance(intsec[0], edge1).Length
        intersections.append(Part.Circle(intsec[0], NORM, radius))

    circles = []
    for intsec in intersections:
        exists = False
        for cir in circles:
            if DraftVecUtils.equals(cir.Center, intsec.Center):
                exists = True
                break
        if not exists:
            circles.append(intsec)

    if circles:
        return circles
    else:
        return None


def circleFromPointLineRadius(point, edge, radius):
    """Return a list of circles from one point, one edge, and one radius.

    It calculates up to 2 possible centers.
    """
    dist = findDistance(point, edge, False)
    center1 = None
    center2 = None

    if dist.Length == 0:
        segment = vec(edge)
        perpVec = DraftVecUtils.crossproduct(segment)
        perpVec.normalize()
        normPoint_c1 = App.Vector(perpVec).multiply(radius)
        normPoint_c2 = App.Vector(perpVec).multiply(-radius)
        center1 = point.add(normPoint_c1)
        center2 = point.add(normPoint_c2)
    elif dist.Length > 2 * radius:
        return None
    elif dist.Length == 2 * radius:
        normPoint = point.add(findDistance(point, edge, False))
        dummy = (normPoint.sub(point)).multiply(0.5)
        cen = point.add(dummy)
        circ = Part.Circle(cen, NORM, radius)
        if circ:
            return [circ]
        else:
            return None
    else:
        normPoint = point.add(findDistance(point, edge, False))
        normDist = DraftVecUtils.dist(normPoint, point)
        dist = math.sqrt(radius**2 - (radius - normDist)**2)
        centerNormVec = DraftVecUtils.scaleTo(point.sub(normPoint), radius)
        edgeDir = edge.Vertexes[0].Point.sub(normPoint)
        edgeDir.normalize()
        center1 = centerNormVec.add(normPoint.add(App.Vector(edgeDir).multiply(dist)))
        center2 = centerNormVec.add(normPoint.add(App.Vector(edgeDir).multiply(-dist)))

    circles = []
    if center1:
        circ = Part.Circle(center1, NORM, radius)
        if circ:
            circles.append(circ)

    if center2:
        circ = Part.Circle(center2, NORM, radius)
        if circ:
            circles.append(circ)

    if circles:
        return circles
    else:
        return None


def circleFrom2PointsRadius(p1, p2, radius):
    """Return a list of circles from two points, and one radius.

    The two points must not be equal.

    It calculates up to 2 possible centers.
    """
    if DraftVecUtils.equals(p1, p2):
        return None

    p1_p2 = Part.LineSegment(p1, p2).toShape()
    dist_p1p2 = DraftVecUtils.dist(p1, p1)
    mid = findMidpoint(p1_p2)

    if dist_p1p2 == 2*radius:
        circle = Part.Circle(mid, NORM, radius)
        if circle:
            return [circle]
        else:
            return None

    _dir = vec(p1_p2)
    _dir.normalize()
    perpDir = _dir.cross(App.Vector(0, 0, 1))
    perpDir.normalize()
    dist = math.sqrt(radius**2 - (dist_p1p2 / 2.0)**2)
    cen1 = App.Vector.add(mid, App.Vector(perpDir).multiply(dist))
    cen2 = App.Vector.add(mid, App.Vector(perpDir).multiply(-dist))

    circles = []
    if cen1:
        circles.append(Part.Circle(cen1, NORM, radius))
    if cen2:
        circles.append(Part.Circle(cen2, NORM, radius))

    if circles:
        return circles
    else:
        return None


def findHomotheticCenterOfCircles(circle1, circle2):
    """Calculate the homothetic centers from two circles.

    Return None if the objects are not circles, or if they are concentric.

    http://en.wikipedia.org/wiki/Homothetic_center
    http://mathworld.wolfram.com/HomotheticCenter.html
    """
    if (geomType(circle1) == "Circle" and geomType(circle2) == "Circle"):
        print("debug: findHomotheticCenterOfCircles bad parameters!")
        return None

    if DraftVecUtils.equals(circle1.Curve.Center,
                            circle2.Curve.Center):
        return None

    cen1_cen2 = Part.LineSegment(circle1.Curve.Center,
                                 circle2.Curve.Center).toShape()
    cenDir = vec(cen1_cen2)
    cenDir.normalize()

    # Get the perpedicular vector.
    perpCenDir = cenDir.cross(App.Vector(0, 0, 1))
    perpCenDir.normalize()

    # Get point on first circle
    p1 = App.Vector.add(circle1.Curve.Center,
                        App.Vector(perpCenDir).multiply(circle1.Curve.Radius))

    centers = []
    # Calculate inner homothetic center
    # Get point on second circle
    p2_inner = App.Vector.add(circle1.Curve.Center,
                              App.Vector(perpCenDir).multiply(-circle1.Curve.Radius))
    hCenterInner = DraftVecUtils.intersect(circle1.Curve.Center,
                                           circle2.Curve.Center,
                                           p1, p2_inner,
                                           True, True)
    if hCenterInner:
        centers.append(hCenterInner)

    # Calculate outer homothetic center; it only exists if the circles
    # have different radii
    if circle1.Curve.Radius != circle2.Curve.Radius:
        # Get point on second circle
        p2_outer = App.Vector.add(circle1.Curve.Center,
                                  App.Vector(perpCenDir).multiply(circle1.Curve.Radius))
        hCenterOuter = DraftVecUtils.intersect(circle1.Curve.Center,
                                               circle2.Curve.Center,
                                               p1, p2_outer,
                                               True, True)
        if hCenterOuter:
            centers.append(hCenterOuter)

    if centers:
        return centers
    else:
        return None


def findRadicalAxis(circle1, circle2):
    """Calculate the radical axis of two circles.

    On the radical axis (also called power line) of two circles any
    tangents drawn from a point on the axis to both circles have
    the same length.

    http://en.wikipedia.org/wiki/Radical_axis
    http://mathworld.wolfram.com/RadicalLine.html

    See Also
    --------
    findRadicalCenter
    """
    if (geomType(circle1) == "Circle") and (geomType(circle2) == "Circle"):
        print("debug: findRadicalAxis bad parameters! Must be circles.")
        return None

    if DraftVecUtils.equals(circle1.Curve.Center, circle2.Curve.Center):
        return None

    r1 = circle1.Curve.Radius
    r2 = circle1.Curve.Radius
    cen1 = circle1.Curve.Center
    # dist .. the distance from cen1 to cen2.
    dist = DraftVecUtils.dist(cen1, circle2.Curve.Center)
    cenDir = cen1.sub(circle2.Curve.Center)
    cenDir.normalize()

    # Get the perpedicular vector.
    perpCenDir = cenDir.cross(App.Vector(0, 0, 1))
    perpCenDir.normalize()

    # J ... The radical center.
    # K ... The point where the cadical axis crosses the line of cen1->cen2.
    # k1 ... Distance from cen1 to K.
    # k2 ... Distance from cen2 to K.
    # dist = k1 + k2

    k1 = (dist + (r1**2 - r2**2) / dist) / 2.0
    # k2 = dist - k1

    K = App.Vector.add(cen1, cenDir.multiply(k1))

    # K_ .. A point somewhere between K and J; actually with a distance
    # of 1 unit from K.
    K_ = App.Vector.add(K, perpCenDir)

    # Original code didn't specify the value of origin nor dir,
    # so this is a guess:
    # radicalAxis = Part.LineSegment(K, Vector.add(origin, dir))

    origin = App.Vector(0, 0, 0)
    radicalAxis = Part.LineSegment(K, App.Vector.add(origin, perpCenDir))

    if radicalAxis:
        return radicalAxis
    else:
        return None


def findRadicalCenter(circle1, circle2, circle3):
    """Calculate the radical center of three circles.

    It is also called the power center.
    It is the intersection point of the three radical axes of the pairs
    of circles.

    http://en.wikipedia.org/wiki/Power_center_(geometry)
    http://mathworld.wolfram.com/RadicalCenter.html

    See Also
    --------
    findRadicalAxis
    """
    if (geomType(circle1) != "Circle"
            or geomType(circle2) != "Circle"
            or geomType(circle3) != "Circle"):
        print("debug: findRadicalCenter bad parameters! Must be circles.")
        return None

    radicalAxis12 = findRadicalAxis(circle1, circle2)
    radicalAxis23 = findRadicalAxis(circle2, circle3)

    if not radicalAxis12 or not radicalAxis23:
        # No radical center could be calculated.
        return None

    intersect = findIntersection(radicalAxis12, radicalAxis23, True, True)

    if intersect:
        return intersect
    else:
        # No radical center could be calculated.
        return None

## @}
