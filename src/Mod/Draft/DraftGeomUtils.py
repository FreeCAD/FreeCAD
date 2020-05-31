# ***************************************************************************
# *   Copyright (c) 2009, 2010 Yorik van Havre <yorik@uncreated.net>        *
# *   Copyright (c) 2009, 2010 Ken Cline <cline@frii.com>                   *
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
"""Define geometry functions for manipulating shapes in the Draft Workbench.

These functions are used by different object creation functions
of the Draft Workbench, both in `Draft.py` and `DraftTools.py`.
They operate on the internal shapes (`Part::TopoShape`) of different objects
and on their subelements, that is, vertices, edges, and faces.
"""
## \defgroup DRAFTGEOMUTILS DraftGeomUtils
#  \ingroup UTILITIES
#  \brief Shape manipulation utilities for the Draft workbench
#
# Shapes manipulation utilities

## \addtogroup DRAFTGEOMUTILS
#  @{
import cmath
import math

import FreeCAD
import Part
import DraftVecUtils
from FreeCAD import Vector

__title__ = "FreeCAD Draft Workbench - Geometry library"
__author__ = "Yorik van Havre, Jacques-Antoine Gaudin, Ken Cline"
__url__ = ["https://www.freecadweb.org"]

from draftgeoutils.general import PARAMGRP as params

from draftgeoutils.general import NORM

# Generic functions *********************************************************


from draftgeoutils.general import precision


from draftgeoutils.general import vec


from draftgeoutils.general import edg


from draftgeoutils.general import getVerts


from draftgeoutils.general import v1


from draftgeoutils.general import isNull


from draftgeoutils.general import isPtOnEdge


from draftgeoutils.general import hasCurves


from draftgeoutils.general import isAligned


from draftgeoutils.general import getQuad


from draftgeoutils.general import areColinear


from draftgeoutils.general import hasOnlyWires


from draftgeoutils.general import geomType


from draftgeoutils.general import isValidPath


# edge functions *************************************************************


from draftgeoutils.edges import findEdge


from draftgeoutils.intersections import findIntersection


from draftgeoutils.intersections import wiresIntersect


from draftgeoutils.offsets import pocket2d


from draftgeoutils.edges import orientEdge


from draftgeoutils.geometry import mirror


from draftgeoutils.arcs import isClockwise


from draftgeoutils.edges import isSameLine


from draftgeoutils.arcs import isWideAngle


from draftgeoutils.general import findClosest


from draftgeoutils.faces import concatenate


from draftgeoutils.faces import getBoundary


from draftgeoutils.edges import isLine


from draftgeoutils.sort_edges import sortEdges


from draftgeoutils.sort_edges import sortEdgesOld


from draftgeoutils.edges import invert


from draftgeoutils.wires import flattenWire


from draftgeoutils.wires import findWires


from draftgeoutils.wires import findWiresOld2


from draftgeoutils.wires import superWire


from draftgeoutils.edges import findMidpoint


from draftgeoutils.geometry import findPerpendicular


from draftgeoutils.offsets import offset


from draftgeoutils.wires import isReallyClosed


from draftgeoutils.geometry import getSplineNormal


from draftgeoutils.geometry import getNormal


from draftgeoutils.geometry import getRotation


from draftgeoutils.geometry import calculatePlacement


from draftgeoutils.offsets import offsetWire


from draftgeoutils.intersections import connect


from draftgeoutils.geometry import findDistance


from draftgeoutils.intersections import angleBisection


def findClosestCircle(point, circles):
    """Return the circle with closest center."""
    dist = 1000000
    closest = None
    for c in circles:
        if c.Center.sub(point).Length < dist:
            dist = c.Center.sub(point).Length
            closest = c
    return closest


from draftgeoutils.faces import isCoplanar


from draftgeoutils.geometry import isPlanar


from draftgeoutils.wires import findWiresOld


from draftgeoutils.edges import getTangent


from draftgeoutils.faces import bind


from draftgeoutils.faces import cleanFaces


from draftgeoutils.cuboids import isCubic


from draftgeoutils.cuboids import getCubicDimensions


from draftgeoutils.wires import removeInterVertices


def arcFromSpline(edge):
        """arcFromSpline(edge): turns the given edge into an arc, by taking
        its first point, midpoint and endpoint. Works best with bspline
        segments such as those from imported svg files. Use this only
        if you are sure your edge is really an arc..."""
        if geomType(edge) == "Line":
                print("This edge is straight, cannot build an arc on it")
                return None
        if len(edge.Vertexes) > 1:
                # 2-point arc
                p1 = edge.Vertexes[0].Point
                p2 = edge.Vertexes[-1].Point
                ml = edge.Length/2
                p3 = edge.valueAt(ml)
                try:
                        return Part.Arc(p1,p3,p2).toShape()
                except:
                        print("Couldn't make an arc out of this edge")
                        return None
        else:
                # circle
                p1 = edge.Vertexes[0].Point
                ml = edge.Length/2
                p2 = edge.valueAt(ml)
                ray = p2.sub(p1)
                ray.scale(.5,.5,.5)
                center = p1.add(ray)
                radius = ray.Length
                try:
                        return Part.makeCircle(radius,center)
                except:
                        print("couldn't make a circle out of this edge")


from draftgeoutils.fillets import fillet


from draftgeoutils.fillets import filletWire


def getCircleFromSpline(edge):
    """Return a circle-based edge from a bspline-based edge."""
    if geomType(edge) != "BSplineCurve":
        return None
    if len(edge.Vertexes) != 1:
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
    r1 = DraftVecUtils.rotate(t1,math.pi/2,n)
    r2 = DraftVecUtils.rotate(t2,math.pi/2,n)
    # get center (intersection of rays)
    i = findIntersection(p1,p1.add(r1),p2,p2.add(r2),True,True)
    if not i:
        return None
    c = i[0]
    r = (p1.sub(c)).Length
    circle = Part.makeCircle(r,c,n)
    #print(circle.Curve)
    return circle


from draftgeoutils.wires import curvetowire


from draftgeoutils.wires import cleanProjection


from draftgeoutils.wires import curvetosegment


from draftgeoutils.wires import tessellateProjection


from draftgeoutils.wires import rebaseWire


from draftgeoutils.faces import removeSplitter


# circle functions *********************************************************


def getBoundaryAngles(angle, alist):
        """returns the 2 closest angles from the list that
        encompass the given angle"""
        negs = True
        while negs:
                negs = False
                for i in range(len(alist)):
                        if alist[i] < 0:
                                alist[i] = 2*math.pi + alist[i]
                                negs = True
                if angle < 0:
                        angle = 2*math.pi + angle
                        negs = True
        lower = None
        for a in alist:
                if a < angle:
                        if lower is None:
                                lower = a
                        else:
                                if a > lower:
                                        lower = a
        if lower is None:
                lower = 0
                for a in alist:
                        if a > lower:
                                lower = a
        higher = None
        for a in alist:
                if a > angle:
                        if higher is None:
                                higher = a
                        else:
                                if a < higher:
                                        higher = a
        if higher is None:
                higher = 2*math.pi
                for a in alist:
                        if a < higher:
                                higher = a
        return (lower,higher)


def circleFrom2tan1pt(tan1, tan2, point):
    """circleFrom2tan1pt(edge, edge, Vector)"""
    if (geomType(tan1) == "Line") and (geomType(tan2) == "Line") and isinstance(point, FreeCAD.Vector):
        return circlefrom2Lines1Point(tan1, tan2, point)
    elif (geomType(tan1) == "Circle") and (geomType(tan2) == "Line") and isinstance(point, FreeCAD.Vector):
        return circlefromCircleLinePoint(tan1, tan2, point)
    elif (geomType(tan2) == "Circle") and (geomType(tan1) == "Line") and isinstance(point, FreeCAD.Vector):
        return circlefromCircleLinePoint(tan2, tan1, point)
    elif (geomType(tan2) == "Circle") and (geomType(tan1) == "Circle") and isinstance(point, FreeCAD.Vector):
        return circlefrom2Circles1Point(tan2, tan1, point)


def circleFrom2tan1rad(tan1, tan2, rad):
    """circleFrom2tan1rad(edge, edge, float)"""
    if (geomType(tan1) == "Line") and (geomType(tan2) == "Line"):
        return circleFrom2LinesRadius(tan1, tan2, rad)
    elif (geomType(tan1) == "Circle") and (geomType(tan2) == "Line"):
        return circleFromCircleLineRadius(tan1, tan2, rad)
    elif (geomType(tan1) == "Line") and (geomType(tan2) == "Circle"):
        return circleFromCircleLineRadius(tan2, tan1, rad)
    elif (geomType(tan1) == "Circle") and (geomType(tan2) == "Circle"):
        return circleFrom2CirclesRadius(tan1, tan2, rad)


def circleFrom1tan2pt(tan1, p1, p2):
    if (geomType(tan1) == "Line") and isinstance(p1, FreeCAD.Vector) and isinstance(p2, FreeCAD.Vector):
        return circlefrom1Line2Points(tan1, p1, p2)
    if (geomType(tan1) == "Line") and isinstance(p1, FreeCAD.Vector) and isinstance(p2, FreeCAD.Vector):
        return circlefrom1Circle2Points(tan1, p1, p2)


def circleFrom1tan1pt1rad(tan1, p1, rad):
    if (geomType(tan1) == "Line") and isinstance(p1, FreeCAD.Vector):
        return circleFromPointLineRadius(p1, tan1, rad)
    if (geomType(tan1) == "Circle") and isinstance(p1, FreeCAD.Vector):
        return circleFromPointCircleRadius(p1, tan1, rad)


def circleFrom3tan(tan1, tan2, tan3):
    tan1IsLine = (geomType(tan1) == "Line")
    tan2IsLine = (geomType(tan2) == "Line")
    tan3IsLine = (geomType(tan3) == "Line")
    tan1IsCircle = (geomType(tan1) == "Circle")
    tan2IsCircle = (geomType(tan2) == "Circle")
    tan3IsCircle = (geomType(tan3) == "Circle")
    if tan1IsLine and tan2IsLine and tan3IsLine:
        return circleFrom3LineTangents(tan1, tan2, tan3)
    elif tan1IsCircle and tan2IsCircle and tan3IsCircle:
        return circleFrom3CircleTangents(tan1, tan2, tan3)
    elif (tan1IsCircle and tan2IsLine and tan3IsLine):
        return circleFrom1Circle2Lines(tan1, tan2, tan3)
    elif (tan1IsLine and tan2IsCircle and tan3IsLine):
        return circleFrom1Circle2Lines(tan2, tan1, tan3)
    elif (tan1IsLine and tan2IsLine and tan3IsCircle):
        return circleFrom1Circle2Lines(tan3, tan1, tan2)
    elif (tan1IsLine and tan2IsCircle and tan3IsCircle):
        return circleFrom2Circle1Lines(tan2, tan3, tan1)
    elif (tan1IsCircle and tan2IsLine and tan3IsCircle):
        return circleFrom2Circle1Lines(tan1, tan3, tan2)
    elif (tan1IsCircle and tan2IsCircle and tan3IsLine):
        return circleFrom2Circle1Lines(tan1, tan2, tan3)


def circlefrom2Lines1Point(edge1, edge2, point):
    """circlefrom2Lines1Point(edge, edge, Vector)"""
    bis = angleBisection(edge1, edge2)
    if not bis: return None
    mirrPoint = mirror(point, bis)
    return circlefrom1Line2Points(edge1, point, mirrPoint)


def circlefrom1Line2Points(edge, p1, p2):
    """circlefrom1Line2Points(edge, Vector, Vector)"""
    p1_p2 = edg(p1, p2)
    s = findIntersection(edge, p1_p2, True, True)
    if not s: return None
    s = s[0]
    v1 = p1.sub(s)
    v2 = p2.sub(s)
    projectedDist = math.sqrt(abs(v1.dot(v2)))
    edgeDir = vec(edge); edgeDir.normalize()
    projectedCen1 = Vector.add(s, Vector(edgeDir).multiply(projectedDist))
    projectedCen2 = Vector.add(s, Vector(edgeDir).multiply(-projectedDist))
    perpEdgeDir = edgeDir.cross(Vector(0,0,1))
    perpCen1 = Vector.add(projectedCen1, perpEdgeDir)
    perpCen2 = Vector.add(projectedCen2, perpEdgeDir)
    mid = findMidpoint(p1_p2)
    x = DraftVecUtils.crossproduct(vec(p1_p2)); x.normalize()
    perp_mid = Vector.add(mid, x)
    cen1 = findIntersection(edg(projectedCen1, perpCen1), edg(mid, perp_mid), True, True)
    cen2 = findIntersection(edg(projectedCen2, perpCen2), edg(mid, perp_mid), True, True)
    circles = []
    if cen1:
        radius = DraftVecUtils.dist(projectedCen1, cen1[0])
        circles.append(Part.Circle(cen1[0], NORM, radius))
    if cen2:
        radius = DraftVecUtils.dist(projectedCen2, cen2[0])
        circles.append(Part.Circle(cen2[0], NORM, radius))

    if circles: return circles
    else: return None


def circleFrom2LinesRadius(edge1, edge2, radius):
    """circleFrom2LinesRadius(edge,edge,radius)"""
    int = findIntersection(edge1, edge2, True, True)
    if not int: return None
    int = int[0]
    bis12 = angleBisection(edge1,edge2)
    bis21 = Part.LineSegment(bis12.Vertexes[0].Point,DraftVecUtils.rotate(vec(bis12), math.pi/2.0))
    ang12 = abs(DraftVecUtils.angle(vec(edge1),vec(edge2)))
    ang21 = math.pi - ang12
    dist12 = radius / math.sin(ang12 * 0.5)
    dist21 = radius / math.sin(ang21 * 0.5)
    circles = []
    cen = Vector.add(int, vec(bis12).multiply(dist12))
    circles.append(Part.Circle(cen, NORM, radius))
    cen = Vector.add(int, vec(bis12).multiply(-dist12))
    circles.append(Part.Circle(cen, NORM, radius))
    cen = Vector.add(int, vec(bis21).multiply(dist21))
    circles.append(Part.Circle(cen, NORM, radius))
    cen = Vector.add(int, vec(bis21).multiply(-dist21))
    circles.append(Part.Circle(cen, NORM, radius))
    return circles


def circleFrom3LineTangents(edge1, edge2, edge3):
    """circleFrom3LineTangents(edge,edge,edge)"""
    def rot(ed):
        return Part.LineSegment(v1(ed),v1(ed).add(DraftVecUtils.rotate(vec(ed),math.pi/2))).toShape()
    bis12 = angleBisection(edge1,edge2)
    bis23 = angleBisection(edge2,edge3)
    bis31 = angleBisection(edge3,edge1)
    intersections = []
    int = findIntersection(bis12, bis23, True, True)
    if int:
        radius = findDistance(int[0],edge1).Length
        intersections.append(Part.Circle(int[0],NORM,radius))
    int = findIntersection(bis23, bis31, True, True)
    if int:
        radius = findDistance(int[0],edge1).Length
        intersections.append(Part.Circle(int[0],NORM,radius))
    int = findIntersection(bis31, bis12, True, True)
    if int:
        radius = findDistance(int[0],edge1).Length
        intersections.append(Part.Circle(int[0],NORM,radius))
    int = findIntersection(rot(bis12), rot(bis23), True, True)
    if int:
        radius = findDistance(int[0],edge1).Length
        intersections.append(Part.Circle(int[0],NORM,radius))
    int = findIntersection(rot(bis23), rot(bis31), True, True)
    if int:
        radius = findDistance(int[0],edge1).Length
        intersections.append(Part.Circle(int[0],NORM,radius))
    int = findIntersection(rot(bis31), rot(bis12), True, True)
    if int:
        radius = findDistance(int[0],edge1).Length
        intersections.append(Part.Circle(int[0],NORM,radius))
    circles = []
    for int in intersections:
        exists = False
        for cir in circles:
            if DraftVecUtils.equals(cir.Center, int.Center):
                exists = True
                break
        if not exists:
            circles.append(int)
    if circles:
        return circles
    else:
        return None

def circleFromPointLineRadius(point, edge, radius):
    """circleFromPointLineRadius (point, edge, radius)"""
    dist = findDistance(point, edge, False)
    center1 = None
    center2 = None
    if dist.Length == 0:
        segment = vec(edge)
        perpVec = DraftVecUtils.crossproduct(segment); perpVec.normalize()
        normPoint_c1 = Vector(perpVec).multiply(radius)
        normPoint_c2 = Vector(perpVec).multiply(-radius)
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
        edgeDir = edge.Vertexes[0].Point.sub(normPoint); edgeDir.normalize()
        center1 = centerNormVec.add(normPoint.add(Vector(edgeDir).multiply(dist)))
        center2 = centerNormVec.add(normPoint.add(Vector(edgeDir).multiply(-dist)))
    circles = []
    if center1:
        circ = Part.Circle(center1, NORM, radius)
        if circ:
            circles.append(circ)
    if center2:
        circ = Part.Circle(center2, NORM, radius)
        if circ:
            circles.append(circ)

    if len(circles):
        return circles
    else:
        return None


def circleFrom2PointsRadius(p1, p2, radius):
    """circleFrom2PointsRadiust(Vector, Vector, radius)"""
    if DraftVecUtils.equals(p1, p2): return None

    p1_p2 = Part.LineSegment(p1, p2).toShape()
    dist_p1p2 = DraftVecUtils.dist(p1, p1)
    mid = findMidpoint(p1_p2)
    if dist_p1p2 == 2*radius:
        circle = Part.Circle(mid, NORM, radius)
        if circle: return [circle]
        else: return None
    dir = vec(p1_p2); dir.normalize()
    perpDir = dir.cross(Vector(0,0,1)); perpDir.normalize()
    dist = math.sqrt(radius**2 - (dist_p1p2 / 2.0)**2)
    cen1 = Vector.add(mid, Vector(perpDir).multiply(dist))
    cen2 = Vector.add(mid, Vector(perpDir).multiply(-dist))
    circles = []
    if cen1: circles.append(Part.Circle(cen1, NORM, radius))
    if cen2: circles.append(Part.Circle(cen2, NORM, radius))
    if circles: return circles
    else: return None


from draftgeoutils.arcs import arcFrom2Pts


#############################33 to include







def outerSoddyCircle(circle1, circle2, circle3):
    """Compute the outer soddy circle for three tightly packed circles."""
    if (geomType(circle1) == "Circle") and (geomType(circle2) == "Circle") \
    and (geomType(circle3) == "Circle"):
        # Original Java code Copyright (rc) 2008 Werner Randelshofer
        # Converted to python by Martin Buerbaum 2009
        # http://www.randelshofer.ch/treeviz/
        # Either Creative Commons Attribution 3.0, the MIT license, or the GNU Lesser General License LGPL.

        A = circle1.Curve.Center
        B = circle2.Curve.Center
        C = circle3.Curve.Center

        ra = circle1.Curve.Radius
        rb = circle2.Curve.Radius
        rc = circle3.Curve.Radius

        # Solution using Descartes' theorem, as described here:
        # http://en.wikipedia.org/wiki/Descartes%27_theorem
        k1 = 1 / ra
        k2 = 1 / rb
        k3 = 1 / rc
        k4 = abs(k1 + k2 + k3 - 2 * math.sqrt(k1 * k2 + k2 * k3 + k3 * k1))

        q1 = (k1 + 0j) * (A.x + A.y * 1j)
        q2 = (k2 + 0j) * (B.x + B.y * 1j)
        q3 = (k3 + 0j) * (C.x + C.y * 1j)

        temp = ((q1 * q2) + (q2 * q3) + (q3 * q1))
        q4 = q1 + q2 + q3 - ((2 + 0j) * cmath.sqrt(temp) )

        z = q4 / (k4 + 0j)

        # If the formula is not solvable, we return no circle.
        if (not z or not (1 / k4)):
            return None

        X = -z.real
        Y = -z.imag
        print("Outer Soddy circle: " + str(X) + " " + str(Y) + "\n") # Debug

        # The Radius of the outer soddy circle can also be calculated with the following formula:
        # radiusOuter = abs(r1*r2*r3 / (r1*r2 + r1*r3 + r2*r3 - 2 * math.sqrt(r1*r2*r3 * (r1+r2+r3))))
        circ = Part.Circle(Vector(X, Y, A.z), norm, 1 / k4)
        return circ

    else:
        print("debug: outerSoddyCircle bad parameters!\n")
        # FreeCAD.Console.PrintMessage("debug: outerSoddyCircle bad parameters!\n")
        return None


def innerSoddyCircle(circle1, circle2, circle3):
    """Compute the inner soddy circle for three tightly packed circles."""
    if (geomType(circle1) == "Circle") and (geomType(circle2) == "Circle") \
    and (geomType(circle3) == "Circle"):
        # Original Java code Copyright (rc) 2008 Werner Randelshofer
        # Converted to python by Martin Buerbaum 2009
        # http://www.randelshofer.ch/treeviz/

        A = circle1.Curve.Center
        B = circle2.Curve.Center
        C = circle3.Curve.Center

        ra = circle1.Curve.Radius
        rb = circle2.Curve.Radius
        rc = circle3.Curve.Radius

        # Solution using Descartes' theorem, as described here:
        # http://en.wikipedia.org/wiki/Descartes%27_theorem
        k1 = 1 / ra
        k2 = 1 / rb
        k3 = 1 / rc
        k4 = abs(k1 + k2 + k3 + 2 * math.sqrt(k1 * k2 + k2 * k3 + k3 * k1))

        q1 = (k1 + 0j) * (A.x + A.y * 1j)
        q2 = (k2 + 0j) * (B.x + B.y * 1j)
        q3 = (k3 + 0j) * (C.x + C.y * 1j)

        temp = ((q1 * q2) + (q2 * q3) + (q3 * q1))
        q4 = q1 + q2 + q3 + ((2 + 0j) * cmath.sqrt(temp) )

        z = q4 / (k4 + 0j)

        # If the formula is not solvable, we return no circle.
        if (not z or not (1 / k4)):
            return None

        X = z.real
        Y = z.imag
        print("Outer Soddy circle: " + str(X) + " " + str(Y) + "\n") # Debug

        # The Radius of the inner soddy circle can also be calculated with the following formula:
        # radiusInner = abs(r1*r2*r3 / (r1*r2 + r1*r3 + r2*r3 + 2 * math.sqrt(r1*r2*r3 * (r1+r2+r3))))
        circ = Part.Circle(Vector(X, Y, A.z), norm, 1 / k4)
        return circ

    else:
        print("debug: innerSoddyCircle bad parameters!\n")
        # FreeCAD.Console.PrintMessage("debug: innerSoddyCircle bad parameters!\n")
        return None


def circleFrom3CircleTangents(circle1, circle2, circle3):
    """
    http://en.wikipedia.org/wiki/Problem_of_Apollonius#Inversive_methods
    http://mathworld.wolfram.com/ApolloniusCircle.html
    http://mathworld.wolfram.com/ApolloniusProblem.html
    """

    if (geomType(circle1) == "Circle") and (geomType(circle2) == "Circle") \
    and (geomType(circle3) == "Circle"):
        int12 = findIntersection(circle1, circle2, True, True)
        int23 = findIntersection(circle2, circle3, True, True)
        int31 = findIntersection(circle3, circle1, True, True)

        if int12 and int23 and int31:
            if len(int12) == 1 and len(int23) == 1 and len(int31) == 1:
                # Only one intersection with each circle.
                # => "Soddy Circle" - 2 solutions.
                # http://en.wikipedia.org/wiki/Problem_of_Apollonius#Mutually_tangent_given_circles:_Soddy.27s_circles_and_Descartes.27_theorem
                # http://mathworld.wolfram.com/SoddyCircles.html
                # http://mathworld.wolfram.com/InnerSoddyCenter.html
                # http://mathworld.wolfram.com/OuterSoddyCenter.html

                r1 = circle1.Curve.Radius
                r2 = circle2.Curve.Radius
                r3 = circle3.Curve.Radius
                outerSoddy = outerSoddyCircle(circle1, circle2, circle3)
#               print(str(outerSoddy) + "\n") # Debug

                innerSoddy = innerSoddyCircle(circle1, circle2, circle3)
#               print(str(innerSoddy) + "\n") # Debug

                circles = []
                if outerSoddy:
                    circles.append(outerSoddy)
                if innerSoddy:
                    circles.append(innerSoddy)
                return circles

            # @todo Calc all 6 homothetic centers.
            # @todo Create 3 lines from the inner and 4 from the outer h. center.
            # @todo Calc. the 4 inversion poles of these lines for each circle.
            # @todo Calc. the radical center of the 3 circles.
            # @todo Calc. the intersection points (max. 8) of 4 lines (through each inversion pole and the radical center) with the circle.
            #       This gives us all the tangent points.
        else:
            # Some circles are inside each other or an error has occurred.
            return None

    else:
        print("debug: circleFrom3CircleTangents bad parameters!\n")
        # FreeCAD.Console.PrintMessage("debug: circleFrom3CircleTangents bad parameters!\n")
        return None


from draftgeoutils.linear_algebra import linearFromPoints


from draftgeoutils.linear_algebra import determinant


def findHomotheticCenterOfCircles(circle1, circle2):
    """Calculate the homothetic center(s) of two circles.

    http://en.wikipedia.org/wiki/Homothetic_center
    http://mathworld.wolfram.com/HomotheticCenter.html
    """

    if (geomType(circle1) == "Circle") and (geomType(circle2) == "Circle"):
        if DraftVecUtils.equals(circle1.Curve.Center, circle2.Curve.Center):
            return None

        cen1_cen2 = Part.LineSegment(circle1.Curve.Center, circle2.Curve.Center).toShape()
        cenDir = vec(cen1_cen2); cenDir.normalize()

        # Get the perpedicular vector.
        perpCenDir = cenDir.cross(Vector(0,0,1)); perpCenDir.normalize()

        # Get point on first circle
        p1 = Vector.add(circle1.Curve.Center, Vector(perpCenDir).multiply(circle1.Curve.Radius))

        centers = []
        # Calculate inner homothetic center
        # Get point on second circle
        p2_inner = Vector.add(circle1.Curve.Center, Vector(perpCenDir).multiply(-circle1.Curve.Radius))
        hCenterInner = DraftVecUtils.intersect(circle1.Curve.Center, circle2.Curve.Center, p1, p2_inner, True, True)
        if hCenterInner:
            centers.append(hCenterInner)

        # Calculate outer homothetic center (only exists of the circles have different radii)
        if circle1.Curve.Radius != circle2.Curve.Radius:
            # Get point on second circle
            p2_outer = Vector.add(circle1.Curve.Center, Vector(perpCenDir).multiply(circle1.Curve.Radius))
            hCenterOuter = DraftVecUtils.intersect(circle1.Curve.Center, circle2.Curve.Center, p1, p2_outer, True, True)
            if hCenterOuter:
                centers.append(hCenterOuter)

        if len(centers):
            return centers
        else:
            return None

    else:
        FreeCAD.Console.PrintMessage("debug: findHomotheticCenterOfCirclescleFrom3tan bad parameters!\n")
        return None


def findRadicalAxis(circle1, circle2):
    """Calculate the radical axis of two circles.

    On the radical axis (also called power line) of two circles any
    tangents drawn from a point on the axis to both circles have the same length.

    http://en.wikipedia.org/wiki/Radical_axis
    http://mathworld.wolfram.com/RadicalLine.html

    @sa findRadicalCenter
    """
    if (geomType(circle1) == "Circle") and (geomType(circle2) == "Circle"):
        if DraftVecUtils.equals(circle1.Curve.Center, circle2.Curve.Center):
            return None
        r1 = circle1.Curve.Radius
        r2 = circle1.Curve.Radius
        cen1 = circle1.Curve.Center
        # dist .. the distance from cen1 to cen2.
        dist = DraftVecUtils.dist(cen1, circle2.Curve.Center)
        cenDir = cen1.sub(circle2.Curve.Center); cenDir.normalize()

        # Get the perpedicular vector.
        perpCenDir = cenDir.cross(Vector(0,0,1)); perpCenDir.normalize()

        # J ... The radical center.
        # K ... The point where the cadical axis crosses the line of cen1->cen2.
        # k1 ... Distance from cen1 to K.
        # k2 ... Distance from cen2 to K.
        # dist = k1 + k2

        k1 = (dist + (r1^2 - r2^2) / dist) / 2.0
        #k2 = dist - k1

        K = Vector.add(cen1, cenDir.multiply(k1))

        # K_ .. A point somewhere between K and J (actually with a distance of 1 unit from K).
        K_ = Vector,add(K, perpCenDir)

        radicalAxis = Part.LineSegment(K, Vector.add(origin, dir))

        if radicalAxis:
            return radicalAxis
        else:
            return None
    else:
        FreeCAD.Console.PrintMessage("debug: findRadicalAxis bad parameters!\n")
        return None


def findRadicalCenter(circle1, circle2, circle3):
    """
    findRadicalCenter(circle1, circle2, circle3):
    Calculates the radical center (also called the power center) of three circles.
    It is the intersection point of the three radical axes of the pairs of circles.

    http://en.wikipedia.org/wiki/Power_center_(geometry)
    http://mathworld.wolfram.com/RadicalCenter.html

    @sa findRadicalAxis
    """
    if (geomType(circle1) == "Circle") and (geomType(circle2) == "Circle"):
        radicalAxis12 = findRadicalAxis(circle1, circle2)
        radicalAxis23 = findRadicalAxis(circle1, circle2)

        if not radicalAxis12 or not radicalAxis23:
            # No radical center could be calculated.
            return None

        int = findIntersection(radicalAxis12, radicalAxis23, True, True)

        if int:
            return int
        else:
            # No radical center could be calculated.
            return None
    else:
        FreeCAD.Console.PrintMessage("debug: findRadicalCenter bad parameters!\n")
        return None


def pointInversion(circle, point):
    """Circle inversion of a point.

    pointInversion(Circle, Vector)

    Will calculate the inversed point an return it.
    If the given point is equal to the center of the circle "None" will be returned.

    See also:
    http://en.wikipedia.org/wiki/Inversive_geometry
    """
    if (geomType(circle) == "Circle") and isinstance(point, FreeCAD.Vector):
        cen = circle.Curve.Center
        rad = circle.Curve.Radius

        if DraftVecUtils.equals(cen, point):
            return None

        # Inverse the distance of the point
        # dist(cen -> P) = r^2 / dist(cen -> invP)

        dist = DraftVecUtils.dist(point, cen)
        invDist = rad**2 / d

        invPoint = Vector(0, 0, point.z)
        invPoint.x = cen.x + (point.x - cen.x) * invDist / dist;
        invPoint.y = cen.y + (point.y - cen.y) * invDist / dist;

        return invPoint

    else:
        FreeCAD.Console.PrintMessage("debug: pointInversion bad parameters!\n")
        return None


def polarInversion(circle, edge):
    """Return the inversion pole of a line.

    polarInversion(circle, edge):

    edge ... The polar.
    i.e. The nearest point on the line is inversed.

    http://mathworld.wolfram.com/InversionPole.html
    """

    if (geomType(circle) == "Circle") and (geomType(edge) == "Line"):
        nearest = circle.Curve.Center.add(findDistance(circle.Curve.Center, edge, False))
        if nearest:
            inversionPole = pointInversion(circle, nearest)
            if inversionPole:
                return inversionPole
    else:
        FreeCAD.Console.PrintMessage("debug: circleInversionPole bad parameters!\n")
        return None


def circleInversion(circle, circle2):
    """
    pointInversion(Circle, Circle)

    Circle inversion of a circle.
    """
    if (geomType(circle) == "Circle") and (geomType(circle2) == "Circle"):
        cen1 = circle.Curve.Center
        rad1 = circle.Curve.Radius

        if DraftVecUtils.equals(cen1, point):
            return None

        invCen2 = Inversion(circle, circle2.Curve.Center)

        pointOnCircle2 = Vector.add(circle2.Curve.Center, Vector(circle2.Curve.Radius, 0, 0))
        invPointOnCircle2 = Inversion(circle, pointOnCircle2)

        return Part.Circle(invCen2, norm, DraftVecUtils.dist(invCen2, invPointOnCircle2))

    else:
        FreeCAD.Console.PrintMessage("debug: circleInversion bad parameters!\n")
        return None

##  @}
